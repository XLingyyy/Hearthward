#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardCompanionNavigationComponent.generated.h"

UCLASS(ClassGroup=(Hearthward))
class HEARTHWARD_API UHearthwardCompanionNavigationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHearthwardCompanionNavigationComponent();

    UFUNCTION(BlueprintPure) bool IsAt(const AActor* Target,float AcceptanceRadius=50.0f) const;
    UFUNCTION(BlueprintCallable) bool MoveToActor(AActor* Target,float Speed,float AcceptanceRadius);
    UFUNCTION(BlueprintCallable) bool MoveToLocation(const FVector& Location,float Speed,float AcceptanceRadius);
    UFUNCTION(BlueprintCallable) void Stop();
    UFUNCTION(BlueprintPure) FString GetStatus() const { return Status; }
    bool IsRetryReady() const;

private:
    class ACharacter* CharacterOwner() const;

    TWeakObjectPtr<AActor> Target;
    bool bToLocation=false;
    FVector Location=FVector::ZeroVector;
    float Acceptance=0;
    double RetryAt=0;
    FString Status;
};
