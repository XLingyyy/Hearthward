#include "HearthwardInventoryComponent.h"

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
