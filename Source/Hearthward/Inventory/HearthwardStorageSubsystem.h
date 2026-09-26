#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HearthwardStorageState.h"
#include "HearthwardStorageSubsystem.generated.h"

class UHearthwardInventoryComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FHearthwardStorageTransferred, FGuid, OperationId, bool, ToCamp, FName, ItemId, int32, Count);

UCLASS()
class HEARTHWARD_API UHearthwardStorageSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    int32 GetItemCount(FName ItemId) const { return State.GetCount(ItemId); }
    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    double GetWeight() const { return State.GetWeightHundredths() / 100.0; }
    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    FGuid GetTimelineEpoch() const { return State.GetEpoch(); }
    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    FGuid GetContainerId() const { return State.GetContainerId(); }

    // Invalidates outstanding operations; save coordination supplies restored container contents separately.
    UFUNCTION(BlueprintCallable, Category="Hearthward|Inventory")
    void AdvanceTimeline() { State.AdvanceTimeline(); Reservations.Reset(); }

    int32 Available(FName Item) const;
    bool Reserve(FGuid Operation,const TMap<FName,int32>& Materials);
    void Release(FGuid Operation) { Reservations.Remove(Operation); }
    bool Adjust(const TMap<FName,int32>& Consumed,const TMap<FName,int32>& Produced,FGuid Reservation=FGuid());
    bool CanAdjust(const TMap<FName,int32>& Consumed,const TMap<FName,int32>& Produced,FGuid Reservation=FGuid()) const;

    FHearthwardTransferResult Transfer(UHearthwardInventoryComponent* Personal, bool ToCamp,
        FName ItemId, int32 Count, FGuid OperationId, FGuid TimelineEpoch);

    const FHearthwardInventorySnapshot& InventorySnapshot() const { return State.Shared.Snapshot(); }
    FHearthwardTransferResult TransferInstance(UHearthwardInventoryComponent* Personal,bool ToCamp,FGuid Instance,FGuid Operation,FGuid Epoch);
    bool CanWorkshop(UHearthwardInventoryComponent* Personal,const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,bool UseStorage,FGuid Repair=FGuid(),double Restore=0,bool Upgrade=false) const;
    bool Workshop(UHearthwardInventoryComponent* Personal,const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,bool UseStorage,FGuid Repair=FGuid(),double Restore=0,bool Upgrade=false);

    UPROPERTY(BlueprintAssignable, Category="Hearthward|Inventory")
    FHearthwardStorageTransferred OnTransferred;
protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
private:
    bool PrepareWorkshop(UHearthwardInventoryComponent* Personal,const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,bool UseStorage,FGuid Repair,double Restore,bool Upgrade,FHearthwardInventoryState& BagAfter,FHearthwardInventoryState& CampAfter) const;
    friend class UHearthwardSaveSubsystem;
    FHearthwardStorageState State;
    TMap<FGuid,TMap<FName,int32>> Reservations;
};
