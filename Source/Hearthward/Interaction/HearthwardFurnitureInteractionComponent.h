#pragma once
#include "CoreMinimal.h"
#include "HearthwardInteractionTargetComponent.h"
#include "HearthwardFurnitureInteractionComponent.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardFurnitureInteractionComponent : public UHearthwardInteractionTargetComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) FName Kind;
    virtual FString GetInteractionPrompt(AActor* Interactor) const override;
    virtual FString CompleteInteraction(AActor* Interactor) override;
};
