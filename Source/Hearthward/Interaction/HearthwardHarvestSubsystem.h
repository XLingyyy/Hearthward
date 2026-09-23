#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HearthwardInteractionTargetComponent.h"
#include "HearthwardHarvestSubsystem.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardHarvestTargetComponent : public UHearthwardInteractionTargetComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) FString ResourceKey;
    UPROPERTY(BlueprintReadOnly) FName Item;
    UPROPERTY(BlueprintReadOnly) int32 Capacity = 0;
    int32 Yield = 2;
    virtual FString GetInteractionPrompt(AActor* Interactor) const override;
    virtual FString CompleteInteraction(AActor* Interactor) override;
};

// Instanced scenery keeps its rendering representation. Only nearby resources receive interactions.
UCLASS()
class HEARTHWARD_API UHearthwardHarvestSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    void RefreshNearby(AActor* Player);
    UFUNCTION(BlueprintPure) int32 Remaining(const FString& Key, int32 Capacity) const;
    bool IsSettling() const { return Settling; }
    const TMap<FString,int32>& Snapshot() const { return Used; }
    void Restore(const TMap<FString,int32>& Snapshot);
    FString Harvest(UHearthwardHarvestTargetComponent* Target, AActor* Player);
    static bool Validate(const TMap<FString,int32>& Snapshot);
private:
    TMap<FString,int32> Used;
    TMap<FString,TWeakObjectPtr<UHearthwardHarvestTargetComponent>> Targets;
    double NextRefresh = 0;
    bool Settling = false;
};
