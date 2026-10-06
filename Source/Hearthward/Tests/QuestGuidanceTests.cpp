#include "../UI/HearthwardQuestGuidance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuestMarkerPlacementTest,"Hearthward.Quest080.MarkerPlacement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FQuestMarkerPlacementTest::RunTest(const FString&)
{
    for(FVector2D Size:{FVector2D(1672,941),FVector2D(1280,720),FVector2D(2560,1080),FVector2D(1024,768)})
    {
        FHearthwardQuestMarker M;const FVector2D Center=Size*.5;
        HearthwardQuestGuidance::Place(M,Size,Center,true,{});
        TestFalse(TEXT("Visible target stays at projection"),M.Offscreen);
        TestTrue(TEXT("Projection preserved"),M.Position.Equals(Center));
        HearthwardQuestGuidance::Place(M,Size,FVector2D(Size.X*2,Center.Y),true,{});
        TestTrue(TEXT("Offscreen target clamped inside right edge"),M.Offscreen && M.Position.X<Size.X && M.Position.X>Center.X);
        TestTrue(TEXT("Right arrow points right"),M.Direction.Equals(FVector2D(1,0)));
        HearthwardQuestGuidance::Place(M,Size,{},false,FVector2D(-10,0));
        TestTrue(TEXT("Behind-left remains left"),M.Position.X>0 && M.Position.X<Center.X && M.Direction.X<0);
        HearthwardQuestGuidance::Place(M,Size,{},false,{});
        TestTrue(TEXT("Exactly behind has stable bottom direction"),M.Position.Y>Center.Y && M.Position.Y<Size.Y && M.Direction.Equals(FVector2D(0,1)));
        HearthwardQuestGuidance::Place(M,Size,FVector2D(-Size.X,-Size.Y),true,{});
        TestTrue(TEXT("Diagonal remains inside viewport"),M.Position.X>0 && M.Position.Y>0);
    }
    TestFalse(TEXT("Missing player hides marker"),HearthwardQuestGuidance::Resolve(nullptr).Visible);
    return true;
}
#endif
