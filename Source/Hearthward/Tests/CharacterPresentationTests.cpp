#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardProjectile.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Animation/HearthwardBrotherAnimInstance.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS
#if WITH_EDITOR
// Isolated PIE visual fixture; never registered in Shipping or saved into a map.
static FAutoConsoleCommandWithWorld FArcherPreview095Command(
    TEXT("Hearthward.Test095.ArcherPreview"),TEXT("Spawn a production archer for the isolated TASK-095 visual review."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        auto* Player=UGameplayStatics::GetPlayerPawn(World,0);if(!Player)return;
        auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();Campaign->State.Phase=NAME_None;
        auto* Record=Campaign->State.Enemies.FindByPredicate([](const auto& Entry){return Entry.Kind==TEXT("archer");});if(!Record)return;
        Record->Home=Player->GetActorLocation()-FVector(1000,0,0);Record->Combat.Position=Record->Home;Record->Located=true;
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Archer=World->SpawnActor<AHearthwardCampaignActor>(Record->Home,FRotator::ZeroRotator,Params);
        Archer->Initialize(Record->Id,true);Archer->Tags.Add(TEXT("Task095ArcherPreview"));Archer->Target->Exposure=1;
        Archer->Target->Memory.Seen.Add(TEXT("player"));
    }));
#endif
struct FArcherShotTestAccess
{
    static float Pending(const AHearthwardCampaignActor* Actor){return Actor->BowRemaining;}
    static void Ready(AHearthwardCampaignActor* Actor){Actor->AttackIn=0;Actor->DecisionIn=0;}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcherShotLifecycle095Test,
    "Hearthward.Iteration.Task095.ArcherWindupReleaseAndInterrupt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcherShotLifecycle095Test::RunTest(const FString&)
{
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Player=World->SpawnActor<ACharacter>(FVector(1000,0,80),FRotator::ZeroRotator,Params);
    auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();
    auto* Record=Campaign->State.Enemies.FindByPredicate([](const auto& Entry){return Entry.Kind==TEXT("archer");});
    if(!TestNotNull(TEXT("Production campaign includes an archer"),Record))
    {GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return false;}
    Record->Home=FVector(0,0,80);Record->Combat.Position=Record->Home;Record->Located=true;
    auto* Archer=World->SpawnActor<AHearthwardCampaignActor>(Record->Home,FRotator::ZeroRotator,Params);
    Archer->Initialize(Record->Id,true);Archer->Target->Memory.Seen.Add(TEXT("player"));
    auto* Mesh=Archer->GetMesh();
    TestEqual(TEXT("Real campaign selects the specialized archer mesh"),Mesh->GetSkeletalMeshAsset()->GetName(),FString(TEXT("SK_Archer_Combat")));
    TestTrue(TEXT("Equipment does not change body height"),FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().Z,160./99.74417,.001));
    auto* Anim=Cast<UHearthwardBrotherAnimInstance>(Mesh->GetAnimInstance());
    TestTrue(TEXT("Campaign binds the actual shoot clip"),Anim && Anim->Clips[4] && Anim->Clips[4]->GetName()==TEXT("A_Archer_Shoot"));
    const auto Count=[&]()
    {int32 N=0;for(TActorIterator<AHearthwardProjectile> It(World);It;++It)if(!It->IsActorBeingDestroyed())++N;return N;};
    Archer->Tick(.01f);
    TestTrue(TEXT("Attack starts a visible 0.6-second windup"),FMath::IsNearlyEqual(FArcherShotTestAccess::Pending(Archer),.6f));
    TestEqual(TEXT("Windup does not spawn an immediate arrow"),Count(),0);
    Archer->Tick(.3f);TestEqual(TEXT("Half windup still has no arrow"),Count(),0);
    Archer->Tick(.3f);TestEqual(TEXT("Release spawns exactly one arrow"),Count(),1);
    float Power=0;
    for(TActorIterator<AHearthwardProjectile> It(World);It;++It)
    {
        TestTrue(TEXT("Arrow keeps calibrated speed"),FMath::IsNearlyEqual(It->Velocity.Size(),3000.,.01));
        TestEqual(TEXT("Arrow keeps range"),It->RemainingRange,3000.);
        TestTrue(TEXT("Arrow is emitted by the actual campaign actor"),It->EnemyShooter.Get()==Archer);
        Power=It->Power;
    }
    TestTrue(TEXT("Existing calibrated damage remains positive"),Power>0);
    Archer->Tick(.1f);TestEqual(TEXT("Subsequent ticks cannot duplicate the release"),Count(),1);
    FArcherShotTestAccess::Ready(Archer);Archer->Tick(.01f);
    Archer->Target->Memory.HitRemaining=1;Archer->Tick(.1f);
    TestEqual(TEXT("Hit interruption cancels the pending arrow"),FArcherShotTestAccess::Pending(Archer),0.f);
    Archer->Target->Memory.HitRemaining=0;Archer->Tick(.7f);
    TestEqual(TEXT("Cancelled shot never appears late"),Count(),1);
    FArcherShotTestAccess::Ready(Archer);Archer->Tick(.01f);
    World->GetSubsystem<UHearthwardStorageSubsystem>()->AdvanceTimeline();Archer->Tick(.6f);
    TestEqual(TEXT("A load/timeline change cancels the pending arrow"),Count(),1);
    auto* PreviewOwner=World->SpawnActor<AActor>();
    auto* Preview=NewObject<USkeletalMeshComponent>(PreviewOwner);PreviewOwner->AddInstanceComponent(Preview);PreviewOwner->SetRootComponent(Preview);
    Preview->SetSkeletalMesh(Mesh->GetSkeletalMeshAsset());Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);Preview->RegisterComponent();
    Preview->SetAnimationMode(EAnimationMode::AnimationSingleNode);Preview->SetAnimation(Anim->Clips[4]);
    const auto Sample=[&](float Time)
    {Preview->SetPosition(Time,false);Preview->TickAnimation(0,false);Preview->RefreshBoneTransforms();};
    Sample(.59f);
    const FVector Grip=Preview->GetSocketLocation(TEXT("BowGrip")),Nock=Preview->GetSocketLocation(TEXT("BowNock"));
    TestTrue(TEXT("Imported drawing pose lifts bow to chest height in centimetres"),Grip.Z>75 && Grip.Z<85 && Grip.X>25 && Grip.X<35);
    TestTrue(TEXT("Imported string draws back toward the character"),Grip.X-Nock.X>27 && Grip.X-Nock.X<30);
    TestTrue(TEXT("Nocked arrow is visible immediately before release"),Preview->GetSocketTransform(TEXT("NockedArrow"),RTS_Component).GetScale3D().GetMin()>.5);
    Sample(.64f);
    TestTrue(TEXT("Held arrow disappears when the real projectile releases"),Preview->GetSocketTransform(TEXT("NockedArrow"),RTS_Component).GetScale3D().GetMax()<.01);
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcherReferencePose095Test,
    "Hearthward.Iteration.Task095.ArcherReferencePose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcherReferencePose095Test::RunTest(const FString&)
{
    auto* Guard=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Hearthward/Campaign/Guard/SK_Guard_Runtime.SK_Guard_Runtime"));
    auto* Archer=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-095/Archer/SK_Archer_Practical.SK_Archer_Practical"));
    if(!TestNotNull(TEXT("Guard loads"),Guard) || !TestNotNull(TEXT("Archer loads"),Archer))return false;
    TestTrue(TEXT("Archer reuses Guard skeleton"),Archer->GetSkeleton()==Guard->GetSkeleton());
    const auto& Expected=Guard->GetRefSkeleton();
    const auto& Actual=Archer->GetRefSkeleton();
    if(!TestEqual(TEXT("Reference bone count"),Actual.GetNum(),Expected.GetNum()))return false;
    for(int32 Index=0;Index<Expected.GetNum();++Index)
    {
        const FString Name=Expected.GetBoneName(Index).ToString();
        TestEqual(Name+TEXT(" name"),Actual.GetBoneName(Index),Expected.GetBoneName(Index));
        TestEqual(Name+TEXT(" parent"),Actual.GetParentIndex(Index),Expected.GetParentIndex(Index));
        TestTrue(Name+TEXT(" reference transform"),Actual.GetRefBonePose()[Index].Equals(Expected.GetRefBonePose()[Index],.01));
    }
    TestTrue(TEXT("Archer feet remain at ground height"),FMath::Abs(Archer->GetBounds().Origin.Z-Archer->GetBounds().BoxExtent.Z)<.01);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcheryProjectilePresentation095Test,
    "Hearthward.Iteration.Task095.ArrowPresentationKeepsTrajectory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcheryProjectilePresentation095Test::RunTest(const FString&)
{
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Owner=World->SpawnActor<AActor>();
    auto* Root=NewObject<USceneComponent>(Owner);
    Owner->AddInstanceComponent(Root);Owner->SetRootComponent(Root);Root->RegisterComponent();
    auto* Shooter=NewObject<UHearthwardCombatComponent>(Owner);
    Owner->AddInstanceComponent(Shooter);Shooter->RegisterComponent();
    const auto Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    const FVector InitialVelocity(900,200,100);
    const auto Spawn=[&](FName Item,FVector Position)
    {
        auto* Projectile=World->SpawnActor<AHearthwardProjectile>(Position,FRotator::ZeroRotator);
        Projectile->Shooter=Shooter;Projectile->Epoch=Epoch;Projectile->Item=Item;
        Projectile->Velocity=InitialVelocity;Projectile->Gravity=980;
        Projectile->Lifetime=2;Projectile->RemainingRange=10000;Projectile->Power=17;
        return Projectile;
    };
    const FVector Offset(0,200,0);
    auto* Arrow=Spawn(TEXT("arrow"),FVector(0,0,500));
    auto* Stone=Spawn(TEXT("stone"),FVector(0,0,500)+Offset);
    Arrow->Tick(.1f);Stone->Tick(.1f);
    auto* ArrowMesh=CastChecked<UStaticMeshComponent>(Arrow->GetRootComponent());
    auto* StoneMesh=CastChecked<UStaticMeshComponent>(Stone->GetRootComponent());
    const FString ArrowPath=TEXT("/Game/Hearthward/Assets/TASK-095/Archery/Arrow/SM_Arrow_Practical.SM_Arrow_Practical");
    TestNotNull(TEXT("Authored arrow mesh loads"),ArrowMesh->GetStaticMesh().Get());
    if(ArrowMesh->GetStaticMesh())
    {
        TestEqual(TEXT("Player arrow uses the authored asset"),ArrowMesh->GetStaticMesh()->GetPathName(),ArrowPath);
        const auto Bounds=ArrowMesh->GetStaticMesh()->GetBoundingBox();
        TestTrue(TEXT("Arrow tip stays at the existing trace origin"),FMath::Abs(Bounds.Max.X)<.01);
        TestTrue(TEXT("Arrow length is in centimetres"),Bounds.GetSize().X>79 && Bounds.GetSize().X<81);
    }
    TestEqual(TEXT("Stone retains its existing visual"),StoneMesh->GetStaticMesh()->GetPathName(),FString(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    TestTrue(TEXT("Stone retains its existing scale"),StoneMesh->GetRelativeScale3D().Equals(FVector(.06),.0001));
    TestTrue(TEXT("Visual replacement preserves the integrated trajectory"),(Stone->GetActorLocation()-Offset).Equals(Arrow->GetActorLocation(),.001));
    TestTrue(TEXT("Gravity updates are unchanged"),Stone->Velocity.Equals(Arrow->Velocity,.001));
    TestEqual(TEXT("Remaining range is unchanged"),Stone->RemainingRange,Arrow->RemainingRange);
    TestEqual(TEXT("Lifetime is unchanged"),Stone->Lifetime,Arrow->Lifetime);
    TestEqual(TEXT("Damage input is unchanged"),Arrow->Power,17.f);
    TestTrue(TEXT("Arrow points along its actual velocity"),FVector::DotProduct(Arrow->GetActorForwardVector(),Arrow->Velocity.GetSafeNormal())>.999);
    TestTrue(TEXT("Mesh does not introduce a second collision path"),ArrowMesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision);

    auto* EnemyArrow=Spawn(NAME_None,FVector(0,400,500));
    EnemyArrow->Shooter.Reset();EnemyArrow->EnemyShooter=Owner;EnemyArrow->Tick(.01f);
    TestEqual(TEXT("Enemy arrows also use the authored asset"),
        CastChecked<UStaticMeshComponent>(EnemyArrow->GetRootComponent())->GetStaticMesh()->GetPathName(),ArrowPath);
    auto* Restored=Spawn(TEXT("arrow"),FVector(0,600,500));
    const FRotator SavedRotation(15,35,5);
    Restored->SetActorRotation(SavedRotation);Restored->Landed=true;Restored->Velocity=FVector::ZeroVector;
    Restored->Tick(.1f);
    TestEqual(TEXT("Restored landed arrow receives the correct visual"),
        CastChecked<UStaticMeshComponent>(Restored->GetRootComponent())->GetStaticMesh()->GetPathName(),ArrowPath);
    TestTrue(TEXT("Restored landed arrow keeps its saved rotation"),Restored->GetActorRotation().Equals(SavedRotation,.001));
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return true;
}
#endif
