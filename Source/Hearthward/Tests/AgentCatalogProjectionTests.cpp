#include "../AI/HearthwardAgentContract.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentCatalog109Test,"Hearthward.Engineering.Task109.AgentCatalogProjection",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAgentCatalog109Test::RunTest(const FString&)
{
    const FString Full=HearthwardAgent::Describe();
    const FString Wood=HearthwardAgent::Describe(TEXT("采两份木头"));
    TestTrue(TEXT("Alias selects the canonical wood glossary"),Wood.Contains(TEXT("[wood]")));
    TestTrue(TEXT("Relevant glossary is smaller than the full one"),Wood.Len()<Full.Len());
    TestFalse(TEXT("Unrelated ore does not occupy the wood glossary"),Wood.Contains(TEXT("[ore]")));
    for(const auto& Capability:HearthwardAgent::Capabilities())
        TestTrue(TEXT("All capability rules remain visible"),Wood.Contains(Capability.Id.ToString()+TEXT("|")));
    const FString Directive=HearthwardAgent::Describe(TEXT("跟着我"));
    TestTrue(TEXT("A directive retains its enum without unrelated item expansion"),Directive.Contains(TEXT("follow")) && Directive.Len()<Full.Len());
    for(const auto& Capability:HearthwardAgent::Capabilities())
        TestTrue(TEXT("Unknown noun still retains capability rules for clarification"),HearthwardAgent::Describe(TEXT("帮忙准备一下")).Contains(Capability.Id.ToString()+TEXT("|")));
    const FString Multi=HearthwardAgent::Describe(TEXT("把木材和矿石送回营地"));
    TestTrue(TEXT("Multiple mentioned items remain visible for ambiguity review"),Multi.Contains(TEXT("[wood]")) && Multi.Contains(TEXT("[ore]")));
    return true;
}
#endif
