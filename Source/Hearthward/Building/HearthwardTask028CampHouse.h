#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HearthwardTask028CampHouse.generated.h"

class USceneComponent;
class UStaticMeshComponent;

// TASK-028's authored camp house is spawned only in the rebuilt natural map.
// Its walkable collision is independent of the visual Tripo meshes.
UCLASS()
class HEARTHWARD_API AHearthwardTask028CampHouse : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardTask028CampHouse();
    virtual void BeginPlay() override;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> HouseRoot;
    UStaticMeshComponent* AddVisual(const TCHAR* Name, const TCHAR* Path, FVector Position,
                                    FVector Scale, FRotator Rotation = FRotator::ZeroRotator);
    void AddBlockingBox(const TCHAR* Name, FVector Position, FVector Extent);
};
