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

FHearthwardTransferResult UHearthwardStorageSubsystem::TransferInstance(UHearthwardInventoryComponent* Personal,bool ToCamp,FGuid Instance,FGuid Operation,FGuid Epoch)
{
    FHearthwardTransferResult Reply;
    if(Epoch!=State.Epoch){Reply.Result=EHearthwardInventoryResult::StaleTimeline;return Reply;}
    if(!IsValid(Personal) || Personal->GetWorld()!=GetWorld() || !Instance.IsValid() || !Operation.IsValid())return Reply;
    const FGuid PersonalId=Personal->GetContainerId();
    if(const auto* Previous=State.Completed.Find(Operation))
    {
        if(Previous->PersonalId!=PersonalId || Previous->ToCamp!=ToCamp || Previous->Instance!=Instance)Reply.Result=EHearthwardInventoryResult::OperationConflict;
        else {Reply.Result=Previous->Result;Reply.Replayed=true;}return Reply;
    }
    const auto* I=ToCamp?Personal->FindInstance(Instance):State.Shared.FindInstance(Instance);
    if(!I)return Reply;
    const FName Item=I->Definition;
    if((ToCamp?Personal->Available(Item):Available(Item))<1)return Reply;
    Reply.Result=ToCamp?Personal->State.TransferInstanceTo(State.Shared,Instance):State.Shared.TransferInstanceTo(Personal->State,Instance);
    State.Completed.Add(Operation,{PersonalId,ToCamp,Item,1,Reply.Result,Instance});
    if(Reply.Result==EHearthwardInventoryResult::Success)
    {
        Reply.MovedCount=1;Personal->OnInventoryChanged.Broadcast();OnTransferred.Broadcast(Operation,ToCamp,Item,1);
    }
    return Reply;
}
bool UHearthwardStorageSubsystem::PrepareWorkshop(UHearthwardInventoryComponent* Personal,const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,bool UseStorage,FGuid Repair,double Restore,bool Upgrade,FHearthwardInventoryState& BagAfter,FHearthwardInventoryState& CampAfter) const
{
    if(!IsValid(Personal) || Personal->GetWorld()!=GetWorld() || Materials.IsEmpty())return false;
    BagAfter=Personal->State;CampAfter=State.Shared;
    for(const auto& M:Materials)
    {
        if(M.Value<=0)return false;
        const int32 Own=FMath::Min(M.Value,Personal->Available(M.Key)),Camp=M.Value-Own;
        if(Camp>0 && (!UseStorage || Camp>Available(M.Key)))return false;
        if(Own>0 && BagAfter.Remove(M.Key,Own)!=EHearthwardInventoryResult::Success)return false;
        if(Camp>0 && CampAfter.Remove(M.Key,Camp)!=EHearthwardInventoryResult::Success)return false;
    }
    for(const auto& O:Outputs)if(BagAfter.Add(O.Key,O.Value)!=EHearthwardInventoryResult::Success)return false;
    if(Repair.IsValid() && !BagAfter.RestoreDurability(Repair,Restore))return false;
    if(Upgrade && !BagAfter.UpgradeBackpack())return false;
    return true;
}
bool UHearthwardStorageSubsystem::CanWorkshop(UHearthwardInventoryComponent* Personal,const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,bool UseStorage,FGuid Repair,double Restore,bool Upgrade) const
{
    FHearthwardInventoryState BagAfter,CampAfter(true);
    return PrepareWorkshop(Personal,Materials,Outputs,UseStorage,Repair,Restore,Upgrade,BagAfter,CampAfter);
}
bool UHearthwardStorageSubsystem::Workshop(UHearthwardInventoryComponent* Personal,const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,bool UseStorage,FGuid Repair,double Restore,bool Upgrade)
{
    FHearthwardInventoryState BagAfter,CampAfter(true);
    if(!PrepareWorkshop(Personal,Materials,Outputs,UseStorage,Repair,Restore,Upgrade,BagAfter,CampAfter))return false;
    Personal->State=MoveTemp(BagAfter);State.Shared=MoveTemp(CampAfter);
    Personal->OnInventoryChanged.Broadcast();return true;
}
