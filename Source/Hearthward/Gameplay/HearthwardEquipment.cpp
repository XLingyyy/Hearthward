#include "HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardDroppedEquipment.h"
#include "HearthwardGameData.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
using namespace HearthwardData;
namespace
{
AHearthwardCompanionFixture* EquipmentBrother(UWorld* World)
{for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It)return *It;return nullptr;}
bool BrotherReady(AActor* Player,AHearthwardCompanionFixture* Brother)
{
    if(!Brother || Brother->EquipmentBusy() || Brother->IsActorBeingDestroyed() || FVector::Dist(Player->GetActorLocation(),Brother->GetActorLocation())>300)return false;
    const auto* S=Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    return S && S->Alive() && !S->Busy() && S->SafeToSave();
}
}
bool UHearthwardGameplayComponent::NearQuartermaster() const
{
    for(TActorIterator<AActor> It(GetWorld());It;++It)
        if(It->ActorHasTag(TEXT("Hearthward.Quartermaster")) && FVector::Dist(GetOwner()->GetActorLocation(),It->GetActorLocation())<=300)return true;
    return false;
}
bool UHearthwardGameplayComponent::NearStorage() const
{
    for(TActorIterator<AActor> It(GetWorld());It;++It)
        if(const auto* R=It->FindComponentByClass<UHearthwardResourceInteractionComponent>();R && R->CanAccessStorage(GetOwner()))return true;
    const auto* B=GetOwner()->FindComponentByClass<UHearthwardBuildingComponent>();
    for(const auto& F:GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Facilities)
        if(F.Kind==TEXT("warehouse_access") && B && B->CanUseFacility(F.Id))return true;
    return false;
}
bool UHearthwardGameplayComponent::UpgradeBackpack(bool Brother,FGuid Epoch)
{
    auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();auto* NPC=EquipmentBrother(GetWorld());
    if(Epoch!=Store->GetTimelineEpoch() || !CanChangeSkills() || !NearQuartermaster() || (Brother && !BrotherReady(GetOwner(),NPC)))return Result(false,TEXT("请双方脱战后靠近营地后勤员"));
    auto* Target=Brother?NPC->Bag.Get():Inventory();const auto Row=Find(TEXT("backpacks"),FString::FromInt(Target->BackpackRank()+1));
    if(!Row || Number(Row,TEXT("minimum_camp_tier"))>CampTier)return Result(false,TEXT("背包已满级或营地等阶不足"));
    TMap<FName,int32> Cost;for(const auto& M:Row->GetObjectField(TEXT("incremental_cost"))->Values)Cost.Add(FName(*M.Key),M.Value->AsNumber());
    // The selected brother pays from their own bag, followed by shared camp stock.
    if(!Store->Workshop(Target,Cost,{},true,FGuid(),0,true))return Result(false,TEXT("升级材料不足，未扣除物资"));
    return Result(true,FString::Printf(TEXT("背包已升级至 %.0f 容量"),Target->GetCapacity()));
}
bool UHearthwardGameplayComponent::TransferInventory(FName From,FName To,FName Item,int32 Count,FGuid Instance,FGuid Epoch)
{
    auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();auto* NPC=EquipmentBrother(GetWorld());
    if(Epoch!=Store->GetTimelineEpoch() || From==To || !CanChangeSkills())return Result(false,TEXT("当前无法转移物品"));
    if((From==TEXT("brother") || To==TEXT("brother")) && !BrotherReady(GetOwner(),NPC))return Result(false,TEXT("弟弟需在3米内、脱战且没有进行中动作"));
    if((From==TEXT("storage") || To==TEXT("storage")) && !NearStorage())return Result(false,TEXT("请靠近营地仓储"));
    auto Bag=[&](FName Who)->UHearthwardInventoryComponent*{return Who==TEXT("player")?Inventory():Who==TEXT("brother") && NPC?NPC->Bag.Get():nullptr;};
    auto* Source=Bag(From);auto* Target=Bag(To);EHearthwardInventoryResult R=EHearthwardInventoryResult::InvalidArgument;
    if(From==TEXT("storage") && Target)
        R=Instance.IsValid()?Store->TransferInstance(Target,false,Instance,FGuid::NewGuid(),Epoch).Result:Store->Transfer(Target,false,Item,Count,FGuid::NewGuid(),Epoch).Result;
    else if(To==TEXT("storage") && Source)
        R=Instance.IsValid()?Store->TransferInstance(Source,true,Instance,FGuid::NewGuid(),Epoch).Result:Store->Transfer(Source,true,Item,Count,FGuid::NewGuid(),Epoch).Result;
    else if(Source && Target)R=Instance.IsValid()?Source->TransferInstanceTo(Target,Instance):Source->TransferTo(Target,Item,Count);
    return Result(R==EHearthwardInventoryResult::Success,R==EHearthwardInventoryResult::Success?TEXT("已转移，装备耐久保留"):TEXT("数量、容量或物品状态不允许转移"));
}
bool UHearthwardGameplayComponent::EquipBrother(FGuid Instance,FGuid Epoch)
{
    auto* NPC=EquipmentBrother(GetWorld());
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch() || !CanChangeSkills() || !BrotherReady(GetOwner(),NPC))return Result(false,TEXT("弟弟需在3米内、脱战且没有进行中动作"));
    return Result(NPC->Bag->EquipInstance(Instance),TEXT("弟弟装备已更新"));
}
bool UHearthwardGameplayComponent::GrantItemReward(FName Fact,FName Item,int32 Count,FGuid Epoch)
{
    auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if(!Enabled || Fact.IsNone() || RewardFacts.Contains(Fact) || Epoch!=Store->GetTimelineEpoch() || Count<=0)return false;
    if(!Store->CanAdjust({},{{Item,Count}}))return false;
    RewardFacts.Add(Fact);
    if(!Store->Adjust({},{{Item,Count}})){RewardFacts.Remove(Fact);return false;}
    OnChanged.Broadcast();return true;
}

void UHearthwardGameplayComponent::GrantInitialEquipment()
{
    auto* Brother=EquipmentBrother(GetWorld());
    for(auto* Bag:TArray<UHearthwardInventoryComponent*>{Inventory(),Brother?Brother->Bag.Get():nullptr})
    {
        if(!Bag)continue;
        const FName Fact=Bag==Inventory()?FName(TEXT("initial:player")):FName(TEXT("initial:brother"));
        if(RewardFacts.Contains(Fact))continue;
        RewardFacts.Add(Fact);
        for(const auto& Item:Catalog()->GetObjectField(TEXT("loadout"))->Values)
            if(Bag==Inventory() || Item.Key!=TEXT("amulet"))Bag->TryAdd(FName(*Item.Key),Item.Value->AsNumber());
        for(const auto& I:Bag->Snapshot().Instances)
            if(!Text(Find(TEXT("items"),I.Definition.ToString()),TEXT("slot")).IsEmpty())Bag->EquipInstance(I.Id);
    }
    GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->RefreshQuartermasters();
}

bool UHearthwardGameplayComponent::DropEquipment(FGuid Instance,FGuid Epoch,bool ConfirmUnique)
{
    const auto* Item=Inventory()->FindInstance(Instance);
    if(!Item || !CanChangeSkills() || Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())return Result(false,TEXT("当前无法放下装备"));
    if(!Item->UniqueClaim.IsNone() && !ConfirmUnique)return Result(false,TEXT("这是唯一物品，请确认放下"));
    const auto Copy=*Item;
    FVector Position=GetOwner()->GetActorLocation()+GetOwner()->GetActorForwardVector()*120;
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(EquipmentDrop),false,GetOwner());
    if(GetWorld()->LineTraceSingleByChannel(Hit,Position,Position-FVector(0,0,500),ECC_WorldStatic,Query))Position=Hit.ImpactPoint+FVector(0,0,10);
    auto* Ground=GetWorld()->SpawnActor<AHearthwardDroppedEquipment>(Position,FRotator::ZeroRotator);
    if(!Ground)return false;
    Ground->Item=Copy;
    if(!Inventory()->RemoveInstance(Instance,false)){Ground->Destroy();return false;}
    Inventory()->OnInventoryChanged.Broadcast();return Result(true,TEXT("装备已放在脚边，可按 E 拾取；随存档保留"));
}
