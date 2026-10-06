// PROTOTYPE_ONLY: explicit isolated PIE checks, absent from Shipping.
#if !UE_BUILD_SHIPPING
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "GameFramework/PlayerInput.h"
#include "EnhancedPlayerInput.h"
#include "EnhancedInputComponent.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

FString UHearthwardScreenWidget::DescribeHUDPreview() const
{
    auto Root=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Rows;
    for(const auto& E:Elements)
    {
        auto Row=MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("id"),E.LayoutId); Row->SetStringField(TEXT("type"),E.Type);
        Row->SetStringField(TEXT("text"),E.Text); Row->SetStringField(TEXT("asset"),E.Asset); Row->SetStringField(TEXT("action"),E.Action);
        Row->SetStringField(TEXT("inventory_item"),E.InventoryItem.ToString());Row->SetStringField(TEXT("instance"),E.InventoryInstance.ToString());
        Row->SetStringField(TEXT("equipment_slot"),E.EquipmentSlot.ToString());Row->SetStringField(TEXT("shortcut"),E.Shortcut);
        Row->SetNumberField(TEXT("inventory_position"),E.InventoryPosition);Row->SetNumberField(TEXT("quick_slot"),E.QuickSlot);
        Row->SetBoolField(TEXT("selected"),E.Selected); Row->SetBoolField(TEXT("visible"),!E.Hidden && E.Opacity()>0);
        Row->SetBoolField(TEXT("enabled"),E.Enabled); Row->SetBoolField(TEXT("text_scroll_clipped"),E.TextScrollClipped);
        Row->SetNumberField(TEXT("opacity"),E.Opacity());
        Row->SetNumberField(TEXT("value"),E.Value); Row->SetNumberField(TEXT("font"),E.Font);
        Row->SetNumberField(TEXT("x"),E.Position.X); Row->SetNumberField(TEXT("y"),E.Position.Y);
        Row->SetNumberField(TEXT("width"),E.Size.X); Row->SetNumberField(TEXT("height"),E.Size.Y);
        Row->SetNumberField(TEXT("brightness"),E.Color.GetLuminance());
        Row->SetNumberField(TEXT("red"),E.Color.R); Row->SetNumberField(TEXT("green"),E.Color.G); Row->SetNumberField(TEXT("blue"),E.Color.B);
        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    Root->SetArrayField(TEXT("elements"),Rows); Root->SetNumberField(TEXT("selection"),HUDQuickSelection);
    Root->SetNumberField(TEXT("quest_remaining"),GetHUDQuestNoticeRemaining());
    Root->SetBoolField(TEXT("inventory_dragging"),InventoryDragging);
    if(Page==TEXT("map"))
    {
        auto Map=MakeShared<FJsonObject>();const auto View=MapViewRect();const auto Size=MapWorldSize();
        auto Values=[](std::initializer_list<double> Items){TArray<TSharedPtr<FJsonValue>> Result;for(double V:Items)Result.Add(MakeShared<FJsonValueNumber>(V));return Result;};
        Map->SetArrayField(TEXT("viewport"),Values({View.Left,View.Top,View.Right,View.Bottom}));
        Map->SetArrayField(TEXT("regionCenterCm"),Values({MapCenter.X,MapCenter.Y}));
        Map->SetArrayField(TEXT("sizeCm"),Values({Size.X,Size.Y}));
        Map->SetArrayField(TEXT("pan"),Values({MapPan.X,MapPan.Y}));
        Map->SetArrayField(TEXT("designViewSize"),Values({MapViewDesignSize.X,MapViewDesignSize.Y}));
        Map->SetNumberField(TEXT("scaleDesignUnitsPerCm"),MapScale()*MapZoom);Map->SetNumberField(TEXT("zoom"),MapZoom);
        Map->SetBoolField(TEXT("dragging"),MapDragging);Root->SetObjectField(TEXT("mapView"),Map);
    }
    FString Json; FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json)); return Json;
}

namespace
{
void VerifyHUDPreview(UWorld* World)
{
    const FString Run=FPlatformMisc::GetEnvironmentVariable(TEXT("HEARTHWARD_TITLE_RUN"));
    FString Pool; FGuid PoolId;
    if(!World || World->WorldType!=EWorldType::PIE || !Run.StartsWith(TEXT("verify_"))
        || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,PoolId) || !PoolId.IsValid()) return;
    auto Report=MakeShared<FJsonObject>(),Checks=MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Captures,Layouts; bool Passed=true;
    auto Check=[&](const FString& Name,bool Value){Checks->SetBoolField(Name,Value);Passed &= Value;};
    auto* Controller=World->GetFirstPlayerController(); auto* HUD=Controller?Cast<AHearthwardHUD>(Controller->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr; Check(TEXT("screen_available"),UI!=nullptr);
    if(UI)
    {
        auto* Pawn=Controller->GetPawn().Get(); auto* G=Pawn->FindComponentByClass<UHearthwardGameplayComponent>();
        auto* Inventory=Pawn->FindComponentByClass<UHearthwardInventoryComponent>();
        auto* Settings=World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
        const TGuardValue<FHearthwardComfortSettings> RestoreComfort(Settings->Comfort,Settings->Comfort);
        const auto InventoryBefore=Inventory->Snapshot(); const float HealthBefore=G->Health;
        const auto ClaimedBefore=G->Claimed; const FName TrackedBefore=G->TrackedQuest;
        const int32 SavesBefore=World->GetSubsystem<UHearthwardSaveSubsystem>()->GetPoints().Num();
        auto Snapshot=[&]()
        {
            TSharedPtr<FJsonObject> Object; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),Object);
            return Object;
        };
        auto Find=[&](const TSharedPtr<FJsonObject>& Object,const FString& Id)->TSharedPtr<FJsonObject>
        {
            for(const auto& Row:Object->GetArrayField(TEXT("elements"))) if(Row->AsObject()->GetStringField(TEXT("id"))==Id) return Row->AsObject();
            return nullptr;
        };
        auto Layout=[&](const FString& Name)
        {
            const auto Object=Snapshot(); Layouts.Add(MakeShared<FJsonValueObject>(Object));
            const auto Quest=Find(Object,TEXT("hud.quest.heading")),Objective=Find(Object,TEXT("hud.quest.objective")),Portrait=Find(Object,TEXT("hud.companion.portrait")),Order=Find(Object,TEXT("hud.companion.order"));
            bool Clean=true,Vitals=true; int32 QuickCount=0,SelectedCount=0; double SelectedY=0,OtherMaxY=0;
            for(const auto& V:Object->GetArrayField(TEXT("elements")))
            {
                const auto E=V->AsObject(); const FString Type=E->GetStringField(TEXT("type")),Text=E->GetStringField(TEXT("text"));
                Clean &= Type!=TEXT("notice") && Type!=TEXT("panel") && !Text.Contains(TEXT("Tab")) && !Text.Contains(TEXT("Esc"))
                    && !Text.Contains(TEXT("按 J")) && !Text.Contains(TEXT("Z 等待")) && !Text.Contains(TEXT("F / R")) && !Text.Contains(TEXT("下达委托"));
                if(Type==TEXT("hudQuickItem"))
                {
                    ++QuickCount; const bool Selected=E->GetBoolField(TEXT("selected"));
                    if(Selected){++SelectedCount;SelectedY=E->GetNumberField(TEXT("y"));}
                    else {OtherMaxY=FMath::Max(OtherMaxY,E->GetNumberField(TEXT("y")));Clean &= E->GetNumberField(TEXT("brightness"))>=.5 && E->GetNumberField(TEXT("brightness"))<.7;}
                    Clean &= !E->GetStringField(TEXT("asset")).IsEmpty();
                }
                Clean &= E->GetNumberField(TEXT("y"))+E->GetNumberField(TEXT("height"))<=942;
            }
            for(int32 I=0;I<3;++I)
            {
                const auto Bar=Find(Object,FString::Printf(TEXT("hud.vitals.%d.bar"),I));
                Vitals &= Bar && Portrait && Bar->GetNumberField(TEXT("y"))+Bar->GetNumberField(TEXT("height"))<Portrait->GetNumberField(TEXT("y"));
                const auto Label=Find(Object,FString::Printf(TEXT("hud.vitals.%d.label"),I));
                Vitals &= Bar && Label && Bar->GetStringField(TEXT("type"))==TEXT("hudVitalBar") && Bar->GetNumberField(TEXT("height"))>=18
                    && Label->GetNumberField(TEXT("x"))<Bar->GetNumberField(TEXT("x"));
            }
            Check(Name+TEXT("_clean_without_tutorial_panels"),Clean);
            Check(Name+TEXT("_heavy_vitals_before_companion"),Vitals);
            Check(Name+TEXT("_transient_quest_below_companion"),Portrait && Order && ((!Quest && !Objective)
                || (Quest && Objective && Quest->GetNumberField(TEXT("y"))>=Order->GetNumberField(TEXT("y"))+Order->GetNumberField(TEXT("height"))
                    && Objective->GetNumberField(TEXT("y"))+Objective->GetNumberField(TEXT("height"))<=633)));
            Check(Name+TEXT("_four_items_one_selected_below"),QuickCount==4 && SelectedCount==1 && SelectedY>OtherMaxY);
        };
        auto Capture=[&](const FString& Name,int32 Width=1672,int32 Height=941)
        {
            const FString File=Run+TEXT("-")+Name; const bool Done=UI->CaptureUI(File,Width,Height);
            Check(TEXT("capture_")+Name,Done); if(Done) Captures.Add(MakeShared<FJsonValueString>(File+TEXT(".png")));
        };
        const FGeometry Geometry=FGeometry::MakeRoot(FVector2D(1672,941),FSlateLayoutTransform());
        const FModifierKeysState Modifiers;
        auto Wheel=[&](float Delta){UI->NativeOnMouseWheel(Geometry,FPointerEvent(0,FVector2D(500,500),FVector2D(500,500),TSet<FKey>(),FKey(),Delta,Modifiers));};
        auto PhysicalKey=[&](FKey Key)
        {
            const FInputDeviceId Device=FInputDeviceId::CreateFromInternalId(0);
            TArray<TSharedPtr<FJsonValue>> Mappings;
            if(const auto* Input=Cast<UEnhancedPlayerInput>(Controller->PlayerInput))
                for(const auto& Mapping:Input->GetEnhancedActionMappingsView()) if(Mapping.Key==Key)
                {auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("action"),GetNameSafe(Mapping.Action));Row->SetBoolField(TEXT("ignored"),Mapping.bShouldBeIgnored);Mappings.Add(MakeShared<FJsonValueObject>(Row));}
            Report->SetArrayField(Key.ToString()+TEXT("_mappings"),Mappings);
            Report->SetStringField(TEXT("player_input_type"),Controller->PlayerInput->GetClass()->GetName());
            if(const auto* Input=Cast<UEnhancedInputComponent>(Pawn->InputComponent)) Report->SetNumberField(TEXT("bound_action_events"),Input->GetActionEventBindings().Num());
            Controller->InputKey(FInputKeyEventArgs(nullptr,Device,Key,IE_Released,0,false,FPlatformTime::Cycles64())); Controller->PlayerTick(.02f);
            const bool Handled=Controller->InputKey(FInputKeyEventArgs(nullptr,Device,Key,IE_Pressed,1,false,FPlatformTime::Cycles64())); Controller->PlayerTick(.02f);
            Report->SetBoolField(Key.ToString()+TEXT("_handled"),Handled);
            Report->SetNumberField(Key.ToString()+TEXT("_selection"),UI->GetHUDQuickSelection());
            Report->SetNumberField(Key.ToString()+TEXT("_value"),Controller->PlayerInput->GetKeyValue(Key));
            Controller->InputKey(FInputKeyEventArgs(nullptr,Device,Key,IE_Released,0,false,FPlatformTime::Cycles64())); Controller->PlayerTick(.02f);
        };
        Settings->Comfort.TextScale=100; UI->OpenPage(TEXT("hud")); UI->ExecuteAction(TEXT("hud.quick.select:0"));
        // Receive a newly unlocked quest in this explicit isolated fixture, rather than replaying a tracked quest.
        G->Claimed.Add(TEXT("ember")); G->TrackedQuest=TEXT("store"); UI->Refresh();
        Check(TEXT("new_quest_receipt_shows_five_second_notice"),UI->GetHUDQuestNoticeRemaining()>4 && UI->GetHUDQuestNoticeRemaining()<=5
            && Find(Snapshot(),TEXT("hud.quest.heading")) && Find(Snapshot(),TEXT("hud.quest.heading"))->GetStringField(TEXT("text")).Contains(TEXT("余烬之路")));
        const float DeadlineBefore=UI->GetHUDQuestNoticeRemaining(); for(int32 I=0;I<30;++I) UI->Refresh();
        Check(TEXT("periodic_refresh_does_not_restart_quest_notice"),UI->GetHUDQuestNoticeRemaining()<=DeadlineBefore);
        const auto BarSnapshot=Snapshot();
        bool OriginalColors=true;
        const TCHAR* ColorNames[]={TEXT("health"),TEXT("hunger"),TEXT("stamina")};
        FString ThemeText; TSharedPtr<FJsonObject> ThemeJson;
        FFileHelper::LoadFileToString(ThemeText,*(FPaths::ProjectDir()/TEXT("Resources/UI/interface.json")));
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ThemeText),ThemeJson);
        for(int32 I=0;I<3;++I)
        {
            const auto Bar=Find(BarSnapshot,FString::Printf(TEXT("hud.vitals.%d.bar"),I));
            const FLinearColor Expected(FColor::FromHex(ThemeJson->GetObjectField(TEXT("colors"))->GetStringField(ColorNames[I])));
            OriginalColors &= Bar && FMath::IsNearlyEqual(Bar->GetNumberField(TEXT("red")),double(Expected.R),.001)
                && FMath::IsNearlyEqual(Bar->GetNumberField(TEXT("green")),double(Expected.G),.001) && FMath::IsNearlyEqual(Bar->GetNumberField(TEXT("blue")),double(Expected.B),.001);
        }
        Check(TEXT("heavy_vital_bars_keep_original_three_colors"),OriginalColors);
        const FString InventoryUnchanged=Inventory->DescribeInventory();
        const FKey SlotKeys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};
        for(int32 I=0;I<4;++I)
        {
            if(I!=0)PhysicalKey(SlotKeys[I]);
            Check(FString::Printf(TEXT("shortcut_selection_%d"),I),UI->GetHUDQuickSelection()==I);
            Layout(FString::Printf(TEXT("item_%d"),I));Capture(FString::Printf(TEXT("hud-item-%d"),I),2560,1600);Wheel(-1);
            Check(FString::Printf(TEXT("wheel_does_not_change_slot_%d"),I),UI->GetHUDQuickSelection()==I);
        }
        Wheel(1);Check(TEXT("reverse_wheel_keeps_selection"),UI->GetHUDQuickSelection()==3);
        Check(TEXT("wheel_does_not_consume_inventory"),Inventory->DescribeInventory()==InventoryUnchanged);
        UI->ExecuteAction(TEXT("hud.quick.select:0")); PhysicalKey(EKeys::MouseScrollDown);
        Check(TEXT("game_only_enhanced_input_wheel_down_ignored"),UI->GetHUDQuickSelection()==0); PhysicalKey(EKeys::MouseScrollUp);
        Check(TEXT("game_only_enhanced_input_wheel_up"),UI->GetHUDQuickSelection()==0);
        UI->ExecuteAction(TEXT("hud.quick.select:3")); UI->ExecuteAction(TEXT("quick:2"));
        Check(TEXT("existing_quick_action_updates_selection"),UI->GetHUDQuickSelection()==2);
        Inventory->TryAdd(TEXT("arrow"),1); // Inventory selection requires an owned stack.
        UI->ExecuteAction(TEXT("hud.quick.select:3")); UI->OpenPage(TEXT("inventory"));
        UI->ExecuteAction(TEXT("item:arrow")); UI->ExecuteAction(TEXT("use"));
        Check(TEXT("inventory_use_keeps_current_selection"),UI->GetHUDQuickSelection()==3);
        UI->OpenPage(TEXT("hud"));
        const int32 Count=Inventory->GetItemCount(TEXT("medicine")); Inventory->TryAdd(TEXT("medicine"),1); UI->Refresh();
        Check(TEXT("live_inventory_quantity_updates"),Find(Snapshot(),TEXT("hud.quick.medicine"))->GetStringField(TEXT("text"))==FString::FromInt(Count+1));
        G->Health=FMath::Max(1.f,HealthBefore-7); UI->Refresh();
        Check(TEXT("live_health_bar_updates"),FMath::IsNearlyEqual(Find(Snapshot(),TEXT("hud.vitals.0.bar"))->GetNumberField(TEXT("value")),double(G->Health/G->MaxHealth()),.001));
        G->Health=HealthBefore; Inventory->RestoreInventory(InventoryBefore);
        UI->ExecuteAction(TEXT("hud.quick.select:0"));
        Settings->Comfort.TextScale=150; G->Claimed.Add(TEXT("store")); G->TrackedQuest=TEXT("prepare"); UI->Refresh();
        Check(TEXT("large_text_new_quest_popup_visible"),UI->GetHUDQuestNoticeRemaining()>4 && Find(Snapshot(),TEXT("hud.quest.heading")));
        Layout(TEXT("text_150")); Capture(TEXT("hud-text-150"),2560,1600);
        Settings->Comfort.TextScale=100; UI->Refresh(); Capture(TEXT("hud-720p"),1280,720); Capture(TEXT("hud-ultrawide"),2520,1080);
        UI->OpenPage(TEXT("settings"));
        Check(TEXT("choice_actions_blocked_on_settings"),!UI->ExecuteAction(TEXT("hud.quick.next")) && UI->GetHUDQuickSelection()==0);
        UI->OpenPage(TEXT("hud")); UI->ExecuteAction(TEXT("ask:quit")); Wheel(-1);
        Check(TEXT("wheel_blocked_by_confirmation"),UI->GetHUDQuickSelection()==0 && !UI->ExecuteAction(TEXT("hud.quick.next")));
        UI->ExecuteAction(TEXT("cancel"));
        Check(TEXT("invalid_selection_rejected"),!UI->ExecuteAction(TEXT("hud.quick.select:99")) && UI->GetHUDQuickSelection()==0);
        Check(TEXT("save_nodes_unchanged"),World->GetSubsystem<UHearthwardSaveSubsystem>()->GetPoints().Num()==SavesBefore);
        Check(TEXT("inventory_fixture_restored"),Inventory->DescribeInventory()==InventoryUnchanged);
        G->Claimed=ClaimedBefore; G->TrackedQuest=TrackedBefore;
        World->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.Broadcast(); UI->OpenPage(TEXT("hud"));
        Check(TEXT("restored_existing_quests_do_not_replay_notice"),UI->GetHUDQuestNoticeRemaining()==0 && !Find(Snapshot(),TEXT("hud.quest.heading")));
        UI->OpenPage(TEXT("title"));
    }
    Report->SetBoolField(TEXT("passed"),Passed); Report->SetObjectField(TEXT("checks"),Checks);
    Report->SetArrayField(TEXT("captures"),Captures); Report->SetArrayField(TEXT("layouts"),Layouts);
    Report->SetStringField(TEXT("input"),TEXT("Native Slate wheel events plus PlayerController InputKey through Enhanced Input; no physical Windows input."));
    FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("TitleWheel")/Run/TEXT("hud-preview.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
FAutoConsoleCommandWithWorld VerifyHUDPreviewCommand(TEXT("Hearthward.UI.VerifyHUDPreview"),TEXT("Verify the isolated in-game HUD prototype."),FConsoleCommandWithWorldDelegate::CreateStatic(&VerifyHUDPreview));

void VerifyHUDHintRange(UWorld* World)
{
    const FString Run=FPlatformMisc::GetEnvironmentVariable(TEXT("HEARTHWARD_HUD_RUN"));
    FString Pool; FGuid PoolId;
    if(!World || World->WorldType!=EWorldType::PIE || !Run.StartsWith(TEXT("hud_"))
        || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,PoolId) || !PoolId.IsValid()) return;
    auto Report=MakeShared<FJsonObject>(),Checks=MakeShared<FJsonObject>(); bool Passed=true;
    auto Check=[&](const FString& Name,bool Value){Checks->SetBoolField(Name,Value);Passed &= Value;};
    auto* Controller=World->GetFirstPlayerController(); auto* HUD=Controller?Cast<AHearthwardHUD>(Controller->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr; Check(TEXT("screen_available"),UI!=nullptr);
    if(UI)
    {
        auto* Pawn=Controller->GetPawn().Get(); auto* G=Pawn->FindComponentByClass<UHearthwardGameplayComponent>();
        const FTransform Pose=Pawn->GetActorTransform(); const auto Discovered=G->Discovered; const FString Feedback=G->Feedback;
        const FString GameplayBefore=G->SaveSnapshot();
        const FName Subject=TEXT("prologue_relic");
        const auto Row=HearthwardData::Find(TEXT("locations"),Subject.ToString());
        const FVector Target=G->LocationPosition(Subject); const FString Label=TEXT("发现：")+HearthwardData::Text(Row,TEXT("name"));
        Report->SetStringField(TEXT("subject_id"),Subject.ToString()); Report->SetStringField(TEXT("subject_position"),Target.ToString());
        Report->SetStringField(TEXT("hint"),Label);
        G->Discovered.Add(Subject); G->Feedback=Label;
        auto HasHint=[&]()
        {
            UI->Refresh(); TSharedPtr<FJsonObject> Object; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),Object);
            for(const auto& V:Object->GetArrayField(TEXT("elements")))
                if(V->AsObject()->GetStringField(TEXT("id"))==TEXT("hud.gameplay.feedback")) return V->AsObject()->GetStringField(TEXT("text"))==G->Feedback;
            return false;
        };
        auto Move=[&](FVector Offset){Pawn->SetActorLocation(Target+Offset,false,nullptr,ETeleportType::TeleportPhysics);};
        Move({990,0,0}); Check(TEXT("hint_visible_at_9_9_metres"),HasHint());
        Move({1000,0,0}); Check(TEXT("hint_visible_at_10_metres"),HasHint());
        Move({1001,0,0}); Check(TEXT("hint_hidden_beyond_10_metres"),!HasHint());
        Move({0,0,990}); Check(TEXT("hint_visible_within_vertical_10_metres"),HasHint());
        Move({0,0,1001}); Check(TEXT("hint_hidden_beyond_vertical_10_metres"),!HasHint());
        Move({3000,0,0}); Check(TEXT("stale_discovery_hint_hidden_far_away"),!HasHint());
        Move({900,0,0}); Check(TEXT("discovery_hint_returns_when_approaching_again"),HasHint());
        G->Feedback=TEXT("发现：未知的测试目标"); Check(TEXT("unknown_hint_subject_hidden"),!HasHint());
        G->Feedback=TEXT("本次动作已完成"); Check(TEXT("action_result_retained_outside_discovery_filter"),HasHint());
        Pawn->SetActorTransform(Pose,false,nullptr,ETeleportType::TeleportPhysics); G->Discovered=Discovered; G->Feedback=Feedback; UI->Refresh();
        Check(TEXT("gameplay_discovery_and_player_pose_restored"),G->SaveSnapshot()==GameplayBefore && Pawn->GetActorTransform().Equals(Pose));
    }
    Report->SetBoolField(TEXT("passed"),Passed);Report->SetObjectField(TEXT("checks"),Checks);
    FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("HUDPreview")/Run/TEXT("hint-range.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
FAutoConsoleCommandWithWorld VerifyHUDHintRangeCommand(TEXT("Hearthward.UI.VerifyHUDHintRange"),TEXT("Verify the ten-metre discovery hint boundary in an isolated PIE world."),FConsoleCommandWithWorldDelegate::CreateStatic(&VerifyHUDHintRange));

void CaptureHUDScene(const TArray<FString>& Args,UWorld* World)
{
    const FString Run=FPlatformMisc::GetEnvironmentVariable(TEXT("HEARTHWARD_HUD_RUN"));
    FString Pool; FGuid PoolId;
    if(!World || World->WorldType!=EWorldType::PIE || !Run.StartsWith(TEXT("hud_")) || Args.Num()!=1
        || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,PoolId) || !PoolId.IsValid()) return;
    const FString Path=FPaths::ProjectSavedDir()/TEXT("HUDPreview")/Run/(FPaths::MakeValidFileName(Args[0])+TEXT(".png"));
    // Capture this PIE viewport directly; a global screenshot request can be consumed by an editor viewport first.
    UGameViewportClient* Viewport=World->GetGameInstance()?World->GetGameInstance()->GetGameViewportClient():nullptr;
    if(!Viewport || !FSlateApplication::IsInitialized()) return;
    const TSharedPtr<SViewport> Widget=Viewport->GetGameViewportWidget();
    TArray<FColor> Pixels; FIntVector Size=FIntVector::ZeroValue;
    if(!Widget.IsValid() || !FSlateApplication::Get().TakeScreenshot(Widget.ToSharedRef(),Pixels,Size)) return;
    TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
    FFileHelper::SaveArrayToFile(PNG,*Path);
}
FAutoConsoleCommandWithWorldAndArgs CaptureHUDSceneCommand(TEXT("Hearthward.UI.CaptureHUDScene"),TEXT("Capture only the isolated PIE game viewport with its HUD."),FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CaptureHUDScene));
}
#endif
