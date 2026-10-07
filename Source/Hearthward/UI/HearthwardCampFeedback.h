#pragma once
#include "CoreMinimal.h"
#include "../Camp/HearthwardCampState.h"
#include "../AI/HearthwardNPCMemory.h"

class UHearthwardStorageSubsystem;

struct FHearthwardCampUpgradeView
{
    bool ConditionsMet=false,MaterialsMet=false,MaxTier=false;
    FString Conditions,Materials,CurrentUnlocks,NextUnlocks;
};

// Session presentation only. Existing receipts and state remain owned by gameplay.
struct FHearthwardCampFeedback
{
    void Observe(FGuid CurrentEpoch,FGuid CurrentCampaign,const FHearthwardCampState& State,
        const TArray<FHearthwardNPCEvent>& Events);
    FString Summary() const;
    FString Notice;
    uint32 Revision=0;
private:
    FGuid Epoch,Campaign;
    int32 Population=0,LastTier=0;
    TSet<FName> People;
    TSet<int32> Tiers;
    TSet<FGuid> EventIds;
    TArray<FString> Entries;
    void Add(const FString& Text);
};

namespace HearthwardCampFeedback
{
    FString Unlocks(int32 Tier,bool Cumulative=false);
    FHearthwardCampUpgradeView ReadUpgrade(const FHearthwardCampState& State,
        const UHearthwardStorageSubsystem* Storage);
    int32 AssignedWorkers(const FHearthwardCampState& State);
    FString RegionStatus(const FHearthwardCampState& State,const FHearthwardCampRegion& Region);
    FString UpgradeToken(FGuid Epoch,int32 Tier);
}
