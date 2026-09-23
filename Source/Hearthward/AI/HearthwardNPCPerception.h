#pragma once

#include "CoreMinimal.h"
#include "HearthwardAgentContract.h"
#include "HearthwardNPCPerception.generated.h"

class AHearthwardCompanionFixture;

USTRUCT(BlueprintType)
struct FHearthwardNPCObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bWorldAvailable = false;
    UPROPERTY(BlueprintReadOnly) bool bPaused = false;
    UPROPERTY(BlueprintReadOnly) bool bCombatStateAvailable = false;
    UPROPERTY(BlueprintReadOnly) bool bCombatActive = false;
    UPROPERTY(BlueprintReadOnly) bool bCampAvailable = false;
    UPROPERTY(BlueprintReadOnly) bool bCollectionSourceAvailable = false;
    // PROTOTYPE_ONLY evidence supplied by the approved safe collection fixture.
    // It is an observation input, not the final safety verdict.
    UPROPERTY(BlueprintReadOnly) bool bCollectionSourceTrustedSafe = false;
    UPROPERTY(BlueprintReadOnly) bool bNavigationRebuilding = false;
    UPROPERTY(BlueprintReadOnly) bool bAtCamp = false;
    UPROPERTY(BlueprintReadOnly) float CampDistanceCm = -1.0f;
    UPROPERTY(BlueprintReadOnly) float CollectionSourceDistanceCm = -1.0f;
    UPROPERTY(BlueprintReadOnly) FString ExecutionPhase;
    UPROPERTY(BlueprintReadOnly) FString ExecutionAction;
    UPROPERTY(BlueprintReadOnly) double CapturedAtWorldSeconds = 0.0;
};

enum class EHearthwardNPCSafetyVerdict : uint8
{
    Allowed,
    Unsafe,
    Unavailable
};

struct FHearthwardNPCSafetyDecision
{
    EHearthwardNPCSafetyVerdict Verdict = EHearthwardNPCSafetyVerdict::Unavailable;
    FString Reason;

    bool IsAllowed() const { return Verdict == EHearthwardNPCSafetyVerdict::Allowed; }
};

namespace HearthwardPerception
{
    // Reads only UE-authoritative runtime state. Player/model text is intentionally absent.
    FHearthwardNPCObservation Capture(const AHearthwardCompanionFixture* Companion);

    // Pure policy function: safe to exercise in automation without a world or model.
    FHearthwardNPCSafetyDecision Evaluate(const FHearthwardNPCObservation& Observation, const FHearthwardAgentGoal& Goal);

    FString VerdictName(EHearthwardNPCSafetyVerdict Verdict);
}
