#include "../Camp/HearthwardCampSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHarvestRefreshIdentityTest,"Hearthward.Time.HarvestRefreshIdentityAndBlock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHarvestRefreshIdentityTest::RunTest(const FString&)
{
    auto* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
    auto* Camp=W->GetSubsystem<UHearthwardCampSubsystem>();Camp->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
    Camp->RegisterSource(TEXT("authored-tree-instance"),TEXT("wood"),12,6,FVector::ZeroVector,2880);
    Camp->Advance(6000,true);
    auto* Source=Camp->Source(TEXT("authored-tree-instance"));
    TestEqual(TEXT("Partial harvest never starts a refresh"),Source->Due,-1.);
    TestEqual(TEXT("Partial patch is not replenished by elapsed days"),Source->Remaining,6);
    Camp->RegisterSource(TEXT("authored-tree-instance"),TEXT("wood"),12,12,FVector::ZeroVector,2880);
    TestEqual(TEXT("Streaming rediscovery cannot refill the shared patch"),Source->Remaining,6);
    TestEqual(TEXT("One persistent authored identity"),Camp->State.Sources.Num(),1);
    Source->Remaining=0;Source->Due=Camp->State.Calendar+2880;Source->Blocked=true;
    const double Due=Source->Due;
    Camp->State.Advance(2880,true,[](const auto&,const auto&){return true;});
    TestEqual(TEXT("Blocking preserves original deadline"),Source->Due,Due);
    TestEqual(TEXT("Blocked patch stays exhausted at due"),Source->Remaining,0);
    Source->Blocked=false;Camp->State.Advance(0,true,[](const auto&,const auto&){return true;});
    TestEqual(TEXT("Unblocking settles the already due boundary"),Source->Remaining,12);
    Camp->State.Advance(40*1440,true,[](const auto&,const auto&){return true;});
    TestEqual(TEXT("Forty idle days never accumulate capacity"),Source->Remaining,12);
    TestEqual(TEXT("Settled deadline is cleared exactly once"),Source->Due,-1.);
    FHearthwardCampState Restored;
    TestTrue(TEXT("Source identity and capacity round trip"),FHearthwardCampState::Parse(Camp->State.Snapshot(),Restored));
    TestTrue(TEXT("Restoring cannot create another patch"),Restored.Sources.Num()==1 && Restored.Sources[0].Id==Source->Id && Restored.Sources[0].Remaining==12);
    GEngine->DestroyWorldContext(W);W->DestroyWorld(false);return true;
}
#endif
