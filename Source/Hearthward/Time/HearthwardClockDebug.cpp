#include "HearthwardWorldClockSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
namespace
{
void ShowClockSnapshot(UWorld* World)
{
    const auto* Clock = World ? World->GetSubsystem<UHearthwardWorldClockSubsystem>() : nullptr;
    if (!Clock)
    {
        UE_LOG(LogTemp, Display, TEXT("Hearthward.Clock: no active game world"));
        return;
    }
    const auto Time = Clock->GetSnapshot();
    const FString Message = FString::Printf(
        TEXT("CLOCK SNAPSHOT | Active %.3f s | Calendar %.3f min | Elapsed %lld days + %.3f min | %s"),
        Time.ActivePlaySeconds, Time.ElapsedCalendarMinutes, Time.ElapsedDays, Time.MinuteOfDay,
        World->IsPaused() ? TEXT("PAUSED") : TEXT("RUNNING"));
    UE_LOG(LogTemp, Display, TEXT("Hearthward.Clock: %s"), *Message);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(5005, 10.0f, FColor::Cyan, Message);
    }
}

FAutoConsoleCommandWithWorld ClockCommand(
    TEXT("Hearthward.Clock"), TEXT("Print an elapsed-time snapshot; does not change time."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&ShowClockSnapshot));
}
#endif
