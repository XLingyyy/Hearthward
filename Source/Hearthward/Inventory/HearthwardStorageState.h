#pragma once

#include "HearthwardInventoryState.h"
#include "HearthwardStorageState.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardTransferResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Hearthward|Inventory")
    EHearthwardInventoryResult Result = EHearthwardInventoryResult::InvalidArgument;
    UPROPERTY(BlueprintReadOnly, Category="Hearthward|Inventory")
    int32 MovedCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="Hearthward|Inventory")
    bool Replayed = false;
};

class FHearthwardStorageState
{
public:
    FGuid GetEpoch() const { return Epoch; }
    FGuid GetContainerId() const { return ContainerId; }
    int32 GetCount(FName ItemId) const { return Shared.GetCount(ItemId); }
    int64 GetWeightHundredths() const { return Shared.GetWeightHundredths(); }
    void AdvanceTimeline() { Epoch = FGuid::NewGuid(); Completed.Reset(); }

    FHearthwardTransferResult Transfer(FHearthwardInventoryState& Personal, FGuid PersonalId,
        bool ToCamp, FName ItemId, int32 Count, FGuid OperationId, FGuid TimelineEpoch)
    {
        FHearthwardTransferResult Reply;
        if (TimelineEpoch != Epoch) { Reply.Result = EHearthwardInventoryResult::StaleTimeline; return Reply; }
        if (!OperationId.IsValid() || !PersonalId.IsValid()) return Reply;
        if (const auto* Prior = Completed.Find(OperationId))
        {
            if (Prior->PersonalId != PersonalId || Prior->ToCamp != ToCamp || Prior->ItemId != ItemId || Prior->Count != Count)
                Reply.Result = EHearthwardInventoryResult::OperationConflict;
            else { Reply.Result = Prior->Result; Reply.Replayed = true; }
            return Reply;
        }
        Reply.Result = ToCamp ? Personal.TransferTo(Shared, ItemId, Count) : Shared.TransferTo(Personal, ItemId, Count);
        if (Reply.Result == EHearthwardInventoryResult::Success) Reply.MovedCount = Count;
        Completed.Add(OperationId, {PersonalId, ToCamp, ItemId, Count, Reply.Result});
        return Reply;
    }

private:
    friend class UHearthwardSaveSubsystem;
    struct FCompleted
    {
        FGuid PersonalId;
        bool ToCamp;
        FName ItemId;
        int32 Count;
        EHearthwardInventoryResult Result;
    };
    FHearthwardInventoryState Shared{true};
    FGuid Epoch = FGuid::NewGuid();
    FGuid ContainerId = FGuid::NewGuid();
    TMap<FGuid, FCompleted> Completed;
};
