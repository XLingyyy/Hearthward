#include "HearthwardDialogueWidget.h"
#include "HearthwardHUD.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UHearthwardDialogueWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        auto* Root = WidgetTree->ConstructWidget<UOverlay>();
        WidgetTree->RootWidget = Root;
        auto* Scale = WidgetTree->ConstructWidget<UScaleBox>();
        Scale->SetStretch(EStretch::ScaleToFit);
        auto* PanelSlot = Root->AddChildToOverlay(Scale);
        PanelSlot->SetHorizontalAlignment(HAlign_Center);
        PanelSlot->SetVerticalAlignment(VAlign_Center);
        PanelSlot->SetPadding(FMargin(20));
        auto* Size = WidgetTree->ConstructWidget<USizeBox>();
        Size->SetWidthOverride(760); Size->SetHeightOverride(520);
        Scale->AddChild(Size);
        auto* Panel = WidgetTree->ConstructWidget<UBorder>();
        Panel->SetBrushColor(FLinearColor(.035f, .042f, .042f, .98f));
        Panel->SetPadding(FMargin(24));
        Size->AddChild(Panel);
        auto* Rows = WidgetTree->ConstructWidget<UVerticalBox>();
        Panel->AddChild(Rows);
        auto Text = [&](const FString& Value, int32 FontSize, FLinearColor Color)
        {
            auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
            Label->SetText(FText::FromString(Value));
            auto Font = Label->GetFont(); Font.Size = FontSize; Label->SetFont(Font);
            Label->SetColorAndOpacity(Color); Label->SetAutoWrapText(true);
            return Label;
        };
        const FLinearColor Ivory(.95f,.94f,.88f), Muted(.70f,.74f,.71f), Gold(.92f,.73f,.38f);
        Rows->AddChildToVerticalBox(Text(TEXT("与弟弟交流"),24,Gold));
        Rows->AddChildToVerticalBox(Text(TEXT("开发夹具 · 实际委托状态 / 本机模型回复"),12,Muted))->SetPadding(FMargin(0,4,0,12));
        Weight = Text(TEXT(""),14,Muted); Rows->AddChildToVerticalBox(Weight);
        Progress = Text(TEXT(""),17,Ivory); Rows->AddChildToVerticalBox(Progress)->SetPadding(FMargin(0,8));
        Status = Text(TEXT(""),17,Gold); Rows->AddChildToVerticalBox(Status)->SetPadding(FMargin(0,4));
        auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
        Rows->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Reply = Text(TEXT(""),18,Ivory); Scroll->AddChild(Reply);
        Rows->AddChildToVerticalBox(Text(TEXT("快捷建议 · 只在你刷新时更换"),14,Muted))->SetPadding(FMargin(0,8,0,2));
        auto MakeSuggestionButton = [&](const FString& Label,bool Track)
        {
            auto* B=WidgetTree->ConstructWidget<UButton>();
            B->SetBackgroundColor(FLinearColor(.12f,.15f,.14f));
            auto* L=Text(Label,13,Ivory); B->AddChild(L);
            Rows->AddChildToVerticalBox(B)->SetPadding(FMargin(0,2,0,0));
            if(Track){SuggestionButtons.Add(B);SuggestionLabels.Add(L);}
            return B;
        };
        SuggestionRefresh=MakeSuggestionButton(TEXT("刷新3条建议"),false);
        SuggestionRefresh->OnClicked.AddDynamic(this,&UHearthwardDialogueWidget::RefreshSuggestions);
        auto* S0=MakeSuggestionButton(TEXT("尚未刷新"),true);S0->OnClicked.AddDynamic(this,&UHearthwardDialogueWidget::SelectSuggestion0);
        auto* S1=MakeSuggestionButton(TEXT("尚未刷新"),true);S1->OnClicked.AddDynamic(this,&UHearthwardDialogueWidget::SelectSuggestion1);
        auto* S2=MakeSuggestionButton(TEXT("尚未刷新"),true);S2->OnClicked.AddDynamic(this,&UHearthwardDialogueWidget::SelectSuggestion2);
        Draft = WidgetTree->ConstructWidget<UEditableTextBox>();
        auto InputStyle = Draft->GetWidgetStyle();
        InputStyle.TextStyle.Font.Size = 18;
        InputStyle.ForegroundColor = FLinearColor(.035f,.042f,.042f);
        InputStyle.FocusedForegroundColor = InputStyle.ForegroundColor;
        Draft->SetWidgetStyle(InputStyle);
        Draft->SetHintText(FText::FromString(TEXT("输入想说的话（最多1000字）")));
        Draft->SetClearKeyboardFocusOnCommit(false);
        Draft->OnTextCommitted.AddDynamic(this,&UHearthwardDialogueWidget::CommitDraft);
        Rows->AddChildToVerticalBox(Draft)->SetPadding(FMargin(0,12,0,10));
        auto* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
        Rows->AddChildToVerticalBox(Buttons);
        auto Button = [&](const FString& Label)
        {
            auto* B = WidgetTree->ConstructWidget<UButton>();
            B->AddChild(Text(Label,15,Ivory));
            B->SetBackgroundColor(FLinearColor(.15f,.18f,.17f));
            Buttons->AddChildToHorizontalBox(B)->SetPadding(FMargin(0,0,12,0));
            return B;
        };
        Send=Button(TEXT("发送 Enter")); Send->OnClicked.AddDynamic(this,&UHearthwardDialogueWidget::SendDraft);
        StopReply=Button(TEXT("取消本次回复")); StopReply->OnClicked.AddDynamic(this,&UHearthwardDialogueWidget::CancelReply);
        StopTask=Button(TEXT("取消当前委托")); StopTask->OnClicked.AddDynamic(this,&UHearthwardDialogueWidget::CancelTask);
        Button(TEXT("关闭 Esc"))->OnClicked.AddDynamic(this,&UHearthwardDialogueWidget::Close);
        Rows->AddChildToVerticalBox(Text(TEXT("交流时世界继续运行 · 关闭面板不撤销已发送请求"),12,Muted))->SetPadding(FMargin(0,10,0,0));
    }
    return Super::RebuildWidget();
}

void UHearthwardDialogueWidget::BindHUD(AHearthwardHUD* InHUD) { HUD=InHUD; Refresh(); }
void UHearthwardDialogueWidget::Refresh()
{
    if (!HUD.IsValid() || !Status) return;
    Status->SetText(FText::FromString(HUD->GetDialogueStatus()));
    Progress->SetText(FText::FromString(HUD->GetDialogueProgress()));
    Reply->SetText(FText::FromString(HUD->GetDialogueReply()));
    Weight->SetText(FText::FromString(HUD->GetDialogueWeight()));
    const FString Value = Draft->GetText().ToString().TrimStartAndEnd();
    Send->SetIsEnabled(HUD->CanSendDialogue() && !Value.IsEmpty() && Value.Len()<=1000);
    StopReply->SetIsEnabled(HUD->CanCancelDialogueReply());
    StopTask->SetIsEnabled(HUD->CanCancelDialogueTask());
    const auto Suggestions=HUD->GetDialogueSuggestions();
    SuggestionIds.Reset();
    for(int32 I=0;I<SuggestionButtons.Num();++I)
    {
        const bool Available=Suggestions.IsValidIndex(I);
        if(SuggestionLabels.IsValidIndex(I))
            SuggestionLabels[I]->SetText(FText::FromString(Available?Suggestions[I].Label:TEXT("尚未刷新")));
        SuggestionButtons[I]->SetIsEnabled(Available && HUD->CanSendDialogue());
        if(Available)SuggestionIds.Add(Suggestions[I].Id);
    }
    if(SuggestionRefresh)SuggestionRefresh->SetIsEnabled(!HUD->CanCancelDialogueReply());
}
void UHearthwardDialogueWidget::NativeTick(const FGeometry& Geometry,float DeltaTime)
{
    Super::NativeTick(Geometry,DeltaTime);
    if(bFocusWhenMounted) { bFocusWhenMounted=false; FocusDraft(); }
    Refresh();
}
void UHearthwardDialogueWidget::SetDraft(const FString& Text) { if(Draft) Draft->SetText(FText::FromString(Text)); Refresh(); }
void UHearthwardDialogueWidget::RefreshSuggestionChoices() { RefreshSuggestions(); }
void UHearthwardDialogueWidget::SendDraft()
{
    if(HUD.IsValid() && Draft && HUD->SubmitDialogue(Draft->GetText().ToString())) Draft->SetText(FText::GetEmpty());
    Refresh(); FocusDraft();
}
void UHearthwardDialogueWidget::CommitDraft(const FText&,ETextCommit::Type Method) { if(Method==ETextCommit::OnEnter) SendDraft(); }
void UHearthwardDialogueWidget::CancelReply() { if(HUD.IsValid()) HUD->CancelDialogueReply(); Refresh(); }
void UHearthwardDialogueWidget::CancelTask() { if(HUD.IsValid()) HUD->CancelDialogueTask(); Refresh(); }
void UHearthwardDialogueWidget::RefreshSuggestions() { if(HUD.IsValid()) HUD->RefreshDialogueSuggestions(); Refresh(); }
void UHearthwardDialogueWidget::SelectSuggestion0() { if(HUD.IsValid()&&SuggestionIds.IsValidIndex(0)) HUD->SubmitDialogueSuggestion(SuggestionIds[0]); Refresh(); }
void UHearthwardDialogueWidget::SelectSuggestion1() { if(HUD.IsValid()&&SuggestionIds.IsValidIndex(1)) HUD->SubmitDialogueSuggestion(SuggestionIds[1]); Refresh(); }
void UHearthwardDialogueWidget::SelectSuggestion2() { if(HUD.IsValid()&&SuggestionIds.IsValidIndex(2)) HUD->SubmitDialogueSuggestion(SuggestionIds[2]); Refresh(); }
void UHearthwardDialogueWidget::Close() { if(HUD.IsValid()) HUD->CloseDialogue(); }
void UHearthwardDialogueWidget::FocusDraft() { if(Draft) Draft->SetKeyboardFocus(); }
bool UHearthwardDialogueWidget::HasDraftFocus() const { return Draft && (Draft->HasKeyboardFocus() || Draft->HasFocusedDescendants()); }
bool UHearthwardDialogueWidget::IsSendEnabled() const { return Send && Send->GetIsEnabled(); }
FString UHearthwardDialogueWidget::GetDisplayedStatus() const { return Status ? Status->GetText().ToString() : FString(); }
FString UHearthwardDialogueWidget::GetDisplayedProgress() const { return Progress ? Progress->GetText().ToString() : FString(); }
FString UHearthwardDialogueWidget::GetDisplayedReply() const { return Reply ? Reply->GetText().ToString() : FString(); }
TArray<FString> UHearthwardDialogueWidget::GetDisplayedSuggestions() const
{
    TArray<FString> Out;for(const auto& Label:SuggestionLabels)if(Label)Out.Add(Label->GetText().ToString());return Out;
}
FReply UHearthwardDialogueWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::F6 && HUD.IsValid()) { HUD->ToggleSaveMenu(); return FReply::Handled(); }
    if(Event.GetKey()==EKeys::Escape) { Close(); return FReply::Handled(); }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
