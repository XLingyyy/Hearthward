// PROTOTYPE_ONLY: journal presentation checks, isolated PIE pool, absent from Shipping.
#if !UE_BUILD_SHIPPING
#include "../Campaign/HearthwardCampaignSubsystem.h"
namespace
{
void VerifyJournalPreview(UWorld* World)
{
    const FString Run=FPlatformMisc::GetEnvironmentVariable(TEXT("HEARTHWARD_TITLE_RUN"));
    FString Pool;FGuid PoolId;
    if(!World || World->WorldType!=EWorldType::PIE || !Run.StartsWith(TEXT("verify_"))
        || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,PoolId) || !PoolId.IsValid()) return;
    auto Report=MakeShared<FJsonObject>(),Checks=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Captures,Layouts;bool Passed=true;
    auto Check=[&](const FString& Name,bool Result){Checks->SetBoolField(Name,Result);Passed&=Result;};
    auto* Controller=World->GetFirstPlayerController();auto* HUD=Controller?Cast<AHearthwardHUD>(Controller->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr;Check(TEXT("screen_available"),UI && Controller->GetPawn());
    if(UI && Controller->GetPawn())
    {
        auto* G=Controller->GetPawn()->FindComponentByClass<UHearthwardGameplayComponent>();
        auto* Bag=Controller->GetPawn()->FindComponentByClass<UHearthwardInventoryComponent>();
        auto* Settings=World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
        const FString GameplayBefore=G->SaveSnapshot(),FeedbackBefore=G->Feedback,InventoryBefore=Bag->DescribeInventory();
        const auto BagBefore=Bag->Snapshot();const auto ComfortBefore=Settings->Comfort;
        const int32 SavesBefore=World->GetSubsystem<UHearthwardSaveSubsystem>()->GetPoints().Num();
        Check(TEXT("fixture_uses_original_noncampaign_rules"),!World->GetSubsystem<UHearthwardCampaignSubsystem>()->Active());
        FHearthwardInventorySnapshot Empty;Empty.BackpackRank=3;Check(TEXT("isolated_empty_inventory"),Bag->RestoreInventory(Empty));
        G->Equipment.Reset();G->Claimed.Reset();G->Events.Reset();G->Discovered.Reset();G->TrackedQuest=TEXT("ember");Settings->Comfort.TextScale=100;
        UI->OpenPage(TEXT("journal"));UI->ExecuteAction(TEXT("category:main"));UI->ExecuteAction(TEXT("quest:ember"));
        auto Snapshot=[&]()
        {TSharedPtr<FJsonObject> Object;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),Object);return Object;};
        auto Find=[&](const TSharedPtr<FJsonObject>& Object,const FString& Id)->TSharedPtr<FJsonObject>
        {for(const auto& V:Object->GetArrayField(TEXT("elements")))if(V->AsObject()->GetStringField(TEXT("id"))==Id)return V->AsObject();return nullptr;};
        auto Label=[&](const FString& Id)->FString {const auto R=Find(Snapshot(),Id);return R?R->GetStringField(TEXT("text")):FString();};
        auto Clean=[&](const FString& Name)
        {
            const auto Object=Snapshot();Layouts.Add(MakeShared<FJsonValueObject>(Object));
            int32 Surfaces=0,Backdrops=0,Tabs=0,Icons=0;bool Inside=true,NoScenery=true,Left=true,Right=true,IconOnly=true;
            for(const auto& V:Object->GetArrayField(TEXT("elements")))
            {
                const auto E=V->AsObject();const FString Id=E->GetStringField(TEXT("id")),Type=E->GetStringField(TEXT("type")),Asset=E->GetStringField(TEXT("asset"));
                Surfaces+=Type==TEXT("menuSurface");Backdrops+=Type==TEXT("journalBackdrop");Tabs+=Type==TEXT("menuTab");Icons+=Id.StartsWith(TEXT("journal.header.icon."));
                if(Type==TEXT("menuTab"))IconOnly&=E->GetStringField(TEXT("text")).IsEmpty();
                NoScenery&=Asset!=TEXT("logo") && Asset!=TEXT("journalStory") && Asset!=TEXT("leatherPanel") && Asset!=TEXT("darkTexture");
                const double X=E->GetNumberField(TEXT("x")),Y=E->GetNumberField(TEXT("y")),W=E->GetNumberField(TEXT("width")),H=E->GetNumberField(TEXT("height"));
                Inside&=X>=0 && Y>=0 && X+W<=1672.01 && Y+H<=941.01;
                if(Id.StartsWith(TEXT("journal.list.")))Left&=X>=40 && X+W<=704.01 && Y>=136 && Y+H<=842.01;
                if(Id.StartsWith(TEXT("journal.detail.")))Right&=X>=752 && X+W<=1632.01 && Y>=136 && Y+H<=842.01;
            }
            Check(Name+TEXT("_two_panes_six_original_symbols"),Surfaces==2 && Backdrops==1 && Tabs==6 && Icons==6 && IconOnly && NoScenery);
            Check(Name+TEXT("_columns_and_controls_inside_viewport"),Inside && Left && Right);
        };
        auto Capture=[&](const FString& Name,int32 Width=1672,int32 Height=941)
        {const FString File=Run+TEXT("-")+Name;const bool Done=UI->CaptureUI(File,Width,Height);Check(TEXT("capture_")+Name,Done);if(Done)Captures.Add(MakeShared<FJsonValueString>(File+TEXT(".png")));};
        const FGeometry Geometry=FGeometry::MakeRoot(FVector2D(1672,941),FSlateLayoutTransform());const FModifierKeysState Modifiers;
        auto Key=[&](FKey K){UI->NativeOnKeyDown(Geometry,FKeyEvent(K,Modifiers,0,false,0,0));};
        auto Wheel=[&](float Delta){UI->NativeOnMouseWheel(Geometry,FPointerEvent(0,FVector2D(500,500),FVector2D(500,500),TSet<FKey>(),FKey(),Delta,Modifiers));};
        auto OneSelectedIcon=[&]()
        {
            const auto Object=Snapshot();int32 SelectedIcons=0,SelectedTabs=0;bool Matches=true;
            for(const auto& V:Object->GetArrayField(TEXT("elements")))
            {
                const auto E=V->AsObject();const FString Id=E->GetStringField(TEXT("id"));
                if(Id.StartsWith(TEXT("journal.header.icon.")))
                {SelectedIcons+=E->GetBoolField(TEXT("selected"));Matches&=E->GetBoolField(TEXT("selected"))==(Id==TEXT("journal.header.icon.")+UI->GetCategory());}
                if(Id.StartsWith(TEXT("journal.header.tab.")))
                {SelectedTabs+=E->GetBoolField(TEXT("selected"));Matches&=UI->IsActionHighlighted(TEXT("category:")+Id.Mid(19))==(Id==TEXT("journal.header.tab.")+UI->GetCategory());}
            }
            return Matches && SelectedIcons==1 && SelectedTabs==1;
        };
        Clean(TEXT("main"));Capture(TEXT("journal-main"));
        const auto Ember=HearthwardData::Find(TEXT("quests"),TEXT("ember"));
        Check(TEXT("original_quest_description_objective_and_progress"),Label(TEXT("journal.detail.description"))==HearthwardData::Text(Ember,TEXT("description")) && Label(TEXT("journal.detail.objective"))==HearthwardData::Text(Ember,TEXT("objective")) && Label(TEXT("journal.detail.progress"))==TEXT("进度 0 / 4"));
        const auto FutureRow=Find(Snapshot(),TEXT("journal.list.entry.store"));
        Check(TEXT("future_quest_name_hidden_and_not_clickable"),Label(TEXT("journal.list.name.store")).IsEmpty() && Label(TEXT("journal.list.status.store")).IsEmpty() && Label(TEXT("journal.list.mark.store")).IsEmpty() && FutureRow && FutureRow->GetStringField(TEXT("type"))==TEXT("menuRow") && !FutureRow->GetBoolField(TEXT("selected")) && UI->ActionAt({310,320}).IsEmpty());
        Check(TEXT("claim_rejects_incomplete_original_goal"),!UI->ExecuteAction(TEXT("claim")) && !G->Claimed.Contains(TEXT("ember")));
        Check(TEXT("undiscovered_map_rejected_in_place"),!UI->ExecuteAction(TEXT("questMap")) && UI->GetPage()==TEXT("journal") && Label(TEXT("journal.footer.help")).Contains(TEXT("尚未发现")));
        const FString BeforeSelection=G->SaveSnapshot();UI->ExecuteAction(TEXT("quest:store"));
        Check(TEXT("locked_selection_does_not_reveal_description"),Label(TEXT("journal.detail.name"))==HearthwardData::Text(Ember,TEXT("name")) && G->SaveSnapshot()==BeforeSelection);
        Check(TEXT("track_toggles_original_selection"),UI->ExecuteAction(TEXT("track")) && G->TrackedQuest.IsNone() && UI->ExecuteAction(TEXT("track")) && G->TrackedQuest==TEXT("ember"));
        const FString Categories[]={TEXT("main"),TEXT("side"),TEXT("world"),TEXT("people"),TEXT("factions"),TEXT("collection")};
        const FString Headings[]={TEXT("主线任务"),TEXT("支线任务"),TEXT("世界见闻"),TEXT("人物档案"),TEXT("势力阵营"),TEXT("收集要素")};
        for(int32 CategoryIndex=0;CategoryIndex<6;++CategoryIndex)
        {
            const FString Action=TEXT("category:")+Categories[CategoryIndex];
            Check(TEXT("original_symbol_hit_")+Categories[CategoryIndex],UI->ActionAt(FVector2D(592+CategoryIndex*100,55))==Action);
            UI->NativeOnMouseButtonDown(Geometry,FPointerEvent(0,FVector2D(592+CategoryIndex*100,55),FVector2D(592+CategoryIndex*100,55),TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,Modifiers));
            Check(TEXT("native_category_click_")+Categories[CategoryIndex],UI->GetCategory()==Categories[CategoryIndex] && Label(TEXT("journal.list.heading"))==Headings[CategoryIndex]);
            const FVector2D HoverOther(592+((CategoryIndex+1)%6)*100,55);
            UI->NativeOnMouseMove(Geometry,FPointerEvent(0,HoverOther,FVector2D(592+CategoryIndex*100,55),TSet<FKey>(),FKey(),0,Modifiers));
            bool SingleHighlight=OneSelectedIcon();
            for(int32 Tick=0;Tick<30;++Tick){UI->NativeTick(Geometry,.21f);SingleHighlight&=OneSelectedIcon();}
            Check(TEXT("only_selected_icon_and_tab_highlight_through_hover_refresh_")+Categories[CategoryIndex],SingleHighlight);
            if(CategoryIndex>=2)
            {
                const auto EmptyView=Snapshot();bool BlankRows=true;int32 SeparatorRows=0;
                for(const auto& V:EmptyView->GetArrayField(TEXT("elements")))
                {
                    const auto E=V->AsObject();const FString Id=E->GetStringField(TEXT("id"));
                    if(Id.StartsWith(TEXT("journal.list.entry.")))
                    {
                        ++SeparatorRows;BlankRows&=E->GetStringField(TEXT("type"))==TEXT("menuRow") && E->GetStringField(TEXT("text")).IsEmpty() && !E->GetBoolField(TEXT("selected")) && UI->ActionAt(FVector2D(E->GetNumberField(TEXT("x"))+40,E->GetNumberField(TEXT("y"))+20)).IsEmpty();
                    }
                    if(Id.StartsWith(TEXT("journal.list.name.")) || Id.StartsWith(TEXT("journal.list.status.")) || Id.StartsWith(TEXT("journal.list.mark.")))BlankRows=false;
                }
                Check(TEXT("unrecorded_name_and_description_hidden_")+Categories[CategoryIndex],BlankRows && SeparatorRows>0 && Label(TEXT("journal.detail.name")).IsEmpty() && !Find(EmptyView,TEXT("journal.detail.icon")) && Label(TEXT("journal.detail.description")).IsEmpty() && !Find(EmptyView,TEXT("journal.detail.empty")));
                const FString Before=G->SaveSnapshot();
                Check(TEXT("codex_cannot_trigger_stale_quest_actions_")+Categories[CategoryIndex],!UI->ExecuteAction(TEXT("track")) && !UI->ExecuteAction(TEXT("claim")) && !UI->ExecuteAction(TEXT("questMap")) && G->SaveSnapshot()==Before && UI->GetPage()==TEXT("journal"));
            }
            Clean(Categories[CategoryIndex]+TEXT("_unknown"));
        }
        Capture(TEXT("journal-collection-unknown"));
        Key(EKeys::E);Check(TEXT("native_e_cycles_to_main"),UI->GetCategory()==TEXT("main") && OneSelectedIcon());
        Key(EKeys::Q);Check(TEXT("native_q_cycles_to_collection"),UI->GetCategory()==TEXT("collection") && OneSelectedIcon());
        const FString FirstRange=Label(TEXT("journal.list.range"));Wheel(-1);
        Check(TEXT("wheel_scrolls_entries_without_changing_selection"),Label(TEXT("journal.list.range"))!=FirstRange && Label(TEXT("journal.detail.name")).IsEmpty());
        Key(EKeys::End);const FString LastRange=Label(TEXT("journal.list.range"));Wheel(-1);
        Check(TEXT("last_page_stops_and_has_no_next_target"),Label(TEXT("journal.list.range"))==LastRange && UI->ActionAt({600,820}).IsEmpty());
        Capture(TEXT("journal-collection-last"));Key(EKeys::Home);Wheel(1);
        Check(TEXT("first_page_stops_and_has_no_previous_target"),Label(TEXT("journal.list.range"))==FirstRange && UI->ActionAt({150,820}).IsEmpty());
        Key(EKeys::PageDown);Check(TEXT("native_page_down_browses_list"),Label(TEXT("journal.list.range"))!=FirstRange);Key(EKeys::PageUp);
        Check(TEXT("native_page_up_returns_first_window"),Label(TEXT("journal.list.range"))==FirstRange);
        UI->ExecuteAction(TEXT("ask:quit"));const FString ModalCategory=UI->GetCategory(),ModalRange=Label(TEXT("journal.list.range"));Key(EKeys::Q);Wheel(-1);
        Check(TEXT("modal_owns_category_paging_and_clicks"),!UI->ExecuteAction(TEXT("category:world")) && !UI->ExecuteAction(TEXT("journal.list.next")) && UI->GetCategory()==ModalCategory && Label(TEXT("journal.list.range"))==ModalRange && UI->ActionAt({590,55}).IsEmpty());UI->ExecuteAction(TEXT("cancel"));
        G->Discovered.Add(TEXT("camp"));G->Events.Add(TEXT("talk:brother"),1);G->Events.Add(TEXT("defeat:any"),1);G->Claimed.Add(TEXT("ember"));
        G->Events.Add(TEXT("collected:wood"),1);Check(TEXT("isolated_real_inventory_count"),Bag->TryAdd(TEXT("wood"),4)==EHearthwardInventoryResult::Success);
        for(int32 CategoryIndex=0;CategoryIndex<6;++CategoryIndex)
        {
            UI->ExecuteAction(TEXT("category:")+Categories[CategoryIndex]);
            if(CategoryIndex==0)UI->ExecuteAction(TEXT("quest:store"));
            if(CategoryIndex==5)UI->ExecuteAction(TEXT("codex:wood"));
            Check(TEXT("known_detail_shown_")+Categories[CategoryIndex],!Label(TEXT("journal.detail.name")).IsEmpty() && !Label(TEXT("journal.detail.description")).IsEmpty());
            Clean(Categories[CategoryIndex]+TEXT("_known"));Capture(TEXT("journal-")+Categories[CategoryIndex]+TEXT("-known"));
        }
        Check(TEXT("collection_uses_true_current_quantity"),Label(TEXT("journal.detail.held"))==TEXT("当前持有 4 件"));Bag->RestoreInventory(Empty);UI->Refresh();
        Check(TEXT("collection_history_survives_no_current_stock"),Label(TEXT("journal.detail.name"))==TEXT("木材") && Label(TEXT("journal.detail.held"))==TEXT("当前持有 0 件"));
        G->Events.Add(TEXT("collected:medicine"),1);UI->Refresh();
        const FString BeforeKeyboard=Label(TEXT("journal.detail.name"));Key(EKeys::Down);
        Check(TEXT("native_down_selects_next_record"),Label(TEXT("journal.detail.name"))==TEXT("药草膏") && Label(TEXT("journal.list.range"))!=FirstRange);Key(EKeys::Up);
        Check(TEXT("native_up_restores_previous_record"),Label(TEXT("journal.detail.name"))==BeforeKeyboard);
        Check(TEXT("settings_entry_preserved"),UI->ExecuteAction(TEXT("page:settings")) && UI->GetPage()==TEXT("settings"));
        Check(TEXT("settings_back_restores_journal_category"),UI->ExecuteAction(TEXT("back")) && UI->GetPage()==TEXT("journal") && UI->GetCategory()==TEXT("collection"));
        UI->ExecuteAction(TEXT("category:main"));UI->ExecuteAction(TEXT("quest:ember"));
        Check(TEXT("discovered_map_opens_original_location"),UI->ExecuteAction(TEXT("questMap")) && UI->GetPage()==TEXT("map"));
        Check(TEXT("map_back_restores_journal_and_selection"),UI->ExecuteAction(TEXT("back")) && UI->GetPage()==TEXT("journal") && Label(TEXT("journal.detail.name"))==HearthwardData::Text(Ember,TEXT("name")));
        G->Claimed.Remove(TEXT("ember"));Bag->TryAdd(TEXT("wood"),4);UI->Refresh();const int32 XPBefore=G->Experience;
        Check(TEXT("completed_goal_claims_once_and_opens_original_successor"),UI->ExecuteAction(TEXT("claim")) && G->Claimed.Contains(TEXT("ember")) && G->Experience>XPBefore && G->QuestAvailable(TEXT("store")));
        const int32 XPAfter=G->Experience;Check(TEXT("repeat_claim_cannot_grant_reward"),!UI->ExecuteAction(TEXT("claim")) && G->Experience==XPAfter);
        UI->ExecuteAction(TEXT("quest:store"));Settings->Comfort.TextScale=150;UI->Refresh();Clean(TEXT("text150"));Capture(TEXT("journal-text-150"));
        Capture(TEXT("journal-720p"),1280,720);Capture(TEXT("journal-16x10"),2560,1600);Capture(TEXT("journal-ultrawide"),2560,1080);
        UI->ExecuteAction(TEXT("category:collection"));Clean(TEXT("collection150"));Capture(TEXT("journal-collection-150"));
        Check(TEXT("large_text_stays_in_two_columns"),Find(Snapshot(),TEXT("journal.detail.surface"))->GetNumberField(TEXT("x"))==752 && Label(TEXT("journal.list.range")).StartsWith(TEXT("1 – 6 /")));
        Check(TEXT("back_returns_to_hud"),UI->ExecuteAction(TEXT("back")) && UI->GetPage()==TEXT("hud"));
        Settings->Comfort=ComfortBefore;Bag->RestoreInventory(BagBefore);G->Restore(GameplayBefore);G->Feedback=FeedbackBefore;
        Check(TEXT("gameplay_inventory_experience_and_receipts_restored"),G->SaveSnapshot()==GameplayBefore && Bag->DescribeInventory()==InventoryBefore);
        Check(TEXT("save_points_unchanged"),World->GetSubsystem<UHearthwardSaveSubsystem>()->GetPoints().Num()==SavesBefore);UI->OpenPage(TEXT("title"));
    }
    Report->SetBoolField(TEXT("passed"),Passed);Report->SetObjectField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("captures"),Captures);Report->SetArrayField(TEXT("layouts"),Layouts);
    FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("TitleWheel")/Run/TEXT("journal-preview.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
FAutoConsoleCommandWithWorld VerifyJournalPreviewCommand(TEXT("Hearthward.UI.VerifyJournalPreview"),TEXT("Verify journal presentation and original knowledge gates in an isolated PIE save pool."),FConsoleCommandWithWorldDelegate::CreateStatic(&VerifyJournalPreview));
}
#endif
