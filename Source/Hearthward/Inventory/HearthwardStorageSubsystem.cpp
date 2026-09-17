#include "HearthwardStorageSubsystem.h"
#include "HearthwardInventoryComponent.h"
#include "Engine/World.h"

bool UHearthwardStorageSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FHearthwardTransferResult UHearthwardStorageSubsystem::Transfer(UHearthwardInventoryComponent* Personal,
    bool ToCamp, FName ItemId, int32 Count, FGuid OperationId, FGuid TimelineEpoch)
{
    if (!IsValid(Personal) || Personal->GetWorld() != GetWorld()) return {};
    const auto Reply = State.Transfer(Personal->State, Personal->GetContainerId(), ToCamp, ItemId, Count, OperationId, TimelineEpoch);
    if (Reply.MovedCount > 0)
    {
        // Both states and the replay journal are committed before callbacks can re-enter.
        Personal->OnInventoryChanged.Broadcast();
        OnTransferred.Broadcast(OperationId, ToCamp, ItemId, Reply.MovedCount);
    }
    return Reply;
}
