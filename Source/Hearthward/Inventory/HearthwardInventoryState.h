#pragma once
#include "CoreMinimal.h"
#include "HearthwardInventoryState.generated.h"

UENUM(BlueprintType)
enum class EHearthwardInventoryResult : uint8
{
    Success, InvalidCount, UnknownItem, CapacityExceeded, InsufficientItems,
    InvalidArgument, StaleTimeline, OperationConflict, QuantityOverflow
};

struct FHearthwardItemDefinition
{
    FName Id;
    int32 WeightHundredths;
    FText DisplayName;
    FName Slot;
    double MaximumDurability = 0;
    FName UniqueClaim;
    bool IsInstance() const { return !Slot.IsNone() || MaximumDurability>0; }
};
HEARTHWARD_API const TArray<FHearthwardItemDefinition>& HearthwardBasicItems();

USTRUCT(BlueprintType)
struct FHearthwardItemInstance
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid Id;
    UPROPERTY(BlueprintReadOnly) FName Definition;
    UPROPERTY(BlueprintReadOnly) double Durability = 0;
    UPROPERTY(BlueprintReadOnly) FName UniqueClaim;
};

USTRUCT()
struct FHearthwardInventorySnapshot
{
    GENERATED_BODY()
    UPROPERTY() int32 Version = 1;
    UPROPERTY() int32 BackpackRank = 1;
    UPROPERTY() TMap<FName,int32> Stacks;
    UPROPERTY() TArray<FHearthwardItemInstance> Instances;
    UPROPERTY() TMap<FName,FGuid> Equipped;
};

class HEARTHWARD_API FHearthwardInventoryState
{
public:
    explicit FHearthwardInventoryState(bool InUnlimited=false) : Unlimited(InUnlimited) {}
    int32 GetCount(FName Item) const;
    int64 GetWeightHundredths() const;
    int32 GetCapacityHundredths() const { return (100+50*(Data.BackpackRank-1))*100; }
    int32 GetBackpackRank() const { return Data.BackpackRank; }
    bool UpgradeBackpack();
    float GetLoadRatio() const { return float(GetWeightHundredths())/GetCapacityHundredths(); }
    float GetMoveSpeedMultiplier() const { return 1.f-.1f*GetLoadRatio(); }
    float GetStaminaCostMultiplier() const { return 1.f+.1f*GetLoadRatio(); }
    EHearthwardInventoryResult Add(FName Item,int32 Count);
    EHearthwardInventoryResult Remove(FName Item,int32 Count);
    EHearthwardInventoryResult Exchange(const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,int32 Batches);
    EHearthwardInventoryResult TransferTo(FHearthwardInventoryState& Target,FName Item,int32 Count);
    EHearthwardInventoryResult TransferInstanceTo(FHearthwardInventoryState& Target,FGuid Id);
    EHearthwardInventoryResult InsertInstance(const FHearthwardItemInstance& Instance);
    bool RemoveInstance(FGuid Id);
    const FHearthwardItemInstance* FindInstance(FGuid Id) const;
    FGuid FirstInstance(FName Item,bool PreferUnequipped=false) const;
    FGuid EquippedInstance(FName Slot) const { return Data.Equipped.FindRef(Slot); }
    FName EquippedItem(FName Slot) const;
    bool Equip(FGuid Id);
    bool IsEquipped(FGuid Id) const;
    bool Wear(FGuid Id,double Amount);
    bool RestoreDurability(FGuid Id,double Amount);
    const FHearthwardInventorySnapshot& Snapshot() const { return Data; }
    bool Restore(const FHearthwardInventorySnapshot& Snapshot);
    static bool Validate(const FHearthwardInventorySnapshot& Snapshot,bool Unlimited=false);
    static const FHearthwardItemDefinition* FindItem(FName Item);
private:
    FHearthwardInventorySnapshot Data;
    bool Unlimited=false;
};
