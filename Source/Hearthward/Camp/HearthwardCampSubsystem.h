#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HearthwardCampState.h"
#include "HearthwardCampSubsystem.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardCampSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    FHearthwardCampState State;
    bool Settling=false;
    UPROPERTY(BlueprintReadOnly) FString Feedback;
    void EnsureCamp(FVector Position);
    void Advance(double Minutes,bool Sleeping);
    UFUNCTION(BlueprintCallable) bool UpgradeCamp(FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool AssignWorker(FName Region,int32 Person,FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool SetProduction(FName Region,bool Enabled,bool ToRations,FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool SelectProduction(FName Region,FGuid Facility,FName Recipe,FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool Prioritize(FName Region,FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool CancelBatch(FName Region,bool ConfirmLoss,FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool DonateFood(FName Item,int32 Count,FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool EatMeal(bool Brother,FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool Craft(FGuid Facility,FName Recipe,int32 Batches,FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool Sleep(FGuid BedId,FGuid Epoch);
    UFUNCTION(BlueprintPure) FString Describe() const;
    bool CanManage(FGuid Epoch) const;
    UFUNCTION(BlueprintCallable) bool RegisterSource(FString Id,FName Item,int32 Capacity,int32 Remaining,FVector Position,double RefreshMinutes);
    FHearthwardCampSource* Source(const FString& Id);
    bool RegisterFacility(FGuid Id,FName Kind,FVector Position,const TMap<FName,int32>& Paid);
    bool RemoveFacility(FGuid Id,bool ConfirmLoss);
    bool CompleteFacilityUpgrade(FGuid Id,const TMap<FName,int32>& Paid);
    UFUNCTION(BlueprintCallable) bool RecordRescue(FName Person);
    UFUNCTION(BlueprintCallable) bool ReclaimHometown(FName Victory,FVector Position);
    bool Restore(const FString& Json,int32 LegacyTier,FVector Camp,double Calendar);
    bool BrotherWorking() const;
    void RefreshQuartermasters();
protected:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
private:
    void SyncTier();
    double Efficiency(AActor* Actor,const FHearthwardCampRegion& Region) const;
};
