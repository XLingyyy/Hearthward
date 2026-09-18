#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "HearthwardStorageWidget.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardStorageWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void BindHUD(class AHearthwardHUD* InHUD);
    UFUNCTION(BlueprintCallable) void SelectItem(FName Id);
    UFUNCTION(BlueprintCallable) void SetQuantityText(const FString& Value);
    UFUNCTION(BlueprintCallable) void Deposit();
    UFUNCTION(BlueprintCallable) void Withdraw();
    UFUNCTION(BlueprintPure) FString GetDisplayedStatus() const;
    UFUNCTION(BlueprintPure) FString GetDisplayedInventory() const;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    void Transfer(bool ToCamp);
    void Refresh();
    UFUNCTION() void Close();
    UFUNCTION() void SelectionChanged(FString Value, ESelectInfo::Type Type);
    UFUNCTION() UWidget* GenerateItem(FString Value);
    TWeakObjectPtr<class AHearthwardHUD> HUD;
    UPROPERTY() TObjectPtr<class UComboBoxString> ItemChoice;
    UPROPERTY() TObjectPtr<class UEditableTextBox> Quantity;
    UPROPERTY() TObjectPtr<class UTextBlock> InventoryText;
    UPROPERTY() TObjectPtr<class UTextBlock> WeightText;
    UPROPERTY() TObjectPtr<class UTextBlock> Status;
    // ComboBox retains generated Slate widgets, not their UObject owners.
    UPROPERTY() TArray<TObjectPtr<UWidget>> GeneratedItems;
};
