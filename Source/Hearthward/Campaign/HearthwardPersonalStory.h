#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/HearthwardInteractionTargetComponent.h"
#include "../Interaction/HearthwardPersistentInteractionState.h"
#include "HearthwardPersonalStory.generated.h"

namespace HearthwardPersonalStory
{
    const TArray<TSharedPtr<class FJsonValue>>& Rows();
    TSharedPtr<class FJsonObject> Row(FName Person);
    FHearthwardPersonChoiceState State(UWorld* World,FName Person);
}
UCLASS()
class HEARTHWARD_API AHearthwardPersonalStoryDirector : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardPersonalStoryDirector();
    virtual void Tick(float Delta) override;
    static AHearthwardPersonalStoryDirector* Find(UWorld* World);
    bool SetStep(FName Person,FName Step,FName Choice=NAME_None);
private:
    FGuid Epoch;
    TMap<FName,float> ObservationSeconds;
};
UCLASS()
class HEARTHWARD_API UHearthwardPersonalInteraction : public UHearthwardInteractionTargetComponent
{
    GENERATED_BODY()
public:
    UHearthwardPersonalInteraction(){MaxDistance=240;PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=.25f;}
    FName Identity;
    FName DisplayCamp;
    FName DisplaySlot;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> DisplayMesh;
    FName PreviewFact;
    FGuid PreviewEpoch;
    bool IsDisplay() const {return !DisplayCamp.IsNone();}
    virtual FString GetInteractionPrompt(AActor* Interactor) const override;
    virtual FString CompleteInteraction(AActor* Interactor) override;
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick) override;
    FString Describe() const;
    FString Choose(AActor* Player,FName Choice,FGuid Epoch);
    FString Preview(AActor* Player,FName Fact,FGuid Epoch);
    FString ConfirmDisplay(AActor* Player,FGuid Epoch);
    void CancelPreview();
private:
    bool CanUse(AActor* Player,FGuid Epoch) const;
    bool OwnsFact(FName Fact) const;
    void RefreshDisplay();
};
