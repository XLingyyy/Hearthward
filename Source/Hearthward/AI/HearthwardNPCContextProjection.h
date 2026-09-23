#pragma once
#include "CoreMinimal.h"
#include "HearthwardNPCMemory.h"
#include "HearthwardNPCCoordination.h"

enum class EHearthwardNPCContextTier : uint8
{
    Full,
    Compact,
    Minimal
};

struct FHearthwardNPCContextSnapshot
{
    FString Query;
    FString InputSource;

    bool bPaused = false;
    bool bCombatStateAvailable = false;
    bool bCombatActive = false;
    bool bCampAvailable = false;
    bool bCollectionSourceAvailable = false;
    bool bCollectionSourceTrustedSafe = false;
    bool bNavigationRebuilding = false;
    bool bAtCamp = false;
    double CampDistanceCm = 0;
    double CollectionSourceDistanceCm = 0;
    FString ExecutionPhase;
    FString ExecutionAction;
    FString CollectionSafety;
    FString CollectionSafetyReason;

    TMap<FName,int32> OwnBag;
    int32 PreviousGoalQuantity = 0;
    int32 PreviousGoalDelivered = 0;

    bool bCombatViewAvailable = false;
    FName RequestedOrder;
    FName TacticalIntent;
    FName CombatTarget;
    FString CombatReason;
    bool bPlayerInCombat = false;
    bool bRoutineEnabled = false;
    FName RoutineActivity;

    FHearthwardNPCCoordinationProfile Coordination;
    FHearthwardNPCMemory Memory;
    bool bKnownWorkbench = false;
};

struct FHearthwardNPCContextProjectionResult
{
    FString Json;
    FString Tier;
    TArray<FString> DroppedFields;
};

namespace HearthwardContextProjection
{
    FString TierName(EHearthwardNPCContextTier Tier);
    FHearthwardNPCContextProjectionResult Project(const FHearthwardNPCContextSnapshot& Snapshot,EHearthwardNPCContextTier Tier);
}
