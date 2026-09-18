#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HearthwardDialogueWidget.generated.h"

class AHearthwardHUD;
class UTextBlock;
class UEditableTextBox;
class UButton;

UCLASS()
class HEARTHWARD_API UHearthwardDialogueWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void BindHUD(AHearthwardHUD* InHUD);
    UFUNCTION(BlueprintCallable) void SetDraft(const FString& Text);
    UFUNCTION(BlueprintCallable) void SendDraft();
    UFUNCTION(BlueprintPure) FString GetDisplayedStatus() const;
    UFUNCTION(BlueprintPure) FString GetDisplayedProgress() const;
    UFUNCTION(BlueprintPure) FString GetDisplayedReply() const;
    UFUNCTION(BlueprintPure) bool IsSendEnabled() const;
    UFUNCTION(BlueprintPure) bool HasDraftFocus() const;
    void FocusDraft();
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    void Refresh();
    UFUNCTION() void CommitDraft(const FText& Text, ETextCommit::Type Method);
    UFUNCTION() void CancelReply();
    UFUNCTION() void CancelTask();
    UFUNCTION() void Close();
    TWeakObjectPtr<AHearthwardHUD> HUD;
    bool bFocusWhenMounted = true;
    UPROPERTY() TObjectPtr<UTextBlock> Status;
    UPROPERTY() TObjectPtr<UTextBlock> Progress;
    UPROPERTY() TObjectPtr<UTextBlock> Reply;
    UPROPERTY() TObjectPtr<UTextBlock> Weight;
    UPROPERTY() TObjectPtr<UEditableTextBox> Draft;
    UPROPERTY() TObjectPtr<UButton> Send;
    UPROPERTY() TObjectPtr<UButton> StopReply;
    UPROPERTY() TObjectPtr<UButton> StopTask;
};
