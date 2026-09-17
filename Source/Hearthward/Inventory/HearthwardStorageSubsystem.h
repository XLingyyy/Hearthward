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

    // Integration hook only: future save coordination must restore containers before advancing.
    UFUNCTION(BlueprintCallable, Category="Hearthward|Inventory")
    void AdvanceTimeline() { State.AdvanceTimeline(); }

    FHearthwardTransferResult Transfer(UHearthwardInventoryComponent* Personal, bool ToCamp,
        FName ItemId, int32 Count, FGuid OperationId, FGuid TimelineEpoch);

    UPROPERTY(BlueprintAssignable, Category="Hearthward|Inventory")
    FHearthwardStorageTransferred OnTransferred;
protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
private:
    FHearthwardStorageState State;
};
