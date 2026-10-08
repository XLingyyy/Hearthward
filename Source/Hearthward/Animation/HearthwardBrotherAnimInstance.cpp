#include "HearthwardBrotherAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_LayeredBoneBlend.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
struct FBrotherActionBlend : FAnimNode_TwoWayBlend
{
    FBrotherActionBlend()
    {
        bResetChildOnActivation = true;
        AlphaInputType = EAnimAlphaInputType::Bool;
        AlphaBoolBlend.BlendInTime = 0.15f;
        AlphaBoolBlend.BlendOutTime = 0.15f;
    }
};

struct FBrotherAnimProxy : FAnimInstanceProxy
{
    FAnimNode_SequencePlayer_Standalone Players[6];
    FAnimNode_TwoWayBlend Gait, Locomotion;
    FBrotherActionBlend Wait, Work, Attack;
    FAnimNode_SequencePlayer_Standalone WeaponCarry;
    FAnimNode_LayeredBoneBlend CarryLayer;
    FAnimNode_SequencePlayer_Standalone LifePlayers[3];
    FBrotherActionBlend LifeLayers[3];
    uint32 SeenAttack = 0;

    explicit FBrotherAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    virtual void Initialize(UAnimInstance* Instance) override
    {
        const auto* Brother = CastChecked<UHearthwardBrotherAnimInstance>(Instance);
        for (int32 Index = 0; Index < 6; ++Index)
        {
            Players[Index].SetSequence(Brother->Clips[Index]);
            Players[Index].SetLoopAnimation(Index != 4);
        }
        for (int32 Index : {1, 2})
        {
            Players[Index].SetGroupName(TEXT("Locomotion"));
            Players[Index].SetGroupMethod(EAnimSyncMethod::SyncGroup);
        }
        Gait.A.SetLinkNode(&Players[1]); Gait.B.SetLinkNode(&Players[2]);
        Locomotion.A.SetLinkNode(&Players[0]); Locomotion.B.SetLinkNode(&Gait);
        Wait.A.SetLinkNode(&Locomotion); Wait.B.SetLinkNode(&Players[5]);
        WeaponCarry.SetSequence(Brother->WeaponCarryClip);WeaponCarry.SetLoopAnimation(true);
        CarryLayer.BasePose.SetLinkNode(&Wait);CarryLayer.AddPose();CarryLayer.BlendPoses[0].SetLinkNode(&WeaponCarry);
        FBranchFilter Arm;Arm.BoneName=TEXT("upperarm_r");Arm.BlendDepth=1;
        CarryLayer.LayerSetup[0].BranchFilters.Add(Arm);CarryLayer.BlendWeights[0]=0;
        Work.A.SetLinkNode(&CarryLayer); Work.B.SetLinkNode(&Players[3]);
        Attack.A.SetLinkNode(&Work); Attack.B.SetLinkNode(&Players[4]);
        for(int32 I=0;I<3;++I)
        {
            LifePlayers[I].SetSequence(Brother->LifeClips[I]);LifePlayers[I].SetLoopAnimation(false);LifePlayers[I].SetPlayRate(0);
            LifeLayers[I].A.SetLinkNode(I==0?&Attack:&LifeLayers[I-1]);LifeLayers[I].B.SetLinkNode(&LifePlayers[I]);
        }
        FAnimInstanceProxy::Initialize(Instance);
    }
    virtual FAnimNode_Base* GetCustomRootNode() override { return &LifeLayers[2]; }
    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* Brother = CastChecked<UHearthwardBrotherAnimInstance>(Instance);
        CarryLayer.BlendWeights[0]=Brother->WeaponCarryWeight;
        for(int32 I=0;I<3;++I)
        {
            LifeLayers[I].bAlphaBoolEnabled=Brother->LifeState==I+1;
            LifePlayers[I].SetStartPosition(Brother->LifePoseTime);LifePlayers[I].SetAccumulatedTime(Brother->LifePoseTime);
        }
        for(int32 I=0;I<6;++I)if(Players[I].GetSequence()!=Brother->Clips[I])Players[I].SetSequence(Brother->Clips[I]);
        Locomotion.Alpha = FMath::Clamp(Brother->GroundSpeed / 40.f, 0.f, 1.f);
        Gait.Alpha = FMath::Clamp((Brother->GroundSpeed - 220.f) / 100.f, 0.f, 1.f);
        Players[1].SetPlayRate(FMath::Clamp(Brother->GroundSpeed / 180.f, 0.25f, 1.6f));
        Players[2].SetPlayRate(FMath::Clamp(Brother->GroundSpeed / 360.f, 0.4f, 1.6f));
        Wait.bAlphaBoolEnabled = Brother->MotionState == TEXT("Wait");
        Work.bAlphaBoolEnabled = Brother->MotionState == TEXT("Dig");
        Attack.bAlphaBoolEnabled = Brother->MotionState == TEXT("Attack");
        if (SeenAttack != Brother->AttackRevision)
        {
            Players[4].SetAccumulatedTime(0.f);
            SeenAttack = Brother->AttackRevision;
        }
    }
};
}

UHearthwardBrotherAnimInstance::UHearthwardBrotherAnimInstance()
{
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Carry(TEXT("/Game/Hearthward/Assets/TASK-095/Weapons/A_Brother_WeaponCarry"));
    WeaponCarryClip=Carry.Object;
    for (const TCHAR* Name : {TEXT("Idle"), TEXT("Walk"), TEXT("Run"), TEXT("Dig"), TEXT("Attack"), TEXT("Wait")})
    {
        const FString Path = FString::Printf(TEXT("/Game/Characters/Brother/Animation/A_Brother_%s"), Name);
        ConstructorHelpers::FObjectFinder<UAnimSequence> Clip(*Path);
        Clips.Add(Clip.Object);
    }
    for(const TCHAR* Name:{TEXT("Down"),TEXT("GetUp"),TEXT("Rescue")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-095/Survival/A_Brother_%s"),Name);
        ConstructorHelpers::FObjectFinder<UAnimSequence> Clip(*Path);LifeClips.Add(Clip.Object);
    }
}

void UHearthwardBrotherAnimInstance::PlayAttack()
{
    AttackRemaining = Clips[4]->GetPlayLength();
    ++AttackRevision;
}

void UHearthwardBrotherAnimInstance::CancelAttack()
{
    AttackRemaining=0;
    MotionState=GroundSpeed<5.f?TEXT("Idle"):TEXT("Walk");
}

void UHearthwardBrotherAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    auto* Brother = Cast<AHearthwardCompanionFixture>(TryGetPawnOwner());
    if (!TryGetPawnOwner()) return;
    GroundSpeed = TryGetPawnOwner()->GetVelocity().Size2D();
    AttackRemaining = FMath::Max(0.f, AttackRemaining - DeltaSeconds);
    MotionState = GroundSpeed < 5.f ? TEXT("Idle") : GroundSpeed > 270.f ? TEXT("Run") : TEXT("Walk");
    if (Brother && GroundSpeed < 5.f)
    {
        const auto* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        const auto* Gameplay = Player ? Player->FindComponentByClass<UHearthwardGameplayComponent>() : nullptr;
        if (Gameplay && Gameplay->CompanionOrder == TEXT("wait")) MotionState = TEXT("Wait");
        if (Brother->Action && Brother->Action->GetStatus() == EHearthwardTimedActionStatus::Running)
            MotionState = TEXT("Dig");
    }
    if (AttackRemaining > 0.f) MotionState = TEXT("Attack");
    LifeState=0;
    const auto* Survival=TryGetPawnOwner()->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(Survival && Survival->Enabled() && !Survival->Alive())
    {
        const double Rise=Survival->AssistedRiseElapsed();
        DownElapsed=Rise>=0?1.2f:FMath::Min(1.2f,DownElapsed+DeltaSeconds);
        LifeState=Rise>=0?2:1;LifePoseTime=Rise>=0?float(Rise):DownElapsed;
        MotionState=Rise>=0?TEXT("GetUp"):TEXT("Down");AttackRemaining=0;GroundSpeed=0;
    }
    else
    {
        DownElapsed=0;
        if(Survival && Survival->RescueElapsed()>=0)
        {LifeState=3;LifePoseTime=float(Survival->RescueElapsed());MotionState=TEXT("Rescue");AttackRemaining=0;}
    }
    bool Carry=false;
    if(Brother)
    {
        Brother->RefreshHeldWeapon();
        const auto* Equipped=Brother->Bag->FindInstance(Brother->Bag->EquippedInstance(TEXT("weapon")));
        Carry=Equipped && Equipped->Durability>0 && Equipped->Definition!=TEXT("axe");
    }
    WeaponCarryWeight=FMath::FInterpTo(WeaponCarryWeight,Carry?1.f:0.f,DeltaSeconds,10.f);
}

FAnimInstanceProxy* UHearthwardBrotherAnimInstance::CreateAnimInstanceProxy() { return new FBrotherAnimProxy(this); }
void UHearthwardBrotherAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
