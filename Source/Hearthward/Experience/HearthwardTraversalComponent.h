#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HearthwardTraversalComponent.generated.h"
UCLASS()
class HEARTHWARD_API UHearthwardMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()
public:
    virtual void PhysSwimming(float DeltaTime,int32 Iterations) override;
    virtual void PhysicsVolumeChanged(class APhysicsVolume* NewVolume) override;
};
UCLASS(ClassGroup=(Hearthward),meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardTraversalComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardTraversalComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick) override;
    UFUNCTION(BlueprintPure) bool IsVaulting() const { return Vaulting; }
    UFUNCTION(BlueprintPure) bool IsInWater() const { return InWater; }
    UFUNCTION(BlueprintPure) FString GetStatus() const { return Status; }
    UFUNCTION(BlueprintCallable) bool BeginVault();
    UFUNCTION(BlueprintCallable) void CancelVault();
    bool FindVault(FVector& Landing) const;
    bool WaterSurface(FVector Position,float& Height) const;
    bool BreathingOnGround() const;
    static float FallDamage(float DownSpeed,float Gravity,float MaximumHealth);
private:
    UFUNCTION() void Restored();
    bool Vaulting=false,InWater=false;
    float Elapsed=0,StartingHealth=0;
    FVector Start,HighStart,HighEnd,End;
    FGuid VaultEpoch;
    FString Status;
    struct FWaterArea {FVector2D Center,Radius;float Height;};
    TArray<FWaterArea> WaterAreas;
};
