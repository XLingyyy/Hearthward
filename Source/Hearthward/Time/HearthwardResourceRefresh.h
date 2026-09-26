#pragma once
#include "CoreMinimal.h"
#include "HearthwardResourceRefresh.generated.h"

USTRUCT()
struct FHearthwardResourceRefresh
{
    GENERATED_BODY()
    UPROPERTY() double DepletedAt = 0;
    UPROPERTY() double DueAt = 0;
    UPROPERTY() FVector Position = FVector::ZeroVector;

    static constexpr double TreePeriodMinutes = 2880;
    static FHearthwardResourceRefresh DepletedTree(double CalendarMinutes, FVector At)
    {
        FHearthwardResourceRefresh Result;
        Result.DepletedAt = CalendarMinutes;
        Result.DueAt = CalendarMinutes + TreePeriodMinutes;
        Result.Position = At;
        return Result;
    }
    bool IsValid(double CalendarMinutes) const
    {
        return FMath::IsFinite(DepletedAt) && FMath::IsFinite(DueAt) && DepletedAt >= 0
            && DepletedAt <= CalendarMinutes && DueAt == DepletedAt + TreePeriodMinutes
            && !Position.ContainsNaN();
    }
    bool CanRefresh(double CalendarMinutes, bool Occupied) const
    { return CalendarMinutes >= DueAt && !Occupied; }
};
