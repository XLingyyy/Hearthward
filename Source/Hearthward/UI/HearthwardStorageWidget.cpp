#include "HearthwardStorageWidget.h"
#include "HearthwardHUD.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

TSharedRef<SWidget> UHearthwardStorageWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        auto* Root = WidgetTree->ConstructWidget<UOverlay>(); WidgetTree->RootWidget = Root;
        auto* Shade = WidgetTree->ConstructWidget<UBorder>(); Shade->SetBrushColor(FLinearColor(0,0,0,.55f));
        auto* ShadeSlot = Root->AddChildToOverlay(Shade);
        ShadeSlot->SetHorizontalAlignment(HAlign_Fill); ShadeSlot->SetVerticalAlignment(VAlign_Fill);
        auto* Scale = WidgetTree->ConstructWidget<UScaleBox>(); Scale->SetStretch(EStretch::ScaleToFit);
        auto* Placement = Root->AddChildToOverlay(Scale);
        Placement->SetHorizontalAlignment(HAlign_Center); Placement->SetVerticalAlignment(VAlign_Center);
        Placement->SetPadding(FMargin(20));
        auto* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(760); Size->SetHeightOverride(540);
        Scale->AddChild(Size);
        auto* Panel = WidgetTree->ConstructWidget<UBorder>();
        Panel->SetBrushColor(FLinearColor(.035f,.042f,.042f,1)); Panel->SetPadding(FMargin(24)); Size->AddChild(Panel);
        auto* Layout = WidgetTree->ConstructWidget<UVerticalBox>(); Panel->AddChild(Layout);
        auto Text = [&](const FString& Value, int32 FontSize, FLinearColor Color)
        {
            auto* T = WidgetTree->ConstructWidget<UTextBlock>(); T->SetText(FText::FromString(Value));
            auto Font = T->GetFont(); Font.Size = FontSize; T->SetFont(Font);
            T->SetColorAndOpacity(Color); T->SetAutoWrapText(true); return T;
        };
        const FLinearColor Gold(.92f,.73f,.38f), Ivory(.95f,.94f,.88f), Muted(.70f,.74f,.71f);
        Layout->AddChildToVerticalBox(Text(TEXT("营地仓储"),26,Gold));
        Layout->AddChildToVerticalBox(Text(TEXT("共享库存 · 仓储不限负重 · 世界已暂停"),14,Muted))->SetPadding(FMargin(0,6,0,18));
        InventoryText = Text(TEXT(""),18,Ivory);
        Layout->AddChildToVerticalBox(InventoryText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        WeightText = Text(TEXT(""),16,Gold);
        Layout->AddChildToVerticalBox(WeightText)->SetPadding(FMargin(0,8,0,16));
        auto* Selection = WidgetTree->ConstructWidget<UHorizontalBox>(); Layout->AddChildToVerticalBox(Selection);
        ItemChoice = WidgetTree->ConstructWidget<UComboBoxString>();
        ItemChoice->OnGenerateWidgetEvent.BindDynamic(this,&UHearthwardStorageWidget::GenerateItem);
        for (const auto& Item : HearthwardBasicItems()) ItemChoice->AddOption(Item.DisplayName.ToString());
        ItemChoice->SetSelectedIndex(0);
        auto* ItemSize = WidgetTree->ConstructWidget<USizeBox>(); ItemSize->SetWidthOverride(180); ItemSize->AddChild(ItemChoice);
        Selection->AddChildToHorizontalBox(ItemSize)->SetPadding(FMargin(0,0,20,0));
        Selection->AddChildToHorizontalBox(Text(TEXT("数量  "),16,Ivory))->SetVerticalAlignment(VAlign_Center);
        Quantity = WidgetTree->ConstructWidget<UEditableTextBox>(); Quantity->SetText(FText::FromString(TEXT("1")));
        auto QuantityStyle = Quantity->GetWidgetStyle();
        QuantityStyle.ForegroundColor = FLinearColor(.03f,.04f,.04f);
        QuantityStyle.FocusedForegroundColor = QuantityStyle.ForegroundColor;
        Quantity->SetWidgetStyle(QuantityStyle);
        Quantity->SetHintText(FText::FromString(TEXT("正整数")));
        auto* QuantitySize = WidgetTree->ConstructWidget<USizeBox>(); QuantitySize->SetWidthOverride(180); QuantitySize->AddChild(Quantity);
        Selection->AddChildToHorizontalBox(QuantitySize);
        auto* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
        Layout->AddChildToVerticalBox(Actions)->SetPadding(FMargin(0,16,0,0));
        auto Button = [&](const FString& Label)
        {
            auto* B = WidgetTree->ConstructWidget<UButton>(); B->SetBackgroundColor(FLinearColor(.15f,.18f,.17f));
            auto* Padding = WidgetTree->ConstructWidget<UBorder>(); Padding->SetBrushColor(FLinearColor::Transparent);
            Padding->SetPadding(FMargin(18,10)); B->AddChild(Padding); Padding->AddChild(Text(Label,16,Ivory));
            Actions->AddChildToHorizontalBox(B)->SetPadding(FMargin(0,0,12,0)); return B;
        };
        Button(TEXT("存入仓储"))->OnClicked.AddDynamic(this,&UHearthwardStorageWidget::Deposit);
        Button(TEXT("取到背包"))->OnClicked.AddDynamic(this,&UHearthwardStorageWidget::Withdraw);
        Button(TEXT("返回 Esc"))->OnClicked.AddDynamic(this,&UHearthwardStorageWidget::Close);
        Status = Text(TEXT("选择物品和数量，再确认存入或取出。"),15,Gold);
        Layout->AddChildToVerticalBox(Status)->SetPadding(FMargin(0,16,0,8));
        Layout->AddChildToVerticalBox(Text(TEXT("R / Esc 返回 · Tab 背包 · F6 存档"),12,Muted));
        ItemChoice->OnSelectionChanged.AddDynamic(this,&UHearthwardStorageWidget::SelectionChanged);
    }
    return Super::RebuildWidget();
}

void UHearthwardStorageWidget::BindHUD(AHearthwardHUD* InHUD) { HUD = InHUD; Refresh(); }
void UHearthwardStorageWidget::SelectItem(FName Id)
{
    const int32 Index = HearthwardBasicItems().IndexOfByPredicate([Id](const auto& Item) { return Item.Id == Id; });
    if (Index != INDEX_NONE && ItemChoice) ItemChoice->SetSelectedIndex(Index);
}
void UHearthwardStorageWidget::SetQuantityText(const FString& Value) { if (Quantity) Quantity->SetText(FText::FromString(Value)); }
void UHearthwardStorageWidget::SelectionChanged(FString Value, ESelectInfo::Type Type) { Refresh(); }
UWidget* UHearthwardStorageWidget::GenerateItem(FString Value)
{
    auto* Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrushColor(FLinearColor(.10f,.13f,.12f)); Background->SetPadding(FMargin(8,4));
    auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Value));
    Label->SetColorAndOpacity(FLinearColor(.95f,.94f,.88f));
    auto Font = Label->GetFont(); Font.Size = 16; Label->SetFont(Font);
    Background->AddChild(Label); GeneratedItems.Add(Background); return Background;
}
void UHearthwardStorageWidget::Refresh()
{
    auto* Pawn = GetOwningPlayerPawn();
    const auto* Bag = Pawn ? Pawn->FindComponentByClass<UHearthwardInventoryComponent>() : nullptr;
    if (!Bag || !InventoryText || !ItemChoice) return;
    const auto* Storage = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    FString Lines;
    for (const auto& Item : HearthwardBasicItems())
        Lines += FString::Printf(TEXT("%s    随身 %d    仓储 %d    单重 %.2f\n"), *Item.DisplayName.ToString(),
            Bag->GetItemCount(Item.Id), Storage->GetItemCount(Item.Id), Item.WeightHundredths / 100.0);
    InventoryText->SetText(FText::FromString(Lines.TrimEnd()));
    const int32 Index = ItemChoice->GetSelectedIndex();
    if (!HearthwardBasicItems().IsValidIndex(Index)) return;
    const auto& Item = HearthwardBasicItems()[Index];
    const int64 FreeHundredths = FMath::RoundToInt64((Bag->GetCapacity() - Bag->GetWeight()) * 100);
    const int32 MaxTake = static_cast<int32>(FMath::Min<int64>(Storage->GetItemCount(Item.Id), FreeHundredths / Item.WeightHundredths));
    WeightText->SetText(FText::FromString(FString::Printf(TEXT("负重 %.2f / %.0f · 余量 %.2f\n%s：现可存入 %d，最多取出 %d"),
        Bag->GetWeight(), Bag->GetCapacity(), FreeHundredths / 100.0, *Item.DisplayName.ToString(), Bag->GetItemCount(Item.Id), MaxTake)));
}
void UHearthwardStorageWidget::Transfer(bool ToCamp)
{
    // Detached widgets must never submit into a newly opened session after a load.
    if (!HUD.IsValid() || HUD->GetStorageWidget() != this) return;
    const FString Input = Quantity->GetText().ToString().TrimStartAndEnd();
    int64 Count = 0;
    const bool Digits = !Input.IsEmpty() && Input.Len() <= 10 && Input.IsNumeric()
        && !Input.Contains(TEXT(".")) && !Input.Contains(TEXT("-")) && !Input.Contains(TEXT("+"));
    if (!Digits || !LexTryParseString(Count,*Input) || Count <= 0 || Count > MAX_int32)
    { Status->SetText(FText::FromString(TEXT("请输入1—2147483647的整数数量"))); return; }
    const int32 Index = ItemChoice->GetSelectedIndex();
    if (!HearthwardBasicItems().IsValidIndex(Index)) return;
    FString Feedback;
    HUD->TransferStorage(ToCamp,HearthwardBasicItems()[Index].Id,static_cast<int32>(Count),Feedback);
    Status->SetText(FText::FromString(Feedback)); Refresh();
}
void UHearthwardStorageWidget::Deposit() { Transfer(true); }
void UHearthwardStorageWidget::Withdraw() { Transfer(false); }
void UHearthwardStorageWidget::Close() { if (HUD.IsValid() && HUD->GetStorageWidget() == this) HUD->CloseStorageMenu(); }
FString UHearthwardStorageWidget::GetDisplayedStatus() const { return Status ? Status->GetText().ToString() : FString(); }
FString UHearthwardStorageWidget::GetDisplayedInventory() const { return InventoryText ? InventoryText->GetText().ToString() : FString(); }
FReply UHearthwardStorageWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    const auto Key = Event.GetKey();
    if (Key == EKeys::Escape || Key == EKeys::R) { Close(); return FReply::Handled(); }
    if (HUD.IsValid() && Key == EKeys::Tab) { HUD->ToggleInventory(); return FReply::Handled(); }
    if (HUD.IsValid() && Key == EKeys::F6) { HUD->ToggleSaveMenu(); return FReply::Handled(); }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
