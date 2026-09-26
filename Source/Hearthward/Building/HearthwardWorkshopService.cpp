#include "HearthwardWorkshopService.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HearthwardBuildingComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
using namespace HearthwardData;
namespace
{
TMap<FName,int32> WorkshopCounts(const TSharedPtr<FJsonObject>& R,const TCHAR* Field,int32 N)
{
    TMap<FName,int32> Out;if(!R || N<1 || N>99)return Out;
    for(const auto& V:R->GetObjectField(Field)->Values)Out.Add(FName(*V.Key),int32(V.Value->AsNumber())*N);return Out;
}
}
TMap<FName,int32> HearthwardWorkshop::Materials(FName Intent,FName Item,int32 N)
{return WorkshopCounts(Find(Intent==TEXT("craft")?TEXT("craftingRecipes"):TEXT("repairRecipes"),Item.ToString()),TEXT("materials"),N);}
TMap<FName,int32> HearthwardWorkshop::Outputs(FName Recipe,int32 N)
{return WorkshopCounts(Find(TEXT("craftingRecipes"),Recipe.ToString()),TEXT("outputs"),N);}
FString HearthwardWorkshop::Check(AActor* Operator,AActor* Station,UHearthwardInventoryComponent* Bag,FName Intent,FName Item,int32 N)
{
    if(!IsValid(Operator) || !IsValid(Station) || Operator->IsActorBeingDestroyed() || Station->IsActorBeingDestroyed() || !IsValid(Bag)
        || Bag->GetOwner()!=Operator || Station->GetWorld()!=Operator->GetWorld() || Operator->GetWorld()->IsPaused())return TEXT("UNAVAILABLE");
    if(FVector::Dist(Operator->GetActorLocation(),Station->GetActorLocation())>Number(Catalog()->GetObjectField(TEXT("crafting")),TEXT("reach")))return TEXT("OUT_OF_RANGE");
    FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(NPCWorkshop),false,Operator);Params.AddIgnoredActor(Station);
    if(Operator->GetWorld()->LineTraceSingleByChannel(Hit,Operator->GetActorLocation(),Station->GetActorLocation(),ECC_Visibility,Params))return TEXT("PATH_BLOCKED");
    auto* Player=UGameplayStatics::GetPlayerPawn(Operator->GetWorld(),0);
    auto* Builder=Player?Player->FindComponentByClass<UHearthwardBuildingComponent>():nullptr;
    auto* G=Player?Player->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    const auto Recipe=Find(Intent==TEXT("craft")?TEXT("craftingRecipes"):TEXT("repairRecipes"),Item.ToString());
    bool Facility=false;
    for(const auto& F:Operator->GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Facilities)
        if(Builder && Builder->ResolveFacility(F.Id)==Station && !F.Paused && F.Kind.ToString()==Text(Recipe,TEXT("facility")) && F.Level>=Number(Recipe,TEXT("facilityLevel")))Facility=true;
    if(!Facility)return TEXT("STATION_UNAVAILABLE");
    if(Intent==TEXT("craft") && (!G || !G->KnowsRecipe(Item)))return TEXT("RECIPE_UNKNOWN");
    auto Cost=Materials(Intent,Item,N);if(Cost.IsEmpty())return TEXT("UNSUPPORTED_CAPABILITY");
    if(Intent==TEXT("repair"))
    {
        double Restored;
        if(N!=1 || Bag->GetItemCount(Item)!=1)return TEXT("AMBIGUOUS_TARGET");
        if(!RepairQuote(Bag,Bag->FirstInstance(Item),1,Cost,Restored))return TEXT("ALREADY_REPAIRED");
        for(const auto& C:Cost)if(Bag->Available(C.Key)<C.Value)return TEXT("INSUFFICIENT_MATERIAL");
        return {};
    }
    auto R=Bag->CheckExchange(Cost,Intent==TEXT("craft")?Outputs(Item,N):TMap<FName,int32>(),1);
    return R==EHearthwardInventoryResult::Success?FString():R==EHearthwardInventoryResult::CapacityExceeded?TEXT("CAPACITY_EXCEEDED"):TEXT("INSUFFICIENT_MATERIAL");
}
bool HearthwardWorkshop::Commit(UHearthwardInventoryComponent* Bag,FName Intent,FName Item,int32 N)
{
    auto Cost=Materials(Intent,Item,N);if(!IsValid(Bag) || Cost.IsEmpty())return false;
    if(Intent==TEXT("craft"))return Bag->TryExchange(Cost,Outputs(Item,N),1)==EHearthwardInventoryResult::Success;
    double Restored;
    if(Intent!=TEXT("repair") || N!=1 || Bag->GetItemCount(Item)!=1 || !RepairQuote(Bag,Bag->FirstInstance(Item),1,Cost,Restored))return false;
    return Bag->RepairInstance(Bag->FirstInstance(Item),Restored,Cost);
}

bool HearthwardWorkshop::RepairQuote(const UHearthwardInventoryComponent* Bag,FGuid Instance,double Fraction,TMap<FName,int32>& Materials,double& Restored)
{
    Materials.Reset();Restored=0;
    if(!Bag || (Fraction!=.25 && Fraction!=.5 && Fraction!=1))return false;
    const auto* I=Bag->FindInstance(Instance);if(!I)return false;
    const auto Definition=Find(TEXT("items"),I->Definition.ToString()),Recipe=Find(TEXT("repairRecipes"),I->Definition.ToString());
    const double Maximum=Number(Definition,TEXT("durability"));
    if(!Recipe || Maximum<=0 || I->Durability>=Maximum)return false;
    Restored=FMath::Min(Maximum-I->Durability,Maximum*Fraction);
    for(const auto& M:Recipe->GetObjectField(TEXT("basis"))->Values)
        Materials.Add(FName(*M.Key),FMath::Max(1,FMath::CeilToInt(M.Value->AsNumber()*.2*Restored/Maximum-1.e-10)));
    return !Materials.IsEmpty();
}
