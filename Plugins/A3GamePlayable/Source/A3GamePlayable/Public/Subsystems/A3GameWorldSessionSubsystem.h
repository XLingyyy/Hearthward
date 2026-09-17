#pragma once

#include "CoreMinimal.h"
#include "DataTypes/A3GameRuntimeTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "A3GameWorldSessionSubsystem.generated.h"

class AActor;

UCLASS()
class A3GAMEPLAYABLE_API UA3GameWorldSessionSubsystem
    : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    void SetEntityFactory(UObject* FactoryObject);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    FA3GameParticipantInfo RegisterParticipant(
        const FString& ParticipantId,
        const FString& UserId);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    void MarkParticipantOffline(const FString& ParticipantId);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    AActor* SpawnEntity(
        const FA3GameEntitySpawnRequest& Request);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    bool RegisterEntity(
        const FString& EntityId,
        AActor* Actor,
        const FString& ParticipantId);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    bool RemoveEntity(
        const FString& EntityId,
        bool bDestroyActor);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    FA3GameControllerState CreateController(
        const FString& ParticipantId,
        const FString& ControllerId,
        const FString& Kind);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    bool BindControllerToEntity(
        const FString& ControllerId,
        const FString& EntityId,
        EA3GameControlMode Mode,
        int32 Priority);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    bool UnbindController(const FString& ControllerId);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    bool EnqueueInputState(
        const FA3GameRuntimeInputState& InputState);

    UFUNCTION(BlueprintPure, Category = "A3Game|Runtime")
    TArray<FA3GameEntitySnapshot> GetWorldStateSnapshot() const;

    UFUNCTION(BlueprintPure, Category = "A3Game|Runtime")
    AActor* GetActorForEntity(const FString& EntityId) const;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "A3Game|Runtime")
    FString WorldId = TEXT("world_001");

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "A3Game|Runtime",
        meta = (ClampMin = "1.0"))
    float InputConsumeHz = 20.0f;

private:
    void ConsumeLatestInputs();
    FString MakeRuntimeId(const FString& Prefix) const;

    UPROPERTY()
    TObjectPtr<UObject> EntityFactory;

    UPROPERTY()
    TMap<FString, FA3GameParticipantInfo> Participants;

    UPROPERTY()
    TMap<FString, FA3GameControllerState> Controllers;

    UPROPERTY()
    TMap<FString, FA3GameControlBinding> ControlBindings;

    UPROPERTY()
    TMap<FString, TObjectPtr<AActor>> EntityActors;

    UPROPERTY()
    TMap<FString, FA3GameRuntimeInputState> LatestInputsByController;

    float InputConsumeAccumulator = 0.0f;
};
