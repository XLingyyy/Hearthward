#include "Misc/AutomationTest.h"
#include "../Camp/HearthwardCampState.h"
#include "../AI/HearthwardAgentContract.h"
#include "../AI/HearthwardNPCMemory.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardWorkPartyTest,"Hearthward.Dialogue081.WorkParty",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHearthwardWorkPartyTest::RunTest(const FString&)
{
    FHearthwardCampState S;S.AddCamp(TEXT("camp"),FVector::ZeroVector);
    FHearthwardCampSource Source;Source.Id=TEXT("party-test");Source.Camp=TEXT("camp");Source.Item=TEXT("wood");Source.Capacity=Source.Remaining=100;S.Sources.Add(Source);
    FName Region;TArray<int32> Workers;const FString Before=S.Snapshot();
    TestTrue(TEXT("plan two idle workers"),S.PlanWorkParty(TEXT("camp"),TEXT("wood"),2,Region,Workers).IsEmpty());
    TestEqual(TEXT("correct region"),Region,FName(TEXT("camp_wood")));TestEqual(TEXT("two workers"),Workers.Num(),2);
    TestEqual(TEXT("preview never mutates"),S.Snapshot(),Before);
    S.Assign(TEXT("camp_stone"),0);
    TestTrue(TEXT("plan excludes occupied workers"),S.PlanWorkParty(TEXT("camp"),TEXT("wood"),4,Region,Workers).IsEmpty());
    TestFalse(TEXT("worker 0 remains at stone"),Workers.Contains(0));
    TestFalse(TEXT("five plus brother exceeds capacity"),S.PlanWorkParty(TEXT("camp"),TEXT("wood"),5,Region,Workers).IsEmpty());
    TestFalse(TEXT("missing stone resource refuses"),S.PlanWorkParty(TEXT("camp"),TEXT("stone"),2,Region,Workers).IsEmpty());
    S.Assign(TEXT("camp_stone"),31);
    TestFalse(TEXT("never transfers brother silently"),S.PlanWorkParty(TEXT("camp"),TEXT("wood"),2,Region,Workers).IsEmpty());
    S.Assign(TEXT("camp_stone"),31);S.Assign(TEXT("camp_stone"),0);
    S.PlanWorkParty(TEXT("camp"),TEXT("wood"),2,Region,Workers);
    auto& R=S.Regions[1];R.Workers=Workers;R.Brother=true;R.Enabled=true;R.BrotherEfficiency=3;
    int32 Output=0;S.Advance(3,false,[&](const auto&,const auto& Outputs){Output+=Outputs.FindRef(TEXT("wood"));return true;});
    TestEqual(TEXT("existing labor rule produces 2 wood in 3 minutes"),Output,2);
    TestTrue(TEXT("same team can resume"),S.PlanWorkParty(TEXT("camp"),TEXT("wood"),2,Region,Workers).IsEmpty());
    R.Enabled=false;const int32 Stopped=Output;S.Advance(30,false,[&](const auto&,const auto& Outputs){Output+=Outputs.FindRef(TEXT("wood"));return true;});
    TestEqual(TEXT("pause produces nothing"),Output,Stopped);
    FHearthwardCampState Restored;TestTrue(TEXT("existing save format restores team"),FHearthwardCampState::Parse(S.Snapshot(),Restored));
    TestTrue(TEXT("brother assignment retained"),Restored.Regions[1].Brother);TestEqual(TEXT("workers retained"),Restored.Regions[1].Workers.Num(),2);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardWorkPartyContractTest,"Hearthward.Dialogue081.Contract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHearthwardWorkPartyContractTest::RunTest(const FString&)
{
    FHearthwardAgentGoal G;G.Intent=TEXT("camp_team");G.Item=TEXT("wood");G.Quantity=2;G.QuantityMode=TEXT("workers");G.SourceRef=TEXT("current_camp");
    G.Original=TEXT("带两名族人一起采集木材");TestTrue(TEXT("explicit party request"),HearthwardAgent::Validate(G).IsEmpty());
    TestTrue(TEXT("requires confirmation"),G.WritesWorld());
    G.Original=TEXT("带族人去采集木材");TestEqual(TEXT("missing count does not invent labor"),HearthwardAgent::Validate(G),FString(TEXT("TEAM_COUNT_REQUIRED")));
    G.Original=TEXT("带两名族人采集32份木材");TestEqual(TEXT("quantity ceiling is not silently discarded"),HearthwardAgent::Validate(G),FString(TEXT("TEAM_AMOUNT_UNSUPPORTED")));
    G.Original=TEXT("带两名族人一起采集木材");G.Limits={TEXT("ban:wood")};TestEqual(TEXT("collection ban applies to party"),HearthwardAgent::Validate(G),FString(TEXT("POLICY_CONFLICT")));
    G.Limits.Reset();G.Quantity=3;TestEqual(TEXT("mismatched labor count refused"),HearthwardAgent::Validate(G),FString(TEXT("TEAM_COUNT_REQUIRED")));
    return true;
}
#endif
