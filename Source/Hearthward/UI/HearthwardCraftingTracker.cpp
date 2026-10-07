#include "HearthwardCraftingTracker.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"

void FHearthwardCraftingTracker::Set(FName InRecipe,int32 InBatches,FGuid InEpoch)
{ Recipe=InRecipe;Batches=InBatches;Epoch=InEpoch; }
void FHearthwardCraftingTracker::Clear()
{ Recipe=NAME_None;Batches=1;Epoch.Invalidate(); }
bool FHearthwardCraftingTracker::IsCurrent(FGuid CurrentEpoch) const
{ return !Recipe.IsNone() && Epoch.IsValid() && Epoch==CurrentEpoch; }

FString HearthwardCraftingDiscovery::Category(const TSharedPtr<FJsonObject>& Recipe)
{
    using namespace HearthwardData;
    FString Category=Text(Recipe,TEXT("category"));
    if(!Category.IsEmpty())return Category;
    for(const auto& Output:Recipe->GetObjectField(TEXT("outputs"))->Values)
    {
        const FString ItemCategory=Text(Find(TEXT("items"),FString(*Output.Key)),TEXT("category"));
        if(Category.IsEmpty())Category=ItemCategory;
        else if(Category!=ItemCategory)return TEXT("其他");
    }
    return Category.IsEmpty()?FString(TEXT("其他")):Category;
}
TArray<FHearthwardCraftingMaterialNeed> HearthwardCraftingDiscovery::Materials(
    const TSharedPtr<FJsonObject>& Recipe,int32 Batches,const UHearthwardInventoryComponent* Bag,
    const UHearthwardStorageSubsystem* Storage,bool UseStorage)
{
    TArray<FHearthwardCraftingMaterialNeed> Out;
    for(const auto& Entry:Recipe->GetObjectField(TEXT("materials"))->Values)
    {
        FHearthwardCraftingMaterialNeed Need;Need.Item=FName(*Entry.Key);
        Need.Needed=int32(Entry.Value->AsNumber())*Batches;
        Need.InBag=Bag->Available(Need.Item);Need.InStorage=Storage->Available(Need.Item);
        Need.Available=Need.InBag+(UseStorage?Need.InStorage:0);
        Need.Missing=FMath::Max(0,Need.Needed-Need.Available);Out.Add(Need);
    }
    Out.Sort([](const auto& A,const auto& B){return A.Item.LexicalLess(B.Item);});
    return Out;
}
