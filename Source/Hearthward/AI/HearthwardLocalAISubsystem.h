#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "HAL/PlatformProcess.h"
#include "../Companion/HearthwardCompanionCommand.h"
#include "HearthwardLocalAIContext.h"
#include "HearthwardNPCMemory.h"
#include "HearthwardLocalAISubsystem.generated.h"

class AHearthwardCompanionFixture;

UCLASS()
class HEARTHWARD_API UHearthwardLocalAISubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UHearthwardLocalAISubsystem, STATGROUP_Tickables); }

    UFUNCTION(BlueprintCallable, Category="Hearthward|AI")
    bool SubmitPlayerText(AActor* Speaker, AHearthwardCompanionFixture* Companion, const FString& Text);
    UFUNCTION(BlueprintCallable, Category="Hearthward|AI")
    void CancelPending();
    void ResetForSnapshot();
    UFUNCTION(BlueprintPure, Category="Hearthward|AI")
    FString GetStatus() const { return Status; }
    UFUNCTION(BlueprintPure, Category="Hearthward|AI")
    FString GetNPCLine() const { return NPCLine; }
    UFUNCTION(BlueprintPure, Category="Hearthward|AI")
    FString GetLastStructuredResult() const { return LastStructuredResult; }
    UFUNCTION(BlueprintPure, Category="Hearthward|AI")
    FString GetLastFilteredContext() const { return LastFilteredContext; }
    UFUNCTION(BlueprintPure, Category="Hearthward|AI")
    bool IsBusy() const { return bPending; }
    UFUNCTION(BlueprintPure, Category="Hearthward|AI")
    bool IsModelReady() const { return bReady; }
    double GetElapsedSeconds() const {return bPending?FPlatformTime::Seconds()-RequestStartedAt:LastLatencySeconds;}
    UFUNCTION(BlueprintPure, Category="Hearthward|AI")
    double GetLastLatencySeconds() const { return LastLatencySeconds; }
    UFUNCTION(BlueprintPure, Category="Hearthward|AI")
    int32 GetServerProcessId() const { return static_cast<int32>(ProcessId); }
    bool CanDisplay() const;
    UFUNCTION(BlueprintPure) TArray<FHearthwardPlayerMemory> GetPlayerMemories() const { return Memory.Records; }
    UFUNCTION(BlueprintCallable) bool PutPlayerMemory(AActor* Speaker, AHearthwardCompanionFixture* Companion, FGuid Id, FName Kind, const FString& Text, FName BlockedItem = NAME_None);
    UFUNCTION(BlueprintCallable) bool RevokePlayerMemory(AActor* Speaker, AHearthwardCompanionFixture* Companion, FGuid Id);
    UFUNCTION(BlueprintCallable) void ClearClarification();
    UFUNCTION(BlueprintPure) int32 GetClarificationTurns() const { return Memory.Clarification.Num(); }
    UFUNCTION(BlueprintPure) FString GetLastAppliedIntent() const { return LastAppliedIntent; }
    UFUNCTION(BlueprintPure) bool HasCandidate() const {return CandidateId.IsValid();}
    UFUNCTION(BlueprintPure) FGuid GetCandidateId() const {return CandidateId;}
    UFUNCTION(BlueprintPure) FString GetCandidateText() const;
    UFUNCTION(BlueprintPure) FHearthwardAgentGoal GetCandidate() const {return Candidate;}
    UFUNCTION(BlueprintPure) int64 GetMemoryRevision() const {return Memory.Revision;}
    UFUNCTION(BlueprintPure) int32 GetInputTokens() const {return InputTokens;}
    UFUNCTION(BlueprintPure) int32 GetOutputTokens() const {return OutputTokens;}
    UFUNCTION(BlueprintPure) FString GetReasonCode() const {return ReasonCode;}
    UFUNCTION(BlueprintPure) FString GetLastInput() const {return Input;}
    UFUNCTION(BlueprintPure) TArray<FHearthwardNPCEvent> GetEvents() const {return Memory.Events;}
    UFUNCTION(BlueprintCallable) bool ConfirmCandidate(FGuid Id);
    UFUNCTION(BlueprintCallable) bool AdjustCandidate(FGuid Id,int32 Delta);
    UFUNCTION(BlueprintCallable) bool QueryInventory(AActor* Speaker,AHearthwardCompanionFixture* Companion,FName Item);
    UFUNCTION(BlueprintCallable) bool CancelExecution(AActor* Speaker,AHearthwardCompanionFixture* Companion);
    UFUNCTION(BlueprintCallable) bool SetStructuredGoal(AActor* Speaker,AHearthwardCompanionFixture* Companion,const FHearthwardAgentGoal& Goal);
    void RecordEvent(const FHearthwardNPCEvent& E) {Memory.RecordEvent(E);}
    const FHearthwardNPCMemory& GetMemorySnapshot() const { return Memory; }
    void RestoreMemory(const FHearthwardNPCMemory& Snapshot);

protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
private:
    bool StartServer();
    void StopServer();
    void PollHealth();
    void SendInference();
    void CountRequest(const TSharedPtr<FJsonObject>& Body);
    void Generate(const TSharedPtr<FJsonObject>& Body);
    void StageCandidate(FHearthwardAgentGoal Goal);
    bool StillCurrent() const;
    void ApplyProposal();
    void Fail(const FString& Message,const FString& Code=TEXT("MODEL_UNAVAILABLE"));
    FString BuildFilteredContext() const;
    void ObserveCamp();
    FHearthwardNPCMemory Memory;
    FString LastAppliedIntent;

    FProcHandle Process;
    void* JobHandle = nullptr;
    uint32 ProcessId = 0;
    FString BaseUrl, ApiKey, BundlePath;
    FString Status, NPCLine, LastStructuredResult, LastFilteredContext, Input;
    FHttpRequestPtr Request;
    FHttpRequestPtr HealthRequest;
    TWeakObjectPtr<AActor> PendingSpeaker;
    TWeakObjectPtr<AHearthwardCompanionFixture> PendingCompanion;
    FHearthwardCommandTicket Ticket;
    FHearthwardAgentGoal Proposal, Candidate;
    FGuid CandidateId;
    int64 CandidateMemoryRevision = 0;
    FString ReasonCode;
    int32 InputTokens = 0, OutputTokens = 0, FailureCount = 0;
    uint64 Serial = 0;
    bool bPending = false, bReady = false, bResponseReady = false;
    double StartedAt = 0, RequestStartedAt = 0, NextHealthAt = 0, LastLatencySeconds = 0;
};
