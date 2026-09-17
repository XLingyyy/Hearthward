#include "Components/A3GameRuntimeEntityComponent.h"

#include "GameFramework/Actor.h"

UA3GameRuntimeEntityComponent::UA3GameRuntimeEntityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UA3GameRuntimeEntityComponent::SetRuntimeEntityId(
    const FString& InEntityId)
{
    EntityId = InEntityId;
}

bool UA3GameRuntimeEntityComponent::ApplyRuntimeInput(
    const FA3GameRuntimeInputState& InputState)
{
    const bool bMoving =
        !FMath::IsNearlyZero(InputState.MoveX)
        || !FMath::IsNearlyZero(InputState.MoveY);
    if (InputState.bJump)
    {
        LocomotionState = EA3GameLocomotionState::Jump;
        MotionState = TEXT("jump");
    }
    else if (bMoving && InputState.bRun)
    {
        LocomotionState = EA3GameLocomotionState::Run;
        MotionState = TEXT("run");
    }
    else if (bMoving)
    {
        LocomotionState = EA3GameLocomotionState::Walk;
        MotionState = TEXT("walk");
    }
    else
    {
        LocomotionState = EA3GameLocomotionState::Idle;
        MotionState = TEXT("idle");
    }
    LastInputTimeSeconds = InputState.TimestampSeconds;
    OnRuntimeInput.Broadcast(InputState);
    return true;
}

FA3GameEntitySnapshot
UA3GameRuntimeEntityComponent::GetRuntimeSnapshot() const
{
    FA3GameEntitySnapshot Snapshot;
    Snapshot.EntityId = EntityId;
    Snapshot.bPersistent = bPersistent;
    Snapshot.LocomotionState = LocomotionState;
    Snapshot.MotionState = MotionState;
    Snapshot.LastInputTimeSeconds = LastInputTimeSeconds;

    const AActor* Owner = GetOwner();
    if (Owner)
    {
        Snapshot.ActorLabel = Owner->GetName();
        Snapshot.Position = Owner->GetActorLocation();
        Snapshot.Rotation = Owner->GetActorRotation();
    }
    return Snapshot;
}
