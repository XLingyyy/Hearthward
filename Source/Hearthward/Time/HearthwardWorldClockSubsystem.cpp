#include "HearthwardWorldClockSubsystem.h"
#include "Engine/World.h"

bool UHearthwardWorldClockSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UHearthwardWorldClockSubsystem::IsTickable() const
{
    return IsInitialized() && GetWorld()->HasBegunPlay();
}

void UHearthwardWorldClockSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Clock.Advance(DeltaTime);
}

TStatId UHearthwardWorldClockSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UHearthwardWorldClockSubsystem, STATGROUP_Tickables);
}

FHearthwardClockSnapshot UHearthwardWorldClockSubsystem::GetSnapshot() const
{
    FHearthwardClockSnapshot Result;
    Result.ActivePlaySeconds = Clock.GetActivePlaySeconds();
    Result.ElapsedCalendarMinutes = Clock.GetElapsedCalendarMinutes();
    Result.ElapsedDays = Clock.GetElapsedDays();
    Result.MinuteOfDay = Clock.GetMinuteOfDay();
    return Result;
}
