// PROTOTYPE_ONLY: opt-in standalone fixture, always requiring an isolated GUID save pool.
#if !UE_BUILD_SHIPPING
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Gameplay/HearthwardGameData.h"

namespace
{
struct FQuickClientVerification
{
    FString Output;
    TSharedPtr<FJsonObject> Report=MakeShared<FJsonObject>(),Checks=MakeShared<FJsonObject>();
    bool Passed=true;
    int32 Step=0,FoodCount=0,MedicineCount=0,ArrowCount=0,ThrowCount=0;
    float Hunger=0,Health=0;
    double Started=FPlatformTime::Seconds(),WaitUntil=0,ActionClock=0,PausedRemaining=0;
    FGuid SaveId;
    void Check(const TCHAR* Name,bool Value)
    {Checks->SetBoolField(Name,Value);Passed&=Value;UE_LOG(LogTemp,Log,TEXT("Quick client check %s: %s"),Name,Value?TEXT("PASS"):TEXT("FAIL"));}
    bool Finish()
    {
        Report->SetBoolField(TEXT("passed"),Passed);Report->SetObjectField(TEXT("checks"),Checks);
        Report->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        Report->SetStringField(TEXT("scope"),TEXT("Real standalone viewport keys and mouse buttons, real save/load and inventory reservations. Controlled inventory, vitals and sustained-medicine fixture in a GUID-isolated pool."));
        FString Json;FJsonSerializer::Serialize(Report.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);return false;
    }
};
bool VerifyQuickClient(float)
{
    static FQuickClientVerification Run;
    if(Run.Output.IsEmpty())
    {
        FString Pool;FGuid Id;
        if(!FParse::Value(FCommandLine::Get(),TEXT("HearthwardQuickVerify="),Run.Output)
            || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id) || !Id.IsValid())return false;
    }
    const double Now=FPlatformTime::Seconds();
    if(Now-Run.Started>150) {Run.Check(TEXT("completion_within_timeout"),false);return Run.Finish();}
    if(Now<Run.WaitUntil || !GEngine)return true;
    UWorld* World=nullptr;
    for(const auto& Context:GEngine->GetWorldContexts()) if(Context.WorldType==EWorldType::Game && Context.World()) {World=Context.World();break;}
    if(!World || !World->HasBegunPlay() || !World->GetGameInstance())return true;
    auto* PC=World->GetFirstPlayerController();auto* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;auto* UI=HUD?HUD->Screen.Get():nullptr;
    if(!UI || World->GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>()->IsLoading() || (Run.Step<2 && UI->GetPage()!=TEXT("hud")))return true;
    auto* G=Pawn?Pawn->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    auto* Bag=Pawn?Pawn->FindComponentByClass<UHearthwardInventoryComponent>():nullptr;
    auto* S=Pawn?Pawn->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    auto* C=Pawn?Pawn->FindComponentByClass<UHearthwardCombatComponent>():nullptr;
    if(!G || !Bag || !S || !C)return true;
    auto* Viewport=World->GetGameInstance()->GetGameViewportClient();
    auto* Settings=World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
    auto* Saves=World->GetSubsystem<UHearthwardSaveSubsystem>();
    auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();const double Active=Clock->GetSnapshot().ActivePlaySeconds;
    const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};
    auto Key=[&](FKey Value,EInputEvent Event)
    {
        Viewport->InputKey(FInputKeyEventArgs(Viewport->Viewport,FInputDeviceId::CreateFromInternalId(0),Value,Event,Event==IE_Released?0.f:1.f,false,FPlatformTime::Cycles64()));
        PC->PlayerTick(.02f);
    };
    auto Press=[&](FKey Value){Key(Value,IE_Released);Key(Value,IE_Pressed);Key(Value,IE_Released);};
    auto Select=[&](int32 Slot){if(UI->GetHUDQuickSelection()!=Slot)Press(Keys[Slot]);};
    auto Snapshot=[&]() {TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),J);return J;};
    auto Find=[&](const FString& Id)->TSharedPtr<FJsonObject>
    {
        const auto J=Snapshot();if(!J)return nullptr;
        for(const auto& E:J->GetArrayField(TEXT("elements"))) if(E->AsObject()->GetStringField(TEXT("id"))==Id && E->AsObject()->GetBoolField(TEXT("visible")))return E->AsObject();return nullptr;
    };
    auto CheckLabels=[&]()
    {
        const TCHAR* Roles[]={TEXT("medicine"),TEXT("roast"),TEXT("arrow"),TEXT("firepot")};bool Correct=true;
        for(int32 Slot=0;Slot<4;++Slot)
        {
            const FString Id=FString(TEXT("hud.quick."))+Roles[Slot];const auto Cell=Find(Id),Label=Find(Id+TEXT(".key"));
            Correct&=Cell && Label && Label->GetStringField(TEXT("text"))==FString::FromInt(Slot+1)
                && Label->GetNumberField(TEXT("x"))>Cell->GetNumberField(TEXT("x"))
                && Label->GetNumberField(TEXT("y"))>=Cell->GetNumberField(TEXT("y"))
                && Label->GetNumberField(TEXT("y"))<Cell->GetNumberField(TEXT("y"))+15;
        }
        return Correct;
    };
    auto Capture=[&](const TCHAR* Name)
    {
        const auto Widget=Viewport->GetGameViewportWidget();TArray<FColor> Pixels;FIntVector Size=FIntVector::ZeroValue;
        if(!Widget || !FSlateApplication::Get().TakeScreenshot(Widget.ToSharedRef(),Pixels,Size)) {Run.Check(TEXT("screenshot_capture"),false);return;}
        TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
        FFileHelper::SaveArrayToFile(PNG,*(FPaths::GetPath(Run.Output)/Name+TEXT(".png")));
    };
    auto ActiveError=[&](FName Item)
    {
        const FString Current=HearthwardData::Text(HearthwardData::Find(TEXT("items"),S->ActiveConsumable().ToString()),TEXT("name"));
        const FString Attempt=HearthwardData::Text(HearthwardData::Find(TEXT("items"),Item.ToString()),TEXT("name"));
        return !Current.IsEmpty() && G->Feedback==FString::Printf(TEXT("当前正在使用%s，不能同时使用%s"),*Current,*Attempt);
    };
    switch(Run.Step)
    {
    case 0:
        ++Run.Step;Run.WaitUntil=Now+2;break;
    case 1:
    {
        C->Cancel();S->State={};S->ResetTransient();G->Health=20;G->Hunger=20;G->Stamina=G->MaxStamina();
        const TPair<FName,int32> Items[]={{TEXT("medicine"),4},{TEXT("medicine_half"),2},{TEXT("roast"),4},{TEXT("meat"),2},{TEXT("arrow"),30},{TEXT("firepot"),3},{TEXT("bow"),1},{TEXT("axe"),1}};
        bool Supplies=true;for(const auto& I:Items) if(Bag->GetItemCount(I.Key)<I.Value)Supplies&=Bag->TryAdd(I.Key,I.Value-Bag->GetItemCount(I.Key))==EHearthwardInventoryResult::Success;
        Run.Check(TEXT("fixture_supplies_from_real_inventory"),Supplies);
        if(!G->Equipment.Contains(TEXT("weapon")))G->CommitEquipment(TEXT("axe"));
        if(!G->Equipment.Contains(TEXT("ranged")))G->CommitEquipment(TEXT("bow"));
        Run.Check(TEXT("bow_uses_real_equipped_instance"),C->SupportsAmmo(TEXT("arrow")));
        Run.Check(TEXT("wrong_medicine_assignment_rejected"),!G->AssignQuickItem(0,TEXT("roast")) && G->QuickItem(0)==TEXT("medicine"));
        Run.Check(TEXT("wrong_food_assignment_rejected"),!G->AssignQuickItem(1,TEXT("medicine")) && G->QuickItem(1)==TEXT("roast"));
        Run.Check(TEXT("wrong_tool_assignment_rejected"),!G->AssignQuickItem(1,TEXT("arrow")) && G->Feedback.Contains(TEXT("无法放置到食物栏")));
        UI->OpenPage(TEXT("inventory"));UI->ExecuteAction(TEXT("filter:食物"));UI->ExecuteAction(TEXT("item:medicine_half"));
        Run.Check(TEXT("backpack_can_assign_actual_medicine"),UI->ExecuteAction(TEXT("quick.assign:0")) && G->QuickItem(0)==TEXT("medicine_half"));
        UI->ExecuteAction(TEXT("quick.assign:1"));Run.Check(TEXT("backpack_rejects_medicine_in_food"),G->QuickItem(1)==TEXT("roast") && G->Feedback.Contains(TEXT("无法放置到食物栏")));
        UI->ExecuteAction(TEXT("item:meat"));Run.Check(TEXT("backpack_can_assign_food_effect"),UI->ExecuteAction(TEXT("quick.assign:1")) && G->QuickItem(1)==TEXT("meat"));
        UI->ExecuteAction(TEXT("item:roast"));UI->ExecuteAction(TEXT("quick.assign:1"));
        UI->OpenPage(TEXT("hud"));
        Run.Check(TEXT("default_number_labels_at_top_left"),CheckLabels());
        const FString Before=Bag->DescribeInventory();
        for(int32 Slot=1;Slot<4;++Slot) {Press(Keys[Slot]);Run.Check(*FString::Printf(TEXT("key_%d_selects_role"),Slot+1),UI->GetHUDQuickSelection()==Slot && !S->Busy() && !C->Busy());}
        Run.Check(TEXT("selection_only_preserves_inventory"),Bag->DescribeInventory()==Before);
        Press(EKeys::MouseScrollDown);Press(EKeys::MouseScrollUp);
        const FGeometry Geometry=FGeometry::MakeRoot(FVector2D(1600,1000),FSlateLayoutTransform());
        const FModifierKeysState Modifiers;
        UI->NativeOnMouseWheel(Geometry,FPointerEvent(0,FVector2D(400,400),FVector2D(400,400),TSet<FKey>(),FKey(),-1,Modifiers));
        Run.Check(TEXT("both_wheel_paths_leave_slot_unchanged"),UI->GetHUDQuickSelection()==3 && !UI->ExecuteAction(TEXT("hud.quick.next")) && !UI->ExecuteAction(TEXT("hud.quick.prev")));
        Press(EKeys::Three);const int32 Arrows=Bag->GetItemCount(TEXT("arrow"));Press(EKeys::Three);
        Run.Check(TEXT("repeated_three_does_not_use_arrow"),Bag->GetItemCount(TEXT("arrow"))==Arrows && !C->Busy());
        Run.Check(TEXT("direct_arrow_use_requires_weapon_attack"),!G->UseItem(TEXT("arrow")) && Bag->GetItemCount(TEXT("arrow"))==Arrows);
        UI->Refresh();
        ++Run.Step;break;
    }
    case 2:
        Run.FoodCount=Bag->GetItemCount(TEXT("roast"));Run.Hunger=G->Hunger;Select(1);
        Run.Check(TEXT("first_two_selects_without_eating"),UI->GetHUDQuickSelection()==1 && !S->Busy());
        Press(EKeys::Two);Run.Check(TEXT("second_two_starts_three_second_food"),S->State.FoodItem==TEXT("roast") && S->State.FoodRemaining==3);
        Run.Check(TEXT("food_start_reserves_without_debit"),Bag->GetItemCount(TEXT("roast"))==Run.FoodCount && Bag->Available(TEXT("roast"))==Run.FoodCount-1);
        Press(EKeys::Two);Run.Check(TEXT("same_food_key_is_blocked"),ActiveError(TEXT("roast")));
        Press(EKeys::One);Run.Check(TEXT("one_selects_without_interrupting_food"),UI->GetHUDQuickSelection()==0 && S->State.FoodItem==TEXT("roast"));
        Press(EKeys::One);Run.Check(TEXT("medicine_key_is_blocked_during_food"),ActiveError(G->QuickItem(0)) && S->State.Medicine.IsNone());
        Press(EKeys::Four);Press(EKeys::Four);Run.Check(TEXT("throw_key_is_blocked_during_food"),ActiveError(TEXT("firepot")) && !C->Busy());
        Press(EKeys::LeftMouseButton);Run.Check(TEXT("throw_left_click_is_blocked_during_food"),ActiveError(TEXT("firepot")) && !C->Busy());
        Press(EKeys::Three);Key(EKeys::LeftMouseButton,IE_Pressed);Key(EKeys::LeftMouseButton,IE_Released);
        Run.Check(TEXT("bow_left_click_is_blocked_during_food"),ActiveError(TEXT("arrow")) && !C->Busy());
        UI->OpenPage(TEXT("inventory"));UI->ExecuteAction(TEXT("filter:食物"));UI->ExecuteAction(TEXT("item:medicine_half"));UI->ExecuteAction(TEXT("use"));
        Run.Check(TEXT("backpack_use_cannot_bypass_food_lock"),S->State.FoodItem==TEXT("roast") && S->State.Medicine.IsNone() && ActiveError(TEXT("medicine_half")));
        UI->Refresh();
        Run.Check(TEXT("active_food_and_assignments_can_be_saved"),Saves->SavePoint(true));Run.Report->SetStringField(TEXT("save_status"),Saves->GetStatus());
        if(Saves->GetPoints().Num()>0)Run.SaveId=Saves->GetPoints().Last().SaveId;
        S->CancelAction();G->AssignQuickItem(1,TEXT("meat"));
        Run.Check(TEXT("actual_save_load_restores_food_and_slots"),Run.SaveId.IsValid() && Saves->LoadPoint(Run.SaveId) && G->QuickItem(0)==TEXT("medicine_half") && G->QuickItem(1)==TEXT("roast") && S->State.FoodItem==TEXT("roast"));
        Run.Check(TEXT("restored_food_is_still_reserved_once"),Bag->GetItemCount(TEXT("roast"))==Run.FoodCount && Bag->Available(TEXT("roast"))==Run.FoodCount-1);
        Run.ActionClock=Clock->GetSnapshot().ActivePlaySeconds;++Run.Step;break;
    case 3:
    {
        if(Active-Run.ActionClock<1)return true;
        Run.Check(TEXT("food_not_consumed_before_three_seconds"),S->State.FoodRemaining>1 && Bag->GetItemCount(TEXT("roast"))==Run.FoodCount);
        G->CanUseItem(TEXT("medicine_half"));UI->Refresh();
        const auto Notice=Find(TEXT("hud.gameplay.feedback"));Run.Check(TEXT("item_lock_message_is_rendered_on_hud"),Notice && Notice->GetStringField(TEXT("text"))==G->Feedback);
        Capture(TEXT("food-use-conflict"));
        Run.PausedRemaining=S->State.FoodRemaining;UI->OpenPage(TEXT("pause"));
        ++Run.Step;Run.WaitUntil=Now+.7;break;
    }
    case 4:
        Run.Check(TEXT("pause_preserves_food_timer"),World->IsPaused() && S->State.FoodRemaining==Run.PausedRemaining);
        UI->OpenPage(TEXT("hud"));++Run.Step;break;
    case 5:
        if(Active-Run.ActionClock<3.2)return true;
        Run.Check(TEXT("food_finishes_and_debits_once"),S->State.FoodItem.IsNone() && Bag->GetItemCount(TEXT("roast"))==Run.FoodCount-1);
        Run.Check(TEXT("food_uses_real_nutrition_amount"),G->Hunger>=Run.Hunger+39.5f);
        Run.Check(TEXT("consume_event_recorded_once"),G->Events.FindRef(TEXT("consume:roast"))==1);
        Select(3);Select(0);Run.MedicineCount=Bag->GetItemCount(TEXT("medicine_half"));Run.Health=G->Health;
        Run.Check(TEXT("first_one_selects_without_using"),UI->GetHUDQuickSelection()==0 && !S->Busy());
        Press(EKeys::One);Run.Check(TEXT("second_one_starts_medicine"),S->State.Medicine==TEXT("medicine_half") && S->State.MedicineRemaining==3);
        Press(EKeys::One);Run.Check(TEXT("same_medicine_is_blocked"),ActiveError(TEXT("medicine_half")));
        Select(1);Press(EKeys::Two);Run.Check(TEXT("food_blocked_during_medicine"),ActiveError(TEXT("roast")) && S->State.FoodItem.IsNone());
        Select(3);Press(EKeys::LeftMouseButton);Run.Check(TEXT("throw_left_click_blocked_during_medicine"),ActiveError(TEXT("firepot")) && !C->Busy());
        Run.Check(TEXT("direct_shoot_cannot_bypass_medicine"),!C->Shoot(false) && ActiveError(TEXT("arrow")));
        Run.ActionClock=Active;++Run.Step;break;
    case 6:
        if(Active-Run.ActionClock<3.2)return true;
        Run.Check(TEXT("medicine_finishes_and_debits_once"),S->State.Medicine.IsNone() && Bag->GetItemCount(TEXT("medicine_half"))==Run.MedicineCount-1);
        Run.Check(TEXT("medicine_applies_data_healing"),G->Health>=Run.Health+G->MaxHealth()*.15f);
        S->State.HotItem=TEXT("medicine");S->State.HotRate=1;S->State.HotRemaining=1.5;
        Run.Check(TEXT("sustained_effect_blocks_food"),!G->UseQuickItem(1) && ActiveError(TEXT("roast")));
        Run.Check(TEXT("sustained_effect_blocks_repeat_medicine"),!G->UseItem(TEXT("medicine")) && ActiveError(TEXT("medicine")));
        Run.Check(TEXT("sustained_effect_blocks_throw"),!C->Throw(TEXT("firepot")) && ActiveError(TEXT("firepot")));
        Run.Check(TEXT("sustained_effect_blocks_shoot"),!C->Shoot(false) && ActiveError(TEXT("arrow")));
        ++Run.Step;Run.WaitUntil=Now+1.8;break;
    case 7:
        Run.Check(TEXT("expired_effect_releases_item_lock"),S->State.HotRemaining==0 && S->State.HotItem.IsNone() && G->CanUseItem(TEXT("roast")));
        C->Cancel();C->Aim(false);Select(0);G->Stamina=G->MaxStamina();Press(EKeys::LeftMouseButton);
        Run.Check(TEXT("medicine_slot_left_click_is_melee_with_bow_equipped"),C->Action==TEXT("attack") && S->State.Medicine.IsNone());C->Cancel();
        Select(1);G->Stamina=G->MaxStamina();Press(EKeys::LeftMouseButton);
        Run.Check(TEXT("food_slot_left_click_is_melee_with_bow_equipped"),C->Action==TEXT("attack") && S->State.FoodItem.IsNone());C->Cancel();
        G->CommitEquipment(TEXT("bow"));Select(2);G->Stamina=G->MaxStamina();Press(EKeys::LeftMouseButton);
        Run.Check(TEXT("arrow_slot_without_bow_falls_back_to_melee"),!C->SupportsAmmo(TEXT("arrow")) && C->Action==TEXT("attack"));C->Cancel();
        G->CommitEquipment(TEXT("axe"));
        for(int32 Slot=0;Slot<3;++Slot)
        {
            Select(Slot);Press(EKeys::LeftMouseButton);
            Run.Check(*FString::Printf(TEXT("unarmed_slot_%d_reports_melee_requirement"),Slot+1),C->Feedback==TEXT("需要可用近战武器") && !C->Busy());
        }
        G->CommitEquipment(TEXT("axe"));G->CommitEquipment(TEXT("bow"));G->Stamina=G->MaxStamina();
        Select(3);Run.ThrowCount=Bag->GetItemCount(TEXT("firepot"));Press(EKeys::LeftMouseButton);
        Run.Check(TEXT("throw_slot_left_click_starts_throw"),C->Action==TEXT("throw"));
        ++Run.Step;Run.WaitUntil=Now+1.1;break;
    case 8:
        Run.Check(TEXT("left_click_throw_debits_one"),Bag->GetItemCount(TEXT("firepot"))==Run.ThrowCount-1 && !C->Busy());
        Press(EKeys::Four);Run.Check(TEXT("repeated_four_starts_throw"),C->Action==TEXT("throw"));
        ++Run.Step;Run.WaitUntil=Now+1.1;break;
    case 9:
        Run.Check(TEXT("second_throw_debits_one"),Bag->GetItemCount(TEXT("firepot"))==Run.ThrowCount-2 && !C->Busy());
        Select(2);Run.ArrowCount=Bag->GetItemCount(TEXT("arrow"));Key(EKeys::LeftMouseButton,IE_Pressed);
        Run.Check(TEXT("equipped_bow_and_arrow_slot_starts_draw"),C->Action==TEXT("draw") && C->Aiming);
        ++Run.Step;Run.WaitUntil=Now+.4;break;
    case 10:
        UI->Refresh();Capture(TEXT("bow-drawing"));Key(EKeys::LeftMouseButton,IE_Released);
        Run.Check(TEXT("releasing_bow_fires_one_arrow"),Bag->GetItemCount(TEXT("arrow"))==Run.ArrowCount-1);
        ++Run.Step;Run.WaitUntil=Now+.6;break;
    case 11:
        Press(EKeys::Three);Press(EKeys::Three);Run.Check(TEXT("three_still_only_selects_after_shot"),Bag->GetItemCount(TEXT("arrow"))==Run.ArrowCount-1 && !C->Busy());
        Key(EKeys::LeftMouseButton,IE_Pressed);Run.Check(TEXT("second_bow_draw_starts"),C->Action==TEXT("draw"));
        ++Run.Step;Run.WaitUntil=Now+.4;break;
    case 12:
        Press(EKeys::Two);Run.Check(TEXT("slot_change_cancels_bow_draw"),UI->GetHUDQuickSelection()==1 && !C->Busy() && !C->Aiming);
        Key(EKeys::LeftMouseButton,IE_Released);Run.Check(TEXT("slot_change_then_release_does_not_fire"),Bag->GetItemCount(TEXT("arrow"))==Run.ArrowCount-1);
        Settings->Comfort.TextScale=150;UI->Refresh();Run.Check(TEXT("large_text_keeps_all_four_shortcuts"),CheckLabels());
        ++Run.Step;Run.WaitUntil=Now+.2;break;
    case 13:
        Capture(TEXT("shortcuts-large-text"));Settings->Comfort.TextScale=100;UI->Refresh();
        ++Run.Step;Run.WaitUntil=Now+.2;break;
    case 14:
        Capture(TEXT("four-shortcut-labels"));UI->OpenPage(TEXT("inventory"));UI->ExecuteAction(TEXT("filter:食物"));UI->ExecuteAction(TEXT("item:medicine_half"));Settings->Comfort.TextScale=150;UI->Refresh();
        ++Run.Step;Run.WaitUntil=Now+.2;break;
    case 15:
        Capture(TEXT("backpack-large-text"));Settings->Comfort.TextScale=100;UI->Refresh();
        ++Run.Step;Run.WaitUntil=Now+.2;break;
    case 16:
        Capture(TEXT("backpack-placement"));return Run.Finish();
    }
    return true;
}
const FTSTicker::FDelegateHandle QuickClientVerificationTicker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&VerifyQuickClient),.01f);
}
#endif
