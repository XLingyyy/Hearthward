#include "../Experience/HearthwardPresentationComponent.h"
#include "../Experience/HearthwardPlayerSettings.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Camp/HearthwardCampSubsystem.h"
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
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundWaveProcedural.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
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
    TArray<UAudioComponent*> Sources() const
    {
        TArray<UAudioComponent*> Result;Player->GetComponents(Result);
        return Result.FilterByPredicate([](const auto* C){return IsValid(C) && C->IsRegistered() && C->ComponentTags.Contains(TEXT("Hearthward.Audio.environment"));});
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
#endif
