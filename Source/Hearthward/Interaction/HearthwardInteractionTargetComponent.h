#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "HearthwardInteractionTargetComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHearthwardInteractionReady, AActor*, Interactor);

UCLASS(ClassGroup=(Hearthward), meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardInteractionTargetComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    // No approved universal distance: unconfigured targets cannot be used.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hearthward|Interaction", meta=(ClampMin="0"))
    float MaxDistance = 0.0f;

    // The domain consumer must still validate resources and settle its own outcome.
    UPROPERTY(BlueprintAssignable, Category="Hearthward|Interaction")
    FHearthwardInteractionReady OnInteractionReady;
};
