#include "HearthwardSaveWidget.h"
#include "HearthwardHUD.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
const FLinearColor Ivory(.95f, .94f, .88f), Muted(.70f, .74f, .71f), Gold(.92f, .73f, .38f);
FString ShortId(FGuid Id) { return Id.ToString(EGuidFormats::Digits).Left(8); }
}

TSharedRef<SWidget> UHearthwardSaveRow::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        Button = WidgetTree->ConstructWidget<UButton>();
        WidgetTree->RootWidget = Button;
        auto* RowPadding = WidgetTree->ConstructWidget<UBorder>();
        RowPadding->SetBrushColor(FLinearColor::Transparent);
        RowPadding->SetPadding(FMargin(10, 9));
        Button->AddChild(RowPadding);
        Text = WidgetTree->ConstructWidget<UTextBlock>();
        auto Font = Text->GetFont(); Font.Size = 15; Text->SetFont(Font);
        Text->SetColorAndOpacity(Ivory);
        Text->SetAutoWrapText(true);
        RowPadding->AddChild(Text);
        Button->OnClicked.AddDynamic(this, &UHearthwardSaveRow::Select);
    }
    return Super::RebuildWidget();
}
void UHearthwardSaveRow::BindPoint(UHearthwardSaveWidget* InMenu, FGuid InId, const FString& Label)
{
    Menu = InMenu; Id = InId;
    TakeWidget();
    Text->SetText(FText::FromString(Label));
}
void UHearthwardSaveRow::SetSelected(bool Selected)
{
    Button->SetBackgroundColor(Selected ? FLinearColor(.30f,.25f,.13f) : FLinearColor(.10f,.13f,.12f));
}
void UHearthwardSaveRow::Select() { if (Menu.IsValid()) Menu->SelectPoint(Id); }

TSharedRef<SWidget> UHearthwardSaveWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        auto* Root = WidgetTree->ConstructWidget<UOverlay>(); WidgetTree->RootWidget = Root;
        auto* Shade = WidgetTree->ConstructWidget<UBorder>();
        Shade->SetBrushColor(FLinearColor(0,0,0,.55f));
        auto* ShadeSlot = Root->AddChildToOverlay(Shade);
        ShadeSlot->SetHorizontalAlignment(HAlign_Fill); ShadeSlot->SetVerticalAlignment(VAlign_Fill);
        auto* Scale = WidgetTree->ConstructWidget<UScaleBox>(); Scale->SetStretch(EStretch::ScaleToFit);
        auto* Placement = Root->AddChildToOverlay(Scale);
        Placement->SetHorizontalAlignment(HAlign_Center); Placement->SetVerticalAlignment(VAlign_Center);
        Placement->SetPadding(FMargin(20));
        auto* Size = WidgetTree->ConstructWidget<USizeBox>();
        Size->SetWidthOverride(960); Size->SetHeightOverride(620); Scale->AddChild(Size);
        auto* Panel = WidgetTree->ConstructWidget<UBorder>();
        Panel->SetBrushColor(FLinearColor(.035f,.042f,.042f,1)); Panel->SetPadding(FMargin(24)); Size->AddChild(Panel);
        auto* Layout = WidgetTree->ConstructWidget<UVerticalBox>(); Panel->AddChild(Layout);
        auto Text = [&](const FString& Value, int32 SizeValue, FLinearColor Color)
        {
            auto* T = WidgetTree->ConstructWidget<UTextBlock>();
            T->SetText(FText::FromString(Value)); auto Font=T->GetFont(); Font.Size=SizeValue; T->SetFont(Font);
            T->SetColorAndOpacity(Color); T->SetAutoWrapText(true); return T;
        };
        auto Button = [&](UHorizontalBox* Box, const FString& Label)
        {
            auto* B = WidgetTree->ConstructWidget<UButton>();
            auto* Pad = WidgetTree->ConstructWidget<UBorder>(); Pad->SetBrushColor(FLinearColor::Transparent);
            Pad->SetPadding(FMargin(12,8)); B->AddChild(Pad);
            auto* ButtonText = Text(Label,15,Ivory); ButtonText->SetAutoWrapText(false); Pad->AddChild(ButtonText);
            B->SetBackgroundColor(FLinearColor(.15f,.18f,.17f));
            Box->AddChildToHorizontalBox(B)->SetPadding(FMargin(0,0,8,0)); return B;
        };
        Layout->AddChildToVerticalBox(Text(TEXT("存档与回档"),26,Gold));
        Capacity = Text(TEXT(""),14,Muted);
        Layout->AddChildToVerticalBox(Capacity)->SetPadding(FMargin(0,6,0,12));
        Content = WidgetTree->ConstructWidget<UVerticalBox>();
        Layout->AddChildToVerticalBox(Content)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
        Content->AddChildToVerticalBox(Columns)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* ListBox = WidgetTree->ConstructWidget<UVerticalBox>();
        auto* ListSlot=Columns->AddChildToHorizontalBox(ListBox); ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); ListSlot->SetPadding(FMargin(0,0,20,0));
        ListBox->AddChildToVerticalBox(Text(TEXT("全部进度 · 最近保存优先"),15,Gold))->SetPadding(FMargin(0,0,0,8));
        List = WidgetTree->ConstructWidget<UScrollBox>(); List->SetAlwaysShowScrollbar(true);
        ListBox->AddChildToVerticalBox(List)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* DetailBox=WidgetTree->ConstructWidget<UVerticalBox>();
        Columns->AddChildToHorizontalBox(DetailBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* DetailScroll=WidgetTree->ConstructWidget<UScrollBox>();
        DetailBox->AddChildToVerticalBox(DetailScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Details=Text(TEXT(""),16,Ivory); DetailScroll->AddChild(Details);
        auto* Actions=WidgetTree->ConstructWidget<UHorizontalBox>();
        DetailBox->AddChildToVerticalBox(Actions)->SetPadding(FMargin(0,10,0,0));
        LoadButton=Button(Actions,TEXT("读取")); LoadButton->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::RequestLoad);
        LockButton=Button(Actions,TEXT("锁定"));
        LockLabel=Cast<UTextBlock>(Cast<UBorder>(LockButton->GetChildAt(0))->GetContent());
        LockButton->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::ToggleLocked);
        DeleteButton=Button(Actions,TEXT("删除")); DeleteButton->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::RequestDelete);
        auto* Settings=WidgetTree->ConstructWidget<UHorizontalBox>();
        Content->AddChildToVerticalBox(Settings)->SetPadding(FMargin(0,14,0,0));
        Settings->AddChildToHorizontalBox(Text(TEXT("自动保存间隔（实玩分钟）  "),14,Muted))->SetVerticalAlignment(VAlign_Center);
        Interval=WidgetTree->ConstructWidget<USpinBox>();
        Interval->SetMinValue(1); Interval->SetMaxValue(60); Interval->SetMinSliderValue(1); Interval->SetMaxSliderValue(60);
        Interval->SetDelta(1); Interval->SetMinFractionalDigits(0); Interval->SetMaxFractionalDigits(0);
        Interval->SetForegroundColor(FLinearColor(.03f,.04f,.04f));
        auto* SpinSize=WidgetTree->ConstructWidget<USizeBox>(); SpinSize->SetWidthOverride(85); SpinSize->AddChild(Interval);
        Settings->AddChildToHorizontalBox(SpinSize)->SetPadding(FMargin(0,0,12,0));
        Button(Settings,TEXT("应用"))->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::ApplyInterval);
        Status=Text(TEXT(""),14,Gold); Layout->AddChildToVerticalBox(Status)->SetPadding(FMargin(0,12));
        Confirmation=Text(TEXT(""),17,Gold); Layout->AddChildToVerticalBox(Confirmation)->SetPadding(FMargin(0,4,0,10));
        ConfirmButtons=WidgetTree->ConstructWidget<UHorizontalBox>(); Layout->AddChildToVerticalBox(ConfirmButtons);
        Button(ConfirmButtons,TEXT("确认"))->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::ConfirmPending);
        Button(ConfirmButtons,TEXT("取消 Esc"))->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::CancelPending);
        Footer=WidgetTree->ConstructWidget<UHorizontalBox>(); Layout->AddChildToVerticalBox(Footer);
        EnableButton=Button(Footer,TEXT("启用存档")); EnableButton->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::EnableSaves);
        Button(Footer,TEXT("手动保存"))->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::SaveManual);
        Button(Footer,TEXT("新进度"))->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::RequestNewProgress);
        Button(Footer,TEXT("退出游戏"))->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::RequestQuit);
        Button(Footer,TEXT("返回 Esc"))->OnClicked.AddDynamic(this,&UHearthwardSaveWidget::Close);
        Layout->AddChildToVerticalBox(Text(TEXT("世界已暂停 · F6 返回 · Tab 背包\n开发验证场：需先创建伙伴夹具；仅恢复当前已接入系统。"),12,Muted))->SetPadding(FMargin(0,10,0,0));
    }
    return Super::RebuildWidget();
}

void UHearthwardSaveWidget::BindHUD(AHearthwardHUD* InHUD)
{
    HUD=InHUD; Saves=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>(); RefreshPoints();
}
void UHearthwardSaveWidget::RefreshPoints()
{
    if (!Saves.IsValid() || !List) return;
    List->ClearChildren(); Rows.Reset(); RowIds.Reset();
    auto Points=Saves->GetPoints();
    Points.StableSort([](const auto& A,const auto& B) { return A.Created>B.Created; });
    bool Found=false;
    for (const auto& Point:Points)
    {
        auto* Row=CreateWidget<UHearthwardSaveRow>(GetOwningPlayer());
        const FString Label=FString::Printf(TEXT("%s  ·  %s%s\n进度 %s%s"),
            *Point.Created.ToString(TEXT("%m-%d %H:%M:%S UTC")),Point.Manual?TEXT("手动"):TEXT("自动"),
            Point.Locked?TEXT(" · 已锁定"):TEXT(""),*ShortId(Point.CampaignId),
            Point.CampaignId==Saves->GetCampaignId()?TEXT(" · 当前"):TEXT(""));
        Row->BindPoint(this,Point.SaveId,Label); List->AddChild(Row); Rows.Add(Row); RowIds.Add(Point.SaveId);
        Found |= Point.SaveId==SelectedId;
    }
    if (!Found) SelectedId=Points.IsEmpty()?FGuid():Points[0].SaveId;
    Capacity->SetText(FText::FromString(FString::Printf(TEXT("全局 %d / %d 个回档点  ·  当前进度 %s"),
        Points.Num(),HearthwardSave::MaxPoints,Saves->GetCampaignId().IsValid()?*ShortId(Saves->GetCampaignId()):TEXT("尚未创建或加载"))));
    Interval->SetValue(Saves->GetAutoMinutes());
    EnableButton->SetVisibility(Saves->IsPrototypeEnabled()?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
    RefreshDetails(); RefreshStatus(); RefreshConfirmation();
    const int32 SelectedIndex = RowIds.IndexOfByKey(SelectedId);
    if (Rows.IsValidIndex(SelectedIndex)) List->ScrollWidgetIntoView(Rows[SelectedIndex],false);
}
void UHearthwardSaveWidget::RefreshStatus()
{
    if (Saves.IsValid()) Status->SetText(FText::FromString(Saves->GetStatus()+TEXT("\n")+Saves->GetSafetyDescription()));
}
void UHearthwardSaveWidget::SelectPoint(FGuid Id)
{
    if (Pending!=EAction::None || !RowIds.Contains(Id)) return;
    SelectedId=Id; RefreshDetails();
}
void UHearthwardSaveWidget::RefreshDetails()
{
    const auto Points=Saves->GetPoints();
    const auto* Point=Points.FindByPredicate([this](const auto& P) { return P.SaveId==SelectedId; });
    for (int32 I=0; I<Rows.Num(); ++I) Rows[I]->SetSelected(RowIds[I]==SelectedId);
    LoadButton->SetIsEnabled(Point!=nullptr); DeleteButton->SetIsEnabled(Point!=nullptr); LockButton->SetIsEnabled(Point!=nullptr);
    if (!Point)
    {
        Details->SetText(FText::FromString(TEXT("暂无可选节点\n\n启用存档后可创建新进度，或读取已有节点。\n\n所有进度共用50点；手动档及锁定档不会被自动覆盖。")));
        return;
    }
    LockLabel->SetText(FText::FromString(Point->Locked?TEXT("解锁"):TEXT("锁定")));
    const FString Value=FString::Printf(TEXT("所属进度  %s\n时间  %s UTC\n地点  %s\n阶段  %s\n类型  %s\n锁定  %s\n\n节点  %s\n\n%s"),
        *Point->CampaignId.ToString(),*Point->Created.ToString(TEXT("%Y-%m-%d %H:%M:%S")),*Point->Location,*Point->Stage,
        Point->Manual?TEXT("手动"):TEXT("自动"),Point->Locked?TEXT("已锁定"):TEXT("未锁定"),*Point->SaveId.ToString(),
        Point->Manual||Point->Locked?TEXT("受保护：不会被自动轮换。主动删除仍需确认。"):TEXT("满额时可能轮换最早的未锁定自动档。"));
    Details->SetText(FText::FromString(Value));
}
void UHearthwardSaveWidget::EnableSaves() { if (Pending==EAction::None && Saves.IsValid()) { Saves->EnablePrototype(); RefreshPoints(); } }
void UHearthwardSaveWidget::SaveManual()
{
    if (Pending!=EAction::None || !Saves.IsValid()) return;
    Saves->SavePoint(true); RefreshPoints();
}
void UHearthwardSaveWidget::ToggleLocked()
{
    if (Pending!=EAction::None || !Saves.IsValid()) return;
    const auto Points=Saves->GetPoints();
    if (const auto* P=Points.FindByPredicate([this](const auto& V) { return V.SaveId==SelectedId; }))
        Saves->SetPointLocked(SelectedId,!P->Locked);
    RefreshPoints();
}
void UHearthwardSaveWidget::Request(EAction Action)
{
    if (Pending!=EAction::None || !Saves.IsValid()) return;
    if ((Action==EAction::Load || Action==EAction::Delete) && !RowIds.Contains(SelectedId)) return;
    Pending=Action; PendingId=SelectedId;
    FString Message;
    switch (Pending)
    {
    case EAction::Load: Message=TEXT("读取节点 ")+ShortId(PendingId)+TEXT("？当前未保存进度将丢失，世界与知识一起回退。"); break;
    case EAction::NewProgress: Message=TEXT("开启新进度？当前未保存进度将丢失；返回本次启用时的初始场景，并占用全局档池位置。"); break;
    case EAction::Delete: Message=TEXT("永久删除节点 ")+ShortId(PendingId)+TEXT("？即使手动或锁定也会删除，无法撤销。"); break;
    case EAction::Quit: Message=TEXT("退出游戏？未保存进度将丢失。本次退出不会额外保存。"); break;
    default: break;
    }
    Confirmation->SetText(FText::FromString(Message)); RefreshConfirmation(); SetKeyboardFocus();
}
void UHearthwardSaveWidget::RefreshConfirmation()
{
    const bool Asking=Pending!=EAction::None;
    Content->SetIsEnabled(!Asking);
    Footer->SetVisibility(Asking?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
    ConfirmButtons->SetVisibility(Asking?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Confirmation->SetVisibility(Asking?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
}
void UHearthwardSaveWidget::RequestLoad() { Request(EAction::Load); }
void UHearthwardSaveWidget::RequestNewProgress() { Request(EAction::NewProgress); }
void UHearthwardSaveWidget::RequestDelete() { Request(EAction::Delete); }
void UHearthwardSaveWidget::RequestQuit() { Request(EAction::Quit); }
void UHearthwardSaveWidget::ConfirmPending()
{
    if (Pending==EAction::None || !Saves.IsValid()) return;
    const EAction Action=Pending; const FGuid Id=PendingId;
    Pending=EAction::None; PendingId.Invalidate();
    switch (Action)
    {
    case EAction::Load: Saves->LoadPoint(Id); break;
    case EAction::NewProgress: Saves->StartNewProgress(); break;
    case EAction::Delete: Saves->DeletePoint(Id); break;
    case EAction::Quit:
        UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false);
        return;
    default: break;
    }
    RefreshPoints(); SetKeyboardFocus();
}
void UHearthwardSaveWidget::CancelPending()
{
    Pending=EAction::None; PendingId.Invalidate(); Confirmation->SetText(FText::GetEmpty()); RefreshConfirmation(); SetKeyboardFocus();
}
void UHearthwardSaveWidget::SetAutoMinutes(int32 Minutes)
{
    if (Pending!=EAction::None || !Saves.IsValid()) return;
    const bool Applied=Saves->SetAutoMinutes(Minutes);
    Interval->SetValue(Saves->GetAutoMinutes());
    Status->SetText(FText::FromString(Applied?FString::Printf(TEXT("自动保存间隔已设为%d实玩分钟；暂停不计时。"),Minutes):TEXT("自动保存间隔须为1—60分钟")));
}
void UHearthwardSaveWidget::ApplyInterval() { SetAutoMinutes(FMath::RoundToInt(Interval->GetValue())); }
void UHearthwardSaveWidget::Close() { if (HUD.IsValid()) HUD->CloseSaveMenu(); }
void UHearthwardSaveWidget::ScrollToEnd() { if (List) List->ScrollToEnd(); }
FString UHearthwardSaveWidget::GetDisplayedStatus() const { return Status?Status->GetText().ToString():FString(); }
FString UHearthwardSaveWidget::GetDisplayedDetails() const { return Details?Details->GetText().ToString():FString(); }
FString UHearthwardSaveWidget::GetConfirmationText() const { return Pending!=EAction::None && Confirmation?Confirmation->GetText().ToString():FString(); }
FReply UHearthwardSaveWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    const auto Key=Event.GetKey();
    if (Key==EKeys::Escape || Key==EKeys::F6)
    {
        if (Pending!=EAction::None) CancelPending(); else Close();
        return FReply::Handled();
    }
    if (Key==EKeys::Tab && Pending==EAction::None && HUD.IsValid())
    { HUD->ToggleInventory(); return FReply::Handled(); }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
