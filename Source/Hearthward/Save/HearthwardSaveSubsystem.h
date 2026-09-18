#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HearthwardSaveGame.h"
#include "HearthwardSaveSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHearthwardSnapshotRestored);

UCLASS()
class HEARTHWARD_API UHearthwardSaveSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;
    // Explicit non-Shipping fixture opt-in; no production danger detection is implied.
    UFUNCTION(BlueprintCallable) bool EnablePrototype();
    UFUNCTION(BlueprintCallable) bool StartNewProgress();
    UFUNCTION(BlueprintCallable) bool SavePoint(bool Manual);
    UFUNCTION(BlueprintCallable) bool LoadPoint(FGuid SaveId);
    UFUNCTION(BlueprintCallable) bool DeletePoint(FGuid SaveId);
    UFUNCTION(BlueprintCallable) bool SetPointLocked(FGuid SaveId, bool Locked);
    UFUNCTION(BlueprintCallable) bool SetAutoMinutes(int32 Minutes);
    UFUNCTION(BlueprintCallable) void SetPrototypeSafety(FHearthwardSaveSafety Value) { Safety = Value; }
    UFUNCTION(BlueprintPure) TArray<FHearthwardSavePoint> GetPoints() const { return Pool ? Pool->Points : TArray<FHearthwardSavePoint>(); }
    UFUNCTION(BlueprintPure) FGuid GetCampaignId() const { return CampaignId; }
    UFUNCTION(BlueprintPure) FString GetStatus() const { return Status; }
    UFUNCTION(BlueprintPure) FString GetKnowledge() const { return FString::Join(Knowledge, TEXT("\n")); }
    UFUNCTION(BlueprintPure) int32 GetAutoMinutes() const { return AutoMinutes; }
    UFUNCTION(BlueprintPure) bool IsPrototypeEnabled() const { return bEnabled; }
    UFUNCTION(BlueprintPure) FString GetSafetyDescription() const;
    UPROPERTY(BlueprintAssignable) FHearthwardSnapshotRestored OnSnapshotRestored;
    void RememberExchange(const FString& Speaker, const FString& Text);
    TArray<FString> RecentKnowledge() const;
    bool IsRestoring() const { return bRestoring; }
protected:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
private:
    bool Capture(FHearthwardWorldSave& Out);
    bool Restore(const FHearthwardWorldSave& Snapshot);
    bool ReloadPool();
    bool CommitPool(UHearthwardSaveGame* Candidate);
    bool WritePoint(bool Manual, bool NewCampaign);
    bool Participants(class APawn*& Player, AHearthwardCompanionFixture*& Companion) const;
    FString PoolPath() const;
    UPROPERTY() TObjectPtr<UHearthwardSaveGame> Pool;
    UPROPERTY() FHearthwardWorldSave InitialWorld;
    FGuid CampaignId;
    TArray<FString> Knowledge;
    int64 KnowledgeRevision = 0;
    FHearthwardSaveSafety Safety;
    int32 AutoMinutes = 10;
    double NextAutoSeconds = 0;
    bool bEnabled = false;
    bool bRestoring = false;
    FString Status = TEXT("存档原型未启用");
};
