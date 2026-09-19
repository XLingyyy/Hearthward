#include "HearthwardWorkshopService.h"
#include "HearthwardBuildingComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
using namespace HearthwardData;
namespace
{
TMap<FName,int32> Counts(const TSharedPtr<FJsonObject>& R,const TCHAR* Field,int32 N)
{
    TMap<FName,int32> Out;if(!R || N<1 || N>99)return Out;
    for(const auto& V:R->GetObjectField(Field)->Values)Out.Add(FName(*V.Key),int32(V.Value->AsNumber())*N);return Out;
}
}
TMap<FName,int32> HearthwardWorkshop::Materials(FName Intent,FName Item,int32 N)
{return Counts(Find(Intent==TEXT("craft")?TEXT("craftingRecipes"):TEXT("repairRecipes"),Item.ToString()),TEXT("materials"),N);}
TMap<FName,int32> HearthwardWorkshop::Outputs(FName Recipe,int32 N)
{return Counts(Find(TEXT("craftingRecipes"),Recipe.ToString()),TEXT("outputs"),N);}
FString HearthwardWorkshop::Check(AActor* Operator,AActor* Station,UHearthwardInventoryComponent* Bag,const TMap<FName,float>* Durability,FName Intent,FName Item,int32 N)
{
    if(!IsValid(Operator) || !IsValid(Station) || Operator->IsActorBeingDestroyed() || Station->IsActorBeingDestroyed() || !IsValid(Bag)
        || Bag->GetOwner()!=Operator || Station->GetWorld()!=Operator->GetWorld() || Operator->GetWorld()->IsPaused())return TEXT("UNAVAILABLE");
    if(FVector::Dist(Operator->GetActorLocation(),Station->GetActorLocation())>Number(Catalog()->GetObjectField(TEXT("crafting")),TEXT("reach")))return TEXT("OUT_OF_RANGE");
    FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(NPCWorkshop),false,Operator);Params.AddIgnoredActor(Station);
    if(Operator->GetWorld()->LineTraceSingleByChannel(Hit,Operator->GetActorLocation(),Station->GetActorLocation(),ECC_Visibility,Params))return TEXT("PATH_BLOCKED");
    auto Cost=Materials(Intent,Item,N);if(Cost.IsEmpty())return TEXT("UNSUPPORTED_CAPABILITY");
    if(Intent==TEXT("repair"))
    {
        if(N!=1 || Bag->GetItemCount(Item)!=1 || !Durability || !Durability->Contains(Item))return TEXT("AMBIGUOUS_TARGET");
        auto Def=Find(TEXT("items"),Item.ToString());if(!Def || (*Durability)[Item]>=Number(Def,TEXT("durability")))return TEXT("ALREADY_REPAIRED");
        for(const auto& C:Cost)if(Bag->GetItemCount(C.Key)<C.Value)return TEXT("INSUFFICIENT_MATERIAL");
        return {};
    }
    auto R=Bag->CheckExchange(Cost,Intent==TEXT("craft")?Outputs(Item,N):TMap<FName,int32>(),1);
    return R==EHearthwardInventoryResult::Success?FString():R==EHearthwardInventoryResult::CapacityExceeded?TEXT("CAPACITY_EXCEEDED"):TEXT("INSUFFICIENT_MATERIAL");
}
bool HearthwardWorkshop::Commit(UHearthwardInventoryComponent* Bag,TMap<FName,float>* Durability,FName Intent,FName Item,int32 N)
{
    auto Cost=Materials(Intent,Item,N);if(!IsValid(Bag) || Cost.IsEmpty())return false;
    if(Intent==TEXT("craft"))return Bag->TryExchange(Cost,Outputs(Item,N),1)==EHearthwardInventoryResult::Success;
    if(Intent!=TEXT("repair") || N!=1 || !Durability || !Durability->Contains(Item) || Bag->GetItemCount(Item)<1)return false;
    if((*Durability)[Item]>=Number(Find(TEXT("items"),Item.ToString()),TEXT("durability")))return false;
    const float Before=(*Durability)[Item];(*Durability)[Item]=Number(Find(TEXT("items"),Item.ToString()),TEXT("durability"));
    if(Bag->TryConsume(Cost)==EHearthwardInventoryResult::Success)return true;
    (*Durability)[Item]=Before;return false;
}
