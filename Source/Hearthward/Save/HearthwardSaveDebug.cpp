#include "HearthwardSaveSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
namespace
{
void SaveCommand(const TArray<FString>& Args, UWorld* World)
{
    if (!World || Args.IsEmpty()) return;
    auto* Save = World->GetSubsystem<UHearthwardSaveSubsystem>();
    if (!Save) return;
    const FString Op = Args[0].ToLower();
    FGuid Id;
    if (Op == TEXT("enable")) Save->EnablePrototype();
    else if (Op == TEXT("new")) Save->StartNewProgress();
    else if (Op == TEXT("manual")) Save->SavePoint(true);
    else if (Op == TEXT("auto")) Save->SavePoint(false);
    else if (Op == TEXT("list"))
    {
        for (const auto& P : Save->GetPoints())
            UE_LOG(LogTemp, Display, TEXT("Save=%s campaign=%s utc=%s manual=%d locked=%d location=%s stage=%s"),
                *P.SaveId.ToString(), *P.CampaignId.ToString(), *P.Created.ToIso8601(), P.Manual, P.Locked, *P.Location, *P.Stage);
    }
    else if (Args.Num() >= 2 && FGuid::Parse(Args[1], Id))
    {
        if (Op == TEXT("load")) Save->LoadPoint(Id);
        else if (Op == TEXT("delete")) Save->DeletePoint(Id);
        else if (Op == TEXT("lock")) Save->SetPointLocked(Id, true);
        else if (Op == TEXT("unlock")) Save->SetPointLocked(Id, false);
    }
    UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_ONLY Save: %s (%d/50)"), *Save->GetStatus(), Save->GetPoints().Num());
    if (GEngine) GEngine->AddOnScreenDebugMessage(16016, 6, FColor::Yellow, TEXT("PROTOTYPE_ONLY: ") + Save->GetStatus());
}
FAutoConsoleCommandWithWorldAndArgs SaveDebugCommand(TEXT("Hearthward.Save"),
    TEXT("PROTOTYPE_ONLY: enable | new | manual | auto | list | load/delete/lock/unlock <GUID>. Explicit fixture required."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SaveCommand));
}
#endif
