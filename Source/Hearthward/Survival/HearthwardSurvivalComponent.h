#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardSurvivalState.h"
#include "HearthwardSurvivalComponent.generated.h"

UCLASS(ClassGroup=(Hearthward),meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardSurvivalComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardSurvivalComponent();
    virtual void BeginPlay() override;
    void AdvanceContinuous(double Active,double Calendar,double StartW);
    void CompleteBoundary(double Delta) { FinishActions(Delta); }
    static bool HasFailed(UWorld* World);
    UPROPERTY(BlueprintReadOnly) FHearthwardSurvivalState State;
    UPROPERTY(BlueprintReadOnly) FString Status;
    UPROPERTY(EditAnywhere) bool Resting=false;
    UPROPERTY(EditAnywhere) bool Treatment=false;
    float& Health(); float& Hunger(); float& Stamina();
    float MaxHealth() const; float MaxStamina() const;
    bool Enabled() const;
    bool Alive() const { return State.Life==EHearthwardLife::Alive; }
    bool Busy() const { return !State.Medicine.IsNone() || Rescue.IsValid(); }
    bool SafeToSave() const;
    UFUNCTION(BlueprintCallable) bool BeginMedicine(FName Item,bool Automatic=false);
    bool Eat(FName Item,bool Automatic=false);
    UFUNCTION(BlueprintCallable) bool BeginRescue(UHearthwardSurvivalComponent* Target);
    bool CanRescue(const UHearthwardSurvivalComponent* Target) const;
    void CancelAction(bool Damaged=false);
    UFUNCTION(BlueprintCallable) void CancelCurrentAction() { CancelAction(); }
    bool ReceiveDamage(float Amount,FGuid Event,FGuid Epoch,bool Fatal=false);
    UFUNCTION(BlueprintCallable) void GiveUp();
    void FatalEnvironment();
    void FallImpact(float Speed);
    UPROPERTY(EditAnywhere) TObjectPtr<class UCurveFloat> FallDamageCurve;
    UPROPERTY(EditAnywhere) float SwimmingCostPerSecond=0;
    void ResetTransient();
    bool AutomaticBehavior(float Delta);
    FString Describe() const;
    void SetAutoPermission(FName Item,bool Allowed);
    bool Permitted(FName Item) const;
    double RescueRemaining=0;
    bool Settling=false;
    UPROPERTY() float BrotherHealth=100;
    UPROPERTY() float BrotherHunger=100;
    UPROPERTY() float BrotherStamina=100;
private:
    UFUNCTION() void NativeDamage(AActor* Actor,float Amount,const class UDamageType* Type,class AController* Instigator,AActor* Causer);
    class UHearthwardInventoryComponent* Bag() const;
    class UHearthwardGameplayComponent* Gameplay() const;
    FGuid Epoch() const;
    bool InCombat() const;
    void FinishActions(double Delta);
    TWeakObjectPtr<UHearthwardSurvivalComponent> Rescue;
    FVector ActionOrigin=FVector::ZeroVector;
    FGuid ActionEpoch;
    TSet<FGuid> DamageEvents;
    uint64 CancelFrame=MAX_uint64;
    FName CancelledMedicine;
    bool PreviousSwimming=false;
    bool InventoryNotificationPending=false;
};
