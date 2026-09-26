#include "HearthwardBuildingComponent.h"
#include "HearthwardWorkshopService.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Engine/World.h"
using namespace HearthwardData;
FString UHearthwardBuildingComponent::RepairStatus(FGuid Station,FName Item,FGuid Epoch) const
{ return RepairInstanceStatus(Station,GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>()->FirstInstance(Item),1,Epoch); }
bool UHearthwardBuildingComponent::RepairEquipment(FGuid Station,FName Item,FGuid Epoch)
{ return RepairInstance(Station,GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>()->FirstInstance(Item),1,Epoch); }
FString UHearthwardBuildingComponent::RepairInstanceStatus(FGuid Station,FGuid Instance,double Fraction,FGuid Epoch) const
{
    auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if(Epoch!=Store->GetTimelineEpoch())return TEXT("存档时间线已变化，请重新打开设施");
    const auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    if(!G || !G->CanChangeSkills())return TEXT("请结束动作并脱战后维修");
    const auto* Bag=GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>();const auto* I=Bag->FindInstance(Instance);
    if(!I)return TEXT("未持有这件装备");
    const auto Recipe=Find(TEXT("repairRecipes"),I->Definition.ToString());
    if(!Recipe || !MatchesFacility(Station,FName(*Text(Recipe,TEXT("facility"))),Number(Recipe,TEXT("facilityLevel"))))return TEXT("请靠近对应种类和等级的设施");
    TMap<FName,int32> Materials;double Restored;
    if(!HearthwardWorkshop::RepairQuote(Bag,Instance,Fraction,Materials,Restored))return TEXT("耐久已满或不支持维修");
    const bool Camp=!GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(GetOwner()->GetActorLocation()).IsNone();
    return Store->CanWorkshop(const_cast<UHearthwardInventoryComponent*>(Bag),Materials,{},Camp,Instance,Restored)?FString():TEXT("维修材料不足");
}
bool UHearthwardBuildingComponent::RepairInstance(FGuid Station,FGuid Instance,double Fraction,FGuid Epoch)
{
    Feedback=RepairInstanceStatus(Station,Instance,Fraction,Epoch);if(!Feedback.IsEmpty())return false;
    auto* Bag=GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>();const FName Item=Bag->FindInstance(Instance)->Definition;
    TMap<FName,int32> Materials;double Restored;
    if(!HearthwardWorkshop::RepairQuote(Bag,Instance,Fraction,Materials,Restored))return false;
    TGuardValue<bool> Guard(Settling,true);
    const bool Camp=!GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(GetOwner()->GetActorLocation()).IsNone();
    if(!GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Workshop(Bag,Materials,{},Camp,Instance,Restored))return false;
    GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->Record(TEXT("repair"),Item);
    Feedback=FString::Printf(TEXT("维修完成，恢复 %.2f 耐久"),Restored);return true;
}
