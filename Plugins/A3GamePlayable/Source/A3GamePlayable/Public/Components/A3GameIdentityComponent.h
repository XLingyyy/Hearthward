#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "A3GameIdentityComponent.generated.h"

UCLASS(
    BlueprintType,
    Blueprintable,
    ClassGroup = (A3Game),
    meta = (BlueprintSpawnableComponent))
class A3GAMEPLAYABLE_API UA3GameIdentityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UA3GameIdentityComponent();

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "A3Game|Identity")
    void SetRuntimeIdentity(
        const FString& InParticipantId,
        const FString& InEntityId);

    UPROPERTY(
        Replicated,
        EditAnywhere,
        BlueprintReadOnly,
        Category = "A3Game|Identity")
    FString ParticipantId;

    UPROPERTY(
        Replicated,
        EditAnywhere,
        BlueprintReadOnly,
        Category = "A3Game|Identity")
    FString EntityId;
};
