#include "HearthwardLoadingSubsystem.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MoviePlayer.h"
#include "UObject/UObjectGlobals.h"
#include "WorldPartition/WorldPartitionSubsystem.h"
#include "ContentStreaming.h"
#include "Misc/Paths.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

class SHearthwardLoadingScreen : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHearthwardLoadingScreen) {} SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        const FString Root=FPaths::ProjectDir()/TEXT("Resources/UI");
        const FName Image(*(Root/TEXT("Art/title-scene.png")));
        const FVector2D Size=FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource(Image);
        Background=MakeShared<FSlateDynamicImageBrush>(Image,Size);
        Font=MakeShared<FCompositeFont>(NAME_None,Root/TEXT("Fonts/LXGWWenKai-Regular.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
        SetCanTick(false);
    }
    float Opacity=1;
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920,1080); }
    virtual int32 OnPaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override
    {
        const FVector2D View=G.GetLocalSize();
        const float Scale=FMath::Min(View.X/1920.f,View.Y/1080.f);
        const FSlateBrush* White=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        auto Box=[&](FVector2D P,FVector2D S,FLinearColor C,int32 L)
        {C.A*=Opacity;FSlateDrawElement::MakeBox(Out,L,G.ToPaintGeometry(S,FSlateLayoutTransform(P)),White,ESlateDrawEffect::None,C);};
        FSlateBrush Cover=*Background;
        const float Aspect=Background->ImageSize.X/Background->ImageSize.Y;
        if(View.X/View.Y>Aspect)
        {const float Crop=(1-Aspect*View.Y/View.X)*.5f;Cover.SetUVRegion(FBox2f(FVector2f(0,Crop),FVector2f(1,1-Crop)));}
        else
        {const float Crop=(1-View.X/View.Y/Aspect)*.5f;Cover.SetUVRegion(FBox2f(FVector2f(Crop,0),FVector2f(1-Crop,1)));}
        FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(View,FSlateLayoutTransform()),&Cover,ESlateDrawEffect::None,FLinearColor(1,1,1,Opacity));
        Box(FVector2D::ZeroVector,View,FLinearColor(0.008f,.012f,.018f,.42f),Layer+1);
        // A solid footer keeps the text legible across all parts of the existing title artwork.
        Box(FVector2D(0,View.Y-205*Scale),FVector2D(View.X,205*Scale),FLinearColor(.009f,.013f,.019f,.96f),Layer+2);
        const FLinearColor Ink(.84f,.76f,.59f,Opacity);
        auto Text=[&](const TCHAR* Value,FVector2D P,int32 Size)
        {FSlateDrawElement::MakeText(Out,Layer+3,G.ToPaintGeometry(FVector2D(1000,60),FSlateLayoutTransform(Scale,P)),Value,FSlateFontInfo(Font,Size),ESlateDrawEffect::None,Ink);};
        Text(TEXT("炉火未熄"),FVector2D(80*Scale,View.Y-174*Scale),30);
        Text(TEXT("正在准备世界"),FVector2D(80*Scale,View.Y-117*Scale),20);
        Text(TEXT("与弟弟同行，重建属于你们的家园。"),FVector2D(80*Scale,View.Y-65*Scale),16);
        const float Phase=FMath::Frac(FPlatformTime::Seconds()*.65);
        const FVector2D Bar(View.X-380*Scale,View.Y-81*Scale);
        Box(Bar,FVector2D(300*Scale,2*Scale),FLinearColor(.3f,.3f,.27f,1),Layer+3);
        Box(Bar+FVector2D(Phase*220*Scale,0),FVector2D(80*Scale,2*Scale),Ink,Layer+4);
        return Layer+4;
    }
private:
    TSharedPtr<FSlateDynamicImageBrush> Background;
    TSharedPtr<FCompositeFont> Font;
};

void UHearthwardLoadingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    BeforeHandle=FCoreUObjectDelegates::PreLoadMap.AddUObject(this,&UHearthwardLoadingSubsystem::BeforeMap);
    AfterHandle=FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this,&UHearthwardLoadingSubsystem::AfterMap);
    TickHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UHearthwardLoadingSubsystem::Tick));
}
void UHearthwardLoadingSubsystem::Deinitialize()
{
    Hide();
    FCoreUObjectDelegates::PreLoadMap.Remove(BeforeHandle);
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(AfterHandle);
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    Super::Deinitialize();
}
void UHearthwardLoadingSubsystem::BeginLoading()
{
    if(IsRunningDedicatedServer() || !FSlateApplication::IsInitialized())return;
    Loading=true;AwaitingSession=true;ReadySince=0;StartedAt=FPlatformTime::Seconds();
    if(!Screen)Screen=SNew(SHearthwardLoadingScreen);
    Screen->Opacity=1;
    if(auto* V=GetGameInstance()->GetGameViewportClient();V && Viewport!=V)
    {
        if(Viewport.IsValid()){Viewport->RemoveViewportWidgetContent(Screen.ToSharedRef());Viewport->SetIgnoreInput(PreviousIgnoreInput);}
        PreviousIgnoreInput=V->IgnoreInput();V->SetIgnoreInput(true);
        V->AddViewportWidgetContent(Screen.ToSharedRef(),10000);Viewport=V;
    }
    UE_LOG(LogTemp,Log,TEXT("Hearthward loading begin"));
}
void UHearthwardLoadingSubsystem::BeforeMap(const FString& Map)
{
    // PIE instances also receive global delegates belonging to other worlds.
    if(GetWorld() && GetWorld()->WorldType==EWorldType::PIE)return;
    BeginLoading();
    if(IsMoviePlayerEnabled() && Screen)
    {
        FLoadingScreenAttributes Attributes;
        Attributes.WidgetLoadingScreen=SNew(SHearthwardLoadingScreen);
        Attributes.bAutoCompleteWhenLoadingCompletes=true;
        Attributes.bMoviesAreSkippable=false;
        GetMoviePlayer()->SetupLoadingScreen(Attributes);
    }
}
void UHearthwardLoadingSubsystem::AfterMap(UWorld* World)
{
    if(!World || World->GetGameInstance()!=GetGameInstance())return;
    if(!World->URL.HasOption(TEXT("HearthwardLoad=")) && !World->URL.HasOption(TEXT("HearthwardNewGame=")))AwaitingSession=false;
}
void UHearthwardLoadingSubsystem::FinishSession(bool Success)
{
    AwaitingSession=false;
    if(!Success)Hide();
}
bool UHearthwardLoadingSubsystem::Tick(float Delta)
{
    if(!Loading || AwaitingSession)return true;
    auto* World=GetWorld();
    if(!World || !World->HasBegunPlay())return true;
    auto* Controller=GetGameInstance()->GetFirstLocalPlayerController();
    if(!Controller || !Controller->GetPawn())return true;
    if(!HeldMovement.IsValid())
        if(auto* Movement=Controller->GetPawn()->FindComponentByClass<UCharacterMovementComponent>();Movement && Movement->MovementMode!=MOVE_None)
        {
            HeldMovement=Movement;PreviousMovementMode=Movement->MovementMode;PreviousCustomMode=Movement->CustomMovementMode;
            Movement->StopMovementImmediately();Movement->DisableMovement();
        }
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();
    auto* Partition=World->GetSubsystem<UWorldPartitionSubsystem>();
    FVector Ground;
    const bool Natural=World->GetName().Contains(TEXT("L_HearthwardWilds"));
    const bool Ready=(!Partition || Partition->IsStreamingCompleted())
        && (!Natural || (Campaign && !Campaign->IsPreparingWorld() && Campaign->Ground(Controller->GetPawn()->GetActorLocation(),Ground)))
        && IStreamingManager::Get().GetNumWantingResources()==0;
    if(!Ready){ReadySince=0;Screen->Opacity=1;return true;}
    if(ReadySince==0)ReadySince=FPlatformTime::Seconds();
    // Let the spring arm, scene proxies and temporal history settle before revealing the view.
    const double Elapsed=FPlatformTime::Seconds()-ReadySince;
    if(Elapsed>.35)Screen->Opacity=1-FMath::Clamp(float((Elapsed-.35)/.35),0.f,1.f);
    if(Elapsed>.7)Hide();
    return true;
}
void UHearthwardLoadingSubsystem::Hide()
{
    if(HeldMovement.IsValid() && HeldMovement->MovementMode==MOVE_None)
        HeldMovement->SetMovementMode(EMovementMode(PreviousMovementMode),PreviousCustomMode);
    HeldMovement.Reset();
    if(Viewport.IsValid())
    {if(Screen)Viewport->RemoveViewportWidgetContent(Screen.ToSharedRef());Viewport->SetIgnoreInput(PreviousIgnoreInput);}
    Viewport.Reset();Screen.Reset();
    if(Loading)UE_LOG(LogTemp,Log,TEXT("Hearthward loading end (%.2f s)"),FPlatformTime::Seconds()-StartedAt);
    Loading=false;AwaitingSession=false;
}
