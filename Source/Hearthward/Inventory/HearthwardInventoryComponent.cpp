#include "HearthwardInventoryComponent.h"

EHearthwardInventoryResult UHearthwardInventoryComponent::TransferTo(UHearthwardInventoryComponent* Target, FName ItemId, int32 Count)
{
    if (!IsValid(Target) || Target->GetWorld() != GetWorld()) return EHearthwardInventoryResult::InvalidArgument;
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

EHearthwardInventoryResult UHearthwardInventoryComponent::TryRemove(FName ItemId, int32 Count)
{
    const auto Result = State.Remove(ItemId, Count);
    if (Result == EHearthwardInventoryResult::Success) OnInventoryChanged.Broadcast();
    return Result;
}

EHearthwardInventoryResult UHearthwardInventoryComponent::TryConsume(const TMap<FName,int32>& Materials)
{
    auto After=State;
    for(const auto& M:Materials)
    {
        const auto Result=After.Remove(M.Key,M.Value);
        if(Result!=EHearthwardInventoryResult::Success) return Result;
    }
    State=MoveTemp(After); OnInventoryChanged.Broadcast();
    return EHearthwardInventoryResult::Success;
}
