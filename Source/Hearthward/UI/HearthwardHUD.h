#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "../Actions/HearthwardTimedActionState.h"
#include "HearthwardHUD.generated.h"

UCLASS()
class HEARTHWARD_API AHearthwardHUD : public AHUD
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) TObjectPtr<class UHearthwardScreenWidget> Screen;
    UFUNCTION(BlueprintCallable) void OpenPause();
    UFUNCTION(BlueprintCallable) void OpenMap();
    UFUNCTION(BlueprintCallable) void OpenSkills();
    UFUNCTION(BlueprintCallable) void OpenJournal();
    void EditUILayout();
    void SprintStart();
    void SprintStop();
    void Attack();
    void HeavyAttack();
    void Shoot();
    void Eat();
    void Heal();
    void Throw();
    void CompanionWait();
    void CompanionFollow();
    void CompanionAttack();
    UFUNCTION(BlueprintCallable) void ToggleStorageMenu();
    UFUNCTION(BlueprintCallable) void CloseStorageMenu();
    UFUNCTION(BlueprintPure) bool IsStorageMenuOpen() const { return StorageWidget != nullptr; }
    UFUNCTION(BlueprintPure) class UHearthwardStorageWidget* GetStorageWidget() const { return StorageWidget; }
    bool CanUseStorageMenu() const;
    bool TransferStorage(bool ToCamp, FName Item, int32 Count, FString& Feedback);
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void DrawHUD() override;

    UFUNCTION(BlueprintCallable) void ToggleSaveMenu();
    UFUNCTION(BlueprintCallable) void CloseSaveMenu();
    UFUNCTION(BlueprintPure) bool IsSaveMenuOpen() const { return SaveWidget != nullptr; }
    UFUNCTION(BlueprintPure) class UHearthwardSaveWidget* GetSaveWidget() const { return SaveWidget; }

    UFUNCTION(BlueprintCallable) void ToggleDialogue();
    UFUNCTION(BlueprintCallable) void CloseDialogue();
    UFUNCTION(BlueprintCallable) bool SubmitDialogue(const FString& Text);
    UFUNCTION(BlueprintCallable) void CancelDialogueReply();
    UFUNCTION(BlueprintCallable) void CancelDialogueTask();
    UFUNCTION(BlueprintPure) bool IsDialogueOpen() const { return DialogueWidget != nullptr; }
    UFUNCTION(BlueprintPure) class UHearthwardDialogueWidget* GetDialogueWidget() const { return DialogueWidget; }
    UFUNCTION(BlueprintPure) FString GetDialogueStatus() const;
    UFUNCTION(BlueprintPure) FString GetDialogueProgress() const;
    UFUNCTION(BlueprintPure) FString GetDialogueReply() const;
    UFUNCTION(BlueprintPure) FString GetDialogueWeight() const;
    bool CanSendDialogue() const;
    bool CanCancelDialogueReply() const;
    bool CanCancelDialogueTask() const;

    UFUNCTION(BlueprintCallable, Category="Hearthward|Inventory")
    void ToggleInventory();

    UFUNCTION(BlueprintPure, Category="Hearthward|Inventory")
    bool IsInventoryOpen() const { return bInventoryOpen; }

private:
    UPROPERTY() TObjectPtr<class UHearthwardStorageWidget> StorageWidget;
    TWeakObjectPtr<class UHearthwardResourceInteractionComponent> StorageTarget;
    FGuid StorageEpoch;
    bool bPausedByStorageMenu = false;
    bool bCursorBeforeStorageMenu = false;
    UPROPERTY() TObjectPtr<class UHearthwardSaveWidget> SaveWidget;
    bool bPausedBySaveMenu = false;
    bool bCursorBeforeSaveMenu = false;
    UFUNCTION() void SnapshotRestored();
    UPROPERTY() TObjectPtr<class UHearthwardDialogueWidget> DialogueWidget;
    TWeakObjectPtr<class AHearthwardCompanionFixture> DialogueCompanion;
    FString DialogueFeedback;
    bool bPreviousCursor = false;
    void DrawInventory(const class UHearthwardInventoryComponent& Inventory);
    bool bInventoryOpen = false;
    bool bPausedByInventory = false;
    uint32 InteractionFeedbackRevision = 0;
    double InteractionFeedbackUntil = 0.0;
    TWeakObjectPtr<APawn> ObservedPawn;
    EHearthwardTimedActionStatus PreviousStatus = EHearthwardTimedActionStatus::Idle;
    double InterruptionVisibleUntil = 0.0;
};
