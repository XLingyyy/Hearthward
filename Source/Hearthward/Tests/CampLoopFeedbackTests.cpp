#include "../UI/HearthwardCampFeedback.h"
#include "../UI/HearthwardScreenWidget.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/SViewport.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FCampFeedbackWorld
{
    UGameInstance* Instance;
    UWorld* World;
    ACharacter* Player;
    UHearthwardCampSubsystem* Camp;
    UHearthwardStorageSubsystem* Storage;
    UHearthwardScreenWidget* Screen;
    TSharedPtr<FSceneViewport> SceneViewport;
    FCampFeedbackWorld()
    {
        Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();World=Instance->GetWorld();
        auto* Viewport=NewObject<UGameViewportClient>(GEngine);Instance->GetWorldContext()->GameViewport=Viewport;
        Viewport->Init(*Instance->GetWorldContext(),Instance,false);SceneViewport=Viewport->CreateViewport(SNew(SViewport));
        auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
        auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);World->AddController(Controller);
        Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);Player->SetActorLocation({0,0,90});
        auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        auto* G=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(G);G->RegisterComponent();G->Enabled=true;
        auto* S=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(S);S->RegisterComponent();
        auto* C=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(C);C->RegisterComponent();
        auto* B=NewObject<UHearthwardBuildingComponent>(Player);Player->AddInstanceComponent(B);B->RegisterComponent();
        Camp=World->GetSubsystem<UHearthwardCampSubsystem>();Camp->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
        Camp->State.AddCamp(TEXT("hometown"),{20000,0,0});Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();
        Screen=CreateWidget<UHearthwardScreenWidget>(Controller);Screen->SetIsFocusable(true);Screen->TakeWidget();Screen->AddToViewport();
    }
    ~FCampFeedbackWorld()
    {
        Screen->RemoveFromParent();SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    FString Field(const FString& Id,const FString& Name) const
    {
        TSharedPtr<FJsonObject> Layout;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),Layout);
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();FString Result;
            if(Row->GetStringField(TEXT("id"))==Id && Row->GetBoolField(TEXT("visible")) && Row->TryGetStringField(Name,Result))return Result;
        }
        return {};
    }
    bool HasAction(const FString& Action) const
    {
        TSharedPtr<FJsonObject> Layout;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),Layout);
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();FString Current;
            if(Row->GetBoolField(TEXT("visible")) && Row->TryGetStringField(TEXT("action"),Current) && Current==Action)return true;
        }
        return false;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampReceipt093Test,"Hearthward.Iteration.Task093.ReceiptTimeline",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampReceipt093Test::RunTest(const FString&)
{
    FHearthwardCampState State;State.AddCamp(TEXT("camp"),FVector::ZeroVector);
    const FGuid Epoch=FGuid::NewGuid(),Campaign=FGuid::NewGuid();FHearthwardCampFeedback View;
    FHearthwardNPCEvent Prior;Prior.Id=FGuid::NewGuid();Prior.Command=FGuid::NewGuid();Prior.Campaign=Campaign;
    Prior.Kind=TEXT("delivered");Prior.Item=TEXT("stone");Prior.Count=2;
    TArray<FHearthwardNPCEvent> Events{Prior};View.Observe(Epoch,Campaign,State,Events);
    TestTrue(TEXT("Existing receipts seed the current state without a return celebration"),View.Notice.IsEmpty());
    TestTrue(TEXT("First genuine state rescue succeeds"),State.Rescue(TEXT("rescued_01")));
    View.Observe(Epoch,Campaign,State,Events);
    TestTrue(TEXT("Authoritative population transition is shown once"),View.Notice.Contains(TEXT("20 → 21")));
    const uint32 RescueRevision=View.Revision;
    TestFalse(TEXT("The same person cannot create another population increase"),State.Rescue(TEXT("rescued_01")));
    View.Observe(Epoch,Campaign,State,Events);TestEqual(TEXT("Repeated state observation does not repeat the notice"),View.Revision,RescueRevision);
    FHearthwardNPCEvent New=Prior;New.Id=FGuid::NewGuid();New.Item=TEXT("wood");New.Count=3;Events.Add(New);
    FHearthwardNPCEvent PlayerDelivery=New;PlayerDelivery.Id=FGuid::NewGuid();PlayerDelivery.Reason=TEXT("player_bag");Events.Add(PlayerDelivery);
    View.Observe(Epoch,Campaign,State,Events);
    TestTrue(TEXT("Only the exact warehouse receipt supplies the delivered amount"),View.Notice.Contains(TEXT("×3")));
    const uint32 DeliveryRevision=View.Revision;View.Observe(Epoch,Campaign,State,Events);
    TestEqual(TEXT("Past events in the same epoch cannot supplement a duplicate popup"),View.Revision,DeliveryRevision);
    State.Tier=2;View.Observe(Epoch,Campaign,State,Events);
    TestTrue(TEXT("Real tier transition presents the approved unlock"),View.Notice.Contains(TEXT("冶炼I")));
    TestEqual(TEXT("Presentation never grants extra population"),State.Population(),21);
    View.Observe(FGuid::NewGuid(),Campaign,State,Events);
    TestTrue(TEXT("Load timeline establishes current state without replaying saved receipts"),View.Notice.IsEmpty());
    TestTrue(TEXT("No session source clearly falls back to current camp state"),View.Summary().Contains(TEXT("当前营地状态")));
    View.Observe(FGuid::NewGuid(),Campaign,State,Events);TestTrue(TEXT("Repeated timeline seed still cannot celebrate"),View.Notice.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampUpgrade093Test,"Hearthward.Iteration.Task093.UpgradePreviewAndReplay",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampUpgrade093Test::RunTest(const FString&)
{
    FCampFeedbackWorld F;F.Screen->OpenPage(TEXT("camp"));
    auto V=HearthwardCampFeedback::ReadUpgrade(F.Camp->State,F.Storage);
    TestFalse(TEXT("Missing rescue and workbench remain explicit real conditions"),V.ConditionsMet);
    TestTrue(TEXT("Both unmet conditions are retained"),V.Conditions.Contains(TEXT("实际获救族人")) && V.Conditions.Contains(TEXT("0 / 1")) && V.Conditions.Contains(TEXT("等级")));
    TestTrue(TEXT("Approved S2 materials report exact shortage"),V.Materials.Contains(TEXT("缺 48")) && V.Materials.Contains(TEXT("缺 24")));
    F.Camp->State.Rescue(TEXT("rescued_01"));
    FHearthwardCampFacility Workbench;Workbench.Id=FGuid::NewGuid();Workbench.Kind=TEXT("workbench");Workbench.Camp=TEXT("camp");F.Camp->State.Facilities.Add(Workbench);
    FHearthwardCampFacility Cooking=Workbench;Cooking.Id=FGuid::NewGuid();Cooking.Kind=TEXT("cooking");F.Camp->State.Facilities.Add(Cooking);F.Camp->State.DonatedPoints=40;
    // Supplies include S3 so repeating an old S1 card would have a concrete second-upgrade consequence.
    TestTrue(TEXT("Diagnostic supplies enter the actual shared container"),F.Storage->Adjust({},{{TEXT("wood"),168},{TEXT("stone"),104},{TEXT("rope"),10}}));
    const FGuid Reservation=FGuid::NewGuid();TestTrue(TEXT("Reserve actual materials"),F.Storage->Reserve(Reservation,{{TEXT("wood"),130}}));
    V=HearthwardCampFeedback::ReadUpgrade(F.Camp->State,F.Storage);
    TestTrue(TEXT("All original S2 conditions are satisfied"),V.ConditionsMet);
    TestFalse(TEXT("Reserved material is unavailable in the preview"),V.MaterialsMet);
    F.Screen->Refresh();const FString Card=F.Field(TEXT("camp.growth.upgrade"),TEXT("action"));
    TestFalse(TEXT("Actual original upgrade refuses unavailable material"),F.Screen->ExecuteAction(Card));
    F.Storage->Release(Reservation);F.Screen->Refresh();
    TestTrue(TEXT("Bound S1 card submits through real camp transaction"),F.Screen->ExecuteAction(Card));
    TestEqual(TEXT("Exactly one tier was committed"),F.Camp->State.Tier,2);
    TestEqual(TEXT("Original wood cost is paid once"),F.Storage->GetItemCount(TEXT("wood")),120);
    TestEqual(TEXT("Original stone cost is paid once"),F.Storage->GetItemCount(TEXT("stone")),80);
    TestFalse(TEXT("The old S1 card cannot perform otherwise affordable S3"),F.Screen->ExecuteAction(Card));
    TestEqual(TEXT("Repeat cannot deduct another tier's materials"),F.Storage->GetItemCount(TEXT("wood")),120);
    TestTrue(TEXT("S2 capability is visible without auto-building it"),F.Field(TEXT("camp.growth.unlocked"),TEXT("text")).Contains(TEXT("冶炼I")));
    const FString NextCard=F.Field(TEXT("camp.growth.upgrade"),TEXT("action"));F.Storage->AdvanceTimeline();
    TestFalse(TEXT("A new timeline rejects the previous bound upgrade card"),F.Screen->ExecuteAction(NextCard));
    TestEqual(TEXT("Timeline rejection preserves the actual tier"),F.Camp->State.Tier,2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampLocal093Test,"Hearthward.Iteration.Task093.LocalFacilitiesAndWork",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampLocal093Test::RunTest(const FString&)
{
    FCampFeedbackWorld F;FHearthwardCampFacility CampBuilding;CampBuilding.Id=FGuid::NewGuid();CampBuilding.Kind=TEXT("workbench");CampBuilding.Camp=TEXT("camp");
    FHearthwardCampFacility HomeBuilding=CampBuilding;HomeBuilding.Id=FGuid::NewGuid();HomeBuilding.Kind=TEXT("cooking");HomeBuilding.Camp=TEXT("hometown");
    F.Camp->State.Facilities={CampBuilding,HomeBuilding};
    F.Screen->OpenPage(TEXT("camp"));F.Screen->ExecuteAction(TEXT("camp.tab:facilities"));
    TestTrue(TEXT("First camp lists its own facility"),F.HasAction(TEXT("camp.facility:")+CampBuilding.Id.ToString()));
    TestFalse(TEXT("First camp does not list hometown facilities"),F.HasAction(TEXT("camp.facility:")+HomeBuilding.Id.ToString()));
    F.Player->SetActorLocation({20000,0,90});F.Screen->OpenPage(TEXT("camp"));F.Screen->ExecuteAction(TEXT("camp.tab:facilities"));
    TestTrue(TEXT("Second camp lists its own facility"),F.HasAction(TEXT("camp.facility:")+HomeBuilding.Id.ToString()));
    TestFalse(TEXT("Second camp does not duplicate the first camp's facility"),F.HasAction(TEXT("camp.facility:")+CampBuilding.Id.ToString()));
    TestTrue(TEXT("Shared population is counted only once"),F.Field(TEXT("camp.summary"),TEXT("text")).Contains(TEXT("族人 20")));
    auto& State=F.Camp->State;auto& Wood=State.Regions[1];
    TestEqual(TEXT("No assignment is displayed as no workers"),HearthwardCampFeedback::RegionStatus(State,Wood),FString(TEXT("无人")));
    State.Assign(Wood.Id,0);TestEqual(TEXT("An assigned disabled queue is paused"),HearthwardCampFeedback::RegionStatus(State,Wood),FString(TEXT("已暂停")));
    Wood.Enabled=true;TestEqual(TEXT("An empty real finite source is waiting for resources"),HearthwardCampFeedback::RegionStatus(State,Wood),FString(TEXT("待资源刷新")));
    FHearthwardCampSource Source;Source.Id=TEXT("camp_wood_source");Source.Camp=TEXT("camp");Source.Item=TEXT("wood");Source.Capacity=Source.Remaining=12;State.Sources.Add(Source);
    Wood.Workers.Reset();Wood.Brother=true;
    TestEqual(TEXT("Assigned brother without current efficiency is waiting to arrive"),HearthwardCampFeedback::RegionStatus(State,Wood),FString(TEXT("等待兄弟到岗")));
    Wood.BrotherEfficiency=1;TestEqual(TEXT("Real arrival permits working presentation"),HearthwardCampFeedback::RegionStatus(State,Wood),FString(TEXT("工作中")));
    State.Assign(TEXT("hometown_stone"),0);TestEqual(TEXT("The same worker in two camp views is only one assigned person"),HearthwardCampFeedback::AssignedWorkers(State),1);
    TestEqual(TEXT("Read-only status leaves the finite source unchanged"),State.Sources[0].Remaining,12);
    return true;
}
#endif
