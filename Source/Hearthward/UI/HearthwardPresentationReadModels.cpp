#include "HearthwardPresentationReadModels.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
FHearthwardCompanionWorkView Personal(FGuid Epoch,FGuid CommandId,FName Item,FName Intent,
    EHearthwardCompanionPhase Phase,int32 Requested,int32 Delivered,int32 Acquired,int32 Carried,
    const FString& BlockReason)
{
    using P=EHearthwardCompanionPhase;
    FHearthwardCompanionWorkView V;V.Epoch=Epoch;V.CommandId=CommandId;V.Item=Item;V.Intent=Intent;
    V.Phase=Phase;V.BlockReason=BlockReason;
    if(!Epoch.IsValid()){V.Reason=TEXT("当前时间线不可用");return V;}
    V.Available=true;V.HasTask=CommandId.IsValid() && Requested>0;
    if(!V.HasTask){V.Reason=TEXT("暂无个人委托");return V;}
    if(Delivered<0 || Delivered>Requested || Acquired<0 || Acquired>Requested || Carried<0 || Carried>Acquired)
    {V.Available=false;V.Reason=TEXT("委托数量不可用，请重新查看当前工作");return V;}
    V.Requested=Requested;V.Delivered=Delivered;V.Acquired=Acquired;V.Carried=Carried;
    V.RemainingToDeliver=Requested-Delivered;
    if(Intent==TEXT("collect") || Intent==TEXT("store") || Intent==TEXT("nature_collect"))V.DestinationText=TEXT("营地仓储");
    else if(Intent==TEXT("retrieve") || Intent==TEXT("give"))V.DestinationText=TEXT("玩家背包");
    else if(Intent==TEXT("fetch") || Intent==TEXT("receive") || Intent==TEXT("fish"))V.DestinationText=TEXT("弟弟背包");
    V.Terminal=Phase==P::Completed || Phase==P::Cancelled;
    V.CanResume=Phase==P::HoldingSafely || Phase==P::WaitingAtCamp;
    V.CanCancel=!V.Terminal && Phase!=P::Idle;
    if(!V.CanResume)V.ResumeReason=V.Terminal?TEXT("委托已经结束"):TEXT("当前阶段无需续接");
    if(!V.CanCancel)V.CancelReason=V.Terminal?TEXT("委托已经结束"):TEXT("当前没有执行中的委托");
    return V;
}
bool SameWorld(const UObject* Source,const UHearthwardStorageSubsystem* Storage)
{
    return IsValid(Source) && IsValid(Storage) && Source->GetWorld()
        && Source->GetWorld()==Storage->GetWorld();
}
}

bool FHearthwardCompanionWorkView::Matches(FGuid CurrentEpoch,FGuid CurrentCommand) const
{
    return Available && HasTask && Epoch.IsValid() && Epoch==CurrentEpoch
        && CommandId.IsValid() && CommandId==CurrentCommand;
}

FString HearthwardPresentation::CompanionPhaseText(EHearthwardCompanionPhase Phase)
{
    using P=EHearthwardCompanionPhase;
    switch(Phase)
    {
    case P::Idle:return TEXT("等待指令");
    case P::GoingToSource:return TEXT("前往采集点");
    case P::Gathering:return TEXT("正在采集");
    case P::Returning:return TEXT("返营入库");
    case P::ReturningBlocked:return TEXT("受阻返营");
    case P::WaitingAtCamp:return TEXT("营地等待");
    case P::Completed:return TEXT("委托完成");
    case P::Cancelled:return TEXT("委托已取消");
    case P::GoingToWorkshop:return TEXT("前往工坊");
    case P::TakingMaterials:return TEXT("领取材料");
    case P::HoldingSafely:return TEXT("受阻停留");
    case P::TakingCargo:return TEXT("领取物资");
    case P::GoingToPlayer:return TEXT("前往会合");
    case P::HandingOff:return TEXT("交付物资");
    case P::LeadingAnimal:return TEXT("牵引牲畜");
    case P::CampBatchWorking:return TEXT("营地生产");
    }
    return TEXT("工作状态不可用");
}

FString HearthwardPresentation::CompanionBlockText(const FString& Reason)
{
    if(Reason.StartsWith(TEXT("实际资源不足")))return TEXT("指定采集点已采尽；原委托只能在该来源再次可采后继续");
    if(Reason==TEXT("SOURCE_UNAVAILABLE"))return TEXT("指定资源点不可采；请检查该来源或等待刷新");
    if(Reason==TEXT("TOOL_REQUIRED"))return TEXT("缺少可用采集工具；请补充或维修工具");
    if(Reason==TEXT("ACTIVE_COMBAT"))return TEXT("附近正在战斗；安全后再继续");
    if(Reason==TEXT("AREA_UNSAFE") || Reason==TEXT("SOURCE_NOT_TRUSTED_SAFE") || Reason==TEXT("ROUTE_NOT_TRUSTED_SAFE"))return TEXT("工作区域或路线不安全；请先排除威胁");
    if(Reason==TEXT("去程受阻") || Reason==TEXT("PATH_BLOCKED") || Reason==TEXT("ROUTE_UNAVAILABLE"))return TEXT("路线受阻；请检查通路后重试");
    if(Reason==TEXT("STATION_UNAVAILABLE"))return TEXT("原工作设施不可用；请检查设施状态");
    if(Reason==TEXT("PLAYER_LEFT_CAMP"))return TEXT("玩家已离开任务营地；请回到原营地后重试");
    if(Reason==TEXT("BAG_INSUFFICIENT"))return TEXT("弟弟背包物资不足；请检查实际持有量");
    return Reason;
}

FString HearthwardPresentation::CompanionWorkText(const FHearthwardCompanionWorkView& V)
{
    if(!V.Available)return V.Reason.IsEmpty()?FString(TEXT("个人委托信息暂不可用")):V.Reason;
    if(!V.HasTask)return TEXT("暂无个人委托。");
    if(!V.Requested.IsSet() || !V.Delivered.IsSet() || !V.Carried.IsSet() || !V.RemainingToDeliver.IsSet())
        return TEXT("委托数量暂不可用，请重新查看当前工作");
    FString S=FString::Printf(TEXT("个人委托：%s\n已交付 %d / %d · 携带 %d\n尚未交付 %d\n阶段：%s"),
        *HearthwardAgent::ItemText(V.Item),V.Delivered.GetValue(),V.Requested.GetValue(),V.Carried.GetValue(),
        V.RemainingToDeliver.GetValue(),*CompanionPhaseText(V.Phase));
    if(!V.DestinationText.IsEmpty())S+=TEXT("\n目的地：")+V.DestinationText;
    if(!V.BlockReason.IsEmpty())S+=TEXT("\n停工原因：")+CompanionBlockText(V.BlockReason);
    if(V.Phase==EHearthwardCompanionPhase::Cancelled)S+=TEXT("\n已取得物资保留；这项委托已结束。");
    if(!V.Reason.IsEmpty())S+=TEXT("\n操作条件：")+V.Reason;
    else if(!V.Terminal && !V.CanResume && !V.ResumeReason.IsEmpty())S+=TEXT("\n续接条件：")+V.ResumeReason;
    return S;
}

FString HearthwardPresentation::WorkPartyText(const FHearthwardCampWorkView& V)
{
    if(!V.Available)return V.Reason.IsEmpty()?FString(TEXT("营地队伍信息暂不可用")):V.Reason;
    if(!V.HasTeam)return TEXT("当前没有弟弟带领的采集队。");
    FString S=FString::Printf(TEXT("营地采集队：%s\n%d名族人 · 弟弟另占1个名额\n生产状态：%s"),
        *HearthwardAgent::ItemText(V.Job),V.Workers.Num(),*V.Status);
    if(V.CompletedBatches.IsSet())S+=FString::Printf(TEXT("\n岗位累计入库 %lld 份"),int64(V.CompletedBatches.GetValue())*2);
    else S+=TEXT("\n岗位累计入库量暂不可用");
    S+=TEXT("\n弟弟：")+V.BrotherStatus;
    return S+TEXT("\n持续采集至暂停；产物进入营地仓储。");
}

FString HearthwardPresentation::PersonalActionToken(const FString& Action,const FHearthwardCompanionWorkView& V)
{
    return Action+TEXT(":")+V.Epoch.ToString(EGuidFormats::Digits)+TEXT(":")+V.CommandId.ToString(EGuidFormats::Digits);
}

FString HearthwardPresentation::WorkPartyActionToken(const FString& Action,const FHearthwardCampWorkView& V)
{
    TArray<FString> Workers;for(int32 Worker:V.Workers)Workers.Add(FString::FromInt(Worker));
    return Action+TEXT(":")+V.Epoch.ToString(EGuidFormats::Digits)+TEXT(":")+V.Region.ToString()
        +TEXT(":")+FString::Join(Workers,TEXT(","));
}

FHearthwardCompanionWorkView HearthwardPresentation::ProjectCompanion(
    const FHearthwardCompanionCommand& Command,EHearthwardCompanionPhase Phase,FGuid CurrentEpoch,
    const FString& BlockReason)
{
    const auto Ticket=Command.GetActive();
    if(Ticket.Id.IsValid() && Ticket.Epoch!=CurrentEpoch)
    {FHearthwardCompanionWorkView V;V.Reason=TEXT("这份委托记录属于旧时间线");return V;}
    return Personal(CurrentEpoch,Ticket.Id,Command.GetItem(),Command.Goal.Intent,Phase,
        Command.GetRequested(),Command.GetDelivered(),Command.GetAcquired(),Command.GetCarried(),BlockReason);
}

FHearthwardCompanionWorkView HearthwardPresentation::ReadCompanion(
    const AHearthwardCompanionFixture* Companion,const UHearthwardStorageSubsystem* Storage,AActor* Speaker)
{
    if(!SameWorld(Companion,Storage) || Companion->IsActorBeingDestroyed())
    {FHearthwardCompanionWorkView V;V.Reason=TEXT("弟弟当前不可用");return V;}
    auto* World=Companion->GetWorld();
    const auto* Save=World->GetSubsystem<UHearthwardSaveSubsystem>();
    if(Save && Save->IsRestoring())
    {FHearthwardCompanionWorkView V;V.Reason=TEXT("正在恢复存档，请稍后查看工作");return V;}
    auto V=Personal(Storage->GetTimelineEpoch(),Companion->GetCommandId(),Companion->GetItem(),
        Companion->GetGoal().Intent,Companion->GetPhase(),Companion->GetRequested(),Companion->GetDelivered(),
        Companion->GetAcquired(),Companion->GetCarried(),Companion->BlockReason);
    if(!Speaker || !Companion->CanCommunicate(Speaker) || World->IsPaused())
    {
        V.CanResume=V.CanCancel=false;
        if(V.HasTask && V.Available)
        {
            V.Reason=World->IsPaused()?TEXT("请返回游戏后操作委托"):TEXT("请靠近弟弟后操作委托");
            V.ResumeReason=V.CancelReason=V.Reason;
        }
    }
    return V;
}

FHearthwardCampWorkView HearthwardPresentation::ReadWorkParty(const UHearthwardCampSubsystem* Camp,
    const UHearthwardStorageSubsystem* Storage,FName CampId)
{
    FHearthwardCampWorkView V;
    if(!SameWorld(Camp,Storage) || CampId.IsNone() || !Storage->GetTimelineEpoch().IsValid())
    {V.Reason=TEXT("当前营地工作信息不可用");return V;}
    const auto* Save=Camp->GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    if(Save && Save->IsRestoring()){V.Reason=TEXT("正在恢复存档，请稍后查看工作");return V;}
    V.Available=true;V.Epoch=Storage->GetTimelineEpoch();V.Camp=CampId;
    const auto* R=Camp->State.Regions.FindByPredicate([&](const auto& Region)
    {return Region.Camp==CampId && Region.Brother && !Region.Facility.IsValid()
        && (Region.Job==TEXT("wood") || Region.Job==TEXT("stone"));});
    if(!R){V.Reason=TEXT("当前没有弟弟带领的采集队");return V;}
    V.HasTeam=true;V.Region=R->Id;V.Job=R->Job;V.Workers=R->Workers;
    V.Enabled=R->Enabled;V.Safe=R->Safe;V.AssignedBrother=R->Brother;
    V.BrotherWorking=R->Enabled && R->Safe && Camp->BrotherWorking();
    V.CompletedBatches=R->Completed;
    V.Status=!R->Enabled?TEXT("已暂停"):!R->Safe?TEXT("区域不安全"):!R->Status.IsEmpty()?R->Status:
        !R->Batch.Active && Camp->State.SourceIndex(*R)==INDEX_NONE?TEXT("等待资源刷新"):TEXT("采集中");
    if(!R->Enabled)V.BrotherStatus=TEXT("已暂停，尚未计入劳动力");
    else if(!R->Safe)V.BrotherStatus=TEXT("区域不安全，尚未计入劳动力");
    else if(V.BrotherWorking)V.BrotherStatus=TEXT("已到岗工作");
    else
    {
        V.BrotherStatus=TEXT("弟弟当前不可用，尚未计入劳动力");
        for(TActorIterator<AHearthwardCompanionFixture> It(Camp->GetWorld());It;++It)
        {
            if(!It->BlockReason.IsEmpty())V.BrotherStatus=CompanionBlockText(It->BlockReason)+TEXT("；尚未计入劳动力");
            else if(It->GetPhase()!=EHearthwardCompanionPhase::Idle && It->GetPhase()!=EHearthwardCompanionPhase::Completed
                && It->GetPhase()!=EHearthwardCompanionPhase::Cancelled)V.BrotherStatus=TEXT("正在执行个人委托，尚未计入岗位劳动力");
            else if(It->GetVelocity().SizeSquared()>25)V.BrotherStatus=TEXT("移动中，尚未计入岗位劳动力");
            else V.BrotherStatus=TEXT("尚未到岗或当前无法工作，尚未计入劳动力");
            break;
        }
    }
    return V;
}

bool HearthwardPresentation::CanApplyPersonalAction(const FHearthwardCompanionWorkView& View,
    const AHearthwardCompanionFixture* Companion,const UHearthwardStorageSubsystem* Storage,
    AActor* Speaker,bool Resume)
{
    if(!SameWorld(Companion,Storage) || Companion->IsActorBeingDestroyed()
        || !View.Matches(Storage->GetTimelineEpoch(),Companion->GetCommandId()))return false;
    const auto Current=ReadCompanion(Companion,Storage,Speaker);
    return Resume?View.CanResume && Current.CanResume:View.CanCancel && Current.CanCancel;
}
