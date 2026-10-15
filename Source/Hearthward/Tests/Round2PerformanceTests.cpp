#include "../Nature/HearthwardNatureActor.h"
#include "../Animals/HearthwardAnimalMotionComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2StaticTickTest,"Hearthward.Round2.Performance.StaticResourcesDoNotTickAnimalMotion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2StaticTickTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Resource=World->SpawnActor<AHearthwardNatureActor>();Resource->Configure(FGuid::NewGuid(),TEXT("resource"),TEXT("tree"));
    TestFalse(TEXT("Static resource has no per-frame movement work"),Resource->IsActorTickEnabled());
    TestFalse(TEXT("Unconfigured animal animation does not tick"),Resource->FindComponentByClass<UHearthwardAnimalMotionComponent>()->IsComponentTickEnabled());
    auto* Animal=World->SpawnActor<AHearthwardNatureActor>();Animal->Configure(FGuid::NewGuid(),TEXT("animal"),TEXT("deer"));
    TestTrue(TEXT("Live animal movement still ticks"),Animal->IsActorTickEnabled());
    TestTrue(TEXT("Successfully configured animal animation ticks"),Animal->FindComponentByClass<UHearthwardAnimalMotionComponent>()->IsComponentTickEnabled());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2TargetEnumerationTest,"Hearthward.Round2.Performance.TargetEnumerationPreservesWorldIdentityAndOrder",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2TargetEnumerationTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);auto* Other=UWorld::CreateWorld(EWorldType::Game,false);
    World->PersistentLevel->bIsVisible=true;Other->PersistentLevel->bIsVisible=true;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Other);
    ON_SCOPE_EXIT{GEngine->DestroyWorldContext(Other);Other->DestroyWorld(false);GEngine->DestroyWorldContext(World);World->DestroyWorld(false);};
    auto* Player=World->SpawnActor<AActor>();auto* Combat=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
    Combat->State.SenseRemaining=1;Combat->State.SenseRadius=5000;
    auto AddTarget=[](AActor* Owner,FName Id)
    {
        auto* Target=NewObject<UHearthwardCombatTargetComponent>(Owner);Owner->AddInstanceComponent(Target);Target->Id=Id;Target->Health=100;Target->Protected=false;Target->RegisterComponent();return Target;
    };
    auto* B=World->SpawnActor<AActor>();auto* BT=AddTarget(B,TEXT("b"));
    auto* A=World->SpawnActor<AActor>();AddTarget(A,TEXT("a"));AddTarget(A,TEXT("a_duplicate"));
    AddTarget(Other->SpawnActor<AActor>(),TEXT("foreign"));
    auto* Anonymous=World->SpawnActor<AActor>();AddTarget(Anonymous,NAME_None);AddTarget(Anonymous,TEXT("second_component"));
    auto* Protected=World->SpawnActor<AActor>();AddTarget(Protected,TEXT("protected"))->Protected=true;
    auto Targets=Combat->SensedTargets();
    TestTrue(TEXT("Only first identified target per current-world actor is returned, sorted by identity"),Targets.Num()==2 && Targets[0]==A && Targets[1]==B);
    TestTrue(TEXT("Original actor iterator exposes the visible fixture"),bool(TActorIterator<AActor>(World)));
    auto* Level=World->PersistentLevel.Get();
    Level->bIsVisible=false;
    TestFalse(TEXT("Original actor iterator excludes the hidden level"),bool(TActorIterator<AActor>(World)));
    TestTrue(TEXT("Component query also excludes hidden levels"),Combat->SensedTargets().IsEmpty());
    Level->bIsAssociatingLevel=true;
    TestEqual(TEXT("Associating level retains original actor iterator visibility semantics"),Combat->SensedTargets().Num(),2);
    Level->bIsAssociatingLevel=false;Level->bIsVisible=true;Level->bIsBeingRemoved=true;
    TestFalse(TEXT("Original actor iterator excludes a level being removed"),bool(TActorIterator<AActor>(World)));
    TestTrue(TEXT("Component query also excludes levels being removed"),Combat->SensedTargets().IsEmpty());
    Level->bIsDisassociatingLevel=true;
    TestEqual(TEXT("Disassociating level remains queryable while UE exposes its actors"),Combat->SensedTargets().Num(),2);
    Level->bIsDisassociatingLevel=false;Level->bIsBeingRemoved=false;
    BT->Health=0;Targets=Combat->SensedTargets();TestTrue(TEXT("Death is reflected without stale cache"),Targets.Num()==1 && Targets[0]==A);
    A->Destroy();TestTrue(TEXT("Destroyed targets disappear immediately"),Combat->SensedTargets().IsEmpty());
    return true;
}
#endif
