#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardProjectile.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
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
