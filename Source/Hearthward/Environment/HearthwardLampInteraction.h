#pragma once
#include "CoreMinimal.h"
#include "../Interaction/HearthwardInteractionTargetComponent.h"
#include "HearthwardLampInteraction.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardLampInteraction : public UHearthwardInteractionTargetComponent
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UHearthwardLampInteraction(){MaxDistance=200;PrimaryComponentTick.bCanEverTick=true;}
    FName Identity;
    bool InitiallyLit=true;
    UPROPERTY() TObjectPtr<class UPointLightComponent> Light;
    bool IsLit() const;
    virtual FString GetInteractionPrompt(AActor* Interactor) const override;
    virtual FString CompleteInteraction(AActor* Interactor) override;
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick) override;
    static double Illumination(UWorld* World,const FVector& Position,double Ambient,const AActor* Subject=nullptr);
};
