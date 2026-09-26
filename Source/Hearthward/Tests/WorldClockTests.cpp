#include "../Time/HearthwardClockState.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClockSkipRestoreTest, "Hearthward.Time.SleepCalendarAndRollback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FClockSkipRestoreTest::RunTest(const FString& Parameters)
{
    FHearthwardClockState Clock;
    Clock.Advance(1200);
    TestTrue(TEXT("Eight hour sleep accepted by calendar state"),Clock.Sleep());
    TestEqual(TEXT("Sleep does not run action timers"),Clock.GetActivePlaySeconds(),1200.0);
    TestEqual(TEXT("Sleep crosses midnight by actual clock"),Clock.GetElapsedDays(),int64(1));
    TestEqual(TEXT("20:00 plus eight hours is 04:00"),Clock.GetMinuteOfDay(),240.0);
    Clock.Sleep();
    TestEqual(TEXT("A second sleep stays on the same date"),Clock.GetElapsedDays(),int64(1));
    TestEqual(TEXT("04:00 plus eight hours is noon"),Clock.GetMinuteOfDay(),720.0);
    Clock.AdvanceCalendar(1440*5);
    TestEqual(TEXT("Multiple periods retained"),Clock.GetElapsedDays(),int64(6));
    TestTrue(TEXT("Restore earlier independent A/W"),Clock.Restore(20,500));
    TestEqual(TEXT("No future calendar carried across rollback"),Clock.GetElapsedCalendarMinutes(),500.0);
    TestFalse(TEXT("Negative advancement rejected"),Clock.Advance(-1));
    TestFalse(TEXT("Invalid snapshot rejected"),Clock.Restore(501,500));
    TestEqual(TEXT("Rejected mutation leaves state intact"),Clock.GetActivePlaySeconds(),20.0);
    FHearthwardClockState Partitioned,Single;
    for(int32 I=0;I<9;++I) Partitioned.Sleep();
    Single.AdvanceCalendar(4320);
    TestEqual(TEXT("Partitioned calendar jumps match"),Partitioned.GetElapsedCalendarMinutes(),Single.GetElapsedCalendarMinutes());
    return true;
}
#endif
