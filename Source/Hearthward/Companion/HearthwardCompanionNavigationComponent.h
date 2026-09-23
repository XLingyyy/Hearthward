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

    bool IsAt(const AActor* Target,float AcceptanceRadius=50.0f) const;
    bool MoveToActor(AActor* Target,float Speed,float AcceptanceRadius);
    bool MoveToLocation(const FVector& Location,float Speed,float AcceptanceRadius);
    void Stop();
    bool IsRetryReady() const;

private:
    class ACharacter* CharacterOwner() const;

    TWeakObjectPtr<AActor> Target;
    bool bToLocation=false;
    FVector Location=FVector::ZeroVector;
    float Acceptance=0;
    double RetryAt=0;
};
