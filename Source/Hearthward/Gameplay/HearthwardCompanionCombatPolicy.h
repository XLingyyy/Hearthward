#pragma once

#include "CoreMinimal.h"
#include "HearthwardCompanionCombatPolicy.generated.h"

UENUM(BlueprintType)
enum class EHearthwardCompanionTacticalIntent : uint8
{
    Hold,
    Follow,
    Assist
};

USTRUCT(BlueprintType)
struct FHearthwardCompanionThreat
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName Id;
    UPROPERTY(BlueprintReadOnly) FVector Position = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float RemainingHealth = 0.0f;
};

USTRUCT(BlueprintType)
struct FHearthwardCompanionCombatObservation
{
    GENERATED_BODY()

    // Legacy persisted order: wait/follow/attack.
    UPROPERTY(BlueprintReadOnly) FName RequestedOrder = TEXT("wait");
    UPROPERTY(BlueprintReadOnly) float PlayerHealthRatio = 1.0f;
    UPROPERTY(BlueprintReadOnly) bool bPlayerInCombat = false;
    UPROPERTY(BlueprintReadOnly) FVector PlayerPosition = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector CompanionPosition = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float CompanionToPlayerDistance = 0.0f;
    UPROPERTY(BlueprintReadOnly) float CommandRange = 0.0f;
    UPROPERTY(BlueprintReadOnly) TArray<FHearthwardCompanionThreat> Threats;
};

USTRUCT(BlueprintType)
struct FHearthwardCompanionCombatDecision
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) EHearthwardCompanionTacticalIntent Intent = EHearthwardCompanionTacticalIntent::Hold;
    UPROPERTY(BlueprintReadOnly) FName Target;
    UPROPERTY(BlueprintReadOnly) FString Reason;
};

namespace HearthwardCombatPolicy
{
    FHearthwardCompanionCombatDecision Evaluate(const FHearthwardCompanionCombatObservation& Observation);
    FName IntentName(EHearthwardCompanionTacticalIntent Intent);
}
