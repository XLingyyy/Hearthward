#pragma once

#include "CoreMinimal.h"
#include "A3GameRuntimeTypes.generated.h"

UENUM(BlueprintType)
enum class EA3GameControlMode : uint8
{
    Exclusive,
    Priority,
    Assisted,
    Observing
};

UENUM(BlueprintType)
enum class EA3GameLocomotionState : uint8
{
    Idle,
    Walk,
    Run,
    Jump
};

USTRUCT(BlueprintType)
struct A3GAMEPLAYABLE_API FA3GameRuntimeInputState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString WorldId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString ParticipantId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString ControllerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString EntityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    float MoveX = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    float MoveY = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    bool bRun = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    bool bJump = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    float Yaw = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    float Pitch = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    int32 Sequence = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    double TimestampSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct A3GAMEPLAYABLE_API FA3GameEntitySpawnRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString WorldId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString ParticipantId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString EntityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FTransform Transform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    TMap<FString, FString> Parameters;
};

USTRUCT(BlueprintType)
struct A3GAMEPLAYABLE_API FA3GameParticipantInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString ParticipantId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString WorldId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString UserId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString EntityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    bool bOnline = true;
};

USTRUCT(BlueprintType)
struct A3GAMEPLAYABLE_API FA3GameControllerState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString ControllerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString ParticipantId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString WorldId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString Kind = TEXT("human");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    bool bOnline = true;
};

USTRUCT(BlueprintType)
struct A3GAMEPLAYABLE_API FA3GameControlBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString ControllerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString EntityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString WorldId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    EA3GameControlMode Mode = EA3GameControlMode::Exclusive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    bool bActive = true;
};

USTRUCT(BlueprintType)
struct A3GAMEPLAYABLE_API FA3GameEntitySnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString EntityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString ActorLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FVector Position = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    EA3GameLocomotionState LocomotionState = EA3GameLocomotionState::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    FString MotionState = TEXT("idle");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    bool bPersistent = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A3Game|Runtime")
    double LastInputTimeSeconds = 0.0;
};
