#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "A3GameAssetImportLibrary.generated.h"

class USkeletalMesh;
class UPhysicsAsset;

UCLASS()
class A3GAMEASSETEDITOR_API UA3GameAssetImportLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Fit and assign collision bodies without an interactive creation dialog. */
    UFUNCTION(BlueprintCallable, Category="A3Game|Asset Import")
    static UPhysicsAsset* CreatePhysicsAsset(USkeletalMesh* SkeletalMesh, float MinBoneSizeCm = 1.0f);
};
