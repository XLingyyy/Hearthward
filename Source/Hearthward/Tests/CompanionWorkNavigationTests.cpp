#include "../Camp/HearthwardCampSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Companion/HearthwardCompanionNavigationComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FScopedCompanionWorkWorld
{
    const FVector Origin=FVector(-38000,-84500,80);
    UWorld* World=nullptr;
    ACharacter* Player=nullptr;
    AHearthwardCompanionFixture* Brother=nullptr;
    UHearthwardGameplayComponent* Gameplay=nullptr;
    UHearthwardCampSubsystem* Camps=nullptr;
    UNavigationSystemV1* Nav=nullptr;
    FVector Workplace=Origin+FVector(1200,0,0);

    FScopedCompanionWorkWorld()
    {
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        auto* Floor=World->SpawnActor<AActor>();
        auto* FloorBox=NewObject<UBoxComponent>(Floor);
        Floor->AddInstanceComponent(FloorBox); Floor->SetRootComponent(FloorBox);
        FloorBox->SetBoxExtent(FVector(3000,3000,50));
        FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        FloorBox->SetCollisionObjectType(ECC_WorldStatic); FloorBox->SetCollisionResponseToAllChannels(ECR_Block);
        Floor->SetActorLocation(Origin-FVector(0,0,130));
        FloorBox->SetMobility(EComponentMobility::Static); FloorBox->RegisterComponent(); Floor->RegisterAllComponents();
        auto* Bounds=World->SpawnActor<ANavMeshBoundsVolume>();
        Bounds->GetRootComponent()->SetMobility(EComponentMobility::Movable);
        auto* BoundsBox=NewObject<UBoxComponent>(Bounds);
        Bounds->AddInstanceComponent(BoundsBox); BoundsBox->SetupAttachment(Bounds->GetRootComponent());
        BoundsBox->SetBoxExtent(FVector(3500,3500,300));
        BoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); BoundsBox->SetCanEverAffectNavigation(false);
        BoundsBox->RegisterComponent(); Bounds->SetActorLocation(Origin); Bounds->RegisterAllComponents();

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Player=World->SpawnActor<ACharacter>(Origin+FVector(0,-600,0),FRotator::ZeroRotator,Params);
        Player->GetCapsuleComponent()->SetCanEverAffectNavigation(false);
        auto* PlayerBag=NewObject<UHearthwardInventoryComponent>(Player);
        Player->AddInstanceComponent(PlayerBag); PlayerBag->RegisterComponent();
        Gameplay=NewObject<UHearthwardGameplayComponent>(Player);
        Player->AddInstanceComponent(Gameplay); Gameplay->RegisterComponent(); Gameplay->Enabled=true;
        Gameplay->CompanionOrder=TEXT("wait"); Gameplay->CompanionRoutineEnabled=false;
        auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);
        Player->AddInstanceComponent(Survival); Survival->RegisterComponent();
        auto* Combat=NewObject<UHearthwardCombatComponent>(Player);
        Player->AddInstanceComponent(Combat); Combat->RegisterComponent();
        auto* Controller=World->SpawnActor<APlayerController>();
        World->AddController(Controller); Controller->Possess(Player);

        auto* Camp=World->SpawnActor<AActor>();
        auto* CampRoot=NewObject<USceneComponent>(Camp);
        Camp->AddInstanceComponent(CampRoot); Camp->SetRootComponent(CampRoot); CampRoot->RegisterComponent(); Camp->SetActorLocation(Origin);
        auto* Stock=NewObject<UHearthwardInventoryComponent>(Camp);
        Camp->AddInstanceComponent(Stock); Stock->RegisterComponent();
        Brother=World->SpawnActor<AHearthwardCompanionFixture>(Origin+FVector(0,600,0),FRotator::ZeroRotator,Params);
        Brother->InitializeCompanion(Stock,Camp);
        if(!Brother->GetController())Brother->SpawnDefaultController();
        if(Brother->GetController())World->AddController(Brother->GetController());
        Brother->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Camps=World->GetSubsystem<UHearthwardCampSubsystem>();
        Camps->EnsureCamp(Origin);
        Camps->RegisterSource(TEXT("camp_work_source"),TEXT("wild_food"),50,50,Workplace,1440);

        Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
        if(!Nav)return;
        Nav->OnNavigationBoundsUpdated(Bounds);
        if(!Nav->IsInitialized())Nav->OnWorldInitDone(FNavigationSystemRunMode::GameMode);
        Nav->FlushPendingOperations();
        for(int32 Pass=0;Pass<6;++Pass)TickNavigation(.25f);
        Nav->Build();
    }

    ~FScopedCompanionWorkWorld() { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); }
    FHearthwardCampRegion& Region() const { return *Camps->State.Regions.FindByPredicate([](const auto& R){return R.Id==TEXT("camp_forage");}); }
    void TickNavigation(float Delta)
    {
        World->Tick(LEVELTICK_TimeOnly,Delta);
        if(auto* Data=Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))Data->EnsureBuildCompletion();
    }
    void TickMovement(float Delta)
    {
        if(auto* AI=Cast<AAIController>(Brother->GetController()))AI->GetPathFollowingComponent()->TickComponent(Delta,LEVELTICK_All,nullptr);
        Brother->GetCharacterMovement()->TickComponent(Delta,LEVELTICK_All,nullptr);
    }
    void Step(float Delta=.1f)
    {
        TickNavigation(Delta);
        Gameplay->TickComponent(Delta,LEVELTICK_All,nullptr);
        Brother->Tick(Delta);
        TickMovement(Delta);
        Camps->Advance(0,false);
    }
    bool HasDestination(FVector Expected) const
    {
        const auto* AI=Cast<AAIController>(Brother->GetController());
        const auto Path=AI?AI->GetPathFollowingComponent()->GetPath():FNavPathSharedPtr();
        return Path.IsValid() && Path->IsValid() && !Path->IsPartial() && Path->GetPathPoints().Num()>1
            && FVector::Dist2D(Path->GetPathPoints().Last().Location,Expected)<150;
    }
    bool Ready(FAutomationTestBase& Test)
    {
        if(!Test.TestNotNull(TEXT("Fixture has real dynamic navigation"),Nav)
            || !Test.TestNotNull(TEXT("Fixture brother has a real AIController"),Cast<AAIController>(Brother->GetController()))
            || !Test.TestTrue(TEXT("GameplayStatics resolves the fixture player"),UGameplayStatics::GetPlayerPawn(World,0)==Player))return false;
        const FVector Probe=Origin+FVector(0,1000,0);
        const bool Requested=Brother->Navigation->MoveToLocation(Probe,180,80);
        if(!Requested || !HasDestination(Probe))
        {
            const auto* Data=Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
            const auto* Recast=Cast<ARecastNavMesh>(Data);
            Test.AddInfo(FString::Printf(TEXT("Companion063 Nav fixture: time=%.2f init=%d lock=%d bounds=%d invokers=%d activeTiles=%d requested=%d mode=%d"),
                World->GetTimeSeconds(),Nav->IsInitialized(),Nav->IsNavigationBuildingLocked(),Nav->GetNavigationBounds().Num(),
                Nav->GetInvokerLocations().Num(),Recast?Recast->GetNumActiveTiles():0,Requested,int32(Brother->GetCharacterMovement()->MovementMode)));
            Test.AddError(TEXT("Shared Navigation must produce a complete probe path before behavior can be tested")); return false;
        }
        for(int32 Frame=0;Frame<60 && FVector::Dist2D(Brother->GetActorLocation(),Probe)>120;++Frame)
        { TickNavigation(.1f); TickMovement(.1f); }
        if(!Test.TestTrue(FString::Printf(TEXT("Production PathFollowing and CharacterMovement reach the probe: actual %s"),*Brother->GetActorLocation().ToString()),
            FVector::Dist2D(Brother->GetActorLocation(),Probe)<=120))return false;
        Brother->Navigation->Stop();
        // Restore only the initial fixture position; all work arrival below uses actual movement.
        Brother->SetActorLocation(Origin+FVector(0,600,0));
        const auto Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
        return Test.TestTrue(TEXT("Normal management assigns the brother to forage"),Camps->AssignWorker(TEXT("camp_forage"),31,Epoch))
            && Test.TestTrue(TEXT("Normal management enables the continuous queue"),Camps->SetProduction(TEXT("camp_forage"),true,false,Epoch));
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionContinuousWorkArrivalTest,"Hearthward.Companion063.ContinuousWorkRequiresArrivalAndStableLabor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompanionContinuousWorkArrivalTest::RunTest(const FString&)
{
    FScopedCompanionWorkWorld Fixture;
    if(!Fixture.Ready(*this))return false;
    const FVector Start=Fixture.Brother->GetActorLocation();
    Fixture.Camps->Advance(1,false);
    TestEqual(TEXT("Assignment alone supplies no labor"),Fixture.Region().BrotherEfficiency,0.);
    TestFalse(TEXT("Remote assignment cannot start a production batch"),Fixture.Region().Batch.Active);
    Fixture.Gameplay->TickComponent(.1f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Production gameplay sends the real shared navigation path to the source"),Fixture.HasDestination(Fixture.Workplace));
    for(int32 Frame=0;Frame<200 && !Fixture.Camps->BrotherWorking();++Frame)Fixture.Step();
    TestTrue(FString::Printf(TEXT("Brother physically reaches the real source: actual %s"),*Fixture.Brother->GetActorLocation().ToString()),
        FVector::Dist2D(Fixture.Brother->GetActorLocation(),Fixture.Workplace)<=240);
    TestTrue(TEXT("Work arrival uses physical travel"),FVector::Dist2D(Fixture.Brother->GetActorLocation(),Start)>600);
    TestTrue(TEXT("Brother stops before labor is counted"),Fixture.Brother->GetVelocity().SizeSquared2D()<=25);
    Fixture.Camps->Advance(1,false);
    TestEqual(TEXT("Actual safe stationary arrival supplies three labor"),Fixture.Region().BrotherEfficiency,3.);
    TestTrue(TEXT("Real labor advances the existing production batch"),Fixture.Region().Batch.Active && Fixture.Region().Batch.Work>0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionWorkRoutinePriorityTest,"Hearthward.Companion063.AuthorizedWorkPrecedesIdleRoutine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompanionWorkRoutinePriorityTest::RunTest(const FString&)
{
    FScopedCompanionWorkWorld Fixture;
    if(!Fixture.Ready(*this))return false;
    Fixture.Gameplay->CompanionRoutineEnabled=true;
    Fixture.Gameplay->TickComponent(.1f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Enabled assigned safe work owns the actual path before idle routine"),Fixture.HasDestination(Fixture.Workplace));
    Fixture.Camps->Advance(0,false);
    TestEqual(TEXT("A path request is still not labor"),Fixture.Region().BrotherEfficiency,0.);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionWorkInterruptionTest,"Hearthward.Companion063.ContinuousWorkYieldsToSafetyOrdersAndRescue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompanionWorkInterruptionTest::RunTest(const FString&)
{
    FScopedCompanionWorkWorld Fixture;
    if(!Fixture.Ready(*this))return false;
    const auto Epoch=Fixture.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    TestTrue(TEXT("Normal management pauses the queue"),Fixture.Camps->SetProduction(TEXT("camp_forage"),false,false,Epoch));
    Fixture.Gameplay->TickComponent(.1f,LEVELTICK_All,nullptr);
    TestFalse(TEXT("Paused production cannot own a work path"),Fixture.HasDestination(Fixture.Workplace));
    TestTrue(TEXT("Normal management resumes the queue"),Fixture.Camps->SetProduction(TEXT("camp_forage"),true,false,Epoch));
    auto* Threat=Fixture.World->SpawnActor<AActor>();
    auto* Root=NewObject<USceneComponent>(Threat);
    Threat->AddInstanceComponent(Root); Threat->SetRootComponent(Root); Root->RegisterComponent(); Threat->SetActorLocation(Fixture.Origin);
    auto* Target=NewObject<UHearthwardCombatTargetComponent>(Threat);
    Threat->AddInstanceComponent(Target); Target->RegisterComponent(); Target->Health=Target->MaximumHealth=100;
    Fixture.Camps->Advance(0,false);
    TestFalse(TEXT("Real camp threat makes the production region unsafe"),Fixture.Region().Safe);
    Fixture.Gameplay->TickComponent(.1f,LEVELTICK_All,nullptr);
    TestFalse(TEXT("Unsafe production cannot own a work path"),Fixture.HasDestination(Fixture.Workplace));
    Threat->Destroy(); Fixture.Camps->Advance(0,false);
    Fixture.Gameplay->NotifyCombat(); Fixture.Gameplay->TickComponent(.1f,LEVELTICK_All,nullptr);
    TestFalse(TEXT("Combat prevents a continuous work path"),Fixture.HasDestination(Fixture.Workplace));
    Fixture.Gameplay->CompanionOrder=TEXT("follow");
    Fixture.Gameplay->TickComponent(4.f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Follow order owns the real path to the player"),Fixture.HasDestination(Fixture.Player->GetActorLocation()));
    Fixture.Gameplay->CompanionOrder=TEXT("wait");
    Fixture.Gameplay->Health=0;
    auto* Survival=Fixture.Player->FindComponentByClass<UHearthwardSurvivalComponent>();
    Survival->State.Life=EHearthwardLife::Downed; Survival->State.DownRemaining=60;
    Fixture.Step();
    TestTrue(TEXT("Production automatic rescue owns the actual path to the downed player"),Fixture.HasDestination(Fixture.Player->GetActorLocation()));
    TestFalse(TEXT("Rescue does not count as camp work"),Fixture.Camps->BrotherWorking());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionWorkHeightTest,"Hearthward.Companion063.ContinuousWorkRejectsDifferentFloorAndVerticalMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompanionWorkHeightTest::RunTest(const FString&)
{
    FScopedCompanionWorkWorld Fixture;
    if(!Fixture.Ready(*this))return false;
    // Position and velocity are fixture stimuli; labor still comes from the production Camp advance.
    Fixture.Brother->SetActorLocation(Fixture.Workplace+FVector(0,0,500));
    Fixture.Camps->Advance(1,false);
    TestEqual(TEXT("Same XY on a different floor supplies no labor"),Fixture.Region().BrotherEfficiency,0.);
    TestFalse(TEXT("Different-floor assignment cannot start a production batch"),Fixture.Region().Batch.Active);
    Fixture.Brother->SetActorLocation(Fixture.Workplace);
    Fixture.Brother->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    Fixture.Brother->GetCharacterMovement()->Velocity=FVector(0,0,50);
    Fixture.Camps->Advance(0,false);
    TestEqual(TEXT("Vertical movement supplies no stationary labor"),Fixture.Region().BrotherEfficiency,0.);
    Fixture.Brother->GetCharacterMovement()->StopMovementImmediately();
    Fixture.Brother->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    Fixture.Camps->Advance(0,false);
    TestEqual(TEXT("Same-floor stopped worker retains normal labor"),Fixture.Region().BrotherEfficiency,3.);
    Fixture.Brother->FindComponentByClass<UHearthwardSurvivalComponent>()->State.SevereDue=4320;
    Fixture.Camps->Advance(0,false);
    TestEqual(TEXT("Severe worker retains the existing reduced labor"),Fixture.Region().BrotherEfficiency,2.1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionWorkSourceAvailabilityTest,"Hearthward.Companion063.ExhaustedSourceDoesNotBlockAvailableWorkplace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompanionWorkSourceAvailabilityTest::RunTest(const FString&)
{
    FScopedCompanionWorkWorld Fixture;
    if(!Fixture.Ready(*this))return false;
    TestTrue(TEXT("Fixture registers a nearby exhausted source"),Fixture.Camps->RegisterSource(
        TEXT("camp_work_exhausted"),TEXT("wild_food"),50,0,Fixture.Origin+FVector(0,650,0),1440));
    const FVector OtherWorkplace=Fixture.Origin+FVector(-1200,0,0);
    TestTrue(TEXT("Fixture registers another available source"),Fixture.Camps->RegisterSource(
        TEXT("camp_work_other"),TEXT("wild_food"),50,50,OtherWorkplace,1440));
    // Source stock and actor position are fixture stimuli; production selects and reserves the actual batch.
    Fixture.Camps->Source(TEXT("camp_work_source"))->Remaining=1;
    Fixture.Camps->Advance(0,false);
    TestEqual(TEXT("Standing beside an exhausted source supplies no new-batch labor"),Fixture.Region().BrotherEfficiency,0.);
    Fixture.Gameplay->TickComponent(.1f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Work navigation selects the partially harvested available source used by production"),Fixture.HasDestination(Fixture.Workplace));
    Fixture.Brother->Navigation->Stop();
    Fixture.Brother->SetActorLocation(Fixture.Workplace);
    Fixture.Camps->Advance(1,false);
    TestTrue(TEXT("Normal production reserves the final source unit"),Fixture.Region().Batch.Active);
    TestEqual(TEXT("The final batch exhausts its source"),Fixture.Camps->Source(TEXT("camp_work_source"))->Remaining,0);
    const double Work=Fixture.Region().Batch.Work;
    Fixture.Camps->Advance(1,false);
    TestEqual(TEXT("A reserved final batch retains labor at its exhausted source"),Fixture.Region().BrotherEfficiency,3.);
    TestTrue(TEXT("The reserved final batch continues making real progress"),Fixture.Region().Batch.Work>Work);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionGatheringDepletionTest,"Hearthward.Companion078.GatheringDepletionAndResume",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompanionGatheringDepletionTest::RunTest(const FString&)
{
    FScopedCompanionWorkWorld Fixture;
    if(!Fixture.Ready(*this))return false;
    auto* Brother=Fixture.Brother;
    const auto Epoch=Fixture.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    Fixture.Camps->AssignWorker(TEXT("camp_forage"),31,Epoch);
    Fixture.Camps->SetProduction(TEXT("camp_forage"),false,false,Epoch);
    auto* SourceActor=Fixture.World->SpawnActor<AActor>();
    auto* Root=NewObject<USceneComponent>(SourceActor);
    SourceActor->AddInstanceComponent(Root);SourceActor->SetRootComponent(Root);Root->RegisterComponent();
    SourceActor->SetActorLocation(Fixture.Workplace);
    auto* Source=NewObject<UHearthwardInventoryComponent>(SourceActor);
    SourceActor->AddInstanceComponent(Source);Source->RegisterComponent();
    Source->TryAdd(TEXT("wood"),14);
    Brother->InitializeCompanion(Source,Brother->Camp);
    if(!TestEqual(TEXT("Fixture supplies a real gathering tool"),Brother->Bag->TryAdd(TEXT("axe"),1),EHearthwardInventoryResult::Success))return false;
    Brother->Action->BeginPlay();
    const auto Ticket=Brother->Request(Fixture.Player,TEXT("采集32个木头"));
    if(!TestEqual(TEXT("Accept the same 32 wood task"),Brother->Submit(Fixture.Player,Ticket,TEXT("wood"),32,
        {TEXT("collect"),TEXT("return"),TEXT("deposit")}),EHearthwardProposalResult::Accepted))return false;
    auto Step=[&]()
    {
        Fixture.World->GetSubsystem<UHearthwardWorldClockSubsystem>()->Tick(.1f);
        Brother->Action->TickComponent(.1f,LEVELTICK_All,nullptr);
        Fixture.Step();
    };
    Step();
    TestTrue(TEXT("Delegated collection uses running speed"),Brother->GetCharacterMovement()->MaxWalkSpeed>=500.f);
    const float LoadedSpeed=Brother->GetCharacterMovement()->MaxWalkSpeed;
    TestTrue(TEXT("Tool weight still reduces running speed"),LoadedSpeed<600.f);
    auto* Survival=Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    Survival->State.SevereDue=1000;
    Brother->Tick(.1f);
    TestEqual(TEXT("Severe state still slows delegated movement"),Brother->GetCharacterMovement()->MaxWalkSpeed,LoadedSpeed*.8f);
    Survival->State.SevereDue=-1;
    for(int32 I=0;I<4000 && Brother->GetPhase()!=EHearthwardCompanionPhase::WaitingAtCamp;++I)Step();
    AddInfo(FString::Printf(TEXT("Gathering stopped: delivered=%d phase=%s reason=%s"),Brother->GetDelivered(),*UEnum::GetValueAsString(Brother->GetPhase()),*Brother->BlockReason));
    TestEqual(TEXT("All fourteen available wood are delivered"),Brother->GetDelivered(),14);
    TestEqual(TEXT("The exhausted source is empty"),Source->GetItemCount(TEXT("wood")),0);
    TestEqual(TEXT("No cargo is lost or left unaccounted"),Brother->GetCarried(),0);
    TestEqual(TEXT("Incomplete task waits safely at camp"),Brother->GetPhase(),EHearthwardCompanionPhase::WaitingAtCamp);
    TestTrue(TEXT("Depletion remains explained"),Brother->BlockReason.Contains(TEXT("资源")));
    TestFalse(TEXT("Retry refuses an unchanged exhausted source"),Brother->ResumeBlocked(Fixture.Player));
    TestEqual(TEXT("Failed retry preserves partial progress"),Brother->GetDelivered(),14);
    Source->TryAdd(TEXT("wood"),18);
    TestTrue(TEXT("Retry resumes once the source has real stock"),Brother->ResumeBlocked(Fixture.Player));
    for(int32 I=0;I<4000 && Brother->GetPhase()!=EHearthwardCompanionPhase::Completed;++I)Step();
    TestEqual(TEXT("Resumed task completes the remaining eighteen"),Brother->GetDelivered(),32);
    TestEqual(TEXT("Completion is terminal"),Brother->GetPhase(),EHearthwardCompanionPhase::Completed);
    TestEqual(TEXT("Exactly 32 wood reached shared storage"),Fixture.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetItemCount(TEXT("wood")),32);
    return true;
}
#endif
