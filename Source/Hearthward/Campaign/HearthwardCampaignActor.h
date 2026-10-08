#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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
    UPROPERTY() TObjectPtr<class UHearthwardCombatTargetComponent> Target;
    void Initialize(FName Id,bool Hostile);
    void PresentLabor(FName Region,const FString& Status,bool Working);
    virtual void Tick(float Delta) override;
    bool WalkTo(FVector Goal,float Acceptance=100);
private:
    void CancelBowShot();
    void UpdateBowShot(float Delta);
    TWeakObjectPtr<AActor> BowTarget;
    FGuid BowEpoch;
    float BowRemaining=0,BowPower=0;
    friend struct FArcherShotTestAccess;
    float DecisionIn=0,AttackIn=0,Pause=0,OffNavigation=0;
    bool Enemy=false;
    bool LaborPresented=false,LaborWorking=false;
    UPROPERTY() TObjectPtr<class UWidgetComponent> LaborLabel;
    TSharedPtr<class STextBlock> LaborText;
    TSharedPtr<struct FCompositeFont> LaborTypeface;
};
