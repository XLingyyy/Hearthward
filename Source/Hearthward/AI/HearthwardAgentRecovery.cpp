#include "HearthwardAgentRecovery.h"

namespace
{
FHearthwardAgentRecoveryDecision Decision(EHearthwardAgentRecoveryMode Mode, const FString& Reason,
    EHearthwardAgentTarget RewindTarget = EHearthwardAgentTarget::None)
{
    FHearthwardAgentRecoveryDecision Out;
    Out.Mode = Mode;
    Out.Reason = Reason;
    Out.RewindTarget = RewindTarget;
    return Out;
}

bool IsTransientRouteFailure(const FString& Reason)
{
    return Reason == TEXT("去程受阻") || Reason == TEXT("PATH_BLOCKED");
}
}

FHearthwardAgentRecoveryDecision HearthwardRecovery::Decide(const FHearthwardAgentRecoveryContext& Context)
{
    // Physical cargo always wins over plan convenience. Once the companion owns command cargo,
    // recovery must preserve it by returning to camp or holding safely if camp itself is unavailable.
    if (Context.bHasCargo)
    {
        return Context.bCampAvailable
            ? Decision(EHearthwardAgentRecoveryMode::ReturnToCamp, TEXT("PRESERVE_CARGO"))
            : Decision(EHearthwardAgentRecoveryMode::Hold, TEXT("CAMP_UNAVAILABLE_WITH_CARGO"));
    }

    const bool HasAdaptiveBudget = Context.AdaptiveAttempts < Context.MaxAdaptiveAttempts;
    if (!HasAdaptiveBudget)
    {
        return Context.bCampAvailable
            ? Decision(EHearthwardAgentRecoveryMode::ReturnToCamp, TEXT("REPLAN_BUDGET_EXHAUSTED"))
            : Decision(EHearthwardAgentRecoveryMode::Hold, TEXT("REPLAN_BUDGET_EXHAUSTED_NO_CAMP"));
    }

    if ((Context.FailureReason == TEXT("SOURCE_POSITION_CHANGED")
            || Context.FailureReason == TEXT("采集被中断"))
        && Context.bSourceAvailable)
    {
        return Decision(EHearthwardAgentRecoveryMode::RewindToMove,
            TEXT("REACQUIRE_SOURCE"), EHearthwardAgentTarget::Source);
    }

    if (IsTransientRouteFailure(Context.FailureReason))
    {
        using A = EHearthwardAgentActionType;
        using T = EHearthwardAgentTarget;

        if (Context.FailedAction.Type == A::MoveTo)
        {
            const bool TargetAvailable =
                (Context.FailedAction.Target == T::Source && Context.bSourceAvailable)
                || (Context.FailedAction.Target == T::Camp && Context.bCampAvailable)
                || (Context.FailedAction.Target == T::Workshop && Context.bStationAvailable);
            if (TargetAvailable)
                return Decision(EHearthwardAgentRecoveryMode::RetryCurrent, TEXT("RETRY_ROUTE"));
        }

        if (Context.FailedAction.Target == T::Workshop && Context.bStationAvailable)
            return Decision(EHearthwardAgentRecoveryMode::RewindToMove,
                TEXT("REACQUIRE_WORKSHOP"), T::Workshop);

        if (Context.FailedAction.Target == T::Camp && Context.bCampAvailable)
            return Decision(EHearthwardAgentRecoveryMode::RewindToMove,
                TEXT("REACQUIRE_CAMP"), T::Camp);
    }

    // Missing/depleted targets, unsafe world state, policy conflicts and material failures are not
    // recoverable with the currently-known world model. Do not invent alternate sources or permissions.
    return Context.bCampAvailable
        ? Decision(EHearthwardAgentRecoveryMode::ReturnToCamp, TEXT("HARD_BLOCK"))
        : Decision(EHearthwardAgentRecoveryMode::Hold, TEXT("HARD_BLOCK_NO_CAMP"));
}
