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
    InsufficientItems
};

struct FHearthwardItemDefinition
{
    FName Id;
    int32 WeightHundredths;
};

inline const TArray<FHearthwardItemDefinition>& HearthwardBasicItems()
{
    // GDD v0.3 section 7.8: accepted initial weights in abstract units.
    static const TArray<FHearthwardItemDefinition> Items = {
        {TEXT("wood"), 100}, {TEXT("stone"), 100}, {TEXT("ore"), 200},
        {TEXT("meat"), 50}, {TEXT("arrow"), 5}
    };
    return Items;
}

class FHearthwardInventoryState
{
public:
    static constexpr int32 CapacityHundredths = 10000;

    int32 GetCount(FName ItemId) const { return Counts.FindRef(ItemId); }
    int32 GetWeightHundredths() const
    {
        int32 Weight = 0;
        for (const auto& Item : HearthwardBasicItems()) Weight += GetCount(Item.Id) * Item.WeightHundredths;
        return Weight;
    }

    EHearthwardInventoryResult Add(FName ItemId, int32 Count)
    {
        if (Count <= 0) return EHearthwardInventoryResult::InvalidCount;
        const auto* Item = FindItem(ItemId);
        if (!Item) return EHearthwardInventoryResult::UnknownItem;
        const int32 Free = CapacityHundredths - GetWeightHundredths();
        // Divide before multiplying so even an INT_MAX request cannot overflow.
        if (Count > Free / Item->WeightHundredths) return EHearthwardInventoryResult::CapacityExceeded;
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

    float GetLoadRatio() const { return static_cast<float>(GetWeightHundredths()) / CapacityHundredths; }
    float GetMoveSpeedMultiplier() const { return 1.0f - 0.1f * GetLoadRatio(); }
    float GetStaminaCostMultiplier() const { return 1.0f + 0.1f * GetLoadRatio(); }

private:
    static const FHearthwardItemDefinition* FindItem(FName ItemId)
    {
        return HearthwardBasicItems().FindByPredicate([ItemId](const auto& Item) { return Item.Id == ItemId; });
    }
    TMap<FName, int32> Counts;
};
