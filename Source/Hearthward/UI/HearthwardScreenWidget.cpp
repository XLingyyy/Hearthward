#include "HearthwardScreenWidget.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "HearthwardHUD.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

using namespace HearthwardData;
TSharedRef<SWidget> UHearthwardScreenWidget::RebuildWidget()
{
    auto* Scale=WidgetTree->ConstructWidget<UScaleBox>(); Scale->SetStretch(EStretch::ScaleToFit);
    auto* Box=WidgetTree->ConstructWidget<USizeBox>(); Box->SetWidthOverride(1672); Box->SetHeightOverride(941); Scale->SetContent(Box);
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>(); Box->SetContent(Canvas);
    Draft=WidgetTree->ConstructWidget<UEditableTextBox>(); Draft->SetHintText(FText::FromString(TEXT("输入想说的话…")));
    FEditableTextBoxStyle Style=FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>(TEXT("NormalEditableTextBox"));
    Style.SetForegroundColor(FLinearColor(.8f,.73f,.58f)); Style.SetFocusedForegroundColor(FLinearColor(.95f,.85f,.65f));
    Style.SetBackgroundColor(FLinearColor(.018f,.016f,.012f));
    if(!Theme) LoadTheme();
    Style.TextStyle.Font=FSlateFontInfo(Typeface,16);
    Draft->SetWidgetStyle(Style);
    auto* CanvasSlot=Canvas->AddChildToCanvas(Draft); CanvasSlot->SetPosition(FVector2D(960,766)); CanvasSlot->SetSize(FVector2D(510,52));
    Draft->OnTextCommitted.AddDynamic(this,&UHearthwardScreenWidget::DraftCommitted);
    Draft->SetVisibility(ESlateVisibility::Collapsed);
    WidgetTree->RootWidget=Scale;
    return Super::RebuildWidget();
}
void UHearthwardScreenWidget::InitializeScreen(AHearthwardHUD* HUD)
{
    OwnerHUD=HUD; SetIsFocusable(true); if(!Theme) LoadTheme(); OpenPage(TEXT("title"));
}
UHearthwardGameplayComponent* UHearthwardScreenWidget::Gameplay() const
{ return GetOwningPlayerPawn() ? GetOwningPlayerPawn()->FindComponentByClass<UHearthwardGameplayComponent>() : nullptr; }
UHearthwardInventoryComponent* UHearthwardScreenWidget::Inventory() const
{ return GetOwningPlayerPawn() ? GetOwningPlayerPawn()->FindComponentByClass<UHearthwardInventoryComponent>() : nullptr; }
void UHearthwardScreenWidget::LoadTheme()
{
    FString Json; const FString Root=FPaths::ProjectDir()/TEXT("Resources/UI");
    if (!FFileHelper::LoadFileToString(Json,*(Root/TEXT("interface.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Theme) || !Theme)
        UE_LOG(LogTemp,Fatal,TEXT("Cannot load Hearthward UI theme"));
    const auto Typography=Theme->GetObjectField(TEXT("typography"));
    Typeface=MakeShared<FCompositeFont>(NAME_None,Root/Text(Typography,TEXT("body")),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
    DisplayTypeface=MakeShared<FCompositeFont>(NAME_None,Root/Text(Typography,TEXT("display")),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
    TMap<FString,UTexture2D*> Loaded;
    for(const auto& Entry:Theme->GetObjectField(TEXT("assets"))->Values)
    {
        const auto R=Entry.Value->AsObject(); const FString File=Text(R,TEXT("file"));
        UTexture2D* Texture=Loaded.FindRef(File);
        if (!Texture)
        {
            Texture=FImageUtils::ImportFileAsTexture2D(Root/File);
            checkf(Texture,TEXT("Missing UI art %s"),*File);
            Textures.Add(Texture); Loaded.Add(File,Texture);
        }
        FSlateBrush B; B.SetResourceObject(Texture); B.DrawAs=ESlateBrushDrawType::Image; B.ImageSize=FVector2D(Texture->GetSizeX(),Texture->GetSizeY());
        B.SetUVRegion(FBox2f(FVector2f(0,0),FVector2f(1,1)));
        const TArray<TSharedPtr<FJsonValue>>* Margin;
        if(R->TryGetArrayField(TEXT("margin"),Margin))
        {
            B.DrawAs=ESlateBrushDrawType::Box;
            B.Margin=FMargin((*Margin)[0]->AsNumber(),(*Margin)[1]->AsNumber(),(*Margin)[2]->AsNumber(),(*Margin)[3]->AsNumber());
            B.ImageSize=FVector2D(Number(R,TEXT("brushSize"),128));
        }
        const TArray<TSharedPtr<FJsonValue>>* UV;
        if(R->TryGetArrayField(TEXT("uv"),UV))
        {
            const FVector2D P((*UV)[0]->AsNumber()/Texture->GetSizeX(),(*UV)[1]->AsNumber()/Texture->GetSizeY());
            const FVector2D S((*UV)[2]->AsNumber()/Texture->GetSizeX(),(*UV)[3]->AsNumber()/Texture->GetSizeY());
            B.SetUVRegion(FBox2f(FVector2f(P),FVector2f(P+S)));
        }
        Brushes.Add(FString(*Entry.Key),B);
    }
    checkf(ReloadLayout(),TEXT("Cannot load Hearthward UI layout"));
}
FSlateBrush* UHearthwardScreenWidget::Brush(const FString& Name) const
{ return const_cast<FSlateBrush*>(Brushes.Find(Name)); }
FLinearColor UHearthwardScreenWidget::Color(const FString& Name) const
{
    return FLinearColor(FColor::FromHex(Theme->GetObjectField(TEXT("colors"))->GetStringField(Name)));
}
void UHearthwardScreenWidget::OpenPage(FName Name)
{
    if(Name==TEXT("crafting") || Name==TEXT("repairing"))
    {
        auto* B=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>();
        Workbench=B->NearbyWorkbench();
        if(!Workbench.IsValid()) { Message=TEXT("请在安全处靠近已建成的工作台"); MessageUntil=FPlatformTime::Seconds()+4; Refresh(); return; }
        CraftingEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
        CraftingBatches=1;
        if(!Find(TEXT("craftingRecipes"),SelectedRecipe.ToString()) && !Rows(TEXT("craftingRecipes")).IsEmpty())
            SelectedRecipe=FName(*Text(Rows(TEXT("craftingRecipes"))[0]->AsObject(),TEXT("id")));
    }
    else { Workbench.Invalidate(); CraftingEpoch.Invalidate(); }
    if(Name!=TEXT("hud") && GetOwningPlayerPawn())
        if(auto* B=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>();B && B->IsPlacing()) B->CancelPlacement();
    if (Gameplay()) Gameplay()->SetSprinting(false);
    if((Name==TEXT("settings") || Name==TEXT("save")) && Page!=Name) ReturnPage=Page;
    if(Page!=Name) Category.Reset();
    Page=Name; Scroll=0; Hover=KeyboardFocus=INDEX_NONE; ConfirmAction.Reset(); Message.Reset(); LayoutSelection.Reset(); LayoutDragging=false;
    if(Name==TEXT("journal") && Category.IsEmpty()) Category=TEXT("main");
    const bool Pause=LayoutEditing || (Name!=TEXT("hud") && Name!=TEXT("dialogue"));
    if (Pause && !GetWorld()->IsPaused()) OwnPause=UGameplayStatics::SetGamePaused(this,true);
    else if (!Pause && OwnPause) { UGameplayStatics::SetGamePaused(this,false); OwnPause=false; }
    auto* Player=GetOwningPlayer(); Player->FlushPressedKeys();
    if (Name==TEXT("hud") && !LayoutEditing)
    {
        SetVisibility(ESlateVisibility::HitTestInvisible); Player->SetInputMode(FInputModeGameOnly()); Player->bShowMouseCursor=false;
    }
    else
    {
        SetVisibility(ESlateVisibility::Visible); FInputModeUIOnly Mode; Mode.SetWidgetToFocus(TakeWidget()); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Player->SetInputMode(Mode); Player->bShowMouseCursor=true; SetKeyboardFocus();
    }
    if (Draft) Draft->SetVisibility(Name==TEXT("dialogue")?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if (Name==TEXT("storage")) StorageEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    Refresh();
}
void UHearthwardScreenWidget::Element(FString Type,FString TextValue,FVector2D P,FVector2D Size,float Font,FString Action,FString Asset,bool Selected)
{
    FHearthwardUIElement E; E.Type=Type; E.Text=TextValue; E.Position=P; E.Size=Size; E.Font=Font; E.Action=Action; E.Asset=Asset; E.Selected=Selected;
    E.Color=Color(TEXT("text"));
    E.Tracking=Number(Theme->GetObjectField(TEXT("typography")),TEXT("tracking"));
    Elements.Add(MoveTemp(E));
}
void UHearthwardScreenWidget::LoadElements(const TArray<TSharedPtr<FJsonValue>>& Rows)
{
    for(const auto& V:Rows)
    {
        const auto R=V->AsObject();
        const TArray<TSharedPtr<FJsonValue>>* Categories;
        if(R->TryGetArrayField(TEXT("categories"),Categories) && !Categories->ContainsByPredicate([&](const auto& C){return C->AsString()==Category;})) continue;
        const auto& Rect=R->GetArrayField(TEXT("rect"));
        Element(Text(R,TEXT("type")),Text(R,TEXT("text")),FVector2D(Rect[0]->AsNumber(),Rect[1]->AsNumber()),FVector2D(Rect[2]->AsNumber(),Rect[3]->AsNumber()),Number(R,TEXT("font"),18),Text(R,TEXT("action")),Text(R,TEXT("asset")));
        auto& E=Elements.Last(); E.Bind=Text(R,TEXT("bind")); E.Id=Text(R,TEXT("id"));
        E.LayoutId=Text(R,TEXT("layoutId")); E.Component=Text(R,TEXT("component"));
        E.TextInset=Number(R,TEXT("textInset"),18); E.Tracking=Number(R,TEXT("tracking"),E.Tracking); E.FontRole=Text(R,TEXT("fontRole"));
        E.Align=Text(R,TEXT("align"));
        if(Text(R,TEXT("anchorX"))==TEXT("center")) E.Position.X+=(DesignSize.X-E.Size.X)*.5;
        const FString C=Text(R,TEXT("color")); if(!C.IsEmpty()) E.Color=Color(C);
        if(!E.Bind.IsEmpty()) E.Text=Resolve(E.Bind);
        E.Selected=E.Id==TEXT("active") || E.Action==TEXT("page:")+Page.ToString() || E.Action==TEXT("category:")+Category || E.Action==TEXT("filter:")+Category;
    }
}
void UHearthwardScreenWidget::Refresh()
{
    if(!Theme || !LayoutConfig || !Gameplay()) return;
    Elements.Reset(); const auto P=Theme->GetObjectField(TEXT("pages"))->GetObjectField(Page.ToString());
    const FString Background=Text(P,TEXT("background"));
    if(!Background.IsEmpty()) { Element(TEXT("image"),TEXT(""),FVector2D::ZeroVector,DesignSize,18,TEXT(""),Background); Elements.Last().LayoutId=TEXT("background"); }
    LoadComponents();
    if(P->GetBoolField(TEXT("header"))) LoadElements(Theme->GetArrayField(TEXT("header")));
    LoadElements(P->GetArrayField(TEXT("elements")));
    if(Page==TEXT("inventory")) ComposeInventory(false);
    if(Page==TEXT("storage")) ComposeInventory(true);
    if(Page==TEXT("skills")) ComposeSkills();
    if(Page==TEXT("map")) ComposeMap();
    if(Page==TEXT("journal")) ComposeJournal();
    if(Page==TEXT("dialogue")) ComposeDialogue();
    if(Page==TEXT("hud")) ComposeHUD();
    if(Page==TEXT("building")) ComposeBuilding();
    if(Page==TEXT("crafting")) ComposeCrafting();
    if(Page==TEXT("repairing")) ComposeRepair();
    if(Page==TEXT("save")) ComposeSave();
    if(!Message.IsEmpty()) Element(TEXT("notice"),Message,FVector2D(440,820),FVector2D(790,42),17);
    if(!ConfirmAction.IsEmpty())
    {
        Element(TEXT("panel"),TEXT(""),FVector2D(490,310),FVector2D(690,290));
        Element(TEXT("text"),TEXT("确认操作"),FVector2D(550,342),FVector2D(580,45),28);
        Element(TEXT("text"),TEXT("未保存的进度可能丢失。是否继续？"),FVector2D(550,410),FVector2D(580,50),20);
        Element(TEXT("button"),TEXT("确认"),FVector2D(550,510),FVector2D(240,55),22,TEXT("confirm"));
        Element(TEXT("button"),TEXT("返回"),FVector2D(850,510),FVector2D(240,55),22,TEXT("cancel"));
    }
    ApplyLayout();
}
void UHearthwardScreenWidget::NativeTick(const FGeometry& G,float Delta)
{
    Super::NativeTick(G,Delta);
    if((RefreshDelay-=Delta)<=0) { RefreshDelay=.2f; Refresh(); }
    if(!Message.IsEmpty() && FPlatformTime::Seconds()>MessageUntil) { Message.Reset(); }
}
FVector2D UHearthwardScreenWidget::CanvasPoint(const FGeometry& G,const FVector2D& Screen) const
{
    const float Scale=FMath::Min(G.GetLocalSize().X/DesignSize.X,G.GetLocalSize().Y/DesignSize.Y);
    return (G.AbsoluteToLocal(Screen)-(G.GetLocalSize()-DesignSize*Scale)*.5)/Scale;
}
int32 UHearthwardScreenWidget::Hit(const FVector2D& P) const
{
    for(int32 I=Elements.Num()-1;I>=0;--I)
    {
        const auto& E=Elements[I]; if(E.Action.IsEmpty() || !E.Enabled || E.Hidden) continue;
        if(!ConfirmAction.IsEmpty() && E.Action!=TEXT("confirm") && E.Action!=TEXT("cancel")) continue;
        const auto MapPoint=ComponentPoint(TEXT("map.canvas"),P,true);
        if(E.MapClipped && (MapPoint.X<407 || MapPoint.X>1517 || MapPoint.Y<95 || MapPoint.Y>855)) continue;
        if(P.X>=E.Position.X && P.Y>=E.Position.Y && P.X<E.Position.X+E.Size.X && P.Y<E.Position.Y+E.Size.Y) return I;
    }
    return INDEX_NONE;
}
FReply UHearthwardScreenWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
    if(LayoutEditing) return LayoutMouseDown(G,E);
    if(Page==TEXT("map") && ConfirmAction.IsEmpty() && E.GetEffectingButton()==EKeys::RightMouseButton)
    {
        if(const auto* Bounds=LayoutBounds.Find(TEXT("map.canvas"));Bounds && Bounds->Hidden) return FReply::Handled();
        const FVector2D P=ComponentPoint(TEXT("map.canvas"),CanvasPoint(G,E.GetScreenSpacePosition()),true);
        if(P.X>=407 && P.X<=1517 && P.Y>=95 && P.Y<=855)
        {
            const FVector2D Local=(P-FVector2D(962,475)-MapPan)/MapZoom;
            const FVector Origin=Gameplay()->LocationPosition(TEXT("camp"));
            Gameplay()->SetWaypoint(Origin+FVector(Local.X*6000/1110,-Local.Y*6000/760,0)); Refresh();
        }
        return FReply::Handled();
    }
    if(E.GetEffectingButton()==EKeys::LeftMouseButton)
    { const int32 I=Hit(CanvasPoint(G,E.GetScreenSpacePosition())); if(Elements.IsValidIndex(I)) { ExecuteAction(Elements[I].Action); return FReply::Handled(); } }
    return Super::NativeOnMouseButtonDown(G,E);
}
FReply UHearthwardScreenWidget::NativeOnMouseMove(const FGeometry& G,const FPointerEvent& E)
{
    if(LayoutEditing)
    {
        if(LayoutDragging)
        {
            FVector2D Delta=CanvasPoint(G,E.GetScreenSpacePosition())-DragStart;
            if(E.IsShiftDown()) { Delta.X=FMath::GridSnap(Delta.X,10.); Delta.Y=FMath::GridSnap(Delta.Y,10.); }
            SetComponentRect(LayoutSelection,LayoutResizing?DragPosition:DragPosition+Delta,LayoutResizing?FVector2D(FMath::Max(16.,DragSize.X+Delta.X),FMath::Max(16.,DragSize.Y+Delta.Y)):DragSize);
        }
        return FReply::Handled();
    }
    if(!E.GetCursorDelta().IsNearlyZero()) KeyboardFocus=INDEX_NONE;
    Hover=Hit(CanvasPoint(G,E.GetScreenSpacePosition())); return FReply::Handled();
}
void UHearthwardScreenWidget::NativeOnMouseLeave(const FPointerEvent& E)
{ Hover=INDEX_NONE; Super::NativeOnMouseLeave(E); }
FReply UHearthwardScreenWidget::NativeOnMouseWheel(const FGeometry& G,const FPointerEvent& E)
{
    if(LayoutEditing) return FReply::Handled();
    if(Page==TEXT("map")) MapZoom=FMath::Clamp(MapZoom+E.GetWheelDelta()*.1f,1.f,2.f);
    else Scroll=FMath::Max(0,Scroll-(E.GetWheelDelta()>0?1:-1));
    Refresh(); return FReply::Handled();
}
FReply UHearthwardScreenWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    if(LayoutKey(E)) return FReply::Handled();
    if(E.GetKey()==EKeys::Escape)
    {
        ExecuteAction(ConfirmAction.IsEmpty()?TEXT("back"):TEXT("cancel"));
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(G,E);
}
FReply UHearthwardScreenWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    const FKey Key=E.GetKey();
    if(!ConfirmAction.IsEmpty())
    {
        if(Key==EKeys::Escape) ExecuteAction(TEXT("cancel"));
        if(Key==EKeys::Enter) ExecuteAction(TEXT("confirm"));
        return FReply::Handled();
    }
    if(Page==TEXT("map") && (Key==EKeys::Left || Key==EKeys::Right || Key==EKeys::Up || Key==EKeys::Down))
    {
        MapPan+=FVector2D(Key==EKeys::Left?30:Key==EKeys::Right?-30:0,Key==EKeys::Up?30:Key==EKeys::Down?-30:0);
        MapPan.X=FMath::Clamp(MapPan.X,-450.f,450.f); MapPan.Y=FMath::Clamp(MapPan.Y,-300.f,300.f);
        Refresh(); return FReply::Handled();
    }
    if(Key==EKeys::Escape) { ExecuteAction(ConfirmAction.IsEmpty()?TEXT("page:hud"):TEXT("cancel")); return FReply::Handled(); }
    const bool HasCampaign=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetCampaignId().IsValid();
    if(Key==EKeys::Tab && HasCampaign) { OpenPage(Page==TEXT("inventory")?TEXT("hud"):TEXT("inventory")); return FReply::Handled(); }
    if(Key==EKeys::Enter && Elements.IsValidIndex(KeyboardFocus) && !Elements[KeyboardFocus].Hidden) { ExecuteAction(Elements[KeyboardFocus].Action); return FReply::Handled(); }
    if(Key==EKeys::Up || Key==EKeys::Down)
    {
        Hover=INDEX_NONE;
        const int32 Direction=Key==EKeys::Down?1:-1;
        for(int32 N=0;N<Elements.Num();++N)
        { KeyboardFocus=(KeyboardFocus+Direction+Elements.Num())%Elements.Num(); if(!Elements[KeyboardFocus].Action.IsEmpty() && !Elements[KeyboardFocus].Hidden && Elements[KeyboardFocus].Enabled) break; }
        return FReply::Handled();
    }
    if(Key==EKeys::F && Page==TEXT("inventory")) ExecuteAction(TEXT("use"));
    if(Key==EKeys::F && Page==TEXT("crafting")) ExecuteAction(TEXT("craft"));
    if(Key==EKeys::F && Page==TEXT("repairing")) ExecuteAction(TEXT("repairEquipment"));
    if(Key==EKeys::F && Page==TEXT("skills")) ExecuteAction(TEXT("learn"));
    if(Key==EKeys::F && Page==TEXT("journal") && (Category==TEXT("main") || Category==TEXT("side"))) ExecuteAction(TEXT("questMap"));
    if(Key==EKeys::V && Page==TEXT("journal") && (Category==TEXT("main") || Category==TEXT("side"))) ExecuteAction(TEXT("track"));
    if(Key==EKeys::R && Page==TEXT("inventory")) ExecuteAction(TEXT("drop"));
    if(Key==EKeys::H && Page==TEXT("inventory")) ExecuteAction(TEXT("repair"));
    if(Key==EKeys::E && Page==TEXT("storage")) ExecuteAction(TEXT("transfer"));
    if(Key==EKeys::P && HasCampaign) ExecuteAction(Page==TEXT("pause")?TEXT("page:hud"):TEXT("page:pause"));
    if(Key==EKeys::M && HasCampaign) OpenPage(TEXT("map"));
    if(Key==EKeys::K && HasCampaign) OpenPage(TEXT("skills"));
    if(Key==EKeys::J && HasCampaign) OpenPage(TEXT("journal"));
    return FReply::Handled();
}
void UHearthwardScreenWidget::DraftCommitted(const FText& TextValue,ETextCommit::Type Method)
{ if(Method==ETextCommit::OnEnter) ExecuteAction(TEXT("send")); }
