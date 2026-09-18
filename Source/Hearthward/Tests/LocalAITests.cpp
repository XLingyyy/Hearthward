#include "../AI/HearthwardLocalAIContext.h"
#include "../Companion/HearthwardCompanionCommand.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalAIProposalTest, "Hearthward.LocalAI.StructuredBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLocalAIProposalTest::RunTest(const FString& Parameters)
{
    FHearthwardAIProposal Out;
    const FString Valid = TEXT(R"({"intent":"collect","item":"wood","quantity":10,"steps":["collect","return","deposit"],"npc_line":"我去收集十份木材，分趟带回营地。"})");
    TestTrue(TEXT("Typed model proposal accepted"), HearthwardLocalAI::ParseProposal(Valid, Out));
    TestEqual(TEXT("Quantity retained"), Out.Quantity, 10);
    TestFalse(TEXT("Fractional count rejected"), HearthwardLocalAI::ParseProposal(Valid.Replace(TEXT(":10,"), TEXT(":1.5,")), Out));
    TestFalse(TEXT("Overflow count rejected"), HearthwardLocalAI::ParseProposal(Valid.Replace(TEXT(":10,"), TEXT(":2147483648,")), Out));
    TestFalse(TEXT("Unknown action rejected"), HearthwardLocalAI::ParseProposal(Valid.Replace(TEXT("\"collect\""), TEXT("\"attack\"")), Out));
    TestFalse(TEXT("Wrong plan order rejected"), HearthwardLocalAI::ParseProposal(Valid.Replace(TEXT("\"collect\",\"return\""), TEXT("\"return\",\"collect\"")), Out));
    TestFalse(TEXT("No free world mutation fields"), HearthwardLocalAI::ParseProposal(Valid.Replace(TEXT("{\"intent\""), TEXT("{\"grant_inventory\":100,\"intent\"")), Out));
    TestFalse(TEXT("Non action cannot smuggle steps"), HearthwardLocalAI::ParseProposal(Valid.Replace(TEXT("\"intent\":\"collect\""), TEXT("\"intent\":\"dialogue\"")), Out));
    TestFalse(TEXT("Truncated output rejected"), HearthwardLocalAI::ParseProposal(Valid.Left(30), Out));
    TestEqual(TEXT("Conflicting routing hints remain unknown"), HearthwardLocalAI::ClassifyHint(TEXT("取消采集木材")), FString(TEXT("unknown")));
    TestEqual(TEXT("Unrecognized wording remains unknown"), HearthwardLocalAI::ClassifyHint(TEXT("陪我聊聊")), FString(TEXT("unknown")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalAIPendingTest, "Hearthward.LocalAI.CancelPendingPreservesExecution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLocalAIPendingTest::RunTest(const FString& Parameters)
{
    FHearthwardCompanionCommand Command;
    const auto Epoch = FGuid::NewGuid();
    const auto Active = Command.Request(Epoch);
    Command.Accept(Active, Epoch, TEXT("wood"), 10, {TEXT("collect"), TEXT("return"), TEXT("deposit")});
    const auto OldPending = Command.Request(Epoch);
    const auto Pending = Command.Request(Epoch);
    Command.DiscardPending(OldPending);
    TestTrue(TEXT("Late cancel cannot discard newer reply"), Command.IsPending(Pending, Epoch));
    Command.DiscardPending(Pending);
    TestFalse(TEXT("Cancelled reply no longer pending"), Command.IsPending(Pending, Epoch));
    TestTrue(TEXT("Existing accepted goal continues"), Command.IsCurrent(Epoch) && Command.GetActive().Matches(Active));
    const FString Knowledge = TEXT(R"({"chunks":[{"id":"public","topic":"gather","visibility":"initial_known","terms":["wood"],"text":"safe"},{"id":"secret","topic":"gather","visibility":"future_world","terms":["wood"],"text":"hidden"}]})");
    const auto Retrieved = HearthwardLocalAI::RetrieveKnowledge(TEXT("wood"), TEXT("gather"), Knowledge);
    TestTrue(TEXT("Retrieval includes allowed knowledge"), Retrieved.Contains(TEXT("safe")));
    TestFalse(TEXT("Future/unobserved records excluded"), Retrieved.Contains(TEXT("hidden")));
    return true;
}
#endif
