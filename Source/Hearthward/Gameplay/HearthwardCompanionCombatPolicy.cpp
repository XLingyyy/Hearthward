#include "HearthwardCompanionCombatPolicy.h"

namespace
{
bool BetterThreat(const FHearthwardCompanionThreat& Candidate, const FHearthwardCompanionThreat& Current,
    const FHearthwardCompanionCombatObservation& Observation)
{
    const float CandidatePlayer = FVector::DistSquared2D(Candidate.Position, Observation.PlayerPosition);
    const float CurrentPlayer = FVector::DistSquared2D(Current.Position, Observation.PlayerPosition);
    if (!FMath::IsNearlyEqual(CandidatePlayer, CurrentPlayer)) return CandidatePlayer < CurrentPlayer;

    const float CandidateCompanion = FVector::DistSquared2D(Candidate.Position, Observation.CompanionPosition);
    const float CurrentCompanion = FVector::DistSquared2D(Current.Position, Observation.CompanionPosition);
    if (!FMath::IsNearlyEqual(CandidateCompanion, CurrentCompanion)) return CandidateCompanion < CurrentCompanion;

    return Candidate.Id.ToString() < Current.Id.ToString();
}
}

FHearthwardCompanionCombatDecision HearthwardCombatPolicy::Evaluate(const FHearthwardCompanionCombatObservation& O)
{
    FHearthwardCompanionCombatDecision Decision;

    if (O.PlayerHealthRatio <= 0.0f)
    {
        Decision.Reason = TEXT("PLAYER_DOWN");
        return Decision;
    }

    if (O.RequestedOrder == TEXT("wait"))
    {
        Decision.Reason = TEXT("EXPLICIT_HOLD");
        return Decision;
    }

    if (O.RequestedOrder == TEXT("follow"))
    {
        Decision.Intent = EHearthwardCompanionTacticalIntent::Follow;
        Decision.Reason = TEXT("EXPLICIT_FOLLOW");
        return Decision;
    }

    if (O.RequestedOrder != TEXT("attack"))
    {
        Decision.Reason = TEXT("UNKNOWN_ORDER");
        return Decision;
    }

    const float CommandRange = FMath::Max(0.0f, O.CommandRange);
    if (O.CompanionToPlayerDistance > CommandRange)
    {
        Decision.Intent = EHearthwardCompanionTacticalIntent::Follow;
        Decision.Reason = TEXT("RETURN_TO_PLAYER_LEASH");
        return Decision;
    }

    const float RangeSq = FMath::Square(CommandRange);
    const FHearthwardCompanionThreat* Best = nullptr;
    for (const auto& Threat : O.Threats)
    {
        if (Threat.Id.IsNone() || Threat.RemainingHealth <= 0.0f) continue;
        if (FVector::DistSquared2D(Threat.Position, O.PlayerPosition) > RangeSq) continue;
        if (!Best || BetterThreat(Threat, *Best, O)) Best = &Threat;
    }

    if (!Best)
    {
        Decision.Intent = EHearthwardCompanionTacticalIntent::Follow;
        Decision.Reason = TEXT("NO_THREAT_IN_LEASH");
        return Decision;
    }

    Decision.Intent = EHearthwardCompanionTacticalIntent::Assist;
    Decision.Target = Best->Id;
    Decision.Reason = O.bPlayerInCombat ? TEXT("ASSIST_ACTIVE_COMBAT") : TEXT("ASSIST_NEAREST_PLAYER_THREAT");
    return Decision;
}

FName HearthwardCombatPolicy::IntentName(EHearthwardCompanionTacticalIntent Intent)
{
    switch (Intent)
    {
    case EHearthwardCompanionTacticalIntent::Hold: return TEXT("hold");
    case EHearthwardCompanionTacticalIntent::Follow: return TEXT("follow");
    case EHearthwardCompanionTacticalIntent::Assist: return TEXT("assist");
    default: return TEXT("hold");
    }
}
