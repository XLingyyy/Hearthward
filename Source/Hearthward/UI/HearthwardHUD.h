#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "../Actions/HearthwardTimedActionState.h"
#include "HearthwardHUD.generated.h"

UCLASS()
class HEARTHWARD_API AHearthwardHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;

    UFUNCTION(BlueprintCallable, Category="Hearthward|Inventory")
    void ToggleInventory();

    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    bool IsInventoryOpen() const { return bInventoryOpen; }

private:
    void DrawInventory(const class UHearthwardInventoryComponent& Inventory);
    bool bInventoryOpen = false;
    bool bPausedByInventory = false;
    TWeakObjectPtr<APawn> ObservedPawn;
    EHearthwardTimedActionStatus PreviousStatus = EHearthwardTimedActionStatus::Idle;
    double InterruptionVisibleUntil = 0.0;
};
