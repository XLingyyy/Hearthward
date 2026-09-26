#pragma once

#include "CoreMinimal.h"

// A is active play seconds; W is calendar minutes. Restore does not replay events.
class FHearthwardClockState
{
public:
    static constexpr double MinutesPerDay = 1440.0;
    static constexpr double SleepMinutes = 480.0;
    bool Advance(double ActiveDeltaSeconds) { return AdvanceBy(ActiveDeltaSeconds, ActiveDeltaSeconds); }
    bool AdvanceCalendar(double Minutes) { return AdvanceBy(0, Minutes); }
    bool Sleep() { return AdvanceCalendar(SleepMinutes); }
    bool Restore(double ActiveSeconds, double CalendarMinutes)
    {
        if (!IsValid(ActiveSeconds, CalendarMinutes)) return false;
        ActivePlaySeconds = ActiveSeconds;
        ElapsedCalendarMinutes = CalendarMinutes;
        return true;
    }
    static bool IsValid(double ActiveSeconds, double CalendarMinutes)
    {
        return FMath::IsFinite(ActiveSeconds) && FMath::IsFinite(CalendarMinutes)
            && ActiveSeconds >= 0 && CalendarMinutes >= ActiveSeconds
            && CalendarMinutes / MinutesPerDay < double(MAX_int64);
    }
    double GetActivePlaySeconds() const { return ActivePlaySeconds; }
    double GetElapsedCalendarMinutes() const { return ElapsedCalendarMinutes; }
    int64 GetElapsedDays() const { return FMath::FloorToInt64(ElapsedCalendarMinutes / MinutesPerDay); }
    double GetMinuteOfDay() const { return FMath::Fmod(ElapsedCalendarMinutes, MinutesPerDay); }

private:
    bool AdvanceBy(double Seconds, double Minutes)
    {
        if (!FMath::IsFinite(Seconds) || !FMath::IsFinite(Minutes) || Seconds < 0 || Minutes < 0) return false;
        return Restore(ActivePlaySeconds + Seconds, ElapsedCalendarMinutes + Minutes);
    }
    double ActivePlaySeconds = 0.0;
    double ElapsedCalendarMinutes = 0.0;
};
