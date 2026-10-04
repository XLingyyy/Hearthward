from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[4]
def edit(name, before, after):
    p=ROOT/name;s=p.read_text(encoding='utf-8-sig');assert before in s,(name,before[:80]);p.write_text(s.replace(before,after,1),encoding='utf-8')

edit('Source/Hearthward/UI/HearthwardScreenHUDTest.inl', '''        for(int32 I=0;I<4;++I)
        {
            Check(FString::Printf(TEXT("wheel_selection_%d"),I),UI->GetHUDQuickSelection()==I);
            Layout(FString::Printf(TEXT("item_%d"),I)); Capture(FString::Printf(TEXT("hud-item-%d"),I),2560,1600); Wheel(-1);
        }
        Check(TEXT("wheel_forward_wrap"),UI->GetHUDQuickSelection()==0); Wheel(1);
        Check(TEXT("wheel_reverse_wrap"),UI->GetHUDQuickSelection()==3);''', '''        const FKey SlotKeys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};
        for(int32 I=0;I<4;++I)
        {
            if(I!=0)PhysicalKey(SlotKeys[I]);
            Check(FString::Printf(TEXT("shortcut_selection_%d"),I),UI->GetHUDQuickSelection()==I);
            Layout(FString::Printf(TEXT("item_%d"),I));Capture(FString::Printf(TEXT("hud-item-%d"),I),2560,1600);Wheel(-1);
            Check(FString::Printf(TEXT("wheel_does_not_change_slot_%d"),I),UI->GetHUDQuickSelection()==I);
        }
        Wheel(1);Check(TEXT("reverse_wheel_keeps_selection"),UI->GetHUDQuickSelection()==3);''')
edit('Source/Hearthward/UI/HearthwardScreenHUDTest.inl','TEXT("game_only_enhanced_input_wheel_down"),UI->GetHUDQuickSelection()==1','TEXT("game_only_enhanced_input_wheel_down_ignored"),UI->GetHUDQuickSelection()==0')
edit('Source/Hearthward/UI/HearthwardScreenHUDTest.inl','TEXT("inventory_item_action_updates_selection"),UI->GetHUDQuickSelection()==2','TEXT("inventory_use_keeps_current_selection"),UI->GetHUDQuickSelection()==3')
edit('Source/Hearthward/Tests/GameplayTests.cpp','    FHearthwardInventoryState Bag;', '''    Corrupt(TEXT("Wrong medicine quick role rejected"),[](auto J){J->SetArrayField(TEXT("quickItems"),{MakeShared<FJsonValueString>(TEXT("roast")),MakeShared<FJsonValueString>(TEXT("roast")),MakeShared<FJsonValueString>(TEXT("arrow")),MakeShared<FJsonValueString>(TEXT("firepot"))});});
    Corrupt(TEXT("Wrong food quick role rejected"),[](auto J){J->SetArrayField(TEXT("quickItems"),{MakeShared<FJsonValueString>(TEXT("medicine")),MakeShared<FJsonValueString>(TEXT("medicine")),MakeShared<FJsonValueString>(TEXT("arrow")),MakeShared<FJsonValueString>(TEXT("firepot"))});});
    Corrupt(TEXT("Malformed quick array rejected"),[](auto J){J->SetStringField(TEXT("quickItems"),TEXT("medicine"));});
    TSharedPtr<FJsonObject> Legacy;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Original),Legacy);
    Legacy->RemoveField(TEXT("quickItems"));FString LegacyJson;FJsonSerializer::Serialize(Legacy.ToSharedRef(),TJsonWriterFactory<>::Create(&LegacyJson));
    TestTrue(TEXT("Old snapshot without quick assignment remains valid"),UHearthwardGameplayComponent::ValidateSnapshot(LegacyJson));
    FHearthwardInventoryState Bag;''')
edit('Source/Hearthward/Tests/SurvivalTests.cpp', '''    TestTrue(TEXT("Equal sustained medicine may refresh"),S->BeginMedicine(TEXT("medicine")));
    S->CompleteBoundary(3);
    TestEqual(TEXT("Equal dose resets duration"),S->State.HotRemaining,15.);
    TestEqual(TEXT("Refresh does not instantly cash out old effect"),G->Health,10.f);
    Row->SetNumberField(TEXT("healing"),25); Bag->TryAdd(TEXT("medicine"),1);''', '''    TestFalse(TEXT("Active sustained medicine forbids equal repeated dose"),S->BeginMedicine(TEXT("medicine")));
    TestFalse(TEXT("Gameplay reports active medicine for repeated use"),G->UseItem(TEXT("medicine")));
    TestTrue(TEXT("Rejection names the active and attempted item"),G->Feedback==TEXT("当前正在使用药草膏，不能同时使用药草膏"));
    TestEqual(TEXT("Rejected dose preserves remaining effect"),S->State.HotRemaining,5.);
    TestEqual(TEXT("Repeated dose is not consumed"),Bag->GetItemCount(TEXT("medicine")),1);
    TestEqual(TEXT("Rejected dose does not cash out effect"),G->Health,10.f);
    Row->SetNumberField(TEXT("healing"),25);''')
edit('Source/Hearthward/Tests/SurvivalTests.cpp','    auto* Target=World->SpawnActor<AActor>();', '''    G->Hunger=20;Bag->TryAdd(TEXT("roast"),2);Bag->TryAdd(TEXT("arrow"),2);Bag->TryAdd(TEXT("firepot"),1);
    TestTrue(TEXT("Food slot accepts an actual food effect"),G->AssignQuickItem(1,TEXT("roast")));
    TestFalse(TEXT("Food cannot be placed in medicine slot"),G->AssignQuickItem(0,TEXT("roast")));
    TestFalse(TEXT("Medicine cannot be placed in food slot despite shared category"),G->AssignQuickItem(1,TEXT("medicine")));
    TestFalse(TEXT("Ammunition cannot be placed in food slot"),G->AssignQuickItem(1,TEXT("arrow")));
    TestTrue(TEXT("Manual food starts a three second reservation"),G->UseQuickItem(1) && S->State.FoodRemaining==3);
    TestEqual(TEXT("Starting food does not debit inventory"),Bag->GetItemCount(TEXT("roast")),2);
    TestEqual(TEXT("Starting food has not yet restored hunger"),G->Hunger,20.f);
    TestFalse(TEXT("Food prevents a duplicate food use"),G->UseQuickItem(1));
    TestFalse(TEXT("Food prevents medicine use"),G->UseQuickItem(0));
    TestFalse(TEXT("Food prevents throwing from the public item command"),G->UseQuickItem(3));
    TestFalse(TEXT("Food prevents direct ammunition use"),G->UseItem(TEXT("arrow")));
    TestTrue(TEXT("Rejection identifies active food and selected ammunition"),G->Feedback==TEXT("当前正在使用烤肉，不能同时使用箭矢"));
    S->CompleteBoundary(2.99);TestEqual(TEXT("Food remains reserved before completion"),Bag->GetItemCount(TEXT("roast")),2);
    S->CompleteBoundary(.01);TestEqual(TEXT("Completed food is charged once"),Bag->GetItemCount(TEXT("roast")),1);
    TestEqual(TEXT("Completed food restores actual data amount"),G->Hunger,60.f);
    S->CompleteBoundary(3);TestEqual(TEXT("Repeating settlement cannot double-charge"),Bag->GetItemCount(TEXT("roast")),1);
    TestTrue(TEXT("A new food action can begin after completion"),G->UseQuickItem(1));
    S->CancelAction();TestTrue(TEXT("Cancellation releases food without charge"),Bag->Available(TEXT("roast"))==1 && S->State.FoodItem.IsNone());
    TestTrue(TEXT("Food state restores its inventory reservation"),G->UseQuickItem(1));
    S->ResetTransient();TestEqual(TEXT("Restored food stays unavailable to recipes"),Bag->Available(TEXT("roast")),0);
    S->CancelAction();
    auto* Target=World->SpawnActor<AActor>();''')
print('Existing regressions updated and strengthened')
