#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "BoneContainer.h"

class USkeletalMesh;
class AActor;

namespace HearthwardAxeHandPose
{
    const TArray<FName>& FingerNames();
    bool Profile(const USkeletalMesh* Mesh, FVector& PalmInHand, TArray<FQuat>& Rotations);
    bool ShouldApply(const AActor* Owner);
    void ApplyRotation(FTransform& Bone, const FQuat& Rotation, bool Enabled);
}

// Final local-pose pass. Exactly fifteen finger rotations, no arm/wrist changes.
// State is copied on the game thread; Evaluate never reads actors/components.
struct FHearthwardAxeFingerPoseNode : FAnimNode_Base
{
    FPoseLink Source;
    bool Enabled = false;
    TArray<FQuat> Rotations;
    TArray<FBoneReference> Bones;

    FHearthwardAxeFingerPoseNode();
    virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    virtual void Evaluate_AnyThread(FPoseContext& Output) override;
};
