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
    const FHearthwardNPCMemory& GetMemorySnapshot() const { return Memory; }
    void RestoreMemory(const FHearthwardNPCMemory& Snapshot) { Memory = Snapshot; }

protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
private:
    bool StartServer();
    void StopServer();
    void PollHealth();
    void SendInference();
    bool StillCurrent() const;
    void ApplyProposal();
    void Fail(const FString& Message);
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
    TWeakObjectPtr<AActor> PendingSpeaker;
    TWeakObjectPtr<AHearthwardCompanionFixture> PendingCompanion;
    FHearthwardCommandTicket Ticket;
    FHearthwardAIProposal Proposal;
    uint64 Serial = 0;
    bool bPending = false, bReady = false, bResponseReady = false;
    double StartedAt = 0, RequestStartedAt = 0, NextHealthAt = 0, LastLatencySeconds = 0;
};
