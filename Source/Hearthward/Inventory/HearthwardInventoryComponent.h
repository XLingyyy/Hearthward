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
    double GetCapacity() const { return State.CapacityHundredths / 100.0; }

    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    float GetMoveSpeedMultiplier() const { return State.GetMoveSpeedMultiplier(); }

    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    float GetStaminaCostMultiplier() const { return State.GetStaminaCostMultiplier(); }

    UPROPERTY(BlueprintAssignable, Category="Hearthward|Inventory")
    FHearthwardInventoryChanged OnInventoryChanged;

    bool Reserve(FName Item);
    void ReleaseReservation() { Reserved=NAME_None; }
    bool CommitReservation(FName Remainder=NAME_None,bool Notify=true);
    int32 Available(FName Item) const { return GetItemCount(Item)-(Reserved==Item?1:0); }
private:
    FName Reserved;
    friend class UHearthwardSaveSubsystem;
    friend class UHearthwardStorageSubsystem;
    FGuid ContainerId;
    FHearthwardInventoryState State;
};
