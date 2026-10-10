#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "HearthwardFootContactNotify.generated.h"

class ACharacter;
class UPhysicalMaterial;

struct FHearthwardFootContactReceipt
{
    FGuid SuccessId;
    FGuid Epoch;
    uint64 Frame=0;
    TWeakObjectPtr<ACharacter> Source;
    FName FootBone;
    FVector Position=FVector::ZeroVector;
    TWeakObjectPtr<UPhysicalMaterial> Material;
    FName Surface=NAME_None; // Classified from this exact grounded hit, never the actor location.
};

UCLASS(meta=(DisplayName="Hearthward Foot Contact"))
class HEARTHWARD_API UHearthwardFootContactNotify : public UAnimNotify
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Foot Contact") FName FootBone=TEXT("foot_l");
    virtual void Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference) override;
private:
    friend struct FFootContactNotifyAccess;
    bool ReadGroundContact(USkeletalMeshComponent* MeshComp,FHearthwardFootContactReceipt& Receipt) const;
};
