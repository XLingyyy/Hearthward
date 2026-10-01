#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HearthwardAnimalAnimInstance.generated.h"
UCLASS()
class HEARTHWARD_API UHearthwardAnimalAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
