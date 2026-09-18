#pragma once

#include "CoreMinimal.h"
#include "HearthwardInteractionTargetComponent.h"
#include "HearthwardResourceInteractionComponent.generated.h"

// Explicit development fixture consumer of the shared interaction/inventory contracts.
UCLASS()
class HEARTHWARD_API UHearthwardResourceInteractionComponent : public UHearthwardInteractionTargetComponent
{
    GENERATED_BODY()
public:
    void InitializePrototype(bool bAtCamp);
    virtual FString GetInteractionPrompt(AActor* Interactor) const override;
    virtual FString CompleteInteraction(AActor* Interactor) override;
private:
    bool bEnabled = false;
    bool bDeposit = false;
};
