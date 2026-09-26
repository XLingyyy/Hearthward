#include "../Interaction/HearthwardHarvestSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Components/BoxComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHarvestRefreshTest, "Hearthward.Time.HarvestRefreshWorld",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHarvestRefreshTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,false);
    auto* Harvest=World->GetSubsystem<UHearthwardHarvestSubsystem>();
    auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
    auto* Player=World->SpawnActor<AActor>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);
    Player->AddInstanceComponent(Bag); Bag->RegisterComponent();
    auto* G=NewObject<UHearthwardGameplayComponent>(Player);
    Player->AddInstanceComponent(G); G->RegisterComponent(); G->Enabled=true;
    auto* Tree=World->SpawnActor<AActor>();
    auto* Target=NewObject<UHearthwardHarvestTargetComponent>(Tree);
    Tree->AddInstanceComponent(Target); Tree->SetRootComponent(Target); Target->RegisterComponent();
    const FString Key=TEXT("TreeActor|Mesh|SM_Tree|-1|0,0,0");
    Target->ResourceKey=Key; Target->Item=TEXT("wood"); Target->Capacity=12; Target->MaxDistance=240;

    Clock->Tick(10);
    Target->CompleteInteraction(Player);
    TestTrue(TEXT("Partial harvest has no regeneration timer"),Harvest->RefreshSnapshot().IsEmpty());
    for(int32 I=0;I<5;++I) Target->CompleteInteraction(Player);
    TestEqual(TEXT("Real inventory receives exactly one depleted tree"),Bag->GetItemCount(TEXT("wood")),12);
    TestEqual(TEXT("Depletion schedules two calendar days"),Harvest->RefreshSnapshot().FindRef(Key).DueAt,2890.0);
    Target->CompleteInteraction(Player);
    TestEqual(TEXT("Duplicate depleted completion awards nothing"),Bag->GetItemCount(TEXT("wood")),12);
    const auto Before=Harvest->Snapshot(); const auto Due=Harvest->RefreshSnapshot();

    auto* Pauser=World->SpawnActor<APlayerState>();
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    Clock->Tick(4000);
    TestEqual(TEXT("Paused clock ignores manual tick"),Clock->GetSnapshot().ElapsedCalendarMinutes,10.0);
    TestEqual(TEXT("Paused sleep rejected"),Clock->RequestSleep(),FString(TEXT("PAUSED")));
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    TestEqual(TEXT("Missing survival rules explicitly block sleep"),Clock->RequestSleep(),FString(TEXT("RULE_UNRESOLVED")));
    TestEqual(TEXT("Rejected sleep has no calendar effects"),Clock->GetSnapshot().ElapsedCalendarMinutes,10.0);
    Clock->Tick(2879);
    TestEqual(TEXT("One minute before due stays exhausted"),Harvest->Remaining(Key,12),0);

    auto* Building=World->SpawnActor<AActor>();
    auto* Box=NewObject<UBoxComponent>(Building);
    Building->AddInstanceComponent(Box); Building->SetRootComponent(Box); Box->SetBoxExtent(FVector(100,200,50)); Box->RegisterComponent();
    Building->Tags.Add(TEXT("Hearthward.Building.Completed"));
    Clock->Tick(1);
    TestEqual(TEXT("Occupied due tree stays exhausted"),Harvest->Remaining(Key,12),0);
    Clock->Tick(1440*8);
    TestEqual(TEXT("Missed cycles do not generate inventory"),Bag->GetItemCount(TEXT("wood")),12);
    TestEqual(TEXT("Blocked record keeps original due time"),Harvest->RefreshSnapshot().FindRef(Key).DueAt,2890.0);
    Building->Destroy();
    Clock->Tick(1);
    TestEqual(TEXT("Removing building releases exactly one fresh tree"),Harvest->Remaining(Key,12),12);
    TestTrue(TEXT("Settled refresh removed"),Harvest->RefreshSnapshot().IsEmpty());
    Clock->Tick(1440*8);
    TestEqual(TEXT("Repeated refresh does not issue resource rewards"),Bag->GetItemCount(TEXT("wood")),12);
    Harvest->Restore(Before,Due);
    TestEqual(TEXT("Rollback removes future regeneration"),Harvest->Remaining(Key,12),0);
    Harvest->RefreshDue(2889);
    TestEqual(TEXT("Restored due keeps its original deadline"),Harvest->Remaining(Key,12),0);
    Harvest->RefreshDue(2890);
    Harvest->RefreshDue(2890);
    TestEqual(TEXT("Repeated settlement has one regeneration"),Harvest->Remaining(Key,12),12);

    Bag->TryAdd(TEXT("stone"),88);
    TestTrue(TEXT("Full inventory does not consume fresh resource"),Target->CompleteInteraction(Player).Contains(TEXT("容量不足")));
    TestEqual(TEXT("Failed inventory transaction keeps tree whole"),Harvest->Remaining(Key,12),12);
    TestTrue(TEXT("Failed inventory transaction schedules nothing"),Harvest->RefreshSnapshot().IsEmpty());
    World->DestroyWorld(false);
    return true;
}
#endif
