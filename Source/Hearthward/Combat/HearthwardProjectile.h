#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HearthwardProjectile.generated.h"
USTRUCT()
struct FHearthwardArrowSave
{
    GENERATED_BODY()
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() FRotator Rotation=FRotator::ZeroRotator;
};
UCLASS()
class HEARTHWARD_API AHearthwardProjectile : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardProjectile();
    virtual void Tick(float Delta) override;
    TWeakObjectPtr<class UHearthwardCombatComponent> Shooter;
    FVector Velocity;
    double Gravity=0,RemainingRange=0,Lifetime=3;
    float Power=0;
    FName Item;
    FGuid Epoch,Event;
    bool Bait=false,Landed=false,HitTarget=false;
    UFUNCTION(BlueprintCallable) bool Recover(AActor* Player);
};
