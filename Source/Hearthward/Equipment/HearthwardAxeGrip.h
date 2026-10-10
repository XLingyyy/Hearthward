#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;
class UStaticMesh;

// Shared by player and companion. Weapon dimensions are absolute world centimetres;
// the attachment translation is in the imported (scaled) hand bone's coordinates.
namespace HearthwardAxeGrip
{
    constexpr double WorldScale = 0.7;
    FVector HandleAxis();
    FTransform AlignHandle(const FVector& MeshGrip, const FQuat& RelativeRotation,
        const FVector& PalmInHand, const FVector& HandWorldScale);
    bool Build(const USkeletalMesh* CharacterMesh, const FVector& CharacterScale,
        const UStaticMesh* AxeMesh, FTransform& OutRelativeTransform);
}
