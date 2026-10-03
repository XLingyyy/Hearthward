#include "../Time/HearthwardClockState.h"
#include "Misc/AutomationTest.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClockOriginTest,"Hearthward.Time.DisplayOriginAndGlobalCycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FClockOriginTest::RunTest(const FString&)
{
    FHearthwardClockState Clock;Clock.SetOrigin(1,1200);Clock.Advance(480);
    TestEqual(TEXT("Elapsed days do not include display origin"),Clock.GetElapsedDays(),int64(0));
    TestEqual(TEXT("20:00 plus eight hours is next-day 04:00"),Clock.GetDisplayDay(),int64(2));
    TestEqual(TEXT("Display minute wraps with origin"),Clock.GetMinuteOfDay(),240.);
    TestEqual(TEXT("Dawn midpoint"),UHearthwardWorldClockSubsystem::DaylightAt(360),.5);
    TestEqual(TEXT("Dusk midpoint"),UHearthwardWorldClockSubsystem::DaylightAt(1080),.5);
    TestEqual(TEXT("Before dawn transition"),UHearthwardWorldClockSubsystem::DaylightAt(345),0.);
    TestEqual(TEXT("After dawn transition"),UHearthwardWorldClockSubsystem::DaylightAt(375),1.);
    TestEqual(TEXT("Field death before cycle uses global boundary"),FHearthwardClockState::FieldRefreshDue(4320),5760.);
    TestEqual(TEXT("Death at cycle waits for next cycle"),FHearthwardClockState::FieldRefreshDue(5760),11520.);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClockDomainsTest,"Hearthward.Time.DomainPartitionRefreshAndEpoch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FClockDomainsTest::RunTest(const FString&)
{
    TArray<UWorld*> Worlds;
    for(int32 I=0;I<2;++I)
    {
        auto* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);Worlds.Add(W);
        auto* C=W->GetSubsystem<UHearthwardCampSubsystem>();C->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
        FHearthwardCampSource Source;Source.Id=TEXT("shared-tree");Source.Camp=TEXT("camp");Source.Item=TEXT("wood");Source.Capacity=Source.Remaining=12;Source.RefreshMinutes=2880;C->State.Sources.Add(Source);
        C->State.Regions[0].Workers={0};C->State.Regions[0].Job=TEXT("wood");C->State.Regions[0].Enabled=true;
        auto* N=W->GetSubsystem<UHearthwardNatureSubsystem>();N->State.Seed=1;
        FHearthwardCampaignEnemy E;E.Id=TEXT("field-cycle");E.Kind=TEXT("guard");E.Stage=1;E.Group=TEXT("field");E.RefreshDue=5760;E.Combat.Health=0;
        W->GetSubsystem<UHearthwardCampaignSubsystem>()->State.Enemies.Add(E);
        auto Live=E;Live.Id=TEXT("live-field");Live.RefreshDue=-1;Live.Combat.Health=37;
        W->GetSubsystem<UHearthwardCampaignSubsystem>()->State.Enemies.Add(Live);
        auto Home=E;Home.Id=TEXT("permanent-home-clear");Home.Group=TEXT("base");Home.RefreshDue=-1;Home.Combat.bStunned=true;
        W->GetSubsystem<UHearthwardCampaignSubsystem>()->State.Enemies.Add(Home);
    }
    auto* A=Worlds[0]->GetSubsystem<UHearthwardWorldClockSubsystem>();auto* B=Worlds[1]->GetSubsystem<UHearthwardWorldClockSubsystem>();
    A->AdvanceCalendar(18720);
    for(int32 I=0;I<78;++I)B->AdvanceCalendar(240);
    const auto& CA=Worlds[0]->GetSubsystem<UHearthwardCampSubsystem>()->State;const auto& CB=Worlds[1]->GetSubsystem<UHearthwardCampSubsystem>()->State;
    TestEqual(TEXT("Long and segmented advance produce identical shared wood"),Worlds[0]->GetSubsystem<UHearthwardStorageSubsystem>()->GetItemCount(TEXT("wood")),Worlds[1]->GetSubsystem<UHearthwardStorageSubsystem>()->GetItemCount(TEXT("wood")));
    TestEqual(TEXT("Same finite source remaining"),CA.Sources[0].Remaining,CB.Sources[0].Remaining);
    TestEqual(TEXT("Same batches"),CA.Regions[0].Completed,CB.Regions[0].Completed);
    TestEqual(TEXT("Source schedule stays equivalent"),CA.Sources[0].Due,CB.Sources[0].Due);
    const auto& E=Worlds[0]->GetSubsystem<UHearthwardCampaignSubsystem>()->State.Enemies[0];
    TestEqual(TEXT("Three cycles without deaths create only one replacement"),E.Combat.Generation,2);
    const auto& Enemies=Worlds[0]->GetSubsystem<UHearthwardCampaignSubsystem>()->State.Enemies;
    TestEqual(TEXT("A living field enemy cannot heal at another cycle"),Enemies[1].Combat.Health,37.f);
    TestEqual(TEXT("A living field enemy keeps its generation"),Enemies[1].Combat.Generation,1);
    TestTrue(TEXT("A hometown clear remains terminal across three cycles"),Enemies[2].Combat.Health==0 && Enemies[2].Combat.bStunned && Enemies[2].Combat.Generation==1);
    TestEqual(TEXT("Refreshing cannot award combat XP"),Worlds[0]->GetSubsystem<UHearthwardCampaignSubsystem>()->State.Facts.Num(),0);
    TestEqual(TEXT("Jump leaves action clock unchanged"),A->GetSnapshot().ActivePlaySeconds,0.);
    TestEqual(TEXT("Camp cursor matches W"),CA.Calendar,A->GetSnapshot().ElapsedCalendarMinutes);
    TestEqual(TEXT("Nature cursor matches W"),Worlds[0]->GetSubsystem<UHearthwardNatureSubsystem>()->State.Calendar,A->GetSnapshot().ElapsedCalendarMinutes);
    FHearthwardTimeAdvanceRequest R;R.OperationId=FGuid::NewGuid();R.Epoch=Worlds[0]->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();R.StartW=A->GetSnapshot().ElapsedCalendarMinutes;
    const auto Rejected=A->RequestTimeAdvance(R);
    TestFalse(TEXT("Facility-free production request is rejected"),Rejected.Accepted);
    TestEqual(TEXT("Identical rejection returns the same receipt"),A->RequestTimeAdvance(R).Reason,Rejected.Reason);
    R.Minutes=60;
    TestTrue(TEXT("Same operation with different payload is rejected"),A->RequestTimeAdvance(R).Reason.Contains(TEXT("重复请求")));
    Worlds[0]->GetSubsystem<UHearthwardStorageSubsystem>()->AdvanceTimeline();
    TestFalse(TEXT("Old epoch is rejected"),A->RequestTimeAdvance(R).Accepted);
    TestEqual(TEXT("Rejected request cannot advance W"),A->GetSnapshot().ElapsedCalendarMinutes,18720.);
    for(auto* W:Worlds){GEngine->DestroyWorldContext(W);W->DestroyWorld(false);}return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClockSurvivalSkillTest,"Hearthward.Time.JumpUsesSurvivalSkills",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FClockSurvivalSkillTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Actor=World->SpawnActor<AActor>();
    auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Actor);Actor->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;Gameplay->Health=10;Gameplay->Hunger=10;Gameplay->Skills.Add(TEXT("sustain"),3);
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Actor);Actor->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto Expected=Survival->State;float HP=Gameplay->Health,Hunger=Gameplay->Hunger;
    const double Fraction=Expected.Advance(HP,Hunger,Survival->MaxHealth(),0,6000,0,1-Gameplay->Effect(TEXT("hunger")));
    const double Advanced=World->GetSubsystem<UHearthwardWorldClockSubsystem>()->AdvanceCalendar(6000);
    TestTrue(TEXT("The whole world stops at the actual skilled hunger deadline"),FMath::IsNearlyEqual(Advanced,6000*Fraction,1.e-4));
    TestTrue(TEXT("A stopped-at-failure jump has actually reached death"),Survival->State.Life==EHearthwardLife::Dead);
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClockRatioTest, "Hearthward.Time.RatioAndDayBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FClockRatioTest::RunTest(const FString& Parameters)
{
    FHearthwardClockState Clock;
    Clock.Advance(60.0);
    TestEqual(TEXT("One minute of play stays 60 action seconds"), Clock.GetActivePlaySeconds(), 60.0);
    TestEqual(TEXT("One minute of play gives 60 calendar minutes"), Clock.GetElapsedCalendarMinutes(), 60.0);
    Clock.Advance(1379.75);
    TestEqual(TEXT("Before boundary"), Clock.GetElapsedDays(), int64(0));
    Clock.Advance(0.25);
    TestEqual(TEXT("24 real minutes yield one calendar day"), Clock.GetElapsedDays(), int64(1));
    TestEqual(TEXT("Day remainder wraps"), Clock.GetMinuteOfDay(), 0.0);
    Clock.Advance(2880.5);
    TestEqual(TEXT("A delta may cross multiple days"), Clock.GetElapsedDays(), int64(3));
    TestEqual(TEXT("Fractional minute preserved"), Clock.GetMinuteOfDay(), 0.5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClockPartitionTest, "Hearthward.Time.FramePartitionAndIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FClockPartitionTest::RunTest(const FString& Parameters)
{
    FHearthwardClockState Clock;
    for (int32 Frame = 0; Frame < 86400; ++Frame)
    {
        Clock.Advance(1.0 / 60.0);
    }
    TestTrue(TEXT("60 fps for 24 minutes preserves duration within 1 microsecond"),
        FMath::Abs(Clock.GetActivePlaySeconds() - 1440.0) < 0.000001);
    const double Before = Clock.GetActivePlaySeconds();
    Clock.Advance(0.0);
    TestEqual(TEXT("No elapsed delta means no advancement"), Clock.GetActivePlaySeconds(), Before);
    FHearthwardClockState OtherWorld;
    TestEqual(TEXT("New world starts with no elapsed time"), OtherWorld.GetActivePlaySeconds(), 0.0);
    OtherWorld.Advance(3.0);
    TestEqual(TEXT("World states are independent"), Clock.GetActivePlaySeconds(), Before);
    return true;
}
#endif
