#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "HAL/PlatformProcess.h"
#include "../Companion/HearthwardCompanionCommand.h"
#include "HearthwardLocalAIContext.h"
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
