#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HearthwardHometownFortress.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// The home district has its own architecture; camp houses keep their existing kit.
UCLASS()
class HEARTHWARD_API AHearthwardHometownFortress : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardHometownFortress();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    FVector BedroomLanding() const;
    FVector RelicPosition() const;
private:
    float BedroomFloor = 120;
    int32 PartIndex = 0;
    UPROPERTY() TObjectPtr<UMaterialInterface> Stone;
    UPROPERTY() TObjectPtr<UMaterialInterface> Timber;
    UPROPERTY() TObjectPtr<class UStaticMesh> DoorframeMesh;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FacadeMaterial;
    float Terrain(float X, float Y) const;
    UStaticMeshComponent* Part(FString Label, FVector Center, FVector Size, bool Blocking = true,
                               UMaterialInterface* Material = nullptr);
    void Furniture(const TCHAR* Label, const TCHAR* Path, FVector Position, float Scale);
    void Wall(FVector2D A, FVector2D B, float Height, float Width = 110);
    void Tower(FVector2D Center, float Width, float Height);
    void Lamp(FVector Position);
};
