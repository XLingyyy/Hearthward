#include "HearthwardHeroAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
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
    FAnimNode_SequencePlayer_Standalone Players[8];
    FAnimNode_TwoWayBlend Gait, Locomotion;
    FHeroBlend Layers[5];
    uint32 SeenAttack = 0;

    explicit FHeroAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

    virtual void Initialize(UAnimInstance* Instance) override
    {
        const auto* Hero = CastChecked<UHearthwardHeroAnimInstance>(Instance);
        for (int32 Index = 0; Index < 8; ++Index)
        {
            Players[Index].SetSequence(Hero->Clips[Index]);
            Players[Index].SetLoopAnimation(Index < 3 || Index == 4 || Index == 7);
        }
        for (int32 Index : {1, 2})
        {
            Players[Index].SetGroupName(TEXT("Locomotion"));
            Players[Index].SetGroupMethod(EAnimSyncMethod::SyncGroup);
        }
        Gait.A.SetLinkNode(&Players[1]);
        Gait.B.SetLinkNode(&Players[2]);
        Locomotion.A.SetLinkNode(&Players[0]);
        Locomotion.B.SetLinkNode(&Gait);
        for (int32 Index = 0; Index < 5; ++Index)
        {
            Layers[Index].A.SetLinkNode(Index == 0 ? static_cast<FAnimNode_Base*>(&Locomotion) : &Layers[Index - 1]);
            Layers[Index].B.SetLinkNode(&Players[Index + 3]);
            Layers[Index].bAlphaBoolEnabled = false;
        }
        FAnimInstanceProxy::Initialize(Instance);
    }

    virtual FAnimNode_Base* GetCustomRootNode() override { return &Layers[4]; }

    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* Hero = CastChecked<UHearthwardHeroAnimInstance>(Instance);
        Locomotion.Alpha = FMath::Clamp(Hero->GroundSpeed / 80.f, 0.f, 1.f);
        Gait.Alpha = FMath::Clamp((Hero->GroundSpeed - 350.f) / 250.f, 0.f, 1.f);
        Players[1].SetPlayRate(FMath::Clamp(Hero->GroundSpeed / 350.f, 0.2f, 1.6f));
        Players[2].SetPlayRate(FMath::Clamp(Hero->GroundSpeed / 600.f, 0.4f, 1.8f));
        for (int32 Index = 0; Index < 5; ++Index)
            Layers[Index].bAlphaBoolEnabled = Hero->ActionState == Index + 1;
        if (SeenAttack != Hero->AttackRevision)
        {
            Players[6].SetAccumulatedTime(0.f);
            SeenAttack = Hero->AttackRevision;
        }
    }
};
}

UHearthwardHeroAnimInstance::UHearthwardHeroAnimInstance()
{
    const TCHAR* Names[] = {TEXT("Idle"), TEXT("Walk"), TEXT("Sprint"), TEXT("JumpStart"),
        TEXT("Fall"), TEXT("Land"), TEXT("Attack"), TEXT("Dig")};
    for (const TCHAR* Name : Names)
    {
        const FString Path = FString::Printf(TEXT("/Game/Characters/Hero/Animation/A_Hero_%s"), Name);
        ConstructorHelpers::FObjectFinder<UAnimSequence> Clip(*Path);
        Clips.Add(Clip.Object);
    }
}

void UHearthwardHeroAnimInstance::PlayAttack()
{
    if (Clips.IsValidIndex(6) && Clips[6])
    {
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
    GroundSpeed = Dead ? 0.f : Character->GetVelocity().Size2D();
    const bool Falling = Character->GetCharacterMovement()->IsFalling();
    if (bWasFalling && !Falling) LandRemaining = 0.2f;
    bWasFalling = Falling;
    AttackRemaining = FMath::Max(0.f, AttackRemaining - DeltaSeconds);
    LandRemaining = FMath::Max(0.f, LandRemaining - DeltaSeconds);
    ActionState = 0;
    MotionState = GroundSpeed < 5.f ? TEXT("Idle") : GroundSpeed > 400.f ? TEXT("Sprint") : TEXT("Walk");
    if (Dead)
    {
        AttackRemaining = LandRemaining = 0.f;
        MotionState = TEXT("Idle");
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
    if (AttackRemaining > 0.f) { ActionState = 4; MotionState = TEXT("Attack"); }
    if (LandRemaining > 0.f) { ActionState = 3; MotionState = TEXT("Land"); }
    if (Falling)
    {
        AttackRemaining = 0.f;
        ActionState = Character->GetVelocity().Z > 0.f ? 1 : 2;
        MotionState = ActionState == 1 ? TEXT("JumpStart") : TEXT("Fall");
    }
}

FAnimInstanceProxy* UHearthwardHeroAnimInstance::CreateAnimInstanceProxy() { return new FHeroAnimProxy(this); }
void UHearthwardHeroAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
