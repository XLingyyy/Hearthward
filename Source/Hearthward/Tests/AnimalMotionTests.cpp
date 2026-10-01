#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Animals/HearthwardAnimalBounds.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimalBoundaryTest,"Hearthward.Animals.FrameBoundaryAndEscape",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAnimalBoundaryTest::RunTest(const FString& Parameters)
{
    const FBox Bounds(FVector(-1200,-1000,-180),FVector(1200,1000,-30));FRandomStream R(5182);
    for(int32 I=0;I<5000;++I)
    {
        const FVector P(R.FRandRange(-1200.f,1200.f),R.FRandRange(-1000.f,1000.f),R.FRandRange(-180.f,-30.f));
        const FVector Threat(R.FRandRange(-2500.f,2500.f),R.FRandRange(-2000.f,2000.f),100);
        const FVector Goal=HearthwardAnimalBounds::FleeGoal(Bounds,P,Threat,FVector::ForwardVector);
        if(!Bounds.IsInsideOrOn(Goal)){AddError(TEXT("Escape destination crossed habitat"));return false;}
        const FVector HitchStep=HearthwardAnimalBounds::Clamp(Bounds,P+(Goal-P).GetSafeNormal2D()*900*3.5f);
        if(!Bounds.IsInsideOrOn(HitchStep)){AddError(TEXT("Large-frame movement crossed habitat"));return false;}
    }
    const FVector Goal=HearthwardAnimalBounds::FleeGoal(Bounds,FVector(0,0,-80),FVector(-300,0,100),FVector::ForwardVector);
    TestTrue(TEXT("Unconstrained flee direction increases separation"),Goal.X>0);
    const FVector Corner=HearthwardAnimalBounds::FleeGoal(Bounds,FVector(1200,1000,-80),FVector(1000,900,100),FVector::ForwardVector);
    TestTrue(TEXT("Corner threat remains contained"),Bounds.IsInsideOrOn(Corner));return true;
}
#endif
