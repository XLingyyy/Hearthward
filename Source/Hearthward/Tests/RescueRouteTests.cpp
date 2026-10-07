#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationData.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"

namespace
{
struct FRescueRouteWorld
{
    const FVector Origin=FVector(-98000,-75000,80);
    UWorld* World=nullptr;
    ACharacter* Player=nullptr;
    UHearthwardGameplayComponent* Gameplay=nullptr;
    UHearthwardCampaignSubsystem* Campaign=nullptr;
    UHearthwardCampSubsystem* Camps=nullptr;
    UNavigationSystemV1* Nav=nullptr;

    FRescueRouteWorld()
    {
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        auto* Floor=World->SpawnActor<AActor>();Floor->Tags.Add(TEXT("Hearthward.NatureGround"));
        auto* Box=NewObject<UBoxComponent>(Floor);Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(12000,12000,50));Box->SetCollisionProfileName(TEXT("BlockAll"));
        Floor->SetActorLocation(Origin-FVector(0,0,130));
        Box->SetMobility(EComponentMobility::Static);Box->RegisterComponent();Floor->RegisterAllComponents();
        auto* Bounds=World->SpawnActor<ANavMeshBoundsVolume>();
        Bounds->GetRootComponent()->SetMobility(EComponentMobility::Movable);
        auto* BoundsBox=NewObject<UBoxComponent>(Bounds);Bounds->AddInstanceComponent(BoundsBox);
        BoundsBox->SetupAttachment(Bounds->GetRootComponent());BoundsBox->SetBoxExtent(FVector(12500,12500,300));
        BoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);BoundsBox->SetCanEverAffectNavigation(false);
        BoundsBox->RegisterComponent();Bounds->SetActorLocation(Origin);Bounds->RegisterAllComponents();

        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Player=World->SpawnActor<ACharacter>(Origin+FVector(6500,0,10),FRotator::ZeroRotator,Params);
        Player->GetCapsuleComponent()->InitCapsuleSize(34,90);Player->GetCapsuleComponent()->SetCanEverAffectNavigation(false);
        auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
        auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
        auto* Combat=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
        auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
        Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();Campaign->State.Phase=TEXT("occupied");
        // Isolated diagnostic seed only; no runtime spawn, reward or patrol data is changed.
        Person().Position=Origin+FVector(6680,0,0);Person().Located=true;
        Camps=World->GetSubsystem<UHearthwardCampSubsystem>();Camps->EnsureCamp(Origin);
        Campaign->Tick(.4f);
        Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
        if(!Nav)return;
        Nav->RegisterNavigationInvoker(Player,12000,13000);
        Nav->OnNavigationBoundsUpdated(Bounds);
        if(!Nav->IsInitialized())Nav->OnWorldInitDone(FNavigationSystemRunMode::GameMode);
        Nav->FlushPendingOperations();
        for(int32 I=0;I<6;++I)TickNavigation(.25f);
        Nav->Build();
        for(int32 I=0;I<6;++I)TickNavigation(.25f);
    }
    ~FRescueRouteWorld(){GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}
    FHearthwardCampaignPerson& Person() const
    {return *Campaign->State.People.FindByPredicate([](const auto& P){return P.Id==TEXT("rescued_01");});}
    AHearthwardCampaignActor* Actor() const{return Campaign->Actor(TEXT("rescued_01"));}
    void TickNavigation(float Delta)
    {
        World->Tick(LEVELTICK_TimeOnly,Delta);
        if(auto* Data=Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))Data->EnsureBuildCompletion();
    }
    void Step()
    {
        TickNavigation(.1f);Campaign->Tick(.1f);
        if(auto* A=Actor())
        {
            if(auto* AI=Cast<AAIController>(A->GetController()))AI->GetPathFollowingComponent()->TickComponent(.1f,LEVELTICK_All,nullptr);
            A->GetCharacterMovement()->TickComponent(.1f,LEVELTICK_All,nullptr);
        }
    }
    bool Ready(FAutomationTestBase& Test)
    {
        if(!Test.TestNotNull(TEXT("Diagnostic has real dynamic navigation"),Nav)
            || !Test.TestNotNull(TEXT("Production refresh spawns the actual rescued identity"),Actor()))return false;
        auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(World,Actor()->GetActorLocation(),Origin,Actor());
        return Test.TestTrue(TEXT("Diagnostic has a complete real AI path into camp"),Path && Path->IsValid() && !Path->IsPartial());
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRescuePhysicalArrival092Test,"Hearthward.Iteration.Task092.Fixture.FollowWaitAndPhysicalArrival",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRescuePhysicalArrival092Test::RunTest(const FString&)
{
    FRescueRouteWorld F;if(!F.Ready(*this))return false;
    AddInfo(TEXT("Layer: isolated flat navigation diagnostic; player destination reposition is fixture setup, not normal input route proof."));
    TestTrue(TEXT("Contact uses production interaction"),F.Campaign->Interact());
    TestEqual(TEXT("Contact begins player escort"),F.Person().Stage,FName(TEXT("following")));
    F.Player->SetActorLocation(F.Actor()->GetActorLocation()+FVector(250,0,10));
    F.Campaign->Tick(.4f);
    auto* AI=Cast<AAIController>(F.Actor()->GetController());
    if(!TestNotNull(TEXT("Rescue has its production AI controller"),AI)
        || !TestTrue(TEXT("Fixture starts a real move while still in interaction range"),AI->GetMoveStatus()==EPathFollowingStatus::Moving))return false;
    TestTrue(TEXT("Second interaction waits"),F.Campaign->Interact());
    TestEqual(TEXT("Wait retains stable person"),F.Person().Stage,FName(TEXT("waiting")));
    TestTrue(TEXT("Explicit wait cancels the active production AI path"),AI->GetMoveStatus()==EPathFollowingStatus::Idle);
    const FVector Waiting=F.Actor()->GetActorLocation();
    for(int32 I=0;I<10;++I)F.Step();
    TestTrue(TEXT("Waiting person does not move toward camp"),FVector::Dist2D(Waiting,F.Actor()->GetActorLocation())<1);
    TestTrue(TEXT("Third interaction resumes"),F.Campaign->Interact());
    const int32 XP=F.Gameplay->Experience,Population=F.Camps->State.Population();
    TestFalse(TEXT("Conversation outside camp grants no rescue"),F.Camps->State.Rescued.Contains(TEXT("rescued_01")));
    F.Player->SetActorLocation(F.Origin+FVector(0,0,10));
    for(int32 I=0;I<200 && F.Person().Stage!=TEXT("arrived");++I)F.Step();
    TestTrue(TEXT("Rescued actor physically enters the actual camp radius"),!F.Camps->State.CampAt(F.Actor()->GetActorLocation()).IsNone());
    TestTrue(TEXT("PathFollowing and CharacterMovement changed rescued actor position"),FVector::Dist2D(Waiting,F.Actor()->GetActorLocation())>1000);
    TestEqual(TEXT("Arrival commits persistent person stage"),F.Person().Stage,FName(TEXT("arrived")));
    TestEqual(TEXT("Actual arrival adds population once"),F.Camps->State.Population(),Population+1);
    TestTrue(TEXT("Arrival grants the configured rescue experience"),F.Gameplay->Experience>XP);
    const int32 ArrivedXP=F.Gameplay->Experience;
    for(int32 I=0;I<20;++I)F.Step();
    TestFalse(TEXT("Repeated arrival cannot add the same rescue"),F.Camps->RecordRescue(TEXT("rescued_01")));
    TestEqual(TEXT("Repeated refresh cannot duplicate population"),F.Camps->State.Population(),Population+1);
    TestEqual(TEXT("Repeated refresh cannot duplicate experience"),F.Gameplay->Experience,ArrivedXP);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRescueLoadedSegment092Test,"Hearthward.Iteration.Task092.Live.LoadedSegmentClearance",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRescueLoadedSegment092Test::RunTest(const FString&)
{
    UWorld* World=nullptr;
    for(const auto& Context:GEngine->GetWorldContexts())
        if(auto* W=Context.World();W && (W->WorldType==EWorldType::Game || W->WorldType==EWorldType::PIE)
            && UGameplayStatics::GetCurrentLevelName(W,true)==TEXT("L_HearthwardWilds"))
        {World=W;break;}
    if(!World){AddError(TEXT("Prerequisite: an active L_HearthwardWilds Game/PIE world is required; no flat fixture substitutes for this check."));return false;}
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();
    auto* Player=Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(World,0));
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if(!TestNotNull(TEXT("Live player"),Player) || !TestNotNull(TEXT("Live campaign"),Campaign) || !TestNotNull(TEXT("Live navigation"),Nav))return false;
    if(Campaign->State.Phase==TEXT("prologue") || Campaign->Busy())
    {AddError(TEXT("Prerequisite: finish the existing prologue and any pending travel/action before the route probe."));return false;}
    TArray<ACharacter*> Actors{Player};
    for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It){Actors.Add(*It);break;}
    auto* Rescued=Campaign->Actor(TEXT("rescued_01"));
    if(!Rescued || Actors.Num()!=2)
    {AddError(TEXT("Prerequisite: load the brother and rescued_01 on the first-rescue route; this test does not spawn or move them."));return false;}
    Actors.Add(Rescued);

    const FVector RescueXY=HearthwardCampaign::XY(HearthwardCampaign::Find(TEXT("locations"),TEXT("slice_rescue")),TEXT("xy"));
    TArray<FVector> Trace;
    for(const auto& V:HearthwardData::Catalog()->GetObjectField(TEXT("campaign"))->GetArrayField(TEXT("route_trace")))
    {const auto& A=V->AsArray();Trace.Add(FVector(A[0]->AsNumber()*100,A[1]->AsNumber()*100,0));}
    int32 Last=INDEX_NONE;double RescueDistance=MAX_dbl;
    for(int32 I=0;I<Trace.Num();++I)if(const double D=FVector::DistSquared2D(Trace[I],RescueXY);D<RescueDistance){RescueDistance=D;Last=I;}
    if(Last<1){AddError(TEXT("First-rescue route trace is unavailable"));return false;}
    Trace.SetNum(Last+1);Trace.Add(RescueXY);
    const FVector Here(Player->GetActorLocation().X,Player->GetActorLocation().Y,0);
    int32 Closest=INDEX_NONE;FVector OnTrace;double Distance=MAX_dbl;
    for(int32 I=0;I<Trace.Num()-1;++I)
    {
        const FVector P=FMath::ClosestPointOnSegment(Here,Trace[I],Trace[I+1]);
        if(const double D=FVector::DistSquared2D(Here,P);D<Distance){Distance=D;Closest=I;OnTrace=P;}
    }
    if(Closest==INDEX_NONE || Distance>FMath::Square(2500.))
    {AddError(TEXT("Prerequisite: player must be within 25m of the existing camp-to-first-rescue trace."));return false;}
    FCollisionQueryParams Collision(SCENE_QUERY_STAT(Task092Clearance),false);
    for(TActorIterator<APawn> It(World);It;++It)Collision.AddIgnoredActor(*It);
    TArray<FVector> Points{Here};
    // Neighbouring authored samples remain inside the currently loaded invoker area.
    Points.Add(Trace[FMath::Max(0,Closest-2)]);
    Points.Add(Trace[FMath::Min(Trace.Num()-1,Closest+3)]);
    TArray<FVector> Floors;
    for(const FVector Point:Points)
    {
        FVector Floor;if(!TestTrue(FString::Printf(TEXT("Loaded terrain ground at %s"),*Point.ToString()),Campaign->Ground(Point,Floor)))return false;
        Floors.Add(Floor);
    }
    AddInfo(FString::Printf(TEXT("LIVE_LOCAL_ONLY: segment=%d player=%s navBuilding=%d; no input, transform, actor, reward or nav-invoker mutation"),
        Closest,*Player->GetActorLocation().ToString(),Nav->IsNavigationBuildInProgress()));
    for(auto* Actor:Actors)
    {
        const auto* Capsule=Actor->GetCapsuleComponent();
        auto Props=Actor->GetNavAgentPropertiesRef();
        const auto* Data=Nav->GetNavDataForProps(Props,Floors[0]);
        if(!TestNotNull(FString::Printf(TEXT("NavData for actual %s agent"),*Actor->GetClass()->GetName()),Data))return false;
        if(!Actor->GetCharacterMovement()->IsMovingOnGround())
        {AddError(TEXT("Prerequisite: let all three actors settle on the ground before the local clearance snapshot."));return false;}
        TestFalse(FString::Printf(TEXT("Actual standing capsule %s at %s"),*Actor->GetClass()->GetName(),*Actor->GetActorLocation().ToString()),
            World->OverlapBlockingTestByChannel(Actor->GetActorLocation(),FQuat::Identity,ECC_Pawn,
                FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()),Collision));
        TArray<FNavLocation> Projected;
        for(const FVector Floor:Floors)
        {
            FNavLocation P;
            if(!TestTrue(TEXT("Loaded local point projects for the actual agent"),Nav->ProjectPointToNavigation(Floor,P,FVector(100,100,220),Data)))return false;
            Projected.Add(P);
        }
        for(int32 I=1;I<Projected.Num();++I)for(const bool Reverse:{false,true})
        {
            FPathFindingQuery Query(Actor,*Data,Projected[Reverse?I:0].Location,Projected[Reverse?0:I].Location);
            Query.SetAllowPartialPaths(false);
            const auto Path=Nav->FindPathSync(Props,Query);
            TestTrue(FString::Printf(TEXT("Complete local %s path for %s; sample %d"),Reverse?TEXT("return"):TEXT("outbound"),*Actor->GetClass()->GetName(),I),
                Path.IsSuccessful() && Path.Path.IsValid() && Path.Path->IsValid() && !Path.Path->IsPartial());
        }
    }
    AddInfo(TEXT("This snapshot checks standing clearance and nearby complete paths only. Full route, physical following, stairs, streaming, saves and normal OS input require separate play evidence."));
    return true;
}
#endif
