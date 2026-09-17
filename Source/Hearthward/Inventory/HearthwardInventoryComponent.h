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
    UFUNCTION(BlueprintCallable, Category="Hearthward|Inventory")
    EHearthwardInventoryResult TryAdd(FName ItemId, int32 Count);

    UFUNCTION(BlueprintCallable, Category="Hearthward|Inventory")
    EHearthwardInventoryResult TryRemove(FName ItemId, int32 Count);

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

private:
    FHearthwardInventoryState State;
};
