#pragma once

#include "CoreMinimal.h"
#include "HearthwardAgentPlan.h"
#include "HearthwardAgentRecovery.generated.h"

UENUM(BlueprintType)
enum class EHearthwardAgentRecoveryMode : uint8
{
    RetryCurrent,
    RewindToMove,
    ReturnToCamp,
    Hold
};

USTRUCT(BlueprintType)
struct FHearthwardAgentRecoveryContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FHearthwardAgentAction FailedAction;
    UPROPERTY(BlueprintReadOnly) FString FailureReason;
    UPROPERTY(BlueprintReadOnly) bool bHasCargo = false;
    UPROPERTY(BlueprintReadOnly) bool bCampAvailable = false;
    UPROPERTY(BlueprintReadOnly) bool bSourceAvailable = false;
    UPROPERTY(BlueprintReadOnly) bool bStationAvailable = false;
    UPROPERTY(BlueprintReadOnly) int32 AdaptiveAttempts = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxAdaptiveAttempts = 0;
};

USTRUCT(BlueprintType)
struct FHearthwardAgentRecoveryDecision
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) EHearthwardAgentRecoveryMode Mode = EHearthwardAgentRecoveryMode::ReturnToCamp;
    UPROPERTY(BlueprintReadOnly) EHearthwardAgentTarget RewindTarget = EHearthwardAgentTarget::None;
    UPROPERTY(BlueprintReadOnly) FString Reason;

    bool IsAdaptive() const
    {
        return Mode == EHearthwardAgentRecoveryMode::RetryCurrent
            || Mode == EHearthwardAgentRecoveryMode::RewindToMove;
    }
};

namespace HearthwardRecovery
{
    // Pure policy: decide how an executor should recover from a failed action.
    // It never changes world state, navigation, inventory, command state, or memory.
    FHearthwardAgentRecoveryDecision Decide(const FHearthwardAgentRecoveryContext& Context);
}
