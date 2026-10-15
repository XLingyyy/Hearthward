#pragma once
#include "CoreMinimal.h"
#include "../Interaction/HearthwardInteractionTargetComponent.h"
#include "HearthwardNoiseInteraction.generated.h"

UCLASS(ClassGroup=(Hearthward),meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardNoiseInteraction : public UHearthwardInteractionTargetComponent
{
    GENERATED_BODY()
public:
    UHearthwardNoiseInteraction(){MaxDistance=200;}
    virtual FString GetInteractionPrompt(AActor* Interactor) const override;
    virtual FString CompleteInteraction(AActor* Interactor) override;
    float RemainingCooldown() const;
private:
    double AvailableAt=0;
    FGuid Epoch;
};
