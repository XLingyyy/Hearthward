#pragma once

#include "CoreMinimal.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Misc/Crc.h"
#include "SavePartialSettlementFixture.generated.h"

// Test observer: a real competing reservation starts only after the first committed transfer.
UCLASS(Transient)
class UHearthwardSavePartialSettlementFixture : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UHearthwardStorageSubsystem> Storage;
    TMap<FName,int32> Materials;
    FGuid Command,Reservation;
    FName FirstTaken;
    bool Triggered=false,Reserved=false;

    UFUNCTION()
    void OnTransferred(FGuid OperationId,bool ToCamp,FName ItemId,int32 Count)
    {
        if(Triggered || ToCamp || !Storage || Count<=0 || Materials.FindRef(ItemId)!=Count)return;
        const FGuid Expected(Command.A,Command.B,Command.C^0xCA01^FCrc::StrCrc32(*ItemId.ToString()),Command.D);
        if(OperationId!=Expected)return;
        Triggered=true;FirstTaken=ItemId;
        auto Remaining=Materials;Remaining.Remove(ItemId);
        Reserved=!Remaining.IsEmpty() && Storage->Reserve(Reservation,Remaining);
    }
};
