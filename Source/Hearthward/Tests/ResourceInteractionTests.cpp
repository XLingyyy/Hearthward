#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FResourceTestWorld
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false);
    ~FResourceTestWorld() { World->DestroyWorld(false); }
    UHearthwardInventoryComponent* Bag(AActor* Actor)
    {
        auto* Inventory = NewObject<UHearthwardInventoryComponent>(Actor);
        Actor->AddInstanceComponent(Inventory);
        Inventory->RegisterComponent();
        return Inventory;
    }
    UHearthwardResourceInteractionComponent* Target(AActor* Actor, bool Camp)
    {
        auto* Target = NewObject<UHearthwardResourceInteractionComponent>(Actor);
        Actor->AddInstanceComponent(Target);
        Actor->SetRootComponent(Target);
        Target->InitializePrototype(Camp);
        Target->RegisterComponent();
        return Target;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResourceTransferTest, "Hearthward.Resource.Settlement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FResourceTransferTest::RunTest(const FString& Parameters)
{
    FResourceTestWorld F;
    auto* Player = F.World->SpawnActor<AActor>();
    auto* Bag = F.Bag(Player);
    auto* SourceActor = F.World->SpawnActor<AActor>();
    auto* Source = F.Bag(SourceActor);
    auto* Gather = F.Target(SourceActor, false);
    auto* Camp = F.Target(F.World->SpawnActor<AActor>(), true);
    auto* Storage = F.World->GetSubsystem<UHearthwardStorageSubsystem>();
    Source->TryAdd(TEXT("wood"), 2);
    TestTrue(TEXT("Gather result"), Gather->CompleteInteraction(Player).Contains(TEXT("已放入背包")));
    TestEqual(TEXT("Source actually debited"), Source->GetItemCount(TEXT("wood")), 1);
    TestEqual(TEXT("Bag actually credited"), Bag->GetItemCount(TEXT("wood")), 1);
    TestTrue(TEXT("Weight visible before collection"), Gather->GetInteractionPrompt(Player).Contains(TEXT("99.00")));
    TestTrue(TEXT("Deposit result"), Camp->CompleteInteraction(Player).Contains(TEXT("已入库")));
    TestEqual(TEXT("Bag debited on deposit"), Bag->GetItemCount(TEXT("wood")), 0);
    TestEqual(TEXT("Shared storage credited"), Storage->GetItemCount(TEXT("wood")), 1);
    TestTrue(TEXT("Empty bag explained"), Camp->CompleteInteraction(Player).Contains(TEXT("没有")));
    TestEqual(TEXT("Empty deposit no duplicate"), Storage->GetItemCount(TEXT("wood")), 1);
    Gather->CompleteInteraction(Player);
    TestTrue(TEXT("Exhaustion explained"), Gather->CompleteInteraction(Player).Contains(TEXT("耗尽")));
    TestEqual(TEXT("Finite total conserved"), Bag->GetItemCount(TEXT("wood")) + Storage->GetItemCount(TEXT("wood")), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResourceRejectionTest, "Hearthward.Resource.Rejection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FResourceRejectionTest::RunTest(const FString& Parameters)
{
    FResourceTestWorld F;
    auto* Player = F.World->SpawnActor<AActor>();
    auto* Bag = F.Bag(Player);
    auto* SourceActor = F.World->SpawnActor<AActor>();
    auto* Source = F.Bag(SourceActor);
    auto* Gather = F.Target(SourceActor, false);
    Source->TryAdd(TEXT("wood"), 1);
    Bag->TryAdd(TEXT("stone"), 100);
    TestTrue(TEXT("Full bag explained"), Gather->CompleteInteraction(Player).Contains(TEXT("容量不足")));
    TestEqual(TEXT("Full bag leaves source"), Source->GetItemCount(TEXT("wood")), 1);
    TestEqual(TEXT("Full bag no reward"), Bag->GetItemCount(TEXT("wood")), 0);
    Bag->TryRemove(TEXT("stone"), 100);
    SourceActor->SetActorLocation(FVector(1000, 0, 0));
    TestTrue(TEXT("Distance revalidated at settlement"), Gather->CompleteInteraction(Player).Contains(TEXT("不可用")));
    TestEqual(TEXT("Remote source preserved"), Source->GetItemCount(TEXT("wood")), 1);
    auto* Disabled = NewObject<UHearthwardResourceInteractionComponent>(SourceActor);
    TestTrue(TEXT("Uninitialized fixture inert"), Disabled->CompleteInteraction(Player).Contains(TEXT("不可用")));
    TestTrue(TEXT("Uninitialized fixture no prompt"), Disabled->GetInteractionPrompt(Player).IsEmpty());
    return true;
}
#endif
