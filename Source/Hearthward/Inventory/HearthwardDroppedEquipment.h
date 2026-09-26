#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HearthwardInventoryState.h"
#include "../Interaction/HearthwardInteractionTargetComponent.h"
#include "HearthwardDroppedEquipment.generated.h"
USTRUCT()
struct FHearthwardGroundEquipment
{
    GENERATED_BODY()
    UPROPERTY() FHearthwardItemInstance Item;
    UPROPERTY() FTransform Transform;
};
UCLASS()
class HEARTHWARD_API UHearthwardEquipmentPickup : public UHearthwardInteractionTargetComponent
{
    GENERATED_BODY()
public:
    virtual FString GetInteractionPrompt(AActor* Player) const override;
    virtual FString CompleteInteraction(AActor* Player) override;
};
UCLASS()
class HEARTHWARD_API AHearthwardDroppedEquipment : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardDroppedEquipment();
    UPROPERTY() FHearthwardItemInstance Item;
    bool PickUp(AActor* Player);
};
