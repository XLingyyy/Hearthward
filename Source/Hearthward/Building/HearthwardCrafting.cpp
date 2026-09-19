#include "HearthwardBuildingComponent.h"
#include "HearthwardWorkshopService.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "Engine/World.h"

using namespace HearthwardData;
AActor* UHearthwardBuildingComponent::ResolveWorkbench(FGuid Id) const
{
    const auto* B=Built.FindByPredicate([&](const auto& X){return X.Id==Id && X.Recipe==TEXT("workbench") && X.Actor.IsValid();});return B?B->Actor.Get():nullptr;
}
FGuid UHearthwardBuildingComponent::KnownWorkbench(AActor* Observer) const
{
    if(!IsValid(Observer))return {};
    for(const auto& B:Built)if(B.Recipe==TEXT("workbench") && B.Actor.IsValid() && FVector::Dist(Observer->GetActorLocation(),B.Actor->GetActorLocation())<=3000)return B.Id;
    return {};
}
namespace
{
TMap<FName,int32> RecipeItems(const TSharedPtr<FJsonObject>& Recipe,const TCHAR* Field)
{
    TMap<FName,int32> Out;
    for(const auto& Entry:Recipe->GetObjectField(Field)->Values) Out.Add(FName(*Entry.Key),Entry.Value->AsNumber());
    return Out;
}
}
bool UHearthwardBuildingComponent::CanUseWorkbench(FGuid Station) const
{
    const auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    const auto* Timer=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>();
    if(!G->Enabled || G->Health<=0 || G->InCombat() || IsPlacing() || IsBuilding() || Timer->GetStatus()==EHearthwardTimedActionStatus::Running) return false;
    const auto* B=Built.FindByPredicate([&](const auto& Entry){return Entry.Id==Station && Entry.Recipe==TEXT("workbench") && Entry.Actor.IsValid();});
    if(!B) return false;
    const FVector Start=GetOwner()->GetActorLocation(),End=B->Actor->GetActorLocation();
    if(FVector::Dist(Start,End)>Number(Catalog()->GetObjectField(TEXT("crafting")),TEXT("reach"))) return false;
    FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(HearthwardWorkbench),false,GetOwner());
    Query.AddIgnoredActor(B->Actor.Get());
    return !GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Query);
}
FGuid UHearthwardBuildingComponent::NearbyWorkbench() const
{
    FGuid Nearest; double Distance=MAX_dbl;
    for(const auto& B:Built)
        if(CanUseWorkbench(B.Id))
        {
            const double D=FVector::DistSquared(GetOwner()->GetActorLocation(),B.Actor->GetActorLocation());
            if(D<Distance) { Distance=D; Nearest=B.Id; }
        }
    return Nearest;
}
FString UHearthwardBuildingComponent::CraftingStatus(FGuid Station,FName Recipe,int32 Batches,FGuid Epoch) const
{
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()) return TEXT("存档时间线已变化，请重新打开工作台");
    if(!CanUseWorkbench(Station)) return TEXT("工作台不可用，请在安全处靠近工作台");
    const auto R=Find(TEXT("craftingRecipes"),Recipe.ToString());
    if(!R) return TEXT("配方不存在");
    if(Batches<=0 || Batches>Number(Catalog()->GetObjectField(TEXT("crafting")),TEXT("maxBatches"))) return TEXT("制作批数超出范围");
    const auto Result=GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>()->CheckExchange(RecipeItems(R,TEXT("materials")),RecipeItems(R,TEXT("outputs")),Batches);
    if(Result==EHearthwardInventoryResult::Success) return FString();
    if(Result==EHearthwardInventoryResult::InsufficientItems) return TEXT("背包材料不足，请先采集或取出材料");
    if(Result==EHearthwardInventoryResult::CapacityExceeded) return TEXT("背包容量不足，制作未执行");
    return TEXT("配方或物品数量无效，制作未执行");
}
bool UHearthwardBuildingComponent::Craft(FGuid Station,FName Recipe,int32 Batches,FGuid Epoch)
{
    Feedback=CraftingStatus(Station,Recipe,Batches,Epoch);
    if(!Feedback.IsEmpty()) return false;
    // Prevent reentrant crafting or saving from observing inventory before its craft event.
    TGuardValue<bool> Guard(Settling,true);
    const auto R=Find(TEXT("craftingRecipes"),Recipe.ToString());
    if(!HearthwardWorkshop::Commit(GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>(),nullptr,TEXT("craft"),Recipe,Batches)) { Feedback=TEXT("物品状态已变化，制作未执行"); return false; }
    GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->Record(TEXT("craft"),Recipe,Batches);
    Feedback=FString::Printf(TEXT("制作完成：%s × %d 批"),*Text(R,TEXT("name")),Batches);
    return true;
}
