#include "../Companion/HearthwardCompanionCommand.h"
#include "../Inventory/HearthwardStorageState.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionCandidateTest, "Hearthward.Companion.CandidateAndSupersession",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompanionCandidateTest::RunTest(const FString& Parameters)
{
    using R = EHearthwardProposalResult;
    FHearthwardCompanionCommand Command;
    const auto Epoch = FGuid::NewGuid();
    const TArray<FName> Steps = {TEXT("collect"), TEXT("return"), TEXT("deposit")};
    auto Ticket = Command.Request(Epoch);
    TestTrue(TEXT("Missing goal clarified"), Command.Accept(Ticket, Epoch, NAME_None, 10, Steps) == R::NeedsClarification);
    TestTrue(TEXT("Missing count clarified"), Command.Accept(Ticket, Epoch, TEXT("wood"), 0, Steps) == R::NeedsClarification);
    TestTrue(TEXT("Unknown item rejected"), Command.Accept(Ticket, Epoch, TEXT("magic"), 10, Steps) == R::Unsupported);
    auto Four = Steps; Four.Add(TEXT("attack"));
    TestTrue(TEXT("Four steps rejected"), Command.Accept(Ticket, Epoch, TEXT("wood"), 10, Four) == R::Unsupported);
    TestTrue(TEXT("Dangerous plan rejected"), Command.Accept(Ticket, Epoch, TEXT("wood"), 10, {TEXT("attack")}) == R::Unsupported);
    TestTrue(TEXT("Typed goal accepted"), Command.Accept(Ticket, Epoch, TEXT("wood"), 10, Steps) == R::Accepted);
    TestTrue(TEXT("Repeated reply stale"), Command.Accept(Ticket, Epoch, TEXT("wood"), 10, Steps) == R::Stale);
    const auto NewTicket = Command.Request(Epoch);
    TestTrue(TEXT("Unapproved candidate preserves execution"), Command.IsCurrent(Epoch) && Command.GetActive().Matches(Ticket));
    TestTrue(TEXT("New matching goal overrides"), Command.Accept(NewTicket, Epoch, TEXT("stone"), 3, Steps) == R::Accepted);
    TestFalse(TEXT("Old completion cannot credit replacement"), Command.RecordDelivery(Ticket, 3));
    Command.Cancel();
    TestFalse(TEXT("Cancelled action inactive"), Command.IsCurrent(Epoch));
    TestTrue(TEXT("Cancelled response cannot reactivate"), Command.Accept(NewTicket, Epoch, TEXT("wood"), 10, Steps) == R::Stale);
    Ticket = Command.Request(Epoch);
    const auto NewEpoch = FGuid::NewGuid();
    TestTrue(TEXT("Old timeline reply rejected"), Command.Accept(Ticket, NewEpoch, TEXT("wood"), 10, Steps) == R::Stale);
    Ticket = Command.Request(NewEpoch);
    const auto Latest = Command.Request(NewEpoch);
    TestTrue(TEXT("Stale suggestion rejected"), Command.Accept(Ticket, NewEpoch, TEXT("wood"), 10, Steps) == R::Stale);
    TestTrue(TEXT("Latest suggestion accepted"), Command.Accept(Latest, NewEpoch, TEXT("wood"), 10, Steps) == R::Accepted);
    TestFalse(TEXT("Advance epoch invalidates running action"), Command.IsCurrent(FGuid::NewGuid()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionDeliveryTest, "Hearthward.Companion.ActualDeliveryAndReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompanionDeliveryTest::RunTest(const FString& Parameters)
{
    FHearthwardCompanionCommand Command;
    FHearthwardStorageState Camp;
    FHearthwardInventoryState Resource, Bag;
    Resource.Add(TEXT("wood"), 10);
    const auto Ticket = Command.Request(Camp.GetEpoch());
    Command.Accept(Ticket, Camp.GetEpoch(), TEXT("wood"), 10, {TEXT("collect"), TEXT("return"), TEXT("deposit")});
    const auto BagId = FGuid::NewGuid();
    for (int32 Batch : {4, 4, 2})
    {
        Resource.TransferTo(Bag, TEXT("wood"), Batch);
        TestEqual(TEXT("Carrying is not delivered"), Command.GetDelivered(), Camp.GetCount(TEXT("wood")));
        const auto Operation = FGuid::NewGuid();
        const auto Result = Camp.Transfer(Bag, BagId, true, TEXT("wood"), Batch, Operation, Camp.GetEpoch());
        TestTrue(TEXT("Credit actual transfer"), Command.RecordDelivery(Ticket, Result.MovedCount));
        const auto Retry = Camp.Transfer(Bag, BagId, true, TEXT("wood"), Batch, Operation, Camp.GetEpoch());
        TestFalse(TEXT("Replay cannot grant progress"), Command.RecordDelivery(Ticket, Retry.MovedCount));
        TestEqual(TEXT("Actual cumulative stock"), Command.GetDelivered(), Camp.GetCount(TEXT("wood")));
        TestEqual(TEXT("Conservation"), Resource.GetCount(TEXT("wood")) + Bag.GetCount(TEXT("wood")) + Camp.GetCount(TEXT("wood")), 10);
    }
    TestEqual(TEXT("Exactly requested amount"), Command.GetDelivered(), 10);
    TestFalse(TEXT("Completed goal stops"), Command.IsCurrent(Camp.GetEpoch()));
    TestTrue(TEXT("Empty source cannot invent inventory"), Resource.TransferTo(Bag, TEXT("wood"), 100) == EHearthwardInventoryResult::InsufficientItems);
    TestEqual(TEXT("Failure preserves camp"), Camp.GetCount(TEXT("wood")), 10);
    TestFalse(TEXT("Late completion cannot duplicate"), Command.RecordDelivery(Ticket, 1));
    return true;
}
#endif
