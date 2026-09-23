#pragma once
#include "CoreMinimal.h"
#include "HearthwardAgentContract.h"
#include "HearthwardNPCBelief.h"
#include "HearthwardNPCMemory.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardPlayerMemory
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid Id;
    UPROPERTY(BlueprintReadOnly) FName Kind;
    UPROPERTY(BlueprintReadOnly) FString Text;
    UPROPERTY(BlueprintReadOnly) double RecordedAt = 0;
    UPROPERTY(BlueprintReadOnly) bool Revoked = false;
    UPROPERTY(BlueprintReadOnly) FName BlockedItem;
    UPROPERTY(BlueprintReadOnly) int64 Revision = 1;
    UPROPERTY(BlueprintReadOnly) FGuid Campaign;
    UPROPERTY(BlueprintReadOnly) FString Constraint;
};

USTRUCT(BlueprintType)
struct FHearthwardNPCEvent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid Id;
    UPROPERTY(BlueprintReadOnly) FGuid Command;
    UPROPERTY(BlueprintReadOnly) FGuid Campaign;
    UPROPERTY(BlueprintReadOnly) FName Kind;
    UPROPERTY(BlueprintReadOnly) FName Item;
    UPROPERTY(BlueprintReadOnly) int32 Count = 0;
    UPROPERTY(BlueprintReadOnly) double At = 0;
    UPROPERTY(BlueprintReadOnly) FString Reason;
};

UENUM(BlueprintType)
enum class EHearthwardNPCEpisodeCoverage : uint8
{
    Unknown,
    Complete,
    Truncated
};

USTRUCT(BlueprintType)
struct FHearthwardNPCCommandCoverage
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid Command;
    UPROPERTY(BlueprintReadOnly) EHearthwardNPCEpisodeCoverage Coverage = EHearthwardNPCEpisodeCoverage::Unknown;
    // Active commands are retained even when every currently buffered event has been evicted.
    UPROPERTY(BlueprintReadOnly) bool Active = false;
};

USTRUCT()
struct FHearthwardClarificationTurn
{
    GENERATED_BODY()
    UPROPERTY() FString Player;
    UPROPERTY() FString Question;
};

// Owned by the NPC, copied atomically with the world. No model-generated facts.
USTRUCT()
struct FHearthwardNPCMemory
{
    GENERATED_BODY()
    UPROPERTY() TArray<FHearthwardPlayerMemory> Records;
    UPROPERTY() TArray<FHearthwardClarificationTurn> Clarification;
    UPROPERTY() bool HasCampObservation = false;
    // Legacy compatibility snapshot. It is not a parallel model fact source.
    UPROPERTY() TMap<FName, int32> CampInventory;
    UPROPERTY() double CampObservedAt = 0;
    UPROPERTY() int64 Revision = 1;
    UPROPERTY() FGuid Campaign;
    UPROPERTY() TArray<FHearthwardNPCEvent> Events;
    UPROPERTY() TArray<FHearthwardNPCBelief> Beliefs;
    UPROPERTY() TArray<FHearthwardNPCCommandCoverage> CommandCoverage;
    UPROPERTY() FHearthwardAgentGoal WorkingGoal;

    static constexpr int32 MaxRecords = 64;
    static constexpr int32 MaxText = 120;
    static constexpr int32 MaxAgreements = 4;
    static constexpr int32 MaxClarificationCharacters = 800;
    bool Put(FGuid Id, FName Kind, const FString& Text, double Now, FName BlockedItem = NAME_None);
    bool Revoke(FGuid Id);
    bool AddClarification(const FString& Player, const FString& Question);
    TArray<FHearthwardPlayerMemory> Retrieve(const FString& Query, bool IncludeAgreements = true) const;
    bool BlocksCollection(FName Item) const;
    bool IsValid(double Now) const;
    bool PutRule(const FString& Constraint,const FString& Original,double Now);
    TArray<FString> ApplicableRules(FName Capability) const;
    void BeginCommand(FGuid Command);
    EHearthwardNPCEpisodeCoverage CoverageFor(FGuid Command) const;
    void RecordEvent(const FHearthwardNPCEvent& Event);
    // bLegacyCognition is only true after an explicitly identified older save version.
    void Migrate(FGuid CampaignId, bool bLegacyCognition=false, FGuid ActiveCommand=FGuid());
};
