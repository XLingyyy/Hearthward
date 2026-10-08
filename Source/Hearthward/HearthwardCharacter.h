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
    AHearthwardCharacter(const FObjectInitializer& Initializer=FObjectInitializer::Get());
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable) void RebuildInputBindings();
    UFUNCTION(BlueprintCallable) void ResetHeldInput();
    UFUNCTION(BlueprintPure) bool SemanticHeld(FName Id) const;
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    void SetLookSettings(int32 Sensitivity,bool InvertY);
    void UpdateRangedVisual(float DrawTime,float PoseWeight);

private:
    UFUNCTION()
    void UpdateCarrySpeed();
    UFUNCTION()
    void RefreshHeldTool();

    UPROPERTY(VisibleAnywhere, Category="Hearthward|Equipment")
    TObjectPtr<UStaticMeshComponent> HeldAxe;

    UPROPERTY(VisibleAnywhere, Category="Hearthward|Equipment")
    TObjectPtr<UStaticMeshComponent> HeldWeapon;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> HeldBow;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> NockedArrow;
    UPROPERTY() TObjectPtr<class UAnimSequence> BowDrawClip;
    bool BowEquipped=false;
    UPROPERTY() TMap<FName,TObjectPtr<class UStaticMesh>> WeaponMeshes;
    FTransform RightWeaponGrip,LeftBowGrip;
    FName DisplayedWeapon;

    UPROPERTY(VisibleAnywhere, Category="Hearthward|Inventory")
    TObjectPtr<UHearthwardInventoryComponent> Inventory;

    UPROPERTY(VisibleAnywhere, Category="Hearthward|Actions")
    TObjectPtr<UHearthwardTimedActionComponent> TimedAction;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    float LookSensitivity=1.f;
    bool bInvertLookY=false;
    TSet<FKey> ActiveKeys;
    bool SprintLatched=false;
    void KeyPressed(FKey Key);
    void KeyReleased(FKey Key);
    void ToggleInventory();
    void Interact(bool AllowVault=false);
    void StartJump();
    void StartSprint();
    void StopSprint();
    void PreviewAttack();
    void Execution(); void ContextR(); void GuardStart(); void GuardEnd(); void ReleaseAttack();
    void CombatDodge(); void CombatLock(); void CombatSense(); void CombatThrow();

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
