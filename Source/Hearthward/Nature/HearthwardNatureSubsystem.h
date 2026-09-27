#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HearthwardNatureState.h"
#include "HearthwardNatureSubsystem.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardNatureSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    FHearthwardNatureState State;
    UPROPERTY(BlueprintReadOnly) FString Feedback;
    UFUNCTION(BlueprintPure) FString Describe() const { return State.Snapshot(); }
    UFUNCTION(BlueprintPure) bool Busy() const { return !PendingAction.IsNone() || FishingId.IsValid() || Settling; }
    UFUNCTION(BlueprintCallable) bool Act(FName Action,FGuid Target,FName Option,FGuid Epoch,int32 Count=1);
    UFUNCTION(BlueprintCallable) void Cancel();
    UFUNCTION(BlueprintCallable) void HoldLine(bool Held) { LineHeld=Held; }
    UFUNCTION(BlueprintPure) FString FishingStatus() const;
    UFUNCTION(BlueprintPure) double FishingTension() const { return Fishing.Tension; }
    UFUNCTION(BlueprintPure) bool IsFishing() const { return FishingId.IsValid(); }
    bool WorkingOn(FGuid Id) const { return !PendingAction.IsNone() && PendingId==Id; }
    UFUNCTION(BlueprintCallable) bool ReadMap(FName Item);
    void EnsureWorld();
    void Advance(double Minutes);
    void Restore(const FString& Json,double Calendar);
    bool DamageAnimal(FName Id,float Health);
    FVector Position(FGuid Id) const;
    class AHearthwardNatureActor* Actor(FGuid Id) const;
    void RebuildActors();
    virtual void Tick(float Delta) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;
protected:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
private:
    friend class AHearthwardNatureActor;
    bool Settling=false,LineHeld=false;
    FGuid PendingId,ActionEpoch,FishingId,FishingRod;
    FName PendingAction,PendingOption,FishingSpecies;
    FVector ActionPosition=FVector::ZeroVector;
    int32 PendingCount=1;
    HearthwardNature::FFishing Fishing;
    double RefreshIn=0;
    TMap<FGuid,TWeakObjectPtr<AHearthwardNatureActor>> Actors;
    class APawn* Player() const;
    class UHearthwardInventoryComponent* Bag() const;
    class UHearthwardGameplayComponent* Gameplay() const;
    bool Safe(FGuid Epoch) const;
    bool Near(FGuid Id,double Distance=300) const;
    bool Ground(FVector Desired,FVector& Result,bool Flat=false) const;
    bool ClearPlot(FVector Point,double Radius,FGuid Ignore=FGuid()) const;
    bool Commit(FName Action,FGuid Target,FName Option,int32 Count,FVector Site);
    bool PrepareBag(const TMap<FName,int32>& Inputs,const TMap<FName,int32>& Outputs,FHearthwardInventoryState& Result) const;
    void PublishBag(const FHearthwardInventoryState& Result);
    bool StartFishing(FGuid Target,FGuid Epoch);
    void TickFishing(double Delta);
    void FinishFishing();
    void SyncAnimals();
    UFUNCTION() void ActionCompleted();
    UFUNCTION() void ActionInterrupted();
};
