#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HearthwardCampaignState.h"
#include "HearthwardCampaignSubsystem.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardCampaignSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    FHearthwardCampaignState State;
    UPROPERTY(BlueprintReadOnly) FString Feedback;
    bool Active() const { return !State.Phase.IsNone(); }
    UFUNCTION(BlueprintPure) bool Busy() const { return IntroRemaining>0 || !PendingFlag.IsNone() || !TravelDestination.IsNone(); }
    bool IsPreparingWorld() const { return IntroRemaining>0 || !TravelDestination.IsNone(); }
    bool IsTraveling() const { return !TravelDestination.IsNone(); }
    double NextBoundary(double Calendar) const;
    void AdvanceBoundary(double Calendar);
    UFUNCTION(BlueprintPure) FString Describe() const { return State.Snapshot(); }
    UFUNCTION(BlueprintPure) FString Prompt() const;
    UFUNCTION(BlueprintCallable) bool Interact();
    bool AssignEscort(FName Person,class AHearthwardCompanionFixture* Brother,FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool Claim(FName Quest,FGuid Epoch);
    UFUNCTION(BlueprintCallable) bool Travel(FName Destination);
    UFUNCTION(BlueprintCallable) void Start();
    bool Restore(const FString& Json,bool Continued=false);
    FString Snapshot();
    bool Available(FName Quest) const;
    int32 Progress(FName Quest) const;
    FName QuestLocation(FName Quest) const;
    FVector Position(FName Id) const;
    bool HasLocation(FName Id) const;
    void Record(FName Fact);
    void Damage(FName Id,float Health);
    void Cancel();
    bool Ground(FVector Desired,FVector& Result) const;
    class AHearthwardCampaignActor* Actor(FName Id) const;
    FHearthwardCampaignEnemy* Enemy(FName Id);
    virtual void Tick(float Delta) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;
protected:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
private:
    class APawn* Player() const;
    class UHearthwardGameplayComponent* Gameplay() const;
    bool Safe() const;
    bool BeginTravel(FName Destination);
    void FinishTravel();
    void Sync();
    void RefreshActors();
    void ResetActors();
    bool ZoneOccupied(FName Zone) const;
    FName Nearest() const;
    bool Use(FName Id);
    TArray<bool> Conditions(FName Quest) const;
    TMap<FName,TWeakObjectPtr<AHearthwardCampaignActor>> Actors;
    TArray<TWeakObjectPtr<AActor>> Scenery;
    TWeakObjectPtr<AActor> StreamSource;
    FName PendingFlag,TravelDestination;
    FVector ActionPosition=FVector::ZeroVector;
    float ActionHealth=0,FlagRemaining=0,RefreshIn=0,IntroRemaining=0;
    FGuid ActionEpoch;
    TArray<TWeakObjectPtr<AActor>> TravelParticipants;
    TMap<FName,int32> TravelEnemies;
    FGuid TravelEpoch;
    bool ScriptedTravel=false;
};
