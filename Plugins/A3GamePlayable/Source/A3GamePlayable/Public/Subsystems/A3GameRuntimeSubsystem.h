#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "A3GameRuntimeSubsystem.generated.h"

class AA3GameRuntimeInputReceiver;
class UA3GameWorldSessionSubsystem;

UCLASS()
class A3GAMEPLAYABLE_API UA3GameRuntimeSubsystem
    : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool DoesSupportWorldType(
        const EWorldType::Type WorldType) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    void SetEntityFactory(UObject* FactoryObject);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    void RegisterMessageHandler(UObject* HandlerObject);

    UFUNCTION(BlueprintCallable, Category = "A3Game|Runtime")
    void UnregisterMessageHandler(UObject* HandlerObject);

    UFUNCTION(BlueprintPure, Category = "A3Game|Runtime")
    UA3GameWorldSessionSubsystem* GetSessionSubsystem() const;

    bool DispatchExtensionMessage(
        const FString& MessageType,
        const FString& JsonPayload) const;

private:
    UPROPERTY()
    TObjectPtr<AA3GameRuntimeInputReceiver> RuntimeInputReceiver;

    UPROPERTY()
    TArray<TObjectPtr<UObject>> MessageHandlers;
};
