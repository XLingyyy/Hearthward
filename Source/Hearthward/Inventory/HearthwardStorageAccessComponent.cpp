#include "HearthwardStorageAccessComponent.h"
#include "HearthwardStorageSubsystem.h"
#include "Engine/World.h"

int32 UHearthwardStorageAccessComponent::GetItemCount(FName ItemId) const
{
    const auto* Storage = GetWorld() ? GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>() : nullptr;
    return Storage ? Storage->GetItemCount(ItemId) : 0;
}

FGuid UHearthwardStorageAccessComponent::GetContainerId() const
{
    const auto* Storage = GetWorld() ? GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>() : nullptr;
    return Storage ? Storage->GetContainerId() : FGuid();
}

FHearthwardTransferResult UHearthwardStorageAccessComponent::Transfer(UHearthwardInventoryComponent* Personal,
    bool ToCamp, FName ItemId, int32 Count, FGuid OperationId, FGuid TimelineEpoch)
{
    auto* Storage = GetWorld() ? GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>() : nullptr;
    return Storage ? Storage->Transfer(Personal, ToCamp, ItemId, Count, OperationId, TimelineEpoch) : FHearthwardTransferResult();
}
