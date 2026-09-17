#include "Runtime/A3GameRuntimeInputReceiver.h"

#include "Async/Async.h"
#include "Common/UdpSocketBuilder.h"
#include "DataTypes/A3GameRuntimeTypes.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Subsystems/A3GameRuntimeSubsystem.h"
#include "Subsystems/A3GameWorldSessionSubsystem.h"

namespace
{
FVector ReadVector(
    const TSharedPtr<FJsonObject>& Object,
    const FVector& DefaultValue)
{
    if (!Object.IsValid())
    {
        return DefaultValue;
    }
    double X = DefaultValue.X;
    double Y = DefaultValue.Y;
    double Z = DefaultValue.Z;
    Object->TryGetNumberField(TEXT("x"), X);
    Object->TryGetNumberField(TEXT("y"), Y);
    Object->TryGetNumberField(TEXT("z"), Z);
    return FVector(X, Y, Z);
}

FRotator ReadRotator(
    const TSharedPtr<FJsonObject>& Object,
    const FRotator& DefaultValue)
{
    if (!Object.IsValid())
    {
        return DefaultValue;
    }
    double Pitch = DefaultValue.Pitch;
    double Yaw = DefaultValue.Yaw;
    double Roll = DefaultValue.Roll;
    Object->TryGetNumberField(TEXT("pitch"), Pitch);
    Object->TryGetNumberField(TEXT("yaw"), Yaw);
    Object->TryGetNumberField(TEXT("roll"), Roll);
    return FRotator(Pitch, Yaw, Roll);
}

EA3GameControlMode ParseControlMode(const FString& Value)
{
    if (Value.Equals(TEXT("priority"), ESearchCase::IgnoreCase))
    {
        return EA3GameControlMode::Priority;
    }
    if (Value.Equals(TEXT("assisted"), ESearchCase::IgnoreCase))
    {
        return EA3GameControlMode::Assisted;
    }
    if (Value.Equals(TEXT("observing"), ESearchCase::IgnoreCase))
    {
        return EA3GameControlMode::Observing;
    }
    return EA3GameControlMode::Exclusive;
}

FString JsonValueToString(const TSharedPtr<FJsonValue>& Value)
{
    if (!Value.IsValid())
    {
        return FString();
    }
    switch (Value->Type)
    {
    case EJson::String:
        return Value->AsString();
    case EJson::Number:
        return FString::SanitizeFloat(Value->AsNumber());
    case EJson::Boolean:
        return Value->AsBool() ? TEXT("true") : TEXT("false");
    default:
        return FString();
    }
}
}

AA3GameRuntimeInputReceiver::AA3GameRuntimeInputReceiver()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AA3GameRuntimeInputReceiver::BeginPlay()
{
    Super::BeginPlay();
    ResolveRuntimePortFromLaunchOptions();
    if (bAutoStart)
    {
        StartReceiver();
    }
}

void AA3GameRuntimeInputReceiver::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    StopReceiver();
    Super::EndPlay(EndPlayReason);
}

bool AA3GameRuntimeInputReceiver::StartReceiver()
{
    if (Socket)
    {
        return true;
    }
    if (
        GetNetMode() == NM_DedicatedServer
        && !bAllowDedicatedServer)
    {
        return false;
    }

    const FIPv4Endpoint Endpoint(
        FIPv4Address::Any,
        ListenPort);
    Socket = FUdpSocketBuilder(
        TEXT("A3GameRuntimeInputReceiver"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToEndpoint(Endpoint)
        .WithReceiveBufferSize(2 * 1024 * 1024);
    if (!Socket)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[A3Game] Failed to bind runtime UDP port %d"),
            ListenPort);
        return false;
    }

    SocketReceiver = MakeUnique<FUdpSocketReceiver>(
        Socket,
        FTimespan::FromMilliseconds(10),
        TEXT("A3GameRuntimeInputReceiver"));
    SocketReceiver->OnDataReceived().BindUObject(
        this,
        &AA3GameRuntimeInputReceiver::OnUdpDataReceived);
    SocketReceiver->Start();
    return true;
}

void AA3GameRuntimeInputReceiver::StopReceiver()
{
    if (SocketReceiver)
    {
        SocketReceiver->Stop();
        SocketReceiver.Reset();
    }
    if (Socket)
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)
            ->DestroySocket(Socket);
        Socket = nullptr;
    }
}

void AA3GameRuntimeInputReceiver::OnUdpDataReceived(
    const FArrayReaderPtr& Data,
    const FIPv4Endpoint& Endpoint)
{
    (void)Endpoint;
    if (!Data.IsValid() || Data->Num() <= 0)
    {
        return;
    }

    const FUTF8ToTCHAR Converted(
        reinterpret_cast<const ANSICHAR*>(Data->GetData()),
        Data->Num());
    const FString JsonString(
        Converted.Length(),
        Converted.Get());
    const TWeakObjectPtr<AA3GameRuntimeInputReceiver> WeakThis(this);
    AsyncTask(
        ENamedThreads::GameThread,
        [WeakThis, JsonString]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->HandleRuntimeJson(JsonString);
            }
        });
}

bool AA3GameRuntimeInputReceiver::HandleRuntimeJson(
    const FString& JsonString)
{
    TSharedPtr<FJsonObject> Payload;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(JsonString);
    if (
        !FJsonSerializer::Deserialize(Reader, Payload)
        || !Payload.IsValid())
    {
        return false;
    }

    FString Type;
    if (!Payload->TryGetStringField(TEXT("type"), Type))
    {
        return false;
    }
    return DispatchCommand(Type, Payload);
}

bool AA3GameRuntimeInputReceiver::DispatchCommand(
    const FString& Type,
    const TSharedPtr<FJsonObject>& Payload)
{
    if (Type == TEXT("register_participant"))
    {
        return RegisterParticipant(Payload);
    }
    if (Type == TEXT("participant_offline"))
    {
        return MarkParticipantOffline(Payload);
    }
    if (Type == TEXT("create_controller"))
    {
        return CreateController(Payload);
    }
    if (Type == TEXT("bind_controller"))
    {
        return BindController(Payload);
    }
    if (Type == TEXT("sync_session"))
    {
        return SyncSession(Payload);
    }
    if (Type == TEXT("ensure_entity"))
    {
        return EnsureEntity(Payload);
    }
    if (Type == TEXT("input_state") || Type == TEXT("input"))
    {
        return ApplyInput(Payload);
    }
    if (Type == TEXT("destroy_entity"))
    {
        return DestroyEntity(Payload);
    }

    FString JsonPayload;
    const TSharedRef<TJsonWriter<>> Writer =
        TJsonWriterFactory<>::Create(&JsonPayload);
    FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer);
    Writer->Close();
    if (UWorld* World = GetWorld())
    {
        if (UA3GameRuntimeSubsystem* Runtime =
            World->GetSubsystem<UA3GameRuntimeSubsystem>())
        {
            return Runtime->DispatchExtensionMessage(
                Type,
                JsonPayload);
        }
    }
    return false;
}

bool AA3GameRuntimeInputReceiver::RegisterParticipant(
    const TSharedPtr<FJsonObject>& Payload)
{
    UWorld* World = GetWorld();
    UA3GameWorldSessionSubsystem* Session =
        World
        ? World->GetSubsystem<UA3GameWorldSessionSubsystem>()
        : nullptr;
    if (!Session)
    {
        return false;
    }
    FString ParticipantId;
    FString UserId;
    Payload->TryGetStringField(
        TEXT("participant_id"),
        ParticipantId);
    Payload->TryGetStringField(TEXT("user_id"), UserId);
    Session->RegisterParticipant(ParticipantId, UserId);
    return true;
}

bool AA3GameRuntimeInputReceiver::MarkParticipantOffline(
    const TSharedPtr<FJsonObject>& Payload)
{
    FString ParticipantId;
    Payload->TryGetStringField(
        TEXT("participant_id"),
        ParticipantId);
    UWorld* World = GetWorld();
    if (UA3GameWorldSessionSubsystem* Session =
        World
        ? World->GetSubsystem<UA3GameWorldSessionSubsystem>()
        : nullptr)
    {
        Session->MarkParticipantOffline(ParticipantId);
        return true;
    }
    return false;
}

bool AA3GameRuntimeInputReceiver::CreateController(
    const TSharedPtr<FJsonObject>& Payload)
{
    FString ParticipantId;
    FString ControllerId;
    FString Kind;
    Payload->TryGetStringField(
        TEXT("participant_id"),
        ParticipantId);
    Payload->TryGetStringField(
        TEXT("controller_id"),
        ControllerId);
    Payload->TryGetStringField(TEXT("kind"), Kind);
    UWorld* World = GetWorld();
    if (UA3GameWorldSessionSubsystem* Session =
        World
        ? World->GetSubsystem<UA3GameWorldSessionSubsystem>()
        : nullptr)
    {
        Session->CreateController(
            ParticipantId,
            ControllerId,
            Kind);
        return true;
    }
    return false;
}

bool AA3GameRuntimeInputReceiver::BindController(
    const TSharedPtr<FJsonObject>& Payload)
{
    FString ControllerId;
    FString EntityId;
    FString Mode;
    double Priority = 0.0;
    Payload->TryGetStringField(
        TEXT("controller_id"),
        ControllerId);
    Payload->TryGetStringField(TEXT("entity_id"), EntityId);
    Payload->TryGetStringField(TEXT("mode"), Mode);
    Payload->TryGetNumberField(TEXT("priority"), Priority);
    UWorld* World = GetWorld();
    if (UA3GameWorldSessionSubsystem* Session =
        World
        ? World->GetSubsystem<UA3GameWorldSessionSubsystem>()
        : nullptr)
    {
        return Session->BindControllerToEntity(
            ControllerId,
            EntityId,
            ParseControlMode(Mode),
            static_cast<int32>(Priority));
    }
    return false;
}

bool AA3GameRuntimeInputReceiver::SyncSession(
    const TSharedPtr<FJsonObject>& Payload)
{
    FString EntityId;
    FString ControllerId;
    Payload->TryGetStringField(TEXT("entity_id"), EntityId);
    Payload->TryGetStringField(
        TEXT("controller_id"),
        ControllerId);
    const bool bSynchronized =
        RegisterParticipant(Payload)
        && CreateController(Payload)
        && EnsureEntity(Payload)
        && BindController(Payload);
    UE_LOG(
        LogTemp,
        Display,
        TEXT(
            "[A3Game] Runtime session sync entity=%s "
            "controller=%s result=%s"),
        *EntityId,
        *ControllerId,
        bSynchronized ? TEXT("ok") : TEXT("failed"));
    return bSynchronized;
}

bool AA3GameRuntimeInputReceiver::EnsureEntity(
    const TSharedPtr<FJsonObject>& Payload)
{
    UWorld* World = GetWorld();
    UA3GameWorldSessionSubsystem* Session =
        World
        ? World->GetSubsystem<UA3GameWorldSessionSubsystem>()
        : nullptr;
    if (!Session)
    {
        return false;
    }

    FString EntityId;
    Payload->TryGetStringField(TEXT("entity_id"), EntityId);
    if (
        !EntityId.IsEmpty()
        && IsValid(Session->GetActorForEntity(EntityId)))
    {
        return true;
    }

    FA3GameEntitySpawnRequest Request;
    Request.EntityId = EntityId;
    Payload->TryGetStringField(TEXT("world_id"), Request.WorldId);
    Payload->TryGetStringField(
        TEXT("participant_id"),
        Request.ParticipantId);

    const TSharedPtr<FJsonObject>* TransformObject = nullptr;
    if (
        Payload->TryGetObjectField(
            TEXT("transform"),
            TransformObject)
        && TransformObject
        && TransformObject->IsValid())
    {
        const TSharedPtr<FJsonObject>* LocationObject = nullptr;
        const TSharedPtr<FJsonObject>* RotationObject = nullptr;
        const TSharedPtr<FJsonObject>* ScaleObject = nullptr;
        (*TransformObject)->TryGetObjectField(
            TEXT("location"),
            LocationObject);
        (*TransformObject)->TryGetObjectField(
            TEXT("rotation"),
            RotationObject);
        (*TransformObject)->TryGetObjectField(
            TEXT("scale"),
            ScaleObject);
        Request.Transform = FTransform(
            ReadRotator(
                RotationObject ? *RotationObject : nullptr,
                FRotator::ZeroRotator),
            ReadVector(
                LocationObject ? *LocationObject : nullptr,
                FVector::ZeroVector),
            ReadVector(
                ScaleObject ? *ScaleObject : nullptr,
                FVector::OneVector));
    }

    const TSharedPtr<FJsonObject>* ParametersObject = nullptr;
    if (
        Payload->TryGetObjectField(
            TEXT("parameters"),
            ParametersObject)
        && ParametersObject
        && ParametersObject->IsValid())
    {
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair
            : (*ParametersObject)->Values)
        {
            Request.Parameters.Add(
                Pair.Key,
                JsonValueToString(Pair.Value));
        }
    }
    return IsValid(Session->SpawnEntity(Request));
}

bool AA3GameRuntimeInputReceiver::ApplyInput(
    const TSharedPtr<FJsonObject>& Payload)
{
    FA3GameRuntimeInputState Input;
    Payload->TryGetStringField(TEXT("world_id"), Input.WorldId);
    Payload->TryGetStringField(
        TEXT("participant_id"),
        Input.ParticipantId);
    Payload->TryGetStringField(
        TEXT("controller_id"),
        Input.ControllerId);
    Payload->TryGetStringField(TEXT("entity_id"), Input.EntityId);
    double MoveX = 0.0;
    double MoveY = 0.0;
    double Yaw = 0.0;
    double Pitch = 0.0;
    double Sequence = 0.0;
    double Timestamp = 0.0;
    Payload->TryGetNumberField(TEXT("move_x"), MoveX);
    Payload->TryGetNumberField(TEXT("move_y"), MoveY);
    Payload->TryGetNumberField(TEXT("yaw"), Yaw);
    Payload->TryGetNumberField(TEXT("pitch"), Pitch);
    Payload->TryGetNumberField(TEXT("seq"), Sequence);
    Payload->TryGetNumberField(TEXT("ts"), Timestamp);
    Payload->TryGetBoolField(TEXT("run"), Input.bRun);
    Payload->TryGetBoolField(TEXT("jump"), Input.bJump);
    Input.MoveX = static_cast<float>(MoveX);
    Input.MoveY = static_cast<float>(MoveY);
    Input.Yaw = static_cast<float>(Yaw);
    Input.Pitch = static_cast<float>(Pitch);
    Input.Sequence = static_cast<int32>(Sequence);
    Input.TimestampSeconds = Timestamp;

    UWorld* World = GetWorld();
    if (UA3GameWorldSessionSubsystem* Session =
        World
        ? World->GetSubsystem<UA3GameWorldSessionSubsystem>()
        : nullptr)
    {
        return Session->EnqueueInputState(Input);
    }
    return false;
}

bool AA3GameRuntimeInputReceiver::DestroyEntity(
    const TSharedPtr<FJsonObject>& Payload)
{
    FString EntityId;
    bool bDestroyActor = true;
    Payload->TryGetStringField(TEXT("entity_id"), EntityId);
    Payload->TryGetBoolField(
        TEXT("destroy_actor"),
        bDestroyActor);
    UWorld* World = GetWorld();
    if (UA3GameWorldSessionSubsystem* Session =
        World
        ? World->GetSubsystem<UA3GameWorldSessionSubsystem>()
        : nullptr)
    {
        return Session->RemoveEntity(
            EntityId,
            bDestroyActor);
    }
    return false;
}

void AA3GameRuntimeInputReceiver::
ResolveRuntimePortFromLaunchOptions()
{
    int32 ParsedPort = 0;
    if (
        FParse::Value(
            FCommandLine::Get(),
            TEXT("A3GameRuntimeInputPort="),
            ParsedPort)
        && ParsedPort > 0)
    {
        ListenPort = ParsedPort;
    }
}
