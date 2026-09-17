#include "Subsystems/A3GameWorldSessionSubsystem.h"

#include "Components/A3GameRuntimeEntityComponent.h"
#include "GameFramework/Actor.h"
#include "Interfaces/A3GameControllableEntity.h"
#include "Interfaces/A3GameEntityFactory.h"

void UA3GameWorldSessionSubsystem::Tick(float DeltaTime)
{
    const float ConsumeInterval =
        InputConsumeHz > 0.0f
        ? 1.0f / InputConsumeHz
        : 0.05f;
    InputConsumeAccumulator += DeltaTime;
    if (InputConsumeAccumulator >= ConsumeInterval)
    {
        InputConsumeAccumulator = 0.0f;
        ConsumeLatestInputs();
    }
}

TStatId UA3GameWorldSessionSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(
        UA3GameWorldSessionSubsystem,
        STATGROUP_Tickables);
}

bool UA3GameWorldSessionSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

void UA3GameWorldSessionSubsystem::SetEntityFactory(
    UObject* FactoryObject)
{
    EntityFactory =
        FactoryObject
        && FactoryObject->GetClass()->ImplementsInterface(
            UA3GameEntityFactory::StaticClass())
        ? FactoryObject
        : nullptr;
}

FA3GameParticipantInfo
UA3GameWorldSessionSubsystem::RegisterParticipant(
    const FString& ParticipantId,
    const FString& UserId)
{
    const FString ResolvedParticipantId =
        ParticipantId.IsEmpty()
        ? MakeRuntimeId(TEXT("p"))
        : ParticipantId;
    if (FA3GameParticipantInfo* Existing =
        Participants.Find(ResolvedParticipantId))
    {
        Existing->bOnline = true;
        if (!UserId.IsEmpty())
        {
            Existing->UserId = UserId;
        }
        return *Existing;
    }

    FA3GameParticipantInfo Participant;
    Participant.ParticipantId = ResolvedParticipantId;
    Participant.WorldId = WorldId;
    Participant.UserId = UserId;
    Participants.Add(ResolvedParticipantId, Participant);
    return Participant;
}

void UA3GameWorldSessionSubsystem::MarkParticipantOffline(
    const FString& ParticipantId)
{
    if (FA3GameParticipantInfo* Participant =
        Participants.Find(ParticipantId))
    {
        Participant->bOnline = false;
    }

    for (TPair<FString, FA3GameControllerState>& Pair : Controllers)
    {
        FA3GameControllerState& Controller = Pair.Value;
        if (Controller.ParticipantId != ParticipantId)
        {
            continue;
        }
        Controller.bOnline = false;
        if (FA3GameControlBinding* Binding =
            ControlBindings.Find(Controller.ControllerId))
        {
            Binding->bActive = false;
        }
    }
}

AActor* UA3GameWorldSessionSubsystem::SpawnEntity(
    const FA3GameEntitySpawnRequest& Request)
{
    if (
        !EntityFactory
        || !EntityFactory->GetClass()->ImplementsInterface(
            UA3GameEntityFactory::StaticClass()))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[A3Game] SpawnEntity requires a registered entity factory"));
        return nullptr;
    }

    FA3GameEntitySpawnRequest ResolvedRequest = Request;
    if (ResolvedRequest.WorldId.IsEmpty())
    {
        ResolvedRequest.WorldId = WorldId;
    }
    if (ResolvedRequest.EntityId.IsEmpty())
    {
        ResolvedRequest.EntityId = MakeRuntimeId(TEXT("ent"));
    }

    AActor* Actor =
        IA3GameEntityFactory::Execute_SpawnRuntimeEntity(
            EntityFactory,
            ResolvedRequest);
    if (!Actor)
    {
        return nullptr;
    }
    return RegisterEntity(
        ResolvedRequest.EntityId,
        Actor,
        ResolvedRequest.ParticipantId)
        ? Actor
        : nullptr;
}

bool UA3GameWorldSessionSubsystem::RegisterEntity(
    const FString& EntityId,
    AActor* Actor,
    const FString& ParticipantId)
{
    if (EntityId.IsEmpty() || !IsValid(Actor))
    {
        return false;
    }

    EntityActors.Add(EntityId, Actor);
    if (Actor->GetClass()->ImplementsInterface(
        UA3GameControllableEntity::StaticClass()))
    {
        IA3GameControllableEntity::Execute_SetRuntimeEntityId(
            Actor,
            EntityId);
    }
    if (UA3GameRuntimeEntityComponent* Component =
        Actor->FindComponentByClass<
            UA3GameRuntimeEntityComponent>())
    {
        Component->SetRuntimeEntityId(EntityId);
    }

    if (!ParticipantId.IsEmpty())
    {
        FA3GameParticipantInfo Participant =
            RegisterParticipant(ParticipantId, TEXT(""));
        Participant.EntityId = EntityId;
        Participants.Add(Participant.ParticipantId, Participant);
    }
    return true;
}

bool UA3GameWorldSessionSubsystem::RemoveEntity(
    const FString& EntityId,
    bool bDestroyActor)
{
    TObjectPtr<AActor> Actor;
    if (!EntityActors.RemoveAndCopyValue(EntityId, Actor))
    {
        return true;
    }
    for (TPair<FString, FA3GameControlBinding>& Pair : ControlBindings)
    {
        if (Pair.Value.EntityId == EntityId)
        {
            Pair.Value.bActive = false;
            LatestInputsByController.Remove(Pair.Key);
        }
    }
    if (bDestroyActor && IsValid(Actor))
    {
        Actor->Destroy();
    }
    return true;
}

FA3GameControllerState
UA3GameWorldSessionSubsystem::CreateController(
    const FString& ParticipantId,
    const FString& ControllerId,
    const FString& Kind)
{
    const FA3GameParticipantInfo Participant =
        RegisterParticipant(ParticipantId, TEXT(""));
    const FString ResolvedControllerId =
        ControllerId.IsEmpty()
        ? MakeRuntimeId(TEXT("ctrl"))
        : ControllerId;

    for (TPair<FString, FA3GameControllerState>& Pair : Controllers)
    {
        FA3GameControllerState& Controller = Pair.Value;
        if (
            Controller.ParticipantId == Participant.ParticipantId
            && Controller.ControllerId != ResolvedControllerId)
        {
            Controller.bOnline = false;
            if (FA3GameControlBinding* Binding =
                ControlBindings.Find(Controller.ControllerId))
            {
                Binding->bActive = false;
            }
        }
    }

    FA3GameControllerState Controller;
    Controller.ControllerId = ResolvedControllerId;
    Controller.ParticipantId = Participant.ParticipantId;
    Controller.WorldId = WorldId;
    Controller.Kind = Kind.IsEmpty() ? TEXT("human") : Kind;
    Controllers.Add(ResolvedControllerId, Controller);
    return Controller;
}

bool UA3GameWorldSessionSubsystem::BindControllerToEntity(
    const FString& ControllerId,
    const FString& EntityId,
    EA3GameControlMode Mode,
    int32 Priority)
{
    if (
        !Controllers.Contains(ControllerId)
        || !EntityActors.Contains(EntityId))
    {
        return false;
    }

    FA3GameControlBinding Binding;
    Binding.ControllerId = ControllerId;
    Binding.EntityId = EntityId;
    Binding.WorldId = WorldId;
    Binding.Mode = Mode;
    Binding.Priority = Priority;
    Binding.bActive = Mode != EA3GameControlMode::Observing;
    ControlBindings.Add(ControllerId, Binding);
    return true;
}

bool UA3GameWorldSessionSubsystem::UnbindController(
    const FString& ControllerId)
{
    FA3GameControlBinding* Binding =
        ControlBindings.Find(ControllerId);
    if (!Binding)
    {
        return false;
    }
    Binding->bActive = false;
    LatestInputsByController.Remove(ControllerId);
    return true;
}

bool UA3GameWorldSessionSubsystem::EnqueueInputState(
    const FA3GameRuntimeInputState& InputState)
{
    FA3GameControlBinding* Binding =
        ControlBindings.Find(InputState.ControllerId);
    if (!Binding || !Binding->bActive)
    {
        return false;
    }
    if (
        !InputState.EntityId.IsEmpty()
        && InputState.EntityId != Binding->EntityId)
    {
        return false;
    }

    FA3GameRuntimeInputState Normalized = InputState;
    Normalized.WorldId =
        Normalized.WorldId.IsEmpty()
        ? WorldId
        : Normalized.WorldId;
    Normalized.EntityId = Binding->EntityId;
    Normalized.MoveX = FMath::Clamp(
        Normalized.MoveX,
        -1.0f,
        1.0f);
    Normalized.MoveY = FMath::Clamp(
        Normalized.MoveY,
        -1.0f,
        1.0f);
    if (Normalized.TimestampSeconds <= 0.0 && GetWorld())
    {
        Normalized.TimestampSeconds =
            GetWorld()->GetTimeSeconds();
    }
    LatestInputsByController.Add(
        InputState.ControllerId,
        Normalized);
    return true;
}

TArray<FA3GameEntitySnapshot>
UA3GameWorldSessionSubsystem::GetWorldStateSnapshot() const
{
    TArray<FA3GameEntitySnapshot> Snapshots;
    for (const TPair<FString, TObjectPtr<AActor>>& Pair : EntityActors)
    {
        AActor* Actor = Pair.Value.Get();
        if (!IsValid(Actor))
        {
            continue;
        }

        FA3GameEntitySnapshot Snapshot;
        if (Actor->GetClass()->ImplementsInterface(
            UA3GameControllableEntity::StaticClass()))
        {
            Snapshot =
                IA3GameControllableEntity::Execute_GetRuntimeSnapshot(
                    Actor);
        }
        else if (
            const UA3GameRuntimeEntityComponent* Component =
                Actor->FindComponentByClass<
                    UA3GameRuntimeEntityComponent>())
        {
            Snapshot = Component->GetRuntimeSnapshot();
        }
        else
        {
            Snapshot.EntityId = Pair.Key;
            Snapshot.ActorLabel = Actor->GetName();
            Snapshot.Position = Actor->GetActorLocation();
            Snapshot.Rotation = Actor->GetActorRotation();
        }
        if (Snapshot.EntityId.IsEmpty())
        {
            Snapshot.EntityId = Pair.Key;
        }
        Snapshots.Add(Snapshot);
    }
    return Snapshots;
}

AActor* UA3GameWorldSessionSubsystem::GetActorForEntity(
    const FString& EntityId) const
{
    const TObjectPtr<AActor>* Actor = EntityActors.Find(EntityId);
    return Actor ? Actor->Get() : nullptr;
}

void UA3GameWorldSessionSubsystem::ConsumeLatestInputs()
{
    TMap<FString, FA3GameRuntimeInputState> InputsToApply =
        MoveTemp(LatestInputsByController);
    LatestInputsByController.Reset();

    for (const TPair<FString, FA3GameRuntimeInputState>& Pair
        : InputsToApply)
    {
        const FA3GameRuntimeInputState& InputState = Pair.Value;
        AActor* Actor = GetActorForEntity(InputState.EntityId);
        if (!IsValid(Actor))
        {
            continue;
        }

        if (Actor->GetClass()->ImplementsInterface(
            UA3GameControllableEntity::StaticClass()))
        {
            IA3GameControllableEntity::Execute_ApplyRuntimeInput(
                Actor,
                InputState);
            continue;
        }
        if (UA3GameRuntimeEntityComponent* Component =
            Actor->FindComponentByClass<
                UA3GameRuntimeEntityComponent>())
        {
            Component->ApplyRuntimeInput(InputState);
        }
    }
}

FString UA3GameWorldSessionSubsystem::MakeRuntimeId(
    const FString& Prefix) const
{
    return FString::Printf(
        TEXT("%s_%s"),
        *Prefix,
        *FGuid::NewGuid()
            .ToString(EGuidFormats::Digits)
            .Left(12));
}
