#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardInventoryState.h"
#include "HearthwardInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHearthwardInventoryChanged);

UCLASS(ClassGroup=(Hearthward), meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    FGuid GetContainerId();
    // Trusted local executor entry; validates both containers before publishing either event.
    EHearthwardInventoryResult TransferTo(UHearthwardInventoryComponent* Target, FName ItemId, int32 Count);
    UFUNCTION(BlueprintCallable, Category="Hearthward|Inventory")
    EHearthwardInventoryResult TryAdd(FName ItemId, int32 Count);

    UFUNCTION(BlueprintCallable, Category="Hearthward|Inventory")
    EHearthwardInventoryResult TryRemove(FName ItemId, int32 Count, bool Notify=true);

    // Atomically consumes an entire recipe; observers never see partial material removal.
    UFUNCTION(BlueprintCallable, Category="Hearthward|Inventory")
    EHearthwardInventoryResult TryConsume(const TMap<FName,int32>& Materials);

    EHearthwardInventoryResult CheckExchange(const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,int32 Batches) const;
    EHearthwardInventoryResult TryExchange(const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,int32 Batches);

    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    int32 GetItemCount(FName ItemId) const { return State.GetCount(ItemId); }

    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    double GetWeight() const { return State.GetWeightHundredths() / 100.0; }

    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    double GetCapacity() const { return State.GetCapacityHundredths() / 100.0; }

    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    float GetMoveSpeedMultiplier() const { return State.GetMoveSpeedMultiplier(); }

    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    float GetStaminaCostMultiplier() const { return State.GetStaminaCostMultiplier(); }

    UPROPERTY(BlueprintAssignable, Category="Hearthward|Inventory")
    FHearthwardInventoryChanged OnInventoryChanged;

    UFUNCTION(BlueprintPure) FString DescribeInventory() const;
    const FHearthwardInventorySnapshot& Snapshot() const { return State.Snapshot(); }
    bool RestoreInventory(const FHearthwardInventorySnapshot& Data,bool Notify=true);
    const FHearthwardItemInstance* FindInstance(FGuid Id) const { return State.FindInstance(Id); }
    UFUNCTION(BlueprintCallable) FGuid FirstInstance(FName Item,bool PreferUnequipped=false) const { return State.FirstInstance(Item,PreferUnequipped); }
    UFUNCTION(BlueprintCallable) FGuid EquippedInstance(FName Slot) const { return State.EquippedInstance(Slot); }
    FName EquippedItem(FName Slot) const { return State.EquippedItem(Slot); }
    bool IsEquipped(FGuid Id) const { return State.IsEquipped(Id); }
    UFUNCTION(BlueprintCallable) bool EquipInstance(FGuid Id);
    UFUNCTION(BlueprintCallable) bool WearInstance(FGuid Id,double Amount);
    bool RepairInstance(FGuid Id,double Amount,const TMap<FName,int32>& Materials,bool Notify=true);
    EHearthwardInventoryResult GatherFrom(UHearthwardInventoryComponent* Source,FName Item,int32 Count,FGuid Tool,double Wear);
    EHearthwardInventoryResult TransferInstanceTo(UHearthwardInventoryComponent* Target,FGuid Id);
    EHearthwardInventoryResult InsertInstance(const FHearthwardItemInstance& Instance,bool Notify=true);
    bool RemoveInstance(FGuid Id,bool Notify=true);
    UFUNCTION(BlueprintCallable) bool UpgradeBackpack(bool Notify=true);
    int32 BackpackRank() const { return State.GetBackpackRank(); }
    bool Reserve(FName Item);
    void ReleaseReservation() { Reserved=NAME_None; }
    bool CommitReservation(FName Remainder=NAME_None,bool Notify=true);
    int32 Available(FName Item) const { return GetItemCount(Item)-(Reserved==Item?1:0)-ReservedMaterials.FindRef(Item); }
    bool ReserveMaterials(const TMap<FName,int32>& Materials);
    void ReleaseMaterials() { ReservedMaterials.Reset(); }
    bool CommitMaterials(bool Notify=true);
private:
    TMap<FName,int32> ReservedMaterials;
    FName Reserved;
    friend class UHearthwardSaveSubsystem;
    friend class UHearthwardStorageSubsystem;
    FGuid ContainerId;
    FHearthwardInventoryState State;
};
