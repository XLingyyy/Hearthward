#include "../AI/HearthwardAgentContract.h"
#include "../AI/HearthwardNPCContextProjection.h"
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionDialogueContextTest,"Hearthward.Companion079.TaskContext",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCompanionDialogueContextTest::RunTest(const FString&)
{
    FHearthwardNPCContextSnapshot S;
    S.Query=TEXT("还差多少，为什么停了？");
    S.PreviousGoalQuantity=32;S.PreviousGoalDelivered=14;
    S.ExecutionPhase=TEXT("WaitingAtCamp");
    S.bHasActiveTask=true;S.ActiveGoal.Intent=TEXT("collect");S.ActiveGoal.Item=TEXT("wood");S.ActiveGoal.SourceRef=TEXT("S1");
    S.TaskCarried=3;S.TaskBlockReason=TEXT("实际资源不足");
    for(auto Tier:{EHearthwardNPCContextTier::Full,EHearthwardNPCContextTier::Compact,EHearthwardNPCContextTier::Minimal})
    {
        TSharedPtr<FJsonObject> Root;
        TestTrue(TEXT("Projection JSON"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(HearthwardContextProjection::Project(S,Tier).Json),Root));
        const TSharedPtr<FJsonObject>* Task=nullptr;
        if(!TestTrue(TEXT("Every tier retains current task"),Root && Root->TryGetObjectField(TEXT("current_task"),Task)))continue;
        TestEqual(TEXT("Actual delivered"),(*Task)->GetIntegerField(TEXT("delivered")),14);
        TestEqual(TEXT("Remaining delivery"),(*Task)->GetIntegerField(TEXT("remaining")),18);
        TestEqual(TEXT("Carried remains distinct from delivered"),(*Task)->GetIntegerField(TEXT("carried")),3);
        TestEqual(TEXT("Actual blocking reason"),(*Task)->GetStringField(TEXT("block_reason")),S.TaskBlockReason);
        TestEqual(TEXT("Original source"),(*Task)->GetStringField(TEXT("source")),FString(TEXT("S1")));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionDialogueContractTest,"Hearthward.Companion079.TaskContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCompanionDialogueContractTest::RunTest(const FString&)
{
    FHearthwardAgentGoal G;G.Intent=TEXT("task_status");G.Item=TEXT("none");G.QuantityMode=TEXT("none");G.SourceRef=TEXT("current_task");
    TestTrue(TEXT("Read-only task status registered"),HearthwardAgent::Validate(G).IsEmpty());
    TestFalse(TEXT("Status never executes"),G.WritesWorld());
    G.Intent=TEXT("resume");G.Quantity=1;G.QuantityMode=TEXT("directive");
    TestTrue(TEXT("Resume registered"),HearthwardAgent::Validate(G).IsEmpty());
    TestTrue(TEXT("Resume requires confirmation"),G.WritesWorld());
    G.Quantity=18;
    TestFalse(TEXT("Resume cannot overwrite remaining quantity"),HearthwardAgent::Validate(G).IsEmpty());
    return true;
}
#endif
