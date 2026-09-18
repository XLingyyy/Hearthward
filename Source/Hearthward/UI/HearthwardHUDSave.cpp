#include "HearthwardHUD.h"
#include "HearthwardSaveWidget.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void AHearthwardHUD::ToggleSaveMenu()
{
#if !UE_BUILD_SHIPPING
    if (SaveWidget) { CloseSaveMenu(); return; }
    auto* Player = GetOwningPlayerController();
    if (!Player || !GetOwningPawn()) return;
    CloseStorageMenu();
    CloseDialogue();
    if (bInventoryOpen) ToggleInventory();
    bPausedBySaveMenu = !GetWorld()->IsPaused() && UGameplayStatics::SetGamePaused(this, true);
    bCursorBeforeSaveMenu = Player->bShowMouseCursor;
    SaveWidget = CreateWidget<UHearthwardSaveWidget>(Player);
    SaveWidget->SetIsFocusable(true);
    SaveWidget->AddToViewport(30);
    SaveWidget->BindHUD(this);
    Player->SetIgnoreMoveInput(true);
    Player->SetIgnoreLookInput(true);
    Player->FlushPressedKeys();
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(SaveWidget->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Player->SetInputMode(Mode);
    Player->bShowMouseCursor = true;
    SaveWidget->SetKeyboardFocus();
#endif
}

void AHearthwardHUD::CloseSaveMenu()
{
    if (!SaveWidget) return;
    SaveWidget->RemoveFromParent();
    SaveWidget = nullptr;
    if (auto* Player = GetOwningPlayerController())
    {
        Player->SetIgnoreMoveInput(false);
        Player->SetIgnoreLookInput(false);
        Player->FlushPressedKeys();
        Player->SetInputMode(FInputModeGameOnly());
        Player->bShowMouseCursor = bCursorBeforeSaveMenu;
    }
    if (bPausedBySaveMenu) UGameplayStatics::SetGamePaused(this, false);
    bPausedBySaveMenu = false;
}
