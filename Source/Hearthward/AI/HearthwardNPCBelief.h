#pragma once
#include "CoreMinimal.h"
#include "HearthwardNPCBelief.generated.h"

UENUM(BlueprintType)
enum class EHearthwardNPCBeliefSource : uint8
{
    Firsthand,
    PlayerReport,
    Receipt
};

USTRUCT(BlueprintType)
struct FHearthwardNPCBelief
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid Id;
    UPROPERTY(BlueprintReadOnly) FName Kind = TEXT("camp_stock");
    UPROPERTY(BlueprintReadOnly) FName Item;
    UPROPERTY(BlueprintReadOnly) int32 Value = 0;
    UPROPERTY(BlueprintReadOnly) EHearthwardNPCBeliefSource Source = EHearthwardNPCBeliefSource::PlayerReport;
    // Last semantic value/source change.
    UPROPERTY(BlueprintReadOnly) double RecordedAt = 0;
    // Most recent valid evidence for the current value/source.
    UPROPERTY(BlueprintReadOnly) double LastEvidenceAt = 0;
    UPROPERTY(BlueprintReadOnly) int64 Revision = 1;
    UPROPERTY(BlueprintReadOnly) FGuid Campaign;
};

USTRUCT(BlueprintType)
struct FHearthwardNPCBeliefView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool Known = false;
    UPROPERTY(BlueprintReadOnly) int32 Value = 0;
    UPROPERTY(BlueprintReadOnly) EHearthwardNPCBeliefSource Source = EHearthwardNPCBeliefSource::PlayerReport;
    UPROPERTY(BlueprintReadOnly) double RecordedAt = 0;
    UPROPERTY(BlueprintReadOnly) double LastEvidenceAt = 0;
    UPROPERTY(BlueprintReadOnly) int64 Revision = 0;
    bool IsConfirmed() const { return Known && Source != EHearthwardNPCBeliefSource::PlayerReport; }
};

namespace HearthwardBeliefs
{
    bool UpsertCampStock(TArray<FHearthwardNPCBelief>& Beliefs, int64& MemoryRevision, FGuid Campaign,
        FName Item, int32 Value, EHearthwardNPCBeliefSource Source, double Now);
    bool ResolveCampStock(const TArray<FHearthwardNPCBelief>& Beliefs, FName Item, FHearthwardNPCBeliefView& Out);
    bool Validate(const TArray<FHearthwardNPCBelief>& Beliefs, int64 MemoryRevision, FGuid Campaign, double Now);
    FString SourceName(EHearthwardNPCBeliefSource Source);
}
