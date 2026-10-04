#include "HearthwardScreenWidget.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Update/HearthwardUpdateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UHearthwardScreenWidget::ComposeTitleControls()
{
    const auto* Update=GetGameInstance()->GetSubsystem<UHearthwardUpdateSubsystem>();
    const auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    const float TextScale=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f;
    auto Caption=[&](const FString& Text,FVector2D Position,FVector2D Size,float Font,const FString& Action=FString())
    {
        Element(Action.IsEmpty()?TEXT("text"):TEXT("titleUtility"),Text,Position,Size,Font,Action);
        auto& E=Elements.Last(); E.Font=Font*TextScale;
        E.Color=FLinearColor(FColor(118,115,109)); E.TextInset=0;
        E.Component=Position.Y>850?TEXT("title.footer"):TEXT("title.header");
    };
    Caption(TEXT("离线可玩"),{35,30},{450,26},16);
    Caption(TEXT("版本  ")+FString(HearthwardVersion::Current),{35,72},{530,25},14);
    Caption(Update->IsChecking()?TEXT("正在检查更新…"):Update->HasUpdate()?TEXT("发现新版本"):TEXT("归火 · Hearthward"),{35,101},{440,25},14);
    Caption(TEXT("检查更新"),{35,135},{120,32},14,TEXT("update.check"));
    Elements.Last().Enabled=!Update->IsChecking();
    Caption(Update->HasUpdate()?TEXT("查看新版本"):TEXT("正式发布页"),{173,135},{180,32},14,TEXT("update.open"));
    if(Save->HasSaveConflicts())
    {
        Caption(TEXT("查看存档兼容提醒"),{35,175},{330,36},16,TEXT("compat.show"));
        Elements.Last().Color=Color(TEXT("gold"));
    }
    Caption(TEXT("滚轮 / ↑ ↓  选择     Enter  确认"),{536,870},{600,30},14);
    Elements.Last().Align=TEXT("center");
    Caption(TEXT("SAME FIRE. A BRIGHTER TOMORROW."),{436,911},{800,22},11);
    Elements.Last().Align=TEXT("center");
    for(auto& E:Elements)
    {
        if(E.Type!=TEXT("titleOption")) continue;
        E.Selected=E.Action==TitleSelection;
        E.Color=FLinearColor(FColor(133,128,119));
    }
}

bool UHearthwardScreenWidget::SelectTitleOption(const FString& Action)
{
    const int32 Index=Elements.IndexOfByPredicate([&](const auto& E)
    { return E.Type==TEXT("titleOption") && E.Action==Action && E.Enabled && !E.Hidden; });
    if(Index==INDEX_NONE || !ConfirmAction.IsEmpty() || LayoutEditing) return false;
    TitleSelection=Action;
    PageFocus.Add(TEXT("title"),Action);
    KeyboardFocus=Index; Hover=INDEX_NONE;
    for(auto& E:Elements) if(E.Type==TEXT("titleOption")) E.Selected=E.Action==Action;
    return true;
}

void UHearthwardScreenWidget::ApplyTitleMenuWindow()
{
    if(Page!=TEXT("title")) return;
    constexpr int32 VisibleCount=4;
    TArray<int32> Options;
    for(int32 I=0;I<Elements.Num();++I)
        if(Elements[I].Type==TEXT("titleOption")) Options.Add(I);
    if(Options.IsEmpty()) return;
    TitleMenuFirst=FMath::Clamp(TitleMenuFirst,0,FMath::Max(0,Options.Num()-VisibleCount));
    const int32 Selected=Options.IndexOfByPredicate([&](int32 I){return Elements[I].Action==TitleSelection;});
    if(Selected!=INDEX_NONE)
    {
        if(Selected<TitleMenuFirst) TitleMenuFirst=Selected;
        if(Selected>=TitleMenuFirst+VisibleCount) TitleMenuFirst=Selected-VisibleCount+1;
    }
    const float Spacing=Options.Num()>1?Elements[Options[1]].Position.Y-Elements[Options[0]].Position.Y:44;
    auto UpdateBounds=[&](const FHearthwardUIElement& E)
    {
        if(auto* B=LayoutBounds.Find(E.LayoutId))
        { B->Position=E.Position;B->Size=E.Size;B->Hidden=E.Hidden; }
    };
    for(int32 I=0;I<Options.Num();++I)
    {
        auto& E=Elements[Options[I]];
        E.Position.Y-=TitleMenuFirst*Spacing;
        E.Hidden|=I<TitleMenuFirst || I>=TitleMenuFirst+VisibleCount;
        UpdateBounds(E);
    }
    for(auto& E:Elements)
    {
        if(E.Type!=TEXT("titleArrow")) continue;
        E.Hidden|=E.Action==TEXT("title.prev")?TitleMenuFirst==0:TitleMenuFirst+VisibleCount>=Options.Num();
        UpdateBounds(E);
    }
}

bool UHearthwardScreenWidget::NavigateTitle(int32 Direction)
{
    if(Page!=TEXT("title") || !ConfirmAction.IsEmpty() || LayoutEditing || Direction==0) return false;
    TArray<FString> Options;
    for(const auto& E:Elements)
        if(E.Type==TEXT("titleOption") && E.Enabled) Options.Add(E.Action);
    if(Options.IsEmpty()) return false;
    int32 Index=Options.IndexOfByKey(TitleSelection);
    if(Index==INDEX_NONE) Index=0;
    const int32 Next=FMath::Clamp(Index+(Direction>0?1:-1),0,Options.Num()-1);
    if(Next==Index) return false;
    // Selection can bring the fifth row into the four-row viewport.
    for(auto& E:Elements) if(E.Type==TEXT("titleOption") && E.Action==Options[Next]) E.Hidden=false;
    if(!SelectTitleOption(Options[Next])) return false;
    Refresh();
    return true;
}
