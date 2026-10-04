// PROTOTYPE_ONLY: opt-in standalone Slate pointer fixture in a GUID-isolated save pool.
#if !UE_BUILD_SHIPPING
#include "../Companion/HearthwardCompanionFixture.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
namespace
{
struct FBackpackDragVerification
{
    FString Output;
    TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>(),States=MakeShared<FJsonObject>();
    bool Passed=true;
    int32 Step=0,ReturnCase=0;
    FString ReturnInventory;
    double Started=FPlatformTime::Seconds(),WaitUntil=0;
    FGuid FirstAxe,SecondAxe,SaveId,ManagedAxe,TransferredAxe;
    FHearthwardInventorySnapshot InventoryBefore;
    TMap<FName,int32> Counts;
    int32 WoodPosition=0;
    void Check(const FString& Name,bool Value)
    {bool Previous=true;Checks->TryGetBoolField(Name,Previous);Checks->SetBoolField(Name,Previous && Value);Passed&=Value;UE_LOG(LogTemp,Log,TEXT("Backpack drag %s: %s"),*Name,Value?TEXT("PASS"):TEXT("FAIL"));}
    bool Finish()
    {
        auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("passed"),Passed);Report->SetObjectField(TEXT("checks"),Checks);Report->SetObjectField(TEXT("states"),States);
        Report->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        Report->SetStringField(TEXT("scope"),TEXT("Actual standalone Slate pointer routing, immediate equipment/stats, vacated equipped cells and spare quantities, all four tabs, sparse placement, real save/load, reservations, management layouts and real instance transfer/equip. Controlled inventories in an isolated GUID pool; no Windows physical mouse automation."));
        FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);return false;
    }
};
bool VerifyBackpackDrag(float)
{
    static FBackpackDragVerification Run;
    if(Run.Output.IsEmpty())
    {
        FString Pool;FGuid Id;
        if(!FParse::Value(FCommandLine::Get(),TEXT("HearthwardDragVerify="),Run.Output)
            || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id) || !Id.IsValid())return false;
    }
    const double Now=FPlatformTime::Seconds();
    if(Now-Run.Started>120){Run.Check(TEXT("completion_within_timeout"),false);return Run.Finish();}
    if(Now<Run.WaitUntil || !GEngine)return true;
    UWorld* World=nullptr;
    for(const auto& Context:GEngine->GetWorldContexts())if(Context.WorldType==EWorldType::Game && Context.World()){World=Context.World();break;}
    auto* PC=World?World->GetFirstPlayerController():nullptr;auto* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;auto* UI=HUD?HUD->Screen.Get():nullptr;
    if(!UI || !World->HasBegunPlay() || World->GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>()->IsLoading()
        || (Run.Step==0 && UI->GetPage()!=TEXT("hud")))return true;
    auto* G=Pawn->FindComponentByClass<UHearthwardGameplayComponent>();auto* Bag=Pawn->FindComponentByClass<UHearthwardInventoryComponent>();
    auto* S=Pawn->FindComponentByClass<UHearthwardSurvivalComponent>();auto* C=Pawn->FindComponentByClass<UHearthwardCombatComponent>();
    auto* Store=World->GetSubsystem<UHearthwardStorageSubsystem>();auto* Saves=World->GetSubsystem<UHearthwardSaveSubsystem>();
    auto* Settings=World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
    auto Snapshot=[&]() {TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),J);return J;};
    auto Find=[&](const FString& Id)->TSharedPtr<FJsonObject>
    {
        const auto J=Snapshot();for(const auto& V:J->GetArrayField(TEXT("elements")))
            if(V->AsObject()->GetStringField(TEXT("id"))==Id && V->AsObject()->GetBoolField(TEXT("visible")))return V->AsObject();
        return nullptr;
    };
    auto Position=[&](const FString& Id)
    {
        const auto E=Find(Id);Run.Check(TEXT("target_available_")+Id,E.IsValid());
        return E?FVector2D(E->GetNumberField(TEXT("x"))+E->GetNumberField(TEXT("width"))*.5,E->GetNumberField(TEXT("y"))+E->GetNumberField(TEXT("height"))*.5):FVector2D::ZeroVector;
    };
    auto Absolute=[&](FVector2D Point)
    {
        const auto Geometry=UI->GetCachedGeometry();const FVector2D Size=Geometry.GetLocalSize();
        const float Scale=FMath::Min(Size.X/1672,Size.Y/941);
        return Geometry.LocalToAbsolute((Size-FVector2D(1672,941)*Scale)*.5+Point*Scale);
    };
    auto Down=[&](FVector2D Point)
    {
        const FVector2D P=Absolute(Point);const FPointerEvent Event(0,P,P,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
        const auto Window=FSlateApplication::Get().FindWidgetWindow(UI->TakeWidget());
        return FSlateApplication::Get().ProcessMouseButtonDownEvent(Window.IsValid()?Window->GetNativeWindow():nullptr,Event);
    };
    auto Move=[&](FVector2D From,FVector2D To)
    {
        const FPointerEvent Event(0,Absolute(To),Absolute(From),TSet<FKey>{EKeys::LeftMouseButton},FKey(),0,FModifierKeysState());
        return FSlateApplication::Get().ProcessMouseMoveEvent(Event,false);
    };
    auto Up=[&](FVector2D Point)
    {
        const FVector2D P=Absolute(Point);const FPointerEvent Event(0,P,P,TSet<FKey>(),EKeys::LeftMouseButton,0,FModifierKeysState());
        return FSlateApplication::Get().ProcessMouseButtonUpEvent(Event);
    };
    auto Drag=[&](const FString& From,FVector2D To)
    {
        const FVector2D Start=Position(From);Down(Start);Move(Start,To);
        Run.Check(TEXT("slate_routes_drag_")+From,Snapshot()->GetBoolField(TEXT("inventory_dragging")));
        Up(To);Run.Check(TEXT("slate_releases_capture_")+From,!UI->TakeWidget()->HasMouseCapture() && !Snapshot()->GetBoolField(TEXT("inventory_dragging")));
    };
    auto Trace=[&](const TCHAR* Name)
    {
        auto J=Snapshot();J->SetStringField(TEXT("page"),UI->GetPage().ToString());J->SetStringField(TEXT("category"),UI->GetCategory());J->SetStringField(TEXT("message"),UI->GetMessage());J->SetStringField(TEXT("gameplay"),G->SaveSnapshot());Run.States->SetObjectField(Name,J);
    };
    auto Slots=[&](FName Tab){return G->InventorySlots(Tab);};
    auto SlotAt=[&](FName Tab,int32 Index){const auto Values=Slots(Tab);return Values.IsValidIndex(Index)?Values[Index]:NAME_None;};
    auto CellCount=[&](const FString& Prefix)
    {
        const auto J=Snapshot();int32 Count=0;for(const auto& V:J->GetArrayField(TEXT("elements")))
            if(V->AsObject()->GetStringField(TEXT("type"))==TEXT("inventorySlot") && V->AsObject()->GetStringField(TEXT("id")).StartsWith(Prefix))++Count;
        return Count;
    };
    auto Capture=[&](const TCHAR* Name){UI->Refresh();FScreenshotRequest::RequestScreenshot(FPaths::GetPath(Run.Output)/(FString(Name)+TEXT(".png")),true,false);};
    auto Click=[&](const FString& Id){const auto P=Position(Id);Down(P);Up(P);};
    auto Escape=[&]()
    {
        const FKeyEvent Event(EKeys::Escape,FModifierKeysState(),0,false,0,0);
        FSlateApplication::Get().ProcessKeyDownEvent(Event);FSlateApplication::Get().ProcessKeyUpEvent(Event);
    };
    auto ManagementLayout=[&](const TCHAR* Name)
    {
        const auto J=Snapshot();int32 Surfaces=0,Tabs=0,Rows=0;bool Bounds=true,Clean=true;
        for(const auto& V:J->GetArrayField(TEXT("elements")))
        {
            const auto E=V->AsObject();const FString Id=E->GetStringField(TEXT("id")),Type=E->GetStringField(TEXT("type")),Asset=E->GetStringField(TEXT("asset"));
            Surfaces+=Type==TEXT("inventorySurface");Tabs+=Type==TEXT("menuTab");Rows+=Type==TEXT("menuRow");
            Clean&=Id!=TEXT("background") && Asset!=TEXT("leatherPanel") && Asset!=TEXT("pauseBackground") && Type!=TEXT("notice");
            Bounds&=E->GetNumberField(TEXT("x"))>=0 && E->GetNumberField(TEXT("y"))>=0
                && E->GetNumberField(TEXT("x"))+E->GetNumberField(TEXT("width"))<=1672
                && E->GetNumberField(TEXT("y"))+E->GetNumberField(TEXT("height"))<=941;
        }
        Run.Check(FString(Name)+TEXT("_three_charcoal_panes_no_legacy_art"),UI->GetPage()==TEXT("equipment") && Surfaces==3 && Tabs==3 && Clean);
        Run.Check(FString(Name)+TEXT("_inside_viewport_and_paged_rows"),Bounds && Rows<=(Settings->Comfort.TextScale>125?6:8));
        Trace(Name);
    };
    auto ExportSize=[&](const TCHAR* Name,int32 Width,int32 Height)
    {
        const FString CaptureName=FPaths::GetBaseFilename(FPaths::GetPath(Run.Output))+TEXT("-")+Name;
        const bool Rendered=UI->CaptureUI(CaptureName,Width,Height);
        const FString Source=FPaths::ProjectSavedDir()/TEXT("Task020")/(FPaths::MakeValidFileName(CaptureName)+TEXT(".png"));
        const FString Target=FPaths::GetPath(Run.Output)/(FString(Name)+TEXT(".png"));
        Run.Check(FString(TEXT("render_"))+Name,Rendered && IFileManager::Get().Copy(*Target,*Source)==COPY_OK);
    };
    auto Quantities=[&]()
    {
        bool Same=Bag->Snapshot().Instances.Num()==Run.InventoryBefore.Instances.Num();
        for(const auto& E:Run.Counts)Same&=Bag->GetItemCount(E.Key)==E.Value;
        for(const auto& I:Run.InventoryBefore.Instances){const auto* After=Bag->FindInstance(I.Id);Same&=After && After->Definition==I.Definition && After->Durability==I.Durability;}
        return Same;
    };
    switch(Run.Step)
    {
    case 0:
    {
        C->Cancel();S->CancelAction();G->Enabled=true;G->Hunger=50;Settings->Comfort.TextScale=100;
        FHearthwardInventorySnapshot Fixture;Fixture.BackpackRank=3;Run.Check(TEXT("controlled_inventory"),Bag->RestoreInventory(Fixture));
        for(const auto& E:TArray<TPair<FName,int32>>{{TEXT("axe"),2},{TEXT("shortblade"),1},{TEXT("bow"),1},{TEXT("hood"),1},{TEXT("armor"),1},{TEXT("leggings"),1},{TEXT("boots"),1},{TEXT("gloves"),1},{TEXT("pickaxe"),1},{TEXT("wood"),5},{TEXT("stone"),5},{TEXT("roast"),4},{TEXT("medicine"),3},{TEXT("medicine_half"),2},{TEXT("arrow"),24},{TEXT("firepot"),2}})
            Run.Check(TEXT("fixture_add_")+E.Key.ToString(),Bag->TryAdd(E.Key,E.Value)==EHearthwardInventoryResult::Success);
        Run.InventoryBefore=Bag->Snapshot();for(const auto& D:HearthwardBasicItems())Run.Counts.Add(D.Id,Bag->GetItemCount(D.Id));
        Run.FirstAxe=Bag->FirstInstance(TEXT("axe"));UI->OpenPage(TEXT("inventory"));
        ++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 1:
    {
        Run.Check(TEXT("inventory_paused_with_ui_cursor"),World->IsPaused() && PC->bShowMouseCursor && !PC->IsMoveInputIgnored());
        Run.Check(TEXT("all_real_equipment_slots_visible"),Find(TEXT("inventory.equipment.hands")) && Find(TEXT("inventory.equipment.tool")) && CellCount(TEXT("inventory.equipment."))==12);
        const FVector2D Axe=Position(TEXT("inventory.bag.item.axe"));Down(Axe);Up(Axe);
        Run.Check(TEXT("click_selects_without_equipping"),!Bag->EquippedInstance(TEXT("weapon")).IsValid());
        Drag(TEXT("inventory.bag.item.axe"),Position(TEXT("inventory.equipment.head")));
        Run.Check(TEXT("wrong_equipment_slot_rejected"),!Bag->EquippedInstance(TEXT("weapon")).IsValid() && UI->GetMessage().Contains(TEXT("不匹配")));
        Drag(TEXT("inventory.bag.item.axe"),Position(TEXT("inventory.equipment.weapon")));
        Run.Check(TEXT("drop_equips_immediately_while_paused"),Bag->EquippedInstance(TEXT("weapon"))==Run.FirstAxe && !C->Busy() && World->IsPaused());
        Run.Check(TEXT("attack_stat_updates_immediately"),G->AttackPower()==30 && Find(TEXT("inventory.stats.5.value"))->GetStringField(TEXT("text"))==TEXT("30"));
        Run.Check(TEXT("equipped_instance_excluded_from_spare_count"),G->BackpackItemCount(TEXT("axe"))==1 && Find(TEXT("inventory.bag.item.axe"))->GetStringField(TEXT("text"))==TEXT("1") && Find(TEXT("inventory.bag.item.axe"))->GetNumberField(TEXT("value"))==0);
        Drag(TEXT("inventory.bag.item.axe"),Position(TEXT("inventory.equipment.weapon")));Run.SecondAxe=Bag->EquippedInstance(TEXT("weapon"));
        Run.Check(TEXT("duplicate_definition_equips_real_other_instance"),Run.SecondAxe.IsValid() && Run.SecondAxe!=Run.FirstAxe && Bag->GetItemCount(TEXT("axe"))==2);
        Drag(TEXT("inventory.equipment.weapon"),Position(TEXT("inventory.bag.empty.14")));
        Run.Check(TEXT("equipped_drag_to_empty_cell_unequips_and_places"),!Bag->EquippedInstance(TEXT("weapon")).IsValid() && SlotAt(TEXT("gear"),14)==TEXT("axe") && G->AttackPower()==0 && Find(TEXT("inventory.stats.5.value"))->GetStringField(TEXT("text"))==TEXT("0"));
        Drag(TEXT("inventory.bag.item.axe"),Position(TEXT("inventory.bag.item.shortblade")));
        Run.Check(TEXT("occupied_cells_swap_without_merging"),SlotAt(TEXT("gear"),14)==TEXT("shortblade") && Slots(TEXT("gear")).IndexOfByKey(TEXT("axe"))!=14);
        const int32 HoodPosition=Slots(TEXT("gear")).IndexOfByKey(TEXT("hood"));
        Drag(TEXT("inventory.bag.item.hood"),Position(TEXT("inventory.equipment.head")));
        Run.Check(TEXT("armor_stat_updates_immediately"),FMath::IsNearlyEqual(G->ArmorReduction(TEXT("head")),3.f,.001f) && Find(TEXT("inventory.stats.11.value"))->GetStringField(TEXT("text")).StartsWith(TEXT("3%")));
        Run.Check(TEXT("only_equipped_instance_vacates_original_cell"),G->BackpackItemCount(TEXT("hood"))==0 && !Find(TEXT("inventory.bag.item.hood")) && SlotAt(TEXT("gear"),HoodPosition).IsNone());
        Run.Check(TEXT("equipped_selection_keeps_real_item_details"),Find(TEXT("inventory.detail.name"))->GetStringField(TEXT("text"))==TEXT("皮制兜帽"));
        Run.Check(TEXT("hidden_equipment_cannot_move_without_unequipping"),!G->MoveInventoryItem(TEXT("hood"),6,Store->GetTimelineEpoch()) && Quantities());
        Drag(TEXT("inventory.bag.item.shortblade"),Position(TEXT("inventory.bag.empty.")+FString::FromInt(HoodPosition)));
        Run.Check(TEXT("vacated_equipment_cell_accepts_other_gear"),SlotAt(TEXT("gear"),HoodPosition)==TEXT("shortblade") && Bag->EquippedItem(TEXT("head"))==TEXT("hood") && Quantities());
        Drag(TEXT("inventory.equipment.head"),{1000,180});
        Run.Check(TEXT("drag_outside_equipment_unequips_without_ground_drop"),!Bag->EquippedInstance(TEXT("head")).IsValid() && G->ArmorReduction(TEXT("head"))==0 && Quantities());
        Run.Check(TEXT("unequipped_item_returns_to_lower_grid"),G->BackpackItemCount(TEXT("hood"))==1 && Find(TEXT("inventory.bag.item.hood")));
        Drag(TEXT("inventory.bag.item.gloves"),Position(TEXT("inventory.equipment.hands")));
        Drag(TEXT("inventory.bag.item.pickaxe"),Position(TEXT("inventory.equipment.tool")));
        Run.Check(TEXT("hand_and_tool_gear_can_be_drag_equipped"),Bag->EquippedItem(TEXT("hands"))==TEXT("gloves") && Bag->EquippedItem(TEXT("tool"))==TEXT("pickaxe"));
        Drag(TEXT("inventory.bag.item.axe"),Position(TEXT("inventory.equipment.weapon")));
        Drag(TEXT("inventory.bag.item.hood"),Position(TEXT("inventory.equipment.head")));
        Run.Check(TEXT("all_worn_singletons_removed_from_lower_grid"),!Find(TEXT("inventory.bag.item.hood")) && !Find(TEXT("inventory.bag.item.gloves")) && !Find(TEXT("inventory.bag.item.pickaxe")) && G->BackpackItemCount(TEXT("axe"))==1 && Quantities());
        Capture(TEXT("equipment-drag"));++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 2:
    {
        UI->ExecuteAction(TEXT("filter:材料"));
        Run.Check(TEXT("materials_use_full_thirty_cell_page"),CellCount(TEXT("inventory.bag."))==30 && !Find(TEXT("inventory.equipment.heading")) && !Find(TEXT("inventory.quick.heading")) && !Find(TEXT("inventory.bag.emptyState")) && Find(TEXT("inventory.bag.item.wood"))->GetNumberField(TEXT("y"))<300);
        Drag(TEXT("inventory.bag.item.wood"),Position(TEXT("inventory.bag.empty.29")));
        Run.Check(TEXT("material_can_move_to_new_upper_and_full_page_space"),SlotAt(TEXT("material"),29)==TEXT("wood") && !UI->ActionAt({106,265}).Contains(TEXT("wood")));
        Drag(TEXT("inventory.bag.item.wood"),Position(TEXT("inventory.bag.item.stone")));
        Run.Check(TEXT("material_occupied_cell_swaps"),SlotAt(TEXT("material"),29)==TEXT("stone"));
        const FVector2D Start=Position(TEXT("inventory.bag.item.wood")),End=Position(TEXT("inventory.bag.empty.28"));Down(Start);Move(Start,End);UI->Refresh();
        Run.Check(TEXT("periodic_refresh_preserves_drag_and_capture"),Snapshot()->GetBoolField(TEXT("inventory_dragging")) && UI->TakeWidget()->HasMouseCapture());
        UI->NativeOnPreviewKeyDown(UI->GetCachedGeometry(),FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));Up(End);
        Run.Check(TEXT("escape_cancels_drag_without_leaving_inventory"),UI->GetPage()==TEXT("inventory") && SlotAt(TEXT("material"),29)==TEXT("stone") && !UI->TakeWidget()->HasMouseCapture());
        Capture(TEXT("materials-full-page"));++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 3:
    {
        UI->ExecuteAction(TEXT("filter:消耗品"));
        bool ThrowablesInTools=true;
        for(const auto& V:HearthwardData::Rows(TEXT("items")))if(HearthwardData::Number(V->AsObject(),TEXT("throwDamage"))>0 || HearthwardData::Number(V->AsObject(),TEXT("bait"))>0)
            ThrowablesInTools&=UHearthwardGameplayComponent::InventoryTab(FName(*HearthwardData::Text(V->AsObject(),TEXT("id"))))==TEXT("tool");
        Run.Check(TEXT("throwables_belong_to_tools_not_consumables"),ThrowablesInTools && !Find(TEXT("inventory.bag.item.firepot")));
        Run.Check(TEXT("consumables_tab_renamed_and_equipment_removed"),UI->GetCategory()==TEXT("消耗品") && Find(TEXT("inventory.tab.消耗品")) && !Find(TEXT("inventory.tab.食物")) && !Find(TEXT("inventory.equipment.heading")) && CellCount(TEXT("inventory.quick."))==4);
        bool Keys=true;for(int32 Index=0;Index<4;++Index)
        {
            const TCHAR* Roles[]={TEXT("medicine"),TEXT("food"),TEXT("ammunition"),TEXT("throwable")};const auto Cell=Find(FString(TEXT("inventory.quick."))+Roles[Index]);
            Keys&=Cell && Cell->GetStringField(TEXT("shortcut"))==FString::FromInt(Index+1);
        }
        Run.Check(TEXT("backpack_four_role_slots_show_key_numbers"),Keys);
        Drag(TEXT("inventory.quick.ammunition"),Position(TEXT("inventory.bag.empty.13")));
        Run.Check(TEXT("other_category_quick_reference_cannot_enter_consumable_grid"),G->QuickItem(2)==TEXT("arrow") && UI->GetMessage().Contains(TEXT("不属于当前背包分类")));
        UI->ExecuteAction(TEXT("item:medicine"));Run.Check(TEXT("medicine_type_and_description_corrected"),Find(TEXT("inventory.detail.category"))->GetStringField(TEXT("text"))==TEXT("类型  药品") && Find(TEXT("inventory.detail.description"))->GetStringField(TEXT("text"))==TEXT("药品。"));
        Drag(TEXT("inventory.bag.item.medicine_half"),Position(TEXT("inventory.quick.medicine")));
        Run.Check(TEXT("medicine_drag_configures_real_quick_reference"),G->QuickItem(0)==TEXT("medicine_half") && Find(TEXT("inventory.quick.medicine"))->GetStringField(TEXT("asset"))==HearthwardData::Text(HearthwardData::Find(TEXT("items"),TEXT("medicine_half")),TEXT("icon")));
        Drag(TEXT("inventory.bag.item.medicine"),Position(TEXT("inventory.quick.food")));
        Run.Check(TEXT("medicine_cannot_fill_food_role"),G->QuickItem(1)==TEXT("roast") && UI->GetMessage().Contains(TEXT("无法放置到食物栏")));
        Drag(TEXT("inventory.bag.item.roast"),Position(TEXT("inventory.quick.medicine")));
        Run.Check(TEXT("food_cannot_fill_medicine_role"),G->QuickItem(0)==TEXT("medicine_half") && UI->GetMessage().Contains(TEXT("无法放置到药品栏")));
        Drag(TEXT("inventory.bag.item.roast"),Position(TEXT("inventory.bag.empty.14")));
        Run.Check(TEXT("consumable_can_move_to_empty_cell"),SlotAt(TEXT("consumable"),14)==TEXT("roast"));
        Drag(TEXT("inventory.quick.medicine"),{1000,180});
        Run.Check(TEXT("quick_drag_out_clears_configuration_without_consumption"),G->QuickItem(0).IsNone() && Quantities());
        Drag(TEXT("inventory.bag.item.medicine"),Position(TEXT("inventory.quick.medicine")));
        Settings->Comfort.TextScale=150;UI->Refresh();Capture(TEXT("consumables-text-150"));++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 4:
    {
        Settings->Comfort.TextScale=100;UI->ExecuteAction(TEXT("filter:工具"));
        Run.Check(TEXT("tools_show_quick_slots_without_equipment"),!Find(TEXT("inventory.equipment.heading")) && CellCount(TEXT("inventory.quick."))==4);
        G->AssignQuickItem(2,NAME_None);G->AssignQuickItem(3,NAME_None);UI->Refresh();
        Drag(TEXT("inventory.bag.item.arrow"),Position(TEXT("inventory.quick.ammunition")));
        Drag(TEXT("inventory.bag.item.firepot"),Position(TEXT("inventory.quick.throwable")));
        Run.Check(TEXT("ammo_and_throwable_can_be_drag_configured"),G->QuickItem(2)==TEXT("arrow") && G->QuickItem(3)==TEXT("firepot"));
        Drag(TEXT("inventory.bag.item.arrow"),Position(TEXT("inventory.quick.food")));
        Run.Check(TEXT("tools_cannot_fill_food_role"),G->QuickItem(1)==TEXT("roast") && UI->GetMessage().Contains(TEXT("无法放置")));
        Drag(TEXT("inventory.bag.item.firepot"),Position(TEXT("inventory.bag.empty.14")));
        Run.Check(TEXT("tool_can_move_to_empty_cell"),SlotAt(TEXT("tool"),14)==TEXT("firepot"));
        Drag(TEXT("inventory.quick.ammunition"),Position(TEXT("inventory.bag.empty.13")));
        Run.Check(TEXT("quick_drag_to_bag_clears_only_reference_and_moves_stack"),G->QuickItem(2).IsNone() && SlotAt(TEXT("tool"),13)==TEXT("arrow") && Quantities());
        Drag(TEXT("inventory.bag.item.arrow"),Position(TEXT("inventory.quick.ammunition")));
        Capture(TEXT("tools-quick-slots"));++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 5:
    {
        UI->OpenPage(TEXT("hud"));Run.Check(TEXT("food_reservation_fixture_starts"),G->UseItem(TEXT("roast")));UI->OpenPage(TEXT("inventory"));UI->ExecuteAction(TEXT("filter:消耗品"));
        Drag(TEXT("inventory.bag.item.roast"),Position(TEXT("inventory.bag.empty.13")));
        Run.Check(TEXT("moving_consumable_preserves_in_progress_reservation"),S->ActiveConsumable()==TEXT("roast") && Bag->Available(TEXT("roast"))==Run.Counts[TEXT("roast")]-1 && Quantities());
        Run.Check(TEXT("gear_drag_command_cannot_bypass_consumable_lock"),!G->SetBackpackEquipment(Bag->EquippedInstance(TEXT("weapon")),TEXT("weapon"),false,Store->GetTimelineEpoch()) && G->Feedback.Contains(TEXT("当前正在使用")));
        S->CancelAction();Run.Check(TEXT("reservation_release_still_preserves_quantity"),Quantities() && Bag->Available(TEXT("roast"))==Run.Counts[TEXT("roast")]);
        UI->ExecuteAction(TEXT("filter:材料"));Run.WoodPosition=Slots(TEXT("material")).IndexOfByKey(TEXT("wood"));
        Run.Check(TEXT("layout_and_quick_snapshot_validate"),UHearthwardGameplayComponent::ValidateSnapshot(G->SaveSnapshot()));
        Run.Check(TEXT("layout_snapshot_saved_to_real_point"),Saves->SavePoint(true));if(!Saves->GetPoints().IsEmpty())Run.SaveId=Saves->GetPoints().Last().SaveId;
        const FGuid WornHood=Bag->EquippedInstance(TEXT("head"));const int32 HiddenHoodPosition=G->InventorySlots(TEXT("gear"),true).IndexOfByKey(TEXT("hood"));
        G->MoveInventoryItem(TEXT("wood"),0,Store->GetTimelineEpoch());G->AssignQuickItem(0,TEXT("medicine_half"));UI->Refresh();
        const FVector2D Start=Position(TEXT("inventory.bag.item.wood"));Down(Start);Move(Start,{520,790});
        Run.Check(TEXT("drag_pending_before_real_load"),Snapshot()->GetBoolField(TEXT("inventory_dragging")));
        Run.Check(TEXT("real_point_restored"),Run.SaveId.IsValid() && Saves->LoadPoint(Run.SaveId));
        Up({520,790});UI->Refresh();Trace(TEXT("after_load"));
        Run.Check(TEXT("load_cancels_old_drag_and_releases_capture"),!UI->TakeWidget()->HasMouseCapture() && !Snapshot()->GetBoolField(TEXT("inventory_dragging")));
        Run.Check(TEXT("real_load_restores_sparse_positions_and_quick_roles"),Slots(TEXT("material")).IndexOfByKey(TEXT("wood"))==Run.WoodPosition && SlotAt(TEXT("material"),29)==TEXT("stone") && G->QuickItem(0)==TEXT("medicine") && G->QuickItem(2)==TEXT("arrow") && Quantities());
        Run.Check(TEXT("real_load_keeps_worn_item_hidden_and_spares_counted"),Bag->EquippedInstance(TEXT("head"))==WornHood && SlotAt(TEXT("gear"),HiddenHoodPosition).IsNone() && G->BackpackItemCount(TEXT("hood"))==0 && G->BackpackItemCount(TEXT("axe"))==1);
        Run.Check(TEXT("stale_layout_command_rejected"),!G->MoveInventoryItem(TEXT("wood"),4,FGuid::NewGuid()));
        Run.Check(TEXT("stale_gear_command_rejected"),!G->SetBackpackEquipment(Bag->FirstInstance(TEXT("axe")),TEXT("weapon"),true,FGuid::NewGuid()));
        Run.Check(TEXT("invalid_cells_do_not_mutate_inventory"),!G->MoveInventoryItem(TEXT("wood"),500,Store->GetTimelineEpoch()) && !G->MoveInventoryItem(TEXT("wood"),-1,Store->GetTimelineEpoch()) && Quantities());
        // A restored session intentionally returns to HUD. Reopen the backpack for subsequent gestures.
        UI->OpenPage(TEXT("inventory"));
        // Exercise moving between pages with a held pointer and backpack wheel paging.
        Run.Check(TEXT("cross_page_fixture_moves_axe"),G->MoveInventoryItem(TEXT("axe"),19,Store->GetTimelineEpoch()));Run.Check(TEXT("gear_filter_after_load"),UI->ExecuteAction(TEXT("filter:装备")));
        Run.Check(TEXT("next_page_after_load"),UI->ExecuteAction(TEXT("inventory.next")));Trace(TEXT("before_cross_page"));const FVector2D Axe=Position(TEXT("inventory.bag.item.axe"));Down(Axe);Move(Axe,Axe+FVector2D(8,8));
        const FPointerEvent Wheel(0,Absolute(Axe),Absolute(Axe),TSet<FKey>{EKeys::LeftMouseButton},FKey(),1,FModifierKeysState());
        UI->NativeOnMouseWheel(UI->GetCachedGeometry(),Wheel);UI->NativeOnMouseWheel(UI->GetCachedGeometry(),Wheel);UI->NativeOnMouseWheel(UI->GetCachedGeometry(),Wheel);
        Trace(TEXT("after_wheel"));Up(Position(TEXT("inventory.bag.empty.13")));
        Run.Check(TEXT("held_drag_can_move_between_inventory_pages"),SlotAt(TEXT("gear"),13)==TEXT("axe") && Quantities() && !UI->TakeWidget()->HasMouseCapture());
        const FVector2D From=Position(TEXT("inventory.bag.item.axe"));Down(From);Move(From,{1200,200});UI->OpenPage(TEXT("settings"));Up({1200,200});
        Run.Check(TEXT("page_change_cancels_capture_without_equipment_mutation"),UI->GetPage()==TEXT("settings") && !UI->TakeWidget()->HasMouseCapture() && Quantities());
        UI->ExecuteAction(TEXT("back"));Trace(TEXT("after_settings_back"));Run.Check(TEXT("settings_returns_to_inventory"),UI->GetPage()==TEXT("inventory"));
        UI->OpenPage(TEXT("hud"));Run.Check(TEXT("drag_session_returns_to_playable_hud"),!World->IsPaused() && !PC->bShowMouseCursor);
        ++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 6:
    {
        Settings->Comfort.TextScale=100;auto State=Bag->Snapshot();State.BackpackRank=1;
        Run.Check(TEXT("management_fixture_preserves_instances"),Bag->RestoreInventory(State));
        Run.ManagedAxe=Bag->EquippedInstance(TEXT("weapon"));
        Run.Check(TEXT("management_real_wear_for_repair_quote"),Run.ManagedAxe.IsValid() && Bag->WearInstance(Run.ManagedAxe,2));
        UI->OpenPage(TEXT("inventory"));UI->ExecuteAction(TEXT("page:equipment"));
        // The HUD is hit-test invisible. Let Slate repaint the newly interactive page before sending clicks.
        ++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 7:
    {
        Click(TEXT("equipment.list.item.")+Run.ManagedAxe.ToString());ManagementLayout(TEXT("management_player_100"));
        Run.Check(TEXT("management_shows_actual_selected_instance_and_wear"),Find(TEXT("equipment.detail.state"))->GetStringField(TEXT("text"))==TEXT("已装备") && Find(TEXT("equipment.detail.property.1.value"))->GetStringField(TEXT("text"))==TEXT("78.00 / 80"));
        Run.Check(TEXT("management_keeps_transfer_equip_repair_drop_upgrade"),Find(TEXT("equipment.transfer.brother")) && Find(TEXT("equipment.transfer.storage")) && Find(TEXT("equipment.equip")) && Find(TEXT("equipment.repair.25")) && Find(TEXT("equipment.repair.50")) && Find(TEXT("equipment.repair.100")) && Find(TEXT("equipment.drop")) && Find(TEXT("equipment.upgrade")));
        Run.Check(TEXT("management_quotes_and_next_rank_read_real_data"),Find(TEXT("equipment.repair.quote"))->GetStringField(TEXT("text")).Contains(TEXT("恢复")) && Find(TEXT("equipment.upgrade.cost"))->GetStringField(TEXT("text")).Contains(TEXT("150")));
        Capture(TEXT("management-player-100"));++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 8:
    {
        Settings->Comfort.TextScale=150;UI->Refresh();
        const FString Before=Bag->DescribeInventory();Click(TEXT("equipment.next"));
        Run.Check(TEXT("management_page_click_changes_visible_range"),Find(TEXT("equipment.list.range"))->GetStringField(TEXT("text")).StartsWith(TEXT("7–")));
        Click(TEXT("equipment.list.item.arrow"));
        Run.Check(TEXT("management_stack_selection_shows_quantity_controls"),Find(TEXT("equipment.quantity")) && !Find(TEXT("equipment.repair.100")) && Find(TEXT("equipment.detail.state"))->GetStringField(TEXT("text"))==TEXT("持有 ×24"));
        Click(TEXT("equipment.quantity.plus"));Run.Check(TEXT("management_quantity_plus_works"),Find(TEXT("equipment.quantity"))->GetStringField(TEXT("text"))==TEXT("2"));
        Click(TEXT("equipment.quantity.minus"));Run.Check(TEXT("management_quantity_minus_works"),Find(TEXT("equipment.quantity"))->GetStringField(TEXT("text"))==TEXT("1"));
        Click(TEXT("equipment.owner.player"));Click(TEXT("equipment.list.item.")+Run.ManagedAxe.ToString());
        Run.Check(TEXT("management_navigation_preserves_inventory"),Bag->DescribeInventory()==Before);
        ManagementLayout(TEXT("management_player_150"));
        Capture(TEXT("management-player-150"));ExportSize(TEXT("management-720p"),1280,720);ExportSize(TEXT("management-ultrawide"),2560,1080);
        ++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 9:
    {
        AHearthwardCompanionFixture* Brother=nullptr;for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It){Brother=*It;break;}
        Run.Check(TEXT("management_brother_available"),Brother!=nullptr);if(!Brother)return Run.Finish();
        Brother->SetActorLocation(Pawn->GetActorLocation()+FVector(100,0,0));
        auto* BrotherSurvival=Brother->FindComponentByClass<UHearthwardSurvivalComponent>();BrotherSurvival->ResetTransient();BrotherSurvival->State.Life=EHearthwardLife::Alive;
        BrotherSurvival->State.DrowningRemaining=-1;Brother->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Run.Check(TEXT("management_brother_command_cancelled"),Brother->Cancel(Pawn,true));
        Run.Check(TEXT("management_brother_ready_for_real_transfer"),!Brother->EquipmentBusy() && !BrotherSurvival->Busy() && BrotherSurvival->SafeToSave());
        FHearthwardInventorySnapshot Empty;Empty.BackpackRank=1;
        Run.Check(TEXT("management_isolated_brother_inventory"),Brother->Bag->RestoreInventory(Empty) && Brother->Bag->TryAdd(TEXT("hood"),1)==EHearthwardInventoryResult::Success);
        Run.TransferredAxe=Bag->FirstInstance(TEXT("axe"),true);const auto* Source=Bag->FindInstance(Run.TransferredAxe);const double Wear=Source?Source->Durability:-1;
        Click(TEXT("equipment.list.item.")+Run.TransferredAxe.ToString());Click(TEXT("equipment.transfer.brother"));
        const auto* Received=Brother->Bag->FindInstance(Run.TransferredAxe);
        Run.Check(TEXT("management_real_transfer_keeps_identity_and_durability"),!Bag->FindInstance(Run.TransferredAxe) && Received && Received->Durability==Wear && Bag->EquippedInstance(TEXT("weapon"))==Run.ManagedAxe && G->BackpackItemCount(TEXT("axe"))==0);
        Click(TEXT("equipment.owner.brother"));Click(TEXT("equipment.list.item.")+Run.TransferredAxe.ToString());Click(TEXT("equipment.equip"));
        ManagementLayout(TEXT("management_brother_150"));
        Run.Check(TEXT("management_brother_equip_remains_functional"),Brother->Bag->EquippedInstance(TEXT("weapon"))==Run.TransferredAxe && Find(TEXT("equipment.detail.state"))->GetStringField(TEXT("text"))==TEXT("已装备"));
        Run.Check(TEXT("management_brother_has_upgrade_without_player_only_actions"),Find(TEXT("equipment.upgrade")) && Find(TEXT("equipment.transfer.player")) && !Find(TEXT("equipment.repair.100")) && !Find(TEXT("equipment.drop")));
        Capture(TEXT("management-brother-150"));++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 10:
    {
        Click(TEXT("equipment.equip"));Click(TEXT("equipment.transfer.player"));
        Run.Check(TEXT("management_roundtrip_instance_returns_as_spare"),Bag->FindInstance(Run.TransferredAxe) && !Bag->IsEquipped(Run.TransferredAxe) && G->BackpackItemCount(TEXT("axe"))==1 && Bag->EquippedInstance(TEXT("weapon"))==Run.ManagedAxe);
        Click(TEXT("equipment.owner.storage"));ManagementLayout(TEXT("management_storage_150"));
        Run.Check(TEXT("management_storage_does_not_offer_personal_equip_or_upgrade"),!Find(TEXT("equipment.equip")) && !Find(TEXT("equipment.upgrade")) && Find(TEXT("equipment.transfer.player")) && Find(TEXT("equipment.transfer.brother")));
        Capture(TEXT("management-storage-150"));++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 11:
    {
        Click(TEXT("equipment.return"));Run.Check(TEXT("management_return_reaches_backpack"),UI->GetPage()==TEXT("inventory") && World->IsPaused());
        UI->ExecuteAction(TEXT("filter:装备"));
        Run.Check(TEXT("management_transfer_updates_backpack_spare_view"),Find(TEXT("inventory.bag.item.axe")) && Find(TEXT("inventory.bag.item.axe"))->GetStringField(TEXT("text"))==TEXT("1") && !Find(TEXT("inventory.bag.item.hood")));
        UI->OpenPage(TEXT("hud"));Run.Check(TEXT("management_returns_to_playable_hud"),!World->IsPaused() && !PC->bShowMouseCursor && !World->GetGameInstance()->GetGameViewportClient()->IgnoreInput());
        Run.ReturnInventory=Bag->DescribeInventory();++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 12:
    {
        const TCHAR* Categories[]={TEXT("装备"),TEXT("材料"),TEXT("消耗品"),TEXT("工具")};
        GConfig->SetBool(TEXT("Hearthward.Survival"),TEXT("MenuPause"),Run.ReturnCase<12,GGameUserSettingsIni);
        UI->OpenPage(TEXT("hud"));UI->OpenPage(TEXT("inventory"));UI->ExecuteAction(TEXT("filter:")+FString(Categories[(Run.ReturnCase/3)%4]));
        ++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 13:
    {
        Click(TEXT("inventory.equipment.manage"));
        Run.Check(TEXT("return_case_")+FString::FromInt(Run.ReturnCase)+TEXT("_real_backpack_entry"),UI->GetPage()==TEXT("equipment"));
        ++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 14:
    {
        const TCHAR* Categories[]={TEXT("装备"),TEXT("材料"),TEXT("消耗品"),TEXT("工具")};
        const FString Prefix=TEXT("return_case_")+FString::FromInt(Run.ReturnCase);
        const int32 Mode=Run.ReturnCase%3;
        if(Mode==1 && (Run.ReturnCase/3)%2==1)
        {
            UI->ExecuteAction(TEXT("page:settings"));Escape();
            Run.Check(Prefix+TEXT("_nested_settings_returns_to_management"),UI->GetPage()==TEXT("equipment"));
        }
        if(Mode==1)Escape();else Click(Mode==0?TEXT("equipment.back"):TEXT("equipment.return"));
        Run.Check(Prefix+TEXT("_returns_to_original_backpack_tab"),UI->GetPage()==TEXT("inventory") && UI->GetCategory()==Categories[(Run.ReturnCase/3)%4]);
        Run.Check(Prefix+TEXT("_backpack_pause_cursor_and_focus"),World->IsPaused()==(Run.ReturnCase<12) && PC->bShowMouseCursor && UI->HasKeyboardFocus());
        Run.Check(Prefix+TEXT("_inventory_preserved"),Bag->DescribeInventory()==Run.ReturnInventory);
        if(Run.ReturnCase==0)ExportSize(TEXT("management-footer-returns-backpack"),1600,1000);
        if(Run.ReturnCase==22)ExportSize(TEXT("management-escape-returns-tools"),1600,1000);
        Escape();
        Run.Check(Prefix+TEXT("_next_escape_leaves_backpack_normally"),UI->GetPage()==TEXT("hud") && !World->IsPaused() && !PC->bShowMouseCursor && !World->GetGameInstance()->GetGameViewportClient()->IgnoreInput());
        if(++Run.ReturnCase<24)Run.Step=12;else Run.Step=15;
        Run.WaitUntil=Now+.3;break;
    }
    case 15:
    {
        UI->OpenPage(TEXT("hud"));UI->OpenPage(TEXT("equipment"));
        ++Run.Step;Run.WaitUntil=Now+.3;break;
    }
    case 16:
    {
        Escape();Run.Check(TEXT("management_direct_entry_escape_still_returns_backpack"),UI->GetPage()==TEXT("inventory") && PC->bShowMouseCursor && !World->IsPaused());
        Escape();Run.Check(TEXT("management_final_escape_restores_playable_world"),UI->GetPage()==TEXT("hud") && !World->IsPaused() && !PC->bShowMouseCursor && !World->GetGameInstance()->GetGameViewportClient()->IgnoreInput());
        return Run.Finish();
    }
    }
    return true;
}
const FTSTicker::FDelegateHandle BackpackDragTicker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&VerifyBackpackDrag),.01f);
}
#endif
