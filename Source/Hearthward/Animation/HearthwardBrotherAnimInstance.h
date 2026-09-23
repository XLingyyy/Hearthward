#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HearthwardBrotherAnimInstance.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardBrotherAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UHearthwardBrotherAnimInstance();
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
    void PlayAttack();

    UPROPERTY(BlueprintReadOnly, Transient, Category="Animation") float GroundSpeed = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Animation") FName MotionState = TEXT("Idle");
    UPROPERTY(VisibleDefaultsOnly, Category="Animation") TArray<TObjectPtr<class UAnimSequence>> Clips;
    uint32 AttackRevision = 0;
private:
    float AttackRemaining = 0;
};
