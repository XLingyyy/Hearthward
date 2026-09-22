#pragma once

#include "CoreMinimal.h"
#include "HearthwardNPCSuggestions.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardNPCSuggestion
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FGuid Id;
    UPROPERTY(BlueprintReadOnly) FName Kind;
    UPROPERTY(BlueprintReadOnly) FString Label;
    UPROPERTY(BlueprintReadOnly) FString Message;
    UPROPERTY(BlueprintReadOnly) FName Item;
    UPROPERTY(BlueprintReadOnly) int32 ObservedCount = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) FGuid TimelineEpoch;
    UPROPERTY(BlueprintReadOnly) int64 MemoryRevision = 0;
    UPROPERTY(BlueprintReadOnly) FGuid CommandId;
};

struct FHearthwardSuggestionContext
{
    bool bHasActiveCommand = false;
    bool bCanCollectWood = false;
    bool bHasCampWood = false;
    int32 CampWood = 0;
};

namespace HearthwardSuggestions
{
    // Pure generator. It sees only caller-approved facts and never writes memory or world state.
    TArray<FHearthwardNPCSuggestion> Build(const FHearthwardSuggestionContext& Context);
}
