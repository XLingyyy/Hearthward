#include "HearthwardWorldClockSubsystem.h"
#include "Engine/World.h"
#include "../Interaction/HearthwardHarvestSubsystem.h"

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
    if (GetWorld()->IsPaused()) return;
    if (Clock.Advance(DeltaTime))
        GetWorld()->GetSubsystem<UHearthwardHarvestSubsystem>()->RefreshDue(Clock.GetElapsedCalendarMinutes());
}

TStatId UHearthwardWorldClockSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UHearthwardWorldClockSubsystem, STATGROUP_Tickables);
}

FString UHearthwardWorldClockSubsystem::RequestSleep()
{
    return RequestCampfireAdvance(FHearthwardClockState::SleepMinutes);
}

FString UHearthwardWorldClockSubsystem::RequestCampfireAdvance(double Minutes)
{
    if (!FMath::IsFinite(Minutes) || Minutes <= 0) return TEXT("INVALID_DURATION");
    if (GetWorld()->IsPaused()) return TEXT("PAUSED");
    return TEXT("RULE_UNRESOLVED");
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
