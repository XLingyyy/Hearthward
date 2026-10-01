#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/HearthwardInteractionTargetComponent.h"
#include "HearthwardNatureActor.generated.h"
UCLASS()
class HEARTHWARD_API UHearthwardNatureInteraction : public UHearthwardInteractionTargetComponent
{
    GENERATED_BODY()
public:
    virtual FString GetInteractionPrompt(AActor* Interactor) const override;
    virtual FString CompleteInteraction(AActor* Interactor) override;
};
UCLASS()
class HEARTHWARD_API AHearthwardNatureActor : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardNatureActor();
    FGuid Id;
    FName Kind,Definition;
    FString InteractionText;
    UPROPERTY() TObjectPtr<class UHearthwardCombatTargetComponent> Combat;
    UPROPERTY() TObjectPtr<class UHearthwardAnimalMotionComponent> AnimalMotion;
    void Configure(FGuid Entity,FName Type,FName Def);
    void Refresh();
    virtual void Tick(float Delta) override;
private:
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Shape;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
    UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Parts;
    UPROPERTY() TObjectPtr<UHearthwardNatureInteraction> Interaction;
    double AttackDelay=0,WanderDelay=0,Age=0;
    double PathRemaining=0;
    FVector PathNext=FVector::ZeroVector;
    FVector AdultScale=FVector::OneVector;
    void MoveAnimal(float Delta);
};
