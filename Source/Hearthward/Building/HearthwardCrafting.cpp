#include "HearthwardBuildingComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Interaction/HearthwardInteractionComponent.h"
#include "../Interaction/HearthwardInteractionTargetComponent.h"
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
    if(Nearest.IsValid())
        if(const auto* Interaction=GetOwner()->FindComponentByClass<UHearthwardInteractionComponent>())
            if(const auto* Target=Interaction->GetNearestTarget(); Target && FVector::DistSquared(GetOwner()->GetActorLocation(),Target->GetComponentLocation())<Distance)
                return {};
    return Nearest;
}
bool UHearthwardBuildingComponent::MatchesFacility(FGuid Station,FName Kind,int32 Level) const
{
    const auto& Facilities=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Facilities;
    const auto* F=Facilities.FindByPredicate([&](const auto& Row){return Row.Id==Station;});
    return F && !F->Paused && F->Kind==Kind && F->Level>=Level && CanUseFacility(Station);
}
FString UHearthwardBuildingComponent::CraftingStatus(FGuid Station,FName Recipe,int32 Batches,FGuid Epoch) const
{
    auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if(Epoch!=Store->GetTimelineEpoch())return TEXT("存档时间线已变化，请重新打开设施");
    const auto R=Find(TEXT("craftingRecipes"),Recipe.ToString());
    auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    if(!R || !G || !G->CanChangeSkills())return TEXT("当前无法加工，请结束动作并脱战");
    if(!MatchesFacility(Station,FName(*Text(R,TEXT("facility"))),Number(R,TEXT("facilityLevel"))))return TEXT("请靠近对应种类和等级的设施");
    if(!G->KnowsRecipe(Recipe))return TEXT("尚未学会图纸配方");
    if(Batches<1 || Batches>99)return TEXT("制作批数超出范围");
    auto* Bag=GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>();
    const bool Camp=!GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(GetOwner()->GetActorLocation()).IsNone();
    return Store->CanWorkshop(Bag,HearthwardWorkshop::Materials(TEXT("craft"),Recipe,Batches),HearthwardWorkshop::Outputs(Recipe,Batches),Camp)?FString():TEXT("材料不足或产出超重，制作未执行");
}
bool UHearthwardBuildingComponent::Craft(FGuid Station,FName Recipe,int32 Batches,FGuid Epoch)
{
    Feedback=CraftingStatus(Station,Recipe,Batches,Epoch);if(!Feedback.IsEmpty())return false;
    TGuardValue<bool> Guard(Settling,true);
    auto* Bag=GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>();
    const bool Camp=!GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(GetOwner()->GetActorLocation()).IsNone();
    if(!GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Workshop(Bag,HearthwardWorkshop::Materials(TEXT("craft"),Recipe,Batches),HearthwardWorkshop::Outputs(Recipe,Batches),Camp))
    {Feedback=TEXT("库存已变化，制作未执行");return false;}
    GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->Record(TEXT("craft"),Recipe,Batches);
    Feedback=TEXT("制作完成，产物已放入背包");return true;
}
