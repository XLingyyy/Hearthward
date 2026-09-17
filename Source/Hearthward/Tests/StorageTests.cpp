#include "../Inventory/HearthwardStorageState.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorageTransferTest, "Hearthward.Inventory.Storage.AtomicTransfer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorageTransferTest::RunTest(const FString& Parameters)
{
    using R = EHearthwardInventoryResult;
    FHearthwardInventoryState Bag, Camp(true);
    Bag.Add(TEXT("wood"), 100);
    TestTrue(TEXT("Deposit whole batch"), Bag.TransferTo(Camp, TEXT("wood"), 100) == R::Success);
    TestEqual(TEXT("Source debited"), Bag.GetCount(TEXT("wood")), 0);
    Bag.Add(TEXT("wood"), 100);
    Bag.TransferTo(Camp, TEXT("wood"), 100);
    TestEqual(TEXT("Camp exceeds personal capacity"), Camp.GetCount(TEXT("wood")), 200);
    TestTrue(TEXT("Overweight withdrawal atomic"), Camp.TransferTo(Bag, TEXT("wood"), 101) == R::CapacityExceeded);
    TestEqual(TEXT("Failed source unchanged"), Camp.GetCount(TEXT("wood")), 200);
    TestEqual(TEXT("Failed target unchanged"), Bag.GetCount(TEXT("wood")), 0);
    TestTrue(TEXT("Exact capacity withdrawal"), Camp.TransferTo(Bag, TEXT("wood"), 100) == R::Success);
    TestTrue(TEXT("Insufficient deposit"), Bag.TransferTo(Camp, TEXT("wood"), 101) == R::InsufficientItems);
    TestTrue(TEXT("Zero rejected"), Bag.TransferTo(Camp, TEXT("wood"), 0) == R::InvalidCount);
    TestTrue(TEXT("Negative rejected"), Bag.TransferTo(Camp, TEXT("wood"), -1) == R::InvalidCount);
    TestTrue(TEXT("Unknown item"), Bag.TransferTo(Camp, TEXT("missing"), 1) == R::UnknownItem);
    TestTrue(TEXT("Self transfer rejected"), Bag.TransferTo(Bag, TEXT("wood"), 1) == R::InvalidArgument);
    TestEqual(TEXT("Total conserved"), Bag.GetCount(TEXT("wood")) + Camp.GetCount(TEXT("wood")), 200);
    TestEqual(TEXT("Weight arithmetic widened"), Camp.GetWeightHundredths(), int64(10000));
    FHearthwardInventoryState Huge(true), Small;
    TestTrue(TEXT("No camp weight cap"), Huge.Add(TEXT("ore"), MAX_int32) == R::Success);
    TestEqual(TEXT("Large weight cannot overflow int32"), Huge.GetWeightHundredths(), int64(MAX_int32) * 200);
    Small.Add(TEXT("ore"), 1);
    TestTrue(TEXT("Count overflow rejected"), Small.TransferTo(Huge, TEXT("ore"), 1) == R::QuantityOverflow);
    TestEqual(TEXT("Overflow preserves source"), Small.GetCount(TEXT("ore")), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorageReplayTest, "Hearthward.Inventory.Storage.ReplayAndEpoch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStorageReplayTest::RunTest(const FString& Parameters)
{
    using R = EHearthwardInventoryResult;
    FHearthwardStorageState Storage;
    FHearthwardInventoryState Bag;
    const auto Owner = FGuid::NewGuid(), Operation = FGuid::NewGuid(), Epoch = Storage.GetEpoch();
    Bag.Add(TEXT("wood"), 10);
    auto Reply = Storage.Transfer(Bag, Owner, true, TEXT("wood"), 10, Operation, Epoch);
    TestTrue(TEXT("First applied"), Reply.Result == R::Success && Reply.MovedCount == 10 && !Reply.Replayed);
    Reply = Storage.Transfer(Bag, Owner, true, TEXT("wood"), 10, Operation, Epoch);
    TestTrue(TEXT("Replay not reapplied despite empty source"), Reply.Result == R::Success && Reply.MovedCount == 0 && Reply.Replayed);
    TestEqual(TEXT("No duplication"), Storage.GetCount(TEXT("wood")), 10);
    TestTrue(TEXT("Payload mismatch rejected"), Storage.Transfer(Bag, Owner, true, TEXT("wood"), 9, Operation, Epoch).Result == R::OperationConflict);
    TestTrue(TEXT("Direction mismatch rejected"), Storage.Transfer(Bag, Owner, false, TEXT("wood"), 10, Operation, Epoch).Result == R::OperationConflict);
    TestTrue(TEXT("Container mismatch rejected"), Storage.Transfer(Bag, FGuid::NewGuid(), true, TEXT("wood"), 10, Operation, Epoch).Result == R::OperationConflict);
    TestTrue(TEXT("Missing operation rejected"), Storage.Transfer(Bag, Owner, true, TEXT("wood"), 1, FGuid(), Epoch).Result == R::InvalidArgument);
    const auto FailedOperation = FGuid::NewGuid();
    Reply = Storage.Transfer(Bag, Owner, true, TEXT("stone"), 1, FailedOperation, Epoch);
    TestTrue(TEXT("First failure"), Reply.Result == R::InsufficientItems);
    Bag.Add(TEXT("stone"), 1);
    Reply = Storage.Transfer(Bag, Owner, true, TEXT("stone"), 1, FailedOperation, Epoch);
    TestTrue(TEXT("Failed request replay stable after replenishment"), Reply.Result == R::InsufficientItems && Reply.Replayed && Reply.MovedCount == 0);
    TestEqual(TEXT("Failure retry does not consume"), Bag.GetCount(TEXT("stone")), 1);
    Storage.AdvanceTimeline();
    TestTrue(TEXT("Old successful ID cannot bypass epoch"), Storage.Transfer(Bag, Owner, true, TEXT("wood"), 10, Operation, Epoch).Result == R::StaleTimeline);
    TestEqual(TEXT("Epoch change preserves contents"), Storage.GetCount(TEXT("wood")), 10);
    TestTrue(TEXT("New epoch accepts new execution"), Storage.Transfer(Bag, Owner, true, TEXT("stone"), 1, FailedOperation, Storage.GetEpoch()).MovedCount == 1);
    FHearthwardStorageState NextWorld;
    TestTrue(TEXT("Next world isolated"), NextWorld.GetCount(TEXT("wood")) == 0 && NextWorld.GetEpoch() != Storage.GetEpoch() && NextWorld.GetContainerId() != Storage.GetContainerId());
    return true;
}
#endif
