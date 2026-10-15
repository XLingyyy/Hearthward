#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HearthwardNatureState.h"
#include "../Interaction/HearthwardPersistentInteractionState.h"
#include "../Interaction/HearthwardInteractionTargetComponent.h"
#include "HearthwardAnimalTraces.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardTrailInteraction : public UHearthwardInteractionTargetComponent
{
    GENERATED_BODY()
public:
    UHearthwardTrailInteraction(){MaxDistance=250;}
    virtual FString GetInteractionPrompt(AActor*) const override {return TEXT("E 查看地面踪迹");}
    virtual FString CompleteInteraction(AActor* Player) override;
};
UCLASS()
class HEARTHWARD_API AHearthwardAnimalTraces : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardAnimalTraces();
    virtual void Tick(float Delta) override;
    static bool Sample(const FHearthwardNatureState& Nature,double Calendar,const FVector& Center,FHearthwardPersistentInteractionState& State);
    static FString DescribeAt(UWorld* World,const FVector& Position,double Radius=12000);
private:
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Prints;
    UPROPERTY() TObjectPtr<UHearthwardTrailInteraction> Interaction;
    int64 RenderedRevision=-1;
    FGuid RenderedEpoch;
};
