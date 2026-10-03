#include "HearthwardWorldClockSubsystem.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardWorldPresentation.h"
#include "../UI/HearthwardLoadingSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void UHearthwardWorldClockSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    const auto* Settings=HearthwardData::Catalog()->Values.Find(TEXT("time"));
    const auto Row=Settings?(*Settings)->AsObject():nullptr;
    Clock.SetOrigin(HearthwardData::Number(Row,TEXT("initial_day"),1),HearthwardData::Number(Row,TEXT("initial_minute"),1200));
}
bool UHearthwardWorldClockSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{ return Type==EWorldType::Game || Type==EWorldType::PIE; }
bool UHearthwardWorldClockSubsystem::IsTickable() const
{ return IsInitialized() && GetWorld()->HasBegunPlay(); }
bool UHearthwardWorldClockSubsystem::Suspended() const
{
    if(Busy() || GetWorld()->IsPaused())return true;
    if(const auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();Save && Save->IsRestoring())return true;
    if(const auto* GI=GetWorld()->GetGameInstance())
        if(const auto* Loading=GI->GetSubsystem<UHearthwardLoadingSubsystem>();Loading && Loading->IsLoading())return true;
    return false;
}
void UHearthwardWorldClockSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if(!Suspended())AdvanceSurvival(DeltaTime,DeltaTime);
}
TStatId UHearthwardWorldClockSubsystem::GetStatId() const
{ RETURN_QUICK_DECLARE_CYCLE_STAT(UHearthwardWorldClockSubsystem, STATGROUP_Tickables); }
double UHearthwardWorldClockSubsystem::DaylightAt(double Minute)
{
    const auto* Settings=HearthwardData::Catalog()->Values.Find(TEXT("time"));
    const auto Row=Settings?(*Settings)->AsObject():nullptr;
    const double Dawn=HearthwardData::Number(Row,TEXT("sunrise_minute"),360);
    const double Dusk=HearthwardData::Number(Row,TEXT("sunset_minute"),1080);
    const double Transition=HearthwardData::Number(Row,TEXT("light_transition_minutes"),30);
    auto Smooth=[](double T){T=FMath::Clamp(T,0.,1.);return T*T*(3-2*T);};
    return Smooth((Minute-Dawn+Transition/2)/Transition)*(1-Smooth((Minute-Dusk+Transition/2)/Transition));
}
FHearthwardClockSnapshot UHearthwardWorldClockSubsystem::GetSnapshot() const
{
    FHearthwardClockSnapshot R;
    R.ActivePlaySeconds=Clock.GetActivePlaySeconds();R.ElapsedCalendarMinutes=Clock.GetElapsedCalendarMinutes();
    R.ElapsedDays=Clock.GetElapsedDays();R.MinuteOfDay=Clock.GetMinuteOfDay();R.DisplayDay=Clock.GetDisplayDay();R.Daylight=DaylightAt(R.MinuteOfDay);
    return R;
}
void UHearthwardWorldClockSubsystem::Install(double Active,double Calendar,int64 InitialDay,double InitialMinute)
{
    Clock.ActivePlaySeconds=Active;Clock.CalendarMinutes=Calendar;Clock.SetOrigin(InitialDay,InitialMinute);
    Receipts.Reset();ReceiptEpoch.Invalidate();
    GetWorld()->GetSubsystem<UHearthwardWorldPresentation>()->ApplyTime(Clock.GetMinuteOfDay());
}
double UHearthwardWorldClockSubsystem::AdvanceSurvival(double Active,double Calendar)
{
    if(Busy() || !FMath::IsFinite(Calendar) || Calendar<=0 || UHearthwardSurvivalComponent::HasFailed(GetWorld()))return 0;
    TGuardValue<bool> Guard(Advancing,true);
    TArray<UHearthwardSurvivalComponent*> Participants;
    for(TActorIterator<AActor> It(GetWorld());It;++It)
        if(auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>();S && S->Enabled())Participants.Add(S);
    auto* Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
    auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
    const double Start=Clock.CalendarMinutes,End=Start+Calendar,Ratio=Active/Calendar;
    while(Clock.CalendarMinutes<End-1.e-8 && !UHearthwardSurvivalComponent::HasFailed(GetWorld()))
    {
        double Step=FMath::Min(End-Clock.CalendarMinutes,Camp->State.NextBoundary(Active==0));
        Step=FMath::Min(Step,Nature->NextBoundary());
        Step=FMath::Min(Step,Campaign->NextBoundary(Clock.CalendarMinutes));
        double Fraction=1;
        for(auto* S:Participants)Fraction=FMath::Min(Fraction,S->PreviewAdvance(Step*Ratio,Step,Clock.CalendarMinutes));
        Step*=Fraction;
        for(auto* S:Participants)S->AdvanceContinuous(Step*Ratio,Step,Clock.CalendarMinutes);
        // Commit the shared boundary before inventory notifications can observe it. Busy rejects reentry.
        Clock.ActivePlaySeconds+=Step*Ratio;Clock.CalendarMinutes+=Step;
        Camp->Advance(Step,Active==0);Nature->Advance(Step);
        Camp->State.Calendar=Nature->State.Calendar=Clock.CalendarMinutes;
        Campaign->AdvanceBoundary(Clock.CalendarMinutes);
        if(Active>0 && !UHearthwardSurvivalComponent::HasFailed(GetWorld()))
            for(auto* S:Participants)S->CompleteBoundary(Step*Ratio);
        if(Fraction<1 || Step<=1.e-8)break;
    }
    GetWorld()->GetSubsystem<UHearthwardWorldPresentation>()->ApplyTime(Clock.GetMinuteOfDay());
    return Clock.CalendarMinutes-Start;
}
FHearthwardTimeAdvanceReceipt UHearthwardWorldClockSubsystem::RequestTimeAdvance(const FHearthwardTimeAdvanceRequest& R)
{
    FHearthwardTimeAdvanceReceipt Result;
    Result.OperationId=R.OperationId;Result.StartW=Result.EndW=Clock.CalendarMinutes;
    const FGuid Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    auto Reject=[&](FName Code,const TCHAR* Reason)
    {
        Result.ReasonCode=Code;Result.Reason=Reason;
        if(R.Epoch==Epoch && R.OperationId.IsValid() && !Receipts.Contains(R.OperationId))Receipts.Add(R.OperationId,{R,Result});
        return Result;
    };
    if(R.Epoch!=Epoch)return Reject(TEXT("STALE_TIMELINE"),TEXT("时间请求已过期，请重新打开设施"));
    if(ReceiptEpoch!=Epoch){Receipts.Reset();ReceiptEpoch=Epoch;}
    if(const auto* Cached=Receipts.Find(R.OperationId))
        return Cached->Request==R?Cached->Receipt:Reject(TEXT("STALE_REQUEST"),TEXT("重复请求的参数发生变化"));
    if(Suspended())return Reject(GetWorld()->IsPaused()?TEXT("PAUSED"):Busy()?TEXT("BUSY"):TEXT("LOADING"),TEXT("世界正在暂停、加载或结算"));
    if(!R.OperationId.IsValid() || !R.Campaign.IsValid() || R.Campaign!=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetCampaignId()
        || !FMath::IsFinite(R.StartW) || R.StartW!=Clock.CalendarMinutes)return Reject(TEXT("STALE_REQUEST"),TEXT("时间请求与当前进度不一致"));
    const bool Duration=R.Kind==EHearthwardTimeAdvanceKind::Sleep?R.Minutes==480:(R.Minutes==60 || R.Minutes==240 || R.Minutes==480);
    if(!Duration)return Reject(TEXT("INVALID_DURATION"),TEXT("请选择有效的等待时长"));
    auto* Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
    auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
    auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    auto* Builder=Player?Player->FindComponentByClass<UHearthwardBuildingComponent>():nullptr;
    const auto* Facility=Camp->State.Facilities.FindByPredicate([&](const auto& F){return F.Id==R.Facility;});
    if(Camp->Settling || Nature->Busy() || Nature->State.Animals.ContainsByPredicate([](const auto& A){return A.Following;}) || Campaign->Busy() || !Builder || !Facility || Facility->Paused || !Builder->CanUseFacility(R.Facility)
        || Facility->Kind!=(R.Kind==EHearthwardTimeAdvanceKind::Sleep?FName(TEXT("bed")):FName(TEXT("campfire"))))return Reject(TEXT("FACILITY_UNAVAILABLE"),TEXT("请靠近可用设施，先结束当前动作"));
    bool BrotherPresent=false;
    for(TActorIterator<AActor> It(GetWorld());It;++It)
    {
        if(const auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>();S && S->Enabled() && (!S->SafeToSave() || S->Busy()))return Reject(TEXT("UNSAFE"),TEXT("兄弟当前无法安全等待"));
        if(const auto* Action=It->FindComponentByClass<UHearthwardTimedActionComponent>();Action && Action->GetStatus()==EHearthwardTimedActionStatus::Running)return Reject(TEXT("BUSY"),TEXT("先完成或取消当前动作"));
        if(const auto* Brother=Cast<AHearthwardCompanionFixture>(*It))
        {BrotherPresent=true;if(Brother->EquipmentBusy())return Reject(TEXT("BUSY"),TEXT("弟弟正在执行独立任务"));}
    }
    if(!BrotherPresent || !Player->FindComponentByClass<UHearthwardSurvivalComponent>())return Reject(TEXT("UNSAFE"),TEXT("兄弟状态尚未就绪"));
    Result.Accepted=true;Result.CommittedMinutes=AdvanceSurvival(0,R.Minutes);Result.Completed=Result.CommittedMinutes>=R.Minutes-1.e-6;
    Result.EndW=Clock.CalendarMinutes;Result.Status=Result.Completed?EHearthwardTimeAdvanceStatus::Completed:EHearthwardTimeAdvanceStatus::StoppedAtFailure;
    Result.ReasonCode=Result.Completed?TEXT("COMPLETED"):TEXT("STOPPED_AT_FAILURE");
    Result.Reason=Result.Completed
        ?FString::Printf(TEXT("已等待 %.1f 游戏分钟，日期、生产和刷新已结算"),Result.CommittedMinutes)
        :FString::Printf(TEXT("等待在生存失败时中止，实际经过 %.1f 游戏分钟"),Result.CommittedMinutes);
    Receipts.Add(R.OperationId,{R,Result});
    return Result;
}
#if !UE_BUILD_SHIPPING
double UHearthwardWorldClockSubsystem::AdvanceCalendar(double Minutes)
{
    if(Suspended() || GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->Busy() || GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Busy())return 0;
    return AdvanceSurvival(0,Minutes);
}
#endif
