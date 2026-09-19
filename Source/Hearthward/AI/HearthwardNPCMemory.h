#pragma once
#include "CoreMinimal.h"
#include "HearthwardAgentContract.h"
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
    UPROPERTY() TMap<FName, int32> CampInventory;
    UPROPERTY() double CampObservedAt = 0;
    UPROPERTY() int64 Revision = 1;
    UPROPERTY() FGuid Campaign;
    UPROPERTY() TArray<FHearthwardNPCEvent> Events;
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
    void RecordEvent(const FHearthwardNPCEvent& Event);
    void Migrate(FGuid CampaignId);
};
