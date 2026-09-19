#include "HearthwardHUD.h"
#include "HearthwardScreenWidget.h"
#include "HearthwardStorageWidget.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void AHearthwardHUD::ToggleStorageMenu()
{
    if(Screen) { Screen->ExecuteAction(TEXT("page:storage")); return; }
#if !UE_BUILD_SHIPPING
    if (StorageWidget) { CloseStorageMenu(); return; }
    auto* Player = GetOwningPlayerController();
    auto* Pawn = GetOwningPawn();
    if (!Player || !Pawn || IsSaveMenuOpen() || IsDialogueOpen()) return;
    StorageTarget.Reset();
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        auto* Target = It->FindComponentByClass<UHearthwardResourceInteractionComponent>();
        if (Target && Target->CanAccessStorage(Pawn)) { StorageTarget = Target; break; }
    }
    if (!StorageTarget.IsValid()) return;
    if (bInventoryOpen) ToggleInventory();
    StorageEpoch = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    bPausedByStorageMenu = !GetWorld()->IsPaused() && UGameplayStatics::SetGamePaused(this, true);
    bCursorBeforeStorageMenu = Player->bShowMouseCursor;
    StorageWidget = CreateWidget<UHearthwardStorageWidget>(Player);
    StorageWidget->SetIsFocusable(true);
    StorageWidget->AddToViewport(25);
    StorageWidget->BindHUD(this);
    Player->SetIgnoreMoveInput(true);
    Player->SetIgnoreLookInput(true);
    Player->FlushPressedKeys();
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(StorageWidget->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Player->SetInputMode(Mode);
    Player->bShowMouseCursor = true;
    StorageWidget->SetKeyboardFocus();
#endif
}

void AHearthwardHUD::CloseStorageMenu()
{
    if (!StorageWidget) return;
    StorageWidget->RemoveFromParent();
    StorageWidget = nullptr;
    StorageTarget.Reset();
    StorageEpoch.Invalidate();
    if (auto* Player = GetOwningPlayerController())
    {
        Player->SetIgnoreMoveInput(false);
        Player->SetIgnoreLookInput(false);
        Player->FlushPressedKeys();
        Player->SetInputMode(FInputModeGameOnly());
        Player->bShowMouseCursor = bCursorBeforeStorageMenu;
    }
    if (bPausedByStorageMenu) UGameplayStatics::SetGamePaused(this, false);
    bPausedByStorageMenu = false;
}

bool AHearthwardHUD::CanUseStorageMenu() const
{
    return StorageWidget && StorageTarget.IsValid() && StorageTarget->CanAccessStorage(GetOwningPawn())
        && StorageEpoch == GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
}

bool AHearthwardHUD::TransferStorage(bool ToCamp, FName Item, int32 Count, FString& Feedback)
{
    if (!CanUseStorageMenu()) { Feedback = TEXT("营地访问已失效，请关闭后靠近营地重新打开"); return false; }
    auto* Bag = GetOwningPawn()->FindComponentByClass<UHearthwardInventoryComponent>();
    const auto Result = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Transfer(
        Bag, ToCamp, Item, Count, FGuid::NewGuid(), StorageEpoch).Result;
    using R = EHearthwardInventoryResult;
    switch (Result)
    {
    case R::Success:
        Feedback = ToCamp ? FString::Printf(TEXT("已存入 %d 件"), Count) : FString::Printf(TEXT("已取出 %d 件"), Count);
        return true;
    case R::InvalidCount: Feedback = TEXT("请输入正整数数量"); break;
    case R::InsufficientItems: Feedback = TEXT("库存不足，未转移"); break;
    case R::CapacityExceeded: Feedback = TEXT("背包容量不足，未转移"); break;
    case R::QuantityOverflow: Feedback = TEXT("目标数量已达上限，未转移"); break;
    default: Feedback = TEXT("转移失败，物资保留原处"); break;
    }
    return false;
}
