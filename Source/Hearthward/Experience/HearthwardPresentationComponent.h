#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardPresentationComponent.generated.h"
UCLASS(ClassGroup=(Hearthward),meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardPresentationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardPresentationComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick) override;
    UFUNCTION(BlueprintCallable) bool PlayFixedCue(FName Id,FGuid Event,bool SharedKnowledge);
    UFUNCTION(BlueprintCallable) void StopFixedCue();
    UFUNCTION(BlueprintPure) FString GetSubtitle() const;
    UFUNCTION(BlueprintPure) FName GetCue() const {return CurrentCue;}
    UFUNCTION(BlueprintPure) FString GetVoiceStatus() const {return VoiceStatus;}
private:
    UFUNCTION() void Restored();
    UPROPERTY() TObjectPtr<class UAudioComponent> Voice;
    UPROPERTY() TObjectPtr<class USoundWaveProcedural> Wave;
    struct FCue {FString Speaker,Text,Group;};
    TMap<FName,FCue> Cues;
    TSet<FGuid> PlayedEvents;
    FName CurrentCue;
    FGuid Epoch;
    float Remaining=0,ShakeRemaining=0,PreviousHealth=100;
    FString VoiceStatus;
    TWeakObjectPtr<class AHearthwardCompanionFixture> ObservedBrother;
    FGuid PreviousCommand;
    uint8 PreviousPhase=0;
    int32 PreviousDelivered=0;
    FName PreviousInitiative;
};
