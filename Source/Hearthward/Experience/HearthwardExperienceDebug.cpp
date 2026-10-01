#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
namespace
{
void ExperienceFixture(const TArray<FString>& Args,UWorld* World)
{
    if(!World || !World->IsGameWorld() || Args.IsEmpty()) return;
    const FVector Origin(5000,5000,5000);
    auto Cube=[&](FName Tag,FVector Offset,FVector Size)
    {
        auto* Actor=World->SpawnActor<AStaticMeshActor>(Origin+Offset,FRotator::ZeroRotator);
        Actor->Tags.Add(TEXT("PROTOTYPE_ONLY.Task051"));Actor->Tags.Add(Tag);
        auto* Mesh=Actor->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));Actor->SetActorScale3D(Size/100);
    };
    if(Args[0]==TEXT("create"))
    {
        Cube(TEXT("Task051VaultFloor"),{0,0,-50},{1000,1000,100});
        Cube(TEXT("Task051VaultBarrier"),{165,0,50},{200,300,100});
    }
    else if(Args[0]==TEXT("ceiling")) Cube(TEXT("Task051VaultCeiling"),{0,0,220},{200,300,40});
    else if(Args[0]==TEXT("clear"))
        for(TActorIterator<AStaticMeshActor> It(World);It;++It) if(It->ActorHasTag(TEXT("PROTOTYPE_ONLY.Task051"))) It->Destroy();
}
FAutoConsoleCommandWithWorldAndArgs FixtureCommand(TEXT("Hearthward.ExperienceFixture"),
    TEXT("PROTOTYPE_ONLY: create | ceiling | clear. Explicit PIE collision fixture; no assets saved."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExperienceFixture));
}
#endif
