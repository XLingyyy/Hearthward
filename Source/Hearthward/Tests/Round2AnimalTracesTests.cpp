#include "../Nature/HearthwardAnimalTraces.h"
#include "../Environment/HearthwardWeatherActor.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2TracesTest,"Hearthward.Round2.Traces.RealIdentityDeathRefreshExpiryAndBudget",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2TracesTest::RunTest(const FString&)
{
    FHearthwardNatureState Nature;FHearthwardWildSlot Slot;Slot.Id=TEXT("nature_deer0");Slot.Definition=TEXT("deer");Slot.Generation=1;Slot.Current=FGuid::NewGuid();Nature.Slots.Add(Slot);
    FHearthwardAnimal Animal;Animal.Id=Slot.Current;Animal.Slot=Slot.Id;Animal.Definition=Slot.Definition;Animal.Health=75;Nature.Animals.Add(Animal);
    FHearthwardPersistentInteractionState State;
    TestTrue(TEXT("Living animal creates first real sample"),AHearthwardAnimalTraces::Sample(Nature,0,FVector::ZeroVector,State));
    TestEqual(TEXT("Sample binds exact individual"),State.Trails[0].Animal,Animal.Id);
    Nature.Animals[0].Position=FVector(500,0,0);
    TestFalse(TEXT("Sampling interval caps high frequency updates"),AHearthwardAnimalTraces::Sample(Nature,1,FVector::ZeroVector,State));
    TestTrue(TEXT("Movement at interval produces history"),AHearthwardAnimalTraces::Sample(Nature,2,FVector::ZeroVector,State));
    Nature.Animals[0].Health=0;Nature.Animals[0].Position=FVector(1000,0,0);
    TestFalse(TEXT("Death adds no further tracking positions"),AHearthwardAnimalTraces::Sample(Nature,3,FVector::ZeroVector,State));
    const auto Old=Nature.Animals[0].Id;Nature.Animals[0].Id=FGuid::NewGuid();Nature.Animals[0].Health=75;Nature.Slots[0].Current=Nature.Animals[0].Id;Nature.Slots[0].Generation=2;
    AHearthwardAnimalTraces::Sample(Nature,4,FVector::ZeroVector,State);
    TestTrue(TEXT("Refresh separates original identity and history"),State.Trails[0].Animal!=Old && State.Trails[0].Generation==2 && State.Trails[0].Samples.Num()==1);
    for(int32 I=1;I<=30;++I){Nature.Animals[0].Position.X+=250;AHearthwardAnimalTraces::Sample(Nature,4+I*2,FVector::ZeroVector,State);}
    TestEqual(TEXT("Per individual budget"),State.Trails[0].Samples.Num(),24);
    Nature.Animals[0].Health=0;AHearthwardAnimalTraces::Sample(Nature,160,FVector::ZeroVector,State);TestTrue(TEXT("Expired traces disappear without creating loot"),State.Trails[0].Samples.IsEmpty());
    Nature.Animals[0].Health=75;Nature.Animals[0].Position.X=60000;AHearthwardAnimalTraces::Sample(Nature,162,FVector::ZeroVector,State);TestTrue(TEXT("Outside observation area stays untracked"),State.Trails[0].Samples.IsEmpty());
    FString Error;TestTrue(TEXT("Production trail state validates"),State.Validate(162,Error));
    State.Trails[0].Species=TEXT("wolf");TestFalse(TEXT("Other species cannot claim the deer slot"),State.Validate(162,Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2WeatherTest,"Hearthward.Round2.Weather.CalendarPhaseAndSmoothBoundary",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2WeatherTest::RunTest(const FString&)
{
    TestEqual(TEXT("Old uninitialized state is clear"),AHearthwardWeatherActor::RainAt(210,0),0.);
    TestEqual(TEXT("Clear phase"),AHearthwardWeatherActor::RainAt(100,40),0.);
    TestEqual(TEXT("Rain onset is smooth"),AHearthwardWeatherActor::RainAt(193,40),.5);
    TestEqual(TEXT("Established light rain"),AHearthwardWeatherActor::RainAt(210,40),1.);
    TestEqual(TEXT("Rain exit is smooth"),AHearthwardWeatherActor::RainAt(237,40),.5);
    TestEqual(TEXT("Saved calendar determines identical phase after two days"),AHearthwardWeatherActor::RainAt(210+2880,40),AHearthwardWeatherActor::RainAt(210,40));
    return true;
}
#endif
