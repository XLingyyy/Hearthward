#include "../Inventory/HearthwardInventoryState.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryCapacityTest, "Hearthward.Inventory.ExactWeightsAndCapacity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryCapacityTest::RunTest(const FString& Parameters)
{
    FHearthwardInventoryState Inventory;
    for (const auto& Item : HearthwardBasicItems())
        TestTrue(TEXT("Known item accepted"), Inventory.Add(Item.Id, 1) == EHearthwardInventoryResult::Success);
    TestEqual(TEXT("Five-item weight is exactly 4.55"), Inventory.GetWeightHundredths(), 455);
    FHearthwardInventoryState Arrows;
    for (int32 N = 0; N < 2000; ++N) Arrows.Add(TEXT("arrow"), 1);
    TestEqual(TEXT("2000 arrows exactly fill the bag"), Arrows.GetWeightHundredths(), 10000);
    TestTrue(TEXT("Extra arrow rejected"), Arrows.Add(TEXT("arrow"), 1) == EHearthwardInventoryResult::CapacityExceeded);
    TestEqual(TEXT("Failure preserves count"), Arrows.GetCount(TEXT("arrow")), 2000);
    TestTrue(TEXT("Remove accepted"), Arrows.Remove(TEXT("arrow"), 1000) == EHearthwardInventoryResult::Success);
    TestTrue(TEXT("Half load speed"), FMath::IsNearlyEqual(Arrows.GetMoveSpeedMultiplier(), 0.95f));
    TestTrue(TEXT("Half load stamina cost"), FMath::IsNearlyEqual(Arrows.GetStaminaCostMultiplier(), 1.05f));
    TestTrue(TEXT("Other instance unchanged"), Inventory.GetWeightHundredths() == 455);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryFailureTest, "Hearthward.Inventory.AtomicFailuresAndRemoval",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryFailureTest::RunTest(const FString& Parameters)
{
    FHearthwardInventoryState Inventory;
    Inventory.Add(TEXT("wood"), 20);
    TestTrue(TEXT("Unknown item"), Inventory.Add(TEXT("invalid"), 1) == EHearthwardInventoryResult::UnknownItem);
    TestTrue(TEXT("Zero count"), Inventory.Add(TEXT("wood"), 0) == EHearthwardInventoryResult::InvalidCount);
    TestTrue(TEXT("Negative add"), Inventory.Add(TEXT("wood"), -1) == EHearthwardInventoryResult::InvalidCount);
    TestTrue(TEXT("Negative remove"), Inventory.Remove(TEXT("wood"), -1) == EHearthwardInventoryResult::InvalidCount);
    TestTrue(TEXT("Huge request cannot overflow"), Inventory.Add(TEXT("ore"), MAX_int32) == EHearthwardInventoryResult::CapacityExceeded);
    TestTrue(TEXT("Whole batch rejected"), Inventory.Add(TEXT("ore"), 41) == EHearthwardInventoryResult::CapacityExceeded);
    TestTrue(TEXT("Insufficient quantity"), Inventory.Remove(TEXT("wood"), 21) == EHearthwardInventoryResult::InsufficientItems);
    TestTrue(TEXT("Unknown removal"), Inventory.Remove(TEXT("invalid"), 1) == EHearthwardInventoryResult::UnknownItem);
    TestEqual(TEXT("All failures preserve original quantity"), Inventory.GetCount(TEXT("wood")), 20);
    TestEqual(TEXT("All failures preserve original weight"), Inventory.GetWeightHundredths(), 2000);
    Inventory.Add(TEXT("ore"), 40);
    TestTrue(TEXT("Full load speed"), FMath::IsNearlyEqual(Inventory.GetMoveSpeedMultiplier(), 0.9f));
    TestTrue(TEXT("Full load stamina cost"), FMath::IsNearlyEqual(Inventory.GetStaminaCostMultiplier(), 1.1f));
    Inventory.Remove(TEXT("wood"), 20);
    Inventory.Remove(TEXT("ore"), 40);
    TestEqual(TEXT("Empty weight restored"), Inventory.GetWeightHundredths(), 0);
    TestEqual(TEXT("Empty speed restored"), Inventory.GetMoveSpeedMultiplier(), 1.0f);
    return true;
}
#endif
