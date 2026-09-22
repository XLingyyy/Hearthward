#include "HearthwardNPCPerception.h"

#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

namespace
{
bool AvailableActor(const AActor* Actor, const UWorld* World)
{
    return IsValid(Actor) && !Actor->IsActorBeingDestroyed() && Actor->GetWorld() == World;
}

FHearthwardNPCSafetyDecision Decision(EHearthwardNPCSafetyVerdict Verdict, const TCHAR* Reason)
{
    FHearthwardNPCSafetyDecision Result;
    Result.Verdict = Verdict;
    Result.Reason = Reason;
    return Result;
}
}

FHearthwardNPCObservation HearthwardPerception::Capture(const AHearthwardCompanionFixture* Companion)
{
    FHearthwardNPCObservation Observation;
    if (!IsValid(Companion) || Companion->IsActorBeingDestroyed()) return Observation;

    UWorld* World = Companion->GetWorld();
    if (!World) return Observation;

    Observation.bWorldAvailable = true;
    Observation.bPaused = World->IsPaused();
    Observation.CapturedAtWorldSeconds = World->GetTimeSeconds();
    Observation.ExecutionPhase = UEnum::GetValueAsString(Companion->GetPhase());
    Observation.ExecutionAction = Companion->GetExecutionAction();
    Observation.bAtCamp = Companion->IsAtCamp();

    Observation.bCampAvailable = AvailableActor(Companion->Camp, World);
    if (Observation.bCampAvailable)
        Observation.CampDistanceCm = FVector::Dist(Companion->GetActorLocation(), Companion->Camp->GetActorLocation());

    const AActor* SourceActor = IsValid(Companion->Source) ? Companion->Source->GetOwner() : nullptr;
    Observation.bCollectionSourceAvailable = AvailableActor(SourceActor, World);
    Observation.bCollectionSourceTrustedSafe = Observation.bCollectionSourceAvailable && Companion->bSourceSafe;
    if (Observation.bCollectionSourceAvailable)
        Observation.CollectionSourceDistanceCm = FVector::Dist(Companion->GetActorLocation(), SourceActor->GetActorLocation());

    if (APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0))
        if (const auto* Gameplay = Player->FindComponentByClass<UHearthwardGameplayComponent>())
        {
            Observation.bCombatStateAvailable = true;
            Observation.bCombatActive = Gameplay->InCombat();
        }

    if (auto* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
        Observation.bNavigationRebuilding = Navigation->IsNavigationBuildInProgress();

    return Observation;
}

FHearthwardNPCSafetyDecision HearthwardPerception::Evaluate(
    const FHearthwardNPCObservation& Observation, const FHearthwardAgentGoal& Goal)
{
    if (!Observation.bWorldAvailable)
        return Decision(EHearthwardNPCSafetyVerdict::Unavailable, TEXT("WORLD_UNAVAILABLE"));

    if (!Goal.WritesWorld())
        return Decision(EHearthwardNPCSafetyVerdict::Allowed, TEXT("READ_ONLY"));

    if (Observation.bPaused)
        return Decision(EHearthwardNPCSafetyVerdict::Unavailable, TEXT("WORLD_PAUSED"));

    if (!Observation.bCombatStateAvailable)
        return Decision(EHearthwardNPCSafetyVerdict::Unavailable, TEXT("SAFETY_STATE_UNAVAILABLE"));

    if (Observation.bCombatActive)
        return Decision(EHearthwardNPCSafetyVerdict::Unsafe, TEXT("ACTIVE_COMBAT"));

    if (Goal.Intent == TEXT("collect"))
    {
        if (!Observation.bCollectionSourceAvailable)
            return Decision(EHearthwardNPCSafetyVerdict::Unavailable, TEXT("SOURCE_UNAVAILABLE"));
        if (!Observation.bCollectionSourceTrustedSafe)
            return Decision(EHearthwardNPCSafetyVerdict::Unsafe, TEXT("SOURCE_NOT_TRUSTED_SAFE"));
        if (!Observation.bCampAvailable)
            return Decision(EHearthwardNPCSafetyVerdict::Unavailable, TEXT("CAMP_UNAVAILABLE"));
    }
    else if (Goal.Intent == TEXT("craft"))
    {
        // Current craft execution always returns produced items to camp, even when materials came from the NPC bag.
        if (!Observation.bCampAvailable)
            return Decision(EHearthwardNPCSafetyVerdict::Unavailable, TEXT("CAMP_UNAVAILABLE"));
    }
    else if (Goal.Intent == TEXT("repair") && Goal.SourceRef == TEXT("camp"))
    {
        if (!Observation.bCampAvailable)
            return Decision(EHearthwardNPCSafetyVerdict::Unavailable, TEXT("CAMP_UNAVAILABLE"));
    }

    return Decision(EHearthwardNPCSafetyVerdict::Allowed, TEXT("SAFE"));
}

FString HearthwardPerception::VerdictName(EHearthwardNPCSafetyVerdict Verdict)
{
    switch (Verdict)
    {
    case EHearthwardNPCSafetyVerdict::Allowed: return TEXT("allowed");
    case EHearthwardNPCSafetyVerdict::Unsafe: return TEXT("unsafe");
    case EHearthwardNPCSafetyVerdict::Unavailable: return TEXT("unavailable");
    default: return TEXT("unavailable");
    }
}
