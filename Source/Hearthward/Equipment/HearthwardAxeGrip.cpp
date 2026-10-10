#include "HearthwardAxeGrip.h"
#include "HearthwardAxeHandPose.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

namespace
{
bool ReferenceBone(const FReferenceSkeleton& Ref, FName Name, FTransform& Out)
{
    int32 Index = Ref.FindBoneIndex(Name);
    if (Index == INDEX_NONE) return false;
    Out = Ref.GetRefBonePose()[Index];
    while ((Index = Ref.GetParentIndex(Index)) != INDEX_NONE)
        Out *= Ref.GetRefBonePose()[Index];
    return true;
}
}

FVector HearthwardAxeGrip::HandleAxis()
{
    // TASK-055 actual production LOD0 shaft cross-sections at Z=-32.5/-22.5 cm.
    // The shaft is diagonal in mesh space, so treating mesh +Z as its axis is wrong.
    // Source: docs/qa/TASK-055/stone-axe-palm-surface-candidate.json,
    // shaft_axis_candidate.axe_source_slab_centres (geometry only, not its rejected pose).
    return (FVector(19.044886589050293, -16.212165594100952, -22.5)
        - FVector(26.028714895248413, -19.51271870136261, -32.5)).GetSafeNormal();
}

FTransform HearthwardAxeGrip::AlignHandle(const FVector& MeshGrip, const FQuat& RelativeRotation,
    const FVector& PalmInHand, const FVector& HandWorldScale)
{
    // Absolute scale stops the mesh inheriting hand scale, but attachment translation
    // still inherits it. Include the skeleton's imported root scale (100), not just
    // the character display scale (~1.84/~1.63), when cancelling the mesh grip offset.
    const FVector Offset = RelativeRotation.RotateVector(MeshGrip * WorldScale);
    return FTransform(RelativeRotation, PalmInHand - Offset / HandWorldScale, FVector(WorldScale));
}

bool HearthwardAxeGrip::Build(const USkeletalMesh* CharacterMesh, const FVector& CharacterScale,
    const UStaticMesh* AxeMesh, FTransform& OutRelativeTransform)
{
    if (!CharacterMesh || !AxeMesh) return false;
    const auto* Grip = AxeMesh->FindSocket(TEXT("Grip"));
    const auto* BladeBase = AxeMesh->FindSocket(TEXT("BladeBase"));
    const auto* BladeTip = AxeMesh->FindSocket(TEXT("BladeTip"));
    if (!Grip || !BladeBase || !BladeTip) return false;
    const auto& Ref = CharacterMesh->GetRefSkeleton();
    FTransform Hand, Thumb, Middle, Index, Pinky;
    if (!ReferenceBone(Ref, TEXT("hand_r"), Hand)
        || !ReferenceBone(Ref, TEXT("thumb_02_r"), Thumb)
        || !ReferenceBone(Ref, TEXT("middle_02_r"), Middle)
        || !ReferenceBone(Ref, TEXT("index_01_r"), Index)
        || !ReferenceBone(Ref, TEXT("pinky_01_r"), Pinky)) return false;
    const FVector HandWorldScale = Hand.GetScale3D() * CharacterScale;
    if (HandWorldScale.GetMin() <= UE_SMALL_NUMBER) return false;
    const FVector AcrossKnuckles = (Index.GetLocation() - Pinky.GetLocation()).GetSafeNormal();
    if (AcrossKnuckles.IsNearlyZero()) return false;
    // Use the same reference palm frame as the other held weapons. The authored
    // Grip is inside the wrapped handle, not the asset origin or the wrist pivot.
    const FQuat PalmFrame = FRotationMatrix::MakeFromZX(AcrossKnuckles, FVector::UpVector).ToQuat();
    const FVector TowardBlade = (BladeBase->RelativeLocation + BladeTip->RelativeLocation) * 0.5 - Grip->RelativeLocation;
    const FQuat AxeFrame = FRotationMatrix::MakeFromZX(HandleAxis(), TowardBlade).ToQuat();
    const FQuat RelativeRotation = Hand.GetRotation().Inverse() * PalmFrame * AxeFrame.Inverse();
    FVector PalmInHand;
    TArray<FQuat> FingerRotations;
    if (!HearthwardAxeHandPose::Profile(CharacterMesh, PalmInHand, FingerRotations)) return false;
    OutRelativeTransform = AlignHandle(Grip->RelativeLocation, RelativeRotation, PalmInHand, HandWorldScale);
    return true;
}
