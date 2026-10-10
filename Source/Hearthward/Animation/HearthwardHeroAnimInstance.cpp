#include "HearthwardHeroAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "../Equipment/HearthwardAxeHandPose.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_LayeredBoneBlend.h"
#include "Animation/AnimNodeSpaceConversions.h"
#include "BoneControllers/AnimNode_ModifyBone.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../HearthwardCharacter.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Interaction/HearthwardInteractionComponent.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "../Interaction/HearthwardHarvestSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
struct FHeroBlend : FAnimNode_TwoWayBlend
{
    FHeroBlend()
    {
        bResetChildOnActivation = true;
        AlphaInputType = EAnimAlphaInputType::Bool;
        AlphaBoolBlend.BlendInTime = 0.12f;
        AlphaBoolBlend.BlendOutTime = 0.12f;
    }
};

struct FHeroAnimProxy : FAnimInstanceProxy
{
    FAnimNode_SequencePlayer_Standalone Players[11];
    FAnimNode_TwoWayBlend Gait, Locomotion;
    FHeroBlend Layers[8];
    FAnimNode_SequencePlayer_Standalone RangedPlayer;
    FAnimNode_LayeredBoneBlend RangedLayer;
    FAnimNode_ConvertLocalToComponentSpace RangedToComponent;
    FAnimNode_ModifyBone AimRotation;
    FAnimNode_ConvertComponentToLocalSpace RangedToLocal;
    FHearthwardAxeFingerPoseNode AxeFingers;
    uint32 SeenAttack = 0;
    float LocomotionPhase = 0;

    explicit FHeroAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

    virtual void Initialize(UAnimInstance* Instance) override
    {
        const auto* Hero = CastChecked<UHearthwardHeroAnimInstance>(Instance);
        for (int32 Index = 0; Index < 11; ++Index)
        {
            Players[Index].SetSequence(Hero->Clips[Index]);
            Players[Index].SetLoopAnimation(Index < 3 || Index == 4 || Index == 7);
        }
        for (int32 Index : {1, 2})
        {
            Players[Index].SetPlayRate(0);
        }
        Gait.A.SetLinkNode(&Players[1]);
        Gait.B.SetLinkNode(&Players[2]);
        Locomotion.A.SetLinkNode(&Players[0]);
        Locomotion.B.SetLinkNode(&Gait);
        RangedPlayer.SetSequence(Hero->Clips[12]);
        RangedPlayer.SetLoopAnimation(false);
        RangedPlayer.SetPlayRate(0);
        RangedToComponent.LocalPose.SetLinkNode(&RangedPlayer);
        AimRotation.ComponentPose.SetLinkNode(&RangedToComponent);
        AimRotation.BoneToModify.BoneName=TEXT("spine_01");
        AimRotation.RotationMode=BMM_Additive;AimRotation.RotationSpace=BCS_ComponentSpace;
        RangedToLocal.ComponentPose.SetLinkNode(&AimRotation);
        RangedLayer.BasePose.SetLinkNode(&Layers[2]);
        RangedLayer.AddPose();
        RangedLayer.BlendPoses[0].SetLinkNode(&RangedToLocal);
        FBranchFilter UpperBody;UpperBody.BoneName=TEXT("spine_01");UpperBody.BlendDepth=1;
        RangedLayer.LayerSetup[0].BranchFilters.Add(UpperBody);
        RangedLayer.bMeshSpaceRotationBlend=true;
        RangedLayer.BlendWeights[0]=0;
        for (int32 Index = 0; Index < 8; ++Index)
        {
            Layers[Index].A.SetLinkNode(Index==0?static_cast<FAnimNode_Base*>(&Locomotion):Index==3?static_cast<FAnimNode_Base*>(&RangedLayer):&Layers[Index-1]);
            Layers[Index].B.SetLinkNode(&Players[Index + 3]);
            Layers[Index].bAlphaBoolEnabled = false;
        }
        AxeFingers.Source.SetLinkNode(&Layers[7]);
        FVector Palm;
        HearthwardAxeHandPose::Profile(Instance->GetSkelMeshComponent()->GetSkeletalMeshAsset(), Palm, AxeFingers.Rotations);
        FAnimInstanceProxy::Initialize(Instance);
    }

    virtual FAnimNode_Base* GetCustomRootNode() override { return &AxeFingers; }

    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        AxeFingers.Enabled = HearthwardAxeHandPose::ShouldApply(Instance->GetOwningActor());
        const auto* Hero = CastChecked<UHearthwardHeroAnimInstance>(Instance);
        RangedLayer.BlendWeights[0]=Hero->RangedWeight;
        AimRotation.Rotation=Hero->RangedAimRotation;
        RangedPlayer.SetSequence(Hero->Clips[Hero->RangedClip]);
        RangedPlayer.SetStartPosition(Hero->RangedPoseTime);
        RangedPlayer.SetAccumulatedTime(Hero->RangedPoseTime);
        Locomotion.Alpha = FMath::Clamp(Hero->GroundSpeed / 80.f, 0.f, 1.f);
        Gait.Alpha = FMath::Clamp((Hero->GroundSpeed - 150.f) / 150.f, 0.f, 1.f);
        // The two-cycle source clips put left-foot contact 0.19 cycles apart.
        // A shared distance phase preserves contact through the walk/run blend.
        const float WalkLength = Hero->Clips[1]->GetPlayLength();
        const float RunLength = Hero->Clips[2]->GetPlayLength();
        const float CycleDistance = FMath::Lerp(110.f * WalkLength, 390.f * RunLength, Gait.Alpha);
        const float CycleRate = Hero->GroundSpeed / CycleDistance;
        Players[1].SetPlayRate(CycleRate * WalkLength);
        Players[2].SetPlayRate(CycleRate * RunLength);
        Players[1].SetStartPosition(LocomotionPhase * WalkLength);
        Players[1].SetAccumulatedTime(LocomotionPhase * WalkLength);
        const float RunTime = FMath::Fmod(LocomotionPhase + .81f, 1.f) * RunLength;
        Players[2].SetStartPosition(RunTime);
        Players[2].SetAccumulatedTime(RunTime);
        LocomotionPhase = FMath::Fmod(LocomotionPhase + CycleRate * DeltaSeconds, 1.f);
        for (int32 Index = 0; Index < 8; ++Index)
            Layers[Index].bAlphaBoolEnabled = Hero->ActionState == Index + 1;
        for(int32 Index=8;Index<11;++Index)
        {
            Players[Index].SetPlayRate(0);
            Players[Index].SetStartPosition(Hero->LifePoseTime);
            Players[Index].SetAccumulatedTime(Hero->LifePoseTime);
        }
        Players[6].SetSequence(Hero->Clips[Hero->IsSpear?11:6]);
        Players[6].SetPlayRate(Hero->IsStoneAxe || Hero->IsSpear ? 0.f : Hero->CombatRate);
        if (Hero->IsStoneAxe || Hero->IsSpear)
        {
            const auto* Combat=Hero->GetOwningActor()->FindComponentByClass<UHearthwardCombatComponent>();
            // Both authored clips use normalized preparation/contact/recovery boundaries at .6/.8/1.
            const float Position=float(HearthwardCombat::StoneAxeClipTime(Combat->Elapsed,Hero->Clips[Hero->IsSpear?11:6]->GetPlayLength(),Hero->IsSpear?Hero->SpearMove:Hero->StoneAxeMove));
            // Child activation initializes from StartPosition after this PreUpdate.
            Players[6].SetStartPosition(Position);
            Players[6].SetAccumulatedTime(Position);
        }
        else
        {
            Players[6].SetStartPosition(0.f);
            if (SeenAttack != Hero->AttackRevision) Players[6].SetAccumulatedTime(0.f);
        }
        SeenAttack = Hero->AttackRevision;
    }
};
}

UHearthwardHeroAnimInstance::UHearthwardHeroAnimInstance()
{
    const TCHAR* Names[] = {TEXT("Idle"), TEXT("Walk"), TEXT("Run"), TEXT("JumpStart"),
        TEXT("Fall"), TEXT("Land"), TEXT("Attack"), TEXT("Dig")};
    for (const TCHAR* Name : Names)
    {
        const FString Path = FString::Printf(TEXT("/Game/Characters/Hero/AnimationV2/A_Hero_%s"), Name);
        ConstructorHelpers::FObjectFinder<UAnimSequence> Clip(*Path);
        Clips.Add(Clip.Object);
    }
    for(const TCHAR* Name:{TEXT("Down"),TEXT("GetUp"),TEXT("Rescue")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-095/Survival/A_Hero_%s"),Name);
        ConstructorHelpers::FObjectFinder<UAnimSequence> Clip(*Path);Clips.Add(Clip.Object);
    }
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Spear(TEXT("/Game/Hearthward/Assets/TASK-095/Weapons/Spear/A_Hero_SpearThrust"));
    Clips.Add(Spear.Object);
    for(const TCHAR* Name:{TEXT("BowDraw"),TEXT("BowRelease"),TEXT("CrossbowAim"),TEXT("CrossbowReload")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-095/PlayerRanged/A_Hero_%s"),Name);
        ConstructorHelpers::FObjectFinder<UAnimSequence> Clip(*Path);Clips.Add(Clip.Object);
    }
}

void UHearthwardHeroAnimInstance::PlayAttack()
{
    if (Clips.IsValidIndex(6) && Clips[6])
    {
        IsStoneAxe=IsSpear=false;
        AttackRemaining = Clips[6]->GetPlayLength();
        ++AttackRevision;
    }
}

void UHearthwardHeroAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const auto* Character = Cast<ACharacter>(TryGetPawnOwner());
    if (!Character) return;
    const auto* Gameplay = Character->FindComponentByClass<UHearthwardGameplayComponent>();
    const bool Dead = Gameplay && Gameplay->Enabled && Gameplay->Health <= 0;
    const auto* RangedCombat=Character->FindComponentByClass<UHearthwardCombatComponent>();
    const bool RangedAction=RangedCombat && (RangedCombat->Action==TEXT("draw") || RangedCombat->Action==TEXT("recoil") || RangedCombat->Action==TEXT("crossbow") || RangedCombat->Action==TEXT("reload"));
    const bool RangedActive=!Dead && RangedCombat && RangedCombat->RangedSelected() && RangedCombat->SupportsAmmo(TEXT("arrow"))
        && ((RangedCombat->Aiming && !RangedCombat->Busy()) || RangedAction);
    RangedWeight=FMath::FInterpConstantTo(RangedWeight,RangedActive?1.f:0.f,DeltaSeconds,8.f);
    if(RangedActive)
    {
        const auto& MeshWorld=Character->GetMesh()->GetComponentTransform();
        RangedAimRotation=FQuat::FindBetweenNormals(MeshWorld.InverseTransformVectorNoScale(Character->GetActorForwardVector()),
            MeshWorld.InverseTransformVectorNoScale(Character->GetControlRotation().Vector())).Rotator();
        const auto Item=HearthwardData::Find(TEXT("items"),Gameplay->Equipment.FindRef(TEXT("ranged")).ToString());
        const bool Bow=HearthwardData::Text(Item,TEXT("combatClass"))==TEXT("bow");
        RangedClip=Bow?12:14;RangedPoseTime=BowDrawTime=0;
        if(Bow && RangedCombat->Action==TEXT("draw"))
        {
            RangedPoseTime=BowDrawTime=FMath::Clamp(float(RangedCombat->Elapsed),0.f,1.f);
            ReleasedDrawTime=BowDrawTime;
        }
        else if(Bow && RangedCombat->Action==TEXT("recoil"))
        {
            RangedClip=13;RangedPoseTime=(1-ReleasedDrawTime)*.35f+float(RangedCombat->Elapsed)*ReleasedDrawTime;
            BowDrawTime=ReleasedDrawTime*FMath::Max(0.f,1-float(RangedCombat->Elapsed)/.08f);
        }
        else if(!Bow && RangedCombat->Action==TEXT("reload"))
        { RangedClip=15;RangedPoseTime=float(RangedCombat->Elapsed); }
        else if(!Bow && RangedCombat->Action==TEXT("crossbow"))RangedPoseTime=float(RangedCombat->Elapsed);
    }
    else BowDrawTime=0;
    if(auto* Hero=Cast<AHearthwardCharacter>(TryGetPawnOwner()))Hero->UpdateRangedVisual(BowDrawTime,RangedWeight);
    GroundSpeed = Dead ? 0.f : Character->GetVelocity().Size2D();
    const bool Falling = Character->GetCharacterMovement()->IsFalling();
    if (bWasFalling && !Falling) LandRemaining = 0.2f;
    bWasFalling = Falling;
    if(IsStoneAxe || IsSpear)
    {
        const auto* Combat=Character->FindComponentByClass<UHearthwardCombatComponent>();
        AttackRemaining=float(FMath::Max(0.,Combat->Duration-Combat->Elapsed));
    }
    else AttackRemaining = FMath::Max(0.f, AttackRemaining - DeltaSeconds);
    LandRemaining = FMath::Max(0.f, LandRemaining - DeltaSeconds);
    ActionState = 0;
    MotionState = GroundSpeed < 5.f ? TEXT("Idle") : GroundSpeed > 400.f ? TEXT("Sprint") : TEXT("Walk");
    const auto* Survival=Character->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(Survival && Survival->Enabled() && !Survival->Alive())
    {
        const double Rise=Survival->AssistedRiseElapsed();
        DownElapsed=Rise>=0?1.2f:FMath::Min(1.2f,DownElapsed+DeltaSeconds);
        ActionState=Rise>=0?7:6;LifePoseTime=Rise>=0?float(Rise):DownElapsed;
        MotionState=Rise>=0?TEXT("GetUp"):TEXT("Down");AttackRemaining=LandRemaining=0;return;
    }
    DownElapsed=0;
    if(Survival && Survival->RescueElapsed()>=0)
    {
        ActionState=8;LifePoseTime=float(Survival->RescueElapsed());MotionState=TEXT("Rescue");AttackRemaining=LandRemaining=0;return;
    }
    if (Dead)
    {
        AttackRemaining = LandRemaining = 0.f;
        MotionState = TEXT("Idle");
        if(IsStoneAxe || IsSpear) for(auto& Layer:GetProxyOnGameThread<FHeroAnimProxy>().Layers) Layer.bAlphaBoolEnabled=false;
        return;
    }
    const auto* Interaction = Character->FindComponentByClass<UHearthwardInteractionComponent>();
    const auto* Resource = Interaction ? Cast<UHearthwardResourceInteractionComponent>(Interaction->GetActiveTarget()) : nullptr;
    if ((Resource && !Resource->CanAccessStorage(const_cast<ACharacter*>(Character)))
        || (Interaction && Cast<UHearthwardHarvestTargetComponent>(Interaction->GetActiveTarget())))
    {
        ActionState = 5;
        MotionState = TEXT("Dig");
    }
    if (AttackRemaining > 0.f) { ActionState = 4; MotionState = IsExecution?TEXT("Execution"):TEXT("Attack"); }
    if (LandRemaining > 0.f) { ActionState = 3; MotionState = TEXT("Land"); }
    if (Falling)
    {
        AttackRemaining = 0.f;
        ActionState = Character->GetVelocity().Z > 0.f ? 1 : 2;
        MotionState = ActionState == 1 ? TEXT("JumpStart") : TEXT("Fall");
    }
    if(IsStoneAxe || IsSpear)
    {
        auto& Proxy=GetProxyOnGameThread<FHeroAnimProxy>();
        for(int32 Index=0;Index<5;++Index) Proxy.Layers[Index].bAlphaBoolEnabled=ActionState==Index+1;
    }
}

FAnimInstanceProxy* UHearthwardHeroAnimInstance::CreateAnimInstanceProxy() { return new FHeroAnimProxy(this); }
void UHearthwardHeroAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }

void UHearthwardHeroAnimInstance::PlayCombat(float Duration,bool Execution,const HearthwardCombat::FMove* InStoneAxeMove)
{
    if(Duration<=0 || !Clips.IsValidIndex(6) || !Clips[6]) return;
    AttackRemaining=Duration; CombatRate=Clips[6]->GetPlayLength()/Duration; IsExecution=Execution; IsStoneAxe=InStoneAxeMove!=nullptr;
    const auto* Gameplay=GetOwningActor()->FindComponentByClass<UHearthwardGameplayComponent>();
    const auto Item=HearthwardData::Find(TEXT("items"),Gameplay->Equipment.FindRef(TEXT("weapon")).ToString());
    IsSpear=!Execution && HearthwardData::Text(Item,TEXT("combatClass"))==TEXT("spear");
    if(IsSpear)SpearMove=HearthwardCombat::Move(TEXT("spear"),Duration>HearthwardCombat::Move(TEXT("spear"),false).Duration()+.001);
    if(InStoneAxeMove)StoneAxeMove=*InStoneAxeMove;
    ++AttackRevision;
}
void UHearthwardHeroAnimInstance::StopCombat() { AttackRemaining=0; CombatRate=1; IsExecution=false; IsStoneAxe=IsSpear=false; }
