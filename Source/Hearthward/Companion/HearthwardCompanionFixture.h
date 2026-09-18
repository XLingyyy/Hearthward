#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HearthwardCompanionCommand.h"
#include "HearthwardCompanionFixture.generated.h"

class UHearthwardInventoryComponent;
class UHearthwardTimedActionComponent;

UENUM(BlueprintType)
enum class EHearthwardCompanionPhase : uint8
{
    Idle, GoingToSource, Gathering, Returning, ReturningBlocked, WaitingAtCamp, Completed, Cancelled
};

// Runtime-only development fixture: no final resource, bag, navigation or safety defaults.
UCLASS(NotPlaceable)
class HEARTHWARD_API AHearthwardCompanionFixture : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardCompanionFixture();
    virtual void Tick(float DeltaSeconds) override;
    void InitializeFixture(UHearthwardInventoryComponent* Resource, AActor* CampActor);

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
    FName GetItem() const { return Command.GetItem(); }
    UFUNCTION(BlueprintPure, Category="Hearthward|Companion|Prototype")
    FString GetPlayerStatement() const { return Statement; }
    UFUNCTION(BlueprintPure, Category="Hearthward|Companion|Prototype")
    bool CanCommunicate(AActor* Speaker) const;
    bool IsProposalCurrent(AActor* Speaker, const FHearthwardCommandTicket& Ticket) const;
    void DiscardProposal(const FHearthwardCommandTicket& Ticket) { Command.DiscardPending(Ticket); }

    UPROPERTY(BlueprintReadOnly) TObjectPtr<UHearthwardInventoryComponent> Bag;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UHearthwardTimedActionComponent> Action;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UHearthwardInventoryComponent> Source;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Camp;
    // Only fixture/world authority sets this; Submit never accepts a model's safety claim.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSourceSafe = false;
    UPROPERTY(BlueprintReadOnly) FString BlockReason;

private:
    friend class UHearthwardSaveSubsystem;
    bool At(const AActor* Target) const;
    bool MoveTowards(const AActor* Target, float DeltaSeconds);
    void ReturnBlocked(const FString& Reason);
    void Deposit();
    bool IsSourceValid() const;

    FHearthwardCompanionCommand Command;
    TWeakObjectPtr<AActor> RequestSpeaker;
    FString Statement;
    EHearthwardCompanionPhase Phase = EHearthwardCompanionPhase::Idle;
    bool bSettling = false;
    bool bFixtureEnabled = false;
};
