#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HearthwardSaveWidget.generated.h"

class UHearthwardSaveWidget;
class UHearthwardSaveSubsystem;
class AHearthwardHUD;
class UTextBlock;
class UButton;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class USpinBox;

UCLASS()
class HEARTHWARD_API UHearthwardSaveRow : public UUserWidget
{
    GENERATED_BODY()
public:
    void BindPoint(UHearthwardSaveWidget* InMenu, FGuid InId, const FString& Label);
    void SetSelected(bool Selected);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    UFUNCTION() void Select();
    TWeakObjectPtr<UHearthwardSaveWidget> Menu;
    FGuid Id;
    UPROPERTY() TObjectPtr<UButton> Button;
    UPROPERTY() TObjectPtr<UTextBlock> Text;
};

UCLASS()
class HEARTHWARD_API UHearthwardSaveWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void BindHUD(AHearthwardHUD* InHUD);
    UFUNCTION(BlueprintCallable) void RefreshPoints();
    UFUNCTION(BlueprintCallable) void SelectPoint(FGuid Id);
    UFUNCTION(BlueprintCallable) void SaveManual();
    UFUNCTION(BlueprintCallable) void RequestLoad();
    UFUNCTION(BlueprintCallable) void RequestNewProgress();
    UFUNCTION(BlueprintCallable) void RequestDelete();
    UFUNCTION(BlueprintCallable) void RequestQuit();
    UFUNCTION(BlueprintCallable) void ToggleLocked();
    UFUNCTION(BlueprintCallable) void ConfirmPending();
    UFUNCTION(BlueprintCallable) void CancelPending();
    UFUNCTION(BlueprintCallable) void SetAutoMinutes(int32 Minutes);
    UFUNCTION(BlueprintPure) FString GetDisplayedStatus() const;
    UFUNCTION(BlueprintPure) FString GetDisplayedDetails() const;
    UFUNCTION(BlueprintPure) FString GetConfirmationText() const;
    UFUNCTION(BlueprintPure) int32 GetDisplayedPointCount() const { return RowIds.Num(); }
    UFUNCTION(BlueprintPure) FGuid GetSelectedId() const { return SelectedId; }
    UFUNCTION(BlueprintPure) bool HasPendingConfirmation() const { return Pending != EAction::None; }
    UFUNCTION(BlueprintCallable) void ScrollToEnd();
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    enum class EAction { None, Load, NewProgress, Delete, Quit };
    void Request(EAction Action);
    void RefreshDetails();
    void RefreshStatus();
    void RefreshConfirmation();
    UFUNCTION() void EnableSaves();
    UFUNCTION() void Close();
    UFUNCTION() void ApplyInterval();
    TWeakObjectPtr<AHearthwardHUD> HUD;
    TWeakObjectPtr<UHearthwardSaveSubsystem> Saves;
    FGuid SelectedId;
    FGuid PendingId;
    EAction Pending = EAction::None;
    TArray<FGuid> RowIds;
    UPROPERTY() TArray<TObjectPtr<UHearthwardSaveRow>> Rows;
    UPROPERTY() TObjectPtr<UTextBlock> Capacity;
    UPROPERTY() TObjectPtr<UTextBlock> Status;
    UPROPERTY() TObjectPtr<UTextBlock> Details;
    UPROPERTY() TObjectPtr<UTextBlock> Confirmation;
    UPROPERTY() TObjectPtr<UScrollBox> List;
    UPROPERTY() TObjectPtr<UVerticalBox> Content;
    UPROPERTY() TObjectPtr<UHorizontalBox> Footer;
    UPROPERTY() TObjectPtr<UHorizontalBox> ConfirmButtons;
    UPROPERTY() TObjectPtr<USpinBox> Interval;
    UPROPERTY() TObjectPtr<UButton> EnableButton;
    UPROPERTY() TObjectPtr<UButton> LoadButton;
    UPROPERTY() TObjectPtr<UButton> DeleteButton;
    UPROPERTY() TObjectPtr<UButton> LockButton;
    UPROPERTY() TObjectPtr<UTextBlock> LockLabel;
};
