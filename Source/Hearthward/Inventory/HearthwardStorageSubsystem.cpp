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
    if(!State.Completed.Contains(OperationId))
    {
        if(ToCamp && Count>Personal->Available(ItemId)) return {};
        if(!ToCamp && Count>Available(ItemId)) return {};
    }
    const auto Reply = State.Transfer(Personal->State, Personal->GetContainerId(), ToCamp, ItemId, Count, OperationId, TimelineEpoch);
    if (Reply.MovedCount > 0)
    {
        // Both states and the replay journal are committed before callbacks can re-enter.
        Personal->OnInventoryChanged.Broadcast();
        OnTransferred.Broadcast(OperationId, ToCamp, ItemId, Reply.MovedCount);
    }
    return Reply;
}

int32 UHearthwardStorageSubsystem::Available(FName Item) const
{
    int32 Count=GetItemCount(Item);
    for(const auto& R:Reservations) Count-=R.Value.FindRef(Item);
    return Count;
}
bool UHearthwardStorageSubsystem::Reserve(FGuid Operation,const TMap<FName,int32>& Materials)
{
    if(!Operation.IsValid() || Reservations.Contains(Operation)) return false;
    for(const auto& M:Materials) if(M.Value<=0 || Available(M.Key)<M.Value) return false;
    Reservations.Add(Operation,Materials);return true;
}
bool UHearthwardStorageSubsystem::CanAdjust(const TMap<FName,int32>& Consumed,const TMap<FName,int32>& Produced,FGuid Reservation) const
{
    const auto* Own=Reservations.Find(Reservation);
    if(Reservation.IsValid() && !Own) return false;
    auto After=State.Shared;
    for(const auto& M:Consumed)
    {
        if(M.Value>Available(M.Key)+(Own?Own->FindRef(M.Key):0)) return false;
        if(After.Remove(M.Key,M.Value)!=EHearthwardInventoryResult::Success) return false;
    }
    for(const auto& O:Produced) if(After.Add(O.Key,O.Value)!=EHearthwardInventoryResult::Success) return false;
    return true;
}
bool UHearthwardStorageSubsystem::Adjust(const TMap<FName,int32>& Consumed,const TMap<FName,int32>& Produced,FGuid Reservation)
{
    if(!CanAdjust(Consumed,Produced,Reservation)) return false;
    for(const auto& M:Consumed) State.Shared.Remove(M.Key,M.Value);
    for(const auto& O:Produced) State.Shared.Add(O.Key,O.Value);
    if(Reservation.IsValid()) Reservations.Remove(Reservation);
    return true;
}
