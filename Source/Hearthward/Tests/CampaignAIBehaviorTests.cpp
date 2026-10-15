#include "../Campaign/HearthwardCampaignActor.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Environment/HearthwardNoiseInteraction.h"
#include "../Environment/HearthwardLampInteraction.h"
#include "../Environment/HearthwardWeatherActor.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationData.h"
#include "NavigationOctree.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FScopedCampaignAIWorld
{
    const FVector Origin=FVector(-38000,-84500,80);
    UWorld* World=nullptr;
    ACharacter* Player=nullptr;
    AHearthwardCampaignActor* Enemy=nullptr;
    UHearthwardCombatComponent* Combat=nullptr;
    UNavigationSystemV1* Nav=nullptr;
    UBoxComponent* FloorBox=nullptr;
    ANavMeshBoundsVolume* Bounds=nullptr;
    FVector PatrolGoal=FVector::ZeroVector;

    FScopedCampaignAIWorld()
    {
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

        auto* Floor=World->SpawnActor<AActor>();
        Floor->Tags.Add(TEXT("Hearthward.NatureGround"));
        FloorBox=NewObject<UBoxComponent>(Floor);
        Floor->AddInstanceComponent(FloorBox); Floor->SetRootComponent(FloorBox);
        FloorBox->SetBoxExtent(FVector(6000,6000,50));
        FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        FloorBox->SetCollisionObjectType(ECC_WorldStatic);
        FloorBox->SetCollisionResponseToAllChannels(ECR_Block);
        Floor->SetActorLocation(Origin-FVector(0,0,130));
        FloorBox->SetMobility(EComponentMobility::Static); FloorBox->RegisterComponent();
        Floor->RegisterAllComponents();

        Bounds=World->SpawnActor<ANavMeshBoundsVolume>();
        Bounds->GetRootComponent()->SetMobility(EComponentMobility::Movable);
        auto* BoundsBox=NewObject<UBoxComponent>(Bounds);
        Bounds->AddInstanceComponent(BoundsBox); BoundsBox->SetupAttachment(Bounds->GetRootComponent());
        BoundsBox->SetBoxExtent(FVector(6500,6500,300));
        BoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BoundsBox->SetCanEverAffectNavigation(false); BoundsBox->RegisterComponent();
        Bounds->SetActorLocation(Origin);
        Bounds->RegisterAllComponents();

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Player=World->SpawnActor<ACharacter>(Origin+FVector(4500,0,0),FRotator::ZeroRotator,Params);
        Player->GetCapsuleComponent()->SetCanEverAffectNavigation(false);
        auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);
        Player->AddInstanceComponent(Bag); Bag->RegisterComponent();
        auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);
        Player->AddInstanceComponent(Gameplay); Gameplay->RegisterComponent(); Gameplay->Enabled=true;
        auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);
        Player->AddInstanceComponent(Survival); Survival->RegisterComponent();
        Combat=NewObject<UHearthwardCombatComponent>(Player);
        Player->AddInstanceComponent(Combat); Combat->RegisterComponent();
        auto* PlayerController=World->SpawnActor<APlayerController>();
        World->AddController(PlayerController); PlayerController->Possess(Player);

        auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();
        Campaign->State.Initialize();
        auto* Record=Campaign->Enemy(TEXT("field_slice_01"));
        if(!Record)return;
        Record->Home=Origin; Record->Combat.Position=Origin; Record->Located=true;
        Enemy=World->SpawnActor<AHearthwardCampaignActor>(Origin,FRotator::ZeroRotator,Params);
        Enemy->Initialize(Record->Id,true); Enemy->Target->Exposure=1;
        Enemy->GetCapsuleComponent()->SetCanEverAffectNavigation(false);

        Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
        if(!Nav)return;
        // The flat fixture includes the full authored patrol; this does not change runtime invoker settings.
        Nav->RegisterNavigationInvoker(Enemy,6000,6500);
        Nav->OnNavigationBoundsUpdated(Bounds);
        if(!Nav->IsInitialized())Nav->OnWorldInitDone(FNavigationSystemRunMode::GameMode);
        Nav->FlushPendingOperations();
        // Invoker polling reads World time. TimeOnly advances it without running the enemy or perception.
        for(int32 Pass=0;Pass<6;++Pass)
        {
            World->Tick(LEVELTICK_TimeOnly,.25f);
            if(auto* Data=Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))Data->EnsureBuildCompletion();
        }
        Nav->Build();
    }

    ~FScopedCampaignAIWorld()
    {
        GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    }

    bool Destination(FVector& Goal) const
    {
        const auto* AI=Enemy?Cast<AAIController>(Enemy->GetController()):nullptr;
        const auto* Following=AI?AI->GetPathFollowingComponent():nullptr;
        const auto Path=Following?Following->GetPath():FNavPathSharedPtr();
        if(!Path.IsValid() || !Path->IsValid() || Path->IsPartial() || Path->GetPathPoints().Num()<2)return false;
        Goal=Path->GetPathPoints().Last().Location; return true;
    }

    bool Ready(FAutomationTestBase& Test)
    {
        if(!Test.TestNotNull(TEXT("Fixture has a real campaign enemy"),Enemy)
            || !Test.TestNotNull(TEXT("Fixture has a navigation system"),Nav))return false;
        if(!Test.TestTrue(TEXT("GameplayStatics resolves the fixture player pawn"),
            UGameplayStatics::GetPlayerPawn(World,0)==Player))return false;
        FVector Ground;
        if(!Test.TestTrue(TEXT("Campaign Ground accepts the fixture collision floor"),
            World->GetSubsystem<UHearthwardCampaignSubsystem>()->Ground(Origin,Ground)))return false;
        auto* Path=Nav->FindPathToLocationSynchronously(World,Origin,Origin+FVector(1000,0,0),Enemy);
        if(!Path || !Path->IsValid() || Path->IsPartial())
        {
            auto* Data=Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
            const auto* Recast=Cast<ARecastNavMesh>(Data);
            const auto* AgentData=Nav->GetNavDataForProps(Enemy->GetNavAgentPropertiesRef(),Origin);
            auto* DefaultPath=Nav->FindPathToLocationSynchronously(World,Origin,Origin+FVector(1000,0,0));
            FNavLocation Start,Goal;
            const bool StartProjected=Nav->ProjectPointToNavigation(Origin,Start,FVector(100,100,250));
            const bool GoalProjected=Nav->ProjectPointToNavigation(Origin+FVector(1000,0,0),Goal,FVector(100,100,250));
            int32 OctreeElements=0,FloorElements=0,FloorCollisionBytes=0;
            if(const auto* Octree=Nav->GetNavOctree())
                Octree->FindElementsWithBoundsTest(FBoxCenterAndExtent(FloorBox->Bounds.GetBox()),[&](const FNavigationOctreeElement& Element)
                {
                    ++OctreeElements;
                    if(Element.GetSourceElement()->GetWeakUObject().Get()==FloorBox)
                    { ++FloorElements; FloorCollisionBytes+=Element.Data->CollisionData.Num(); }
                });
            Test.AddInfo(FString::Printf(TEXT("Campaign056 Nav fixture: worldTime=%.2f init=%d locked=%d boundsCount=%d invokers=%d runtime=%d navData=%s agentData=%s activeTiles=%d octreeElements=%d floorElements=%d floorCollisionBytes=%d"),
                World->GetTimeSeconds(),Nav->IsInitialized(),Nav->IsNavigationBuildingLocked(),Nav->GetNavigationBounds().Num(),
                Nav->GetInvokerLocations().Num(),Data?static_cast<int32>(Data->GetRuntimeGenerationMode()):-1,*GetNameSafe(Data),
                *GetNameSafe(AgentData),Recast?Recast->GetNumActiveTiles():0,OctreeElements,FloorElements,FloorCollisionBytes));
            Test.AddInfo(FString::Printf(TEXT("Campaign056 geometry: boundsRegistered=%d bounds=%s brushBounds=%s floorRegistered=%d floorNavRelevant=%d floorBounds=%s startProjected=%d goalProjected=%d agentPathValid=%d agentPathPartial=%d defaultPathValid=%d defaultPathPartial=%d"),
                Bounds->HasActorRegisteredAllComponents(),*Bounds->GetComponentsBoundingBox(true).ToString(),*Bounds->GetRootComponent()->Bounds.GetBox().ToString(),
                FloorBox->IsRegistered(),FloorBox->IsNavigationRelevant(),*FloorBox->Bounds.GetBox().ToString(),StartProjected,GoalProjected,
                Path && Path->IsValid(),Path && Path->IsPartial(),DefaultPath && DefaultPath->IsValid(),DefaultPath && DefaultPath->IsPartial()));
        }
        if(!Test.TestTrue(TEXT("Fixture has a complete navigation path to the observation point"),
            Path && Path->IsValid() && !Path->IsPartial()))return false;
        Enemy->Tick(.4f);
        return Test.TestTrue(TEXT("Production Actor Tick produces a complete authored patrol path"),Destination(PatrolGoal));
    }

    bool ExpectDestination(FAutomationTestBase& Test,const TCHAR* Label,FVector Expected) const
    {
        FVector Actual;
        if(!Test.TestTrue(FString(Label)+TEXT(" has a complete real AI path"),Destination(Actual)))return false;
        return Test.TestTrue(FString::Printf(TEXT("%s: expected %s, actual %s"),Label,*Expected.ToString(),*Actual.ToString()),
            FVector::Dist2D(Actual,Expected)<150);
    }

    void Perception(float Delta) { Combat->TickComponent(Delta,LEVELTICK_All,nullptr); }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignLastKnownBehaviorTest,"Hearthward.Campaign056.LastKnownSearchUsesRecordedPosition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignLastKnownBehaviorTest::RunTest(const FString&)
{
    FScopedCampaignAIWorld Fixture;
    if(!Fixture.Ready(*this))return false;
    Fixture.Player->SetActorLocation(Fixture.Origin+FVector(1000,0,0));
    Fixture.Perception(.4f);
    auto& Memory=Fixture.Enemy->Target->Memory;
    if(!TestTrue(TEXT("Production perception records an observed player position"),Memory.LastKnown.Contains(TEXT("player"))))return false;
    const FVector Known=Memory.LastKnown[TEXT("player")];
    Fixture.Player->SetActorLocation(Fixture.Origin+FVector(4500,0,0));
    Fixture.Perception(.4f);
    TestTrue(TEXT("Lost sight retains positive observer detection"),Memory.Detection.FindRef(TEXT("player"))>0);
    TestTrue(TEXT("Lost sight clears confirmed sight"),Memory.Seen.IsEmpty());
    TestEqual(TEXT("Lost sight preserves the recorded position"),Memory.LastKnown[TEXT("player")],Known);
    Fixture.Enemy->Tick(.4f);
    Fixture.ExpectDestination(*this,TEXT("Campaign enemy searches the recorded last-known position"),Known);

    Fixture.Player->SetActorLocation(Fixture.Origin+FVector(4500,1000,0));
    Fixture.Perception(.1f); Fixture.Enemy->Tick(.4f);
    TestEqual(TEXT("Hidden movement cannot update LastKnown"),Memory.LastKnown[TEXT("player")],Known);
    Fixture.ExpectDestination(*this,TEXT("Hidden movement cannot redirect the real search path"),Known);
    Fixture.Perception(1.f); Fixture.Enemy->Tick(.4f);
    TestTrue(TEXT("Production decay ends the suspicion"),FMath::IsNearlyZero(Memory.Detection.FindRef(TEXT("player"))));
    Fixture.ExpectDestination(*this,TEXT("Detection expiry resumes the authored patrol"),Fixture.PatrolGoal);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignBaitBehaviorTest,"Hearthward.Campaign056.BaitInvestigationUsesRecordedEightSeconds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignBaitBehaviorTest::RunTest(const FString&)
{
    FScopedCampaignAIWorld Fixture;
    if(!Fixture.Ready(*this))return false;
    auto& Memory=Fixture.Enemy->Target->Memory;
    // This is the saved stimulus written by a landed bait projectile; the actor and timer remain production code.
    Memory.Investigation=Fixture.Origin+FVector(0,1000,0); Memory.InvestigationRemaining=8;
    Fixture.Enemy->Tick(.4f);
    Fixture.ExpectDestination(*this,TEXT("Campaign enemy consumes the recorded bait location"),Memory.Investigation);
    Fixture.Perception(4.f); Fixture.Enemy->Tick(.4f);
    TestEqual(TEXT("Production timer retains four seconds"),Memory.InvestigationRemaining,4.);
    Fixture.ExpectDestination(*this,TEXT("Bait remains the real movement destination before eight seconds"),Memory.Investigation);
    Fixture.Perception(4.f); Fixture.Enemy->Tick(.4f);
    TestEqual(TEXT("Production timer expires at eight seconds"),Memory.InvestigationRemaining,0.);
    Fixture.ExpectDestination(*this,TEXT("Expired bait resumes the authored patrol"),Fixture.PatrolGoal);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignProtectedResident066Test,"Hearthward.Campaign066.ProtectedResidentCannotActAsCorpse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignProtectedResident066Test::RunTest(const FString&)
{
    for(const bool Hostile:{false,true})
    {
        FScopedCampaignAIWorld Fixture;
        if(!Fixture.Ready(*this))return false;
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Resident=Fixture.World->SpawnActor<AHearthwardCampaignActor>(Fixture.Origin+FVector(600,0,0),FRotator::ZeroRotator,Params);
        if(!TestNotNull(TEXT("The corpse-stimulus fixture has a real campaign actor"),Resident))return false;
        Resident->Initialize(Hostile?FName(TEXT("field_slice_02")):FName(TEXT("protected_01")),Hostile);
        Resident->SetActorLocation(Fixture.Origin+FVector(600,0,0));
        Fixture.Enemy->SetActorRotation(FRotator::ZeroRotator);
        const FName Region=Fixture.Enemy->Target->Region;
        if(Hostile)
        {
            Fixture.Combat->HitTarget(Resident->Target,Resident->Target->Health,TEXT("body"),false,FGuid::NewGuid());
            if(!TestEqual(TEXT("The positive control dies through the production damage transaction"),Resident->Target->Health,0.f))return false;
        }
        else
        {
            TestTrue(TEXT("Actual non-hostile initialization protects the resident"),Resident->Target->Protected);
            TestEqual(TEXT("Actual protected initialization does not create combat health"),Resident->Target->Health,0.f);
        }
        auto* Clock=Fixture.World->GetSubsystem<UHearthwardWorldClockSubsystem>();
        const double Started=Clock->GetSnapshot().ActivePlaySeconds;
        auto Advance=[&](float Delta){Clock->Tick(Delta);Fixture.Perception(Delta);};
        Advance(.01f);Advance(2.99f);
        TestTrue(TEXT("Neither stimulus may publish an alarm before the real three-second report"),Fixture.Combat->State.Alarms.IsEmpty());
        Advance(.011f);
        TestTrue(TEXT("The production active clock advances for the corpse-report boundary"),FMath::IsNearlyEqual(Clock->GetSnapshot().ActivePlaySeconds,Started+3.011,1.e-4));
        TestTrue(TEXT("The reporting observer never sees the distant fixture player"),Fixture.Enemy->Target->Memory.Seen.IsEmpty());
        if(Hostile)
        {
            TestTrue(TEXT("A genuine defeated enemy remains a reportable corpse"),Resident->Target->Memory.BroadcastRegions.Contains(Region));
            TestTrue(TEXT("The completed genuine corpse report publishes the actual region alarm"),Fixture.Combat->State.Alarms.Contains(Region));
            TestTrue(TEXT("The genuine alarm expires 120 active seconds after the report"),FMath::IsNearlyEqual(Fixture.Combat->State.Alarms.FindRef(Region),Clock->GetSnapshot().ActivePlaySeconds+120,1.e-4));
        }
        else
        {
            TestTrue(TEXT("A living protected resident is never recorded as a reported corpse"),Resident->Target->Memory.BroadcastRegions.IsEmpty());
            TestTrue(TEXT("A living protected resident cannot publish a corpse alarm"),Fixture.Combat->State.Alarms.IsEmpty());
            TestTrue(TEXT("A protected resident leaves no pending corpse report"),Fixture.Enemy->Target->Memory.ReportingBody.IsNone());
            Fixture.Player->SetActorLocation(Resident->GetActorLocation()-FVector(130,0,0));
            Fixture.Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            const bool Carried=Fixture.Combat->Carry(false);
            TestFalse(TEXT("The production pickup entry refuses a living protected resident"),Carried);
            TestFalse(TEXT("A protected resident is never assigned a corpse carrier"),Resident->Target->Carrier.IsValid());
            if(Carried)Fixture.Combat->Cancel();
        }
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2NoiseNavigationTest,"Hearthward.Round2.Enemies.NoiseUsesSourcePathAndVisualPriority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRound2NoiseNavigationTest::RunTest(const FString&)
{
    FScopedCampaignAIWorld Fixture;if(!Fixture.Ready(*this))return false;
    auto* Wall=Fixture.World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Wall);
    Wall->AddInstanceComponent(Box);Wall->SetRootComponent(Box);Box->SetBoxExtent(FVector(50,400,250));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();Wall->SetActorLocation(Fixture.Origin+FVector(500,0,0));
    Fixture.Enemy->SetActorRotation(FRotator::ZeroRotator);Fixture.Player->SetActorLocation(Fixture.Origin+FVector(1000,0,0));
    Fixture.Perception(.4f);auto& Memory=Fixture.Enemy->Target->Memory;
    TestTrue(TEXT("Opaque wall hides the actual player from perception"),Memory.LastKnown.IsEmpty() && Memory.Seen.IsEmpty());
    const FVector Source=Fixture.Origin+FVector(0,1000,0);
    const auto Epoch=Fixture.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    if(!TestTrue(TEXT("Actual navigation accepts the separate noise source"),Fixture.Enemy->BeginNoiseInvestigation(Source,Epoch)))return false;
    Fixture.ExpectDestination(*this,TEXT("Noise path ends at source rather than hidden player"),Source);
    TestFalse(TEXT("An active investigation refuses repeated distraction"),Fixture.Enemy->BeginNoiseInvestigation(Fixture.Player->GetActorLocation(),Epoch));
    Fixture.Player->SetActorLocation(Fixture.Origin+FVector(1000,500,0));Fixture.Perception(.4f);Fixture.Enemy->Tick(.4f);
    Fixture.ExpectDestination(*this,TEXT("Hidden movement does not redirect source investigation"),Source);
    TestTrue(TEXT("Source investigation does not publish player knowledge"),Memory.LastKnown.IsEmpty() && Memory.Seen.IsEmpty());
    Wall->Destroy();Fixture.Player->SetActorLocation(Fixture.Origin+FVector(1000,0,0));Fixture.Perception(5.1f);Fixture.Enemy->Tick(.4f);
    TestTrue(TEXT("Uncovered player is confirmed through actual perception"),Memory.Seen.Contains(TEXT("player")));
    Fixture.ExpectDestination(*this,TEXT("Confirmed combat supersedes the source path"),Fixture.Player->GetActorLocation());
    TestFalse(TEXT("Further sound cannot interrupt confirmed combat"),Fixture.Enemy->BeginNoiseInvestigation(Source,Epoch));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2NoiseSelectionTest,"Hearthward.Round2.Enemies.FixedNoiseSelectsTwoNearestWithoutInventoryCost",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRound2NoiseSelectionTest::RunTest(const FString&)
{
    FScopedCampaignAIWorld Fixture;if(!Fixture.Ready(*this))return false;
    const FVector Source=Fixture.Origin+FVector(0,1000,0);
    auto* Campaign=Fixture.World->GetSubsystem<UHearthwardCampaignSubsystem>();
    TArray<AHearthwardCampaignActor*> Listeners{Fixture.Enemy};
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FName Ids[]={TEXT("field_slice_02"),TEXT("field_slice_03"),TEXT("home_workshops_18")};
    const double Distances[]={1200.,2000.,3500.};
    for(int32 Index=0;Index<3;++Index)
    {
        const FName Id=Ids[Index];const double Distance=Distances[Index];auto* Record=Campaign->Enemy(Id);
        if(!TestNotNull(TEXT("Listener reuses an existing soldier record"),Record))return false;
        Record->Home=Source+FVector(Distance,0,0);Record->Combat.Position=Record->Home;Record->Located=true;
        auto* Actor=Fixture.World->SpawnActor<AHearthwardCampaignActor>(Record->Home,FRotator::ZeroRotator,Params);Actor->Initialize(Id,true);Listeners.Add(Actor);
    }
    for(auto* Listener:Listeners)if(!TestTrue(TEXT("Initial listener is eligible before the source event"),Listener->CanInvestigateNoise()))return false;
    auto* Prop=Fixture.World->SpawnActor<AActor>();auto* Noise=NewObject<UHearthwardNoiseInteraction>(Prop);Prop->AddInstanceComponent(Noise);Prop->SetRootComponent(Noise);Noise->RegisterComponent();Prop->SetActorLocation(Source);
    Fixture.Player->SetActorLocation(Source+FVector(100,0,0));
    auto* Bag=Fixture.Player->FindComponentByClass<UHearthwardInventoryComponent>();Bag->TryAdd(TEXT("wood"),3);const auto Before=Bag->Snapshot();
    Noise->CompleteInteraction(Fixture.Player);
    TestFalse(TEXT("Nearest soldier accepts investigation"),Listeners[0]->CanInvestigateNoise());
    TestFalse(TEXT("Second nearest soldier accepts investigation"),Listeners[1]->CanInvestigateNoise());
    TestTrue(TEXT("Third nearby soldier remains free"),Listeners[2]->CanInvestigateNoise());
    TestTrue(TEXT("Soldier outside thirty metres remains free"),Listeners[3]->CanInvestigateNoise());
    Fixture.ExpectDestination(*this,TEXT("Actual fixed source produces the source navigation goal"),Source);
    Noise->CompleteInteraction(Fixture.Player);
    TestTrue(TEXT("Cooldown prevents recruiting a third listener"),Listeners[2]->CanInvestigateNoise());
    TestTrue(TEXT("Fixed source cooldown is ten seconds"),Noise->RemainingCooldown()>9.9f);
    const auto After=Bag->Snapshot();
    TestTrue(TEXT("Approved fixed source never consumes or creates inventory"),FHearthwardInventorySnapshot::StaticStruct()->CompareScriptStruct(&Before,&After,0));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2ConfirmedEnvironmentTest,"Hearthward.Round2.Enemies.ConfirmedTargetSurvivesLampAndRain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRound2ConfirmedEnvironmentTest::RunTest(const FString&)
{
    FScopedCampaignAIWorld Fixture;if(!Fixture.Ready(*this))return false;
    Fixture.Enemy->SetActorRotation(FRotator::ZeroRotator);Fixture.Enemy->Target->Exposure=0;
    Fixture.Player->SetActorLocation(Fixture.Origin+FVector(2400,0,0));
    auto* Prop=Fixture.World->SpawnActor<AActor>();auto* Lamp=NewObject<UHearthwardLampInteraction>(Prop);
    Prop->AddInstanceComponent(Lamp);Prop->SetRootComponent(Lamp);Lamp->Identity=TEXT("workshops_lamp_entry");Lamp->RegisterComponent();
    Prop->SetActorLocation(Fixture.Player->GetActorLocation()+FVector(0,0,20));Prop->DispatchBeginPlay();
    Fixture.Perception(13.f);auto& Memory=Fixture.Enemy->Target->Memory;
    if(!TestTrue(TEXT("Local lit lamp allows actual confirmation at twenty-four metres"),Memory.Seen.Contains(TEXT("player"))))return false;
    Lamp->CompleteInteraction(Fixture.Player);TestFalse(TEXT("Actual lamp interaction switches the lamp off"),Lamp->IsLit());
    Fixture.Perception(.4f);TestTrue(TEXT("Extinguishing light cannot erase a confirmed target"),Memory.Seen.Contains(TEXT("player")));
    auto* Clock=Fixture.World->GetSubsystem<UHearthwardWorldClockSubsystem>();Clock->AdvanceCalendar(210);
    auto* Save=Fixture.World->GetSubsystem<UHearthwardSaveSubsystem>();
    TestTrue(TEXT("Weather fixture binds its production identity"),Save->BindInteractionTarget(TEXT("world_light_rain"),EHearthwardPersistentKind::Weather,Prop));
    const auto Ticket=Save->BeginInteraction(TEXT("world_light_rain"));auto Next=Save->InteractionState();
    auto& Weather=Next.Weather.AddDefaulted_GetRef();Weather.Id=TEXT("world_light_rain");Weather.Seed=40;Weather.Phase=EHearthwardWeatherPhase::Rain;
    TestTrue(TEXT("Weather fixture commits a valid persistent state"),Save->CommitInteraction(Ticket,Next));
    TestEqual(TEXT("Calendar produces actual full light rain"),AHearthwardWeatherActor::Rain(Fixture.World),1.);
    Fixture.Enemy->Target->Exposure=1;Fixture.Perception(.4f);
    TestTrue(TEXT("Rain cannot erase confirmed enemy knowledge"),Memory.Seen.Contains(TEXT("player")));
    TestTrue(TEXT("Exposed unconfirmed visibility is reduced by ten percent"),FMath::IsNearlyEqual(AHearthwardWeatherActor::VisibilityFactor(Fixture.World,Fixture.Player->GetActorLocation(),Fixture.Player),.9));
    Memory.Detection.FindOrAdd(TEXT("player"))=0;Memory.Seen.Reset();Fixture.Perception(.4f);
    TestEqual(TEXT("Unconfirmed target beyond rainy sight range remains undiscovered"),Memory.Detection.FindRef(TEXT("player")),0.f);
    return true;
}
#endif
