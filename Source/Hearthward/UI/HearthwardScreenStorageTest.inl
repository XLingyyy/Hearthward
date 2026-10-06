// PROTOTYPE_ONLY: opt-in Development fixture in a disposable GUID save pool.
#if !UE_BUILD_SHIPPING
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
namespace
{
struct FSimpleUIVerification078
{
    FString Output; int32 Step=0; bool Passed=true;
    double Started=FPlatformTime::Seconds();
    TWeakObjectPtr<AActor> Access;
    TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>(),States=MakeShared<FJsonObject>();
    void Check(FString Name,bool Value) {Checks->SetBoolField(Name,Value);Passed&=Value;UE_LOG(LogTemp,Log,TEXT("Simple UI %s: %s"),*Name,Value?TEXT("PASS"):TEXT("FAIL"));}
    bool Finish()
    {
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("passed"),Passed);J->SetObjectField(TEXT("checks"),Checks);J->SetObjectField(TEXT("states"),States);
        J->SetStringField(TEXT("scope"),TEXT("Actual standalone new-game client with controlled inventory, a production storage-access component and workbench in an isolated GUID pool. Real Slate clicks and production storage actions, atomic rejection, page/style audit and offscreen screenshots; no player save changes or physical Windows mouse claims."));
        J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);return false;
    }
};
bool VerifySimpleUI078(float)
{
    static FSimpleUIVerification078 Run;
    if(Run.Output.IsEmpty())
    {
        FString Pool;FGuid Id;
        if(!FParse::Value(FCommandLine::Get(),TEXT("HearthwardStorageVerify="),Run.Output)
            || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id) || !Id.IsValid()) return false;
    }
    if(FPlatformTime::Seconds()-Run.Started>150){Run.Check(TEXT("completion_within_timeout"),false);return Run.Finish();}
    if(!GEngine) return true;
    UWorld* World=nullptr;for(const auto& W:GEngine->GetWorldContexts())if(W.WorldType==EWorldType::Game && W.World()){World=W.World();break;}
    auto* PC=World?World->GetFirstPlayerController():nullptr;auto* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;auto* UI=HUD?HUD->Screen.Get():nullptr;
    if(!UI || !Pawn || !World->HasBegunPlay() || World->GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>()->IsLoading()
        || (Run.Step==0 && UI->GetPage()!=TEXT("hud"))) return true;
    auto* Bag=Pawn->FindComponentByClass<UHearthwardInventoryComponent>();auto* Store=World->GetSubsystem<UHearthwardStorageSubsystem>();
    auto* G=Pawn->FindComponentByClass<UHearthwardGameplayComponent>();auto* Settings=World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
    auto Snapshot=[&]() {TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),J);return J;};
    auto Find=[&](FString Id) ->TSharedPtr<FJsonObject>
    {const auto J=Snapshot();for(const auto& V:J->GetArrayField(TEXT("elements")))if(V->AsObject()->GetStringField(TEXT("id"))==Id)return V->AsObject();return nullptr;};
    auto Click=[&](FString Id)
    {
        const auto E=Find(Id);Run.Check(TEXT("click_target_")+Id,E && E->GetBoolField(TEXT("enabled")));if(!E)return;
        const FVector2D Point(E->GetNumberField(TEXT("x"))+E->GetNumberField(TEXT("width"))*.5,E->GetNumberField(TEXT("y"))+E->GetNumberField(TEXT("height"))*.5);
        Run.Check(TEXT("hit_action_")+Id,UI->ActionAt(Point)==E->GetStringField(TEXT("action")));
        const auto Geometry=UI->GetCachedGeometry();const auto View=Geometry.GetLocalSize();const double Scale=FMath::Min(View.X/1672,View.Y/941);
        const auto Absolute=Geometry.LocalToAbsolute((View-FVector2D(1672,941)*Scale)*.5+Point*Scale);
        const auto Window=FSlateApplication::Get().FindWidgetWindow(UI->TakeWidget());
        const FPointerEvent Down(0,Absolute,Absolute,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
        FSlateApplication::Get().ProcessMouseButtonDownEvent(Window.IsValid()?Window->GetNativeWindow():nullptr,Down);
        const FPointerEvent Up(0,Absolute,Absolute,TSet<FKey>(),EKeys::LeftMouseButton,0,FModifierKeysState());FSlateApplication::Get().ProcessMouseButtonUpEvent(Up);
    };
    auto Capture=[&](FString Name,int32 Width=1672,int32 Height=941)
    {
        Run.States->SetObjectField(Name,Snapshot());const FString Key=FPaths::GetBaseFilename(FPaths::GetPath(Run.Output))+TEXT("-")+Name;
        const bool Rendered=UI->CaptureUI(Key,Width,Height);
        Run.Check(TEXT("capture_")+Name,Rendered && IFileManager::Get().Copy(*(FPaths::GetPath(Run.Output)/(Name+TEXT(".png"))),*(FPaths::ProjectSavedDir()/TEXT("Task020")/(Key+TEXT(".png"))))==COPY_OK);
    };
    auto Clean=[&](FString Name)
    {
        bool Clean=true;const auto J=Snapshot();for(const auto& V:J->GetArrayField(TEXT("elements")))
        {const auto E=V->AsObject();const auto Asset=E->GetStringField(TEXT("asset"));Clean&=Asset!=TEXT("logo") && Asset!=TEXT("pauseBackground") && Asset!=TEXT("storageBackground") && Asset!=TEXT("leatherPanel") && !E->GetStringField(TEXT("text")).Contains(TEXT("余 烬"));}
        Run.Check(Name+TEXT("_no_legacy_brand_fire_or_leather"),Clean);
    };
    if(Run.Step==0)
    {
        Pawn->FindComponentByClass<UHearthwardCombatComponent>()->Cancel();Pawn->FindComponentByClass<UHearthwardSurvivalComponent>()->CancelAction();
        Cast<ACharacter>(Pawn)->GetCharacterMovement()->DisableMovement();Pawn->SetActorLocation(Pawn->GetActorLocation()+FVector(0,0,1000));
        G->Enabled=true;G->Health=G->MaxHealth();Settings->Comfort.TextScale=100;
        FHearthwardInventorySnapshot Empty;Empty.BackpackRank=3;Run.Check(TEXT("controlled_bag"),Bag->RestoreInventory(Empty));
        TMap<FName,int32> Existing;for(const auto& V:HearthwardData::Rows(TEXT("items"))){const FName Id(*HearthwardData::Text(V->AsObject(),TEXT("id")));if(Store->GetItemCount(Id)>0)Existing.Add(Id,Store->GetItemCount(Id));}
        Run.Check(TEXT("controlled_empty_stock"),Store->Adjust(Existing,{}));
        Run.Check(TEXT("fixture_wood"),Bag->TryAdd(TEXT("wood"),5)==EHearthwardInventoryResult::Success);
        int32 Gear=0;for(const auto& V:HearthwardData::Rows(TEXT("items")))
        {const auto R=V->AsObject();if(HearthwardData::Text(R,TEXT("category"))==TEXT("装备") && Gear<22 && Bag->TryAdd(FName(*HearthwardData::Text(R,TEXT("id"))),1)==EHearthwardInventoryResult::Success)++Gear;}
        Run.Check(TEXT("fixture_two_pages"),Gear==22);
        Run.Access=World->SpawnActor<AActor>();auto* Access=NewObject<UHearthwardResourceInteractionComponent>(Run.Access.Get());Run.Access->SetRootComponent(Access);Run.Access->AddInstanceComponent(Access);Access->InitializeResource(true);Access->RegisterComponent();Run.Access->SetActorLocation(Pawn->GetActorLocation());
        Run.Check(TEXT("opens_through_real_access_check"),UI->ExecuteAction(TEXT("page:storage")) && UI->GetPage()==TEXT("storage"));Clean(TEXT("storage_empty"));Capture(TEXT("storage-empty"));Run.Step=1;return true;
    }
    if(Run.Step==1)
    {
        Click(TEXT("storage.tab.2"));Run.Check(TEXT("material_filter"),UI->GetCategory()==TEXT("材料"));Click(TEXT("storage.bag.cell.0"));Click(TEXT("storage.quantity.more"));
        Run.Check(TEXT("quantity_two"),Find(TEXT("storage.quantity"))->GetStringField(TEXT("text"))==TEXT("2"));Click(TEXT("storage.transfer"));
        Run.Check(TEXT("deposit_two_atomic"),Bag->GetItemCount(TEXT("wood"))==3 && Store->GetItemCount(TEXT("wood"))==2);Capture(TEXT("storage-deposit"));
        Run.Step=2;return true;
    }
    if(Run.Step==2)
    {
        Click(TEXT("storage.stock.cell.0"));Click(TEXT("storage.transfer"));Run.Check(TEXT("withdraw_one_atomic"),Bag->GetItemCount(TEXT("wood"))==4 && Store->GetItemCount(TEXT("wood"))==1);Capture(TEXT("storage-withdraw"));
        Run.Step=3;return true;
    }
    if(Run.Step==3)
    {
        UI->ExecuteAction(TEXT("withdraw:wood"));UI->ExecuteAction(TEXT("quantity:1"));
        Run.Check(TEXT("insufficient_quantity_is_atomic"),!UI->ExecuteAction(TEXT("transfer")) && UI->GetMessage().Contains(TEXT("数量不足")) && Bag->GetItemCount(TEXT("wood"))==4 && Store->GetItemCount(TEXT("wood"))==1);
        Capture(TEXT("storage-insufficient"));
        const auto Before=Bag->Snapshot();FHearthwardInventorySnapshot Full;Full.BackpackRank=1;Bag->RestoreInventory(Full);Bag->TryAdd(TEXT("wood"),100);
        UI->ExecuteAction(TEXT("withdraw:wood"));Run.Check(TEXT("capacity_rejection_is_atomic"),!UI->ExecuteAction(TEXT("transfer")) && UI->GetMessage().Contains(TEXT("容量不足")) && Bag->GetItemCount(TEXT("wood"))==100 && Store->GetItemCount(TEXT("wood"))==1);Capture(TEXT("storage-capacity"));Bag->RestoreInventory(Before);
        UI->ExecuteAction(TEXT("deposit:wood"));Pawn->SetActorLocation(Pawn->GetActorLocation()+FVector(300,0,0));
        Run.Check(TEXT("distance_rejection_is_atomic"),!UI->ExecuteAction(TEXT("transfer")) && UI->GetMessage().Contains(TEXT("访问已失效")) && Bag->GetItemCount(TEXT("wood"))==4);Pawn->SetActorLocation(Run.Access->GetActorLocation());
        UI->OpenPage(TEXT("storage"));Store->AdvanceTimeline();Run.Check(TEXT("epoch_rejection_is_atomic"),!UI->ExecuteAction(TEXT("transfer")) && UI->GetMessage().Contains(TEXT("访问已失效")) && Store->GetItemCount(TEXT("wood"))==1);
        UI->OpenPage(TEXT("storage"));UI->ExecuteAction(TEXT("filter:全部"));UI->ExecuteAction(TEXT("storage.next"));Run.Check(TEXT("second_page_keeps_inventory"),Bag->GetItemCount(TEXT("wood"))==4);Capture(TEXT("storage-page2"));Run.Step=4;return true;
    }
    if(Run.Step==4)
    {
        Click(TEXT("storage.prev"));
        for(int32 Scale:{100,125,150})
        {Settings->Comfort.TextScale=Scale;UI->Refresh();Clean(FString::Printf(TEXT("storage_%d"),Scale));Capture(FString::Printf(TEXT("storage-%d"),Scale));}
        Capture(TEXT("storage-150-720p"),1280,720);Capture(TEXT("storage-150-ultrawide"),2560,1080);
        UI->ExecuteAction(TEXT("deposit:axe"));
        Run.Check(TEXT("equipment_stats_and_durability_preserved"),Find(TEXT("storage.detail.properties")) && Find(TEXT("storage.detail.properties"))->GetStringField(TEXT("text")).Contains(TEXT("耐久")));
        Capture(TEXT("storage-gear-150"));
        Settings->Comfort.TextScale=100;auto* Camp=World->GetSubsystem<UHearthwardCampSubsystem>();Camp->State=FHearthwardCampState();Camp->EnsureCamp(Pawn->GetActorLocation());
        auto* Builder=Pawn->FindComponentByClass<UHearthwardBuildingComponent>();Run.Check(TEXT("fixture_workbench"),Builder->AddGift(TEXT("workbench"),Pawn->GetActorLocation()+FVector(100,0,0)));
        if(Builder->NearbyWorkbench().IsValid()) {}
        else for(auto* Actor:Builder->GetBuildings()) {Pawn->SetActorLocation(Actor->GetActorLocation());break;}
        for(const FName Page:{FName(TEXT("inventory")),FName(TEXT("crafting")),FName(TEXT("memory")),FName(TEXT("camp")),FName(TEXT("nature")),FName(TEXT("dialogue"))})
        {UI->OpenPage(Page);Run.Check(Page.ToString()+TEXT("_opens"),UI->GetPage()==Page);Clean(Page.ToString());Capture(Page.ToString());}
        UI->OpenPage(TEXT("repairing"));Run.Check(TEXT("repair_port_uses_current_equipment"),UI->GetPage()==TEXT("equipment"));Clean(TEXT("equipment"));Capture(TEXT("equipment"));
        for(const FName Page:{FName(TEXT("crafting")),FName(TEXT("memory")),FName(TEXT("camp")),FName(TEXT("nature")),FName(TEXT("dialogue"))})
        {Settings->Comfort.TextScale=150;UI->OpenPage(Page);Clean(Page.ToString()+TEXT("_150"));Capture(Page.ToString()+TEXT("-150"));}
        UI->OpenPage(TEXT("camp"));
        const FPointerEvent Wheel(0,FVector2D::ZeroVector,FVector2D::ZeroVector,TSet<FKey>(),FKey(),-10000.f,FModifierKeysState()); UI->NativeOnMouseWheel(UI->GetCachedGeometry(),Wheel);
        Run.Check(TEXT("readable_backdrop_survives_scroll"),Find(TEXT("simple.backdrop")) && !Find(TEXT("simple.backdrop"))->GetBoolField(TEXT("text_scroll_clipped")));
        Capture(TEXT("camp-150-bottom-ultrawide"),2560,1080);
        return Run.Finish();
    }
    return true;
}
const FTSTicker::FDelegateHandle SimpleUITicker078=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&VerifySimpleUI078),.05f);
}
#endif
