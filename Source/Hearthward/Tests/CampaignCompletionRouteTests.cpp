#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../UI/HearthwardQuestGuidance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"

namespace
{
struct FQuestKnowledgeWorld100
{
    UWorld* World=nullptr;
    ACharacter* Player=nullptr;
    APlayerController* Controller=nullptr;
    UHearthwardGameplayComponent* Gameplay=nullptr;
    UHearthwardCampaignSubsystem* Campaign=nullptr;

    explicit FQuestKnowledgeWorld100(FName Phase)
    {
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Player=World->SpawnActor<ACharacter>(FVector(-98000,-75000,100),FRotator::ZeroRotator,Params);
        Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
        Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
        Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();Campaign->State.Phase=Phase;
        Gameplay->Discovered.Reset();Gameplay->Explored.Reset();Gameplay->Claimed.Reset();
    }
    ~FQuestKnowledgeWorld100(){GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}

    bool Ready(FAutomationTestBase& Test) const
    {
        return Test.TestTrue(TEXT("The registered real world resolves the possessed player"),UGameplayStatics::GetPlayerPawn(World,0)==Player)
            && Test.TestTrue(TEXT("The production campaign is active"),Campaign->Active());
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnknownSideGoal100Test,"Hearthward.Iteration.Task100.Fixture.UnknownSideGoalIsHidden",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FUnknownSideGoal100Test::RunTest(const FString&)
{
    FQuestKnowledgeWorld100 F(TEXT("occupied"));if(!F.Ready(*this))return false;
    F.Gameplay->TrackedQuest=TEXT("side_05");
    if(!TestTrue(TEXT("Existing occupied-phase availability is preserved"),F.Gameplay->QuestAvailable(TEXT("side_05")))
        || !TestEqual(TEXT("The real quest points to the undiscovered dwellings"),F.Campaign->QuestLocation(TEXT("side_05")),FName(TEXT("loc_dwellings"))))return false;
    TestFalse(TEXT("The player has not discovered this place"),F.Gameplay->Discovered.Contains(TEXT("loc_dwellings")));
    TestTrue(TEXT("The player has no explored samples"),F.Gameplay->Explored.IsEmpty());
    TestFalse(TEXT("The hunter-record clue has not been acquired"),F.Campaign->State.Facts.Contains(TEXT("hunter_record")));
    const int32 RewardFacts=F.Gameplay->RewardFacts.Num(),Facts=F.Campaign->State.Facts.Num();
    const auto Unknown=HearthwardQuestGuidance::Resolve(F.Controller);
    AddInfo(FString::Printf(TEXT("Task100 actual HUD resolver: side_05 available=%d location=%s visible=%d world=%s"),
        F.Gameplay->QuestAvailable(TEXT("side_05")),*Unknown.Location.ToString(),Unknown.Visible,*Unknown.World.ToString()));
    TestFalse(TEXT("Tracking an available side quest must not reveal an unknown location"),Unknown.Visible);
    TestTrue(TEXT("Reading guidance does not discover or explore the target"),F.Gameplay->Discovered.IsEmpty() && F.Gameplay->Explored.IsEmpty());
    TestEqual(TEXT("Reading guidance cannot grant a reward"),F.Gameplay->RewardFacts.Num(),RewardFacts);
    TestEqual(TEXT("Reading guidance cannot manufacture a clue"),F.Campaign->State.Facts.Num(),Facts);

    F.Gameplay->Discovered.Add(TEXT("loc_dwellings"));
    const auto Known=HearthwardQuestGuidance::Resolve(F.Controller);
    TestTrue(TEXT("The existing known destination remains guidable"),Known.Visible);
    TestEqual(TEXT("Known guidance retains the original stable location"),Known.Location,FName(TEXT("loc_dwellings")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPrologueActionGoal100Test,"Hearthward.Iteration.Task100.Fixture.PrologueActionGoalRemainsVisible",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPrologueActionGoal100Test::RunTest(const FString&)
{
    FQuestKnowledgeWorld100 F(TEXT("prologue"));if(!F.Ready(*this))return false;
    F.Gameplay->TrackedQuest=TEXT("main_01");
    if(!TestTrue(TEXT("The normal opening action quest remains available"),F.Gameplay->QuestAvailable(TEXT("main_01"))))return false;
    const auto Relic=HearthwardQuestGuidance::Resolve(F.Controller);
    TestTrue(TEXT("The opening relic action is already known through the prologue"),Relic.Visible);
    TestEqual(TEXT("The opening retains its real relic destination"),Relic.Location,FName(TEXT("prologue_relic")));
    F.Campaign->State.Facts.Add(TEXT("relic"));
    const auto Exit=HearthwardQuestGuidance::Resolve(F.Controller);
    TestTrue(TEXT("Taking the relic retains the known escape action"),Exit.Visible);
    TestEqual(TEXT("The existing prologue chooses the escape destination"),Exit.Location,FName(TEXT("prologue_exit")));
    TestTrue(TEXT("Guidance does not fabricate map discovery"),F.Gameplay->Discovered.IsEmpty() && F.Gameplay->Explored.IsEmpty());
    AddInfo(TEXT("Layer: isolated real-world/controller resolver fixture; no normal-input, rendering or campaign-completion claim."));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaitingRescueGoal100Test,"Hearthward.Iteration.Task100.Fixture.WaitingRescueUsesCurrentPersonPosition",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWaitingRescueGoal100Test::RunTest(const FString&)
{
    FQuestKnowledgeWorld100 F(TEXT("occupied"));if(!F.Ready(*this))return false;
    F.Gameplay->Claimed.Add(TEXT("main_01"));F.Gameplay->Claimed.Add(TEXT("main_02"));
    F.Gameplay->TrackedQuest=TEXT("main_03");
    F.Gameplay->Discovered.Add(TEXT("slice_rescue"));F.Gameplay->Discovered.Add(TEXT("route_fork"));
    auto* Person=F.Campaign->State.People.FindByPredicate([](const auto& P){return P.Id==TEXT("rescued_01");});
    if(!TestNotNull(TEXT("The real first rescue exists"),Person)
        || !TestTrue(TEXT("The original main quest prerequisites are satisfied"),F.Gameplay->QuestAvailable(TEXT("main_03"))))return false;
    const FVector SavedPosition(-81000,-80000,100);
    Person->Stage=TEXT("waiting");Person->Position=SavedPosition;Person->Located=true;
    TestTrue(TEXT("The contacted person has moved away from the original rescue site"),
        FVector::Dist2D(SavedPosition,F.Campaign->Position(TEXT("slice_rescue")))>10000);
    TestNull(TEXT("The isolated unloaded person has no campaign actor yet"),F.Campaign->Actor(TEXT("rescued_01")));
    const int32 RewardFacts=F.Gameplay->RewardFacts.Num(),Discovered=F.Gameplay->Discovered.Num();
    const auto Unloaded=HearthwardQuestGuidance::Resolve(F.Controller);
    AddInfo(FString::Printf(TEXT("Task100 waiting rescue fallback: world=%s person=%s route=%d route_id=%s"),
        *Unloaded.World.ToString(),*SavedPosition.ToString(),Unloaded.HasRoute,*Unloaded.RouteId.ToString()));
    TestTrue(TEXT("A known waiting rescue remains visible"),Unloaded.Visible);
    TestTrue(TEXT("An unloaded waiting rescue points to the recorded current person position"),Unloaded.World.Equals(SavedPosition,1));
    TestFalse(TEXT("An already contacted rescue does not reuse the outbound fork route"),Unloaded.HasRoute);

    F.Campaign->Tick(0);
    auto* Actor=F.Campaign->Actor(TEXT("rescued_01"));
    if(!TestNotNull(TEXT("The actual campaign refresh loads the waiting person actor"),Actor))return false;
    const FVector ActorPosition=SavedPosition+FVector(1200,700,0);
    Actor->SetActorLocation(ActorPosition,false,nullptr,ETeleportType::TeleportPhysics);
    const auto Loaded=HearthwardQuestGuidance::Resolve(F.Controller);
    TestTrue(TEXT("Loaded waiting guidance uses the live actor before the next snapshot sync"),Loaded.World.Equals(Actor->GetActorLocation(),1));
    TestFalse(TEXT("Loaded waiting guidance still omits the outbound fork route"),Loaded.HasRoute);
    TestEqual(TEXT("Guidance does not grant rescue rewards"),F.Gameplay->RewardFacts.Num(),RewardFacts);
    TestEqual(TEXT("Guidance does not add discoveries"),F.Gameplay->Discovered.Num(),Discovered);

    Person=F.Campaign->State.People.FindByPredicate([](const auto& P){return P.Id==TEXT("rescued_01");});
    Person->Stage=TEXT("following");
    const auto Following=HearthwardQuestGuidance::Resolve(F.Controller);
    TestTrue(TEXT("A resumed escort retains its known camp destination"),Following.Visible);
    TestEqual(TEXT("Following retains the original camp location"),Following.Location,FName(TEXT("camp")));
    TestFalse(TEXT("Following does not reuse the outbound fork route"),Following.HasRoute);
    AddInfo(TEXT("Layer: isolated real Game world/controller/player, actual campaign refresh and live person actor; fixture movement is diagnostic, not normal rescue completion."));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZoneArchitecture100Test,"Hearthward.Iteration.Task100.Fixture.ZoneArchitecturePassages",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FZoneArchitecture100Test::RunTest(const FString&)
{
    FQuestKnowledgeWorld100 F(TEXT("occupied"));if(!F.Ready(*this))return false;
    const TCHAR* Names[]={TEXT("SM_RiverGate"),TEXT("SM_WorkshopShelter"),TEXT("SM_DwellingPorch"),TEXT("SM_AssemblyColonnade")};
    const float SideY[]={270,280,240,320};
    for(int32 I=0;I<4;++I)
    {
        const FString Path=FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-100/Zones/%s.%s"),Names[I],Names[I]);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
        if(!TestNotNull(Names[I],Mesh))continue;
        const FVector Size=Mesh->GetBoundingBox().GetSize();
        AddInfo(FString::Printf(TEXT("%s dimensions cm=%s convex=%d"),Names[I],*Size.ToString(),Mesh->GetBodySetup()?Mesh->GetBodySetup()->AggGeom.ConvexElems.Num():0));
        TestTrue(TEXT("Building is imported in centimetres"),Size.X>=190 && Size.X<=730 && Size.Y>=550 && Size.Y<=740 && Size.Z>=450 && Size.Z<=550);
        TestTrue(TEXT("Separate authored convex collision preserves openings"),Mesh->GetBodySetup() && Mesh->GetBodySetup()->AggGeom.ConvexElems.Num()>=20);
        auto* Actor=F.World->SpawnActor<AActor>();
        auto* Part=NewObject<UStaticMeshComponent>(Actor);Actor->AddInstanceComponent(Part);Actor->SetRootComponent(Part);
        Part->SetStaticMesh(Mesh);Part->SetCollisionProfileName(TEXT("BlockAll"));Part->RegisterComponent();
        const FVector Origin(I*2000,0,0);Actor->SetActorLocation(Origin);
        FHitResult Hit;
        TestFalse(TEXT("Player capsule can pass through the centre in both directions"),F.World->SweepSingleByChannel(Hit,Origin+FVector(-450,0,100),Origin+FVector(450,0,100),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,90)));
        TestTrue(TEXT("Side masonry really blocks the capsule"),F.World->SweepSingleByChannel(Hit,Origin+FVector(-450,SideY[I],100),Origin+FVector(450,SideY[I],100),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,90)));
    }
    AddInfo(TEXT("Isolated asset collision fixture; does not certify terrain or a completed campaign."));
    return true;
}
#endif
