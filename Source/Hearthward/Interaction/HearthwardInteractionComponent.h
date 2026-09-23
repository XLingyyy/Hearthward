#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardInteractionComponent.generated.h"

class UHearthwardInteractionTargetComponent;
class UHearthwardTimedActionComponent;

UENUM(BlueprintType)
enum class EHearthwardInteractionStatus : uint8
{
    Idle, Running, Ready, Interrupted, InvalidTarget, Unconfigured, OutOfRange, Paused, Busy
};

UCLASS(ClassGroup=(Hearthward), meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardInteractionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardInteractionComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="Hearthward|Interaction")
    bool BeginInteraction(UHearthwardInteractionTargetComponent* Target);
    UFUNCTION(BlueprintCallable, Category="Hearthward|Interaction")
    bool InteractNearest();
    UFUNCTION(BlueprintPure, Category="Hearthward|Interaction")
    UHearthwardInteractionTargetComponent* GetNearestTarget() const;
    UFUNCTION(BlueprintPure, Category="Hearthward|Interaction")
    FString GetCompletionFeedback() const { return Status == EHearthwardInteractionStatus::Ready ? CompletionFeedback : FString(); }
    UFUNCTION(BlueprintPure, Category="Hearthward|Interaction")
    EHearthwardInteractionStatus GetStatus() const { return Status; }
    uint32 GetFeedbackRevision() const { return FeedbackRevision; }
    UHearthwardInteractionTargetComponent* GetActiveTarget() const { return bActive ? PendingTarget.Get() : nullptr; }
private:
    friend class UHearthwardSaveSubsystem;
    EHearthwardInteractionStatus ValidateTarget(UHearthwardInteractionTargetComponent* Target) const;
    void SetStatus(EHearthwardInteractionStatus Value) { Status = Value; ++FeedbackRevision; }
    void Cancel(EHearthwardInteractionStatus Reason);
    UFUNCTION()
    void TimerCompleted();
    UFUNCTION()
    void TimerInterrupted();

    UPROPERTY(Transient)
    TObjectPtr<UHearthwardTimedActionComponent> Action;
    TWeakObjectPtr<UHearthwardInteractionTargetComponent> PendingTarget;
    bool bActive = false;
    EHearthwardInteractionStatus Status = EHearthwardInteractionStatus::Idle;
    uint32 FeedbackRevision = 0;
    FString CompletionFeedback;
};
