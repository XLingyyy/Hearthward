#include "Subsystems/A3GameRuntimeSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interfaces/A3GameEntityFactory.h"
#include "Interfaces/A3GameRuntimeMessageHandler.h"
#include "Runtime/A3GameRuntimeInputReceiver.h"
#include "Subsystems/A3GameWorldSessionSubsystem.h"

bool UA3GameRuntimeSubsystem::DoesSupportWorldType(
    const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game
        || WorldType == EWorldType::PIE;
}

void UA3GameRuntimeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (!InWorld.IsGameWorld())
    {
        return;
    }
    TActorIterator<AA3GameRuntimeInputReceiver> ExistingReceiver(
        &InWorld);
    if (ExistingReceiver)
    {
        RuntimeInputReceiver = *ExistingReceiver;
        return;
    }

    FActorSpawnParameters Params;
    Params.Name = TEXT("A3Game_RuntimeInputReceiver");
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    RuntimeInputReceiver =
        InWorld.SpawnActor<AA3GameRuntimeInputReceiver>(
            AA3GameRuntimeInputReceiver::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            Params);
}

void UA3GameRuntimeSubsystem::Deinitialize()
{
    MessageHandlers.Reset();
    RuntimeInputReceiver = nullptr;
    Super::Deinitialize();
}

void UA3GameRuntimeSubsystem::SetEntityFactory(
    UObject* FactoryObject)
{
    if (UA3GameWorldSessionSubsystem* Session =
        GetSessionSubsystem())
    {
        Session->SetEntityFactory(FactoryObject);
    }
}

void UA3GameRuntimeSubsystem::RegisterMessageHandler(
    UObject* HandlerObject)
{
    if (
        !HandlerObject
        || !HandlerObject->GetClass()->ImplementsInterface(
            UA3GameRuntimeMessageHandler::StaticClass()))
    {
        return;
    }
    MessageHandlers.AddUnique(HandlerObject);
}

void UA3GameRuntimeSubsystem::UnregisterMessageHandler(
    UObject* HandlerObject)
{
    MessageHandlers.Remove(HandlerObject);
}

UA3GameWorldSessionSubsystem*
UA3GameRuntimeSubsystem::GetSessionSubsystem() const
{
    UWorld* World = GetWorld();
    return World
        ? World->GetSubsystem<
            UA3GameWorldSessionSubsystem>()
        : nullptr;
}

bool UA3GameRuntimeSubsystem::DispatchExtensionMessage(
    const FString& MessageType,
    const FString& JsonPayload) const
{
    bool bHandled = false;
    for (UObject* Handler : MessageHandlers)
    {
        if (
            !IsValid(Handler)
            || !Handler->GetClass()->ImplementsInterface(
                UA3GameRuntimeMessageHandler::StaticClass()))
        {
            continue;
        }
        bHandled =
            IA3GameRuntimeMessageHandler::
                Execute_HandleRuntimeMessage(
                    Handler,
                    MessageType,
                    JsonPayload)
            || bHandled;
    }
    return bHandled;
}
