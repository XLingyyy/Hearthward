#include "HearthwardInventoryComponent.h"
#include "JsonObjectConverter.h"

EHearthwardInventoryResult UHearthwardInventoryComponent::CheckExchange(const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,int32 Batches) const
{
    for(const auto& M:Materials) if(int64(M.Value)*Batches>Available(M.Key)) return EHearthwardInventoryResult::InsufficientItems;
    auto After=State;
    return After.Exchange(Materials,Outputs,Batches);
}
EHearthwardInventoryResult UHearthwardInventoryComponent::TryExchange(const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,int32 Batches)
{
    const auto Check=CheckExchange(Materials,Outputs,Batches);
    if(Check!=EHearthwardInventoryResult::Success) return Check;
    const auto R=State.Exchange(Materials,Outputs,Batches);
    if(R==EHearthwardInventoryResult::Success) OnInventoryChanged.Broadcast();
    return R;
}

EHearthwardInventoryResult UHearthwardInventoryComponent::TransferTo(UHearthwardInventoryComponent* Target, FName ItemId, int32 Count)
{
    if (!IsValid(Target) || Target->GetWorld() != GetWorld()) return EHearthwardInventoryResult::InvalidArgument;
    if(Count>Available(ItemId)) return EHearthwardInventoryResult::InsufficientItems;
    const auto Result = State.TransferTo(Target->State, ItemId, Count);
    if (Result == EHearthwardInventoryResult::Success)
    {
        OnInventoryChanged.Broadcast();
        Target->OnInventoryChanged.Broadcast();
    }
    return Result;
}

FGuid UHearthwardInventoryComponent::GetContainerId()
{
    if (!ContainerId.IsValid()) ContainerId = FGuid::NewGuid();
    return ContainerId;
}

EHearthwardInventoryResult UHearthwardInventoryComponent::TryAdd(FName ItemId, int32 Count)
{
    const auto Result = State.Add(ItemId, Count);
    if (Result == EHearthwardInventoryResult::Success) OnInventoryChanged.Broadcast();
    return Result;
}

EHearthwardInventoryResult UHearthwardInventoryComponent::TryRemove(FName ItemId, int32 Count, bool Notify)
{
    if(Count>Available(ItemId)) return EHearthwardInventoryResult::InsufficientItems;
    const auto Result = State.Remove(ItemId, Count);
    if (Result == EHearthwardInventoryResult::Success && Notify) OnInventoryChanged.Broadcast();
    return Result;
}

EHearthwardInventoryResult UHearthwardInventoryComponent::TryConsume(const TMap<FName,int32>& Materials)
{
    auto After=State;
    for(const auto& M:Materials)
    {
        if(M.Value>Available(M.Key)) return EHearthwardInventoryResult::InsufficientItems;
        const auto Result=After.Remove(M.Key,M.Value);
        if(Result!=EHearthwardInventoryResult::Success) return Result;
    }
    State=MoveTemp(After); OnInventoryChanged.Broadcast();
    return EHearthwardInventoryResult::Success;
}

bool UHearthwardInventoryComponent::Reserve(FName Item)
{
    if(!Reserved.IsNone() || Available(Item)<1) return false;
    Reserved=Item; return true;
}
bool UHearthwardInventoryComponent::CommitReservation(FName Remainder,bool Notify)
{
    if(Reserved.IsNone()) return false;
    TMap<FName,int32> Materials,Outputs;
    Materials.Add(Reserved,1); if(!Remainder.IsNone()) Outputs.Add(Remainder,1);
    const auto Result=Remainder.IsNone()?State.Remove(Reserved,1):State.Exchange(Materials,Outputs,1);
    if(Result!=EHearthwardInventoryResult::Success) return false;
    Reserved=NAME_None;
    if(Notify) OnInventoryChanged.Broadcast(); return true;
}

bool UHearthwardInventoryComponent::ReserveMaterials(const TMap<FName,int32>& Materials)
{
    if(!ReservedMaterials.IsEmpty()) return false;
    for(const auto& M:Materials) if(M.Value<=0 || Available(M.Key)<M.Value) return false;
    ReservedMaterials=Materials;return true;
}
bool UHearthwardInventoryComponent::CommitMaterials(bool Notify)
{
    auto After=State;
    for(const auto& M:ReservedMaterials) if(After.Remove(M.Key,M.Value)!=EHearthwardInventoryResult::Success) return false;
    State=MoveTemp(After);ReservedMaterials.Reset();if(Notify)OnInventoryChanged.Broadcast();return true;
}

bool UHearthwardInventoryComponent::RestoreInventory(const FHearthwardInventorySnapshot& Data,bool Notify)
{
    if(!State.Restore(Data))return false;
    ReleaseReservation();ReleaseMaterials();if(Notify)OnInventoryChanged.Broadcast();return true;
}
bool UHearthwardInventoryComponent::EquipInstance(FGuid Id)
{ if(!State.Equip(Id))return false;OnInventoryChanged.Broadcast();return true; }
bool UHearthwardInventoryComponent::WearInstance(FGuid Id,double Amount)
{ if(!State.Wear(Id,Amount))return false;OnInventoryChanged.Broadcast();return true; }
bool UHearthwardInventoryComponent::RepairInstance(FGuid Id,double Amount,const TMap<FName,int32>& Materials,bool Notify)
{
    auto After=State;
    for(const auto& M:Materials)
        if(M.Value>Available(M.Key) || After.Remove(M.Key,M.Value)!=EHearthwardInventoryResult::Success)return false;
    if(!After.RestoreDurability(Id,Amount))return false;
    State=MoveTemp(After);if(Notify)OnInventoryChanged.Broadcast();return true;
}
EHearthwardInventoryResult UHearthwardInventoryComponent::TransferInstanceTo(UHearthwardInventoryComponent* Target,FGuid Id)
{
    if(!IsValid(Target) || Target->GetWorld()!=GetWorld())return EHearthwardInventoryResult::InvalidArgument;
    const auto* I=FindInstance(Id);if(!I || Available(I->Definition)<1)return EHearthwardInventoryResult::InsufficientItems;
    const auto R=State.TransferInstanceTo(Target->State,Id);
    if(R==EHearthwardInventoryResult::Success){OnInventoryChanged.Broadcast();Target->OnInventoryChanged.Broadcast();}return R;
}
EHearthwardInventoryResult UHearthwardInventoryComponent::InsertInstance(const FHearthwardItemInstance& Instance,bool Notify)
{ const auto R=State.InsertInstance(Instance);if(Notify && R==EHearthwardInventoryResult::Success)OnInventoryChanged.Broadcast();return R; }
bool UHearthwardInventoryComponent::RemoveInstance(FGuid Id,bool Notify)
{
    const auto* I=FindInstance(Id);if(!I || Available(I->Definition)<1 || !State.RemoveInstance(Id))return false;
    if(Notify)OnInventoryChanged.Broadcast();return true;
}
bool UHearthwardInventoryComponent::UpgradeBackpack(bool Notify)
{ if(!State.UpgradeBackpack())return false;if(Notify)OnInventoryChanged.Broadcast();return true; }

FString UHearthwardInventoryComponent::DescribeInventory() const
{FString Json;FJsonObjectConverter::UStructToJsonObjectString(State.Snapshot(),Json);return Json;}

EHearthwardInventoryResult UHearthwardInventoryComponent::GatherFrom(UHearthwardInventoryComponent* Source,FName Item,int32 Count,FGuid Tool,double Wear)
{
    if(!IsValid(Source) || Source==this || Source->GetWorld()!=GetWorld() || Source->Available(Item)<Count)return EHearthwardInventoryResult::InvalidArgument;
    auto SourceAfter=Source->State,PersonalAfter=State;
    const auto Result=SourceAfter.TransferTo(PersonalAfter,Item,Count);
    if(Result!=EHearthwardInventoryResult::Success)return Result;
    if(!PersonalAfter.Wear(Tool,Wear))return EHearthwardInventoryResult::InvalidArgument;
    Source->State=MoveTemp(SourceAfter);State=MoveTemp(PersonalAfter);
    Source->OnInventoryChanged.Broadcast();OnInventoryChanged.Broadcast();return Result;
}
