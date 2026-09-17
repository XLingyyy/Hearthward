#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "DataTypes/A3GameRuntimeTypes.h"
#include "A3GameRuntimeEntityComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FA3GameRuntimeInputDelegate,
    const FA3GameRuntimeInputState&,
    InputState);

UCLASS(
    BlueprintType,
    Blueprintable,
    ClassGroup = (A3Game),
    meta = (BlueprintSpawnableComponent))
class A3GAMEPLAYABLE_API UA3GameRuntimeEntityComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:
    UA3GameRuntimeEntityComponent();

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    void SetRuntimeEntityId(const FString& InEntityId);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    bool ApplyRuntimeInput(
        const FA3GameRuntimeInputState& InputState);

    UFUNCTION(BlueprintPure, Category = "A3Game|Runtime")
    FA3GameEntitySnapshot GetRuntimeSnapshot() const;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "A3Game|Runtime")
    FString EntityId;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "A3Game|Runtime")
    bool bPersistent = true;

    UPROPERTY(
        BlueprintReadOnly,
        Category = "A3Game|Runtime")
    EA3GameLocomotionState LocomotionState =
        EA3GameLocomotionState::Idle;

    UPROPERTY(
        BlueprintReadOnly,
        Category = "A3Game|Runtime")
    FString MotionState = TEXT("idle");

    UPROPERTY(
        BlueprintAssignable,
        Category = "A3Game|Runtime")
    FA3GameRuntimeInputDelegate OnRuntimeInput;

private:
    double LastInputTimeSeconds = 0.0;
};
