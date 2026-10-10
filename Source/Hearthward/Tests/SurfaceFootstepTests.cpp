#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "CoreGlobals.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "../Experience/HearthwardFootContactNotify.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "../Experience/HearthwardFootstepSurface.h"
#include "../Building/HearthwardTask028CampHouse.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysicsSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurfaceFootstepMapping104Test,"Hearthward.Iteration.Task104.Footstep.SurfacePolicy",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSurfaceFootstepMapping104Test::RunTest(const FString&)
{
    using namespace HearthwardFootstepSurface;
    for(const FName Surface:{FName(TEXT("grass")),FName(TEXT("dirt")),FName(TEXT("stone")),FName(TEXT("wood"))})
    {
        TestEqual(TEXT("Configured physical surface names are canonical"),Canonical(Surface),Surface);
        TestEqual(TEXT("Each surface selects its own event"),EventFor(Surface),FName(*(FString(TEXT("movement.footstep."))+Surface.ToString())));
    }
    TestEqual(TEXT("Unmapped surfaces select the neutral fallback"),EventFor(TEXT("metal")),FName(TEXT("movement.footstep")));
    TestEqual(TEXT("No material identity is guessed from substrings"),FromMaterialPath(TEXT("/Game/Other/WoodenSign.WoodenSign")),NAME_None);
    TestEqual(TEXT("Fortress bedroom timber maps to wood"),FromMaterialPath(TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber.M_OldTimber")),FName(TEXT("wood")));
    TestEqual(TEXT("Fortress stairs map to stone"),FromMaterialPath(TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_RoughStone.M_RoughStone")),FName(TEXT("stone")));
    FHitResult Miss;TestEqual(TEXT("Nonblocking result cannot establish a surface"),Resolve(Miss),NAME_None);
    Miss.bBlockingHit=true;Miss.bStartPenetrating=true;
    TestEqual(TEXT("Penetrating result cannot establish a surface"),Resolve(Miss),NAME_None);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurfaceFootstepVariation104Test,"Hearthward.Iteration.Task104.Footstep.NoImmediateRepeat",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSurfaceFootstepVariation104Test::RunTest(const FString&)
{
    using namespace HearthwardFootstepSurface;
    TestEqual(TEXT("No variants uses the legacy single file"),SelectVariant(0,INDEX_NONE,0),INDEX_NONE);
    TestEqual(TEXT("Single variant stays valid"),SelectVariant(1,0,0),0);
    for(int32 Count=2;Count<=8;++Count)for(int32 Previous=0;Previous<Count;++Previous)
    {
        TSet<int32> Selected;
        for(int32 Draw=0;Draw<Count-1;++Draw)
        {
            const int32 Index=SelectVariant(Count,Previous,Draw);
            TestTrue(TEXT("Selection stays in bounds and excludes the immediately prior sample"),Index>=0 && Index<Count && Index!=Previous);
            Selected.Add(Index);
        }
        TestEqual(TEXT("Every other variant remains reachable with equal draw probability"),Selected.Num(),Count-1);
    }
    for(int32 Draw=0;Draw<3;++Draw)TestEqual(TEXT("First playback can use every variant"),SelectVariant(3,INDEX_NONE,Draw),Draw);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurfaceFootstepTerrain104Test,"Hearthward.Iteration.Task104.Footstep.AuthoredTerrain",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSurfaceFootstepTerrain104Test::RunTest(const FString&)
{
    using namespace HearthwardFootstepSurface;
    TestEqual(TEXT("Actual camp clearing mask is grass"),TerrainAt(FVector(-98000,-75000,0)),FName(TEXT("grass")));
    TestEqual(TEXT("Actual bounded S1 forest floor overlay is dirt"),TerrainAt(FVector(-110000,-70000,0)),FName(TEXT("dirt")));
    TestEqual(TEXT("Actual riverbank dirt mask"),TerrainAt(FVector(60000,0,0)),FName(TEXT("dirt")));
    TestEqual(TEXT("Actual northern exposed rock mask"),TerrainAt(FVector(-30000,140000,0)),FName(TEXT("stone")));
    TestEqual(TEXT("Unmapped world position cannot wrap to another surface"),TerrainAt(FVector(201601,0,0)),NAME_None);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurfaceFootstepCollision104Test,"Hearthward.Iteration.Task104.Footstep.ActualHitAndPhysicalPriority",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSurfaceFootstepCollision104Test::RunTest(const FString&)
{
    // New solvers snapshot the registered materials for scene queries at creation.
    auto* Physical=NewObject<UPhysicalMaterial>();Physical->SurfaceType=SurfaceType62;
    Physical->GetPhysicsMaterial();
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT {World->DestroyWorld(false);GEngine->DestroyWorldContext(World);};
    // The production house's collision-only floor has no render material. Reproduce
    // that exact owning class/component identity, not a fake wood actor-wide tag.
    auto* House=World->SpawnActor<AHearthwardTask028CampHouse>();
    auto* Floor=NewObject<UBoxComponent>(House,TEXT("WalkableFloor"));House->AddInstanceComponent(Floor);
    Floor->SetupAttachment(House->GetRootComponent());Floor->SetBoxExtent(FVector(100,100,10));
    Floor->SetCollisionProfileName(TEXT("BlockAll"));Floor->RegisterComponent();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SurfaceFootstepTest104));Query.bReturnPhysicalMaterial=true;
    const auto Trace=[&](FHitResult& Hit)
    {return World->LineTraceSingleByChannel(Hit,FVector(0,0,100),FVector(0,0,-100),ECC_Visibility,Query);};
    FHitResult Hit;if(!TestTrue(TEXT("Test reaches an actual blocking floor"),Trace(Hit)))return false;
    TestTrue(TEXT("The trace actually hits the named floor box"),Hit.GetComponent()==Floor);
    TestEqual(TEXT("Real camp floor collision selects timber"),HearthwardFootstepSurface::Resolve(Hit),FName(TEXT("wood")));
    // The production entrance steps also use dedicated, invisible collision boxes.
    int32 StepIndex=0;
    for(const FName Name:{FName(TEXT("StepLow")),FName(TEXT("StepMiddle")),FName(TEXT("StepHigh"))})
    {
        const FVector Center(300+StepIndex++*300,0,0);
        auto* Step=NewObject<UBoxComponent>(House,Name);House->AddInstanceComponent(Step);
        Step->SetupAttachment(House->GetRootComponent());Step->SetRelativeLocation(Center);
        Step->SetBoxExtent(FVector(100,100,10));Step->SetCollisionProfileName(TEXT("BlockAll"));Step->RegisterComponent();
        FHitResult StepHit;
        if(!TestTrue(TEXT("Entrance step has an actual collision hit"),World->LineTraceSingleByChannel(StepHit,Center+FVector(0,0,100),Center-FVector(0,0,100),ECC_Visibility,Query)))return false;
        TestTrue(TEXT("Step trace reaches the intended collision component"),StepHit.GetComponent()==Step);
        TestEqual(TEXT("Each real camp entrance step selects timber"),HearthwardFootstepSurface::Resolve(StepHit),FName(TEXT("wood")));
    }
    auto* Settings=UPhysicsSettings::Get();const auto Saved=Settings->PhysicalSurfaces;
    ON_SCOPE_EXIT {Settings->PhysicalSurfaces=Saved;};
    Settings->PhysicalSurfaces.RemoveAll([](const auto& Entry){return Entry.Type==SurfaceType62;});
    Floor->SetPhysMaterialOverride(Physical);
    for(const FName Surface:{FName(TEXT("grass")),FName(TEXT("dirt")),FName(TEXT("stone")),FName(TEXT("wood")),FName(TEXT("unmapped"))})
    {
        Settings->PhysicalSurfaces.RemoveAll([](const auto& Entry){return Entry.Type==SurfaceType62;});
        FPhysicalSurfaceName Entry;Entry.Type=SurfaceType62;Entry.Name=Surface;Settings->PhysicalSurfaces.Add(Entry);
        if(!TestTrue(TEXT("Physical-material test still uses an actual collision query"),Trace(Hit)))return false;
        if(!TestTrue(TEXT("The hit returns the real physical material"),Hit.PhysMaterial.Get()==Physical))return false;
        TestEqual(TEXT("Explicit physical surface overrides the visible wood floor"),HearthwardFootstepSurface::Resolve(Hit),HearthwardFootstepSurface::Canonical(Surface));
    }
    Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TestFalse(TEXT("Removing ground prevents a fabricated material hit"),Trace(Hit));
    return true;
}

struct FSurfaceFootstepAccess
{
    static int32 Last(const UHearthwardPresentationComponent* P,FName Event)
    {const auto* Cue=P->SoundCues.Find(Event);return Cue?Cue->LastVariant:INDEX_NONE;}
    static int32 Sources(const UHearthwardPresentationComponent* P){return P->Effects.Num();}
    static void Remove(UHearthwardPresentationComponent* P,FName Event){P->SoundCues.Remove(Event);}
    static void MissingFile(UHearthwardPresentationComponent* P,FName Event)
    {auto& Cue=P->SoundCues.FindChecked(Event);Cue.Variants.Reset();Cue.File=TEXT("TASK-104/intentionally-missing-test.wav");}
    static void Restore(UHearthwardPresentationComponent* P){P->Restored();}
};

namespace
{
struct FSurfacePlaybackWorld104
{
    UGameInstance* Instance;UWorld* World;ACharacter* Player;UHearthwardPresentationComponent* Presentation;
    FSurfacePlaybackWorld104()
    {
        Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();World=Instance->GetWorld();
        Player=World->SpawnActor<ACharacter>();
        auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
        auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
        Presentation=NewObject<UHearthwardPresentationComponent>(Player);Player->AddInstanceComponent(Presentation);Presentation->RegisterComponent();
        World->InitializeActorsForPlay(FURL(),false);World->GetWorldSettings()->NotifyBeginPlay();
    }
    ~FSurfacePlaybackWorld104()
    {
        if(World->HasBegunPlay())World->EndPlay(EEndPlayReason::Quit);
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    // Consumer contract fixture; actual notify/grounding is exercised by Task099.FootContact.
    FHearthwardFootContactReceipt Receipt(FName Surface)
    {
        FHearthwardFootContactReceipt R;R.Source=Player;R.SuccessId=FGuid::NewGuid();R.Frame=GFrameCounter;
        R.Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();R.FootBone=TEXT("foot_l");
        R.Position=Player->GetActorLocation();R.Surface=Surface;return R;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurfaceFootstepPlayback104Test,"Hearthward.Iteration.Task104.Footstep.PlaybackFallbackAndRestore",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSurfaceFootstepPlayback104Test::RunTest(const FString&)
{
    FSurfacePlaybackWorld104 F;
    for(const FName Surface:{FName(TEXT("grass")),FName(TEXT("dirt")),FName(TEXT("stone")),FName(TEXT("wood"))})
    {
        const FName Event=HearthwardFootstepSurface::EventFor(Surface);int32 Previous=INDEX_NONE;
        for(int32 Step=0;Step<8;++Step)
        {
            const auto R=F.Receipt(Surface);const int32 Before=FSurfaceFootstepAccess::Sources(F.Presentation);
            if(!TestTrue(TEXT("Current grounded receipt contract loads its actual surface WAV"),F.Presentation->FootContactSucceeded(R)))return false;
            const int32 Selected=FSurfaceFootstepAccess::Last(F.Presentation,Event);
            TestTrue(TEXT("Live playback selection uses valid, nonrepeating variants"),Selected>=0 && Selected<3 && Selected!=Previous);
            TestEqual(TEXT("Exactly one source is created for each accepted receipt"),FSurfaceFootstepAccess::Sources(F.Presentation),Before+1);
            TestFalse(TEXT("Same-frame receipt duplicate cannot play again"),F.Presentation->FootContactSucceeded(R));Previous=Selected;
        }
    }
    FSurfaceFootstepAccess::Remove(F.Presentation,TEXT("movement.footstep.wood"));
    TestTrue(TEXT("Missing specialized cue falls back to a real neutral WAV"),F.Presentation->FootContactSucceeded(F.Receipt(TEXT("wood"))));
    TestTrue(TEXT("Neutral cue records successful fallback variant"),FSurfaceFootstepAccess::Last(F.Presentation,TEXT("movement.footstep"))>=0);
    FSurfaceFootstepAccess::MissingFile(F.Presentation,TEXT("movement.footstep.stone"));
    AddExpectedError(TEXT("Invalid PCM16 event sound:"),EAutomationExpectedErrorFlags::Contains,1);
    const int32 BeforeFallback=FSurfaceFootstepAccess::Sources(F.Presentation);
    TestTrue(TEXT("Missing specialized file falls back instead of silencing the contact"),F.Presentation->FootContactSucceeded(F.Receipt(TEXT("stone"))));
    TestEqual(TEXT("Failed specialized load adds no ghost source"),FSurfaceFootstepAccess::Sources(F.Presentation),BeforeFallback+1);
    auto Paused=F.Receipt(TEXT("grass"));F.World->GetWorldSettings()->SetPauserPlayerState(F.World->SpawnActor<APlayerState>());
    const int32 BeforePause=FSurfaceFootstepAccess::Sources(F.Presentation);
    TestFalse(TEXT("Paused contact stays silent"),F.Presentation->FootContactSucceeded(Paused));
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    TestFalse(TEXT("Paused receipt cannot replay when resumed"),F.Presentation->FootContactSucceeded(Paused));
    TestEqual(TEXT("Pause/resume added no source"),FSurfaceFootstepAccess::Sources(F.Presentation),BeforePause);
    auto Stale=F.Receipt(TEXT("grass"));Stale.Epoch=FGuid::NewGuid();
    TestFalse(TEXT("A foreign timeline cannot choose a footstep variant"),F.Presentation->FootContactSucceeded(Stale));
    FSurfaceFootstepAccess::Restore(F.Presentation);
    TestEqual(TEXT("Restore removes every pre-restore effect"),FSurfaceFootstepAccess::Sources(F.Presentation),0);
    TestEqual(TEXT("Restore clears specialized no-repeat state"),FSurfaceFootstepAccess::Last(F.Presentation,TEXT("movement.footstep.grass")),INDEX_NONE);
    TestEqual(TEXT("Restore clears fallback no-repeat state"),FSurfaceFootstepAccess::Last(F.Presentation,TEXT("movement.footstep")),INDEX_NONE);
    TestTrue(TEXT("Fresh receipt can play after restore"),F.Presentation->FootContactSucceeded(F.Receipt(TEXT("grass"))));
    return true;
}
#endif
