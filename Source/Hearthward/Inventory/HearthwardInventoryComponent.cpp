#include "HearthwardInventoryComponent.h"

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
