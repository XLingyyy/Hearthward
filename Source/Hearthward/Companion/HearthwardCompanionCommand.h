#pragma once

#include "CoreMinimal.h"
#include "../Inventory/HearthwardInventoryState.h"
#include "HearthwardCompanionCommand.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardCommandTicket
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid Id;
    UPROPERTY(BlueprintReadOnly) FGuid Epoch;
    UPROPERTY(BlueprintReadOnly) int64 Revision = 0;

    bool Matches(const FHearthwardCommandTicket& Other) const
    {
        return Id.IsValid() && Id == Other.Id && Epoch == Other.Epoch && Revision == Other.Revision;
    }
};

UENUM(BlueprintType)
enum class EHearthwardProposalResult : uint8
{
    Accepted, NeedsClarification, Unsupported, Stale, OutOfRange, Unsafe, Unavailable
};

// Typed execution boundary only. Player text is never inventory authority.
class FHearthwardCompanionCommand
{
public:
    FHearthwardCommandTicket Request(FGuid Epoch)
    {
        Pending = {FGuid::NewGuid(), Epoch, ++Revision};
        return Pending;
    }

    EHearthwardProposalResult Accept(const FHearthwardCommandTicket& Ticket, FGuid CurrentEpoch,
        FName Item, int32 Quantity, const TArray<FName>& Steps)
    {
        using R = EHearthwardProposalResult;
        if (!Pending.Matches(Ticket) || Ticket.Epoch != CurrentEpoch) return R::Stale;
        if (Item.IsNone() || Quantity == 0) return R::NeedsClarification;
        const TArray<FName> SupportedSteps = {TEXT("collect"), TEXT("return"), TEXT("deposit")};
        if (Quantity < 0 || Steps != SupportedSteps ||
            !HearthwardBasicItems().ContainsByPredicate([Item](const auto& Def) { return Def.Id == Item; })) return R::Unsupported;
        Active = Ticket;
        Pending = {};
        ItemId = Item;
        Requested = Quantity;
        Delivered = 0;
        bActive = true;
        return R::Accepted;
    }

    void Cancel() { Pending = {}; bActive = false; }
    bool IsCurrent(FGuid Epoch) const { return bActive && Active.Epoch == Epoch; }
    bool RecordDelivery(const FHearthwardCommandTicket& Ticket, int32 ActuallyMoved)
    {
        if (!bActive || !Active.Matches(Ticket) || ActuallyMoved <= 0 || ActuallyMoved > Requested - Delivered) return false;
        Delivered += ActuallyMoved;
        if (Delivered == Requested) bActive = false;
        return true;
    }
    FHearthwardCommandTicket GetActive() const { return Active; }
    FName GetItem() const { return ItemId; }
    int32 GetRequested() const { return Requested; }
    int32 GetDelivered() const { return Delivered; }
private:
    FHearthwardCommandTicket Pending, Active;
    int64 Revision = 0;
    FName ItemId;
    int32 Requested = 0, Delivered = 0;
    bool bActive = false;
};
