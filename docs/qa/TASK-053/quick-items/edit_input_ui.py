from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]

def edit(name, changes):
    p = ROOT / name
    text = p.read_text(encoding='utf-8-sig')
    for before, after in changes:
        assert before in text, (name, before[:100])
        text = text.replace(before, after, 1)
    p.write_text(text, encoding='utf-8')



edit('Source/Hearthward/Gameplay/HearthwardGameplayComponent.cpp', [
    ('    const FString ActiveName=Text(Find(TEXT("items"),Active.ToString()),TEXT("name"),TEXT("药品"));\n    const FString ItemName=Text(Find(TEXT("items"),Item.ToString()),TEXT("name"),TEXT("选中道具"));',
     '    FString ActiveName=Text(Find(TEXT("items"),Active.ToString()),TEXT("name"));\n    FString ItemName=Text(Find(TEXT("items"),Item.ToString()),TEXT("name"));\n    if(ActiveName.IsEmpty()) ActiveName=TEXT("药品");\n    if(ItemName.IsEmpty()) ItemName=TEXT("选中道具");'),
])
edit('Source/Hearthward/Input/HearthwardInputBindings.cpp', [
    ('Add(TEXT("combat.throw"),TEXT("快捷投掷物"),EKeys::G,TEXT("hud"),TEXT(""),false,EKeys::Four);', 'Add(TEXT("combat.throw"),TEXT("投掷栏：选择／再次使用"),EKeys::Four,TEXT("hud"),TEXT(""),false);\n        Add(TEXT("combat.ammunition"),TEXT("弓箭栏：选择"),EKeys::Three,TEXT("hud"),TEXT(""),false);'),
    ('TEXT("快捷药品")', 'TEXT("药品栏：选择／再次使用")'),
    ('TEXT("快捷食物")', 'TEXT("食物栏：选择／再次使用")'),
    ('    return Validate(Result).IsEmpty()?Result:Defaults();', '''    // Upgrade only the previous stock G / 4 pair; preserve the player's custom bindings.
    auto& Throw=Result.FindChecked(TEXT("combat.throw"));
    if(Throw[0].Key==EKeys::G && !Throw[0].Modifier.IsValid() && Throw[1].Key==EKeys::Four && !Throw[1].Modifier.IsValid())
        Throw={FHearthwardKeyBinding{EKeys::Four,FKey()},FHearthwardKeyBinding{}};
    return Validate(Result).IsEmpty()?Result:Defaults();'''),
])
edit('Source/Hearthward/Combat/HearthwardCombatComponent.h', [
    ('    UFUNCTION(BlueprintCallable) bool Shoot(bool Release=false);', '    UFUNCTION(BlueprintCallable) bool Shoot(bool Release=false);\n    bool SupportsAmmo(FName Item) const;'),
])
edit('Source/Hearthward/Combat/HearthwardCombatComponent.cpp', [
    ('    const auto* I=Bag()->FindInstance(Instance);if(!I)return false;\n    const auto R=', '    const auto* I=Bag()->FindInstance(Instance);if(!I || !G()->CanUseItem(I->Definition))return false;\n    const auto R='),
    ('    return Text(R,TEXT("combatClass"))==TEXT("crossbow")', '    return G()->CanUseItem(TEXT("arrow")) && Text(R,TEXT("combatClass"))==TEXT("crossbow")'),
    ('bool UHearthwardCombatComponent::Shoot(bool Release)\n{', '''bool UHearthwardCombatComponent::SupportsAmmo(FName Item) const
{
    if(Item!=TEXT("arrow") || !Bag()->EquippedInstance(TEXT("ranged")).IsValid() || G()->EquippedDurability(TEXT("ranged"))<=0) return false;
    const FString Kind=Text(Find(TEXT("items"),G()->Equipment.FindRef(TEXT("ranged")).ToString()),TEXT("combatClass"));
    return Kind==TEXT("bow") || Kind==TEXT("crossbow");
}
bool UHearthwardCombatComponent::Shoot(bool Release)
{
    if(!G()->CanUseItem(TEXT("arrow"))) return false;
    if(!SupportsAmmo(TEXT("arrow"))) {SetFeedback(TEXT("箭矢需要装备可用弓"));return false;}
    if(Bag()->Available(TEXT("arrow"))<=0) {SetFeedback(TEXT("没有可用箭矢"));return false;}'''),
    ('bool UHearthwardCombatComponent::Throw(FName Item)\n{', 'bool UHearthwardCombatComponent::Throw(FName Item)\n{\n    if(!G()->CanUseItem(Item)) return false;'),
])
edit('Source/Hearthward/HearthwardCharacter.cpp', [
    ('    PhysicalAction(EKeys::MouseScrollUp); PhysicalAction(EKeys::MouseScrollDown);\n', ''),
    ('''    if(Key==EKeys::MouseScrollUp || Key==EKeys::MouseScrollDown)
    {
        if(HUD && HUD->Screen) HUD->Screen->ExecuteAction(Key==EKeys::MouseScrollUp?TEXT("hud.quick.prev"):TEXT("hud.quick.next"));
        return;
    }
''', ''),
    ('    auto* C=FindComponentByClass<UHearthwardCombatComponent>();\n    if(FindComponentByClass<UHearthwardTraversalComponent>()->IsInWater()) return;', '''    if(Is(TEXT("combat.throw"))) {CombatThrow();return;}
    if(Is(TEXT("survival.medicine"))) {Screen(TEXT("quick:0"));return;}
    if(Is(TEXT("survival.food"))) {Screen(TEXT("quick:1"));return;}
    if(Is(TEXT("combat.ammunition"))) {Screen(TEXT("quick:2"));return;}
    auto* C=FindComponentByClass<UHearthwardCombatComponent>();
    if(FindComponentByClass<UHearthwardTraversalComponent>()->IsInWater()) return;'''),
    ('''    if(Is(TEXT("combat.throw"))) {CombatThrow();return;}
    if(Is(TEXT("survival.medicine"))) {Screen(TEXT("quick:0"));return;}
    if(Is(TEXT("survival.food"))) {Screen(TEXT("quick:1"));return;}
    if(Is(TEXT("companion.wait")))''', '    if(Is(TEXT("companion.wait")))'),
    ('''    if(Combat->Aiming)
    {
        if(auto* PC=Cast<APlayerController>(GetController()))
            if(auto* HUD=Cast<AHearthwardHUD>(PC->GetHUD());HUD && HUD->Screen) HUD->Screen->ExecuteAction(TEXT("hud.quick.select:2"));
        Combat->Shoot(false); return;
    }
    Combat->Attack''', '''    auto* PC=Cast<APlayerController>(GetController());
    const auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;
    const int32 Slot=HUD && HUD->Screen?HUD->Screen->GetHUDQuickSelection():0;
    if(Slot==3) { Gameplay->UseQuickItem(3); return; }
    if(Slot==2 && Combat->SupportsAmmo(Gameplay->QuickItem(2)))
    {
        Combat->Aim(true); Combat->Shoot(false); return;
    }
    Combat->Aim(false);
    Combat->Attack'''),
    ('    if(C->RangedSelected()) C->Aim(Comfort.AimToggle?!C->Aiming:true); else C->SetGuard(Comfort.GuardToggle?!C->Guarding():true);', '''    const auto* PC=Cast<APlayerController>(GetController()); const auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;
    if(HUD && HUD->Screen && HUD->Screen->GetHUDQuickSelection()==2 && C->SupportsAmmo(Gameplay->QuickItem(2)))
        C->Aim(Comfort.AimToggle?!C->Aiming:true);
    else {C->Aim(false);C->SetGuard(Comfort.GuardToggle?!C->Guarding():true);}'''),
    ('void AHearthwardCharacter::ReleaseAttack() { auto* C=FindComponentByClass<UHearthwardCombatComponent>(); if(C->Aiming && !FindComponentByClass<UHearthwardTraversalComponent>()->IsInWater()) C->Shoot(true); }', '''void AHearthwardCharacter::ReleaseAttack()
{
    auto* C=FindComponentByClass<UHearthwardCombatComponent>();
    const auto* PC=Cast<APlayerController>(GetController()); const auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;
    if(HUD && HUD->Screen && HUD->Screen->GetHUDQuickSelection()==2 && C->Aiming && C->SupportsAmmo(Gameplay->QuickItem(2)) && !FindComponentByClass<UHearthwardTraversalComponent>()->IsInWater()) C->Shoot(true);
}'''),
])
edit('Source/Hearthward/UI/HearthwardScreenActions.cpp', [
    ('        if(Action==TEXT("hud.quick.next")) Selection=(Selection+1)%Count;\n        else if(Action==TEXT("hud.quick.prev")) Selection=(Selection+Count-1)%Count;\n        else if(Action.StartsWith(TEXT("hud.quick.select:")))', '        if(Action.StartsWith(TEXT("hud.quick.select:")))'),
    ('        // A wheel choice belongs to the HUD; it does not use an item or alter a save.\n        HUDQuickSelection=Selection;', '        // Selection is UI state. A different slot also cancels a pending bow release.\n        if(Selection!=HUDQuickSelection) if(auto* C=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardCombatComponent>()) C->Aim(false);\n        HUDQuickSelection=Selection;'),
    ('''        const auto& Slots=Theme->GetObjectField(TEXT("inventory"))->GetArrayField(TEXT("quickSlots"));
        const int32 QuickIndex=Slots.IndexOfByPredicate([&](const auto& Slot){return FName(*Slot->AsString())==UsedItem;});
        if(QuickIndex!=INDEX_NONE) HUDQuickSelection=QuickIndex;
''', ''),
    ('if(Number(Item,TEXT("healing"))>0 || !Text(Item,TEXT("slot")).IsEmpty() || Number(Item,TEXT("throwDamage"))>0) OpenPage', 'if(Number(Item,TEXT("healing"))>0 || Number(Item,TEXT("food"))>0 || !Text(Item,TEXT("slot")).IsEmpty() || Number(Item,TEXT("throwDamage"))>0) OpenPage'),
    ('''        const auto& Slots=Theme->GetObjectField(TEXT("inventory"))->GetArrayField(TEXT("quickSlots"));
        const int32 SlotIndex=FCString::Atoi(*Action.Mid(6));
        if(Slots.IsValidIndex(SlotIndex)) HUDQuickSelection=SlotIndex;
        Success=Slots.IsValidIndex(SlotIndex) && G->UseItem(FName(*Slots[SlotIndex]->AsString())); Message=G->Feedback;''', '''        if(Page!=TEXT("hud")) return false;
        const int32 SlotIndex=FCString::Atoi(*Action.Mid(6));
        if(SlotIndex<0 || SlotIndex>=4) return false;
        if(HUDQuickSelection!=SlotIndex)
        {
            if(auto* C=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardCombatComponent>()) C->Aim(false);
            HUDQuickSelection=SlotIndex;
        }
        else if(SlotIndex!=2) {Success=G->UseQuickItem(SlotIndex);Message=G->Feedback;}
    }
    else if(Action.StartsWith(TEXT("quick.assign:")))
    {
        if(Page!=TEXT("inventory")) return false;
        Success=G->AssignQuickItem(FCString::Atoi(*Action.Mid(13)),SelectedItem);Message=G->Feedback;'''),
])
edit('Source/Hearthward/UI/HearthwardScreenWidget.cpp', [
    ('        if(!FMath::IsNearlyZero(E.GetWheelDelta())) ExecuteAction(E.GetWheelDelta()>0?TEXT("hud.quick.prev"):TEXT("hud.quick.next"));\n', ''),
])
edit('Source/Hearthward/UI/HearthwardScreenContent.cpp', [
    ('const FString ItemId=Slots[Index]->AsString();', 'const FString RoleId=Slots[Index]->AsString(),ItemId=G->QuickItem(Index).ToString();'),
    ('20,TEXT("hud.quick.")+ItemId,TEXT("hud.quickslots"));', '20,TEXT("hud.quick.")+RoleId,TEXT("hud.quickslots"));'),
    ('        if(Selected) HUD(TEXT("text"),Text(Item,TEXT("name")),', '''        const FName Actions[]={TEXT("survival.medicine"),TEXT("survival.food"),TEXT("combat.ammunition"),TEXT("combat.throw")};
        const auto* Settings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
        const auto& Keys=Settings->Bindings.FindChecked(Actions[Index]);
        const FString Key=Keys[0].Key.IsValid()?Keys[0].Label():(Keys[1].Key.IsValid()?Keys[1].Label():TEXT("—"));
        auto& Shortcut=HUD(TEXT("text"),Key,Positions[Offset]+FVector2D(6,3),{50,28},18,TEXT("hud.quick.")+RoleId+TEXT(".key"),TEXT("hud.quickslots"));
        Shortcut.Color=Selected?Color(TEXT("gold")):FLinearColor(.78f,.74f,.64f,1.f);
        if(Selected) HUD(TEXT("text"),Text(Item,TEXT("name")),'''),
])
edit('Source/Hearthward/UI/HearthwardScreenMenu.cpp', [
    ('    Rule(40,854,1592,TEXT("inventory.footer.rule"),TEXT("inventory.footer"));', '''    // Dedicated fixed-role placement commands validate the selected item's effect in Gameplay.
    auto& MedicineSlot=Inv(TEXT("menuAction"),TEXT("放入药品栏  1"),{624,810},{248,32},16,TEXT("inventory.quick.medicine"),TEXT("inventory.detail"),TEXT("quick.assign:0"));MedicineSlot.Enabled=Selected.IsValid();
    auto& FoodSlot=Inv(TEXT("menuAction"),TEXT("放入食物栏  2"),{894,810},{248,32},16,TEXT("inventory.quick.food"),TEXT("inventory.detail"),TEXT("quick.assign:1"));FoodSlot.Enabled=Selected.IsValid();
    Rule(40,854,1592,TEXT("inventory.footer.rule"),TEXT("inventory.footer"));'''),
])
print('Input, combat and UI changes applied')
