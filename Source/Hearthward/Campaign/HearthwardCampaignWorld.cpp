#include "HearthwardCampaignSubsystem.h"
#include "HearthwardCampaignActor.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Building/HearthwardTask028CampHouse.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardCombatRegion.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
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
void UHearthwardCampaignSubsystem::Cancel(){PendingFlag=NAME_None;FlagRemaining=0;}
void UHearthwardCampaignSubsystem::ResetActors()
{
    if(IntroRemaining>0)if(auto* Character=Cast<ACharacter>(Player()))Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    for(const auto& A:Actors)if(A.Value.IsValid())A.Value->Destroy();Actors.Reset();
    for(const auto& A:Scenery)if(A.IsValid())A->Destroy();Scenery.Reset();
    if(StreamSource.IsValid())StreamSource->Destroy();StreamSource.Reset();TravelDestination=NAME_None;IntroRemaining=0;Cancel();
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
        if(Health<=0 && E->Group==TEXT("field") && E->RefreshDue<0)E->RefreshDue=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Calendar+4*1440;
    }
}
bool UHearthwardCampaignSubsystem::Travel(FName Id)
{
    if(!Active() || State.Phase==TEXT("prologue") || !Safe() || Busy())return false;
    const FName From=Gameplay()->NearbyLocation();
    if(From.IsNone() || From==Id || !Gameplay()->Activated.Contains(From) || !Gameplay()->Activated.Contains(Id))return false;
    return BeginTravel(Id);
}
bool UHearthwardCampaignSubsystem::BeginTravel(FName Id)
{
    if(!HasLocation(Id) || !TravelDestination.IsNone())return false;
    TravelDestination=Id;
    auto* Source=GetWorld()->SpawnActor<AActor>();StreamSource=Source;
    auto* Root=NewObject<USceneComponent>(Source);Source->AddInstanceComponent(Root);Source->SetRootComponent(Root);Root->RegisterComponent();Source->SetActorLocation(Position(Id));
    auto* Component=NewObject<UWorldPartitionStreamingSourceComponent>(Source);Source->AddInstanceComponent(Component);Component->RegisterComponent();Component->EnableStreamingSource();
    Feedback=TEXT("正在准备目的地，请稍候");return true;
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
    const bool Night=State.Phase==TEXT("prologue");
    if(Night!=NightApplied)
    {
        if(DayLights.IsEmpty())for(TActorIterator<AActor> It(GetWorld());It;++It)
            if(auto* Light=It->FindComponentByClass<UDirectionalLightComponent>())DayLights.Add(Light,{Light->Intensity,Light->GetComponentRotation()});
        for(const auto& Pair:DayLights)if(auto* Light=Pair.Key.Get())
        {Light->SetIntensity(Night?.15f:Pair.Value.Key);Light->SetWorldRotation(Night?FRotator(-8,30,0):Pair.Value.Value);}
        NightApplied=Night;
    }
    const FVector PlayerPosition=Player()->GetActorLocation();
    if(State.Phase==TEXT("prologue") && !Scenery.ContainsByPredicate([](const auto& A){return A.IsValid() && A->ActorHasTag(TEXT("CampaignPrologueHouse"));}))
    {
        FVector Floor;if(Ground(Position(TEXT("prologue_relic"))-FVector(150,0,0),Floor))
        {
            FVector DoorFloor;
            if(!Ground(Floor+FVector(270,-136,0),DoorFloor))return;
            // The doorway is uphill of the room center. Seat the foundation at the door
            // so terrain cannot reduce its headroom below the player capsule height.
            Floor.Z=DoorFloor.Z;
            auto* House=GetWorld()->SpawnActor<AHearthwardTask028CampHouse>(Floor,FRotator::ZeroRotator);House->Tags.Add(TEXT("CampaignPrologueHouse"));Scenery.Add(House);
            State.Positions.Add(TEXT("prologue_relic"),Floor+FVector(150,0,133));
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
        Region->RegionId=FName(*Text(R,TEXT("id")));Region->Lighting=Night?.12f:-1.f;Region->Bounds->SetBoxExtent(FVector(12000,10000,80000));Scenery.Add(Region);
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
    // Protected non-combat residents never enter the hostile or rescued registries.
    for(int32 I=1;I<=24;++I)
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
        for(int32 I=0;I<4;++I)
        {
            const FName Tag(*FString::Printf(TEXT("CampaignHouse:%s:%d"),*Text(Z,TEXT("id")),I));
            if(Scenery.ContainsByPredicate([&](const auto& A){return A.IsValid() && A->ActorHasTag(Tag);}))continue;
            const auto& At=Z->GetArrayField(TEXT("houses"))[I]->AsArray();
            FVector Floor;if(!Ground(FVector(At[0]->AsNumber()*100,At[1]->AsNumber()*100,0),Floor))continue;
            const FRotator Rotation(0,I*90,0);FVector DoorFloor;
            if(!Ground(Floor+Rotation.RotateVector(FVector(270,-136,0)),DoorFloor))continue;Floor.Z=DoorFloor.Z;
            auto* House=GetWorld()->SpawnActor<AHearthwardTask028CampHouse>(Floor,Rotation);House->Tags.Add(Tag);Scenery.Add(House);
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
        FVector Floor;auto* Source=StreamSource.IsValid()?StreamSource->FindComponentByClass<UWorldPartitionStreamingSourceComponent>():nullptr;
        if(Source && Source->IsStreamingCompleted() && Ground(Position(TravelDestination),Floor))
        {
            const FVector Landing=Floor+FVector(0,0,100);
            if(Player()->TeleportTo(Landing,Player()->GetActorRotation()))
            {
                for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)It->TeleportTo(Landing+FVector(0,180,0),It->GetActorRotation());
                State.Positions.Add(TravelDestination,Landing);
                const bool Opening=TravelDestination==TEXT("prologue_relic");
                if(Opening)Record(TEXT("prologue_placed"));
                const bool Escaped=TravelDestination==TEXT("camp") && State.Phase==TEXT("prologue");
                if(Escaped){State.Phase=TEXT("occupied");Record(TEXT("prologue_complete"));Gameplay()->TrackedQuest=TEXT("main_01");}
                StreamSource->Destroy();StreamSource.Reset();TravelDestination=NAME_None;Feedback=TEXT("已抵达，按 J 查看当前目标");
                if(Escaped)ResetActors();
                if(Opening && !State.Facts.Contains(TEXT("prologue_intro")))
                {
                    RefreshActors();IntroRemaining=3;
                    if(auto* Character=Cast<ACharacter>(Player()))
                    {Character->SetActorLocation(Position(TEXT("prologue_relic"))+FVector(0,0,15));Character->GetCharacterMovement()->StopMovementImmediately();Character->GetCharacterMovement()->DisableMovement();}
                    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)It->SetActorLocation(Position(TEXT("prologue_relic"))+FVector(0,140,0));
                }
            }
        }
        return;
    }
    if((RefreshIn-=Delta)>0)return;RefreshIn=.4f;Sync();RefreshActors();
    auto* G=Gameplay();auto* Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    State.ResolveUntriggered();
    if(auto* Combat=Player()->FindComponentByClass<UHearthwardCombatComponent>())for(const auto& Alarm:Combat->State.Alarms)
        if(Alarm.Value>GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds)State.RegisterReinforcement(Alarm.Key);
    for(auto& E:State.Enemies)if(E.Group==TEXT("field") && E.RefreshDue>=0 && Camp->State.Calendar>=E.RefreshDue)
    {
        const bool NearPerson=State.People.ContainsByPredicate([&](const auto& P){return P.Stage!=TEXT("arrived") && FVector::Dist2D(P.Position,E.Home)<5000;});
        if(!NearPerson && !Actor(E.Id)){++E.Combat.Generation;E.Combat.Health=HearthwardCampaign::Health(E.Kind,E.Stage);E.Combat.Position=E.Home;E.RefreshDue=-1;}
    }
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
    if(!PendingFlag.IsNone())
    {
        if(!Safe() || G->Health<ActionHealth || FVector::Dist2D(ActionPosition,Player()->GetActorLocation())>40 || ZoneOccupied(PendingFlag) || ActionEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())
        {Cancel();Feedback=TEXT("占旗已中断");}
        else if((FlagRemaining-=.4f)<=0){State.Flags.Add(PendingFlag);Cancel();Feedback=TEXT("此区已控制");}
    }
    if(!State.Victory && State.ReadyForVictory())
    {
        FVector Floor;if(Ground(Position(TEXT("hometown")),Floor) && Camp->ReclaimHometown(TEXT("campaign_victory"),Floor+FVector(0,0,100)))
        {State.Victory=true;State.Phase=TEXT("reclaimed");G->Discovered.Add(TEXT("hometown"));Feedback=TEXT("故乡已夺回。仓储、床位和篝火已开放；未完成的救援与旧物仍然保留。");}
    }
}
