#include "HearthwardBrotherAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
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
        Work.A.SetLinkNode(&Wait); Work.B.SetLinkNode(&Players[3]);
        Attack.A.SetLinkNode(&Work); Attack.B.SetLinkNode(&Players[4]);
        FAnimInstanceProxy::Initialize(Instance);
    }
    virtual FAnimNode_Base* GetCustomRootNode() override { return &Attack; }
    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* Brother = CastChecked<UHearthwardBrotherAnimInstance>(Instance);
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
    for (const TCHAR* Name : {TEXT("Idle"), TEXT("Walk"), TEXT("Run"), TEXT("Dig"), TEXT("Attack"), TEXT("Wait")})
    {
        const FString Path = FString::Printf(TEXT("/Game/Characters/Brother/Animation/A_Brother_%s"), Name);
        ConstructorHelpers::FObjectFinder<UAnimSequence> Clip(*Path);
        Clips.Add(Clip.Object);
    }
}

void UHearthwardBrotherAnimInstance::PlayAttack()
{
    AttackRemaining = Clips[4]->GetPlayLength();
    ++AttackRevision;
}

void UHearthwardBrotherAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const auto* Brother = Cast<AHearthwardCompanionFixture>(TryGetPawnOwner());
    if (!Brother) return;
    GroundSpeed = Brother->GetVelocity().Size2D();
    AttackRemaining = FMath::Max(0.f, AttackRemaining - DeltaSeconds);
    MotionState = GroundSpeed < 5.f ? TEXT("Idle") : GroundSpeed > 270.f ? TEXT("Run") : TEXT("Walk");
    if (GroundSpeed < 5.f)
    {
        const auto* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        const auto* Gameplay = Player ? Player->FindComponentByClass<UHearthwardGameplayComponent>() : nullptr;
        if (Gameplay && Gameplay->CompanionOrder == TEXT("wait")) MotionState = TEXT("Wait");
        if (Brother->Action && Brother->Action->GetStatus() == EHearthwardTimedActionStatus::Running)
            MotionState = TEXT("Dig");
    }
    if (AttackRemaining > 0.f) MotionState = TEXT("Attack");
}

FAnimInstanceProxy* UHearthwardBrotherAnimInstance::CreateAnimInstanceProxy() { return new FBrotherAnimProxy(this); }
void UHearthwardBrotherAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
