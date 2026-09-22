#pragma once

#include "CoreMinimal.h"

class AHearthwardCompanionFixture;

struct FHearthwardCompanionBehaviorThreat
{
    FName Id;
    TWeakObjectPtr<AActor> Actor;
    float RemainingHealth=0;
};

struct FHearthwardCompanionBehaviorContext
{
    AActor* Player=nullptr;
    AHearthwardCompanionFixture* Companion=nullptr;
    FName RequestedOrder=TEXT("wait");
    bool bRoutineEnabled=false;
    bool bPlayerInCombat=false;
    bool bPlayerDown=false;
    bool bCanAttack=false;
    float PlayerHealthRatio=1.0f;
    double GameSeconds=0;
    TArray<FHearthwardCompanionBehaviorThreat> Threats;
};

struct FHearthwardCompanionBehaviorResult
{
    FName EffectiveOrder=TEXT("wait");
    FName TacticalIntent=TEXT("hold");
    FName CombatTarget;
    FString Reason=TEXT("COMPANION_UNAVAILABLE");
    FName RoutineActivity;
    bool bAttackCommitted=false;
    FName DamageTarget;
};

namespace HearthwardCompanionBehavior
{
    FHearthwardCompanionBehaviorResult Tick(const FHearthwardCompanionBehaviorContext& Context);
}
