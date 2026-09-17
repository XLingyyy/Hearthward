#include "HearthwardInteractionTargetComponent.h"
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
void CreateTestTarget(UWorld* World)
{
    APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
    if (!Pawn) return;
    const FName Tag(TEXT("Hearthward.Interaction.PROTOTYPE_ONLY"));
    for (TActorIterator<AActor> It(World); It; ++It) if (It->ActorHasTag(Tag)) return;
    auto* Actor = World->SpawnActor<AActor>();
    if (!Actor) return;
    Actor->Tags.Add(Tag);
    auto* Mesh = NewObject<UStaticMeshComponent>(Actor);
    Actor->AddInstanceComponent(Mesh);
    Actor->SetRootComponent(Mesh);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->RegisterComponent();
    auto* Target = NewObject<UHearthwardInteractionTargetComponent>(Actor);
    Actor->AddInstanceComponent(Target);
    Target->SetupAttachment(Mesh);
    // PROTOTYPE_ONLY: explicit fixture tuning, not an approved interaction-distance default.
    Target->MaxDistance = 200.0f;
    Target->RegisterComponent();
    Actor->SetActorLocation(Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 150.0f);
}
FAutoConsoleCommandWithWorld TestTargetCommand(TEXT("Hearthward.Interaction.CreateTestTarget"),
    TEXT("PROTOTYPE_ONLY: create a runtime cube with 200cm interaction range; grants no resources or buildings."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&CreateTestTarget));
}
#endif
