#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HearthwardCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UHearthwardTimedActionComponent;
class UHearthwardInventoryComponent;
class UHearthwardInteractionComponent;
class UStaticMeshComponent;
struct FInputActionValue;

UCLASS()
class HEARTHWARD_API AHearthwardCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    virtual void FellOutOfWorld(const class UDamageType& DamageType) override;
    virtual void Landed(const FHitResult& Hit) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UHearthwardGameplayComponent> Gameplay;
    AHearthwardCharacter();
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void UpdateCarrySpeed();
    UFUNCTION()
    void RefreshHeldTool();

    UPROPERTY(VisibleAnywhere, Category="Hearthward|Equipment")
    TObjectPtr<UStaticMeshComponent> HeldAxe;

    UPROPERTY(VisibleAnywhere, Category="Hearthward|Inventory")
    TObjectPtr<UHearthwardInventoryComponent> Inventory;

    UPROPERTY(VisibleAnywhere, Category="Hearthward|Actions")
    TObjectPtr<UHearthwardTimedActionComponent> TimedAction;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void ToggleInventory();
    void Interact();
    void StartJump();
    void StartSprint();
    void StopSprint();
    void PreviewAttack();

    UPROPERTY() TObjectPtr<UInputAction> SprintAction;
    UPROPERTY() TObjectPtr<UInputAction> AttackPreviewAction;

    UPROPERTY(VisibleAnywhere, Category="Hearthward|Interaction")
    TObjectPtr<UHearthwardInteractionComponent> Interaction;

    UPROPERTY()
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY()
    TObjectPtr<UInputAction> InventoryAction;

    UPROPERTY()
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY()
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY()
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY()
    TObjectPtr<UInputMappingContext> InputMapping;
};
