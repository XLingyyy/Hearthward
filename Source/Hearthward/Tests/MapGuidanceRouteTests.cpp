#include "../UI/HearthwardGuidanceRoutes.h"
#include "../Building/HearthwardHometownFortress.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuidanceRoute091Test,"Hearthward.Iteration.Task091.LoadedFortressSpatialNodes",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGuidanceRoute091Test::RunTest(const FString&)
{
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Ground=World->SpawnActor<AActor>();Ground->Tags.Add(TEXT("Hearthward.NatureGround"));
    auto* Box=NewObject<UBoxComponent>(Ground);Ground->AddInstanceComponent(Box);Ground->SetRootComponent(Box);
    Box->SetBoxExtent({20000,20000,50});Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Ground->SetActorLocation({0,0,-50});
    auto* Home=World->SpawnActor<AHearthwardHometownFortress>();Home->DispatchBeginPlay();
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();
    const auto Before=Campaign->State.Snapshot();
    auto Step=HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),Home->BedroomLanding());
    TestTrue(TEXT("loaded bedroom has an actual door step"),Step.Visible && Step.Id==TEXT("bedroom_door"));
    TestEqual(TEXT("door height derives from loaded floor"),Step.World.Z,Home->BedroomLanding().Z);
    TestFalse(TEXT("door destination clears a full player capsule"),World->OverlapBlockingTestByChannel(Step.World,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96)));
    Step=HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),Home->BedroomLanding()+FVector(0,700,0));
    TestEqual(TEXT("gallery selects the real staircase entrance"),Step.Id,FName(TEXT("escape_stair_top")));
    // Nav06 ordinary PathFollowing arrived 40.056 cm short of the loaded stair-top node.
    const FVector Nav06Arrival=Step.World+FVector(-37.74372813509,-10.87451043522,-7.849997997284);
    TestTrue(TEXT("recorded normal arrival retains its measured node distance"),
        FMath::IsNearlyEqual(FVector::Dist(Step.World,Nav06Arrival),40.055791832123475,.001));
    const auto AfterArrival=HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),Nav06Arrival);
    TestEqual(TEXT("actual normal-path stair-top arrival advances to the existing lower stair node"),
        AfterArrival.Id,FName(TEXT("escape_stair_bottom")));
    const auto OuterGallery=HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),
        Home->BedroomLanding()+FVector(900,940,0));
    TestEqual(TEXT("gallery outside the stair entrance strip keeps the top node"),OuterGallery.Id,FName(TEXT("escape_stair_top")));
    Step=HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),FVector(1500,1250,180));
    TestEqual(TEXT("stair node uses generated lower step"),Step.Id,FName(TEXT("escape_stair_bottom")));
    TestTrue(TEXT("lower step lies beyond staircase entry"),Step.World.Y>1250 && Step.World.Z<Home->BedroomLanding().Z);
    Step=HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),FVector(1500,2900,100));
    TestEqual(TEXT("courtyard identifies existing western opening"),Step.Id,FName(TEXT("courtyard_side_gate")));
    const FVector Nav07GateArrival=Step.World+FVector(37.67290882219,5.52572665088,-.98352943916);
    TestTrue(TEXT("recorded normal gate arrival retains its measured node distance"),
        FMath::IsNearlyEqual(FVector::Dist(Step.World,Nav07GateArrival),38.088699693,.001));
    TestFalse(TEXT("actual normal-path gate arrival hands over to the ordinary world target"),
        HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),Nav07GateArrival).Visible);
    const auto InsideGate=HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),Step.World+FVector(100,0,0));
    TestTrue(TEXT("court 100 cm inside the actual gate retains the gate node"),InsideGate.Visible && InsideGate.Id==TEXT("courtyard_side_gate"));
    TestFalse(TEXT("out of applicable court returns ordinary target"),HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),FVector(-2500,4000,100)).Visible);
    Step=HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),Home->BedroomLanding());
    TestEqual(TEXT("return walk recomputes door without saved node progress"),Step.Id,FName(TEXT("bedroom_door")));
    TestFalse(TEXT("relic stage does not prematurely guide evacuation"),HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_relic"),Home->BedroomLanding()).Visible);
    TestFalse(TEXT("unrelated quest falls back safely"),HearthwardGuidanceRoutes::Resolve(World,TEXT("side_01"),TEXT("prologue_exit"),Home->BedroomLanding()).Visible);
    TestEqual(TEXT("route resolution changes no campaign facts"),Campaign->State.Snapshot(),Before);
    Campaign->State.Phase=TEXT("occupied");
    TestFalse(TEXT("phase change clears opening nodes"),HearthwardGuidanceRoutes::Resolve(World,TEXT("main_01"),TEXT("prologue_exit"),Home->BedroomLanding()).Visible);
    TestFalse(TEXT("missing world has no node"),HearthwardGuidanceRoutes::Resolve(nullptr,TEXT("main_01"),TEXT("prologue_exit"),{}).Visible);
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
#endif
