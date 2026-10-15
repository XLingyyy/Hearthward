#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Combat/HearthwardEnemyRolePolicy.h"
#include "HearthwardCampaignActor.generated.h"

UCLASS()
class HEARTHWARD_API AHearthwardCampaignActor : public ACharacter
{
    GENERATED_BODY()
public:
    AHearthwardCampaignActor();
    UPROPERTY(BlueprintReadOnly) FName Identity;
    UPROPERTY(BlueprintReadOnly) FName LaborRegion;
    UPROPERTY(BlueprintReadOnly) FString LaborStatus;
    UPROPERTY(BlueprintReadOnly) FName CampLifePose;
    UPROPERTY() TObjectPtr<class UHearthwardCombatTargetComponent> Target;
    void Initialize(FName Id,bool Hostile);
    void PresentLabor(FName Region,const FString& Status,bool Working,FName LifePose=NAME_None);
    virtual void Tick(float Delta) override;
    bool WalkTo(FVector Goal,float Acceptance=100);
    bool BeginNoiseInvestigation(FVector Source,FGuid Epoch);
    bool CanInvestigateNoise() const;
private:
    bool UpdateNoiseInvestigation(float Delta);
    FVector NoisePoint=FVector::ZeroVector;
    FGuid NoiseEpoch;
    float NoiseTravelRemaining=0,NoiseInspectRemaining=0;
    void CancelMelee();
    void UpdateMelee(float Delta);
    bool HasAttackSpace(AActor* Threat) const;
    TWeakObjectPtr<AActor> MeleeTarget;
    FGuid MeleeEpoch,MeleeEvent;
    HearthwardEnemyRole::FAttack MeleeTiming;
    FVector MeleeFacing=FVector::ForwardVector;
    float MeleeElapsed=0,MeleePower=0;
    bool MeleeCommitted=false;
    friend struct FRound2EnemyTestAccess;
    void CancelBowShot();
    void UpdateBowShot(float Delta);
    TWeakObjectPtr<AActor> BowTarget;
    FGuid BowEpoch;
    float BowRemaining=0,BowPower=0;
    friend struct FArcherShotTestAccess;
    float DecisionIn=0,AttackIn=0,Pause=0,OffNavigation=0;
    bool Enemy=false;
    bool LaborPresented=false,LaborWorking=false;
    float LifeTransitionRemaining=0;
    UPROPERTY() TObjectPtr<class UAnimSequence> LifeTransitionNext;
    UPROPERTY() TObjectPtr<class UWidgetComponent> LaborLabel;
    TSharedPtr<class STextBlock> LaborText;
    TSharedPtr<struct FCompositeFont> LaborTypeface;
};
