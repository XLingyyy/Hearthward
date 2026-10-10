#pragma once
#include "CoreMinimal.h"
struct FHitResult;

// Surface policy for actual contact receipts. No timer-generated steps or gameplay mutations.
namespace HearthwardFootstepSurface
{
    HEARTHWARD_API FName Canonical(FName Surface);
    HEARTHWARD_API FName FromMaterialPath(const FString& Path);
    HEARTHWARD_API FName Resolve(const FHitResult& Hit);
    HEARTHWARD_API FName TerrainAt(const FVector& ImpactPoint);
    HEARTHWARD_API FName EventFor(FName Surface);
    // Draw is in [0, Count-2] when Previous is valid, otherwise [0, Count-1].
    HEARTHWARD_API int32 SelectVariant(int32 Count,int32 Previous,int32 Draw);
}
