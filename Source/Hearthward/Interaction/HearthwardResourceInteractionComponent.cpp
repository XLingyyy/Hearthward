#include "HearthwardResourceInteractionComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Engine/World.h"

void UHearthwardResourceInteractionComponent::InitializePrototype(bool bAtCamp)
{
#if !UE_BUILD_SHIPPING
    bEnabled = true;
    bDeposit = bAtCamp;
    MaxDistance = 150.0f; // PROTOTYPE_ONLY; not a universal interaction distance.
#endif
}

bool UHearthwardResourceInteractionComponent::CanAccessStorage(AActor* Interactor) const
{
    return bEnabled && bDeposit && IsValid(GetOwner()) && !GetOwner()->IsActorBeingDestroyed()
        && IsValid(Interactor) && !Interactor->IsActorBeingDestroyed() && Interactor->GetWorld() == GetWorld()
        && MaxDistance > 0 && FVector::DistSquared(Interactor->GetActorLocation(), GetComponentLocation()) <= FMath::Square(double(MaxDistance));
}

FString UHearthwardResourceInteractionComponent::GetInteractionPrompt(AActor* Interactor) const
{
    const auto* Bag = IsValid(Interactor) ? Interactor->FindComponentByClass<UHearthwardInventoryComponent>() : nullptr;
    if (!bEnabled || !Bag) return FString();
    if (bDeposit)
    {
        const auto* Storage = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        return FString::Printf(TEXT("R 管理仓储 · E 五秒存入木材\n随身木材 %d   仓储木材 %d"),
            Bag->GetItemCount(TEXT("wood")), Storage->GetItemCount(TEXT("wood")));
    }
    const auto* Source = GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>();
    return FString::Printf(TEXT("E 采集木材 ×1 · 资源测试点\n剩余 %d   单重 1   背包余量 %.2f"),
        Source ? Source->GetItemCount(TEXT("wood")) : 0, Bag->GetCapacity() - Bag->GetWeight());
}

FString UHearthwardResourceInteractionComponent::CompleteInteraction(AActor* Interactor)
{
    if (!bEnabled || !IsValid(Interactor) || Interactor->IsActorBeingDestroyed()
        || Interactor->GetWorld() != GetWorld() || GetWorld()->IsPaused()
        || FVector::DistSquared(Interactor->GetActorLocation(), GetComponentLocation()) > FMath::Square(double(MaxDistance)))
        return TEXT("目标不可用，未转移物资");
    auto* Bag = Interactor->FindComponentByClass<UHearthwardInventoryComponent>();
    if (!Bag) return TEXT("背包不可用，未转移物资");
    EHearthwardInventoryResult Result;
    int32 Count = 1; // PROTOTYPE_ONLY yield; the final resource/tool economy is OPEN.
    if (bDeposit)
    {
        Count = Bag->GetItemCount(TEXT("wood"));
        if (Count == 0) return TEXT("没有可入库的木材");
        auto* Storage = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        Result = Storage->Transfer(Bag, true, TEXT("wood"), Count, FGuid::NewGuid(), Storage->GetTimelineEpoch()).Result;
    }
    else
    {
        auto* Source = GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>();
        if (!Source) return TEXT("资源点不可用");
        Result = Source->TransferTo(Bag, TEXT("wood"), Count);
    }
    if (Result == EHearthwardInventoryResult::Success)
        return bDeposit ? FString::Printf(TEXT("木材 ×%d 已入库"), Count)
            : FString::Printf(TEXT("木材 ×%d 已放入背包"), Count);
    if (Result == EHearthwardInventoryResult::CapacityExceeded) return TEXT("背包容量不足，未采集");
    if (Result == EHearthwardInventoryResult::InsufficientItems) return TEXT("资源已耗尽，未采集");
    if (Result == EHearthwardInventoryResult::QuantityOverflow) return TEXT("仓储数量已达上限，未入库");
    return TEXT("转移失败，物资保留原处");
}
