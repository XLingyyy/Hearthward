#pragma once
#include "CoreMinimal.h"
#include "HearthwardNPCInitiative.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardNPCInitiative
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid Id;
    UPROPERTY(BlueprintReadOnly) FName Kind;
    UPROPERTY(BlueprintReadOnly) FName Item;
    UPROPERTY(BlueprintReadOnly) FString Message;
    UPROPERTY(BlueprintReadOnly) FString DedupeKey;
    UPROPERTY(BlueprintReadOnly) double CreatedAt = 0;
    UPROPERTY(BlueprintReadOnly) double VisibleSeconds = 8;
    UPROPERTY(BlueprintReadOnly) FGuid EvidenceId;

    bool IsValid() const { return Id.IsValid() && !Kind.IsNone() && !Message.IsEmpty() && !DedupeKey.IsEmpty(); }
};

namespace HearthwardInitiative
{
    FHearthwardNPCInitiative FromEvent(FName Kind,FName Item,int32 Count,const FString& Reason,FGuid EvidenceId,double Now);
    FHearthwardNPCInitiative BeliefCorrection(FName Item,int32 Reported,int32 Confirmed,double Now);
}
