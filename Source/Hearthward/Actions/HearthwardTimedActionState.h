#pragma once

#include "CoreMinimal.h"
#include "HearthwardTimedActionState.generated.h"

UENUM(BlueprintType)
enum class EHearthwardTimedActionStatus : uint8
{
    Idle,
    Running,
    Interrupted,
    Completed
};

struct FHearthwardTimedActionState
{
    static constexpr double DurationSeconds = 5.0;
    EHearthwardTimedActionStatus Status = EHearthwardTimedActionStatus::Idle;
    double StartedAt = 0.0;
    double ElapsedSeconds = 0.0;

    bool Start(double ActiveSeconds)
    {
        if (Status == EHearthwardTimedActionStatus::Running) return false;
        StartedAt = ActiveSeconds;
        ElapsedSeconds = 0.0;
        Status = EHearthwardTimedActionStatus::Running;
        return true;
    }

    bool Update(double ActiveSeconds)
    {
        if (Status != EHearthwardTimedActionStatus::Running) return false;
        ElapsedSeconds = FMath::Clamp(ActiveSeconds - StartedAt, 0.0, DurationSeconds);
        if (ElapsedSeconds < DurationSeconds) return false;
        Status = EHearthwardTimedActionStatus::Completed;
        return true;
    }

    bool Interrupt(double ActiveSeconds)
    {
        if (Status != EHearthwardTimedActionStatus::Running) return false;
        ElapsedSeconds = FMath::Clamp(ActiveSeconds - StartedAt, 0.0, DurationSeconds);
        Status = EHearthwardTimedActionStatus::Interrupted;
        return true;
    }
};
