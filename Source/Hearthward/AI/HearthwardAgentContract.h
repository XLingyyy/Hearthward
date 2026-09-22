#pragma once
#include "CoreMinimal.h"
#include "HearthwardAgentContract.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardAgentGoal
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName Intent;
    UPROPERTY(BlueprintReadOnly) FName Item;
    UPROPERTY(BlueprintReadOnly) int32 Quantity = 0;
    UPROPERTY(BlueprintReadOnly) FString QuantityMode;
    UPROPERTY(BlueprintReadOnly) FString SourceRef;
    // Canonical typed forms: ban:wood, source:S1, no:herb, max:wood:6.
    UPROPERTY(BlueprintReadOnly) TArray<FString> Limits;
    UPROPERTY(BlueprintReadOnly) TArray<FString> Unresolved;
    UPROPERTY(BlueprintReadOnly) FString Line;
    UPROPERTY(BlueprintReadOnly) FString Original;
    UPROPERTY(BlueprintReadOnly) FGuid Station;
    UPROPERTY(BlueprintReadOnly) int64 RuleRevision = 0;
    UPROPERTY(BlueprintReadOnly) int32 CapabilityVersion = 2;
    bool WritesWorld() const { return Intent==TEXT("collect") || Intent==TEXT("craft") || Intent==TEXT("repair") || Intent==TEXT("companion_order"); }
};

struct FHearthwardAgentCapability
{
    FName Id;
    FString Description;
    TArray<FName> Items;
    int32 MaxQuantity;
    FString QuantityMode;
    TArray<FString> Sources;
    TArray<FString> Constraints;
    bool Writes;
};

USTRUCT()
struct FHearthwardAgentReceipt
{
    GENERATED_BODY()
    UPROPERTY() FGuid Id;
    UPROPERTY() FGuid Command;
    UPROPERTY() FString Payload;
};

namespace HearthwardAgent
{
    const TArray<FHearthwardAgentCapability>& Capabilities();
    FString Describe();
    FString Schema();
    bool Parse(const FString& Json,FHearthwardAgentGoal& Out);
    FString Validate(const FHearthwardAgentGoal& Goal);
    bool ValidLimit(const FString& Limit);
    bool AllowsCost(const TArray<FString>& Limits,const TMap<FName,int32>& Cost,const TMap<FName,int32>& AlreadySpent);
    FString Normalize(const FString& Text);
    FString GoalText(const FHearthwardAgentGoal& Goal);
    FString ItemText(FName Item);
    FString LimitText(const FString& Limit);
    FString EventText(FName Kind);
    int32 Policy(const TCHAR* Key);
    // Caller holds the settlement guard through effect, progress and receipt publication.
    bool Settle(TArray<FHearthwardAgentReceipt>& Receipts,FGuid Id,FGuid Command,
        FGuid Epoch,FGuid CurrentEpoch,const FString& Payload,TFunctionRef<bool()> Effect);
}
