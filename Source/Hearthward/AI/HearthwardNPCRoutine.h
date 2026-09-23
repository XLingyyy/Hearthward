#pragma once
#include "CoreMinimal.h"
#include "HearthwardNPCRoutine.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardNPCRoutineContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool Enabled = false;
    UPROPERTY(BlueprintReadOnly) bool bOrderIdle = true;
    UPROPERTY(BlueprintReadOnly) bool bPlayerInCombat = false;
    UPROPERTY(BlueprintReadOnly) bool bPlayerDown = false;
    UPROPERTY(BlueprintReadOnly) bool bTaskActive = false;
    UPROPERTY(BlueprintReadOnly) float CompanionToCampDistance = 0.0f;
    UPROPERTY(BlueprintReadOnly) double GameSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct FHearthwardNPCRoutineDecision
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool Active = false;
    UPROPERTY(BlueprintReadOnly) FName Activity;
    UPROPERTY(BlueprintReadOnly) FVector CampOffset = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float MoveSpeed = 120.0f;
    UPROPERTY(BlueprintReadOnly) float AcceptanceRadius = 45.0f;
};

namespace HearthwardRoutine
{
    FHearthwardNPCRoutineDecision Evaluate(const FHearthwardNPCRoutineContext& Context);
}
