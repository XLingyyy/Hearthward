#include "HearthwardCampaignSubsystem.h"
#include "HearthwardCampaignActor.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
using namespace HearthwardData;

bool UHearthwardCampaignSubsystem::M2HomeReady() const
{
    const auto& Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    return State.Victory && Camp.Hometown && Camp.Camps.ContainsByPredicate([](const auto& C){return C.Id==TEXT("hometown");});
}

FString UHearthwardCampaignSubsystem::M2WorkInterruption() const
{
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        const auto Phase=It->GetPhase();
        if(!It->BlockReason.IsEmpty() && (Phase==EHearthwardCompanionPhase::WaitingAtCamp || Phase==EHearthwardCompanionPhase::HoldingSafely || Phase==EHearthwardCompanionPhase::ReturningBlocked))
            return FString::Printf(TEXT("弟弟：%s\n已取得 %d，已入库 %d。会合后打开伙伴任务卡，查看原因并恢复或取消。"),
                *It->BlockReason.Left(24),It->GetAcquired(),It->GetDelivered());
    }
    const auto& Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    const auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    for(const auto& R:Camp.Regions)
    {
        if(R.Camp!=TEXT("camp") || !R.Enabled || R.Status.IsEmpty() || R.Status==TEXT("生产中") || (R.Workers.IsEmpty() && !R.Brother && !R.Player))continue;
        FString Detail=R.Status;
        if(R.Status==TEXT("等待共享仓储补料"))
        {
            const auto Inputs=HearthwardCamp::Counts(HearthwardCamp::Recipe(R.Job),TEXT("inputs"));
            for(const auto& Input:Inputs)if(Store->GetItemCount(Input.Key)<Input.Value)
            {
                Detail+=FString::Printf(TEXT("：%s缺 %d"),*Text(Find(TEXT("items"),Input.Key.ToString()),TEXT("name")),Input.Value-Store->GetItemCount(Input.Key));
                break;
            }
        }
        return Detail+TEXT("。\n打开营地岗位页核对；补料或排除阻碍后，原岗位继续工作。");
    }
    return {};
}

bool UHearthwardCampaignSubsystem::M2EventAvailable(FName Event) const
{
    if(Event==TEXT("work"))return !M2WorkInterruption().IsEmpty();
    if(Event==TEXT("owner"))return (State.Facts.Contains(TEXT("m1:river_delivered")) || State.Facts.Contains(TEXT("m2:well_delivered"))) && !State.Facts.Contains(TEXT("m2:owner_heard"));
    if(Event==TEXT("departure"))return Gameplay() && Gameplay()->Claimed.Contains(TEXT("main_04")) && !State.Victory && !State.Facts.Contains(TEXT("m2:departure_heard"));
    return false;
}

FString UHearthwardCampaignSubsystem::UseM2Event(FName Event)
{
    if(Event==TEXT("work")){Record(TEXT("m2:work_explained"));return M2WorkInterruption();}
    if(Event==TEXT("owner"))
    {
        Record(TEXT("m2:owner_heard"));
        return State.Facts.Contains(TEXT("m2:well_delivered"))?TEXT("族人：三道刻痕是我家的记号。旧碗先收在这里；等回到井院，我想再看看那口井。")
            :TEXT("族人：木牌上的家纹我认得。渡口送行的人总会摸一下它；这次，换我们把它带回家。");
    }
    Record(TEXT("m2:departure_heard"));
    return TEXT("弟弟：这里已经安顿好了。回去前先看清水岸和退路；要我同行就叫我，营地的事也能交给我。");
}

FString UHearthwardCampaignSubsystem::M2PatrolObservation(FName Location) const
{
    const FName Zone(*Text(HearthwardCampaign::Find(TEXT("locations"),Location),TEXT("zone")));
    int32 Guards=0,Archers=0,Heavies=0;
    const FVector Eye=Player()->GetPawnViewLocation();
    const FVector Forward=Player()->GetControlRotation().Vector();
    for(const auto& E:State.Enemies)
    {
        const auto* A=Actor(E.Id);
        if(E.Zone!=Zone || E.Combat.Health<=0 || !A || A->IsHidden() || FVector::Dist2D(A->GetActorLocation(),Player()->GetActorLocation())>6500)continue;
        if(FVector::DotProduct(Forward,(A->GetActorLocation()-Eye).GetSafeNormal())<.15)continue;
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(M2LocalObservation),false,Player());
        if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,A->GetActorLocation()+FVector(0,0,45),ECC_Visibility,Query) && Hit.GetActor()!=A)continue;
        if(E.Kind==TEXT("archer"))++Archers;else if(E.Kind==TEXT("heavy"))++Heavies;else ++Guards;
    }
    if(Guards+Archers+Heavies==0)return TEXT("从这里暂时看不到附近的守军。帐外旧足迹不代表当前位置；墙后和远处仍需亲自观察。");
    return FString::Printf(TEXT("眼前可见：守卫 %d、弓手 %d、重兵 %d。\n只代表此刻近处无遮挡的观察；取物后可沿帐外边缘退回。"),Guards,Archers,Heavies);
}
