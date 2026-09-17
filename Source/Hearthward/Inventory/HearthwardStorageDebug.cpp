#include "HearthwardStorageAccessComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
namespace
{
void CreateTestAccess(UWorld* World)
{
    if (!World || !World->IsGameWorld()) return;
    const FName Tag(TEXT("Hearthward.Storage.TestAccess"));
    int32 Existing = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
        if (It->ActorHasTag(Tag)) ++Existing;
    for (int32 Index = Existing; Index < 2; ++Index)
    {
        auto* Actor = World->SpawnActor<AActor>();
        if (!Actor) return;
        Actor->Tags.Add(Tag);
        auto* Access = NewObject<UHearthwardStorageAccessComponent>(Actor);
        Actor->AddInstanceComponent(Access);
        Access->RegisterComponent();
    }
}

FAutoConsoleCommandWithWorld TestAccessCommand(TEXT("Hearthward.Storage.CreateTestAccess"),
    TEXT("Development fixture only: create two runtime storage access actors, no assets or items."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&CreateTestAccess));
}
#endif
