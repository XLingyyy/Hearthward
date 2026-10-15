#include "../Combat/HearthwardCombatComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2RetreatTest,"Hearthward.Round2.Combat.RetreatPhasesAndCancellation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2RetreatTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT{GEngine->DestroyWorldContext(World);World->DestroyWorld(false);};
    auto* Player=World->SpawnActor<ACharacter>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Game=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Game);Game->RegisterComponent();Game->Enabled=true;
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Combat=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
    Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    const FName Weapons[]={TEXT("shortblade"),TEXT("axe"),TEXT("spear")};
    const FName Kinds[]={TEXT("shortblade"),TEXT("blunt"),TEXT("spear")};
    const double Scales[]={.85,.55,.70};
    for(int32 I=0;I<3;++I)
    {
        Bag->TryAdd(Weapons[I],1);Game->CommitEquipment(Weapons[I]);
        for(bool Heavy:{false,true})
        {
            Game->Stamina=100;
            if(!TestTrue(TEXT("Real equipped attack begins"),Combat->Attack(Heavy)))return false;
            const auto Move=HearthwardCombat::Move(Kinds[I],Heavy);
            TestFalse(TEXT("Normal attack remains movable"),Combat->MovementLocked());
            TestTrue(TEXT("Windup retreat unchanged"),Combat->AttackMovementInput({0,-1}).Equals({0,-1}));
            for(double Time:{Move.Windup,Move.Windup+Move.Active,Move.Duration()-.001})
            {
                Combat->Elapsed=Time;
                TestTrue(TEXT("Approved effective/recovery retreat scale"),FMath::IsNearlyEqual(Combat->AttackMovementInput({0,-1}).Y,-Scales[I],.0001));
                TestTrue(TEXT("Forward unchanged"),Combat->AttackMovementInput({0,1}).Equals({0,1}));
                TestTrue(TEXT("Sideways unchanged"),Combat->AttackMovementInput({1,0}).Equals({1,0}));
                const auto Diagonal=Combat->AttackMovementInput({1,-1});
                TestTrue(TEXT("Diagonal keeps baseline lateral component"),FMath::IsNearlyEqual(Diagonal.X,1/FMath::Sqrt(2.),.0001));
                TestTrue(TEXT("Diagonal retreat component scaled without re-normalization"),FMath::IsNearlyEqual(Diagonal.Y,-Scales[I]/FMath::Sqrt(2.),.0001));
            }
            Combat->Cancel();
            TestTrue(TEXT("Cancellation restores movement immediately"),Combat->AttackMovementInput({0,-1}).Equals({0,-1}));
        }
    }
    return true;
}
#endif
