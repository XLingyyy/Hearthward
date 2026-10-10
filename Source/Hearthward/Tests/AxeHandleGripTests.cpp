#include "../Equipment/HearthwardAxeGrip.h"
#include "../Equipment/HearthwardAxeHandPose.h"
#include "../HearthwardCharacter.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Animation/HearthwardHeroAnimInstance.h"
#include "../Animation/HearthwardBrotherAnimInstance.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "StaticMeshResources.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAxeAbsoluteScaleGripTest,
    "Hearthward.Iteration.Task104.Grip.AbsoluteScale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAxeAbsoluteScaleGripTest::RunTest(const FString&)
{
    const FVector MeshGrip(22.647987604141235, -17.993159770965576, -27.5);
    const FVector Palm(-0.02, -0.03, 0.01);
    for (const double Scale : {1.0, 100.0 * 180.0 / 97.869893, 100.0 * 160.0 / 97.863766})
    {
        const FVector ParentScale(Scale);
        const FQuat RelativeRotation = FRotator(27, -63, 41).Quaternion();
        const FTransform Relative = HearthwardAxeGrip::AlignHandle(MeshGrip, RelativeRotation, Palm, ParentScale);
        const FTransform Hand(FRotator(-37, 102, 18), FVector(1400, -620, 170), ParentScale);
        // USceneComponent absolute-scale semantics: composed location/rotation,
        // but the child's world scale remains .7 instead of .7 * parent scale.
        FTransform World = Relative * Hand;
        World.SetScale3D(FVector(HearthwardAxeGrip::WorldScale));
        TestTrue(TEXT("Measured handle point meets palm after parent rotation and scale"),
            World.TransformPosition(MeshGrip).Equals(Hand.TransformPosition(Palm), 0.0001));
        TestTrue(TEXT("Axe world size remains unchanged"), World.GetScale3D().Equals(FVector(0.7)));
        const FTransform OldWorld(Hand.GetRotation() * FRotator(0, 0, -90).Quaternion(), Hand.GetLocation(), FVector(0.7));
        TestTrue(TEXT("Old origin-at-wrist attachment misses the palm"),
            FVector::Dist(OldWorld.TransformPosition(MeshGrip), Hand.TransformPosition(Palm)) > 10);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAxeProductionGripTest,
    "Hearthward.Iteration.Task104.Grip.ProductionBothCharacters",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAxeProductionGripTest::RunTest(const FString&)
{
    auto* Axe = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.SM_stone_bone_axe"));
    if (!TestNotNull(TEXT("Production axe loads"), Axe)) return false;
    const auto* Grip = Axe->FindSocket(TEXT("Grip"));
    if (!TestNotNull(TEXT("Production axe retains measured Grip socket"), Grip)) return false;
    TestTrue(TEXT("Grip agrees with production LOD0 measurement"), Grip->RelativeLocation.Equals(
        FVector(22.647987604141235, -17.993159770965576, -27.5), 0.001));
    const auto* RenderData = Axe->GetRenderData();
    if (!TestTrue(TEXT("Production axe has render geometry"), RenderData && RenderData->LODResources.Num() > 0)) return false;
    const auto& Positions = RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
    double MaxCoreRadius = 0;
    int32 CoreVertices = 0;
    for (uint32 I = 0; I < Positions.GetNumVertices(); ++I)
    {
        const FVector P(Positions.VertexPosition(I));
        const FVector FromGrip = P - Grip->RelativeLocation;
        const double Along = FVector::DotProduct(FromGrip, HearthwardAxeGrip::HandleAxis());
        if (FMath::Abs(Along) < 9.9 && P.Z < -12)
        {
            MaxCoreRadius = FMath::Max(MaxCoreRadius,
                (FromGrip - HearthwardAxeGrip::HandleAxis() * Along).Length() * HearthwardAxeGrip::WorldScale);
            ++CoreVertices;
        }
    }
    TestTrue(TEXT("Production render mesh contains the measured grasp region"), CoreVertices > 100);
    TestTrue(TEXT("TASK-104 corrected handle is present, not the old oversized binary"), MaxCoreRadius <= .9001);
    FTransform Invalid;
    TestFalse(TEXT("Missing character fails explicitly"), HearthwardAxeGrip::Build(nullptr, FVector::OneVector, Axe, Invalid));
    TestFalse(TEXT("Missing mesh fails explicitly"), HearthwardAxeGrip::Build(nullptr, FVector::OneVector, nullptr, Invalid));
    auto* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world exists"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Hero = World->SpawnActor<AHearthwardCharacter>(FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    auto* Brother = World->SpawnActor<AHearthwardCompanionFixture>(FVector(0, 500, 200), FRotator::ZeroRotator, Spawn);
    if (Hero && Brother)
    {
        Brother->Bag->TryAdd(TEXT("axe"), 1);
        Brother->Bag->EquipInstance(Brother->Bag->FirstInstance(TEXT("axe")));
        Brother->RefreshHeldWeapon();
        TestTrue(TEXT("Equipped visible companion axe enables grip"), HearthwardAxeHandPose::ShouldApply(Brother));
        for (auto* Character : {static_cast<ACharacter*>(Hero), static_cast<ACharacter*>(Brother)})
        {
            FTransform Expected;
            auto* Mesh = Character->GetMesh();
            if (!TestTrue(TEXT("Production skeleton produces a grip"), HearthwardAxeGrip::Build(
                Mesh->GetSkeletalMeshAsset(), Mesh->GetRelativeScale3D(), Axe, Expected))) continue;
            TestFalse(TEXT("Degenerate parent scale fails explicitly"), HearthwardAxeGrip::Build(
                Mesh->GetSkeletalMeshAsset(), FVector::ZeroVector, Axe, Invalid));
            TArray<UStaticMeshComponent*> Components;
            Character->GetComponents(Components);
            const auto* Found = Components.FindByPredicate([&](const auto* C) { return C->GetStaticMesh() == Axe; });
            if (TestNotNull(TEXT("Actor uses production axe"), Found ? *Found : nullptr))
            {
                TestTrue(TEXT("Both actors consume the shared grip"), (*Found)->GetRelativeTransform().Equals(Expected, 0.0001));
                TestTrue(TEXT("Weapon scale is absolute"), (*Found)->IsUsingAbsoluteScale());
                TestEqual(TEXT("Axe follows the hand bone"), (*Found)->GetAttachSocketName(), FName(TEXT("hand_r")));
                const auto& Ref = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
                const auto Bone = [&](FName Name)
                {
                    int32 I = Ref.FindBoneIndex(Name);
                    FTransform Transform = Ref.GetRefBonePose()[I];
                    while ((I = Ref.GetParentIndex(I)) != INDEX_NONE) Transform *= Ref.GetRefBonePose()[I];
                    return Transform;
                };
                FVector Palm;
                TArray<FQuat> FingerRotations;
                TestTrue(TEXT("Verified per-rig palm profile exists"), HearthwardAxeHandPose::Profile(Mesh->GetSkeletalMeshAsset(), Palm, FingerRotations));
                const FVector KnuckleAxisInHand = Bone(TEXT("hand_r")).GetRotation().UnrotateVector(
                    (Bone(TEXT("index_01_r")).GetLocation() - Bone(TEXT("pinky_01_r")).GetLocation()).GetSafeNormal());
                TestTrue(TEXT("Measured diagonal shaft follows the knuckle line"),
                    Expected.GetRotation().RotateVector(HearthwardAxeGrip::HandleAxis()).Equals(KnuckleAxisInHand, .0001));
                Character->SetActorLocationAndRotation(FVector(800, -500, 200), FRotator(0, 73, 0));
                const bool IsHero = Character == Hero;
                for (const TCHAR* Pose : {TEXT("Idle"), TEXT("Walk"), TEXT("Attack")})
                {
                    const FString Path = IsHero
                        ? FString::Printf(TEXT("/Game/Characters/Hero/AnimationV2/A_Hero_%s"), Pose)
                        : FString::Printf(TEXT("/Game/Characters/Brother/Animation/A_Brother_%s"), Pose);
                    auto* Clip = LoadObject<UAnimSequence>(nullptr, *Path);
                    if (!TestNotNull(*Path, Clip)) continue;
                    Mesh->PlayAnimation(Clip, false);
                    Mesh->SetPlayRate(0);
                    for (const float Phase : {0.f, .5f, 1.f})
                    {
                        Mesh->SetPosition(Clip->GetPlayLength() * Phase, false);
                        Mesh->TickAnimation(0, false);
                        Mesh->RefreshBoneTransforms();
                        (*Found)->UpdateComponentToWorld();
                        const FVector Actual = (*Found)->GetComponentTransform().TransformPosition(Grip->RelativeLocation);
                        const FVector Target = Mesh->GetSocketTransform(TEXT("hand_r")).TransformPosition(Palm);
                        TestTrue(FString::Printf(TEXT("%s phase %.1f keeps handle at reference palm"), *Path, Phase),
                            Actual.Equals(Target, .05));
                        TestTrue(TEXT("Animated attachment preserves axe world dimensions"),
                            (*Found)->GetComponentScale().Equals(FVector(.7), .0001));
                    }
                }
            }
        }
        Brother->Bag->TryAdd(TEXT("shortblade"), 1);
        Brother->Bag->EquipInstance(Brother->Bag->FirstInstance(TEXT("shortblade")));
        Brother->RefreshHeldWeapon();
        TestFalse(TEXT("Switching to another weapon disables axe fingers"), HearthwardAxeHandPose::ShouldApply(Brother));
    }
    else AddError(TEXT("Production characters failed to spawn"));
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAxeFingerScopeTest,
    "Hearthward.Iteration.Task104.Grip.FingerOnlyScope",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAxeFingerScopeTest::RunTest(const FString&)
{
    TestEqual(TEXT("Exactly fifteen finger bones"), HearthwardAxeHandPose::FingerNames().Num(), 15);
    TestFalse(TEXT("Hand/wrist is never overridden"), HearthwardAxeHandPose::FingerNames().Contains(TEXT("hand_r")));
    const FTransform Original(FRotator(18, 29, -37), FVector(.01, -.03, .005), FVector(1.0, .99, 1.01));
    FTransform Disabled = Original;
    HearthwardAxeHandPose::ApplyRotation(Disabled, FQuat::Identity, false);
    TestTrue(TEXT("Disabled grip leaves the complete original transform unchanged"), Disabled.Equals(Original, 0));
    FTransform Enabled = Original;
    HearthwardAxeHandPose::ApplyRotation(Enabled, FQuat::Identity, true);
    TestTrue(TEXT("Grip preserves finger translation"), Enabled.GetTranslation().Equals(Original.GetTranslation(), 0));
    TestTrue(TEXT("Grip preserves finger scale"), Enabled.GetScale3D().Equals(Original.GetScale3D(), 0));
    TestTrue(TEXT("Grip replaces only local rotation"), Enabled.GetRotation().Equals(FQuat::Identity));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAxeRealAnimationGraphTest,
    "Hearthward.Iteration.Task104.Grip.RealAnimationGraphs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAxeRealAnimationGraphTest::RunTest(const FString&)
{
    auto* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Graph test world exists"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Hero = World->SpawnActor<AHearthwardCharacter>(FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    auto* Brother = World->SpawnActor<AHearthwardCompanionFixture>(FVector(0, 500, 200), FRotator::ZeroRotator, Spawn);
    for (auto* Character : {static_cast<ACharacter*>(Hero), static_cast<ACharacter*>(Brother)})
    {
        if (!TestNotNull(TEXT("Production graph owner spawned"), Character)) continue;
        auto* Mesh = Character->GetMesh();
        auto* Bag = Character->FindComponentByClass<UHearthwardInventoryComponent>();
        if (!TestNotNull(TEXT("Production inventory exists"), Bag)) continue;
        if (!TestNotNull(TEXT("Production AnimInstance exists"), Mesh->GetAnimInstance())) continue;
        TestTrue(TEXT("Real production AnimInstance remains active"),
            Character == Hero ? Mesh->GetAnimInstance()->IsA<UHearthwardHeroAnimInstance>()
                : Mesh->GetAnimInstance()->IsA<UHearthwardBrotherAnimInstance>());
        const auto Evaluate = [&]()
        {
            // Zero time holds the base clip phase fixed while exercising normal
            // NativeUpdateAnimation / PreUpdate / custom-root evaluation.
            for (int32 I = 0; I < 2; ++I) { Mesh->TickAnimation(0, false); Mesh->RefreshBoneTransforms(); }
        };
        Evaluate();
        const auto Base = Mesh->GetBoneSpaceTransforms();
        Bag->TryAdd(TEXT("axe"), 1);
        const FGuid AxeId = Bag->FirstInstance(TEXT("axe"));
        Bag->EquipInstance(AxeId);
        if (Character == Brother) Brother->RefreshHeldWeapon();
        TArray<UStaticMeshComponent*> Held;
        Character->GetComponents(Held);
        UStaticMeshComponent* Axe = nullptr;
        for (auto* C : Held) if (C->GetStaticMesh() && C->GetStaticMesh()->GetFName() == TEXT("SM_stone_bone_axe")) Axe = C;
        if (!TestNotNull(TEXT("Production axe component exists"), Axe)) continue;
        Axe->SetVisibility(true);
        Evaluate();
        TestTrue(TEXT("Actual graph gate sees the durable visible equipped axe"), HearthwardAxeHandPose::ShouldApply(Character));
        FVector Palm;
        TArray<FQuat> Targets;
        if (!TestTrue(TEXT("Rig profile exists"), HearthwardAxeHandPose::Profile(Mesh->GetSkeletalMeshAsset(), Palm, Targets))) continue;
        const auto Active = Mesh->GetBoneSpaceTransforms();
        const auto& Ref = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
        for (int32 I = 0; I < Ref.GetNum(); ++I)
        {
            const int32 Finger = HearthwardAxeHandPose::FingerNames().Find(Ref.GetBoneName(I));
            if (Finger == INDEX_NONE)
                TestTrue(TEXT("Active real graph preserves every non-finger bone including wrist and arm"), Active[I].Equals(Base[I], .0001));
            else
            {
                TestTrue(TEXT("Real graph applies each authored local finger rotation"), Active[I].GetRotation().Equals(Targets[Finger], .0001));
                TestTrue(TEXT("Real graph retains original finger translation and scale"),
                    Active[I].GetTranslation().Equals(Base[I].GetTranslation(), .0001)
                    && Active[I].GetScale3D().Equals(Base[I].GetScale3D(), .0001));
            }
        }
        const auto ExpectBase = [&](const TCHAR* Reason)
        {
            Evaluate();
            TestFalse(Reason, HearthwardAxeHandPose::ShouldApply(Character));
            const auto Restored = Mesh->GetBoneSpaceTransforms();
            for (int32 I = 0; I < Base.Num(); ++I) TestTrue(Reason, Restored[I].Equals(Base[I], .0001));
        };
        Axe->SetVisibility(false);
        TestFalse(TEXT("Hidden axe cannot enable finger pose"), HearthwardAxeHandPose::ShouldApply(Character));
        Axe->SetVisibility(true);
        const auto EquippedSnapshot = Bag->Snapshot();
        Bag->RemoveInstance(AxeId);
        ExpectBase(TEXT("Unequipping restores unmodified real graph pose"));
        Bag->RestoreInventory(EquippedSnapshot);
        Bag->WearInstance(AxeId, 10000);
        ExpectBase(TEXT("Broken axe restores unmodified real graph pose"));
        Bag->RestoreInventory(EquippedSnapshot);
        Bag->TryAdd(TEXT("shortblade"), 1);
        Bag->EquipInstance(Bag->FirstInstance(TEXT("shortblade")));
        ExpectBase(TEXT("Other weapon preserves original real graph pose"));
        Bag->RestoreInventory(EquippedSnapshot);
        if (auto* Combat = Character->FindComponentByClass<UHearthwardCombatComponent>())
        {
            Combat->SelectRanged(true);
            ExpectBase(TEXT("Ranged selection disables axe pose"));
            Combat->SelectRanged(false);
        }
    }
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif
