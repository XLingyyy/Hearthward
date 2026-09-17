#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardTimedActionState.h"
#include "HearthwardTimedActionComponent.generated.h"

class UHearthwardWorldClockSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHearthwardTimedActionEvent);

UCLASS(ClassGroup=(Hearthward), meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardTimedActionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardTimedActionComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="Hearthward|Actions")
    bool StartAction();

    UFUNCTION(BlueprintCallable, Category="Hearthward|Actions")
    void InterruptAction();

    UFUNCTION(BlueprintPure, Category="Hearthward|Actions")
    EHearthwardTimedActionStatus GetStatus() const { return State.Status; }

    UFUNCTION(BlueprintPure, Category="Hearthward|Actions")
    double GetElapsedSeconds() const { return State.ElapsedSeconds; }

    // Timer completion only; a domain consumer must validate and settle its own result.
    UPROPERTY(BlueprintAssignable, Category="Hearthward|Actions")
    FHearthwardTimedActionEvent OnTimerCompleted;

    UPROPERTY(BlueprintAssignable, Category="Hearthward|Actions")
    FHearthwardTimedActionEvent OnInterrupted;

private:
    UFUNCTION()
    void OnOwnerDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
        AController* InstigatedBy, AActor* DamageCauser);

    UPROPERTY(Transient)
    TObjectPtr<UHearthwardWorldClockSubsystem> Clock;

    FHearthwardTimedActionState State;
};
