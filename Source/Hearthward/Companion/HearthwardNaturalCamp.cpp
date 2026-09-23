#include "HearthwardNaturalCamp.h"
#include "HearthwardCompanionFixture.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"

namespace
{
bool Ground(UWorld* World, FVector& Position)
{
    FHitResult Hit;
    if (!World->LineTraceSingleByChannel(Hit, Position + FVector(0,0,2000),
        Position - FVector(0,0,5000), ECC_WorldStatic)) return false;
    Position = Hit.ImpactPoint + FVector(0,0,80);
    return Hit.ImpactNormal.Z > .7;
}

AActor* Marker(UWorld* World, FVector Position, FVector Scale, const TCHAR* Label)
{
    auto* Actor = World->SpawnActor<AActor>();
    auto* Root = NewObject<USceneComponent>(Actor);
    Actor->AddInstanceComponent(Root); Actor->SetRootComponent(Root); Root->RegisterComponent();
    auto* Mesh = NewObject<UStaticMeshComponent>(Actor);
    Actor->AddInstanceComponent(Mesh); Mesh->SetupAttachment(Root);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetRelativeScale3D(Scale); Mesh->SetRelativeLocation(FVector(0,0,-80+Scale.Z*50));
    Mesh->RegisterComponent();
    auto* Text = NewObject<UTextRenderComponent>(Actor);
    Actor->AddInstanceComponent(Text); Text->SetupAttachment(Root);
    Text->SetText(FText::FromString(Label)); Text->SetWorldSize(24);
    Text->SetRelativeLocation(FVector(0,0,65)); Text->RegisterComponent();
    Actor->SetActorLocation(Position);
    return Actor;
}
}

bool HearthwardNaturalCamp::Initialize(UWorld* World, FString& Error)
{
    for (TActorIterator<AHearthwardCompanionFixture> It(World); It; ++It) return true;
    APlayerStart* Start = nullptr;
    for (TActorIterator<APlayerStart> It(World); It; ++It) { Start=*It; break; }
    if (!Start) { Error=TEXT("营地出生点尚未加载"); return false; }
    const FVector Forward = Start->GetActorForwardVector(), Right = Start->GetActorRightVector();
    FVector CampPosition=Start->GetActorLocation()+Right*250;
    FVector SourcePosition=CampPosition+Forward*600;
    FVector CompanionPosition=CampPosition+Right*120;
    if (!Ground(World,CampPosition) || !Ground(World,SourcePosition) || !Ground(World,CompanionPosition))
    { Error=TEXT("营地地形尚未就绪，请稍后重试"); return false; }

    auto* Camp=Marker(World,CampPosition,FVector(1.2,1.2,.4),TEXT("营地仓储"));
    auto* Resource=Marker(World,SourcePosition,FVector(.7,.7,.7),TEXT("木材采集点"));
    Camp->Tags.Add(TEXT("Hearthward.NaturalCamp"));
    Resource->Tags.Add(TEXT("Hearthward.NaturalCamp.Resource"));
    auto* Stock=NewObject<UHearthwardInventoryComponent>(Resource);
    Resource->AddInstanceComponent(Stock); Stock->RegisterComponent(); Stock->TryAdd(TEXT("wood"),16);
    for (AActor* Target : {Camp,Resource})
    {
        auto* Interaction=NewObject<UHearthwardResourceInteractionComponent>(Target);
        Target->AddInstanceComponent(Interaction); Interaction->SetupAttachment(Target->GetRootComponent());
        Interaction->InitializeResource(Target==Camp); Interaction->RegisterComponent();
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Companion=World->SpawnActor<AHearthwardCompanionFixture>(CompanionPosition,Start->GetActorRotation(),Params);
    Companion->InitializeCompanion(Stock,Camp);

    // Bounds cover the authored 4.032 km world; invokers generate only nearby loaded terrain.
    auto* Volume=World->SpawnActor<ANavMeshBoundsVolume>();
    Volume->GetRootComponent()->SetMobility(EComponentMobility::Movable);
    auto* Box=NewObject<UBoxComponent>(Volume);
    Volume->AddInstanceComponent(Box); Box->SetupAttachment(Volume->GetRootComponent());
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision); Box->SetCanEverAffectNavigation(false);
    Box->SetBoxExtent(FVector(205000,205000,100000)); Box->RegisterComponent();
    Volume->SetActorLocation(FVector::ZeroVector);
    if (auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World)) Nav->OnNavigationBoundsUpdated(Volume);
    return true;
}
