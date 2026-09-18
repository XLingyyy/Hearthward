#include "HearthwardCompanionFixture.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING
namespace
{
AActor* CompanionFixtureMarker(UWorld* World, FVector Location, FVector Scale)
{
    auto* Actor = World->SpawnActor<AActor>();
    auto* Mesh = NewObject<UStaticMeshComponent>(Actor);
    Actor->AddInstanceComponent(Mesh);
    Actor->SetRootComponent(Mesh);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->RegisterComponent();
    Actor->SetActorScale3D(Scale);
    Actor->SetActorLocation(Location);
    return Actor;
}

AHearthwardCompanionFixture* FindCompanionFixture(UWorld* World)
{
    if (World) for (TActorIterator<AHearthwardCompanionFixture> It(World); It; ++It) return *It;
    return nullptr;
}

void CreateCompanionFixture(UWorld* World)
{
    APawn* Player = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
    if (!Player || FindCompanionFixture(World)) return;
    const FVector Start = Player->GetActorLocation() + Player->GetActorForwardVector() * 200 + Player->GetActorRightVector() * 200;
    auto* Camp = CompanionFixtureMarker(World, Start, FVector(1.2, 1.2, 0.2));
    Camp->Tags.Add(TEXT("Hearthward.Companion.Camp.PROTOTYPE_ONLY"));
    auto* Resource = CompanionFixtureMarker(World, Start + Player->GetActorForwardVector() * 600, FVector(0.7, 0.7, 0.7));
    Resource->Tags.Add(TEXT("Hearthward.Companion.Source.PROTOTYPE_ONLY"));
    auto* Stock = NewObject<UHearthwardInventoryComponent>(Resource);
    Resource->AddInstanceComponent(Stock);
    Stock->RegisterComponent();
    Stock->TryAdd(TEXT("wood"), 16);
    for (AActor* Target : {Camp, Resource})
    {
        auto* Interaction = NewObject<UHearthwardResourceInteractionComponent>(Target);
        Target->AddInstanceComponent(Interaction);
        Interaction->SetupAttachment(Target->GetRootComponent());
        Interaction->InitializePrototype(Target == Camp);
        Interaction->RegisterComponent();
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Companion = World->SpawnActor<AHearthwardCompanionFixture>(Start, FRotator::ZeroRotator, Params);
    if (Companion) Companion->InitializeFixture(Stock, Camp);
}

void SubmitCompanionFixture(const TArray<FString>& Args, UWorld* World)
{
    auto* Companion = FindCompanionFixture(World);
    auto* Player = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
    if (!Companion || !Player || Args.Num() != 1) return;
    int64 Count = 0;
    for (TCHAR Digit : Args[0])
    {
        if (Digit < TEXT('0') || Digit > TEXT('9')) return;
        Count = Count * 10 + (Digit - TEXT('0'));
        if (Count > MAX_int32) return;
    }
    if (Count == 0) return;
    const auto Ticket = Companion->Request(Player, TEXT("PROTOTYPE_ONLY structured fixture input"));
    const auto Result = Companion->Submit(Player, Ticket, TEXT("wood"), static_cast<int32>(Count), {TEXT("collect"), TEXT("return"), TEXT("deposit")});
    UE_LOG(LogTemp, Display, TEXT("Companion fixture submit: %s"), *UEnum::GetValueAsString(Result));
}

void CancelCompanionFixture(UWorld* World)
{
    if (auto* Companion = FindCompanionFixture(World)) Companion->Cancel(UGameplayStatics::GetPlayerPawn(World, 0));
}

FAutoConsoleCommandWithWorld CompanionCreateCommand(TEXT("Hearthward.Companion.CreateTest"),
    TEXT("PROTOTYPE_ONLY: finite 16 wood, 4-weight trips, flat swept lane; no map edits."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&CreateCompanionFixture));
FAutoConsoleCommandWithWorldAndArgs CompanionCollectCommand(TEXT("Hearthward.Companion.Collect"),
    TEXT("PROTOTYPE_ONLY structured goal: Collect <positive count>. No language model."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SubmitCompanionFixture));
FAutoConsoleCommandWithWorld CompanionCancelCommand(TEXT("Hearthward.Companion.Cancel"),
    TEXT("Cancel fixture within 30m; retain actual carried resources."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&CancelCompanionFixture));
}
#endif
