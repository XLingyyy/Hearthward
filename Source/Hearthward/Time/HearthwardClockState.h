#pragma once

#include "CoreMinimal.h"

// A and W remain elapsed durations; the display origin is saved separately.
class FHearthwardClockState
{
public:
    void Advance(double ActiveDeltaSeconds) { ActivePlaySeconds += ActiveDeltaSeconds; CalendarMinutes += ActiveDeltaSeconds; }
    double GetActivePlaySeconds() const { return ActivePlaySeconds; }
    // GDD v0.3: one active play second equals one calendar minute.
    double GetElapsedCalendarMinutes() const { return CalendarMinutes; }
    int64 GetElapsedDays() const { return FMath::FloorToInt64(GetElapsedCalendarMinutes() / 1440.0); }
    double GetMinuteOfDay() const { return FMath::Fmod(InitialMinute + CalendarMinutes, 1440.0); }
    int64 GetDisplayDay() const { return InitialDay + FMath::FloorToInt64((InitialMinute + CalendarMinutes) / 1440.0); }
    int64 GetInitialDay() const { return InitialDay; }
    double GetInitialMinute() const { return InitialMinute; }
    void SetOrigin(int64 Day,double Minute) { InitialDay=Day;InitialMinute=Minute; }
    static double FieldRefreshDue(double DeathW) { return (FMath::FloorToDouble(DeathW / 5760.0) + 1) * 5760.0; }

private:
    friend class UHearthwardSaveSubsystem;
    friend class UHearthwardWorldClockSubsystem;
    double ActivePlaySeconds = 0.0;
    double CalendarMinutes = 0.0;
    int64 InitialDay = 1;
    double InitialMinute = 0;
};
