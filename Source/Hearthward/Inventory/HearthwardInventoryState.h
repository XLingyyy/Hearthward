#pragma once

#include "CoreMinimal.h"
#include "HearthwardInventoryState.generated.h"

UENUM(BlueprintType)
enum class EHearthwardInventoryResult : uint8
{
    Success,
    InvalidCount,
    UnknownItem,
    CapacityExceeded,
    InsufficientItems,
    InvalidArgument,
    StaleTimeline,
    OperationConflict,
    QuantityOverflow
};

struct FHearthwardItemDefinition
{
    FName Id;
    int32 WeightHundredths;
    FText DisplayName;
};

HEARTHWARD_API const TArray<FHearthwardItemDefinition>& HearthwardBasicItems();

class FHearthwardInventoryState
{
public:
    static constexpr int32 CapacityHundredths = 10000;
    explicit FHearthwardInventoryState(bool bInUnlimited = false) : bUnlimited(bInUnlimited) {}

    int32 GetCount(FName ItemId) const { return Counts.FindRef(ItemId); }
    int64 GetWeightHundredths() const
    {
        int64 Weight = 0;
        for (const auto& Item : HearthwardBasicItems()) Weight += int64(GetCount(Item.Id)) * Item.WeightHundredths;
        return Weight;
    }

    EHearthwardInventoryResult Add(FName ItemId, int32 Count)
    {
        if (Count <= 0) return EHearthwardInventoryResult::InvalidCount;
        const auto* Item = FindItem(ItemId);
        if (!Item) return EHearthwardInventoryResult::UnknownItem;
        const int64 Free = CapacityHundredths - GetWeightHundredths();
        // Divide before multiplying so even an INT_MAX request cannot overflow.
        if (!bUnlimited && Item->WeightHundredths > 0 && Count > Free / Item->WeightHundredths) return EHearthwardInventoryResult::CapacityExceeded;
        if (Count > MAX_int32 - GetCount(ItemId)) return EHearthwardInventoryResult::QuantityOverflow;
        Counts.FindOrAdd(ItemId) += Count;
        return EHearthwardInventoryResult::Success;
    }

    EHearthwardInventoryResult Remove(FName ItemId, int32 Count)
    {
        if (Count <= 0) return EHearthwardInventoryResult::InvalidCount;
        if (!FindItem(ItemId)) return EHearthwardInventoryResult::UnknownItem;
        const int32 Available = GetCount(ItemId);
        if (Count > Available) return EHearthwardInventoryResult::InsufficientItems;
        if (Count == Available) Counts.Remove(ItemId);
        else Counts[ItemId] -= Count;
        return EHearthwardInventoryResult::Success;
    }

    EHearthwardInventoryResult Exchange(const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,int32 Batches)
    {
        if(Batches<=0) return EHearthwardInventoryResult::InvalidCount;
        if(Materials.IsEmpty() || Outputs.IsEmpty()) return EHearthwardInventoryResult::InvalidArgument;
        auto After=*this;
        for(const auto* Entries:{&Materials,&Outputs})
            for(const auto& Entry:*Entries)
            {
                if(Entry.Value<=0) return EHearthwardInventoryResult::InvalidCount;
                if(int64(Entry.Value)*Batches>MAX_int32) return EHearthwardInventoryResult::QuantityOverflow;
            }
        for(const auto& M:Materials)
        {
            const auto R=After.Remove(M.Key,M.Value*Batches);
            if(R!=EHearthwardInventoryResult::Success) return R;
        }
        // Check capacity after all ingredients are removed; publish no partial result on failure.
        for(const auto& O:Outputs)
        {
            const auto R=After.Add(O.Key,O.Value*Batches);
            if(R!=EHearthwardInventoryResult::Success) return R;
        }
        *this=MoveTemp(After);
        return EHearthwardInventoryResult::Success;
    }

    float GetLoadRatio() const { return static_cast<float>(GetWeightHundredths()) / CapacityHundredths; }
    float GetMoveSpeedMultiplier() const { return 1.0f - 0.1f * GetLoadRatio(); }
    float GetStaminaCostMultiplier() const { return 1.0f + 0.1f * GetLoadRatio(); }

    EHearthwardInventoryResult TransferTo(FHearthwardInventoryState& Target, FName ItemId, int32 Count)
    {
        if (this == &Target) return EHearthwardInventoryResult::InvalidArgument;
        // Validate against copies before committing either end. No event can observe a half-transfer.
        auto SourceAfter = *this;
        auto TargetAfter = Target;
        auto Result = SourceAfter.Remove(ItemId, Count);
        if (Result != EHearthwardInventoryResult::Success) return Result;
        Result = TargetAfter.Add(ItemId, Count);
        if (Result != EHearthwardInventoryResult::Success) return Result;
        *this = MoveTemp(SourceAfter);
        Target = MoveTemp(TargetAfter);
        return Result;
    }

private:
    static const FHearthwardItemDefinition* FindItem(FName ItemId)
    {
        return HearthwardBasicItems().FindByPredicate([ItemId](const auto& Item) { return Item.Id == ItemId; });
    }
    TMap<FName, int32> Counts;
    bool bUnlimited = false;
};
