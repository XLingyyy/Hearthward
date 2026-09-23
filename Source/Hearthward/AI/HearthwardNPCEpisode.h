#pragma once
#include "CoreMinimal.h"
#include "HearthwardNPCMemory.h"
#include "HearthwardNPCEpisode.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardNPCEpisode
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid Command;
    UPROPERTY(BlueprintReadOnly) FName Item;
    UPROPERTY(BlueprintReadOnly) int32 Acquired = 0;
    UPROPERTY(BlueprintReadOnly) int32 Delivered = 0;
    UPROPERTY(BlueprintReadOnly) int32 Crafted = 0;
    UPROPERTY(BlueprintReadOnly) int32 Repaired = 0;
    UPROPERTY(BlueprintReadOnly) int32 Replans = 0;
    UPROPERTY(BlueprintReadOnly) bool Completed = false;
    UPROPERTY(BlueprintReadOnly) bool Cancelled = false;
    UPROPERTY(BlueprintReadOnly) EHearthwardNPCEpisodeCoverage Coverage = EHearthwardNPCEpisodeCoverage::Unknown;
    UPROPERTY(BlueprintReadOnly) double LastAt = 0;
    UPROPERTY(BlueprintReadOnly) TArray<FString> Reasons;
    UPROPERTY(BlueprintReadOnly) TArray<FGuid> Evidence;

    bool HasEvidence() const { return Command.IsValid() && !Evidence.IsEmpty(); }
    bool IsCompleteCoverage() const { return Coverage==EHearthwardNPCEpisodeCoverage::Complete; }
};

namespace HearthwardEpisodes
{
    // Production projection: uses persisted command coverage metadata.
    TArray<FHearthwardNPCEpisode> Build(const FHearthwardNPCMemory& Memory,int32 MaxEpisodes=3);
    // Compatibility/test projection when coverage metadata is unavailable: never claims complete.
    TArray<FHearthwardNPCEpisode> Build(const TArray<FHearthwardNPCEvent>& Events,int32 MaxEpisodes=3);
    FString CoverageName(EHearthwardNPCEpisodeCoverage Coverage);
    FString Describe(const FHearthwardNPCEpisode& Episode);
}
