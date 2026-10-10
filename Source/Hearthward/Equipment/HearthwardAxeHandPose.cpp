#include "HearthwardAxeHandPose.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatComponent.h"

// Generated from rig-verified offline fits in art_source/TASK-104/grip/hand_pose_profiles.json.
// Local rotations use UE parent-bone axes; original translation and scale are never replaced.
const TArray<FName>& HearthwardAxeHandPose::FingerNames()
{
    static const TArray<FName> Names = {TEXT("index_01_r"), TEXT("index_02_r"), TEXT("index_03_r"), TEXT("middle_01_r"), TEXT("middle_02_r"), TEXT("middle_03_r"), TEXT("pinky_01_r"), TEXT("pinky_02_r"), TEXT("pinky_03_r"), TEXT("ring_01_r"), TEXT("ring_02_r"), TEXT("ring_03_r"), TEXT("thumb_01_r"), TEXT("thumb_02_r"), TEXT("thumb_03_r")};
    return Names;
}

bool HearthwardAxeHandPose::Profile(const USkeletalMesh* Mesh, FVector& PalmInHand, TArray<FQuat>& Rotations)
{
    Rotations.Reset();
    if (!Mesh) return false;
    if (Mesh->GetFName() == TEXT("SK_Hero"))
    {
        PalmInHand = FVector(0.00596340931919, -0.042241222474, 0.029131772948);
        Rotations = {
            FQuat(-0.0971899737792, -0.420898769891, -0.567031988398, 0.70133662291), // index_01_r
            FQuat(0.797446227156, 0.1134729402, -0.0602705273294, -0.589551414359), // index_02_r
            FQuat(-0.40081910587, 0.0334937664224, 0.0949106889273, 0.910611977248), // index_03_r
            FQuat(-0.0498367130244, 0.022746501878, -0.484665416313, 0.872982435629), // middle_01_r
            FQuat(-0.530163598925, -0.336832915214, -0.115379091094, 0.769517908136), // middle_02_r
            FQuat(-0.306434758686, 0.466725589608, -0.0597304483984, 0.827464341351), // middle_03_r
            FQuat(-0.627210744523, -0.18839641901, -0.277519566547, 0.702919882663), // pinky_01_r
            FQuat(0.140006595489, 0.134524122163, -0.338609954448, 0.920676225676), // pinky_02_r
            FQuat(-0.0494512605386, -0.00425052063871, 0.308662793353, 0.949875668656), // pinky_03_r
            FQuat(-0.340453058897, -0.422704751608, -0.20080828605, 0.8155295457), // ring_01_r
            FQuat(-0.513656042491, -0.167237424117, 0.422803222837, 0.727617034398), // ring_02_r
            FQuat(0.129889251216, 0.164070021674, 0.304847738883, 0.929127368289), // ring_03_r
            FQuat(-0.512886629199, 0.620261371997, -0.294265685787, 0.515393870905), // thumb_01_r
            FQuat(0.0233041077151, -0.3210273009, 0.377110721699, 0.868438768262), // thumb_02_r
            FQuat(-0.33976327643, -0.379028425875, -0.103078734459, 0.854560204357), // thumb_03_r
        };
        return true;
    }
    if (Mesh->GetFName() == TEXT("SK_Brother"))
    {
        PalmInHand = FVector(0.00296019854456, -0.0439406599455, 0.0266863985411);
        Rotations = {
            FQuat(0.179796405942, 0.472954419302, 0.624507107114, -0.594960706969), // index_01_r
            FQuat(0.770833237961, 0.0822334047278, -0.0284006909738, -0.631068290405), // index_02_r
            FQuat(-0.386066080116, 0.133509896162, 0.0686540033791, 0.910172905129), // index_03_r
            FQuat(-0.0692994717161, -0.0123747012688, -0.583843508128, 0.808808511333), // middle_01_r
            FQuat(-0.457541575145, -0.270989331831, -0.111596753631, 0.839503813944), // middle_02_r
            FQuat(-0.29826660861, 0.522883951192, -0.0596751083173, 0.796284048077), // middle_03_r
            FQuat(-0.572510586081, -0.260800073561, -0.433011740607, 0.645535268558), // pinky_01_r
            FQuat(0.0219445809423, 0.191938094669, -0.499531532093, 0.844479988885), // pinky_02_r
            FQuat(0.0241040158201, -0.237851290684, 0.181354259312, 0.95391634464), // pinky_03_r
            FQuat(-0.4163250064, -0.431652836209, -0.0976419216162, 0.794238864059), // ring_01_r
            FQuat(-0.508667983293, -0.185610982881, 0.473355480879, 0.694794958623), // ring_02_r
            FQuat(0.465318129675, 0.215720711277, 0.326123960249, 0.794094941095), // ring_03_r
            FQuat(-0.53285563781, 0.624359792875, -0.477126185716, 0.313990957194), // thumb_01_r
            FQuat(0.0960967694892, -0.0842048205837, 0.562331879464, 0.81698091558), // thumb_02_r
            FQuat(-0.578806439786, -0.465169696353, 0.0866442513334, 0.664148351325), // thumb_03_r
        };
        return true;
    }
    return false;
}

bool HearthwardAxeHandPose::ShouldApply(const AActor* Owner)
{
    if (!Owner) return false;
    const auto* Bag = Owner->FindComponentByClass<UHearthwardInventoryComponent>();
    const auto* Item = Bag ? Bag->FindInstance(Bag->EquippedInstance(TEXT("weapon"))) : nullptr;
    if (!Item || Item->Definition != TEXT("axe") || Item->Durability <= 0) return false;
    const auto* Survival = Owner->FindComponentByClass<UHearthwardSurvivalComponent>();
    if (Survival && ((!Survival->Alive() && Survival->Enabled()) || Survival->RescueElapsed() >= 0)) return false;
    const auto* Combat = Owner->FindComponentByClass<UHearthwardCombatComponent>();
    if (Combat && Combat->RangedSelected()) return false;
    TArray<UStaticMeshComponent*> Components;
    Owner->GetComponents(Components);
    for (const auto* Component : Components)
        if (Component->IsVisible() && Component->GetAttachSocketName() == TEXT("hand_r")
            && Component->GetStaticMesh()
            && Component->GetStaticMesh()->GetFName() == TEXT("SM_stone_bone_axe")) return true;
    return false;
}

void HearthwardAxeHandPose::ApplyRotation(FTransform& Bone, const FQuat& Rotation, bool Enabled)
{
    if (Enabled) Bone.SetRotation(Rotation.GetNormalized());
}

FHearthwardAxeFingerPoseNode::FHearthwardAxeFingerPoseNode()
{
    for (const FName Name : HearthwardAxeHandPose::FingerNames())
    {
        FBoneReference Bone;
        Bone.BoneName = Name;
        Bones.Add(Bone);
    }
}

void FHearthwardAxeFingerPoseNode::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
    Source.Initialize(Context);
}

void FHearthwardAxeFingerPoseNode::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
    Source.CacheBones(Context);
    for (auto& Bone : Bones) Bone.Initialize(Context.AnimInstanceProxy->GetRequiredBones());
}

void FHearthwardAxeFingerPoseNode::Update_AnyThread(const FAnimationUpdateContext& Context)
{
    Source.Update(Context);
}

void FHearthwardAxeFingerPoseNode::Evaluate_AnyThread(FPoseContext& Output)
{
    Source.Evaluate(Output);
    if (!Enabled || Rotations.Num() != Bones.Num()) return;
    const auto& Required = Output.Pose.GetBoneContainer();
    // An incomplete LOD/skeleton keeps its original pose rather than half a grip.
    for (const auto& Bone : Bones) if (!Bone.IsValidToEvaluate(Required)) return;
    for (int32 I = 0; I < Bones.Num(); ++I)
        HearthwardAxeHandPose::ApplyRotation(Output.Pose[Bones[I].GetCompactPoseIndex(Required)], Rotations[I], true);
}
