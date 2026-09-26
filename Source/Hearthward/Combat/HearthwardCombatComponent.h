#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardCombatRules.h"
#include "HearthwardCombatTargetComponent.h"
#include "HearthwardProjectile.h"
#include "HearthwardCombatComponent.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardCombatSave
{
    GENERATED_BODY()
    UPROPERTY() int32 Version=1;
    UPROPERTY() bool CrossbowLoaded=false;
    UPROPERTY() bool RangedSelected=false;
    UPROPERTY(BlueprintReadOnly) double SenseRemaining=0;
    UPROPERTY(BlueprintReadOnly) double SenseCooldown=0;
    UPROPERTY() double SenseRadius=1500;
    UPROPERTY() double OutsideSeconds=0;
    UPROPERTY() int32 Infiltration=0;
    UPROPERTY() TMap<FName,double> Alarms;
    UPROPERTY() TArray<FHearthwardCombatTargetSave> Targets;
    UPROPERTY() TArray<FHearthwardArrowSave> Arrows;
};
UCLASS(ClassGroup=(Hearthward),meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardCombatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardCombatComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* Function) override;
    UFUNCTION(BlueprintCallable) bool Attack(bool Heavy=false);
    UFUNCTION(BlueprintCallable) bool Execute(AActor* Target=nullptr);
    UFUNCTION(BlueprintCallable) void Cancel();
    UFUNCTION(BlueprintCallable) bool Dodge(FVector Direction);
    UFUNCTION(BlueprintCallable) bool SetGuard(bool Value);
    UFUNCTION(BlueprintCallable) bool Sense();
    UFUNCTION(BlueprintCallable) bool ToggleLock();
    UFUNCTION(BlueprintCallable) bool Carry(bool Back=false);
    UFUNCTION(BlueprintCallable) bool Reload();
    UFUNCTION(BlueprintCallable) void Aim(bool Value);
    UFUNCTION(BlueprintCallable) bool Shoot(bool Release=false);
    UFUNCTION(BlueprintCallable) bool Throw(FName Item);
    UFUNCTION(BlueprintCallable) bool SwitchEquipment(FName Item);
    bool SwitchEquipmentInstance(FGuid Instance);
    UFUNCTION(BlueprintPure) bool Busy() const { return Action!=NAME_None; }
    UFUNCTION(BlueprintPure) bool Executing() const { return Action==TEXT("execution"); }
    UFUNCTION(BlueprintPure) bool MovementLocked() const;
    UFUNCTION(BlueprintPure) FString Describe() const;
    UFUNCTION(BlueprintPure) TArray<AActor*> SensedTargets() const;
    UPROPERTY(BlueprintReadOnly) FName Action;
    UPROPERTY(BlueprintReadOnly) double Elapsed=0;
    UPROPERTY(BlueprintReadOnly) double Duration=0;
    UPROPERTY(BlueprintReadOnly) float Discovery=0;
    UPROPERTY(BlueprintReadOnly) FString Feedback;
    UPROPERTY(BlueprintReadOnly) bool Aiming=false;
    UPROPERTY(BlueprintReadOnly) bool CrossbowLoaded=false;
    UPROPERTY(BlueprintReadOnly) FHearthwardCombatSave State;
    bool RangedSelected() const { return State.RangedSelected; }
    void SelectRanged(bool Value) { State.RangedSelected=Value; }
    bool Guarding() const { return Guard.Held; }
    float MovementMultiplier() const;
    bool CanSave() const;
    void InterruptTravel() { Cancel(); DropBody(); Guard.Release(); Aiming=false; }
    bool Damage(float Raw,FName Part,FVector Source,bool Heavy=false,bool Projectile=false,FGuid Event=FGuid());
    void HitTarget(UHearthwardCombatTargetComponent* Target,float Raw,FName Part,bool Projectile,FGuid Event,AActor* Source=nullptr);
    void ObserveDamage(UHearthwardCombatTargetComponent* Target,AActor* Source=nullptr);
    FString Snapshot() const;
    static bool ValidateSnapshot(const FString& Json);
    void Restore(const FString& Json);
    TArray<UHearthwardCombatTargetComponent*> Targets() const;
    UHearthwardCombatTargetComponent* ExecutionTarget() const { return Captive.Get(); }
private:
    class UHearthwardGameplayComponent* G() const;
    class UHearthwardInventoryComponent* Bag() const;
    double Now() const;
    FGuid Epoch() const;
    bool Available() const;
    bool Start(FName Name,double Seconds);
    bool Visible(const AActor* From,const AActor* To) const;
    bool Eligible(UHearthwardCombatTargetComponent* T) const;
    FName WeaponKind() const;
    void Finish();
    void LaunchProjectile(FName Item,float Scale,bool Thrown);
    void Sweep(double From,double To);
    void Perception(double Delta);
    void UpdateCarry(double Delta);
    void DropBody();
    HearthwardCombat::FGuard Guard;
    HearthwardCombat::FMove Move;
    TWeakObjectPtr<UHearthwardCombatTargetComponent> Captive,Body;
    TWeakObjectPtr<AActor> Locked;
    FVector StartPosition,DodgeDirection;
    TSet<FName> HitIds;
    FGuid ActionEpoch,ActionId,ActionInstance,PendingInstance;
    float ActionPower=0;
    bool ChargedWear=false;
    TSet<FGuid> DamageIds;
    FName ActionWeapon,PendingItem,BufferedAttack;
    double BufferedUntil=0,LostLock=0,StartedAt=0;
    FVector CaptivePosition; FRotator CaptiveRotation;
    uint8 CaptiveMovementMode=0;
    bool HeavyAttack=false,CarryingOnBack=false;
    bool Committing=false;
};
