#include "HearthwardPersonalStory.h"
#include "HearthwardCampaignSubsystem.h"
#include "HearthwardCampaignActor.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../UI/HearthwardHUD.h"
#include "../UI/HearthwardScreenWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
using namespace HearthwardData;

const TArray<TSharedPtr<FJsonValue>>& HearthwardPersonalStory::Rows()
{
    static TSharedPtr<FJsonObject> Data=[]
    {
        FString Source;FFileHelper::LoadFileToString(Source,*(FPaths::ProjectDir()/TEXT("Resources/Data/round2_people.json")));
        TSharedPtr<FJsonObject> Result;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),Result);check(Result);return Result;
    }();
    return Data->GetArrayField(TEXT("people"));
}
TSharedPtr<FJsonObject> HearthwardPersonalStory::Row(FName Person)
{for(const auto& V:Rows())if(Text(V->AsObject(),TEXT("id"))==Person.ToString())return V->AsObject();return nullptr;}
FHearthwardPersonChoiceState HearthwardPersonalStory::State(UWorld* World,FName Person)
{
    const auto& People=World->GetSubsystem<UHearthwardSaveSubsystem>()->InteractionState().People;
    if(const auto* S=People.FindByPredicate([&](const auto& V){return V.Id==Person;}))return *S;
    FHearthwardPersonChoiceState Empty;Empty.Id=Person;return Empty;
}
AHearthwardPersonalStoryDirector::AHearthwardPersonalStoryDirector()
{PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.4f;}
AHearthwardPersonalStoryDirector* AHearthwardPersonalStoryDirector::Find(UWorld* World)
{for(TActorIterator<AHearthwardPersonalStoryDirector> It(World);It;++It)return *It;return nullptr;}
bool AHearthwardPersonalStoryDirector::SetStep(FName Person,FName Step,FName Choice)
{
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    if(!Save->BindInteractionTarget(Person,EHearthwardPersistentKind::Person,this))return false;
    const auto Ticket=Save->BeginInteraction(Person);if(!Ticket.Operation.IsValid())return false;
    auto Next=Save->InteractionState();auto* S=Next.People.FindByPredicate([&](const auto& V){return V.Id==Person;});
    if(!S){FHearthwardPersonChoiceState New;New.Id=Person;S=&Next.People.Add_GetRef(New);}S->Step=Step;S->Choice=Choice;
    if(!Save->CommitInteraction(Ticket,Next)){Save->CancelInteraction();return false;}return true;
}
void AHearthwardPersonalStoryDirector::Tick(float Delta)
{
    Super::Tick(Delta);if(GetWorld()->IsPaused())return;
    const auto Current=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    if(Current!=Epoch){Epoch=Current;ObservationSeconds.Reset();}
    auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);if(!Player)return;
    const auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();
    const auto* Survival=Player->FindComponentByClass<UHearthwardSurvivalComponent>();if(!G || G->InCombat() || !Survival || !Survival->Alive())return;
    auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
    for(const auto& Value:HearthwardPersonalStory::Rows())
    {
        const auto R=Value->AsObject();const FName Person(*Text(R,TEXT("id")));
        const auto S=HearthwardPersonalStory::State(GetWorld(),Person);if(S.Step!=TEXT("met"))continue;
        const FName Site(*Text(R,TEXT("site")));
        const bool LegacyTools=Person==TEXT("civilian_initial_02") && Campaign->State.Facts.Contains(TEXT("m1:tools_delivered"));
        const bool Near=FVector::Dist(Player->GetActorLocation(),Campaign->Position(Site))<=300;
        auto& Elapsed=ObservationSeconds.FindOrAdd(Person);Elapsed=Near?Elapsed+Delta:0;
        if(LegacyTools || Elapsed>=2)if(SetStep(Person,TEXT("visited")))
        {const FString Notice=Text(R,TEXT("visited"));G->GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->SetFeedback(Notice);}
    }
}
bool UHearthwardPersonalInteraction::CanUse(AActor* Player,FGuid InEpoch) const
{
    if(!IsValid(Player) || GetOwner()->IsActorBeingDestroyed() || GetOwner()->IsHidden() || Player->GetWorld()!=GetWorld()
        || InEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()
        || FVector::Dist(Player->GetActorLocation(),GetComponentLocation())>MaxDistance)return false;
    const auto* S=Player->FindComponentByClass<UHearthwardSurvivalComponent>();
    const auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();
    if(!S || !S->Alive() || !G || G->InCombat())return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(PersonalConversation),false,Player);Query.AddIgnoredActor(GetOwner());
    return !GetWorld()->LineTraceTestByChannel(Player->GetActorLocation(),GetComponentLocation(),ECC_Visibility,Query);
}
FString UHearthwardPersonalInteraction::GetInteractionPrompt(AActor*) const
{return IsDisplay()?TEXT("E 整理纪念陈列"):TEXT("E 交谈 · ")+Text(HearthwardPersonalStory::Row(Identity),TEXT("title"));}
FString UHearthwardPersonalInteraction::CompleteInteraction(AActor* Player)
{
    const auto Current=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    if(!CanUse(Player,Current))return TEXT("请在安全处靠近后再交谈");
    if(auto* Pawn=Cast<APawn>(Player))if(auto* PC=Cast<APlayerController>(Pawn->GetController()))if(auto* HUD=Cast<AHearthwardHUD>(PC->GetHUD()))
        if(HUD->Screen){HUD->Screen->OpenPersonal(this);return FString();}
    return Describe();
}
FString UHearthwardPersonalInteraction::Describe() const
{
    if(IsDisplay())return TEXT("一份纪念只放在一个位置。选择物件先看预览，再确认摆放；从别处移来会同时撤下旧位置。离开页面会取消预览。");
    const auto R=HearthwardPersonalStory::Row(Identity);const auto S=HearthwardPersonalStory::State(GetWorld(),Identity);
    if(!S.Choice.IsNone())return Text(R,S.Choice.ToString());
    const auto* G=UGameplayStatics::GetPlayerPawn(GetWorld(),0)->FindComponentByClass<UHearthwardGameplayComponent>();
    if(Identity==TEXT("civilian_initial_03") && !G->Discovered.Contains(TEXT("poi_herb_01")))return Text(R,TEXT("unknown"));
    return S.Step==TEXT("visited")?Text(R,TEXT("visited")):Text(R,TEXT("intro"));
}
FString UHearthwardPersonalInteraction::Choose(AActor* Player,FName Choice,FGuid InEpoch)
{
    if(IsDisplay() || !CanUse(Player,InEpoch))return TEXT("交谈已失效，请重新靠近人物");
    const auto R=HearthwardPersonalStory::Row(Identity);const auto S=HearthwardPersonalStory::State(GetWorld(),Identity);
    auto* Director=AHearthwardPersonalStoryDirector::Find(GetWorld());if(!Director)return TEXT("人物记录尚未准备好");
    auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
    if(Choice==TEXT("begin"))
    {
        if(!S.Step.IsNone())return Describe();
        if(Identity==TEXT("civilian_initial_03") && !Player->FindComponentByClass<UHearthwardGameplayComponent>()->Discovered.Contains(TEXT("poi_herb_01")))return Text(R,TEXT("unknown"));
        if(!Director->SetStep(Identity,TEXT("met")))return TEXT("请先结束当前交互");
        GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->RememberExchange(Text(R,TEXT("name")),Text(R,TEXT("intro")));return Text(R,TEXT("intro"));
    }
    if(!S.Choice.IsNone())return Describe();
    if(Choice!=TEXT("camp") && Choice!=TEXT("hometown"))return TEXT("无效选择");
    if(S.Step!=TEXT("visited"))return TEXT("先完成现场观察，再回来交谈");
    if((Choice==TEXT("hometown") || Identity==TEXT("civilian_initial_04")) && !Campaign->State.Victory)return TEXT("故乡尚未真正夺回，结尾会保留到归乡后");
    if(!Director->SetStep(Identity,TEXT("remembered"),Choice))return TEXT("当前无法记录选择，原进度保留");
    const FString Answer=Text(R,Choice.ToString());GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->RememberExchange(Text(R,TEXT("name")),Answer);
    return Answer+TEXT("\n可到相应营地的纪念台预览并摆放；日常岗位保持原安排。");
}
bool UHearthwardPersonalInteraction::OwnsFact(FName Fact) const
{
    for(const auto& V:HearthwardPersonalStory::Rows())
        if(Text(V->AsObject(),TEXT("fact"))==Fact.ToString())
        {const auto S=HearthwardPersonalStory::State(GetWorld(),FName(*Text(V->AsObject(),TEXT("id"))));return S.Step==TEXT("remembered") && S.Choice==DisplayCamp;}
    return false;
}
FString UHearthwardPersonalInteraction::Preview(AActor* Player,FName Fact,FGuid InEpoch)
{
    if(!IsDisplay() || !CanUse(Player,InEpoch) || (!Fact.IsNone() && !OwnsFact(Fact)))return TEXT("这份纪念尚未获得，或人物选择了另一处营地");
    PreviewFact=Fact;PreviewEpoch=InEpoch;RefreshDisplay();return Fact.IsNone()?TEXT("预览：撤下当前陈列 · 确认后生效"):TEXT("正在预览 · 确认后摆放");
}
FString UHearthwardPersonalInteraction::ConfirmDisplay(AActor* Player,FGuid InEpoch)
{
    if(!PreviewEpoch.IsValid() || PreviewEpoch!=InEpoch || !CanUse(Player,InEpoch) || (!PreviewFact.IsNone() && !OwnsFact(PreviewFact)))return TEXT("预览已失效，请重新选择");
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    if(!Save->BindInteractionTarget(Identity,EHearthwardPersistentKind::Display,GetOwner()))return TEXT("槽位尚未准备好");
    const auto Ticket=Save->BeginInteraction(Identity);if(!Ticket.Operation.IsValid())return TEXT("请先结束当前交互");
    auto Next=Save->InteractionState();
    for(auto& S:Next.Displays)if(!PreviewFact.IsNone() && S.Fact==PreviewFact)S.Fact=NAME_None;
    auto* S=Next.Displays.FindByPredicate([&](const auto& V){return V.Id==Identity;});
    if(!S){FHearthwardDisplayState New;New.Id=Identity;New.Camp=DisplayCamp;New.Slot=DisplaySlot;S=&Next.Displays.Add_GetRef(New);}S->Fact=PreviewFact;
    if(!Save->CommitInteraction(Ticket,Next)){Save->CancelInteraction();return TEXT("陈列未提交，原位置保留");}
    CancelPreview();return TEXT("陈列已更新；纪念物只有一个展示位置");
}
void UHearthwardPersonalInteraction::CancelPreview(){PreviewEpoch.Invalidate();PreviewFact=NAME_None;RefreshDisplay();}
void UHearthwardPersonalInteraction::RefreshDisplay()
{
    if(!DisplayMesh)return;
    FName Fact=PreviewFact;
    if(!PreviewEpoch.IsValid())
    {
        const auto& State=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->InteractionState();
        if(const auto* S=State.Displays.FindByPredicate([&](const auto& V){return V.Id==Identity;}))Fact=S->Fact;
    }
    DisplayMesh->SetVisibility(!Fact.IsNone());if(Fact.IsNone())return;
    for(const auto& V:HearthwardPersonalStory::Rows())if(Text(V->AsObject(),TEXT("fact"))==Fact.ToString())
    {
        const FString Name=TEXT("SM_Round2_")+Text(V->AsObject(),TEXT("mesh"));
        const FString Path=TEXT("/Game/Hearthward/Assets/TASK-118/")+Name+TEXT(".")+Name;
        DisplayMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));break;
    }
}
void UHearthwardPersonalInteraction::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Delta,Type,Tick);
    if(PreviewEpoch.IsValid() && !CanUse(UGameplayStatics::GetPlayerPawn(GetWorld(),0),PreviewEpoch))CancelPreview();
    RefreshDisplay();
}
