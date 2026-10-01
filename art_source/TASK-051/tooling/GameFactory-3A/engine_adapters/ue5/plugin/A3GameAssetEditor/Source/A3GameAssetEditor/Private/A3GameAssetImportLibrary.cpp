#include "A3GameAssetImportLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"
#include "PhysicsAssetUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/Package.h"

UPhysicsAsset* UA3GameAssetImportLibrary::CreatePhysicsAsset(USkeletalMesh* SkeletalMesh, float MinBoneSizeCm)
{
    if (!SkeletalMesh || MinBoneSizeCm <= 0.0f)
        return nullptr;
    if (UPhysicsAsset* Existing = SkeletalMesh->GetPhysicsAsset())
        return Existing;

    const FString Name = SkeletalMesh->GetName() + TEXT("_PhysicsAsset");
    const FString PackagePath = FPackageName::GetLongPackagePath(SkeletalMesh->GetOutermost()->GetName()) + TEXT("/") + Name;
    UPackage* Package = CreatePackage(*PackagePath);
    UPhysicsAsset* Asset = FindObject<UPhysicsAsset>(Package, *Name);
    const bool bNew = Asset == nullptr;
    if (!Asset)
        Asset = NewObject<UPhysicsAsset>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
    FPhysAssetCreateParams Params;
    Params.MinBoneSize = MinBoneSizeCm;
    FText Error;
    if (!FPhysicsAssetUtils::CreateFromSkeletalMesh(Asset, SkeletalMesh, Params, Error, true, false))
    {
        UE_LOG(LogTemp, Error, TEXT("A3Game PhysicsAsset fit failed: %s"), *Error.ToString());
        if (bNew)
            Asset->ClearFlags(RF_Public | RF_Standalone);
        return nullptr;
    }
    if (bNew)
        FAssetRegistryModule::AssetCreated(Asset);
    Asset->MarkPackageDirty();
    SkeletalMesh->MarkPackageDirty();
    return Asset;
}
