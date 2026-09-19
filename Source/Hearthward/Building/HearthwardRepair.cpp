#include "HearthwardBuildingComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Engine/World.h"

using namespace HearthwardData;
FString UHearthwardBuildingComponent::RepairStatus(FGuid Station,FName Item,FGuid Epoch) const
{
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()) return TEXT("存档时间线已变化，请重新打开工作台");
    if(!CanUseWorkbench(Station)) return TEXT("请在安全处靠近已建成的工作台");
    const auto R=Find(TEXT("repairRecipes"),Item.ToString());
    const auto Definition=Find(TEXT("items"),Item.ToString());
    if(!R || !Definition || Number(Definition,TEXT("durability"))<=0) return TEXT("这件物品不支持维修");
    const auto* Bag=GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>();
    if(Bag->GetItemCount(Item)<1) return TEXT("背包中没有这件装备");
    const auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    const auto* Current=G->Durability.Find(Item);
    if(!Current || *Current>=Number(Definition,TEXT("durability"))) return TEXT("装备耐久已满，无需维修");
    for(const auto& M:R->GetObjectField(TEXT("materials"))->Values)
        if(Bag->GetItemCount(FName(*M.Key))<M.Value->AsNumber()) return TEXT("维修材料不足，请先采集、制作或从仓储取出");
    return FString();
}
bool UHearthwardBuildingComponent::RepairEquipment(FGuid Station,FName Item,FGuid Epoch)
{
    Feedback=RepairStatus(Station,Item,Epoch);
    if(!Feedback.IsEmpty()) return false;
    TGuardValue<bool> Guard(Settling,true);
    auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    const float Before=G->Durability[Item];
    const auto Definition=Find(TEXT("items"),Item.ToString());
    TMap<FName,int32> Materials;
    for(const auto& M:Find(TEXT("repairRecipes"),Item.ToString())->GetObjectField(TEXT("materials"))->Values)
        Materials.Add(FName(*M.Key),M.Value->AsNumber());
    // Inventory observers see the restored durability and the complete material debit together.
    // Settling blocks reentrant repairs and snapshots until the repair event has also been recorded.
    G->Durability[Item]=Number(Definition,TEXT("durability"));
    if(GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>()->TryConsume(Materials)!=EHearthwardInventoryResult::Success)
    { G->Durability[Item]=Before; Feedback=TEXT("材料状态已变化，维修未执行"); return false; }
    G->Record(TEXT("repair"),Item);
    Feedback=TEXT("维修完成：")+Text(Definition,TEXT("name"));
    return true;
}
