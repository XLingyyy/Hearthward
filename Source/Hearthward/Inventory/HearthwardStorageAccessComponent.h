#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardStorageState.h"
#include "HearthwardStorageAccessComponent.generated.h"

class UHearthwardInventoryComponent;

// Attach to a camp storage access actor. Gameplay interaction/unlock checks belong to its caller.
UCLASS(ClassGroup=(Hearthward), meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardStorageAccessComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    int32 GetItemCount(FName ItemId) const;
    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    FGuid GetContainerId() const;
    UFUNCTION(BlueprintCallable, Category="Hearthward|Inventory")
    FHearthwardTransferResult Transfer(UHearthwardInventoryComponent* Personal, bool ToCamp,
        FName ItemId, int32 Count, FGuid OperationId, FGuid TimelineEpoch);
};
