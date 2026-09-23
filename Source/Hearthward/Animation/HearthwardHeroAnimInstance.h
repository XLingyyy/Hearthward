#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
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

    UPROPERTY(BlueprintReadOnly, Transient, Category="Animation") float GroundSpeed = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Animation") FName MotionState = TEXT("Idle");
    UPROPERTY(VisibleDefaultsOnly, Category="Animation") TArray<TObjectPtr<class UAnimSequence>> Clips;
    int32 ActionState = 0;
    uint32 AttackRevision = 0;
private:
    float AttackRemaining = 0;
    float LandRemaining = 0;
    bool bWasFalling = false;
};
