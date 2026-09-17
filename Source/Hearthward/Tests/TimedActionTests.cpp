#include "../Actions/HearthwardTimedActionState.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimedActionBoundaryTest, "Hearthward.Actions.BoundaryAndCompletion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTimedActionBoundaryTest::RunTest(const FString& Parameters)
{
    FHearthwardTimedActionState State;
    TestFalse(TEXT("Idle cannot complete"), State.Update(100.0));
    TestTrue(TEXT("Start at an existing world time"), State.Start(100.0));
    TestFalse(TEXT("Not complete before five active seconds"), State.Update(104.999));
    TestFalse(TEXT("Duplicate start is rejected"), State.Start(104.999));
    TestTrue(TEXT("Exactly five seconds completes original timer"), State.Update(105.0));
    TestFalse(TEXT("Completion emitted once"), State.Update(200.0));
    TestFalse(TEXT("Completed action cannot be interrupted"), State.Interrupt(200.0));
    TestEqual(TEXT("Elapsed is clamped"), State.ElapsedSeconds, 5.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimedActionRestartTest, "Hearthward.Actions.InterruptionPauseAndRestart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTimedActionRestartTest::RunTest(const FString& Parameters)
{
    FHearthwardTimedActionState State;
    State.Start(20.0);
    State.Update(22.0);
    State.Update(22.0);
    TestEqual(TEXT("Frozen clock keeps progress"), State.ElapsedSeconds, 2.0);
    TestTrue(TEXT("Interruption accepted"), State.Interrupt(22.0));
    TestFalse(TEXT("Interruption emitted once"), State.Interrupt(23.0));
    TestFalse(TEXT("Interrupted action cannot complete"), State.Update(100.0));
    TestTrue(TEXT("Can restart"), State.Start(100.0));
    TestEqual(TEXT("Restart clears progress"), State.ElapsedSeconds, 0.0);
    TestFalse(TEXT("Old progress not carried"), State.Update(103.0));
    TestTrue(TEXT("Overshoot completes"), State.Update(106.0));
    TestEqual(TEXT("Overshoot clamps"), State.ElapsedSeconds, 5.0);
    FHearthwardTimedActionState NewSession;
    TestTrue(TEXT("Fresh session idle"), NewSession.Status == EHearthwardTimedActionStatus::Idle);
    return true;
}
#endif
