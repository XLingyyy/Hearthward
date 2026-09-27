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
    UPROPERTY() TObjectPtr<class UHearthwardCombatTargetComponent> Target;
    void Initialize(FName Id,bool Hostile);
    virtual void Tick(float Delta) override;
    bool WalkTo(FVector Goal,float Acceptance=100);
private:
    float DecisionIn=0,AttackIn=0,Pause=0,OffNavigation=0;
    bool Enemy=false;
};
