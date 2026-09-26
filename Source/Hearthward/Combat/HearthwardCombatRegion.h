#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "HearthwardCombatRegion.generated.h"

// Authored enemy territory. No production region or lighting is inferred from enemy distance.
UCLASS()
class HEARTHWARD_API AHearthwardCombatRegion : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardCombatRegion()
    {
        Bounds=CreateDefaultSubobject<UBoxComponent>(TEXT("Region")); SetRootComponent(Bounds);
        Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Bounds;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName RegionId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float Lighting=-1;
    bool Contains(FVector Point) const
    { const FVector P=Bounds->GetComponentTransform().InverseTransformPosition(Point); return FBox(-Bounds->GetUnscaledBoxExtent(),Bounds->GetUnscaledBoxExtent()).IsInsideOrOn(P); }
};
