#pragma once
#include "CoreMinimal.h"
class UHearthwardInventoryComponent;

// Actor-neutral settlement. The caller owns the operation ledger and snapshot guard.
namespace HearthwardWorkshop
{
    TMap<FName,int32> Materials(FName Intent,FName Item,int32 Batches);
    TMap<FName,int32> Outputs(FName Recipe,int32 Batches);
    bool RepairQuote(const UHearthwardInventoryComponent* Bag,FGuid Instance,double Fraction,TMap<FName,int32>& Materials,double& Restored);
    FString Check(AActor* Operator,AActor* Station,UHearthwardInventoryComponent* Bag,
        FName Intent,FName Item,int32 Batches);
    bool Commit(UHearthwardInventoryComponent* Bag,FName Intent,FName Item,int32 Batches);
}
