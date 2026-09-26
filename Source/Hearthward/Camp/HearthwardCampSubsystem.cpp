#include "HearthwardCampSubsystem.h"
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

using namespace HearthwardData;
namespace
{
APawn* CampPlayer(UWorld* W) {return UGameplayStatics::GetPlayerPawn(W,0);}
AHearthwardCompanionFixture* CampBrother(UWorld* W)
{for(TActorIterator<AHearthwardCompanionFixture> It(W);It;++It)return *It;return nullptr;}
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
    if(!CanManage(Epoch) || !State.Assign(Region,Person)) {Feedback=TEXT("岗位已满或人员不可用");return false;}
    Feedback=TEXT("人员分配已更新；兄弟到达岗位且实际工作时计产出");return true;
}
bool UHearthwardCampSubsystem::SetProduction(FName Region,bool Enabled,bool ToRations,FGuid Epoch)
{
    auto* R=State.Regions.FindByPredicate([&](const auto& Entry){return Entry.Id==Region;});
    if(!CanManage(Epoch) || !R)return false;
    R->Enabled=Enabled;R->ToRations=ToRations;Feedback=Enabled?TEXT("队列已开启，按真实劳动力和投入生产"):TEXT("队列已暂停，保留已投入进度");return true;
}
bool UHearthwardCampSubsystem::SelectProduction(FName Region,FGuid Facility,FName Recipe,FGuid Epoch)
{
    auto* R=State.Regions.FindByPredicate([&](const auto& Entry){return Entry.Id==Region;});
    auto* B=State.Facilities.FindByPredicate([&](const auto& Entry){return Entry.Id==Facility;});
    auto Def=HearthwardCamp::Recipe(Recipe);
    if(!CanManage(Epoch) || !R || !R->Facility.IsValid() || R->Batch.Active || !B || B->Camp!=R->Camp || !Def || Text(Def,TEXT("facility"))!=B->Kind.ToString() || Number(Def,TEXT("level"))>B->Level)
    {Feedback=TEXT("需先完成／取消当前批次，并选择本营地可用设施与配方");return false;}
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
    S->State.Food(S->Hunger());Feedback=TEXT("用餐完成：口粮−5，饱食最多＋40");return true;
}
bool UHearthwardCampSubsystem::Craft(FGuid Facility,FName Recipe,int32 Batches,FGuid Epoch)
{
    const auto* B=State.Facilities.FindByPredicate([&](const auto& F){return F.Id==Facility;});const auto Def=HearthwardCamp::Recipe(Recipe);
    auto* P=CampPlayer(GetWorld());auto* Builder=P?P->FindComponentByClass<UHearthwardBuildingComponent>():nullptr;
    if(!CanManage(Epoch) || !B || B->Paused || !Def || Batches<1 || Batches>99 || !Builder || !Builder->CanUseFacility(Facility)
        || Text(Def,TEXT("facility"))!=B->Kind.ToString() || Number(Def,TEXT("level"))>B->Level) {Feedback=TEXT("请靠近对应等级设施，选择有效配方");return false;}
    auto Inputs=HearthwardCamp::Counts(Def,TEXT("inputs")),Outputs=HearthwardCamp::Counts(Def,TEXT("outputs"));
    for(auto& V:Inputs)V.Value*=Batches;for(auto& V:Outputs)V.Value*=Batches;
    TGuardValue<bool> Guard(Settling,true);
    if(!GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Adjust(Inputs,Outputs)) {Feedback=TEXT("共享仓储材料不足");return false;}
    Feedback=TEXT("加工完成，产物已入共享仓储");return true;
}
bool UHearthwardCampSubsystem::Sleep(FGuid Bed,FGuid Epoch)
{
    auto* P=CampPlayer(GetWorld());auto* Builder=P?P->FindComponentByClass<UHearthwardBuildingComponent>():nullptr;
    const auto* B=State.Facilities.FindByPredicate([&](const auto& F){return F.Id==Bed;});
    if(!CanManage(Epoch) || !Builder || !Builder->CanUseFacility(Bed) || !B || B->Kind!=TEXT("bed"))return false;
    for(TActorIterator<AActor> It(GetWorld());It;++It)
        if(const auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>();S && S->Enabled() && !S->SafeToSave())return false;
    const double Advanced=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->AdvanceCalendar(480);
    Feedback=Advanced>=480-1.e-6?TEXT("睡眠结束，日期与生产按八小时推进"):TEXT("睡眠被生存失败中止");return Advanced>0;
}
double UHearthwardCampSubsystem::Efficiency(AActor* Actor,const FHearthwardCampRegion& Region) const
{
    if(!Actor || Actor->GetVelocity().SizeSquared2D()>25)return 0;
    const auto* S=Actor->FindComponentByClass<UHearthwardSurvivalComponent>();
    const auto* Timer=Actor->FindComponentByClass<UHearthwardTimedActionComponent>();
    if(!S || !S->Alive() || S->Busy() || S->Resting || (Timer && Timer->GetStatus()==EHearthwardTimedActionStatus::Running))return 0;
    const auto* G=CampPlayer(GetWorld())?CampPlayer(GetWorld())->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    if(!G || G->InCombat())return 0;
    if(const auto* C=Cast<AHearthwardCompanionFixture>(Actor);C && (C->GetGoal().Intent!=NAME_None || G->CompanionOrder!=TEXT("wait")))return 0;
    FVector Workplace=FVector::ZeroVector;bool Found=false;
    if(Region.Facility.IsValid())
    {
        if(auto* B=CampPlayer(GetWorld())->FindComponentByClass<UHearthwardBuildingComponent>())
            if(auto* A=B->ResolveFacility(Region.Facility)) {Workplace=A->GetActorLocation();Found=true;}
    }
    else
    {
        const FName Item=Region.Job==TEXT("forage")?FName(TEXT("wild_food")):Region.Job;
        for(const auto& Source:State.Sources)if(Source.Camp==Region.Camp && Source.Item==Item && !Source.Blocked
            && FVector::Dist2D(Actor->GetActorLocation(),Source.Position)<=240) {Found=true;Workplace=Source.Position;break;}
    }
    return Found && FVector::Dist2D(Actor->GetActorLocation(),Workplace)<=240?(S->State.Severe()?2.1:3):0;
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
        const auto* Site=State.Camps.FindByPredicate([&](const auto& C){return C.Id==R.Camp;});
        for(TActorIterator<AActor> It(GetWorld());Site && It;++It)
            if(const auto* Target=It->FindComponentByClass<UHearthwardCombatTargetComponent>();Target && Target->Alive()
                && FVector::Dist2D(It->GetActorLocation(),Site->Position)<=State.Radius()) {R.Safe=false;break;}
        R.PlayerEfficiency=Efficiency(P,R);R.BrotherEfficiency=Efficiency(Brother,R);
    }
    if(P) if(auto* Builder=P->FindComponentByClass<UHearthwardBuildingComponent>())
        for(auto& Source:State.Sources)
        {
            Source.Blocked=false;
            for(auto* Building:Builder->GetBuildings())
            {FVector Origin,Extent;Building->GetActorBounds(true,Origin,Extent);if(FMath::Abs(Source.Position.X-Origin.X)<Extent.X && FMath::Abs(Source.Position.Y-Origin.Y)<Extent.Y){Source.Blocked=true;break;}}
        }
    TGuardValue<bool> Guard(Settling,true);auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    State.Advance(Minutes,Sleeping,[&](const auto& In,const auto& Out){return Store->Adjust(In,Out);});
}
FHearthwardCampSource* UHearthwardCampSubsystem::Source(const FString& Id)
{return State.Sources.FindByPredicate([&](const auto& S){return S.Id==Id;});}
bool UHearthwardCampSubsystem::RegisterSource(FString Id,FName Item,int32 Capacity,int32 Remaining,FVector Position,double RefreshMinutes)
{
    if(Source(Id))return true;
    const FName Camp=State.CampAt(Position);if(Id.IsEmpty() || Camp.IsNone() || Capacity<=0 || Remaining<0 || Remaining>Capacity || RefreshMinutes<0)return false;
    FHearthwardCampSource S;S.Id=Id;S.Camp=Camp;S.Item=Item;S.Capacity=Capacity;S.Remaining=Remaining;S.Position=Position;S.RefreshMinutes=RefreshMinutes;
    if(Remaining==0 && RefreshMinutes>0)S.Due=State.Calendar+RefreshMinutes;State.Sources.Add(S);return true;
}
bool UHearthwardCampSubsystem::RegisterFacility(FGuid Id,FName Kind,FVector Position,const TMap<FName,int32>& Paid)
{
    if(State.Facilities.ContainsByPredicate([&](const auto& B){return B.Id==Id;}))return false;
    FHearthwardCampFacility B;B.Id=Id;B.Kind=Kind;B.Camp=State.CampAt(Position);B.Paid=Paid;State.Facilities.Add(B);
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
bool UHearthwardCampSubsystem::RecordRescue(FName Person){return State.Rescue(Person);}
bool UHearthwardCampSubsystem::ReclaimHometown(FName Victory,FVector Position)
{
    if(Victory.IsNone() || State.Hometown || Position.ContainsNaN())return false;
    auto* P=CampPlayer(GetWorld());auto* B=P?P->FindComponentByClass<UHearthwardBuildingComponent>():nullptr;if(!B)return false;
    State.Hometown=true;State.AddCamp(TEXT("hometown"),Position);
    B->AddGift(TEXT("warehouse_access"),Position+FVector(300,0,0));
    B->AddGift(TEXT("bed"),Position+FVector(0,300,0));B->AddGift(TEXT("bed"),Position+FVector(250,300,0));
    B->AddGift(TEXT("campfire"),Position+FVector(-300,0,0));return true;
}
bool UHearthwardCampSubsystem::Restore(const FString& Json,int32 LegacyTier,FVector Camp,double Calendar)
{
    FHearthwardCampState Restored;
    if(!Json.IsEmpty()){if(!FHearthwardCampState::Parse(Json,Restored))return false;}
    else {Restored.Tier=LegacyTier;Restored.Calendar=Calendar;Restored.AddCamp(TEXT("camp"),Camp);}
    State=MoveTemp(Restored);SyncTier();return true;
}
FString UHearthwardCampSubsystem::Describe() const {return State.Snapshot();}
