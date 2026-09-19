#include "../AI/HearthwardLocalAIContext.h"
#include "../AI/HearthwardNPCMemory.h"
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
    const FString Query=TEXT(R"({"intent":"inventory","item":"rope","quantity":0,"steps":[],"npc_line":"我查一下亲见记录。"})");
    TestTrue(TEXT("Query uses registered item catalogue"),HearthwardLocalAI::ParseProposal(Query,Out));
    TestFalse(TEXT("Query cannot smuggle mutation"),HearthwardLocalAI::ParseProposal(Query.Replace(TEXT(":0,"),TEXT(":2,")),Out));
    TestFalse(TEXT("Unknown query item rejected"),HearthwardLocalAI::ParseProposal(Query.Replace(TEXT("rope"),TEXT("secret_item")),Out));
    TestTrue(TEXT("Read-only memory query"),HearthwardLocalAI::ParseProposal(Query.Replace(TEXT("inventory"),TEXT("recall")).Replace(TEXT("rope"),TEXT("none")),Out));
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCMemoryTest, "Hearthward.LocalAI.MemorySourcesAndRevocation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNPCMemoryTest::RunTest(const FString& Parameters)
{
    FHearthwardNPCMemory M;
    TestTrue(TEXT("Explicit player claim stored"),M.Put({},TEXT("claim"),TEXT("故乡门口有一棵银杏树"),1));
    const auto Id=M.Records[0].Id;
    for(int32 I=0;I<12;++I) M.Put({},TEXT("claim"),FString::Printf(TEXT("昨晚聊过第%d件小事"),I),I+2);
    TestTrue(TEXT("Relevant old memory retrieved beyond recent eight exchanges"),M.Retrieve(TEXT("故乡银杏树在哪里")).ContainsByPredicate([&](const auto& R){return R.Id==Id;}));
    TestTrue(TEXT("Clarification retained"),M.AddClarification(TEXT("帮我采木材，别进入危险区域"),TEXT("需要多少？")));
    TestTrue(TEXT("Player edit succeeds"),M.Put(Id,TEXT("claim"),TEXT("故乡门口其实是槐树"),20));
    TestEqual(TEXT("Edit invalidates pending interpretation"),M.Clarification.Num(),0);
    TestFalse(TEXT("Cannot write authoritative record kind"),M.Put({},TEXT("observation"),TEXT("木材100"),20));
    TestFalse(TEXT("Cannot edit unknown record"),M.Put(FGuid::NewGuid(),TEXT("claim"),TEXT("任意事实"),20));
    TestTrue(TEXT("Revoke own record"),M.Revoke(Id));
    TestFalse(TEXT("Revoked record never retrieved"),M.Retrieve(TEXT("故乡门口槐树")).ContainsByPredicate([&](const auto& R){return R.Id==Id;}));
    TestFalse(TEXT("Cannot silently resurrect revoked record"),M.Put(Id,TEXT("claim"),TEXT("银杏树"),20));
    TestTrue(TEXT("Valid temporal state"),M.IsValid(20));
    M.Records.Last().RecordedAt=21;
    TestFalse(TEXT("Future record invalidates snapshot"),M.IsValid(20));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCMemoryBudgetTest, "Hearthward.LocalAI.MemoryBudgetAndClarification",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNPCMemoryBudgetTest::RunTest(const FString& Parameters)
{
    FHearthwardNPCMemory M;
    for(int32 I=0;I<4;++I) TestTrue(TEXT("Supported agreement slot"),M.Put({},TEXT("agreement"),FString::Printf(TEXT("约定%d"),I),0));
    TestFalse(TEXT("No silent agreement eviction"),M.Put({},TEXT("agreement"),TEXT("第五条"),0));
    TestEqual(TEXT("All active agreements always retrieved"),M.Retrieve(TEXT("完全无关问题")).Num(),4);
    TestTrue(TEXT("Edit at agreement capacity"),M.Put(M.Records[0].Id,TEXT("agreement"),TEXT("更正约定"),0));
    TestFalse(TEXT("No truncation of long restrictions"),M.Put({},TEXT("claim"),FString::ChrN(121,TCHAR('x')),0));
    for(int32 I=0;I<4;++I) TestTrue(TEXT("Bounded clarification turn"),M.AddClarification(TEXT("补充"),TEXT("问题")));
    TestFalse(TEXT("Overflow explicit, original restrictions retained"),M.AddClarification(TEXT("不能忽略限制"),TEXT("问题")));
    TestEqual(TEXT("State remains valid after rejection"),M.Clarification.Num(),4);
    M.HasCampObservation=true; M.CampObservedAt=5; M.CampInventory.Add(TEXT("wood"),7);
    TestTrue(TEXT("Observed inventory validates"),M.IsValid(5));
    auto Saved=M; M.CampInventory[TEXT("wood")]=99; M.Revoke(M.Records[0].Id);
    TestEqual(TEXT("Snapshot owns observation by value"),Saved.CampInventory[TEXT("wood")],7);
    TestEqual(TEXT("Snapshot owns pending dialogue"),Saved.Clarification.Num(),4);
    M=Saved; M.CampInventory.Add(TEXT("unknown_item"),1);
    TestFalse(TEXT("Unknown observation item rejected"),M.IsValid(5));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCRestrictionTest, "Hearthward.LocalAI.StructuredRestrictions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNPCRestrictionTest::RunTest(const FString& Parameters)
{
    FHearthwardNPCMemory M;
    TestFalse(TEXT("Rule requires registered item"),M.Put({},TEXT("collection_ban"),TEXT("采集限制"),0,TEXT("unknown")));
    TestTrue(TEXT("Typed player restriction accepted"),M.Put({},TEXT("collection_ban"),TEXT("留给我处理"),0,TEXT("wood")));
    const auto Id=M.Records[0].Id;
    TestTrue(TEXT("Blocked independent of model wording"),M.BlocksCollection(TEXT("wood")));
    TestFalse(TEXT("Unrelated item unaffected"),M.BlocksCollection(TEXT("stone")));
    auto Saved=M; M.Revoke(Id);
    TestFalse(TEXT("Revocation releases restriction"),M.BlocksCollection(TEXT("wood")));
    M=Saved;
    TestTrue(TEXT("Rollback restores restriction"),M.BlocksCollection(TEXT("wood")));
    TestTrue(TEXT("Edit changes target"),M.Put(Id,TEXT("collection_ban"),TEXT("保留石材"),0,TEXT("stone")));
    TestTrue(TEXT("Only current typed target enforced"),!M.BlocksCollection(TEXT("wood")) && M.BlocksCollection(TEXT("stone")));
    TestTrue(TEXT("Rule state validates"),M.IsValid(0));
    return true;
}
#endif
