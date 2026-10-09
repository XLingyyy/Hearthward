#include "HearthwardCampaignSubsystem.h"
#include "HearthwardCampaignActor.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardWorldPresentation.h"
#include "../UI/HearthwardLoadingSubsystem.h"
#include "Engine/GameInstance.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Building/HearthwardTask028CampHouse.h"
#include "../Building/HearthwardHometownFortress.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardCombatRegion.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Experience/HearthwardTraversalComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
using namespace HearthwardData;
bool UHearthwardCampaignSubsystem::DoesSupportWorldType(EWorldType::Type T) const {return T==EWorldType::Game || T==EWorldType::PIE;}
bool UHearthwardCampaignSubsystem::IsTickable() const {return Active() && Player() && !GetWorld()->IsPaused();}
TStatId UHearthwardCampaignSubsystem::GetStatId() const {RETURN_QUICK_DECLARE_CYCLE_STAT(UHearthwardCampaignSubsystem,STATGROUP_Tickables);}
bool UHearthwardCampaignSubsystem::HasLocation(FName Id) const {return bool(HearthwardCampaign::Find(TEXT("locations"),Id));}
FVector UHearthwardCampaignSubsystem::Position(FName Id) const
{
    if(const auto* P=State.Positions.Find(Id))return *P;
    const auto R=HearthwardCampaign::Find(TEXT("locations"),Id);
    return R?HearthwardCampaign::XY(R,TEXT("xy"))+FVector(0,0,20000):FVector::ZeroVector;
}
bool UHearthwardCampaignSubsystem::Ground(FVector Desired,FVector& Out) const
{
    FCollisionQueryParams Q(SCENE_QUERY_STAT(CampaignGround),false);TArray<FHitResult> Hits;
    GetWorld()->LineTraceMultiByObjectType(Hits,FVector(Desired.X,Desired.Y,80000),FVector(Desired.X,Desired.Y,-30000),FCollisionObjectQueryParams(ECC_WorldStatic),Q);
    for(const auto& H:Hits)if(H.GetActor() && (H.GetActor()->GetClass()->GetName().Contains(TEXT("Landscape")) || H.GetActor()->ActorHasTag(TEXT("Hearthward.NatureGround"))) && H.ImpactNormal.Z>.65)
    {Out=H.ImpactPoint;return true;}
    return false;
}
AHearthwardCampaignActor* UHearthwardCampaignSubsystem::Actor(FName Id) const {const auto* A=Actors.Find(Id);return A?A->Get():nullptr;}
FHearthwardCampaignEnemy* UHearthwardCampaignSubsystem::Enemy(FName Id) {return State.Enemies.FindByPredicate([&](const auto& E){return E.Id==Id;});}
void UHearthwardCampaignSubsystem::Cancel(){PendingFlag=NAME_None;FlagRemaining=0;FlagCompleteAt=0;}
void UHearthwardCampaignSubsystem::ResetActors()
{
    if(IntroRemaining>0)if(auto* Character=Cast<ACharacter>(Player()))Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    for(const auto& A:Actors)if(A.Value.IsValid())A.Value->Destroy();Actors.Reset();
    for(const auto& A:Scenery)if(A.IsValid())A->Destroy();Scenery.Reset();
    FinishTravel();IntroRemaining=0;Cancel();
}
void UHearthwardCampaignSubsystem::Start()
{
    ResetActors();State.Initialize();Gameplay()->TrackedQuest=TEXT("main_01");
    Feedback=TEXT("夜袭已经冲进前街。拿上家中的遗物，叫弟弟跟紧，从后巷撤离。");
    BeginTravel(TEXT("prologue_relic"));
}
bool UHearthwardCampaignSubsystem::Restore(const FString& Json,bool Continued)
{
    FHearthwardCampaignState Saved;if(!FHearthwardCampaignState::Parse(Json,Saved))return false;
    ResetActors();State=MoveTemp(Saved);
    if(Active() && !HearthwardCampaign::Find(TEXT("quests"),Gameplay()->TrackedQuest))Gameplay()->TrackedQuest=State.Legacy?TEXT("main_02"):TEXT("main_01");
    if(State.Phase==TEXT("prologue") && !State.Facts.Contains(TEXT("prologue_order")))Gameplay()->CompanionRoutineEnabled=false;
    if(Continued && State.Victory && State.Facts.Contains(TEXT("home_saved")))Record(TEXT("home_continued"));
    if(State.Phase==TEXT("prologue") && !State.Facts.Contains(TEXT("prologue_placed")))BeginTravel(TEXT("prologue_relic"));
    RefreshIn=0;return true;
}
void UHearthwardCampaignSubsystem::Sync()
{
    for(auto& E:State.Enemies)if(auto* A=Actor(E.Id))E.Combat=A->Target->Snapshot();
    for(auto& P:State.People)if(auto* A=Actor(P.Id)){P.Position=A->GetActorLocation();P.Located=true;}
}
FString UHearthwardCampaignSubsystem::Snapshot(){Sync();return State.Snapshot();}
void UHearthwardCampaignSubsystem::Damage(FName Id,float Health)
{
    if(auto* E=Enemy(Id))
    {
        E->Combat.Health=Health;
        if(Health<=0 && E->Group==TEXT("field") && E->RefreshDue<0)E->RefreshDue=FHearthwardClockState::FieldRefreshDue(GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes);
    }
}
bool UHearthwardCampaignSubsystem::Travel(FName Id)
{
    const auto Reject=[&](const FString& Reason){Feedback=Reason;return false;};
    auto* G=Gameplay();
    const auto Destination=HearthwardCampaign::Find(TEXT("locations"),Id);
    const FString Name=Text(Destination,TEXT("name"));
    if(!Active() || !G)return Reject(TEXT("剧情尚未准备完成，当前不能传送；请等待进入游戏后重试"));
    if(!G->IsTravelLocation(Id))return Reject(Name+TEXT("不是传送路标；占领旗帜、任务物品和普通地点不能用于传送"));
    auto* Clock=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>();
    auto* Survival=Player()?Player()->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    if(UHearthwardSurvivalComponent::HasFailed(GetWorld()))return Reject(TEXT("兄弟已无法继续，传送不能恢复生存状态；请读取保存节点"));
    if(!Survival || !Survival->Alive())return Reject(TEXT("玩家已倒地，当前不能传送；请先接受救助，或读取保存节点"));
    if(State.Phase==TEXT("prologue"))return Reject(FString(TEXT("夜袭序章尚未结束，传送未开放。"))+(!State.Facts.Contains(TEXT("relic"))
        ?TEXT("先在卧室遗物包旁按 E 取回护符，再叫弟弟跟随，从后巷撤离。")
        :!State.Facts.Contains(TEXT("prologue_order"))?TEXT("先按 X 叫弟弟跟随，等他会合后到后巷撤离口按 E；进入营地后可用路标传送。")
        :TEXT("带弟弟到后巷撤离口，等他会合后按 E；进入营地后可用路标传送。")));
    if(Id==TEXT("hometown") && !State.Victory)return Reject(TEXT("故乡路标尚未开放。需清除故乡驻军、占领河门／工坊／住区／议场四面旗帜，并解决增援，永久夺回故乡后才能传送。"));
    if(IsTraveling())return Reject(TEXT("已有前往")+Text(HearthwardCampaign::Find(TEXT("locations"),TravelDestination),TEXT("name"))+TEXT("的传送正在进行，请等待抵达后再选择地点"));
    if(Busy())return Reject(TEXT("正在占领旗帜或进行剧情动作，暂时不能传送；请等待动作完成，或先移动取消占旗"));
    if(Clock->Suspended())return Reject(GetWorld()->IsPaused()?TEXT("游戏被其他界面暂停，暂时不能传送；请返回游戏后再打开地图")
        :Clock->Busy()?TEXT("世界正在结算休息或恢复进度，暂时不能传送；请等待结算结束")
        :TEXT("世界或存档仍在加载，暂时不能传送；请等待加载画面结束后重试"));
    if(GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->Busy())return Reject(TEXT("正在采集或垂钓，暂时不能传送；请先完成或取消当前动作"));
    if(auto* Building=Player()->FindComponentByClass<UHearthwardBuildingComponent>();Building && (Building->IsBuilding() || Building->IsPlacing()))
        return Reject(Building->IsPlacing()?TEXT("仍在建造预览中，暂时不能传送；请确认或退出预览后重试"):TEXT("正在施工，暂时不能传送；请先完成或取消建造动作"));
    if(!G->Activated.Contains(Id))return Reject(Name+TEXT("尚未激活；请先步行到该地点，在路标旁按 E 激活后再传送"));
    const FName From=G->NearbyTravelStation();
    if(From.IsNone())
    {
        FName Nearest;double Distance=TNumericLimits<double>::Max();
        for(const FName Station:G->Activated)if(G->IsTravelLocation(Station))
        {
            const double D=FVector::Dist2D(Player()->GetActorLocation(),G->LocationPosition(Station));
            if(D<Distance){Distance=D;Nearest=Station;}
        }
        const double Radius=Number(Catalog()->GetObjectField(TEXT("tuning")),TEXT("interactRadius"))/100;
        return Reject(Nearest.IsNone()?TEXT("当前附近没有已激活的传送路标；请先找到营地或渡口路标，靠近后按 E 激活")
            :FString::Printf(TEXT("需站在已激活的路标旁才能传送。最近是%s，距离约%.0f米；请进入路标%.1f米范围后重试。"),*Text(Find(TEXT("locations"),Nearest.ToString()),TEXT("name")),FMath::CeilToDouble(Distance/100),Radius));
    }
    if(!G->Activated.Contains(From))return Reject(Text(Find(TEXT("locations"),From.ToString()),TEXT("name"))+TEXT("的出发路标尚未激活；请靠近按 E 激活后再传送"));
    if(From==Id)return Reject(TEXT("你已在")+Name+TEXT("旁，无需传送；请选择另一个已激活的路标"));
    Sync();TravelParticipants.Reset();TravelEnemies.Reset();TravelParticipants.Add(Player());
    bool BrotherDown=false;
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        if(const auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>())BrotherDown|=S->State.Life==EHearthwardLife::Downed;
    // Perception stops observing a downed brother, but retains the confirmed detection.
    const auto EngagedBrother=[&](const auto& E){return E.Combat.Health>0 && (E.Combat.Seen.Contains(TEXT("brother")) || (BrotherDown && E.Combat.Detection.FindRef(TEXT("brother"))>=1));};
    const bool BrotherCombat=State.Enemies.ContainsByPredicate(EngagedBrother);
    if(BrotherCombat)for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)TravelParticipants.Add(*It);
    for(const auto& E:State.Enemies)if(E.Combat.Health>0 && (E.Combat.Seen.Contains(TEXT("player")) || (BrotherCombat && EngagedBrother(E))))TravelEnemies.Add(E.Id,E.Combat.Generation);
    TravelEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();ScriptedTravel=false;
    return BeginTravel(Id);
}
bool UHearthwardCampaignSubsystem::BeginTravel(FName Id)
{
    if(UHearthwardSurvivalComponent::HasFailed(GetWorld()) || !HasLocation(Id) || !TravelDestination.IsNone())return false;
    if(TravelParticipants.IsEmpty())
    {
        ScriptedTravel=true;TravelParticipants.Add(Player());
        for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)TravelParticipants.Add(*It);
        TravelEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    }
    for(const auto& Participant:TravelParticipants)
        if(auto* A=Participant.Get())
            if(auto* Traversal=A->FindComponentByClass<UHearthwardTraversalComponent>())Traversal->CancelVault();
    TravelDestination=Id;
    // An independent brother keeps his terrain loaded when the player's source moves away.
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        if(!TravelParticipants.ContainsByPredicate([&](const auto& A){return A.Get()==*It;}))
        {
            auto* Stay=It->FindComponentByClass<UWorldPartitionStreamingSourceComponent>();
            if(!Stay){Stay=NewObject<UWorldPartitionStreamingSourceComponent>(*It);It->AddInstanceComponent(Stay);Stay->RegisterComponent();}
            Stay->EnableStreamingSource();
        }
    auto* Source=GetWorld()->SpawnActor<AActor>();StreamSource=Source;
    if(!Source){FinishTravel();Feedback=TEXT("未能准备目的地区域，传送已取消；位置保持不变，请稍后重试");return false;}
    auto* Root=NewObject<USceneComponent>(Source);Source->AddInstanceComponent(Root);Source->SetRootComponent(Root);Root->RegisterComponent();Source->SetActorLocation(Position(Id));
    auto* Component=NewObject<UWorldPartitionStreamingSourceComponent>(Source);Source->AddInstanceComponent(Component);Component->RegisterComponent();Component->EnableStreamingSource();
    if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))Nav->RegisterNavigationInvoker(Source,4000,5000);
    auto* Loading=GetWorld()->GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>();
    if(!Loading->IsLoading()){Loading->BeginLoading();Loading->FinishSession(true);}
    TravelStartedAt=FPlatformTime::Seconds();
    Feedback=TEXT("正在前往")+Text(HearthwardCampaign::Find(TEXT("locations"),Id),TEXT("name"))+TEXT("，加载目的地区域后会自动抵达");return true;
}
void UHearthwardCampaignSubsystem::FinishTravel()
{
    if(StreamSource.IsValid())
    {
        if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))Nav->UnregisterNavigationInvoker(StreamSource.Get());
        StreamSource->Destroy();
    }
    StreamSource.Reset();TravelDestination=NAME_None;
    TravelParticipants.Reset();TravelEnemies.Reset();TravelEpoch.Invalidate();ScriptedTravel=false;TravelStartedAt=0;
}
bool UHearthwardCampaignSubsystem::ZoneOccupied(FName Zone) const
{
    const auto R=HearthwardCampaign::Find(TEXT("zones"),Zone);if(!R)return true;
    const auto& B=R->GetArrayField(TEXT("bounds"));
    for(const auto& E:State.Enemies)if(E.Combat.Health>0 && E.Group!=TEXT("prologue"))
    {
        const FVector P=Actor(E.Id)?Actor(E.Id)->GetActorLocation():E.Combat.Position;
        if(P.X>=B[0]->AsNumber()*100 && P.X<=B[2]->AsNumber()*100 && P.Y>=B[1]->AsNumber()*100 && P.Y<=B[3]->AsNumber()*100)return true;
    }
    return false;
}
void UHearthwardCampaignSubsystem::RefreshActors()
{
    const FVector PlayerPosition=Player()->GetActorLocation();
    const auto& Labor=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    const FVector HomeOrigin=HearthwardCampaign::XY(HearthwardCampaign::Find(TEXT("locations"),TEXT("prologue_relic")),TEXT("xy"));
    if(FVector::Dist2D(PlayerPosition,HomeOrigin)<60000 && !Scenery.ContainsByPredicate([](const auto& A){return A.IsValid() && A->ActorHasTag(TEXT("CampaignHometownFortress"));}))
    {
        FVector Floor;if(Ground(HomeOrigin,Floor))
        {
            auto* Home=GetWorld()->SpawnActor<AHearthwardHometownFortress>(Floor,FRotator::ZeroRotator);
            Scenery.Add(Home);
            State.Positions.Add(TEXT("prologue_relic"),Home->RelicPosition());
            // Old prologue saves can be standing below the replacement bedroom floor.
            if(State.Phase==TEXT("prologue"))
            {
                auto Lift=[&](AActor* Person)
                {
                    if(!Person)return;
                    const FVector Local=Person->GetActorLocation()-Floor;
                    if(FMath::Abs(Local.X)<600 && FMath::Abs(Local.Y)<500 && Person->GetActorLocation().Z<Home->BedroomLanding().Z)
                        Person->SetActorLocation(FVector(Person->GetActorLocation().X,Person->GetActorLocation().Y,Home->BedroomLanding().Z),false,nullptr,ETeleportType::TeleportPhysics);
                };
                Lift(Player());
                for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)Lift(*It);
            }
        }
    }
    auto Spawn=[&](FName Id,FVector P,bool Hostile)->AHearthwardCampaignActor*
    {
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
        auto* A=GetWorld()->SpawnActor<AHearthwardCampaignActor>(P,FRotator::ZeroRotator,Params);
        if(A){Actors.Add(Id,A);A->Initialize(Id,Hostile);}return A;
    };
    for(auto& E:State.Enemies)
    {
        const bool Phase=(State.Phase==TEXT("prologue"))==(E.Group==TEXT("prologue"));
        const float D=FVector::Dist2D(PlayerPosition,E.Combat.Position);
        if(auto* A=Actor(E.Id))
        {if(!Phase || D>75000){E.Combat=A->Target->Snapshot();A->Destroy();Actors.Remove(E.Id);}continue;}
        if(!Phase || D>65000)continue;
        if(!E.Located){FVector P;if(!Ground(E.Home,P))continue;E.Home=P+FVector(0,0,80);E.Combat.Position=E.Home;E.Located=true;}
        Spawn(E.Id,E.Combat.Position,true);
    }
    if(State.Phase!=TEXT("prologue"))for(auto& P:State.People)
    {
        if(P.Stage==TEXT("arrived") && Labor.Rescued.Contains(P.Id))continue;
        if(auto* A=Actor(P.Id))
        {if(FVector::Dist2D(PlayerPosition,A->GetActorLocation())>75000){P.Position=A->GetActorLocation();P.Stage=P.Stage==TEXT("following")?FName(TEXT("waiting")):P.Stage;A->Destroy();Actors.Remove(P.Id);}continue;}
        if(FVector::Dist2D(PlayerPosition,P.Position)>65000)continue;
        if(!P.Located){FVector Floor;if(!Ground(P.Position,Floor))continue;P.Position=Floor+FVector(0,0,80);P.Located=true;}
        if(!Spawn(P.Id,P.Position,false) && P.Stage==TEXT("uncontacted"))
        {
            // Landscape projection may land inside a foliage collider. Only initial
            // uncontacted placement may move to nearby navigation; escorts keep their saved position.
            if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
            {
                FNavLocation Clear;
                if(Nav->ProjectPointToNavigation(P.Position,Clear,FVector(1200,1200,600)))
                    if(auto* A=Spawn(P.Id,Clear.Location+FVector(0,0,80),false))P.Position=A->GetActorLocation();
            }
        }
    }
    // One persistent region per control zone; enemy ownership remains in State.
    if(!Scenery.ContainsByPredicate([](const auto& A){return A.IsValid() && A->IsA<AHearthwardCombatRegion>();}))for(const auto& V:HearthwardCampaign::Rows(TEXT("zones")))
    {
        const auto R=V->AsObject();const auto& Bounds=R->GetArrayField(TEXT("bounds"));
        const FVector Center((Bounds[0]->AsNumber()+Bounds[2]->AsNumber())*50,(Bounds[1]->AsNumber()+Bounds[3]->AsNumber())*50,0);
        auto* Region=GetWorld()->SpawnActor<AHearthwardCombatRegion>(Center,FRotator::ZeroRotator);
        Region->RegionId=FName(*Text(R,TEXT("id")));Region->Lighting=-1.f;Region->Bounds->SetBoxExtent(FVector(12000,10000,80000));Scenery.Add(Region);
    }
    for(const auto& V:HearthwardCampaign::Rows(TEXT("locations")))
    {
        const auto R=V->AsObject();const FName Id(*Text(R,TEXT("id")));
        if(FVector::Dist2D(Position(Id),PlayerPosition)>60000)continue;
        FVector Floor;if(!State.Positions.Contains(Id)){if(!Ground(Position(Id),Floor))continue;State.Positions.Add(Id,Floor+FVector(0,0,80));}
        if(Id==TEXT("camp") || Id==TEXT("hometown") || Id==TEXT("slice_rescue") || Id==TEXT("route_mine"))continue;
        const FName Tag(*FString::Printf(TEXT("CampaignNode:%s"),*Id.ToString()));
        if(Scenery.ContainsByPredicate([&](const auto& A){return A.IsValid() && A->ActorHasTag(Tag);}))continue;
        if(Text(R,TEXT("kind"))==TEXT("person")){if(!Actor(Id))Spawn(Id,Position(Id),false);continue;}
        auto* A=GetWorld()->SpawnActor<AActor>();A->Tags.Add(Tag);
        auto* Mesh=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(Mesh);A->SetRootComponent(Mesh);
        const bool Flag=Id.ToString().StartsWith(TEXT("loc_"));
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Flag?TEXT("/Engine/BasicShapes/Cylinder.Cylinder"):TEXT("/Game/Hearthward/Assets/TASK-028/furniture/wood_chest/SM_wood_chest.SM_wood_chest")));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->RegisterComponent();
        A->SetActorLocation(Position(Id)+(Flag?FVector(0,0,100):FVector(0,0,-80)));A->SetActorScale3D(Flag?FVector(.08,.08,3.6):FVector(1));Scenery.Add(A);
        if(Flag)
        {
            auto* Cloth=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(Cloth);Cloth->SetupAttachment(Mesh);
            Cloth->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Cloth->SetCollisionEnabled(ECollisionEnabled::NoCollision);Cloth->RegisterComponent();
            Cloth->SetWorldScale3D(FVector(1.1,.025,.65));Cloth->SetWorldLocation(Position(Id)+FVector(52,0,225));
        }
    }
    for(int32 Worker=0;Worker<Labor.Population();++Worker)
    {
        const FName Id=Worker<20?FName(*FString::Printf(TEXT("civilian_initial_%02d"),Worker+1)):Labor.Rescued[Worker-20];
        const auto* Region=Labor.Regions.FindByPredicate([&](const auto& R){return R.Workers.Contains(Worker);});
        auto* A=Actor(Id);
        FName CampId=Region?Region->Camp:NAME_None;
        if(!Region)
        {
            if(A)CampId=Labor.CampAt(A->GetActorLocation());
            else if(const auto* Person=State.People.FindByPredicate([&](const auto& P){return P.Id==Id;}))CampId=Labor.CampAt(Person->Position);
            if(CampId.IsNone())CampId=TEXT("camp");
        }
        const auto* Site=Labor.Camps.FindByPredicate([&](const auto& C){return C.Id==CampId;});
        const bool Safe=Region?Region->Safe:!Labor.Regions.ContainsByPredicate([&](const auto& R){return R.Camp==CampId && !R.Safe;});
        if(!Safe)
        {
            if(A){A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}
            continue;
        }
        FVector Workplace=Site?Site->Position:Position(State.Phase==TEXT("prologue")?TEXT("prologue_exit"):CampId);
        FVector Seat=Workplace+FVector(((Worker+1)%5-2)*240,((Worker+1)/5)*260+800,0);
        bool HasWorkplace=false;
        FString Job,Status=TEXT("等待分配");
        if(Region)
        {
            if(Region->Facility.IsValid())
            {
                const auto* Facility=Labor.Facilities.FindByPredicate([&](const auto& F){return F.Id==Region->Facility;});
                if(auto* Builder=Player()->FindComponentByClass<UHearthwardBuildingComponent>();Facility && !Facility->Paused && Builder)
                    if(auto* Building=Builder->ResolveFacility(Region->Facility)){Workplace=Building->GetActorLocation();HasWorkplace=true;}
                Job=Text(Find(TEXT("craftingRecipes"),Region->Job.ToString()),TEXT("name"));
            }
            else
            {
                const FName Item=Region->Job==TEXT("forage")?FName(TEXT("wild_food")):Region->Job;
                const int32 Yield=Region->Job==TEXT("forage")?1:2;
                const FHearthwardCampSource* Source=nullptr;
                for(const auto& S:Labor.Sources)if(S.Camp==Region->Camp && S.Item==Item && !S.Blocked && S.Remaining>=Yield
                    && (!Source || S.Remaining<Source->Remaining))Source=&S;
                if(!Source)Source=Labor.Sources.FindByPredicate([&](const auto& S){return S.Camp==Region->Camp && S.Item==Item && !S.Blocked;});
                if(Source){Workplace=Source->Position;HasWorkplace=true;}
                Job=Region->Job==TEXT("forage")?TEXT("采食"):Region->Job==TEXT("wood")?TEXT("伐木")
                    :Region->Job==TEXT("stone")?TEXT("采石"):TEXT("采矿");
            }
            if(HasWorkplace)
            {
                const float Angle=FMath::DegreesToRadians(Region->Workers.IndexOfByKey(Worker)*72.f);
                Seat=Workplace+FVector(FMath::Cos(Angle)*180,FMath::Sin(Angle)*180,0);
            }
            Status=!Region->Enabled?TEXT("暂离 · 岗位已暂停"):!HasWorkplace?TEXT("等待合法岗位")
                :Region->Batch.Active && Region->Batch.Work<Region->Batch.Required?TEXT("工作中")
                :Region->Status.IsEmpty()?TEXT("等待开工"):Region->Status;
        }
        if(FVector::Dist2D(PlayerPosition,Seat)>40000)
        {
            if(A){A->Destroy();Actors.Remove(Id);}continue;
        }
        FVector Floor;
        if(!Ground(Seat,Floor))
        {
            if(A){A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}continue;
        }
        const FVector At=Floor+FVector(0,0,80);
        if(!A)A=Spawn(Id,At,false);
        if(!A)continue;
        A->SetActorEnableCollision(true);
        const FRotator Facing=HasWorkplace?FRotator(0,(Workplace-Seat).Rotation().Yaw,0):A->GetActorRotation();
        if(!A->GetActorLocation().Equals(At,1) && !A->TeleportTo(At,Facing))
        {A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);continue;}
        A->SetActorRotation(Facing);A->SetActorHiddenInGame(false);
        const FString Label=FString::Printf(TEXT("族人%d · "),Worker+1)+(Job.IsEmpty()?FString():Job+TEXT(" · "))+Status;
        A->PresentLabor(Region?Region->Id:NAME_None,Label,Region && Region->Enabled && HasWorkplace && Region->Batch.Active && Region->Batch.Work<Region->Batch.Required);
    }
    // Protected non-combat residents never enter the hostile or rescued registries.
    for(int32 I=21;I<=24;++I)
    {
        const bool ProtectedEnemy=I>20;
        const FName Id(*FString::Printf(TEXT("%s%02d"),ProtectedEnemy?TEXT("protected_"):TEXT("civilian_initial_"),ProtectedEnemy?I-20:I));
        FVector At=Position(ProtectedEnemy?TEXT("hometown"):State.Phase==TEXT("prologue")?TEXT("prologue_exit"):TEXT("camp"));
        At+=FVector((I%5-2)*240,(I/5)*260+800,0);
        if(FVector::Dist2D(PlayerPosition,At)>40000 || Actor(Id))continue;
        FVector Floor;if(Ground(At,Floor))Spawn(Id,Floor+FVector(0,0,80),false);
    }
    for(const auto& V:HearthwardCampaign::Rows(TEXT("zones")))
    {
        const auto Z=V->AsObject();const FVector Center=HearthwardCampaign::XY(Z,TEXT("center"));
        if(FVector::Dist2D(Center,PlayerPosition)>60000)continue;
        const FString ZoneId=Text(Z,TEXT("id"));
        // Measured level sites avoid the natural-map slopes without moving gameplay anchors or patrols.
        const TMap<FString,TArray<FVector2D>> SiteOffsets={
            {TEXT("river_gate"),{{1500,0},{0,0},{0,0},{0,0}}},
            {TEXT("workshops"),{{-1000,2500},{-1500,1000},{0,0},{0,0}}},
            {TEXT("dwellings"),{{3500,0},{0,0},{0,0},{-4500,2000}}},
            {TEXT("assembly"),{{0,0},{0,0},{2000,-5500},{-4000,-5500}}}};
        for(int32 I=0;I<4;++I)
        {
            const FName Tag(*FString::Printf(TEXT("CampaignHouse:%s:%d"),*Text(Z,TEXT("id")),I));
            if(Scenery.ContainsByPredicate([&](const auto& A){return A.IsValid() && A->ActorHasTag(Tag);}))continue;
            const auto& At=Z->GetArrayField(TEXT("houses"))[I]->AsArray();
            const FVector2D Offset=SiteOffsets[ZoneId][I];
            FVector Floor;if(!Ground(FVector(At[0]->AsNumber()*100+Offset.X,At[1]->AsNumber()*100+Offset.Y,0),Floor))continue;
            const FRotator Rotation(0,I*90,0);
            const TCHAR* MeshName=ZoneId==TEXT("river_gate")?TEXT("SM_RiverGate"):
                ZoneId==TEXT("workshops")?TEXT("SM_WorkshopShelter"):
                ZoneId==TEXT("dwellings")?TEXT("SM_DwellingPorch"):TEXT("SM_AssemblyColonnade");
            const FString MeshPath=FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-100/Zones/%s.%s"),MeshName,MeshName);
            UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*MeshPath);if(!Mesh)continue;
            const FBox Bounds=Mesh->GetBoundingBox();
            for(double X:{Bounds.Min.X,Bounds.Max.X})for(double Y:{Bounds.Min.Y,Bounds.Max.Y})
            {
                FVector Support;
                if(Ground(Floor+Rotation.RotateVector(FVector(X,Y,0)),Support))Floor.Z=FMath::Min(Floor.Z,Support.Z);
            }
            auto* House=GetWorld()->SpawnActor<AActor>(Floor,Rotation);
            auto* Root=NewObject<USceneComponent>(House);House->SetRootComponent(Root);Root->RegisterComponent();
            House->SetActorLocationAndRotation(Floor,Rotation);
            auto AddMesh=[&](UStaticMesh* Asset,const FVector& Local)
            {
                auto* Part=NewObject<UStaticMeshComponent>(House);House->AddInstanceComponent(Part);
                Part->SetupAttachment(Root);Part->SetStaticMesh(Asset);Part->SetRelativeLocation(Local);
                Part->SetCollisionProfileName(TEXT("BlockAll"));Part->RegisterComponent();return Part;
            };
            auto* Architecture=AddMesh(Mesh,FVector::ZeroVector);
            for(int32 Slot=0;Slot<Mesh->GetStaticMaterials().Num();++Slot)
            {
                const FString Name=Mesh->GetStaticMaterials()[Slot].MaterialSlotName.ToString();
                const TCHAR* MaterialPath=Name.Contains(TEXT("Stone"))?
                    TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_RoughStone.M_RoughStone"):
                    Name.Contains(TEXT("Timber"))?TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber.M_OldTimber"):nullptr;
                if(MaterialPath)Architecture->SetMaterial(Slot,LoadObject<UMaterialInterface>(nullptr,MaterialPath));
            }
            if(ZoneId==TEXT("workshops"))
            {
                const TCHAR* FacilityName=I%3==0?TEXT("Workbench"):I%3==1?TEXT("Forge"):TEXT("Warehouse");
                const FString FacilityPath=FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-098/%s/SM_%s_Practical.SM_%s_Practical"),FacilityName,FacilityName,FacilityName);
                if(auto* Facility=LoadObject<UStaticMesh>(nullptr,*FacilityPath))AddMesh(Facility,FVector(0,-195,0));
            }
            House->Tags.Add(Tag);House->Tags.Add(TEXT("Hearthward.ZoneArchitecture"));Scenery.Add(House);
        }
    }
    if(State.Positions.Contains(TEXT("route_mine")))
    {
        auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        const FName Key(TEXT("campaign_route_mine"));
        if(!Nature->State.Points.ContainsByPredicate([&](const auto& P){return P.Key==Key;}))
        {
            FHearthwardNaturePoint Point;Point.Id=FGuid::NewGuid();Point.Key=Key;Point.Kind=TEXT("resource");Point.Definition=TEXT("ore_vein");
            Point.Position=Position(TEXT("route_mine"))-FVector(0,0,80);Nature->State.Points.Add(Point);Nature->RebuildActors();
        }
        if(FVector::Dist2D(PlayerPosition,Position(TEXT("route_mine")))<2500)Record(TEXT("mine_source"));
    }
}
void UHearthwardCampaignSubsystem::Tick(float Delta)
{
    if(!Gameplay() || !Gameplay()->Enabled)return;
    if(UHearthwardSurvivalComponent::HasFailed(GetWorld()))
    {
        Cancel();if(!TravelDestination.IsNone())FinishTravel();IntroRemaining=0;
        Feedback=TEXT("兄弟已无法继续，请载入保存节点");return;
    }
    if(IntroRemaining>0)
    {
        IntroRemaining=FMath::Max(0.f,IntroRemaining-Delta);
        if(IntroRemaining==0)
        {
            Record(TEXT("prologue_intro"));
            if(auto* Character=Cast<ACharacter>(Player());Character && Character->FindComponentByClass<UHearthwardSurvivalComponent>()->Alive())Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
        return;
    }
    if(!TravelDestination.IsNone())
    {
        if(TravelEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())
        {FinishTravel();Feedback=TEXT("旅行已中止，进度或生存状态发生变化；请在当前进度的路标旁重新发起传送");Gameplay()->SetFeedback(Feedback);return;}
        const FString DestinationName=Text(HearthwardCampaign::Find(TEXT("locations"),TravelDestination),TEXT("name"));
        if(FPlatformTime::Seconds()-TravelStartedAt>45)
        {FinishTravel();Feedback=DestinationName+TEXT("的区域或通路未能在45秒内准备完成，传送已取消；当前位置和状态保留，请稍后从路标重试。");Gameplay()->SetFeedback(Feedback);return;}
        FVector Floor;auto* Source=StreamSource.IsValid()?StreamSource->FindComponentByClass<UWorldPartitionStreamingSourceComponent>():nullptr;
        if(Source && Source->IsStreamingCompleted())
        {
            TArray<FVector> Landings;bool Clear=true;FString LandingIssue;
            auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
            // Unrelated navigation tiles can keep rebuilding across the world.
            // Only the destination's actual projected landing is relevant.
            for(int32 I=0;I<TravelParticipants.Num();++I)
            {
                auto* Participant=TravelParticipants[I].Get();FVector Point;FNavLocation Projected;
                if(!Participant || !Ground(Position(TravelDestination)+FVector(0,I*180,0),Floor)) {Clear=false;LandingIssue=TEXT("落点没有可站立的地面");break;}
                Point=Floor+FVector(0,0,100);
                if(!ScriptedTravel && (!Nav || !Nav->ProjectPointToNavigation(Floor,Projected,FVector(200,200,300))))
                {
                    // Streaming can finish before the invoker's next navigation update.
                    // Retry this landing within the deadline even between build batches.
                    if(Nav)return;
                    Clear=false;LandingIssue=TEXT("路标附近没有可通行的落点");break;
                }
                if(!ScriptedTravel)Point=Projected.Location+FVector(0,0,100);
                if(!GetWorld()->FindTeleportSpot(Participant,Point,Participant->GetActorRotation())) {Clear=false;LandingIssue=TEXT("角色落点被障碍物占据");break;}
                for(const auto& Existing:Landings)if(FVector::Dist2D(Existing,Point)<100)Clear=false;
                Landings.Add(Point);
            }
            if(Clear)
            {
                // All required landing points are checked before committing any actor or enemy health.
                for(int32 I=0;I<TravelParticipants.Num();++I)
                {
                    auto* A=TravelParticipants[I].Get();
                    if(auto* Traversal=A->FindComponentByClass<UHearthwardTraversalComponent>())Traversal->CancelVault();
                    A->SetActorLocation(Landings[I],false,nullptr,ETeleportType::TeleportPhysics);
                    if(auto* Survival=A->FindComponentByClass<UHearthwardSurvivalComponent>())Survival->CancelAction();
                    if(auto* Combat=A->FindComponentByClass<UHearthwardCombatComponent>())Combat->InterruptTravel();
                    if(auto* Action=A->FindComponentByClass<UHearthwardTimedActionComponent>())Action->InterruptAction();
                    if(auto* Character=Cast<ACharacter>(A))Character->GetCharacterMovement()->StopMovementImmediately();
                    if(auto* Brother=Cast<AHearthwardCompanionFixture>(A))Brother->StopNavigation();
                }
                for(const auto& Pair:TravelEnemies)if(auto* E=Enemy(Pair.Key);E && E->Combat.Generation==Pair.Value && E->Combat.Health>0)
                {
                    E->Combat.Health=HearthwardCampaign::Health(E->Kind,E->Stage);
                    if(auto* A=Actor(E->Id))A->Target->Health=E->Combat.Health;
                }
                State.Positions.Add(TravelDestination,Landings[0]);
                const bool Opening=TravelDestination==TEXT("prologue_relic");
                if(Opening)Record(TEXT("prologue_placed"));
                const bool Escaped=TravelDestination==TEXT("camp") && State.Phase==TEXT("prologue");
                if(Escaped){State.Phase=TEXT("occupied");Record(TEXT("prologue_complete"));Gameplay()->TrackedQuest=TEXT("main_01");}
                FinishTravel();Feedback=TEXT("已抵达")+DestinationName+TEXT("，按 J 查看当前目标");Gameplay()->SetFeedback(Feedback);
                if(Escaped)ResetActors();
                if(Opening && !State.Facts.Contains(TEXT("prologue_intro")))
                {
                    RefreshActors();IntroRemaining=3;
                    if(auto* Character=Cast<ACharacter>(Player()))
                    {Character->SetActorLocation(Position(TEXT("prologue_relic"))+FVector(280,-70,20));Character->GetCharacterMovement()->StopMovementImmediately();Character->GetCharacterMovement()->DisableMovement();}
                    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)It->SetActorLocation(Position(TEXT("prologue_relic"))+FVector(440,30,20));
                }
            }
            else {FinishTravel();Feedback=DestinationName+TEXT("的")+LandingIssue+TEXT("，传送已取消；当前位置和状态保留，请稍后重试。");Gameplay()->SetFeedback(Feedback);}
        }
        return;
    }
    auto* Clock=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>();
    if(Clock->Suspended())return;
    auto* G=Gameplay();
    if(!PendingFlag.IsNone())
    {
        if(!Safe() || G->Health<ActionHealth || FVector::Dist2D(ActionPosition,Player()->GetActorLocation())>40 || ZoneOccupied(PendingFlag) || ActionEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())
        {Cancel();Feedback=TEXT("占旗已中断");}
        else
        {
            const double Now=Clock->GetSnapshot().ActivePlaySeconds;
            FlagRemaining=float(FMath::Max(0.,FlagCompleteAt-Now));
            if(Now>=FlagCompleteAt){State.Flags.Add(PendingFlag);Cancel();Feedback=TEXT("此区已控制");}
        }
    }
    if((RefreshIn-=Delta)>0)return;RefreshIn=.4f;Sync();RefreshActors();
    auto* Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    State.ResolveUntriggered();
    if(auto* Combat=Player()->FindComponentByClass<UHearthwardCombatComponent>())for(const auto& Alarm:Combat->State.Alarms)
        if(Alarm.Value>GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds)State.RegisterReinforcement(Alarm.Key);
    for(auto& P:State.People)if(auto* A=Actor(P.Id);A && P.Stage==TEXT("following"))
    {
        AActor* Leader=Player();
        if(P.Escort==TEXT("brother"))
        {
            AHearthwardCompanionFixture* Brother=nullptr;
            for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
                if(It->Escorting(P.Id)){Brother=*It;break;}
            if(!Brother)
            {P.Stage=TEXT("waiting");P.Escort=TEXT("player");if(auto* AI=Cast<AAIController>(A->GetController()))AI->StopMovement();continue;}
            if(FVector::Dist2D(Brother->GetActorLocation(),Player()->GetActorLocation())>3000)
            {if(auto* AI=Cast<AAIController>(A->GetController()))AI->StopMovement();continue;}
            Leader=Brother;
        }
        const bool Danger=State.Enemies.ContainsByPredicate([&](const auto& E){return E.Combat.Health>0 && FVector::Dist2D(E.Combat.Position,A->GetActorLocation())<2200;});
        if(Danger || FVector::Dist2D(A->GetActorLocation(),Leader->GetActorLocation())>7000 || !A->WalkTo(Leader->GetActorLocation(),180))
        {P.Stage=TEXT("waiting");if(auto* AI=Cast<AAIController>(A->GetController()))AI->StopMovement();Feedback=TEXT("族人停下等待；清除危险后靠近按 E 继续带路");}
        else if(!Camp->State.CampAt(A->GetActorLocation()).IsNone() && !G->InCombat())
        {if(Camp->RecordRescue(P.Id)){P.Stage=TEXT("arrived");Feedback=TEXT("族人已安全报到，人口与救援奖励已登记");}}
    }
    if(!State.Victory && State.ReadyForVictory())
    {
        FVector Floor;if(Ground(Position(TEXT("hometown")),Floor) && Camp->ReclaimHometown(TEXT("campaign_victory"),Floor+FVector(0,0,100)))
        {State.Victory=true;State.Phase=TEXT("reclaimed");G->Discovered.Add(TEXT("hometown"));G->Activated.Add(TEXT("hometown"));Feedback=TEXT("故乡已夺回。仓储、床位和篝火已开放；未完成的救援与旧物仍然保留。");}
    }
}

double UHearthwardCampaignSubsystem::NextBoundary(double Calendar) const
{
    double Step=1440;
    for(const auto& E:State.Enemies)if(E.Group==TEXT("field") && E.RefreshDue>Calendar+1.e-8)Step=FMath::Min(Step,E.RefreshDue-Calendar);
    return Step;
}
void UHearthwardCampaignSubsystem::AdvanceBoundary(double Calendar)
{
    for(auto& E:State.Enemies)if(E.Group==TEXT("field") && E.RefreshDue>=0 && Calendar+1.e-8>=E.RefreshDue)
    {
        const bool NearPerson=State.People.ContainsByPredicate([&](const auto& P){return P.Stage!=TEXT("arrived") && FVector::Dist2D(P.Position,E.Home)<5000;});
        if(!NearPerson && !Actor(E.Id)){++E.Combat.Generation;E.Combat.Health=HearthwardCampaign::Health(E.Kind,E.Stage);E.Combat.bStunned=false;E.Combat.Position=E.Home;E.RefreshDue=-1;}
    }
}
