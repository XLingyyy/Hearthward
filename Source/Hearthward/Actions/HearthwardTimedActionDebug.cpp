#include "HearthwardTimedActionComponent.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

#if !UE_BUILD_SHIPPING
namespace
{
UHearthwardTimedActionComponent* FindAction(UWorld* World)
{
    APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
    return Pawn ? Pawn->FindComponentByClass<UHearthwardTimedActionComponent>() : nullptr;
}

void ShowAction(UWorld* World)
{
    const auto* Action = FindAction(World);
    if (!Action) return;
    const FString Message = FString::Printf(TEXT("ACTION SNAPSHOT | %s | %.3f / 5.000 s | TIMER ONLY"),
        *UEnum::GetValueAsString(Action->GetStatus()), Action->GetElapsedSeconds());
    UE_LOG(LogTemp, Display, TEXT("Hearthward.Action: %s"), *Message);
    if (GEngine) GEngine->AddOnScreenDebugMessage(5006, 10.0f, FColor::Cyan, Message);
}

void StartAction(UWorld* World)
{
    if (auto* Action = FindAction(World)) Action->StartAction();
    ShowAction(World);
}

FAutoConsoleCommandWithWorld StartCommand(TEXT("Hearthward.Action.Start"),
    TEXT("Start the five-second action timer; does not build or grant items."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&StartAction));
FAutoConsoleCommandWithWorld QueryCommand(TEXT("Hearthward.Action"),
    TEXT("Show an action timer snapshot."), FConsoleCommandWithWorldDelegate::CreateStatic(&ShowAction));
}
#endif
