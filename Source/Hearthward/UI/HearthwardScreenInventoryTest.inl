// PROTOTYPE_ONLY: inventory UI checks in an explicitly isolated PIE save pool.
#if !UE_BUILD_SHIPPING
#include "../Combat/HearthwardCombatComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
namespace
{
void VerifyInventoryPreview(UWorld* World)
{
    const FString Run=FPlatformMisc::GetEnvironmentVariable(TEXT("HEARTHWARD_TITLE_RUN"));
    FString Pool; FGuid PoolId;
    if(!World || World->WorldType!=EWorldType::PIE || !Run.StartsWith(TEXT("verify_"))
        || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,PoolId) || !PoolId.IsValid()) return;
    auto Report=MakeShared<FJsonObject>(),Checks=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Captures,Layouts;
    bool Passed=true;
    auto Check=[&](const FString& Name,bool Result){Checks->SetBoolField(Name,Result);Passed&=Result;};
    auto* Controller=World->GetFirstPlayerController(); auto* HUD=Controller?Cast<AHearthwardHUD>(Controller->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr; Check(TEXT("screen_available"),UI!=nullptr);
    if(UI && Controller->GetPawn())
    {
        auto* Pawn=Controller->GetPawn().Get(); auto* G=Pawn->FindComponentByClass<UHearthwardGameplayComponent>();
        auto* Combat=Pawn->FindComponentByClass<UHearthwardCombatComponent>();
        Check(TEXT("fixture_starts_without_pending_combat_action"),Combat && !Combat->Busy());
        auto* Bag=Pawn->FindComponentByClass<UHearthwardInventoryComponent>(); auto* Settings=World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
        const auto BagBefore=Bag->Snapshot(); const FString BagDescriptionBefore=Bag->DescribeInventory(),GameplayBefore=G->SaveSnapshot(),FeedbackBefore=G->Feedback;
        const auto ComfortBefore=Settings->Comfort; const int32 SavesBefore=World->GetSubsystem<UHearthwardSaveSubsystem>()->GetPoints().Num();
        FHearthwardInventorySnapshot Fixture; Fixture.BackpackRank=3;
        Check(TEXT("isolated_inventory_fixture"),Bag->RestoreInventory(Fixture)); G->Equipment.Reset();G->Durability.Reset();
        int32 EquipmentAdded=0;
        for(const auto& V:HearthwardData::Rows(TEXT("items")))
        {
            const auto R=V->AsObject();
            if(HearthwardData::Text(R,TEXT("category"))==TEXT("装备") && EquipmentAdded<22)
            {
                if(Bag->TryAdd(FName(*HearthwardData::Text(R,TEXT("id"))),1)==EHearthwardInventoryResult::Success) ++EquipmentAdded;
            }
        }
        for(const auto& Entry:TArray<TPair<FName,int32>>{{TEXT("wood"),5},{TEXT("roast"),2},{TEXT("medicine"),1},{TEXT("arrow"),8},{TEXT("firepot"),2},{TEXT("amulet"),1},{TEXT("treasure_map_1"),1},{TEXT("blueprint_hunter_bow"),1}})
            Check(TEXT("fixture_add_")+Entry.Key.ToString(),Bag->TryAdd(Entry.Key,Entry.Value)==EHearthwardInventoryResult::Success);
        Check(TEXT("fixture_has_multiple_equipment_pages"),EquipmentAdded==22);
        auto Snapshot=[&]()
        {TSharedPtr<FJsonObject> Object;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),Object);return Object;};
        auto Find=[&](const TSharedPtr<FJsonObject>& Object,const FString& Id)->TSharedPtr<FJsonObject>
        {for(const auto& V:Object->GetArrayField(TEXT("elements")))if(V->AsObject()->GetStringField(TEXT("id"))==Id)return V->AsObject();return nullptr;};
        auto Clean=[&](const FString& Name)
        {
            const auto Object=Snapshot();Layouts.Add(MakeShared<FJsonValueObject>(Object));
            int32 Tabs=0,Surfaces=0;bool InBounds=true,NoLegacy=true;
            for(const auto& V:Object->GetArrayField(TEXT("elements")))
            {
                const auto E=V->AsObject();const FString Type=E->GetStringField(TEXT("type")),Id=E->GetStringField(TEXT("id"));
                Tabs+=Type==TEXT("menuTab");Surfaces+=Type==TEXT("inventorySurface");
                InBounds&=E->GetNumberField(TEXT("x"))>=0 && E->GetNumberField(TEXT("y"))>=0
                    && E->GetNumberField(TEXT("x"))+E->GetNumberField(TEXT("width"))<=1672
                    && E->GetNumberField(TEXT("y"))+E->GetNumberField(TEXT("height"))<=941;
                NoLegacy&=Id!=TEXT("background") && E->GetStringField(TEXT("asset"))!=TEXT("inventoryBackground") && E->GetStringField(TEXT("asset"))!=TEXT("leatherPanel");
            }
            Check(Name+TEXT("_four_tabs_three_opaque_panes"),Tabs==4 && Surfaces==3 && NoLegacy);
            Check(Name+TEXT("_inside_viewport"),InBounds);
            const auto Description=Find(Object,TEXT("inventory.detail.description"));
            if(Description) Check(Name+TEXT("_description_above_properties"),Description->GetNumberField(TEXT("y"))+Description->GetNumberField(TEXT("height"))+16<=Find(Object,TEXT("inventory.detail.propertiesHeading"))->GetNumberField(TEXT("y"))+.01); // Float rectangle serialization can differ by a fraction of a pixel.
        };
        auto Capture=[&](const FString& Name,int32 Width=1672,int32 Height=941)
        {const FString File=Run+TEXT("-")+Name;const bool Done=UI->CaptureUI(File,Width,Height);Check(TEXT("capture_")+Name,Done);if(Done)Captures.Add(MakeShared<FJsonValueString>(File+TEXT(".png")));};
        UI->OpenPage(TEXT("inventory"));
        Check(TEXT("equipment_is_initial_tab"),UI->GetCategory()==TEXT("装备"));
        const FString InventoryUnchanged=Bag->DescribeInventory();
        for(int32 Tab=0;Tab<4;++Tab)
        {
            const FString Category=TArray<FString>{TEXT("装备"),TEXT("材料"),TEXT("消耗品"),TEXT("工具")}[Tab];
            Check(TEXT("tab_hit_")+Category,UI->ActionAt(FVector2D(304+192*Tab,58))==TEXT("filter:")+Category);
            Check(TEXT("tab_action_")+Category,UI->ExecuteAction(TEXT("filter:")+Category) && UI->GetCategory()==Category);
            Clean(Category);Capture(TEXT("inventory-tab-")+FString::FromInt(Tab));
        }
        const auto Tools=Snapshot();
        Check(TEXT("quest_objects_and_blueprints_remain_accessible"),Find(Tools,TEXT("inventory.bag.item.amulet")) && Find(Tools,TEXT("inventory.bag.item.treasure_map_1")) && Find(Tools,TEXT("inventory.bag.item.blueprint_hunter_bow")));
        Check(TEXT("removed_quest_tab_action_rejected"),!UI->ExecuteAction(TEXT("filter:任务")));
        UI->ExecuteAction(TEXT("filter:装备"));
        Check(TEXT("first_page_prev_not_clickable"),UI->ActionAt({145,821}).IsEmpty());
        Check(TEXT("next_page_changes_visible_items"),UI->ExecuteAction(TEXT("inventory.next")) && Find(Snapshot(),TEXT("inventory.bag.range"))->GetStringField(TEXT("text"))==TEXT("11–22 / 22"));
        Check(TEXT("last_page_next_not_clickable"),UI->ActionAt({463,821}).IsEmpty());Clean(TEXT("last_page"));Capture(TEXT("inventory-last-page"));
        Check(TEXT("previous_page_returns_first"),UI->ExecuteAction(TEXT("inventory.prev")) && Find(Snapshot(),TEXT("inventory.bag.range"))->GetStringField(TEXT("text"))==TEXT("1–15 / 22"));
        Check(TEXT("selection_and_navigation_do_not_consume_items"),Bag->DescribeInventory()==InventoryUnchanged);
        UI->ExecuteAction(TEXT("item:axe")); const auto Axe=Snapshot();
        Check(TEXT("axe_description_from_original_data"),Find(Axe,TEXT("inventory.detail.description"))->GetStringField(TEXT("text"))==HearthwardData::Text(HearthwardData::Find(TEXT("items"),TEXT("axe")),TEXT("description")));
        Check(TEXT("axe_original_attack_and_durability"),Find(Axe,TEXT("inventory.detail.property.0.value"))->GetStringField(TEXT("text"))==TEXT("30") && Find(Axe,TEXT("inventory.detail.property.1.value"))->GetStringField(TEXT("text"))==TEXT("80 / 80"));
        Check(TEXT("character_values_read_gameplay"),Find(Axe,TEXT("inventory.stats.2.value"))->GetStringField(TEXT("text"))==FString::Printf(TEXT("%.0f / %.0f"),G->Health,G->MaxHealth()) && Find(Axe,TEXT("inventory.stats.5.value"))->GetStringField(TEXT("text"))==FString::Printf(TEXT("%.0f"),G->AttackPower()));
        bool OriginalAttributes=true;
        for(const auto& V:Axe->GetArrayField(TEXT("elements")))
        {
            const FString Label=V->AsObject()->GetStringField(TEXT("text"));
            OriginalAttributes&=!Label.Contains(TEXT("智力")) && !Label.Contains(TEXT("信仰")) && !Label.Contains(TEXT("咒语")) && !Label.Contains(TEXT("专注值")) && !Label.Contains(TEXT("记忆空格"));
        }
        Check(TEXT("no_reference_game_only_attributes"),OriginalAttributes);
        Check(TEXT("existing_equip_action_starts_original_delay"),UI->ExecuteAction(TEXT("use")) && Combat->Action==TEXT("switch") && FMath::IsNearlyEqual(Combat->Duration,.4));
        Combat->Cancel(); // A separate PIE frame test below verifies the real delayed completion.
        Check(TEXT("equipped_render_fixture"),Bag->EquipInstance(Bag->FirstInstance(TEXT("axe"))));
        UI->OpenPage(TEXT("inventory"));UI->ExecuteAction(TEXT("item:axe"));Capture(TEXT("inventory-equipped-axe"));
        const auto Equipped=Snapshot();Check(TEXT("equipped_weapon_slot_updates"),Find(Equipped,TEXT("inventory.equipment.weapon"))->GetStringField(TEXT("asset"))==TEXT("axe"));
        const int32 FoodBefore=Bag->GetItemCount(TEXT("roast"));G->Hunger=55;
        UI->ExecuteAction(TEXT("item:roast"));Check(TEXT("external_item_selection_switches_category"),UI->GetCategory()==TEXT("消耗品"));
        Check(TEXT("existing_food_use_starts_reserved_three_second_action"),UI->ExecuteAction(TEXT("use")) && Bag->GetItemCount(TEXT("roast"))==FoodBefore && Bag->Available(TEXT("roast"))==FoodBefore-1 && G->Hunger==55);
        Pawn->FindComponentByClass<UHearthwardSurvivalComponent>()->CancelAction();
        Check(TEXT("food_cancellation_preserves_quantity"),Bag->Available(TEXT("roast"))==FoodBefore);
        UI->ExecuteAction(TEXT("item:wood"));const int32 WoodBefore=Bag->GetItemCount(TEXT("wood"));
        Check(TEXT("existing_drop_retained"),UI->ExecuteAction(TEXT("drop")) && Bag->GetItemCount(TEXT("wood"))==WoodBefore-1);
        Check(TEXT("settings_entry_from_inventory"),UI->ExecuteAction(TEXT("page:settings")) && UI->GetPage()==TEXT("settings"));
        Check(TEXT("settings_back_preserves_inventory_tab"),UI->ExecuteAction(TEXT("back")) && UI->GetPage()==TEXT("inventory") && UI->GetCategory()==TEXT("材料"));
        UI->ExecuteAction(TEXT("ask:quit"));
        Check(TEXT("confirmation_blocks_inventory_paging"),!UI->ExecuteAction(TEXT("inventory.next")));UI->ExecuteAction(TEXT("cancel"));
        Settings->Comfort.TextScale=150;UI->ExecuteAction(TEXT("item:axe"));Clean(TEXT("text_150"));Capture(TEXT("inventory-text-150"));
        UI->ExecuteAction(TEXT("item:firepot"));Clean(TEXT("long_description_150"));Capture(TEXT("inventory-tools-text-150"));
        Capture(TEXT("inventory-720p"),1280,720);Capture(TEXT("inventory-ultrawide"),2560,1080);
        UI->ExecuteAction(TEXT("page:equipment"));Check(TEXT("instance_equipment_management_entry_retained"),UI->GetPage()==TEXT("equipment"));
        UI->OpenPage(TEXT("inventory"));Check(TEXT("back_returns_hud"),UI->ExecuteAction(TEXT("back")) && UI->GetPage()==TEXT("hud"));
        FHearthwardInventorySnapshot Empty;Empty.BackpackRank=3;Bag->RestoreInventory(Empty);G->Equipment.Reset();UI->OpenPage(TEXT("inventory"));Clean(TEXT("empty"));Capture(TEXT("inventory-empty"));
        Check(TEXT("empty_has_no_item_hit_or_stale_description"),UI->ActionAt({106,586}).IsEmpty() && Find(Snapshot(),TEXT("inventory.detail.emptyState")) && !Find(Snapshot(),TEXT("inventory.detail.description")));
        Settings->Comfort=ComfortBefore;Bag->RestoreInventory(BagBefore);G->Restore(GameplayBefore);G->Feedback=FeedbackBefore;
        Check(TEXT("inventory_and_gameplay_fixture_restored"),Bag->DescribeInventory()==BagDescriptionBefore && Bag->Snapshot().Stacks.OrderIndependentCompareEqual(BagBefore.Stacks) && Bag->Snapshot().Instances.Num()==BagBefore.Instances.Num() && Bag->Snapshot().Equipped.OrderIndependentCompareEqual(BagBefore.Equipped) && G->SaveSnapshot()==GameplayBefore);
        Check(TEXT("save_nodes_unchanged"),World->GetSubsystem<UHearthwardSaveSubsystem>()->GetPoints().Num()==SavesBefore);UI->OpenPage(TEXT("title"));
    }
    Report->SetBoolField(TEXT("passed"),Passed);Report->SetObjectField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("captures"),Captures);Report->SetArrayField(TEXT("layouts"),Layouts);
    FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("TitleWheel")/Run/TEXT("inventory-preview.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
FAutoConsoleCommandWithWorld VerifyInventoryPreviewCommand(TEXT("Hearthward.UI.VerifyInventoryPreview"),TEXT("Verify inventory presentation in an explicit isolated PIE save pool."),FConsoleCommandWithWorldDelegate::CreateStatic(&VerifyInventoryPreview));
}
#endif
