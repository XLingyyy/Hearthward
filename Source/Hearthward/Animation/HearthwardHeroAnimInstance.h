#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "../Combat/HearthwardCombatRules.h"
#include "HearthwardHeroAnimInstance.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardHeroAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UHearthwardHeroAnimInstance();
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
    void PlayAttack();
    void PlayCombat(float Duration,bool Execution,const HearthwardCombat::FMove* InStoneAxeMove=nullptr);
    void StopCombat();
    float CombatRate=1;
    bool IsExecution=false;
    bool IsStoneAxe=false;
    bool IsSpear=false;
    HearthwardCombat::FMove StoneAxeMove;
    HearthwardCombat::FMove SpearMove;

    UPROPERTY(BlueprintReadOnly, Transient, Category="Animation") float GroundSpeed = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Animation") FName MotionState = TEXT("Idle");
    UPROPERTY(VisibleDefaultsOnly, Category="Animation") TArray<TObjectPtr<class UAnimSequence>> Clips;
    int32 ActionState = 0;
    float LifePoseTime = 0;
    float RangedWeight = 0;
    float RangedPoseTime = 0;
    float BowDrawTime = 0;
    int32 RangedClip = 12;
    FRotator RangedAimRotation;
    uint32 AttackRevision = 0;
private:
    float AttackRemaining = 0;
    float LandRemaining = 0;
    bool bWasFalling = false;
    float DownElapsed = 0;
    float ReleasedDrawTime = 0;
};
