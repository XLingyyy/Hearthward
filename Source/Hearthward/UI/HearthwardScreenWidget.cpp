#include "HearthwardScreenWidget.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "Misc/ConfigCacheIni.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../AI/HearthwardLocalAISubsystem.h"
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
#include "Engine/World.h"
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
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    if(!Save->LoadPointIndex()) { Message=Save->GetStatus(); Refresh(); return; }
    if(UGameplayStatics::GetCurrentLevelName(GetWorld(),true)!=TEXT("L_HearthwardWilds")) return;
    const FString LoadId=GetWorld()->URL.GetOption(TEXT("HearthwardLoad="),TEXT(""));
    if(FCString::Strcmp(GetWorld()->URL.GetOption(TEXT("HearthwardNewGame="),TEXT("")),TEXT("1"))==0)
    {
        if(Save->EnableNaturalWorld() && Save->StartNewProgress()) { OpenPage(TEXT("hud")); return; }
    }
    else if(!LoadId.IsEmpty())
    {
        FGuid Id;
        if(FGuid::Parse(LoadId,Id) && Save->EnableNaturalWorld() && Save->LoadPoint(Id)) { OpenPage(TEXT("hud")); return; }
    }
    else return;
    Message=Save->GetStatus(); Refresh();
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
    if(!ReloadLayout()) UE_LOG(LogTemp,Fatal,TEXT("Cannot load Hearthward UI layout"));
}
FSlateBrush* UHearthwardScreenWidget::Brush(const FString& Name) const
{ return const_cast<FSlateBrush*>(Brushes.Find(Name)); }
FLinearColor UHearthwardScreenWidget::Color(const FString& Name) const
{
    return FLinearColor(FColor::FromHex(Theme->GetObjectField(TEXT("colors"))->GetStringField(Name)));
}
void UHearthwardScreenWidget::OpenPage(FName Name)
{
    if(UHearthwardSurvivalComponent::HasFailed(GetWorld()) && Name!=TEXT("save") && Name!=TEXT("title")) Name=TEXT("save");
    if(GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsNaturalWorldEnabled() && !Gameplay()->Enabled
        && Name!=TEXT("hud") && Name!=TEXT("title") && Name!=TEXT("pause")
        && Name!=TEXT("save") && Name!=TEXT("settings") && Name!=TEXT("inventory")
        && Name!=TEXT("dialogue") && Name!=TEXT("memory") && Name!=TEXT("storage")
        && Name!=TEXT("building") && Name!=TEXT("crafting") && Name!=TEXT("repairing") && Name!=TEXT("skills"))
    { Message=TEXT("该功能尚未接入自然地图"); MessageUntil=FPlatformTime::Seconds()+4; Refresh(); return; }
    if(Name==TEXT("camp"))
    {
        CampEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
        if(!GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->CanManage(CampEpoch)) {Message=TEXT("请在安全营地打开管理面板");MessageUntil=FPlatformTime::Seconds()+4;Refresh();return;}
    }
    const FName PreviousPage=Page;
    const bool RestoringMemoryDraft=Name==TEXT("memory") && !ReturnPages.IsEmpty() && ReturnPages.Last()==Name;
    if(Page==TEXT("dialogue") && Name!=Page) GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->CancelPending();
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
    bool RestoringParent=false;
    FString RestoredCategory;
    if(Name!=PreviousPage)
    {
        const bool Nested=Name==TEXT("settings") || Name==TEXT("save") ||
            (Name==TEXT("memory") && PreviousPage==TEXT("dialogue")) ||
            (Name==TEXT("map") && PreviousPage==TEXT("journal"));
        if(!ReturnPages.IsEmpty() && Name==ReturnPages.Last())
        {
            ReturnPages.Pop(); RestoredCategory=ReturnCategories.Pop(); RestoringParent=true;
        }
        else if(Nested) { ReturnPages.Add(PreviousPage); ReturnCategories.Add(Category); }
        else { ReturnPages.Reset(); ReturnCategories.Reset(); }
    }
    if(Page!=Name) Category=RestoringParent?RestoredCategory:FString();
    Page=Name; Scroll=0; Hover=KeyboardFocus=INDEX_NONE; ConfirmAction.Reset(); Message.Reset(); LayoutSelection.Reset(); LayoutDragging=false;
    if(Name==TEXT("journal") && Category.IsEmpty()) Category=TEXT("main");
    GConfig->GetBool(TEXT("Hearthward.Survival"),TEXT("MenuPause"),MenuPause,GGameUserSettingsIni);
    const bool Pause=LayoutEditing || Name==TEXT("title") || Name==TEXT("pause") || Name==TEXT("save")
        || (MenuPause && Name!=TEXT("hud") && Name!=TEXT("dialogue"));
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
    if (Draft)
    {
        Draft->SetVisibility(Name==TEXT("dialogue") || Name==TEXT("memory")?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        Draft->SetHintText(FText::FromString(Name==TEXT("memory")?TEXT("填写你要告诉弟弟的记录，最多120字…"):TEXT("输入想说的话…")));
        if(Name==TEXT("memory") && !RestoringMemoryDraft) Draft->SetText(FText::GetEmpty());
    }
    if(Name==TEXT("memory") && !RestoringMemoryDraft) { MemoryEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(); MemoryRevision=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->GetMemoryRevision(); SelectedMemory.Invalidate(); MemoryKind=TEXT("claim"); }
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
        if(GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsNaturalWorldEnabled() && !Gameplay()->Enabled)
        {
            if(Page==TEXT("hud")) continue;
            if(Page==TEXT("pause"))
            {
                const FString Bind=Text(R,TEXT("bind")),Label=Text(R,TEXT("text"));
                if(Bind==TEXT("quest") || Bind==TEXT("objective") || Bind==TEXT("location")
                    || Label==TEXT("当前任务") || Label==TEXT("当前位置")) continue;
            }
        }
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
    const FString FocusedAction=Elements.IsValidIndex(KeyboardFocus) && Elements[KeyboardFocus].Enabled &&
        !Elements[KeyboardFocus].Hidden ? Elements[KeyboardFocus].Action : FString();
    Elements.Reset(); const auto P=Theme->GetObjectField(TEXT("pages"))->GetObjectField(Page.ToString());
    const FString Background=Text(P,TEXT("background"));
    if(!Background.IsEmpty()) { Element(TEXT("image"),TEXT(""),FVector2D::ZeroVector,DesignSize,18,TEXT(""),Background); Elements.Last().LayoutId=TEXT("background"); }
    LoadComponents();
    if(P->GetBoolField(TEXT("header"))) LoadElements(Theme->GetArrayField(TEXT("header")));
    LoadElements(P->GetArrayField(TEXT("elements")));
    if(Page==TEXT("pause") && GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsNaturalWorldEnabled() && !Gameplay()->Enabled)
    {
        Element(TEXT("text"),TEXT("自然地图探索"),FVector2D(1096,220),FVector2D(280,36),22);
        const FVector Camp(-98000,-75000,0),Here=GetOwningPlayerPawn()->GetActorLocation();
        Element(TEXT("text"),FString::Printf(TEXT("距新营地 %.0f 米"),FVector::Dist2D(Camp,Here)/100),FVector2D(1152,510),FVector2D(245,40),19);
    }
    if(Page==TEXT("pause"))
    {
        Element(TEXT("button"),MenuPause?TEXT("普通菜单暂停：开"):TEXT("普通菜单暂停：关"),FVector2D(1100,765),FVector2D(280,48),18,TEXT("menuPause"));
        auto* S=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardSurvivalComponent>();
        if(S && S->State.Life==EHearthwardLife::Downed)
            Element(TEXT("button"),TEXT("放弃救援并回档"),FVector2D(1100,640),FVector2D(280,55),20,TEXT("giveUp"));
        if(S && S->Busy()) Element(TEXT("button"),TEXT("取消当前动作"),FVector2D(1100,705),FVector2D(280,55),20,TEXT("cancelSurvival"));
    }
    if(Page==TEXT("inventory")) ComposeInventory(false);
    if(Page==TEXT("storage")) ComposeInventory(true);
    if(Page==TEXT("skills")) ComposeSkills();
    if(Page==TEXT("map")) ComposeMap();
    if(Page==TEXT("journal")) ComposeJournal();
    if(Page==TEXT("dialogue")) ComposeDialogue();
    if(Page==TEXT("memory")) ComposeMemory();
    if(Page==TEXT("hud")) ComposeHUD();
    if(Page==TEXT("building")) ComposeBuilding();
    if(Page==TEXT("camp")) ComposeCamp();
    if(Page==TEXT("crafting")) ComposeCrafting();
    if(Page==TEXT("repairing")) ComposeRepair();
    if(Page==TEXT("save")) ComposeSave();
    if(!Message.IsEmpty()) Element(TEXT("notice"),Message,FVector2D(440,820),FVector2D(790,42),17);
    if(!ConfirmAction.IsEmpty())
    {
        Element(TEXT("panel"),TEXT(""),FVector2D(490,310),FVector2D(690,290));
        Element(TEXT("text"),TEXT("确认操作"),FVector2D(550,342),FVector2D(580,45),28);
        Element(TEXT("text"),(ConfirmAction.StartsWith(TEXT("camp."))?ConfirmMessage:TEXT("未保存的进度可能丢失。是否继续？")),FVector2D(550,410),FVector2D(580,50),20);
        Element(TEXT("button"),TEXT("确认"),FVector2D(550,510),FVector2D(240,55),22,TEXT("confirm"));
        Element(TEXT("button"),TEXT("返回"),FVector2D(850,510),FVector2D(240,55),22,TEXT("cancel"));
    }
    ApplyLayout();
    KeyboardFocus=FocusedAction.IsEmpty()?INDEX_NONE:Elements.IndexOfByPredicate([&](const FHearthwardUIElement& E)
    { return E.Action==FocusedAction && E.Enabled && !E.Hidden; });
}
void UHearthwardScreenWidget::NativeTick(const FGeometry& G,float Delta)
{
    Super::NativeTick(G,Delta);
    if(UHearthwardSurvivalComponent::HasFailed(GetWorld()) && Page!=TEXT("save") && Page!=TEXT("title"))
    {
        GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->LoadPointIndex();
        OpenPage(TEXT("save"));
    }
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
    if(E.GetKey()==EKeys::F6)
    {
        if(ConfirmAction.IsEmpty()) ExecuteAction(Page==TEXT("save")?TEXT("back"):TEXT("page:save"));
        return FReply::Handled();
    }
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
    if(Key==EKeys::Escape) { ExecuteAction(TEXT("back")); return FReply::Handled(); }
    const bool HasCampaign=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetCampaignId().IsValid();
    if(Key==EKeys::Tab && HasCampaign) { OpenPage(Page==TEXT("inventory")?TEXT("hud"):TEXT("inventory")); return FReply::Handled(); }
    if(Key==EKeys::T && Page==TEXT("dialogue")) { ExecuteAction(TEXT("back")); return FReply::Handled(); }
    if(Key==EKeys::T && HasCampaign) { ExecuteAction(TEXT("page:dialogue")); return FReply::Handled(); }
    if(Key==EKeys::R && Page==TEXT("storage")) { ExecuteAction(TEXT("back")); return FReply::Handled(); }
    if(Key==EKeys::Enter && Elements.IsValidIndex(KeyboardFocus) && Elements[KeyboardFocus].Enabled &&
        !Elements[KeyboardFocus].Hidden && !Elements[KeyboardFocus].Action.IsEmpty())
    { ExecuteAction(Elements[KeyboardFocus].Action); return FReply::Handled(); }
    if(Key==EKeys::Up || Key==EKeys::Down)
    {
        Hover=INDEX_NONE;
        const int32 Direction=Key==EKeys::Down?1:-1;
        int32 Candidate=KeyboardFocus==INDEX_NONE?(Direction>0?Elements.Num()-1:0):KeyboardFocus;
        KeyboardFocus=INDEX_NONE;
        for(int32 N=0;N<Elements.Num();++N)
        {
            Candidate=(Candidate+Direction+Elements.Num())%Elements.Num();
            if(!Elements[Candidate].Action.IsEmpty() && !Elements[Candidate].Hidden && Elements[Candidate].Enabled)
            { KeyboardFocus=Candidate; break; }
        }
        return FReply::Handled();
    }
    if(Key==EKeys::F && Page==TEXT("inventory")) ExecuteAction(TEXT("use"));
    if(Key==EKeys::F && Page==TEXT("crafting") && Elements.ContainsByPredicate([](const FHearthwardUIElement& Item)
        { return Item.Action==TEXT("craft") && Item.Enabled && !Item.Hidden; })) ExecuteAction(TEXT("craft"));
    if(Key==EKeys::F && Page==TEXT("repairing") && Elements.ContainsByPredicate([](const FHearthwardUIElement& Item)
        { return Item.Action==TEXT("repairEquipment") && Item.Enabled && !Item.Hidden; })) ExecuteAction(TEXT("repairEquipment"));
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
{ if(Method==ETextCommit::OnEnter) ExecuteAction(Page==TEXT("memory")?TEXT("memorySave"):TEXT("send")); }
