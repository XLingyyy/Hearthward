#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UHearthwardInventoryComponent;
class UHearthwardStorageSubsystem;

struct FHearthwardCraftingMaterialNeed
{
    FName Item;
    int32 Needed=0,InBag=0,InStorage=0,Available=0,Missing=0;
};

// Session-only intent. The storage epoch invalidates it when a save timeline changes.
struct FHearthwardCraftingTracker
{
    FName Recipe;
    int32 Batches=1;
    FGuid Epoch;
    void Set(FName InRecipe,int32 InBatches,FGuid InEpoch);
    void Clear();
    bool IsCurrent(FGuid CurrentEpoch) const;
};

namespace HearthwardCraftingDiscovery
{
    FString Category(const TSharedPtr<FJsonObject>& Recipe);
    TArray<FHearthwardCraftingMaterialNeed> Materials(const TSharedPtr<FJsonObject>& Recipe,int32 Batches,
        const UHearthwardInventoryComponent* Bag,const UHearthwardStorageSubsystem* Storage,bool UseStorage);
}
