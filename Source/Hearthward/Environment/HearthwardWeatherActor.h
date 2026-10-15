#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HearthwardWeatherActor.generated.h"

UCLASS()
class HEARTHWARD_API AHearthwardWeatherActor : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardWeatherActor();
    virtual void Tick(float Delta) override;
    static double RainAt(double Calendar,int32 Seed);
    static double Rain(UWorld* World);
    static bool Sheltered(UWorld* World,const FVector& Position,const AActor* Subject=nullptr);
    static double VisibilityFactor(UWorld* World,const FVector& Position,const AActor* Subject=nullptr);
private:
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Drops;
    TArray<FVector> DropPositions;
    TArray<bool> Covered;
    float RoofCheck=0;
};
