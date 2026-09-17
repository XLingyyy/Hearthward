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

private:
    TWeakObjectPtr<APawn> ObservedPawn;
    EHearthwardTimedActionStatus PreviousStatus = EHearthwardTimedActionStatus::Idle;
    double InterruptionVisibleUntil = 0.0;
};
