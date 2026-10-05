#include "HearthwardCampSubsystem.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"

using namespace HearthwardData;
namespace
{
APawn* CampPlayer(UWorld* W) {return UGameplayStatics::GetPlayerPawn(W,0);}
AHearthwardCompanionFixture* CampBrother(UWorld* W)
{for(TActorIterator<AHearthwardCompanionFixture> It(W);It;++It)return *It;return nullptr;}
bool AtWorkplace(FVector Position,FVector Workplace)
{return FVector::DistSquared2D(Position,Workplace)<=FMath::Square(240.) && FMath::Abs(Position.Z-Workplace.Z)<=240;}
}
bool UHearthwardCampSubsystem::DoesSupportWorldType(EWorldType::Type Type) const {return Type==EWorldType::Game || Type==EWorldType::PIE;}
void UHearthwardCampSubsystem::EnsureCamp(FVector Position)
{
    if(!State.Camps.IsEmpty()) return;
    State.AddCamp(TEXT("camp"),Position);State.Calendar=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes;
    SyncTier();
}
void UHearthwardCampSubsystem::SyncTier()
{if(auto* P=CampPlayer(GetWorld()))if(auto* G=P->FindComponentByClass<UHearthwardGameplayComponent>())G->CampTier=State.Tier;}
bool UHearthwardCampSubsystem::CanManage(FGuid Epoch) const
{
    if(GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->Busy() || GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->Busy() || GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Busy())return false;
    const auto* P=CampPlayer(GetWorld());const auto* G=P?P->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    const auto* S=P?P->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    const auto* B=P?P->FindComponentByClass<UHearthwardBuildingComponent>():nullptr;
    return !Settling && Epoch==GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()
        && G && G->Enabled && G->Health>0 && !G->InCombat() && !State.CampAt(P->GetActorLocation()).IsNone()
        && (!S || (S->Alive() && !S->Busy())) && (!B || !B->IsBuilding());
}
bool UHearthwardCampSubsystem::UpgradeCamp(FGuid Epoch)
{
    if(!CanManage(Epoch)) {Feedback=TEXT("请在安全营地重新打开管理面板");return false;}
    Feedback=State.UpgradeReason();if(!Feedback.IsEmpty())return false;
    TGuardValue<bool> Guard(Settling,true);
    if(!GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Adjust(HearthwardCamp::Counts(HearthwardCamp::Tier(State.Tier+1),TEXT("cost")),{}))
    {Feedback=TEXT("共享仓储升阶材料不足（已预留材料不可使用）");return false;}
    State.Tier++;SyncTier();Feedback=TEXT("营地已升阶，设施仍需单独升级");return true;
}
bool UHearthwardCampSubsystem::AssignWorker(FName Region,int32 Person,FGuid Epoch)
{
    if(const auto* R=State.Regions.FindByPredicate([&](const auto& Entry){return Entry.Id==Region;});R && R->Enabled && R->BatchStopAt>0)
    {Feedback=TEXT("先暂停当前限定批次任务");return false;}
    if(!CanManage(Epoch) || !State.Assign(Region,Person)) {Feedback=TEXT("岗位已满或人员不可用");return false;}
    auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->Record(TEXT("worker_assigned"));
    if(const auto* R=State.Regions.FindByPredicate([&](const auto& V){return V.Id==Region;});R && R->Camp==TEXT("hometown"))Campaign->Record(TEXT("home_work"));
    Feedback=TEXT("人员分配已更新；兄弟到达岗位且实际工作时计产出");return true;
}
bool UHearthwardCampSubsystem::SetProduction(FName Region,bool Enabled,bool ToRations,FGuid Epoch)
{
    auto* R=State.Regions.FindByPredicate([&](const auto& Entry){return Entry.Id==Region;});
    if(!CanManage(Epoch) || !R)return false;
    if(Enabled && R->BatchStopAt>0)
    {
        if(const auto* Brother=CampBrother(GetWorld());Brother && Brother->PerformingCampBatch(R->Id))return false;
        R->BatchStopAt=0;
    }
    R->Enabled=Enabled;R->ToRations=ToRations;R->Status.Reset();Feedback=Enabled?TEXT("队列已开启，按真实劳动力和投入生产"):TEXT("队列已暂停，保留已投入进度");return true;
}
bool UHearthwardCampSubsystem::SelectProduction(FName Region,FGuid Facility,FName Recipe,FGuid Epoch)
{
    auto* R=State.Regions.FindByPredicate([&](const auto& Entry){return Entry.Id==Region;});
    auto* B=State.Facilities.FindByPredicate([&](const auto& Entry){return Entry.Id==Facility;});
    auto Def=HearthwardCamp::Recipe(Recipe);
    if(!CanManage(Epoch) || !R || !R->Facility.IsValid() || R->Batch.Active || (R->Enabled && R->BatchStopAt>0) || !B || B->Camp!=R->Camp || !Def || Text(Def,TEXT("facility"))!=B->Kind.ToString() || Number(Def,TEXT("level"))>B->Level)
    {Feedback=TEXT("需先完成／取消当前批次，并选择本营地可用设施与配方");return false;}
    if(!CampPlayer(GetWorld())->FindComponentByClass<UHearthwardGameplayComponent>()->KnowsRecipe(Recipe)){Feedback=TEXT("尚未学会图纸配方");return false;}
    R->Facility=Facility;R->Job=Recipe;Feedback=TEXT("生产配方已设置");return true;
}
bool UHearthwardCampSubsystem::Prioritize(FName Region,FGuid Epoch)
{
    auto* R=State.Regions.FindByPredicate([&](const auto& Entry){return Entry.Id==Region;});
    if(!CanManage(Epoch) || !R)return false;
    for(auto& Other:State.Regions)Other.Priority++;R->Priority=0;Feedback=TEXT("已置于共享材料生产优先序首位");return true;
}
bool UHearthwardCampSubsystem::CancelBatch(FName Region,bool ConfirmLoss,FGuid Epoch)
{
    auto* R=State.Regions.FindByPredicate([&](const auto& Entry){return Entry.Id==Region;});
    if(!CanManage(Epoch) || !R || (R->Batch.Active && !ConfirmLoss))return false;
    R->Batch={};R->Enabled=false;Feedback=TEXT("已取消；已投入材料不返还，未开始批次未扣料");return true;
}
bool UHearthwardCampSubsystem::DonateFood(FName Item,int32 Count,FGuid Epoch)
{
    if(!CanManage(Epoch) || Count<=0 || HearthwardCamp::FoodPoints(Item)<=0)return false;
    auto Next=State;if(!Next.Donate(Item,Count))return false;
    TGuardValue<bool> Guard(Settling,true);
    if(!GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Adjust({{Item,Count}},{})) {Feedback=TEXT("共享仓储食物不足");return false;}
    State=MoveTemp(Next);Feedback=TEXT("指定食物已存入公共口粮");return true;
}
bool UHearthwardCampSubsystem::EatMeal(bool Brother,FGuid Epoch)
{
    AActor* A=Brother?static_cast<AActor*>(CampBrother(GetWorld())):CampPlayer(GetWorld());
    auto* S=A?A->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    if(!CanManage(Epoch) || !S || !S->Alive() || S->Busy() || State.CampAt(A->GetActorLocation()).IsNone()
        || FVector::Dist2D(A->GetActorLocation(),CampPlayer(GetWorld())->GetActorLocation())>3000 || !State.Eat(S->Hunger()))
    {Feedback=TEXT("需要角色在营地、非满饱食，且口粮至少5点");return false;}
    S->State.Food(S->Hunger());GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Record(TEXT("public_meal"));Feedback=TEXT("用餐完成：口粮−5，饱食最多＋40");return true;
}
bool UHearthwardCampSubsystem::Craft(FGuid Facility,FName Recipe,int32 Batches,FGuid Epoch)
{
    auto* P=CampPlayer(GetWorld());auto* Builder=P?P->FindComponentByClass<UHearthwardBuildingComponent>():nullptr;
    if(!Builder || !CanManage(Epoch))return false;
    const bool Result=Builder->Craft(Facility,Recipe,Batches,Epoch);Feedback=Builder->Feedback;return Result;
}
bool UHearthwardCampSubsystem::Sleep(FGuid BedId,FGuid Epoch)
{
    auto* Clock=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>();
    FHearthwardTimeAdvanceRequest R;R.Facility=BedId;R.Epoch=Epoch;R.OperationId=FGuid::NewGuid();
    R.Campaign=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetCampaignId();R.StartW=Clock->GetSnapshot().ElapsedCalendarMinutes;
    const auto Result=Clock->RequestTimeAdvance(R);Feedback=Result.Reason;return Result.Accepted;
}
bool UHearthwardCampSubsystem::WaitAtCampfire(FGuid Id,int32 Minutes,FGuid Epoch)
{
    auto* Clock=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>();
    FHearthwardTimeAdvanceRequest R;R.Kind=EHearthwardTimeAdvanceKind::Campfire;R.Facility=Id;R.Minutes=Minutes;
    R.Epoch=Epoch;R.OperationId=FGuid::NewGuid();R.Campaign=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetCampaignId();R.StartW=Clock->GetSnapshot().ElapsedCalendarMinutes;
    const auto Result=Clock->RequestTimeAdvance(R);Feedback=Result.Reason;return Result.Accepted;
}
bool UHearthwardCampSubsystem::Workplace(const FHearthwardCampRegion& Region,FVector From,FVector& Position,AActor*& Facility) const
{
    Facility=nullptr;
    if(Region.Facility.IsValid())
    {
        const auto* Record=State.Facilities.FindByPredicate([&](const auto& B){return B.Id==Region.Facility;});
        if(!Record || Record->Paused || Record->Camp!=Region.Camp)return false;
        if(auto* Player=CampPlayer(GetWorld()))if(auto* Builder=Player->FindComponentByClass<UHearthwardBuildingComponent>())
            if(auto* Actor=Builder->ResolveFacility(Region.Facility)) {Facility=Actor;Position=Actor->GetActorLocation();return true;}
        return false;
    }
    if(!Region.Batch.Active)
    {
        const int32 Selected=State.SourceIndex(Region);
        if(Selected==INDEX_NONE)return false;
        Position=State.Sources[Selected].Position;return true;
    }
    const FName Item=Region.Job==TEXT("forage")?FName(TEXT("wild_food")):Region.Job;
    const FHearthwardCampSource* Closest=nullptr;double Distance=DBL_MAX;
    for(const auto& Source:State.Sources)if(Source.Camp==Region.Camp && Source.Item==Item && !Source.Blocked)
    {
        if(AtWorkplace(From,Source.Position)) {Position=Source.Position;return true;}
        const double Candidate=FVector::DistSquared(From,Source.Position);
        if(Candidate<Distance) {Closest=&Source;Distance=Candidate;}
    }
    if(!Closest)return false;
    Position=Closest->Position;return true;
}
bool UHearthwardCampSubsystem::BrotherWorkplace(FVector& Position,AActor*& Facility) const
{
    auto* Brother=CampBrother(GetWorld());if(!Brother)return false;
    const auto* Survival=Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!Survival || !Survival->Alive() || Survival->Busy() || Survival->Resting
        || Brother->Action->GetStatus()==EHearthwardTimedActionStatus::Running)return false;
    for(const auto& Region:State.Regions)if(Region.Enabled && Region.Safe && Region.Brother && Region.BatchStopAt==0)
        return Workplace(Region,Brother->GetActorLocation(),Position,Facility);
    return false;
}
double UHearthwardCampSubsystem::Efficiency(AActor* Actor,const FHearthwardCampRegion& Region) const
{
    if(!Actor || Actor->GetVelocity().SizeSquared()>25)return 0;
    const auto* S=Actor->FindComponentByClass<UHearthwardSurvivalComponent>();
    const auto* Timer=Actor->FindComponentByClass<UHearthwardTimedActionComponent>();
    if(!S || !S->Alive() || S->Busy() || S->Resting || (Timer && Timer->GetStatus()==EHearthwardTimedActionStatus::Running))return 0;
    const auto* G=CampPlayer(GetWorld())?CampPlayer(GetWorld())->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    if(!G || G->InCombat())return 0;
    if(const auto* C=Cast<AHearthwardCompanionFixture>(Actor);C)
    {
        const auto Phase=C->GetPhase();
        if(!C->PerformingCampBatch(Region.Id) && ((Phase!=EHearthwardCompanionPhase::Idle && Phase!=EHearthwardCompanionPhase::Completed
            && Phase!=EHearthwardCompanionPhase::Cancelled) || G->CompanionOrder!=TEXT("wait")))return 0;
    }
    FVector Position;AActor* Facility=nullptr;
    return Workplace(Region,Actor->GetActorLocation(),Position,Facility) && AtWorkplace(Actor->GetActorLocation(),Position)?(S->State.Severe()?2.1:3):0;
}
bool UHearthwardCampSubsystem::BrotherWorking() const
{
    auto* Brother=CampBrother(GetWorld());
    for(const auto& R:State.Regions)if(R.Enabled && R.Safe && R.Brother && Efficiency(Brother,R)>0)return true;
    return false;
}
void UHearthwardCampSubsystem::Advance(double Minutes,bool Sleeping)
{
    if(State.Camps.IsEmpty() || Settling)return;
    auto* P=CampPlayer(GetWorld());auto* Brother=CampBrother(GetWorld());
    for(auto& R:State.Regions)
    {
        R.Safe=true;
        if(R.Facility.IsValid() && R.Job!=TEXT("workshop") && P)
            if(auto* G=P->FindComponentByClass<UHearthwardGameplayComponent>();G && !G->KnowsRecipe(R.Job))R.Safe=false;
        const auto* Site=State.Camps.FindByPredicate([&](const auto& C){return C.Id==R.Camp;});
        for(TActorIterator<AActor> It(GetWorld());Site && It;++It)
            if(const auto* Target=It->FindComponentByClass<UHearthwardCombatTargetComponent>();Target && Target->Alive()
                && FVector::Dist2D(It->GetActorLocation(),Site->Position)<=State.Radius())
            {
                if(Target->NaturalTarget)
                {
                    const auto& Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->State;
                    const auto* Animal=Nature.Animals.FindByPredicate([&](const auto& A){return FName(A.Id.ToString())==Target->Id;});
                    if(!Animal || Animal->Domestic || Animal->AlertRemaining<=0)continue;
                }
                R.Safe=false;break;
            }
        R.PlayerEfficiency=Efficiency(P,R);R.BrotherEfficiency=Efficiency(Brother,R);
    }
    if(P) if(auto* Builder=P->FindComponentByClass<UHearthwardBuildingComponent>())
        for(auto& Source:State.Sources)
        {
            Source.Blocked=false;
            for(auto* Building:Builder->GetBuildings())
            {FVector Origin,Extent;Building->GetActorBounds(true,Origin,Extent);if(FMath::Abs(Source.Position.X-Origin.X)<Extent.X && FMath::Abs(Source.Position.Y-Origin.Y)<Extent.Y){Source.Blocked=true;break;}}
            for(const auto& Pen:GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->State.Pens)
                if(FMath::Abs(Source.Position.X-Pen.Position.X)<200 && FMath::Abs(Source.Position.Y-Pen.Position.Y)<200)Source.Blocked=true;
        }
    TGuardValue<bool> Guard(Settling,true);auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    State.Advance(Minutes,Sleeping,[&](const auto& In,const auto& Out){return Store->Adjust(In,Out);});
}
FHearthwardCampSource* UHearthwardCampSubsystem::Source(const FString& Id)
{return State.Sources.FindByPredicate([&](const auto& S){return S.Id==Id;});}
bool UHearthwardCampSubsystem::RegisterSource(FString Id,FName Item,int32 Capacity,int32 Remaining,FVector Position,double RefreshMinutes)
{
    if(auto* Existing=Source(Id))
    {
        if(Existing->RefreshMinutes==0 && RefreshMinutes>0){Existing->RefreshMinutes=RefreshMinutes;if(Existing->Remaining==0)Existing->Due=State.Calendar+RefreshMinutes;}
        return true;
    }
    const FName Camp=State.CampAt(Position);if(Id.IsEmpty() || Capacity<=0 || Remaining<0 || Remaining>Capacity || RefreshMinutes<0)return false;
    FHearthwardCampSource S;S.Id=Id;S.Camp=Camp;S.Item=Item;S.Capacity=Capacity;S.Remaining=Remaining;S.Position=Position;S.RefreshMinutes=RefreshMinutes;
    if(Remaining==0 && RefreshMinutes>0)S.Due=State.Calendar+RefreshMinutes;State.Sources.Add(S);return true;
}
bool UHearthwardCampSubsystem::RegisterFacility(FGuid Id,FName Kind,FVector Position,const TMap<FName,int32>& Paid)
{
    if(State.Facilities.ContainsByPredicate([&](const auto& B){return B.Id==Id;}))return false;
    FHearthwardCampFacility B;B.Id=Id;B.Kind=Kind;B.Camp=State.CampAt(Position);B.Paid=Paid;State.Facilities.Add(B);
    if(B.Camp==TEXT("hometown") && !Paid.IsEmpty())GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Record(TEXT("home_work"));
    if(HearthwardCamp::Facility(Kind,1))
    {
        FHearthwardCampRegion R;R.Id=FName(*(TEXT("facility_")+Id.ToString()));R.Camp=B.Camp;R.Facility=Id;R.Job=TEXT("workshop");
        R.Priority=Kind==TEXT("cooking")?1:Kind==TEXT("smelter")?2:Kind==TEXT("workbench")?3:4;
        State.Regions.Add(R);
    }
    return true;
}
bool UHearthwardCampSubsystem::CompleteFacilityUpgrade(FGuid Id,const TMap<FName,int32>& Paid)
{
    auto* B=State.Facilities.FindByPredicate([&](const auto& F){return F.Id==Id;});
    if(!B || HearthwardCamp::RequiredTier(B->Kind,B->Level+1)>State.Tier)return false;
    for(const auto& M:Paid)B->Paid.FindOrAdd(M.Key)+=M.Value;B->Level++;B->Paused=false;return true;
}
bool UHearthwardCampSubsystem::RemoveFacility(FGuid Id,bool ConfirmLoss)
{
    auto* B=State.Facilities.FindByPredicate([&](const auto& F){return F.Id==Id;});if(!B)return false;
    for(const auto& R:State.Regions)if(R.Facility==Id && R.Batch.Active && !ConfirmLoss)return false;
    if(!GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Adjust({},HearthwardCamp::Refund(B->Paid)))return false;
    State.Regions.RemoveAll([&](const auto& R){return R.Facility==Id;});
    State.Facilities.RemoveAll([&](const auto& F){return F.Id==Id;});return true;
}
bool UHearthwardCampSubsystem::RecordRescue(FName Person)
{
    auto* P=CampPlayer(GetWorld());auto* G=P?P->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    if(!G || !G->Enabled)return false;
    auto Next=State;if(!Next.Rescue(Person))return false;
    auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    const bool Bow=Next.Rescued.Num()>=5 && !G->RewardFacts.Contains(TEXT("reward:rescue5"));
    if(Bow && !Store->CanAdjust({},{{TEXT("bow_rare"),1}}))return false;
    const int32 XP=G->Experience;const auto Facts=G->RewardFacts;
    if(!G->GrantExperience(TEXT("first_rescue"),FName(*(TEXT("rescue:")+Person.ToString())),Store->GetTimelineEpoch()))return false;
    if(Bow && !G->GrantItemReward(TEXT("reward:rescue5"),TEXT("bow_rare"),1,Store->GetTimelineEpoch())){G->Experience=XP;G->RewardFacts=Facts;return false;}
    State=MoveTemp(Next);
    return true;
}
bool UHearthwardCampSubsystem::ReclaimHometown(FName Victory,FVector Position)
{
    if(Victory.IsNone() || State.Hometown || Position.ContainsNaN())return false;
    auto* P=CampPlayer(GetWorld());auto* B=P?P->FindComponentByClass<UHearthwardBuildingComponent>():nullptr;if(!B)return false;
    auto* G=P->FindComponentByClass<UHearthwardGameplayComponent>();auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if(!G || !G->Enabled || (!G->RewardFacts.Contains(TEXT("reward:hometown")) && !Store->CanAdjust({},{{TEXT("hearth_blade"),1}})))return false;
    const auto Previous=State;const auto Buildings=B->Snapshot();
    State.Hometown=true;State.AddCamp(TEXT("hometown"),Position);
    const bool Built=B->AddGift(TEXT("warehouse_access"),Position+FVector(300,0,0)) && B->AddGift(TEXT("bed"),Position+FVector(0,300,0))
        && B->AddGift(TEXT("bed"),Position+FVector(250,300,0)) && B->AddGift(TEXT("campfire"),Position+FVector(-300,0,0));
    if(!Built || (!G->RewardFacts.Contains(TEXT("reward:hometown")) && !G->GrantItemReward(TEXT("reward:hometown"),TEXT("hearth_blade"),1,Store->GetTimelineEpoch())))
    {B->Restore(Buildings);State=Previous;Feedback=TEXT("故乡设施落点尚未准备好，胜利结算待重试");return false;}
    RefreshQuartermasters();
    return true;
}
bool UHearthwardCampSubsystem::Restore(const FString& Json,int32 LegacyTier,FVector Camp,double Calendar)
{
    FHearthwardCampState Restored;
    if(!Json.IsEmpty()){if(!FHearthwardCampState::Parse(Json,Restored))return false;}
    else {Restored.Tier=LegacyTier;Restored.Calendar=Calendar;Restored.AddCamp(TEXT("camp"),Camp);}
    State=MoveTemp(Restored);SyncTier();return true;
}
FString UHearthwardCampSubsystem::Describe() const {return State.Snapshot();}
