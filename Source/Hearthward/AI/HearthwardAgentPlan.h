#pragma once

#include "CoreMinimal.h"
#include "HearthwardAgentContract.h"
#include "HearthwardAgentPlan.generated.h"

UENUM(BlueprintType)
enum class EHearthwardAgentActionType : uint8
{
    MoveTo,
    Gather,
    TakeMaterials,
    CommitWorkshop,
    Deposit
};

UENUM(BlueprintType)
enum class EHearthwardAgentTarget : uint8
{
    None,
    Source,
    Camp,
    Workshop
};

USTRUCT(BlueprintType)
struct FHearthwardAgentAction
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) EHearthwardAgentActionType Type = EHearthwardAgentActionType::MoveTo;
    UPROPERTY(BlueprintReadOnly) EHearthwardAgentTarget Target = EHearthwardAgentTarget::None;

    bool Matches(EHearthwardAgentActionType InType, EHearthwardAgentTarget InTarget = EHearthwardAgentTarget::None) const
    { return Type == InType && (InTarget == EHearthwardAgentTarget::None || Target == InTarget); }
};

USTRUCT(BlueprintType)
struct FHearthwardAgentPlan
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 Version = 1;
    UPROPERTY(BlueprintReadOnly) TArray<FHearthwardAgentAction> Actions;

    bool IsValid() const { return Version == 1 && !Actions.IsEmpty(); }
};

struct FHearthwardAgentExecutionState
{
    FHearthwardAgentPlan Plan;
    int32 Cursor = INDEX_NONE;
    bool bRecoveryToCamp = false;
    bool bStarted = false;

    void Reset()
    {
        Plan = {};
        Cursor = INDEX_NONE;
        bRecoveryToCamp = false;
        bStarted = false;
    }

    const FHearthwardAgentAction* Current() const
    {
        return Plan.Actions.IsValidIndex(Cursor) ? &Plan.Actions[Cursor] : nullptr;
    }
};

namespace HearthwardPlan
{
    bool Build(const FHearthwardAgentGoal& Goal, FHearthwardAgentPlan& Out, FString& Error);
    int32 Find(const FHearthwardAgentPlan& Plan, EHearthwardAgentActionType Type,
        EHearthwardAgentTarget Target = EHearthwardAgentTarget::None, int32 StartAt = 0);
    int32 FindLast(const FHearthwardAgentPlan& Plan, EHearthwardAgentActionType Type,
        EHearthwardAgentTarget Target = EHearthwardAgentTarget::None);
    FString ActionName(const FHearthwardAgentAction& Action);
}
