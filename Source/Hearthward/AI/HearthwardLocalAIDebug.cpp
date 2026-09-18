#include "HearthwardLocalAISubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING
namespace
{
FAutoConsoleCommandWithWorldAndArgs LocalAISayCommand(TEXT("Hearthward.AI.Say"), TEXT("Send real player text to the bundled local Qwen model. Requires Companion.CreateTest."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        if (!World || Args.IsEmpty()) return;
        for (TActorIterator<AHearthwardCompanionFixture> It(World); It; ++It)
        {
            World->GetSubsystem<UHearthwardLocalAISubsystem>()->SubmitPlayerText(UGameplayStatics::GetPlayerPawn(World, 0), *It, FString::Join(Args, TEXT(" ")));
            break;
        }
    }));
FAutoConsoleCommandWithWorld LocalAICancelCommand(TEXT("Hearthward.AI.CancelReply"), TEXT("Cancel pending model reply without cancelling an already executing goal."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    { if (World) World->GetSubsystem<UHearthwardLocalAISubsystem>()->CancelPending(); }));
}
#endif
