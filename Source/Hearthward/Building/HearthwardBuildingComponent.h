#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dom/JsonObject.h"
#include "HearthwardBuildingComponent.generated.h"

UCLASS(ClassGroup=(Hearthward), meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardBuildingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardBuildingComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function) override;
    UFUNCTION(BlueprintCallable) bool SelectBuilding(FName Id);
    UFUNCTION(BlueprintCallable) void RotatePreview();
    UFUNCTION(BlueprintCallable) bool ConfirmPlacement();
    UFUNCTION(BlueprintCallable) void CancelPlacement();
    UFUNCTION(BlueprintPure) bool IsPlacing() const { return !Selected.IsNone(); }
    UFUNCTION(BlueprintPure) bool IsBuilding() const { return Pending || Settling; }
    UFUNCTION(BlueprintPure) int32 BuildingCount() const { return Built.Num(); }
    UFUNCTION(BlueprintPure) TArray<AActor*> GetBuildings() const;
    UFUNCTION(BlueprintPure) FGuid NearbyWorkbench() const;
    UFUNCTION(BlueprintPure) bool CanUseWorkbench(FGuid Station) const;
    UFUNCTION(BlueprintPure) FString CraftingStatus(FGuid Station,FName Recipe,int32 Batches,FGuid Epoch) const;
    UFUNCTION(BlueprintCallable) bool Craft(FGuid Station,FName Recipe,int32 Batches,FGuid Epoch);
    UPROPERTY(BlueprintReadOnly) FString Feedback;
    UPROPERTY(BlueprintReadOnly) bool ValidPlacement = false;
    UPROPERTY(BlueprintReadOnly) FVector Placement = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float Yaw = 0;
    UPROPERTY(BlueprintReadOnly) FName Selected;
    TArray<TSharedPtr<FJsonValue>> Snapshot() const;
    static bool Validate(const TArray<TSharedPtr<FJsonValue>>& Rows);
    void Restore(const TArray<TSharedPtr<FJsonValue>>& Rows);
private:
    UFUNCTION() void Complete();
    UFUNCTION() void Interrupted();
    bool CheckPlacement(FString& Reason) const;
    bool HasMaterials() const;
    TMap<FName,int32> Materials() const;
    AActor* SpawnBuilding(FName Id, FVector Position, float Rotation, bool PreviewOnly);
    void ClearPreview();
    struct FBuilt { FGuid Id; FName Recipe; FVector Position; float Rotation; TWeakObjectPtr<AActor> Actor; };
    TArray<FBuilt> Built;
    TWeakObjectPtr<AActor> Preview;
    bool Pending = false, Settling = false;
    FVector StartedAt = FVector::ZeroVector;
};
