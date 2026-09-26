#include "HearthwardProgression.h"
#include "HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "Engine/World.h"
using namespace HearthwardData;
bool UHearthwardGameplayComponent::CanChangeSkills() const
{
    if(!Enabled || Health<=0 || InCombat())return false;
    if(const auto* C=GetOwner()->FindComponentByClass<UHearthwardCombatComponent>();C && C->Busy())return false;
    if(const auto* S=GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>();S && (!S->Alive() || S->Busy()))return false;
    if(const auto* A=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>();A && A->GetStatus()==EHearthwardTimedActionStatus::Running)return false;
    if(const auto* B=GetOwner()->FindComponentByClass<UHearthwardBuildingComponent>();B && (B->IsBuilding() || B->IsPlacing()))return false;
    return true;
}
bool UHearthwardGameplayComponent::GrantExperience(FName Kind,FName Fact,FGuid Epoch)
{
    if(!Enabled || Fact.IsNone() || Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch() || RewardFacts.Contains(Fact))return false;
    const auto Rewards=Catalog()->GetObjectField(TEXT("progression"))->GetObjectField(TEXT("experienceRewards"));
    const int32 XP=Number(Rewards,Kind.ToString());if(XP<=0)return false;
    RewardFacts.Add(Fact);Experience=int32(FMath::Min(int64(HearthwardProgression::MaximumExperience()),int64(Experience)+XP));
    OnChanged.Broadcast();return true;
}
bool UHearthwardGameplayComponent::KnowsRecipe(FName Recipe) const
{
    const auto R=Find(TEXT("craftingRecipes"),Recipe.ToString());
    return R && (Text(R,TEXT("knowledge"))==TEXT("known") || KnownRecipes.Contains(Recipe));
}
bool UHearthwardGameplayComponent::LearnBlueprint(FName Item)
{
    if(!CanChangeSkills())return Result(false,TEXT("当前无法学习图纸"));
    const FName Recipe(*Text(Find(TEXT("items"),Item.ToString()),TEXT("blueprint")));
    if(Recipe.IsNone() || KnownRecipes.Contains(Recipe) || Inventory()->Available(Item)<1)return Result(false,TEXT("图纸不存在或已经学会"));
    if(Inventory()->TryRemove(Item,1,false)!=EHearthwardInventoryResult::Success)return false;
    KnownRecipes.Add(Recipe);Inventory()->OnInventoryChanged.Broadcast();return Result(true,TEXT("已学会配方"));
}
float UHearthwardGameplayComponent::EquippedDurability(FName Slot) const
{
    const auto* I=Inventory()->FindInstance(Inventory()->EquippedInstance(Slot));return I?I->Durability:0;
}
void UHearthwardGameplayComponent::WearEquipment(FName Slot,double Amount,bool Armor)
{
    const double Wear=Amount/(1+Effect(TEXT("durability")))*(Armor?1-Effect(TEXT("armor_wear_reduction")):1);
    Inventory()->WearInstance(Inventory()->EquippedInstance(Slot),Wear);
}
bool UHearthwardGameplayComponent::EquipInstance(FGuid Id)
{ return GetOwner()->FindComponentByClass<UHearthwardCombatComponent>()->SwitchEquipmentInstance(Id); }
