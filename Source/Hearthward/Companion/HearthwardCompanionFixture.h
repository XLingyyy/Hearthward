#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HearthwardCompanionCommand.h"
#include "../AI/HearthwardAgentPlan.h"
#include "HearthwardCompanionFixture.generated.h"

class UHearthwardInventoryComponent;
class UHearthwardTimedActionComponent;
class UHearthwardCompanionNavigationComponent;

UENUM(BlueprintType)
enum class EHearthwardCompanionPhase : uint8
{
    Idle, GoingToSource, Gathering, Returning, ReturningBlocked, WaitingAtCamp, Completed, Cancelled,
    GoingToWorkshop, TakingMaterials, HoldingSafely
};

// Runtime companion. Development fixtures and the natural camp supply explicit world participants.
UCLASS(NotPlaceable)
class HEARTHWARD_API AHearthwardCompanionFixture : public ACharacter
{
    GENERATED_BODY()
public:
    virtual void FellOutOfWorld(const class UDamageType& DamageType) override;
    virtual void Landed(const FHitResult& Hit) override;
    AHearthwardCompanionFixture();
    virtual void Tick(float DeltaSeconds) override;
    void InitializeFixture(UHearthwardInventoryComponent* Resource, AActor* CampActor);
    void InitializeCompanion(UHearthwardInventoryComponent* Resource, AActor* CampActor);

    UFUNCTION(BlueprintCallable, Category="Hearthward|Companion|Prototype")
    FHearthwardCommandTicket Request(AActor* Speaker, const FString& PlayerStatement);
    UFUNCTION(BlueprintCallable, Category="Hearthward|Companion|Prototype")
    EHearthwardProposalResult Submit(AActor* Speaker, FHearthwardCommandTicket Ticket,
        FName ItemId, int32 Quantity, const TArray<FName>& Steps);
    UFUNCTION(BlueprintCallable, Category="Hearthward|Companion|Prototype")
    bool Cancel(AActor* Speaker);
    UFUNCTION(BlueprintPure, Category="Hearthward|Companion|Prototype")
    EHearthwardCompanionPhase GetPhase() const { return Phase; }
    UFUNCTION(BlueprintPure, Category="Hearthward|Companion|Prototype")
    int32 GetDelivered() const { return Command.GetDelivered(); }
    UFUNCTION(BlueprintPure, Category="Hearthward|Companion|Prototype")
    int32 GetRequested() const { return Command.GetRequested(); }
    UFUNCTION(BlueprintPure) int32 GetAcquired() const { return Command.GetAcquired(); }
    UFUNCTION(BlueprintPure) int32 GetCarried() const { return Command.GetCarried(); }
    UFUNCTION(BlueprintPure) FHearthwardAgentGoal GetGoal() const {return Command.Goal;}
    UFUNCTION(BlueprintPure) FGuid GetCommandId() const {return Command.GetActive().Id;}
    UFUNCTION(BlueprintPure) FString GetExecutionAction() const;
    UFUNCTION(BlueprintCallable) bool ResumeBlocked(AActor* Speaker);
    EHearthwardProposalResult SubmitGoal(AActor* Speaker,FHearthwardCommandTicket Ticket,const FHearthwardAgentGoal& Goal);
    FString PreviewGoal(const FHearthwardAgentGoal& Goal) const;
    UPROPERTY(BlueprintReadWrite) TMap<FName,float> OwnedDurability;
    UPROPERTY(BlueprintReadOnly) TMap<FName,int32> Spent;
    FName GetItem() const { return Command.GetItem(); }
    bool IsAtCamp() const;
    UFUNCTION(BlueprintPure, Category="Hearthward|Companion|Prototype")
    FString GetPlayerStatement() const { return Statement; }
    UFUNCTION(BlueprintPure, Category="Hearthward|Companion|Prototype")
    bool CanCommunicate(AActor* Speaker) const;
    bool IsProposalCurrent(AActor* Speaker, const FHearthwardCommandTicket& Ticket) const;
    void DiscardProposal(const FHearthwardCommandTicket& Ticket) { Command.DiscardPending(Ticket); }

    UPROPERTY(BlueprintReadOnly) TObjectPtr<UHearthwardInventoryComponent> Bag;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UHearthwardTimedActionComponent> Action;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UHearthwardCompanionNavigationComponent> Navigation;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UHearthwardInventoryComponent> Source;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Camp;
    // PROTOTYPE_ONLY safe-point evidence. Perception captures this input and the safety policy combines it
    // with current UE world state; player/model text never writes this value or the final verdict.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSourceSafe = false;
    UPROPERTY(BlueprintReadOnly) FString BlockReason;

    bool NavigateTo(AActor* Target, float Speed, float AcceptanceRadius);
    bool NavigateToLocation(const FVector& Location, float Speed, float AcceptanceRadius);
    void StopNavigation();
    void StopForSurvival();

private:
    friend class UHearthwardSaveSubsystem;
    bool At(const AActor* Target) const;
    bool MoveTowards(const AActor* Target, float DeltaSeconds,float AcceptanceRadius=40);
    void ReturnBlocked(const FString& Reason);
    void HandleExecutionFailure(const FString& Reason);
    void Deposit();
    bool IsSourceValid() const;
    EHearthwardProposalResult AcceptGoal(AActor* Speaker, FHearthwardCommandTicket Ticket,
        FName ItemId, int32 Quantity, const TArray<FName>& Steps, const FHearthwardAgentGoal& Goal);
    void WorkshopTick();
    bool BuildExecutionPlan(bool PreferReturnForExistingCargo=true);
    void RestoreExecutionPlan();
    void TickExecution(float DeltaSeconds);
    void TickRecovery(float DeltaSeconds);
    void AdvanceExecution();
    void SyncPhaseFromExecution();
    AActor* ResolveActionTarget(const FHearthwardAgentAction& Action) const;
    bool ActionRequiresSafety(const FHearthwardAgentAction& Action) const;
    void Event(FName Kind,FName Item,int32 Count,const FString& Reason=FString(),FGuid Operation=FGuid());

    FHearthwardCompanionCommand Command;
    FHearthwardAgentExecutionState Execution;
    TWeakObjectPtr<AActor> RequestSpeaker;
    FString Statement;
    EHearthwardCompanionPhase Phase = EHearthwardCompanionPhase::Idle;
    bool bSettling = false;
    bool bFixtureEnabled = false;
    int32 NavigationFailures = 0;
    FVector LastProgressPosition = FVector::ZeroVector;
    double LastProgressAt = 0;
    TSet<FGuid> AppliedOperations;
    TArray<FHearthwardAgentReceipt> Receipts;
};
