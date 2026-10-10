#include "../Experience/HearthwardPresentationComponent.h"
#include "../Experience/HearthwardPlayerSettings.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Audio.h"
#include "AudioThread.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundWaveProcedural.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "HAL/PlatformTime.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
const FName FireEvent(TEXT("environment.fire.active"));
const FName WindEvent(TEXT("environment.wind.lookouts"));
const TCHAR* FlameAsset=TEXT("/Game/Hearthward/Assets/TASK-096/Fire/NS_HearthFire.NS_HearthFire");
const TCHAR* SmokeAsset=TEXT("/Game/Hearthward/Assets/TASK-096/Fire/NS_HearthSmoke.NS_HearthSmoke");
const TCHAR* LookoutAsset=TEXT("/Game/Hearthward/Assets/TASK-097/Route/SM_RouteLookout.SM_RouteLookout");

struct FEnvironmentAudioWorld
{
    UGameInstance* Instance=nullptr;
    UWorld* World=nullptr;
    ACharacter* Player=nullptr;
    AHearthwardCompanionFixture* Brother=nullptr;
    AStaticMeshActor* Water=nullptr;
    UStaticMesh* Mesh=nullptr;
    UHearthwardPresentationComponent* Presentation=nullptr;
    UHearthwardGameplayComponent* Gameplay=nullptr;
    UHearthwardSurvivalComponent* Survival=nullptr;
    UHearthwardBuildingComponent* Building=nullptr;
    TArray<FVector> Vertices;
    TArray<FIntVector> Triangles;
    bool Ready=false;
    FString Failure;
    FEnvironmentAudioWorld()
    {
        FString Text;TSharedPtr<FJsonObject> Geometry;
        if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("Resources/Data/TASK-099-water-audio.json")))
            || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Geometry))
        {Failure=TEXT("Actual UE-exported geometry resource is unavailable");return;}
        for(const auto& Value:Geometry->GetArrayField(TEXT("vertices_local_cm")))
        {const auto& P=Value->AsArray();Vertices.Add(FVector(P[0]->AsNumber(),P[1]->AsNumber(),P[2]->AsNumber()));}
        for(const auto& Value:Geometry->GetArrayField(TEXT("triangles")))
        {const auto& P=Value->AsArray();Triangles.Add(FIntVector(P[0]->AsNumber(),P[1]->AsNumber(),P[2]->AsNumber()));}
        Mesh=LoadObject<UStaticMesh>(nullptr,*Geometry->GetStringField(TEXT("mesh")));
        if(!Mesh || Vertices.Num()!=98 || Triangles.Num()!=96){Failure=TEXT("Actual source mesh/98-vertex96-triangle metadata is missing");return;}
        Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();World=Instance->GetWorld();
        const FVector Origin(12340,-54321,200);
        auto* Floor=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);
        Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);Box->SetBoxExtent(FVector(100000,100000,10));
        Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Floor->SetActorLocation(Origin-FVector(0,0,10));
        Player=World->SpawnActor<ACharacter>();Player->SetActorLocation(Origin+FVector(0,0,92));
        auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();
        Gameplay->Enabled=true;Gameplay->CompanionRoutineEnabled=false;
        Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
        auto* Timer=NewObject<UHearthwardTimedActionComponent>(Player);Player->AddInstanceComponent(Timer);Timer->RegisterComponent();
        auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
        auto* Local=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(Local,FPlatformUserId::CreateFromInternalId(0));Controller->SetPlayer(Local);
        auto* Camp=World->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Camp);
        Camp->AddInstanceComponent(Root);Camp->SetRootComponent(Root);Root->RegisterComponent();Camp->SetActorLocation(Origin+FVector(150,0,80));
        auto* Stock=NewObject<UHearthwardInventoryComponent>(Camp);Camp->AddInstanceComponent(Stock);Stock->RegisterComponent();
        FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Brother=World->SpawnActor<AHearthwardCompanionFixture>(Camp->GetActorLocation(),FRotator::ZeroRotator,Spawn);Brother->InitializeCompanion(Stock,Camp);
        World->GetSubsystem<UHearthwardCampSubsystem>()->State.AddCamp(TEXT("camp"),Origin);
        Water=World->SpawnActor<AStaticMeshActor>();Water->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        Water->GetStaticMeshComponent()->SetStaticMesh(Mesh);Water->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("NoCollision"));
        Water->SetActorTransform(FTransform(FRotator(0,25,0),Origin,FVector(.7,1.1,1)));
        Presentation=NewObject<UHearthwardPresentationComponent>(Player);Player->AddInstanceComponent(Presentation);Presentation->RegisterComponent();
        World->InitializeActorsForPlay(FURL(),false);World->BeginPlay();World->GetWorldSettings()->NotifyBeginPlay();World->Tick(LEVELTICK_All,.05f);
        for(int32 I=0;I<80 && (!Player->GetCharacterMovement()->IsMovingOnGround() || !Brother->GetCharacterMovement()->IsMovingOnGround());++I)
        {Player->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);Brother->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);}
        Ready=Player->IsLocallyControlled() && Player->GetCharacterMovement()->IsMovingOnGround() && Brother->GetCharacterMovement()->IsMovingOnGround()
            && Water->HasActorBegunPlay() && Water->GetStaticMeshComponent()->IsRegistered() && Water->GetWorld()==World;
        Failure=FString::Printf(TEXT("Local=%d playerMode=%d brotherMode=%d waterBegin=%d registered=%d"),Player->IsLocallyControlled(),
            int32(Player->GetCharacterMovement()->MovementMode),int32(Brother->GetCharacterMovement()->MovementMode),Water->HasActorBegunPlay(),Water->GetStaticMeshComponent()->IsRegistered());
    }
    ~FEnvironmentAudioWorld()
    {
        if(!World)return;
        if(World->HasBegunPlay())World->EndPlay(EEndPlayReason::Quit);
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    void Observe(){Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);}
    TArray<UAudioComponent*> Sources(FName Event=NAME_None) const
    {
        TArray<UAudioComponent*> Result;Player->GetComponents(Result);
        return Result.FilterByPredicate([Event](const auto* C){return IsValid(C) && C->IsRegistered()
            && C->ComponentTags.Contains(TEXT("Hearthward.Audio.environment")) && (Event.IsNone() || C->ComponentHasTag(Event));});
    }
    AActor* Lookout(FName Tag,FVector Position,UStaticMesh* Asset=nullptr)
    {
        // A live authored mesh/tag association, not a coordinate, fake source or private consumer injection.
        auto* Actor=World->SpawnActor<AActor>();
        auto* Root=NewObject<UStaticMeshComponent>(Actor);Actor->AddInstanceComponent(Root);Actor->SetRootComponent(Root);
        Root->SetMobility(EComponentMobility::Movable);Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Root->SetStaticMesh(Asset?Asset:LoadObject<UStaticMesh>(nullptr,LookoutAsset));
        if(!Tag.IsNone())Actor->Tags.Add(Tag);
        Root->RegisterComponent();Actor->SetActorLocation(Position);return Actor;
    }
    UBoxComponent* Roof()
    {
        auto* Actor=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Actor);
        Actor->AddInstanceComponent(Box);Actor->SetRootComponent(Box);Box->SetBoxExtent(FVector(200,200,20));
        Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();
        Actor->SetActorLocation(Player->GetPawnViewLocation()+FVector(0,0,300));return Box;
    }
    void EnableBuildings()
    {
        Building=NewObject<UHearthwardBuildingComponent>(Player);
        Player->AddInstanceComponent(Building);Building->RegisterComponent();
    }
    void RefreshBuildings(){Building->TickComponent(.3f,LEVELTICK_All,nullptr);}
    AActor* GiftFire(FVector Position)
    {
        if(!Building || !Building->AddGift(TEXT("campfire"),Position))return nullptr;
        const auto Actors=Building->GetBuildings();return Actors.IsEmpty()?nullptr:Actors.Last();
    }
    FVector Closest(FVector Point) const
    {
        FVector Best;double Distance=MAX_dbl;
        const FTransform Transform=Water->GetStaticMeshComponent()->GetComponentTransform();
        for(const auto& T:Triangles)
        {
            const FVector Candidate=FMath::ClosestPointOnTriangleToPoint(Point,Transform.TransformPosition(Vertices[T.X]),Transform.TransformPosition(Vertices[T.Y]),Transform.TransformPosition(Vertices[T.Z]));
            const double D=FVector::DistSquared(Point,Candidate);if(D<Distance){Distance=D;Best=Candidate;}
        }
        return Best;
    }
};

TArray<UNiagaraComponent*> FacilityEffects(AActor* Actor,const TCHAR* Asset)
{
    TArray<UNiagaraComponent*> Effects;Actor->GetComponents(Effects);
    return Effects.FilterByPredicate([Asset](const auto* Effect){return IsValid(Effect) && Effect->GetAsset()
        && Effect->GetAsset()->GetPathName()==Asset && Effect->ComponentHasTag(TEXT("Hearthward.FacilityFire"));});
}

bool RequireRenderedFire(FAutomationTestBase& Test)
{
    // Never claim an inactive NullRHI Niagara component or its light is an active audio producer.
    if(FParse::Param(FCommandLine::Get(),TEXT("NullRHI")) || !FApp::CanEverRender())
    {
        Test.AddError(TEXT("RENDERED_REQUIRED: run this fire test with a rendering RHI; NullRHI cannot establish an active Niagara flame. This is not a passing headless substitute."));
        return false;
    }
    return true;
}

// UE 5.8 API: https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Niagara/UNiagaraSystem
// Poll rather than WaitForCompilationComplete: the latter has no bounded timeout. Latent updates
// leave editor/compiler work running between frames; no forced activation/readiness flags are set.
class FEnvironmentFireReady099Command final : public IAutomationLatentCommand
{
public:
    FEnvironmentFireReady099Command(FAutomationTestBase& InTest,TFunction<bool()> InFixture)
        : Test(InTest),RunFixture(MoveTemp(InFixture)),
          Flame(LoadObject<UNiagaraSystem>(nullptr,FlameAsset)),Smoke(LoadObject<UNiagaraSystem>(nullptr,SmokeAsset)),
          Deadline(FPlatformTime::Seconds()+30.0) {}
    virtual bool Update() override
    {
        if(!Flame.Get() || !Smoke.Get())
        {
            Test.AddError(FString::Printf(TEXT("READINESS_BLOCK: exact Niagara asset preload failed (flame=%d, smoke=%d); rendered fire behavior was NOT_RUN."),
                Flame.Get()!=nullptr,Smoke.Get()!=nullptr));
            return true;
        }
#if WITH_EDITOR
        // true flushes a deferred request to compile; it is not a forced compile or activation flag.
        if(!Flame->IsReadyToRun())Flame->PollForCompilationComplete(true);
        if(!Smoke->IsReadyToRun())Smoke->PollForCompilationComplete(true);
#endif
        const bool FlameReady=Flame->IsReadyToRun(),SmokeReady=Smoke->IsReadyToRun();
        if(FlameReady && SmokeReady)
        {
            Test.AddInfo(TEXT("READINESS_READY: exact flame and smoke systems report IsReadyToRun; beginning actual rendered producer assertions."));
            RunFixture();return true;
        }
        if(FPlatformTime::Seconds()>=Deadline)
        {
            Test.AddError(FString::Printf(TEXT("READINESS_BLOCK: Niagara preload did not become ready within 30 seconds (flameReady=%d, smokeReady=%d); rendered fire behavior was NOT_RUN."),
                FlameReady,SmokeReady));
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase& Test;
    TFunction<bool()> RunFixture;
    TStrongObjectPtr<UNiagaraSystem> Flame,Smoke;
    double Deadline;
};

bool RequireActiveFlame(FAutomationTestBase& Test,AActor* Actor,UNiagaraComponent*& Flame)
{
    if(!Test.TestTrue(TEXT("AddGift produced a real begun completed building"),IsValid(Actor) && Actor->HasActorBegunPlay()
        && Actor->ActorHasTag(TEXT("Hearthward.Building.Completed"))))return false;
    const auto Flames=FacilityEffects(Actor,FlameAsset);
    if(!Test.TestEqual(TEXT("Actual building presentation creates one exact flame Niagara asset"),Flames.Num(),1))return false;
    Flame=Flames[0];
    return Test.TestTrue(TEXT("FIRE_PRODUCER_ACTIVE: after asset readiness, the actual flame is registered, active and visible"),
        Flame->IsRegistered() && Flame->IsActive() && Flame->IsVisible() && !Flame->bHiddenInGame && Flame->GetWorld()==Actor->GetWorld());
}

UHearthwardSaveSubsystem* PrepareAmbientSave(FAutomationTestBase& Test,FEnvironmentAudioWorld& F)
{
    FString Pool;FGuid PoolId;
    if(!Test.TestTrue(TEXT("Ambient actual-load tests require the runner's isolated UUID save pool"),
        FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) && FGuid::Parse(Pool,PoolId)))return nullptr;
    auto* Save=F.World->GetSubsystem<UHearthwardSaveSubsystem>();
    if(!Test.TestTrue(*(TEXT("Real ambient save coordination enables: ")+Save->GetStatus()),Save->EnablePrototype())
        || !Test.TestTrue(*(TEXT("Real ambient campaign starts: ")+Save->GetStatus()),Save->StartNewProgress()))return nullptr;
    return Save;
}

bool SaveAmbientPoint(FAutomationTestBase& Test,UHearthwardSaveSubsystem* Save,FGuid& SavedId)
{
    TSet<FGuid> Before;for(const auto& Point:Save->GetPoints())Before.Add(Point.SaveId);
    if(!Test.TestTrue(*(TEXT("Real ambient manual snapshot persists: ")+Save->GetStatus()),Save->SavePoint(true)))return false;
    const auto Points=Save->GetPoints();const auto* Point=Points.FindByPredicate([&](const auto& P){return !Before.Contains(P.SaveId);});
    if(!Test.TestNotNull(TEXT("Actual ambient snapshot publishes a new identity"),Point))return false;
    SavedId=Point->SaveId;return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvironmentAssociation099Test,"Hearthward.Iteration.Task099.Environment.LoadedMeshAssociationAndTransform",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEnvironmentAssociation099Test::RunTest(const FString&)
{
    FEnvironmentAudioWorld F;if(!TestTrue(*(TEXT("Real local participants and actual water component are ready: ")+F.Failure),F.Ready))return false;
    TestEqual(TEXT("An actual matching mesh without the authored water tag is not a sound source"),F.Sources().Num(),0);
    F.Water->Tags.Add(TEXT("water"));F.Observe();
    bool Passed=TestEqual(TEXT("Actual registered begun matching mesh/tag produces one environment source"),F.Sources().Num(),1);
    if(F.Sources().Num()==1)
    {
        const FVector Nearest=F.Closest(F.Player->GetActorLocation());
        TestTrue(TEXT("Sound uses the closest actual transformed triangle, not saved cache identity"),F.Sources()[0]->GetComponentLocation().Equals(Nearest,.1));
        TestTrue(TEXT("Actual transformed source is outside the origin used by saved unregistered cache"),Nearest.Size()>10000);
    }
    const FIntVector Triangle=F.Triangles[0];
    const FVector InteriorLocal=(F.Vertices[Triangle.X]+F.Vertices[Triangle.Y]+F.Vertices[Triangle.Z])/3.;
    const FVector Interior=F.Water->GetStaticMeshComponent()->GetComponentTransform().TransformPosition(InteriorLocal);
    F.Player->SetActorLocation(Interior+FVector(0,0,92));
    F.Player->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);F.Observe();
    TestTrue(TEXT("Actual transformed triangle interior is beyond the actor-centre hearing boundary"),FVector::Dist(F.Player->GetActorLocation(),F.Water->GetActorLocation())>3000);
    TestTrue(TEXT("The real player capsule remains grounded at the triangle-interior location"),F.Player->GetCharacterMovement()->IsMovingOnGround());
    TestEqual(TEXT("A listener beside actual surface beyond the actor centre retains one loop"),F.Sources().Num(),1);
    if(F.Sources().Num()==1)
    {
        const FVector Nearest=F.Closest(F.Player->GetActorLocation());
        TestTrue(TEXT("The source follows an actual rotated and scaled triangle away from actor origin"),F.Sources()[0]->GetComponentLocation().Equals(Nearest,.1));
        TestTrue(TEXT("Triangle source is clearly distinct from actor-origin placement"),FVector::Dist(Nearest,F.Water->GetActorLocation())>3000);
    }
    const FVector Inside=F.Player->GetActorLocation();F.Player->SetActorLocation(Inside+FVector(0,0,4000));F.Observe();
    TestEqual(TEXT("Moving forty metres above the actual surface removes the distant source"),F.Sources().Num(),0);
    F.Player->SetActorLocation(Inside);F.Observe();TestEqual(TEXT("Approach reconstructs only the current water source"),F.Sources().Num(),1);
    F.Water->SetActorHiddenInGame(true);F.Observe();TestEqual(TEXT("Actual actor hidden state removes the loop"),F.Sources().Num(),0);
    F.Water->SetActorHiddenInGame(false);F.Water->GetStaticMeshComponent()->SetVisibility(false);F.Observe();
    TestEqual(TEXT("An invisible actual mesh cannot remain a loop source"),F.Sources().Num(),0);
    F.Water->GetStaticMeshComponent()->SetVisibility(true);F.Observe();TestEqual(TEXT("Making the same current mesh visible creates one source"),F.Sources().Num(),1);
    F.Water->GetStaticMeshComponent()->UnregisterComponent();F.Observe();
    TestEqual(TEXT("Real component unregistration removes the loaded-source sound"),F.Sources().Num(),0);
    F.Water->GetStaticMeshComponent()->RegisterComponent();F.Observe();TestEqual(TEXT("Real re-registration creates at most one current source"),F.Sources().Num(),1);
    auto* Duplicate=F.World->SpawnActor<AStaticMeshActor>();Duplicate->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);Duplicate->GetStaticMeshComponent()->SetStaticMesh(F.Mesh);Duplicate->Tags.Add(TEXT("water"));
    Duplicate->SetActorTransform(F.Water->GetActorTransform());F.Observe();
    TestEqual(TEXT("Multiple loaded matching candidates do not duplicate the region loop"),F.Sources().Num(),1);
    F.Water->Destroy();Duplicate->Destroy();F.Observe();TestEqual(TEXT("Destroying actual loaded actors leaves no region loop"),F.Sources().Num(),0);
    return Passed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvironmentLoad099Test,"Hearthward.Iteration.Task099.Environment.ActualLoadPauseAndExit",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEnvironmentLoad099Test::RunTest(const FString&)
{
    FString Pool;FGuid PoolId;if(!TestTrue(TEXT("Actual environment load uses the runner's isolated UUID pool"),FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) && FGuid::Parse(Pool,PoolId)))return false;
    FEnvironmentAudioWorld F;if(!TestTrue(*(TEXT("Real grounded save participants are ready: ")+F.Failure),F.Ready))return false;
    F.Water->Tags.Add(TEXT("water"));F.Observe();
    const bool Started=TestEqual(TEXT("The current loaded region starts exactly one real environment component"),F.Sources().Num(),1);
    auto* Pauser=F.World->SpawnActor<APlayerState>();F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);F.Observe();
    TestEqual(TEXT("Actual world pause removes current loop source"),F.Sources().Num(),0);
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);F.Observe();TestEqual(TEXT("Resume reconstructs one current source without queue backlog"),F.Sources().Num(),1);
    auto* Save=F.World->GetSubsystem<UHearthwardSaveSubsystem>();
    if(!TestTrue(*(TEXT("Real save coordination enables: ")+Save->GetStatus()),Save->EnablePrototype())
        || !TestTrue(*(TEXT("A real isolated campaign starts: ")+Save->GetStatus()),Save->StartNewProgress()))return false;
    F.Observe();TSet<FGuid> Before;for(const auto& Point:Save->GetPoints())Before.Add(Point.SaveId);
    if(!TestTrue(*(TEXT("A real manual snapshot persists: ")+Save->GetStatus()),Save->SavePoint(true)))return false;
    const auto Points=Save->GetPoints();const auto* Point=Points.FindByPredicate([&](const auto& P){return !Before.Contains(P.SaveId);});
    if(!TestTrue(TEXT("Real manual snapshot publishes a fresh identity"),Point!=nullptr))return false;
    const FGuid SavedId=Point->SaveId,OldEpoch=F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    UAudioComponent* Previous=F.Sources().Num()==1?F.Sources()[0]:nullptr;
    if(!TestTrue(*(TEXT("Actual LoadPoint restores the saved world: ")+Save->GetStatus()),Save->LoadPoint(SavedId)))return false;
    TestTrue(TEXT("Actual load replaces the timeline identity"),F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()!=OldEpoch);
    TestEqual(TEXT("Actual LoadPoint removes old loop before current observation"),F.Sources().Num(),0);
    TestTrue(TEXT("The previous source is destroyed or unregistered by real restore"),!IsValid(Previous) || !Previous->IsRegistered());
    F.Observe();TestEqual(TEXT("Post-load observation rebuilds only one current loaded-region source"),F.Sources().Num(),1);
    for(int32 I=0;I<8;++I)F.Observe();TestEqual(TEXT("Repeated post-load frames keep source count bounded to one"),F.Sources().Num(),1);
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);TestEqual(TEXT("Real EndPlay cleans a live current source"),F.Sources().Num(),0);
    F.Presentation->BeginPlay();F.Observe();TestEqual(TEXT("BeginPlay rebuilds one current source with no persistent loop history"),F.Sources().Num(),1);
    if(!TestTrue(TEXT("Actual fatal damage establishes the dead-listener boundary"),F.Survival->ReceiveDamage(1,FGuid::NewGuid(),F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(),true)))return false;
    F.Observe();TestEqual(TEXT("Dead listener cannot retain an environment loop"),F.Sources().Num(),0);
    return Started;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvironmentPCM099Test,"Hearthward.Iteration.Task099.Environment.ContinuousPCM",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEnvironmentPCM099Test::RunTest(const FString&)
{
    FEnvironmentAudioWorld F;if(!TestTrue(*(TEXT("Actual water/audio participants are ready: ")+F.Failure),F.Ready))return false;
    F.Water->Tags.Add(TEXT("water"));F.Observe();
    if(!TestEqual(TEXT("Continuous PCM comes from the actual registered environment consumer"),F.Sources().Num(),1))return false;
    UAudioComponent* Source=F.Sources()[0];auto* Sound=Cast<USoundWaveProcedural>(Source->GetSound());
    if(!TestNotNull(TEXT("The actual environment component owns a procedural wave"),Sound))return false;
    TestTrue(TEXT("The actual procedural environment wave is configured to loop"),Sound->bLooping);
    TestEqual(TEXT("The actual mixer source has indefinite duration instead of the finite file length"),Sound->Duration,float(INDEFINITELY_LOOPING_DURATION));
    TArray<uint8> Data;FWaveModInfo Info;
    if(!TestTrue(TEXT("Bound real CC0 runtime file decodes as mono PCM16"),FFileHelper::LoadFileToArray(Data,*(FPaths::ProjectDir()/TEXT("Resources/Audio/TASK-099/candidate-water-west.wav")))
        && Info.ReadWaveInfo(Data.GetData(),Data.Num()) && *Info.pFormatTag==1 && *Info.pBitsPerSample==16 && *Info.pChannels==1))return false;
    // Stop and fence the actual device source before reading its wave directly.
    Source->Stop();FAudioCommandFence Fence;Fence.BeginFence();Fence.Wait();
    const int32 Frames=Info.SampleDataSize/2;TArray<uint8> Out;
    const int32 Generated=Sound->OnGeneratePCMAudio(Out,Frames*3);
    if(!TestEqual(TEXT("Actual loop fills a request across three complete real PCM periods"),Generated,Frames*3))return false;
    TestEqual(TEXT("Output allocation is exactly the requested PCM byte count"),Out.Num(),int32(Info.SampleDataSize)*3);
    if(Out.Num()!=int32(Info.SampleDataSize)*3)return false;
    TestTrue(TEXT("Second and third full periods repeat the first without silent exhaustion"),
        FMemory::Memcmp(Out.GetData(),Out.GetData()+Info.SampleDataSize,Info.SampleDataSize)==0
        && FMemory::Memcmp(Out.GetData(),Out.GetData()+Info.SampleDataSize*2,Info.SampleDataSize)==0);
    TestTrue(TEXT("Actual generated PCM is not placeholder silence"),Out.ContainsByPredicate([](uint8 Value){return Value!=0;}));
    for(int32 I=0;I<32;++I)
    {TArray<uint8> Block;TestEqual(TEXT("Subsequent requests stay continuous and bounded"),Sound->OnGeneratePCMAudio(Block,512),512);TestEqual(TEXT("Small output blocks are not cumulative"),Block.Num(),1024);}
    TestTrue(TEXT("Underlying queue does not grow beyond one actual PCM period"),Sound->GetAvailableAudioByteCount()<=int32(Info.SampleDataSize));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvironmentWindAssociation099Test,"Hearthward.Iteration.Task099.Environment.Wind.LoadedLookoutDistanceAndMix",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEnvironmentWindAssociation099Test::RunTest(const FString&)
{
    FEnvironmentAudioWorld F;if(!TestTrue(*(TEXT("Real local wind listener is ready: ")+F.Failure),F.Ready))return false;
    auto* Settings=F.Instance->GetSubsystem<UHearthwardPlayerSettings>();Settings->Comfort.Environment=100;
    const float Category=Settings->Volume(TEXT("environment"));
    if(!TestTrue(TEXT("Distance-gain verification requires an audible master mix; this fixture does not persist preferences"),Category>0))return false;
    const FVector AnchorPosition=F.Player->GetActorLocation()+FVector(0,1000,0);
    auto* Untagged=F.Lookout(NAME_None,AnchorPosition);
    auto* WrongMesh=F.Lookout(TEXT("CampaignNode:route_ridge"),AnchorPosition,F.Mesh);F.Observe();
    TestEqual(TEXT("Neither a real lookout without a route tag nor a tagged wrong mesh creates wind"),F.Sources(WindEvent).Num(),0);
    auto* Ridge=F.Lookout(TEXT("CampaignNode:route_ridge"),AnchorPosition);
    auto* Root=Cast<UStaticMeshComponent>(Ridge->GetRootComponent());
    if(!TestTrue(TEXT("The actual route lookout mesh is loaded, registered, visible and begun"),Ridge->HasActorBegunPlay()
        && Root && Root->IsRegistered() && Root->IsVisible() && Root->GetStaticMesh() && Root->GetStaticMesh()->GetPathName()==LookoutAsset))return false;
    F.Observe();
    if(!TestEqual(TEXT("A current live ridge lookout creates exactly one local wind source"),F.Sources(WindEvent).Num(),1))return false;
    UAudioComponent* Source=F.Sources(WindEvent)[0];
    TestTrue(TEXT("Lookout wind is a diffuse local bed rather than an extra point-source attenuation"),!Source->bAllowSpatialization);
    TestTrue(TEXT("Inner-zone source follows the current listener"),Source->GetComponentLocation().Equals(F.Player->GetActorLocation(),.1));
    TestTrue(TEXT("Inside 1800 cm the source uses the configured light gain"),FMath::IsNearlyEqual(Source->VolumeMultiplier,Category*.24f,1.e-5f));
    auto* Wave=Cast<USoundWaveProcedural>(Source->GetSound());
    TestTrue(TEXT("Actual lookout wind owns a continuous procedural wave"),Wave && Wave->bLooping && Wave->Duration==float(INDEFINITELY_LOOPING_DURATION));
    TestTrue(TEXT("Wind wave policy remains eligible to play when silent; this is not device-audibility evidence"),Wave && Wave->VirtualizationMode==EVirtualizationMode::PlayWhenSilent);
    F.Player->SetActorLocation(AnchorPosition+FVector(1800,0,0));F.Observe();
    TestTrue(TEXT("The exact inner radius retains full local gain"),F.Sources(WindEvent).Num()==1
        && FMath::IsNearlyEqual(F.Sources(WindEvent)[0]->VolumeMultiplier,Category*.24f,1.e-5f));
    F.Player->SetActorLocation(AnchorPosition+FVector(3900,0,0));F.Observe();
    if(!TestEqual(TEXT("The middle of the outer falloff keeps one current source"),F.Sources(WindEvent).Num(),1))return false;
    TestTrue(TEXT("Mid-falloff smoothstep gives half the inner-zone gain"),FMath::IsNearlyEqual(F.Sources(WindEvent)[0]->VolumeMultiplier,Category*.12f,1.e-5f));
    TestTrue(TEXT("Moving within one live zone updates the source without recreating its PCM cursor"),F.Sources(WindEvent)[0]==Source);
    TestTrue(TEXT("The diffuse source tracks movement instead of staying at the lookout root"),Source->GetComponentLocation().Equals(F.Player->GetActorLocation(),.1));
    F.Player->SetActorLocation(AnchorPosition+FVector(4950,0,0));F.Observe();
    TestTrue(TEXT("Quarter remaining edge weight uses smoothstep rather than a hard global volume"),F.Sources(WindEvent).Num()==1
        && FMath::IsNearlyEqual(F.Sources(WindEvent)[0]->VolumeMultiplier,Category*.24f*.15625f,1.e-5f));
    F.Player->SetActorLocation(AnchorPosition+FVector(6000,0,0));F.Observe();
    TestEqual(TEXT("The exact 6000 cm outer edge has zero wind and no retained source"),F.Sources(WindEvent).Num(),0);
    F.Player->SetActorLocation(AnchorPosition+FVector(6500,0,0));F.Observe();
    TestEqual(TEXT("Outside a lookout zone there is no global wind fallback"),F.Sources(WindEvent).Num(),0);
    F.Player->SetActorLocation(AnchorPosition);F.Observe();
    auto* Watch=F.Lookout(TEXT("CampaignNode:route_watch"),AnchorPosition+FVector(500,0,0));F.Observe();
    if(!TestEqual(TEXT("Overlapping ridge and watch anchors share exactly one wind source"),F.Sources(WindEvent).Num(),1))return false;
    TestTrue(TEXT("Overlapping local zones use the strongest weight without summing gains"),FMath::IsNearlyEqual(F.Sources(WindEvent)[0]->VolumeMultiplier,Category*.24f,1.e-5f));
    const FString GameplayBefore=F.Gameplay->SaveSnapshot();
    const FString CampBefore=F.World->GetSubsystem<UHearthwardCampSubsystem>()->State.Snapshot();
    const FGuid EpochBefore=F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    for(int32 I=0;I<16;++I)F.Observe();
    TestEqual(TEXT("Repeated overlapping-zone observations remain bounded to one loop"),F.Sources(WindEvent).Num(),1);
    TestEqual(TEXT("Wind observation leaves authoritative gameplay untouched"),F.Gameplay->SaveSnapshot(),GameplayBefore);
    TestEqual(TEXT("Wind observation leaves camp economy untouched"),F.World->GetSubsystem<UHearthwardCampSubsystem>()->State.Snapshot(),CampBefore);
    TestEqual(TEXT("Wind observation does not advance the save timeline"),F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(),EpochBefore);
    Settings->Comfort.Environment=25;F.Observe();
    TestTrue(TEXT("Existing local wind responds to the environment channel mix"),F.Sources(WindEvent).Num()==1
        && FMath::IsNearlyEqual(F.Sources(WindEvent)[0]->VolumeMultiplier,Settings->Volume(TEXT("environment"))*.24f,1.e-5f));
    if(!TestEqual(TEXT("The real mix fixture has one current wind component before muting"),F.Sources(WindEvent).Num(),1))return false;
    const TWeakObjectPtr<UAudioComponent> BeforeMute=F.Sources(WindEvent)[0];
    Settings->Comfort.Environment=0;F.Observe();
    TestTrue(TEXT("Muting the actual environment preference keeps one current component at zero gain"),F.Sources(WindEvent).Num()==1
        && F.Sources(WindEvent)[0]==BeforeMute.Get() && FMath::IsNearlyZero(F.Sources(WindEvent)[0]->VolumeMultiplier));
    Settings->Comfort.Environment=100;F.Observe();
    TestTrue(TEXT("Restoring the actual environment preference restores component gain without duplication; no device playback is asserted"),F.Sources(WindEvent).Num()==1
        && F.Sources(WindEvent)[0]==BeforeMute.Get() && FMath::IsNearlyEqual(F.Sources(WindEvent)[0]->VolumeMultiplier,Category*.24f,1.e-5f));
    Ridge->Destroy();F.Observe();TestEqual(TEXT("The remaining live watch anchor preserves a single loop"),F.Sources(WindEvent).Num(),1);
    Watch->Destroy();Untagged->Destroy();WrongMesh->Destroy();F.Observe();
    TestEqual(TEXT("Removing all actual lookout anchors leaves no wind source"),F.Sources(WindEvent).Num(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvironmentWindLifecycle099Test,"Hearthward.Iteration.Task099.Environment.Wind.ShelterPauseSwimmingAndExit",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEnvironmentWindLifecycle099Test::RunTest(const FString&)
{
    FEnvironmentAudioWorld F;if(!TestTrue(*(TEXT("Real local wind lifecycle participants are ready: ")+F.Failure),F.Ready))return false;
    auto* Anchor=F.Lookout(TEXT("CampaignNode:route_watch"),F.Player->GetActorLocation()+FVector(500,0,0));
    auto* Root=Cast<UStaticMeshComponent>(Anchor->GetRootComponent());F.Observe();
    if(!TestEqual(TEXT("Current visible watch anchor begins one wind source"),F.Sources(WindEvent).Num(),1))return false;
    auto* Roof=F.Roof();FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(Task099ActualWindRoof),true,F.Player);
    const FVector Head=F.Player->GetPawnViewLocation();
    if(!TestTrue(TEXT("A real solid UBoxComponent blocks the actual upward visibility trace"),
        F.World->LineTraceSingleByChannel(Hit,Head,Head+FVector(0,0,2500),ECC_Visibility,Query) && Hit.GetComponent()==Roof))return false;
    F.Observe();TestEqual(TEXT("A current solid overhead roof suppresses outdoor wind"),F.Sources(WindEvent).Num(),0);
    Roof->SetCollisionEnabled(ECollisionEnabled::NoCollision);F.Observe();
    TestEqual(TEXT("Removing actual shelter collision restores exactly one wind source"),F.Sources(WindEvent).Num(),1);
    Roof->SetCollisionEnabled(ECollisionEnabled::QueryOnly);F.Observe();
    TestEqual(TEXT("Restoring actual roof collision stops the current source again"),F.Sources(WindEvent).Num(),0);
    Roof->GetOwner()->Destroy();F.Observe();TestEqual(TEXT("Destroying the roof restores the current outdoor bed"),F.Sources(WindEvent).Num(),1);
    // Diagnostic boundary: use the real public movement-mode callback, not a fabricated receipt or a claim of water traversal.
    F.Player->GetCharacterMovement()->SetMovementMode(MOVE_Swimming);
    if(!TestTrue(TEXT("The real movement component enters the swimming mode boundary"),F.Player->GetCharacterMovement()->IsSwimming()))return false;
    F.Observe();TestEqual(TEXT("Swimming removes the local lookout wind source"),F.Sources(WindEvent).Num(),0);
    F.Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);F.Observe();
    TestEqual(TEXT("Leaving the swimming boundary reconstructs one current loop"),F.Sources(WindEvent).Num(),1);
    auto* Pauser=F.World->SpawnActor<APlayerState>();F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);F.Observe();
    TestEqual(TEXT("Actual world pause removes wind"),F.Sources(WindEvent).Num(),0);
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);F.Observe();
    TestEqual(TEXT("Actual world resume restores one local source without replay backlog"),F.Sources(WindEvent).Num(),1);
    Anchor->SetActorHiddenInGame(true);F.Observe();TestEqual(TEXT("A hidden lookout actor cannot sustain wind"),F.Sources(WindEvent).Num(),0);
    Anchor->SetActorHiddenInGame(false);F.Observe();TestEqual(TEXT("Unhiding the current lookout restores one source"),F.Sources(WindEvent).Num(),1);
    Root->SetVisibility(false);F.Observe();TestEqual(TEXT("Invisible real lookout mesh stops wind"),F.Sources(WindEvent).Num(),0);
    Root->SetVisibility(true);Root->SetHiddenInGame(true);F.Observe();TestEqual(TEXT("Hidden-in-game real lookout mesh stops wind"),F.Sources(WindEvent).Num(),0);
    Root->SetHiddenInGame(false);F.Observe();TestEqual(TEXT("Visible root mesh restores current wind"),F.Sources(WindEvent).Num(),1);
    Root->UnregisterComponent();F.Observe();TestEqual(TEXT("Actual lookout-root unregistration removes wind"),F.Sources(WindEvent).Num(),0);
    Root->RegisterComponent();F.Observe();TestEqual(TEXT("Actual lookout-root re-registration restores one source"),F.Sources(WindEvent).Num(),1);
    Anchor->Tags.Remove(TEXT("CampaignNode:route_watch"));F.Observe();TestEqual(TEXT("Removing the required live route tag removes wind"),F.Sources(WindEvent).Num(),0);
    Anchor->Tags.Add(TEXT("CampaignNode:route_watch"));F.Observe();TestEqual(TEXT("Restoring the current anchor's route tag restores one source"),F.Sources(WindEvent).Num(),1);
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);TestEqual(TEXT("Actual presentation EndPlay removes current wind"),F.Sources(WindEvent).Num(),0);
    F.Presentation->BeginPlay();F.Observe();TestEqual(TEXT("A new presentation lifetime observes the current lookout once"),F.Sources(WindEvent).Num(),1);
    Anchor->Destroy();F.Observe();TestEqual(TEXT("Destroying the actual lookout actor leaves no source"),F.Sources(WindEvent).Num(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvironmentWindLoad099Test,"Hearthward.Iteration.Task099.Environment.Wind.ActualLoadAndDeadListener",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEnvironmentWindLoad099Test::RunTest(const FString&)
{
    FEnvironmentAudioWorld F;if(!TestTrue(*(TEXT("Real grounded wind save participants are ready: ")+F.Failure),F.Ready))return false;
    auto* Save=PrepareAmbientSave(*this,F);if(!Save)return false;
    auto* Anchor=F.Lookout(TEXT("CampaignNode:route_ridge"),F.Player->GetActorLocation()+FVector(500,0,0));F.Observe();
    if(!TestEqual(TEXT("Actual-load fixture starts one live lookout source"),F.Sources(WindEvent).Num(),1))return false;
    FGuid SavedId;if(!SaveAmbientPoint(*this,Save,SavedId))return false;
    const FGuid Epoch=F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    const TWeakObjectPtr<UAudioComponent> OldSource=F.Sources(WindEvent)[0];
    if(!TestTrue(*(TEXT("Real LoadPoint restores the wind listener: ")+Save->GetStatus()),Save->LoadPoint(SavedId)))return false;
    TestTrue(TEXT("The real wind load changes the timeline identity"),F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()!=Epoch);
    TestEqual(TEXT("LoadPoint clears old wind before another presentation observation"),F.Sources(WindEvent).Num(),0);
    TestTrue(TEXT("The old wind source is actually destroyed or unregistered"),!OldSource.IsValid() || !OldSource->IsRegistered());
    F.Observe();TestEqual(TEXT("After load the current registered anchor rebuilds one source"),F.Sources(WindEvent).Num(),1);
    for(int32 I=0;I<12;++I)F.Observe();TestEqual(TEXT("Repeated post-load frames never duplicate lookout wind"),F.Sources(WindEvent).Num(),1);
    // The transient anchor is deliberately absent on the next load: no saved coordinate may revive it.
    Anchor->Destroy();F.Observe();
    if(!TestTrue(*(TEXT("A second real LoadPoint succeeds without the lookout: ")+Save->GetStatus()),Save->LoadPoint(SavedId)))return false;
    F.Observe();TestEqual(TEXT("Load cannot synthesize wind for a destroyed live lookout"),F.Sources(WindEvent).Num(),0);
    F.Lookout(TEXT("CampaignNode:route_watch"),F.Player->GetActorLocation()+FVector(500,0,0));F.Observe();
    if(!TestEqual(TEXT("A newly loaded current lookout can still start wind"),F.Sources(WindEvent).Num(),1))return false;
    if(!TestTrue(TEXT("Real fatal damage establishes the dead wind-listener boundary"),F.Survival->ReceiveDamage(1,FGuid::NewGuid(),
        F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(),true)))return false;
    F.Observe();TestEqual(TEXT("A dead listener cannot retain local wind"),F.Sources(WindEvent).Num(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvironmentFireLifecycle099Test,"Hearthward.Iteration.Task099.Environment.Fire.RenderedActualFacilityFlameLifecycle",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEnvironmentFireLifecycle099Test::RunTest(const FString&)
{
    if(!RequireRenderedFire(*this))return false;
    auto RunFixture=[this]() -> bool
    {
        FEnvironmentAudioWorld F;if(!TestTrue(*(TEXT("Real rendered fire participants are ready: ")+F.Failure),F.Ready))return false;
        F.EnableBuildings();
        if(!TestTrue(TEXT("The registered real building producer has begun with its existing action timer"),F.Building->HasBegunPlay()
            && F.Player->FindComponentByClass<UHearthwardTimedActionComponent>()!=nullptr))return false;
        const FVector Listener=F.Player->GetActorLocation();
        auto* Campfire=F.GiftFire(Listener+FVector(-250,0,-100));
        if(!TestNotNull(TEXT("The production AddGift API places an actual campfire on the real floor"),Campfire))return false;
        F.RefreshBuildings();UNiagaraComponent* Flame=nullptr;if(!RequireActiveFlame(*this,Campfire,Flame))return false;
        const auto Smoke=FacilityEffects(Campfire,SmokeAsset);
        if(!TestEqual(TEXT("The same real producer also owns its separate smoke system"),Smoke.Num(),1))return false;
        TestTrue(TEXT("Rendered smoke is genuinely active beside the active flame"),Smoke[0]->IsRegistered() && Smoke[0]->IsActive());
        F.Observe();if(!TestEqual(TEXT("One real flame plus real smoke produces exactly one fire source"),F.Sources(FireEvent).Num(),1))return false;
        auto* Source=F.Sources(FireEvent)[0];auto* Wave=Cast<USoundWaveProcedural>(Source->GetSound());
        TestTrue(TEXT("The source follows the exact active flame component, not its building root or smoke"),Source->GetComponentLocation().Equals(Flame->GetComponentLocation(),.1)
            && !Source->GetComponentLocation().Equals(Smoke[0]->GetComponentLocation(),.1));
        TestTrue(TEXT("The actual fire consumer owns a continuous procedural wave"),Wave && Wave->bLooping && Wave->Duration==float(INDEFINITELY_LOOPING_DURATION));
        TestTrue(TEXT("Fire wave policy remains eligible to play when silent; this is not device-audibility evidence"),Wave && Wave->VirtualizationMode==EVirtualizationMode::PlayWhenSilent);
        TestTrue(TEXT("Actual fire is spatial with the configured finite hearing radius"),Source->bAllowSpatialization && Source->bOverrideAttenuation
            && FMath::IsNearlyEqual(Source->AttenuationOverrides.FalloffDistance,1800.f));
        TestTrue(TEXT("Actual fire obeys the environment channel and configured gain"),FMath::IsNearlyEqual(Source->VolumeMultiplier,
            F.Instance->GetSubsystem<UHearthwardPlayerSettings>()->Volume(TEXT("environment"))*.35f,1.e-5f));
        const FString Before=F.Gameplay->SaveSnapshot();for(int32 I=0;I<12;++I)F.Observe();
        TestEqual(TEXT("Repeated active flame observations never duplicate smoke or fire"),F.Sources(FireEvent).Num(),1);
        TestEqual(TEXT("Observing actual fire does not alter gameplay facts"),F.Gameplay->SaveSnapshot(),Before);
        Flame->SetVisibility(false);F.Observe();TestEqual(TEXT("An invisible actual flame stops its loop"),F.Sources(FireEvent).Num(),0);
        Flame->SetVisibility(true);Flame->SetHiddenInGame(true);F.Observe();TestEqual(TEXT("A hidden-in-game actual flame stops its loop"),F.Sources(FireEvent).Num(),0);
        Flame->SetHiddenInGame(false);F.Observe();TestEqual(TEXT("The visible live flame reconstructs one source"),F.Sources(FireEvent).Num(),1);
        Campfire->SetActorHiddenInGame(true);F.Observe();TestEqual(TEXT("Hiding the actual producer actor removes fire"),F.Sources(FireEvent).Num(),0);
        Campfire->SetActorHiddenInGame(false);F.Observe();TestEqual(TEXT("Unhiding the real producer restores one current source"),F.Sources(FireEvent).Num(),1);
        Flame->DeactivateImmediate();F.Observe();
        TestFalse(TEXT("The actual flame has genuinely deactivated"),Flame->IsActive());
        TestTrue(TEXT("The real smoke remains active while the flame is off"),Smoke[0]->IsActive());
        TestEqual(TEXT("Active smoke alone cannot be a fire-audio source"),F.Sources(FireEvent).Num(),0);
        F.RefreshBuildings();if(!RequireActiveFlame(*this,Campfire,Flame))return false;F.Observe();
        TestEqual(TEXT("The real building presentation tick reactivates one flame source"),F.Sources(FireEvent).Num(),1);
        Flame->UnregisterComponent();F.Observe();TestEqual(TEXT("Unregistering the actual flame removes its loop"),F.Sources(FireEvent).Num(),0);
        Flame->RegisterComponent();F.RefreshBuildings();if(!RequireActiveFlame(*this,Campfire,Flame))return false;F.Observe();
        TestEqual(TEXT("Re-registering and refreshing the genuine flame restores only one loop"),F.Sources(FireEvent).Num(),1);
        Flame->ComponentTags.Remove(TEXT("Hearthward.FacilityFire"));F.Observe();
        TestEqual(TEXT("An exact flame asset without the required current producer tag stays silent"),F.Sources(FireEvent).Num(),0);
        Flame->ComponentTags.Add(TEXT("Hearthward.FacilityFire"));F.Observe();TestEqual(TEXT("Restoring the real producer tag restores one loop"),F.Sources(FireEvent).Num(),1);
        F.Player->SetActorLocation(Flame->GetComponentLocation()+FVector(1800,0,0));F.Observe();
        TestEqual(TEXT("The exact 1800 cm fire boundary retains no audio source"),F.Sources(FireEvent).Num(),0);
        F.Player->SetActorLocation(Flame->GetComponentLocation()+FVector(1900,0,0));F.Observe();
        TestEqual(TEXT("An out-of-range flame cannot keep a loop"),F.Sources(FireEvent).Num(),0);
        F.Player->SetActorLocation(Listener);F.Observe();TestEqual(TEXT("Approaching the same active flame restores one loop"),F.Sources(FireEvent).Num(),1);
        auto* Camp=F.World->GetSubsystem<UHearthwardCampSubsystem>();
        const auto* Facility=Camp->State.Facilities.FindByPredicate([&](const auto& Entry){return F.Building->ResolveFacility(Entry.Id)==Campfire;});
        if(!TestNotNull(TEXT("The actual campfire has a registered economy facility"),Facility))return false;
        const FGuid FacilityId=Facility->Id;
        if(!TestTrue(TEXT("The actual MoveFacility API pauses this in-range safe campfire"),F.Building->MoveFacility(FacilityId,
            F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())))return false;
        TestTrue(TEXT("MoveFacility really changed the existing facility's paused state"),Facility->Paused);
        F.RefreshBuildings();F.Observe();
        TestFalse(TEXT("The real presentation producer deactivates its flame while the facility is being moved"),Flame->IsActive());
        TestEqual(TEXT("An actually paused facility has no fire loop"),F.Sources(FireEvent).Num(),0);
        F.Building->CancelPlacement();F.RefreshBuildings();
        TestFalse(TEXT("CancelPlacement really clears the facility pause"),Facility->Paused);
        if(!RequireActiveFlame(*this,Campfire,Flame))return false;F.Observe();
        TestEqual(TEXT("The actual cancellation/resume path produces one current flame loop"),F.Sources(FireEvent).Num(),1);
        auto* Pauser=F.World->SpawnActor<APlayerState>();F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);F.Observe();
        TestEqual(TEXT("Actual world pause removes the fire source"),F.Sources(FireEvent).Num(),0);
        F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);F.Observe();TestEqual(TEXT("World resume reconstructs only the current active flame"),F.Sources(FireEvent).Num(),1);
        F.Presentation->EndPlay(EEndPlayReason::Destroyed);TestEqual(TEXT("Actual EndPlay destroys a current fire source"),F.Sources(FireEvent).Num(),0);
        F.Presentation->BeginPlay();F.Observe();TestEqual(TEXT("A new presentation lifetime observes the existing active flame once"),F.Sources(FireEvent).Num(),1);
        Flame->DestroyComponent();F.Observe();TestEqual(TEXT("Destroying the actual flame leaves its surviving smoke silent"),F.Sources(FireEvent).Num(),0);
        Campfire->Destroy();F.Observe();TestEqual(TEXT("Destroying the real producer cannot leave a fire loop"),F.Sources(FireEvent).Num(),0);
        return true;
    };
    ADD_LATENT_AUTOMATION_COMMAND(FEnvironmentFireReady099Command(*this,MoveTemp(RunFixture)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvironmentFireLoad099Test,"Hearthward.Iteration.Task099.Environment.Fire.RenderedActualLoadAndBoundedSources",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEnvironmentFireLoad099Test::RunTest(const FString&)
{
    if(!RequireRenderedFire(*this))return false;
    auto RunFixture=[this]() -> bool
    {
        FEnvironmentAudioWorld F;if(!TestTrue(*(TEXT("Real rendered fire save participants are ready: ")+F.Failure),F.Ready))return false;
        F.EnableBuildings();auto* Save=PrepareAmbientSave(*this,F);if(!Save)return false;
        const FVector Listener=F.Player->GetActorLocation();
        const FVector Offsets[]={FVector(-250,0,-100),FVector(-700,0,-100),FVector(-1150,0,-100),FVector(0,-700,-100),FVector(0,-1150,-100)};
        for(const FVector Offset:Offsets)if(!TestNotNull(TEXT("Production AddGift places each non-overlapping real campfire"),F.GiftFire(Listener+Offset)))return false;
        F.RefreshBuildings();TArray<FVector> Flames;
        for(auto* Actor:F.Building->GetBuildings())
        {UNiagaraComponent* Flame=nullptr;if(!RequireActiveFlame(*this,Actor,Flame))return false;Flames.Add(Flame->GetComponentLocation());}
        F.Observe();if(!TestEqual(TEXT("Five real active flames are bounded to the configured four fire sources"),F.Sources(FireEvent).Num(),4))return false;
        Flames.Sort([Listener](const FVector& A,const FVector& B){return FVector::DistSquared(Listener,A)<FVector::DistSquared(Listener,B);});
        const double FarthestAllowed=FVector::DistSquared(Listener,Flames[3]);
        TSet<USoundBase*> Waves;TArray<TWeakObjectPtr<UAudioComponent>> OldSources;
        for(auto* Source:F.Sources(FireEvent))
        {
            Waves.Add(Source->GetSound());OldSources.Add(Source);
            TestTrue(TEXT("Each bounded audio source belongs to an exact current flame position"),Flames.ContainsByPredicate([Source](const FVector& P){return P.Equals(Source->GetComponentLocation(),.1);}));
            TestTrue(TEXT("The bounded set contains the closest current flames"),FVector::DistSquared(Listener,Source->GetComponentLocation())<=FarthestAllowed+.1);
        }
        TestEqual(TEXT("Simultaneous real fire sources never share a procedural PCM cursor"),Waves.Num(),4);
        for(int32 I=0;I<16;++I)F.Observe();TestEqual(TEXT("Repeated five-flame observations keep the active source count bounded"),F.Sources(FireEvent).Num(),4);
        FGuid SavedId;if(!SaveAmbientPoint(*this,Save,SavedId))return false;
        const FGuid Epoch=F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
        const TWeakObjectPtr<AActor> OldBuilding=F.Building->GetBuildings()[0];
        if(!TestTrue(*(TEXT("Real LoadPoint restores saved fire facilities: ")+Save->GetStatus()),Save->LoadPoint(SavedId)))return false;
        TestTrue(TEXT("Actual fire load replaces the timeline identity"),F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()!=Epoch);
        TestEqual(TEXT("Actual LoadPoint clears every previous fire loop before observation"),F.Sources(FireEvent).Num(),0);
        for(const auto& Source:OldSources)TestTrue(TEXT("Real restoration destroys or unregisters each previous fire source"),!Source.IsValid() || !Source->IsRegistered());
        TestTrue(TEXT("Real restoration replaces the original building actor"),!OldBuilding.IsValid() || OldBuilding->IsActorBeingDestroyed());
        TestEqual(TEXT("Saved actual facilities are rebuilt through the existing building restore path"),F.Building->GetBuildings().Num(),5);
        F.Observe();TestEqual(TEXT("Restored facilities alone cannot synthesize loops before their real flame producer runs"),F.Sources(FireEvent).Num(),0);
        F.RefreshBuildings();
        for(auto* Actor:F.Building->GetBuildings()){UNiagaraComponent* Flame=nullptr;if(!RequireActiveFlame(*this,Actor,Flame))return false;}
        F.Observe();TestEqual(TEXT("Current post-load active producers recreate only the bounded four loops"),F.Sources(FireEvent).Num(),4);
        for(int32 I=0;I<16;++I)F.Observe();TestEqual(TEXT("Repeated post-load observations do not duplicate current flames"),F.Sources(FireEvent).Num(),4);
        if(!TestTrue(TEXT("Actual fatal damage establishes the dead fire-listener boundary"),F.Survival->ReceiveDamage(1,FGuid::NewGuid(),
            F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(),true)))return false;
        F.Observe();TestEqual(TEXT("The dead listener retains none of the actual fire loops"),F.Sources(FireEvent).Num(),0);
        return true;
    };
    ADD_LATENT_AUTOMATION_COMMAND(FEnvironmentFireReady099Command(*this,MoveTemp(RunFixture)));
    return true;
}
#endif
