#include "HearthwardScreenWidget.h"
#include "../HearthwardCharacter.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "Misc/ConfigCacheIni.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "HearthwardHUD.h"
#include "HearthwardLoadingSubsystem.h"
#include "Engine/GameInstance.h"
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
#include "EngineUtils.h"
#include "ImageUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

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
    OwnerHUD=HUD; SetIsFocusable(true); if(!Theme) LoadTheme();
    GConfig->GetInt(TEXT("Hearthward.Audio"),TEXT("MasterVolume"),SettingsVolume,GGameUserSettingsIni);
    SettingsVolume=FMath::Clamp(SettingsVolume,0,100);
    ApplyMasterVolume();
    OpenPage(TEXT("title"));
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    if(!Save->LoadPointIndex()) { Message=Save->GetStatus(); Refresh(); return; }
    if(UGameplayStatics::GetCurrentLevelName(GetWorld(),true)!=TEXT("L_HearthwardWilds")) return;
    const FString LoadId=GetWorld()->URL.GetOption(TEXT("HearthwardLoad="),TEXT(""));
    auto* Loading=GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>();
    if(FCString::Strcmp(GetWorld()->URL.GetOption(TEXT("HearthwardNewGame="),TEXT("")),TEXT("1"))==0)
    {
        Loading->BeginLoading();
        if(Save->EnableNaturalWorld() && Save->StartNewProgress()) { OpenPage(TEXT("hud"));Loading->FinishSession(true); return; }
    }
    else if(!LoadId.IsEmpty())
    {
        Loading->BeginLoading();
        FGuid Id;
        if(FGuid::Parse(LoadId,Id) && Save->EnableNaturalWorld() && Save->LoadPoint(Id))
        {
            auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
            if(Campaign->State.Victory && Campaign->State.Facts.Contains(TEXT("home_saved")))Campaign->Record(TEXT("home_continued"));
            OpenPage(TEXT("hud"));Loading->FinishSession(true);return;
        }
    }
    else return;
    Loading->FinishSession(false);
    Message=Save->GetStatus(); Refresh();
}
void UHearthwardScreenWidget::NativeDestruct()
{
    if(SettingsSoundMix && GetWorld()) UGameplayStatics::PopSoundMixModifier(this,SettingsSoundMix);
    Super::NativeDestruct();
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
        if(GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Tier>=2)GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Record(TEXT("tier2_viewed"));
    }
    if(Name==TEXT("repairing"))Name=TEXT("equipment");
    if(Name==TEXT("equipment"))EquipmentEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    if(DisplayDeadline>0) {FinishDisplayChange(false);}
    if(Elements.IsValidIndex(KeyboardFocus)) PageFocus.Add(Page,Elements[KeyboardFocus].Action);
    BindingCapture=NAME_None;
    if(auto* C=Cast<AHearthwardCharacter>(GetOwningPlayerPawn())) C->ResetHeldInput();
    const FName PreviousPage=Page;
    const bool RestoringMemoryDraft=Name==TEXT("memory") && !ReturnPages.IsEmpty() && ReturnPages.Last()==Name;
    if(Page==TEXT("dialogue") && Name!=Page)
    {GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->CancelPending();GetOwningPlayerPawn()->FindComponentByClass<UHearthwardPresentationComponent>()->StopFixedCue();}
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
    Page=Name; TextScroll=0; Scroll=0; Hover=KeyboardFocus=INDEX_NONE; ConfirmAction.Reset(); Message.Reset(); LayoutSelection.Reset(); LayoutDragging=false;
    if(Name==TEXT("journal") && Category.IsEmpty()) Category=TEXT("main");
    GConfig->GetBool(TEXT("Hearthward.Survival"),TEXT("MenuPause"),MenuPause,GGameUserSettingsIni);
    if(Name==TEXT("settings") && PreviousPage!=Name)
    {
        Category=TEXT("游戏");
        SettingsSelection.Reset();
        LoadSettingsDraft();
    }
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
    if(Name==TEXT("nature")) NatureEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    if (Name==TEXT("storage")) StorageEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    if(Name==TEXT("dialogue") && PreviousPage!=Name)
    {
        auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();const FName Quest=Gameplay()->TrackedQuest;
        const FName Cue(*(TEXT("fixed.")+Quest.ToString()+TEXT(".intro"))),Fact(*(TEXT("cue:")+Cue.ToString()));
        const FName Location=Campaign->QuestLocation(Quest);
        const bool Observed=Gameplay()->Discovered.Contains(Location) && FVector::Dist(GetOwningPlayerPawn()->GetActorLocation(),Gameplay()->LocationPosition(Location))<=3000;
        if(Campaign->Available(Quest) && !Campaign->State.Facts.Contains(Fact) && Observed)
            if(GetOwningPlayerPawn()->FindComponentByClass<UHearthwardPresentationComponent>()->PlayFixedCue(Cue,FGuid::NewGuid(),true)) Campaign->Record(Fact);
    }
    Refresh();
}
void UHearthwardScreenWidget::Element(FString Type,FString TextValue,FVector2D P,FVector2D Size,float Font,FString Action,FString Asset,bool Selected)
{
    const auto& Bindings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings;
    for(const auto& D:HearthwardInput::Definitions()) if(D.Contexts.Contains(Page) && D.Primary.Key.IsValid())
    {
        const FString Old=D.Primary.Key.GetDisplayName().ToString(),Current=HearthwardInput::Label(Bindings,D.Id);
        if(Current!=Old)
        {
            if(TextValue.StartsWith(Old+TEXT(" "))) TextValue=Current+TextValue.Mid(Old.Len());
            TextValue.ReplaceInline(*(TEXT("[")+Old+TEXT("]")),*(TEXT("[")+Current+TEXT("]")));
        }
    }
    FHearthwardUIElement E; E.Type=Type; E.Text=TextValue; E.Position=P; E.Size=Size; E.Font=Font; E.Action=Action; E.Asset=Asset; E.Selected=Selected;
    const float TextScale=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f;
    if(!TextValue.IsEmpty()) E.Font=FMath::Max(Font,24.f)*TextScale;
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
    if(Page==TEXT("settings")) ComposeSettings();
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
    if(Page==TEXT("inventory")){ComposeInventory(false);Element(TEXT("button"),TEXT("逐件装备 · 行装管理"),FVector2D(260,810),FVector2D(280,45),18,TEXT("page:equipment"));}
    if(Page==TEXT("equipment"))ComposeEquipment();
    if(Page==TEXT("nature"))ComposeNature();
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
        if(ConfirmAction==TEXT("resetAgreements"))
        {
            Element(TEXT("panel"),TEXT(""),FVector2D(350,165),FVector2D(972,635));
            Element(TEXT("text"),TEXT("取消所有任务和约定"),FVector2D(405,202),FVector2D(850,45),27);
            Element(TEXT("text"),TEXT("确认后停止所列委托、放弃候选卡、撤销所列玩家规则。事实、行动回执和物资保留。"),FVector2D(405,254),FVector2D(850,70),18);
            for(int32 I=ResetScroll;I<FMath::Min(ResetScroll+2,ResetItems.Num());++I)
                Element(TEXT("text"),FString::Printf(TEXT("%d/%d  %s"),I+1,ResetItems.Num(),*ResetItems[I]),
                    FVector2D(405,340+(I-ResetScroll)*124),FVector2D(850,115),19);
            Element(TEXT("button"),TEXT("上一页"),FVector2D(405,600),FVector2D(170,45),18,TEXT("resetPrev"));Elements.Last().Enabled=ResetScroll>0;
            Element(TEXT("button"),TEXT("下一页"),FVector2D(595,600),FVector2D(170,45),18,TEXT("resetNext"));Elements.Last().Enabled=ResetScroll+2<ResetItems.Num();
            Element(TEXT("button"),TEXT("确认取消"),FVector2D(405,704),FVector2D(280,55),22,TEXT("confirm"));
            Element(TEXT("button"),TEXT("返回"),FVector2D(975,704),FVector2D(280,55),22,TEXT("cancel"));
        }
        else
        {
        Element(TEXT("panel"),TEXT(""),FVector2D(490,310),FVector2D(690,290));
        Element(TEXT("text"),TEXT("确认操作"),FVector2D(550,342),FVector2D(580,45),28);
        Element(TEXT("text"),(ConfirmAction==TEXT("gear.dropConfirmed")?TEXT("这是唯一装备，放下后仅能在原地拾回。确认放到地面？"):(ConfirmAction.StartsWith(TEXT("camp.")) || ConfirmAction==TEXT("settings.display"))?ConfirmMessage:TEXT("未保存的进度可能丢失。是否继续？")),FVector2D(550,410),FVector2D(580,50),20);
        Element(TEXT("button"),TEXT("确认"),FVector2D(550,510),FVector2D(240,55),22,TEXT("confirm"));
        Element(TEXT("button"),TEXT("返回"),FVector2D(850,510),FVector2D(240,55),22,TEXT("cancel"));
        }
    }
    ApplyLayout();
    ApplyReadableLayout();
    ApplyReadableHUD();
    if(Draft)
    {
        FEditableTextBoxStyle Style=Draft->GetWidgetStyle();
        Style.TextStyle.Font=FSlateFontInfo(Typeface,FMath::RoundToInt(18*GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f));
        Draft->SetWidgetStyle(Style);
    }
    KeyboardFocus=FocusedAction.IsEmpty()?INDEX_NONE:Elements.IndexOfByPredicate([&](const FHearthwardUIElement& E)
    { return E.Action==FocusedAction && E.Enabled && !E.Hidden; });
    if(KeyboardFocus==INDEX_NONE && ConfirmAction.IsEmpty() && PageFocus.Contains(Page))
        KeyboardFocus=Elements.IndexOfByPredicate([&](const auto& E){return E.Action==PageFocus[Page] && E.Enabled && !E.Hidden;});
}
void UHearthwardScreenWidget::NativeTick(const FGeometry& G,float Delta)
{
    Super::NativeTick(G,Delta);
    if(DisplayDeadline>0 && FPlatformTime::Seconds()>=DisplayDeadline) FinishDisplayChange(false);
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
        const auto& E=Elements[I]; if(E.Action.IsEmpty() || !E.Enabled || E.Hidden || E.TextScrollClipped) continue;
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
    if(CaptureBinding(E.GetEffectingButton(),E.IsShiftDown(),E.IsControlDown(),E.IsAltDown())) return FReply::Handled();
    if(Page==TEXT("map") && ConfirmAction.IsEmpty() && HearthwardInput::Matches(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings,TEXT("map.marker"),E.GetEffectingButton(),E.IsShiftDown(),E.IsControlDown(),E.IsAltDown()))
    {
        if(const auto* Bounds=LayoutBounds.Find(TEXT("map.canvas"));Bounds && Bounds->Hidden) return FReply::Handled();
        const FVector2D P=ComponentPoint(TEXT("map.canvas"),CanvasPoint(G,E.GetScreenSpacePosition()),true);
        if(P.X>=407 && P.X<=1517 && P.Y>=95 && P.Y<=855)
        {
            const FVector2D Local=(P-FVector2D(962,475)-MapPan)/MapZoom;
            const bool Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Active();
            const double Extent=Campaign?403200:6000;
            const FVector Origin=Campaign?FVector::ZeroVector:Gameplay()->LocationPosition(TEXT("camp"));
            Gameplay()->SetWaypoint(Origin+FVector(Local.X*Extent/1110,-Local.Y*Extent/760,0)); Refresh();
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
    if(ReadableLayout()) {TextScroll=FMath::Clamp(TextScroll-E.GetWheelDelta()*90,0.f,TextScrollMaximum);Refresh();return FReply::Handled();}
    if(Page==TEXT("map")) MapZoom=FMath::Clamp(MapZoom+E.GetWheelDelta()*.1f,1.f,2.f);
    else Scroll=FMath::Max(0,Scroll-(E.GetWheelDelta()>0?1:-1));
    Refresh(); return FReply::Handled();
}
bool UHearthwardScreenWidget::InputMatches(FName Id,const FKeyEvent& E) const
{ return HearthwardInput::Matches(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings,Id,E.GetKey(),E.IsShiftDown(),E.IsControlDown(),E.IsAltDown()); }
FReply UHearthwardScreenWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    if(LayoutKey(E)) return FReply::Handled();
    if(CaptureBinding(E.GetKey(),E.IsShiftDown(),E.IsControlDown(),E.IsAltDown())) return FReply::Handled();
    if(E.GetKey()==EKeys::Escape)
    { ExecuteAction(ConfirmAction.IsEmpty()?TEXT("back"):TEXT("cancel"));return FReply::Handled(); }
    if(!ConfirmAction.IsEmpty()) return NativeOnKeyDown(G,E);
    // Slate owns IME composition and emits OnTextCommitted after composition completes.
    if(Draft && Draft->HasKeyboardFocus()) return Super::NativeOnPreviewKeyDown(G,E);
    if(InputMatches(TEXT("ui.save"),E))
    { ExecuteAction(Page==TEXT("save")?TEXT("back"):TEXT("page:save"));return FReply::Handled(); }
    return Super::NativeOnPreviewKeyDown(G,E);
}
FReply UHearthwardScreenWidget::NativeOnKeyUp(const FGeometry& G,const FKeyEvent& E)
{
    if(!BindingCapture.IsNone() && E.GetKey().IsModifierKey())
    {
        const FKey Key=E.GetKey()==EKeys::RightShift?EKeys::LeftShift:E.GetKey()==EKeys::RightControl?EKeys::LeftControl:E.GetKey()==EKeys::RightAlt?EKeys::LeftAlt:E.GetKey();
        SettingsBindings.FindChecked(BindingCapture)[BindingSlot]={Key,FKey()};BindingCapture=NAME_None;SettingsDirty=true;Refresh();return FReply::Handled();
    }
    return Super::NativeOnKeyUp(G,E);
}
FReply UHearthwardScreenWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    const FKey Key=E.GetKey();
    if(ReadableLayout() && (Key==EKeys::PageUp || Key==EKeys::PageDown || Key==EKeys::Home || Key==EKeys::End))
    {TextScroll=Key==EKeys::Home?0:Key==EKeys::End?TextScrollMaximum:FMath::Clamp(TextScroll+(Key==EKeys::PageDown?600:-600),0.f,TextScrollMaximum);Refresh();return FReply::Handled();}
    if(E.IsRepeat()) return FReply::Handled();
    const bool Confirming=!ConfirmAction.IsEmpty();
    if(Key==EKeys::Enter && Elements.IsValidIndex(KeyboardFocus))
    {
        const auto& Focus=Elements[KeyboardFocus];
        if(Focus.Enabled && !Focus.Hidden && !Focus.Action.IsEmpty() && (!Confirming || Focus.Action==TEXT("confirm") || Focus.Action==TEXT("cancel") || Focus.Action.StartsWith(TEXT("reset"))))
            ExecuteAction(Focus.Action);
        return FReply::Handled();
    }
    if(Key==EKeys::Escape) {ExecuteAction(Confirming?TEXT("cancel"):TEXT("back"));return FReply::Handled();}
    if(Page==TEXT("map") && !Confirming && KeyboardFocus==INDEX_NONE && (Key==EKeys::Left || Key==EKeys::Right || Key==EKeys::Up || Key==EKeys::Down))
    {
        MapPan+=FVector2D(Key==EKeys::Left?30:Key==EKeys::Right?-30:0,Key==EKeys::Up?30:Key==EKeys::Down?-30:0);
        MapPan.X=FMath::Clamp(MapPan.X,-450.f,450.f);MapPan.Y=FMath::Clamp(MapPan.Y,-300.f,300.f);Refresh();return FReply::Handled();
    }
    if(Key==EKeys::Up || Key==EKeys::Down || Key==EKeys::Left || Key==EKeys::Right || Key==EKeys::Tab)
    {
        // Tab enters map button navigation; arrows then reach every actionable element.
        if(Key==EKeys::Tab && !Confirming && InputMatches(TEXT("ui.inventory"),E) && Page!=TEXT("map"))
        {OpenPage(Page==TEXT("inventory")?TEXT("hud"):TEXT("inventory"));return FReply::Handled();}
        Hover=INDEX_NONE;const int32 Direction=Key==EKeys::Up || Key==EKeys::Left || (Key==EKeys::Tab && E.IsShiftDown())?-1:1;
        int32 Candidate=KeyboardFocus==INDEX_NONE?(Direction>0?Elements.Num()-1:0):KeyboardFocus;KeyboardFocus=INDEX_NONE;
        for(int32 N=0;N<Elements.Num();++N)
        {
            Candidate=(Candidate+Direction+Elements.Num())%Elements.Num();const auto& Item=Elements[Candidate];
            if(!Item.Action.IsEmpty() && !Item.Hidden && Item.Enabled && (!Confirming || Item.Action==TEXT("confirm") || Item.Action==TEXT("cancel") || Item.Action==TEXT("resetPrev") || Item.Action==TEXT("resetNext")))
            {
                KeyboardFocus=Candidate;PageFocus.Add(Page,Item.Action);
                if(ReadableLayout())
                {
                    if(Item.Position.Y<70) TextScroll+=Item.Position.Y-70;
                    else if(Item.Position.Y+Item.Size.Y>790) TextScroll+=Item.Position.Y+Item.Size.Y-790;
                    TextScroll=FMath::Clamp(TextScroll,0.f,TextScrollMaximum);Refresh();
                }
                break;
            }
        }
        return FReply::Handled();
    }
    if(Confirming) return FReply::Handled();
    auto Is=[&](const TCHAR* Id){return InputMatches(FName(Id),E);};
    if(Page==TEXT("settings") && Is(TEXT("settings.defaults"))) {ExecuteAction(TEXT("settings.defaults"));return FReply::Handled();}
    if(Page==TEXT("storage") && Is(TEXT("storage.open"))) {ExecuteAction(TEXT("back"));return FReply::Handled();}
    struct FPageAction {const TCHAR* Page;const TCHAR* Id;const TCHAR* Action;};
    const FPageAction Shortcuts[]={
        {TEXT("inventory"),TEXT("inventory.use"),TEXT("use")},{TEXT("inventory"),TEXT("inventory.drop"),TEXT("drop")},{TEXT("inventory"),TEXT("inventory.repair"),TEXT("repair")},
        {TEXT("crafting"),TEXT("crafting.commit"),TEXT("craft")},{TEXT("repairing"),TEXT("repair.commit"),TEXT("repairEquipment")},{TEXT("skills"),TEXT("skills.learn"),TEXT("learn")},
        {TEXT("journal"),TEXT("journal.locate"),TEXT("questMap")},{TEXT("journal"),TEXT("journal.track"),TEXT("track")},{TEXT("storage"),TEXT("storage.transfer"),TEXT("transfer")}};
    for(const auto& S:Shortcuts) if(Page==S.Page && Is(S.Id))
    {ExecuteAction(S.Action);return FReply::Handled();}
    const bool Campaign=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetCampaignId().IsValid();
    if(!Campaign) return FReply::Handled();
    if(Is(TEXT("ui.inventory"))) {OpenPage(Page==TEXT("inventory")?TEXT("hud"):TEXT("inventory"));return FReply::Handled();}
    if(Is(TEXT("companion.dialogue"))) {ExecuteAction(Page==TEXT("dialogue")?TEXT("back"):TEXT("page:dialogue"));return FReply::Handled();}
    if(Is(TEXT("ui.pause"))) {ExecuteAction(Page==TEXT("pause")?TEXT("page:hud"):TEXT("page:pause"));return FReply::Handled();}
    if(Is(TEXT("ui.map"))) {OpenPage(TEXT("map"));return FReply::Handled();}
    if(Is(TEXT("ui.skills"))) {OpenPage(TEXT("skills"));return FReply::Handled();}
    if(Is(TEXT("ui.journal"))) {OpenPage(TEXT("journal"));return FReply::Handled();}
    return FReply::Handled();
}
void UHearthwardScreenWidget::DraftCommitted(const FText& TextValue,ETextCommit::Type Method)
{ if(Method==ETextCommit::OnEnter) ExecuteAction(Page==TEXT("memory")?TEXT("memorySave"):TEXT("send")); }

bool UHearthwardScreenWidget::ReadableLayout() const
{ return Page!=TEXT("hud") && Page!=TEXT("map") && !LayoutEditing && ConfirmAction.IsEmpty() && GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale>100; }
void UHearthwardScreenWidget::ApplyReadableLayout()
{
    if(!ReadableLayout()) return;
    if(Draft && (Page==TEXT("dialogue") || Page==TEXT("memory")))
    {auto* CanvasSlot=CastChecked<UCanvasPanelSlot>(Draft->Slot);CanvasSlot->SetPosition({190,795});CanvasSlot->SetSize({1292,90});}
    const auto Original=Elements;TArray<FHearthwardUIElement> Rows;
    const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    float Y=70;
    for(const auto& Source:Original)
    {
        if(Source.Hidden) continue;
        if(Source.Type==TEXT("image") && Source.Position.IsNearlyZero() && Source.Size.X>=1600) {Rows.Add(Source);continue;}
        if(Source.Text.IsEmpty() && Source.Action.IsEmpty()) continue;
        if(Source.Action.IsEmpty() && Source.Type!=TEXT("text") && Source.Type!=TEXT("notice")) continue;
        auto E=Source;E.Asset.Reset();E.MapClipped=false;E.FontRole.Reset();
        if(!E.Action.IsEmpty())
        {
            E.Type=TEXT("button");
            if(E.Text.IsEmpty())
                for(const auto& Label:Original)
                    if(!Label.Text.IsEmpty() && Label.Position.X>=Source.Position.X && Label.Position.Y>=Source.Position.Y
                        && Label.Position.X<Source.Position.X+Source.Size.X && Label.Position.Y<Source.Position.Y+Source.Size.Y)
                        if(Label.Text.Len()>E.Text.Len()) E.Text=Label.Text;
            if(E.Action.StartsWith(TEXT("skill:"))) E.Text=Text(Find(TEXT("skills"),E.Action.Mid(6)),TEXT("name"));
            else if(E.Action.StartsWith(TEXT("item:"))) E.Text=Text(Find(TEXT("items"),E.Action.Mid(5)),TEXT("name"))+(E.Text.IsEmpty()?FString():TEXT(" · ")+E.Text);
            if(E.Text.IsEmpty()) continue;
        }
        else
        {
            const bool LabelOfButton=Original.ContainsByPredicate([&](const auto& Button)
            {return !Button.Action.IsEmpty() && Source.Position.X>=Button.Position.X && Source.Position.Y>=Button.Position.Y && Source.Position.X<Button.Position.X+Button.Size.X && Source.Position.Y<Button.Position.Y+Button.Size.Y;});
            if(LabelOfButton) continue;
        }
        E.Font=FMath::Max(24.f*GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f,E.Font);E.Align=TEXT("left");E.TextInset=20;
        FSlateFontInfo Font(Typeface,FMath::RoundToInt(E.Font*.75f));
        TArray<FString> SourceLines;E.Text.ParseIntoArrayLines(SourceLines,false);int32 Lines=0;
        for(FString Line:SourceLines)
        {
            do {const int32 Count=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,1230)+1,1,FMath::Max(1,Line.Len()));Line=Line.Mid(Count);++Lines;} while(!Line.IsEmpty());
        }
        E.Position={190,Y};E.Size={1292,FMath::Max(68.f,Lines*E.Font*1.6f+22)};Y+=E.Size.Y+14;
        Rows.Add(E);
    }
    const float Bottom=Page==TEXT("dialogue") || Page==TEXT("memory")?770:835;
    TextScrollMaximum=FMath::Max(0.f,Y-Bottom);TextScroll=FMath::Clamp(TextScroll,0.f,TextScrollMaximum);
    for(auto& E:Rows) if(!(E.Type==TEXT("image") && E.Size.X>=1600))
    {E.Position.Y-=TextScroll;E.TextScrollClipped=E.Position.Y+E.Size.Y<60 || E.Position.Y>Bottom;}
    Elements=MoveTemp(Rows);
}

void UHearthwardScreenWidget::ApplyReadableHUD()
{
    if(Page!=TEXT("hud") || LayoutEditing || GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale<=100) return;
    const auto Original=Elements;Elements.Reset();
    auto* G=Gameplay();const auto& Bindings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings;
    auto Key=[&](const TCHAR* Id){return HearthwardInput::Label(Bindings,FName(Id));};
    auto Label=[&](FString Text,FVector2D P,FVector2D Size)
    {Element(TEXT("text"),Text,P,Size,24);Elements.Last().FontRole=TEXT("body");};
    const auto Quest=Find(TEXT("quests"),G->TrackedQuest.ToString());
    Label(Text(Quest,TEXT("name")),{48,42},{650,60});Elements.Last().Color=Color(TEXT("gold"));
    Label(Text(Quest,TEXT("objective")),{48,108},{650,115});
    Label(FString::Printf(TEXT("进度 %d / %.0f · %s 任务"),G->QuestProgress(G->TrackedQuest),Number(Quest,TEXT("required")),*Key(TEXT("ui.journal"))),{48,233},{650,55});
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>();
        Label(FString::Printf(TEXT("弟弟 · 生命 %.0f · 饱食 %.0f"),S->Health(),S->Hunger()),{48,310},{650,55});
        Label(S->Describe().IsEmpty()?GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->GetStatus():S->Describe(),{48,373},{650,110});
        Label(Key(TEXT("companion.wait"))+TEXT(" 等待 · ")+Key(TEXT("companion.follow"))+TEXT(" 跟随 · ")+Key(TEXT("companion.attack"))+TEXT(" 进攻"),{48,495},{650,55});
        break;
    }
    const float Values[]={G->Health,G->Hunger,G->Stamina},Maximum[]={G->MaxHealth(),100,G->MaxStamina()};
    const TCHAR* Names[]={TEXT("生命"),TEXT("饱食"),TEXT("耐力")},*Colors[]={TEXT("health"),TEXT("hunger"),TEXT("stamina")};
    for(int32 I=0;I<3;++I)
    {
        const float Y=610+I*56;Label(Names[I],{48,Y},{130,55});Elements.Last().Color=Color(Colors[I]);
        Element(TEXT("bar"),TEXT(""),{190,Y+20},{175,12});Elements.Last().Value=Values[I]/Maximum[I];Elements.Last().Color=Color(Colors[I]);
        Label(FString::Printf(TEXT("%.0f / %.0f"),Values[I],Maximum[I]),{388,Y},{265,55});
    }
    const auto& Slots=Theme->GetObjectField(TEXT("inventory"))->GetArrayField(TEXT("quickSlots"));
    const TCHAR* Shortcuts[]={TEXT("survival.medicine"),TEXT("survival.food"),TEXT(""),TEXT("combat.throw")};
    for(int32 I=0;I<Slots.Num();++I)
    {
        const FString Id=Slots[I]->AsString();const auto Item=Find(TEXT("items"),Id);const float X=48+I*155;
        Element(TEXT("image"),TEXT(""),{X,797},{52,52},18,TEXT(""),Text(Item,TEXT("quickIcon")).IsEmpty()?Text(Item,TEXT("icon")):Text(Item,TEXT("quickIcon")));
        FString Binding;if(I<UE_ARRAY_COUNT(Shortcuts) && Shortcuts[I][0])
        {const auto& Keys=Bindings.FindChecked(FName(Shortcuts[I]));Binding=(Keys[0].Key.IsValid()?Keys[0]:Keys[1]).Label()+TEXT(" · ");}
        Label(Binding+FString::FromInt(Inventory()->GetItemCount(FName(*Id))),{X,854},{150,55});
    }
    TSet<FString> Seen;float Y=110;
    const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    for(const auto& Source:Original)
    {
        if(Source.Hidden || Source.Text.IsEmpty() || Seen.Contains(Source.Text)) continue;
        const bool Notice=Source.Type==TEXT("notice");
        const bool WorldText=Source.Type==TEXT("text") && Source.Position.X>=900 && Source.Component.IsEmpty();
        if(!Notice && !WorldText) continue;
        if(Source.Text.StartsWith(Key(TEXT("combat.dodge")))) continue;
        Seen.Add(Source.Text);auto E=Source;E.Type=TEXT("text");E.FontRole=TEXT("body");E.Asset.Reset();E.TextInset=0;
        E.Position={1000,Y};E.Size={620,0};FSlateFontInfo Font(Typeface,FMath::RoundToInt(E.Font*.75f));
        TArray<FString> SourceLines;E.Text.ParseIntoArrayLines(SourceLines,false);int32 Count=0;
        for(FString Line:SourceLines) do {const int32 N=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,600)+1,1,FMath::Max(1,Line.Len()));Line=Line.Mid(N);++Count;} while(!Line.IsEmpty());
        E.Size.Y=FMath::Min(3,Count)*E.Font*1.6f;
        if(Y+E.Size.Y>570) continue;Y+=E.Size.Y+18;Elements.Add(E);
    }
    for(const auto& Source:Original) if(Source.Type==TEXT("bar") && Source.Text==TEXT("发现程度"))
    {auto E=Source;E.Text.Reset();E.Position={1000,588};E.Size={620,12};Elements.Add(E);}
    const int32 Time=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes;
    Label(Key(TEXT("ui.inventory"))+TEXT(" 背包 · ")+Key(TEXT("ui.map"))+TEXT(" 地图 · ")+Key(TEXT("ui.save"))+TEXT(" 存档"),{1000,886},{620,55});
    Label(FString::Printf(TEXT("第%d天 %02d:%02d"),Time/1440+1,(Time/60)%24,Time%60),{1080,42},{540,55});
}
