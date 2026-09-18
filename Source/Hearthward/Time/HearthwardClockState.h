#pragma once

#include "CoreMinimal.h"

// Elapsed durations only. The GDD does not yet specify a starting date/time.
class FHearthwardClockState
{
public:
    void Advance(double ActiveDeltaSeconds) { ActivePlaySeconds += ActiveDeltaSeconds; }
    double GetActivePlaySeconds() const { return ActivePlaySeconds; }
    // GDD v0.3: one active play second equals one calendar minute.
    double GetElapsedCalendarMinutes() const { return ActivePlaySeconds; }
    int64 GetElapsedDays() const { return FMath::FloorToInt64(GetElapsedCalendarMinutes() / 1440.0); }
    double GetMinuteOfDay() const { return FMath::Fmod(GetElapsedCalendarMinutes(), 1440.0); }

private:
    friend class UHearthwardSaveSubsystem;
    double ActivePlaySeconds = 0.0;
};
