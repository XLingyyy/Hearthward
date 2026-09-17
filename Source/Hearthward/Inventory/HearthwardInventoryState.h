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

inline const TArray<FHearthwardItemDefinition>& HearthwardBasicItems()
{
    // GDD v0.3 section 7.8: accepted initial weights in abstract units.
    static const TArray<FHearthwardItemDefinition> Items = {
        {TEXT("wood"), 100, NSLOCTEXT("Hearthward", "ItemWood", "木材")},
        {TEXT("stone"), 100, NSLOCTEXT("Hearthward", "ItemStone", "石材")},
        {TEXT("ore"), 200, NSLOCTEXT("Hearthward", "ItemOre", "矿石")},
        {TEXT("meat"), 50, NSLOCTEXT("Hearthward", "ItemMeat", "肉")},
        {TEXT("arrow"), 5, NSLOCTEXT("Hearthward", "ItemArrow", "箭")}
    };
    return Items;
}

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
        if (!bUnlimited && Count > Free / Item->WeightHundredths) return EHearthwardInventoryResult::CapacityExceeded;
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
