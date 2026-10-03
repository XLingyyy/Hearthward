#include "HearthwardWorldClockSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "../UI/HearthwardHUD.h"
#include "../UI/HearthwardScreenWidget.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "EngineUtils.h"

#if !UE_BUILD_SHIPPING
namespace
{
void PrepareTravelFixture(const TArray<FString>& Args,UWorld* World)
{
    FString TestPool;FGuid PoolId;
    if(!World || Args.Num()!=1 || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),TestPool) || !FGuid::Parse(TestPool,PoolId))return;
    const FString Mode=Args[0];if(Mode!=TEXT("down") && Mode!=TEXT("stay") && Mode!=TEXT("blocked"))return;
    auto* Player=UGameplayStatics::GetPlayerPawn(World,0);auto* G=Player?Player->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();
    if(!G || !Campaign->Active() || Campaign->Busy())return;
    G->Activated.Add(TEXT("camp"));G->Activated.Add(TEXT("route_mine"));
    for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It)
    {
        It->StopNavigation();
        if(auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>())
        {S->State.Life=Mode==TEXT("down")?EHearthwardLife::Downed:EHearthwardLife::Alive;S->State.DownRemaining=Mode==TEXT("down")?45:0;S->BrotherHealth=Mode==TEXT("down")?0:100;}
    }
    int32 Field=0;
    for(auto& E:Campaign->State.Enemies)
    {
        if(auto* Actor=Campaign->Actor(E.Id))E.Combat=Actor->Target->Snapshot();
        E.Combat.Seen.Reset();E.Combat.Detection.Reset();
        if(E.Group==TEXT("field") && Field<3)
        {
            E.Combat.Health=Field==2?0:11;E.Combat.bStunned=Field==2;
            E.Combat.HitRemaining=2;E.Combat.InvestigationRemaining=12;
            if(Field==0)
            {
                E.Combat.Seen.Add(TEXT("player"));
                if(Mode==TEXT("down"))E.Combat.Detection.Add(TEXT("brother"),1);
            }
            ++Field;
        }
        if(auto* Actor=Campaign->Actor(E.Id))Actor->Target->Restore(E.Combat);
    }
    // Only this test pool substitutes an unreachable landing for the existing destination.
    if(Mode==TEXT("blocked"))Campaign->State.Positions.Add(TEXT("route_mine"),FVector(500000,500000,20000));
    FFileHelper::SaveStringToFile(Campaign->Snapshot(),*FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Task052/travel-before.json")));
}
void CheckMenuKeyboard(UWorld* World)
{
    FString TestPool;FGuid Id;
    if(!World || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),TestPool) || !FGuid::Parse(TestPool,Id))return;
    auto* PC=UGameplayStatics::GetPlayerController(World,0);auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr;if(!UI)return;
    UI->OpenPage(TEXT("title"));
    UI->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::Down,FModifierKeysState(),0,false,0,0));
    UI->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::Down,FModifierKeysState(),0,false,0,0));
    UI->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));
    UI->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));
    const bool Passed=UI->GetPage()==TEXT("hud") && World->GetSubsystem<UHearthwardSaveSubsystem>()->GetCampaignId().IsValid();
    FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"page\":\"%s\",\"method\":\"NativeOnKeyDown Down/Down/Enter/Enter; new-game prompt and confirmation; synthetic engine events, no desktop input\"}"),Passed?TEXT("true"):TEXT("false"),*UI->GetPage().ToString()),*FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Task052/keyboard.json")));
}
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
FAutoConsoleCommandWithWorld KeyboardCheckCommand(TEXT("Hearthward.Clock.CheckMenuKeyboard"),TEXT("Nonshipping isolated-save-pool UI key-event regression check."),FConsoleCommandWithWorldDelegate::CreateStatic(&CheckMenuKeyboard));
FAutoConsoleCommandWithWorldAndArgs TravelFixtureCommand(TEXT("Hearthward.Clock.TravelFixture"),TEXT("Nonshipping GUID test-pool fixture: down/stay/blocked. Real travel code performs settlement."),FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PrepareTravelFixture));
}
#endif
