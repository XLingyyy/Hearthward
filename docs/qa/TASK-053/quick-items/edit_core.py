from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]

def edit(name, changes):
    p = ROOT / name
    text = p.read_text(encoding='utf-8-sig')
    for before, after in changes:
        assert before in text, (name, before[:100])
        text = text.replace(before, after, 1)
    p.write_text(text, encoding='utf-8')

edit('Source/Hearthward/Gameplay/HearthwardGameplayComponent.h', [
    ('    UFUNCTION(BlueprintCallable) bool UseItem(FName Id);', '''    UFUNCTION(BlueprintCallable) bool UseItem(FName Id);
    UFUNCTION(BlueprintPure) FName QuickItem(int32 Slot) const;
    static bool FitsQuickSlot(int32 Slot,FName Item);
    UFUNCTION(BlueprintCallable) bool AssignQuickItem(int32 Slot,FName Item);
    UFUNCTION(BlueprintCallable) bool UseQuickItem(int32 Slot);
    bool CanUseItem(FName Item);'''),
    ('private:\n    uint32 FeedbackRevision=0;', '''private:
    UPROPERTY() TArray<FName> QuickItems={TEXT("medicine"),TEXT("roast"),TEXT("arrow"),TEXT("firepot")};
    uint32 FeedbackRevision=0;'''),
])
edit('Source/Hearthward/Gameplay/HearthwardGameplayComponent.cpp', [
    ('bool UHearthwardGameplayComponent::UseItem(FName Id)\n{', '''FName UHearthwardGameplayComponent::QuickItem(int32 Slot) const
{ return QuickItems.IsValidIndex(Slot)?QuickItems[Slot]:NAME_None; }
bool UHearthwardGameplayComponent::FitsQuickSlot(int32 Slot,FName Item)
{
    const auto Row=Find(TEXT("items"),Item.ToString());
    if(!Row) return false;
    switch(Slot)
    {
    case 0:return Number(Row,TEXT("healing"))>0;
    case 1:return Number(Row,TEXT("food"))>0;
    case 2:return Item==TEXT("arrow");
    case 3:return Number(Row,TEXT("throwDamage"))>0 || Number(Row,TEXT("bait"))>0;
    default:return false;
    }
}
bool UHearthwardGameplayComponent::AssignQuickItem(int32 Slot,FName Item)
{
    if(!QuickItems.IsValidIndex(Slot)) return false;
    const TCHAR* Names[]={TEXT("药品栏"),TEXT("食物栏"),TEXT("弓箭栏"),TEXT("投掷栏")};
    if(!FitsQuickSlot(Slot,Item)) return Result(false,FString::Printf(TEXT("无法放置到%s：物品作用不符"),Names[Slot]));
    if(!Inventory() || Inventory()->GetItemCount(Item)<=0) return Result(false,TEXT("无法放置：背包中没有该物品"));
    QuickItems[Slot]=Item;
    return Result(true,FString::Printf(TEXT("已放入%s：%s"),Names[Slot],*Text(Find(TEXT("items"),Item.ToString()),TEXT("name"))));
}
bool UHearthwardGameplayComponent::CanUseItem(FName Item)
{
    const auto* Survival=GetOwner()?GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    const FName Active=Survival?Survival->ActiveConsumable():NAME_None;
    if(Active.IsNone()) return true;
    const FString ActiveName=Text(Find(TEXT("items"),Active.ToString()),TEXT("name"),TEXT("药品"));
    const FString ItemName=Text(Find(TEXT("items"),Item.ToString()),TEXT("name"),TEXT("选中道具"));
    return Result(false,FString::Printf(TEXT("当前正在使用%s，不能同时使用%s"),*ActiveName,*ItemName));
}
bool UHearthwardGameplayComponent::UseQuickItem(int32 Slot)
{
    const FName Item=QuickItem(Slot);
    if(!FitsQuickSlot(Slot,Item)) return Result(false,TEXT("该道具栏中的物品作用不符"));
    return UseItem(Item);
}
bool UHearthwardGameplayComponent::UseItem(FName Id)
{
    if(!Enabled || !CanUseItem(Id)) return false;
    if(Id==TEXT("arrow")) return Result(false,TEXT("箭矢需装备对应弓，选中弓箭栏后用左键射击"));'''),
    ('    if(Ate) Record(TEXT("consume"),Id);\n    return Result(Ate,Ate?TEXT("已食用：")+Text(R,TEXT("name")):TEXT("当前无法食用"));',
     '    return Result(Ate,Ate?TEXT("正在进食：")+Text(R,TEXT("name")):TEXT("当前无法食用"));'),
    ('    J->SetStringField(TEXT("tracked"),TrackedQuest.ToString());', '''    TArray<TSharedPtr<FJsonValue>> Quick;
    for(FName Item:QuickItems) Quick.Add(MakeShared<FJsonValueString>(Item.ToString()));
    J->SetArrayField(TEXT("quickItems"),Quick);
    J->SetStringField(TEXT("tracked"),TrackedQuest.ToString());'''),
    ('    if(Number(J,TEXT("version"))!=1) return false;', '''    if(Number(J,TEXT("version"))!=1) return false;
    if(J->HasField(TEXT("quickItems")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Quick;
        if(!J->TryGetArrayField(TEXT("quickItems"),Quick) || Quick->Num()!=4) return false;
        for(int32 Slot=0;Slot<4;++Slot)
        { FString Item;if(!(*Quick)[Slot]->TryGetString(Item) || !FitsQuickSlot(Slot,FName(*Item)))return false; }
    }'''),
    ('    Sprinting=false; RecoveryDelay=0;', '    QuickItems={TEXT("medicine"),TEXT("roast"),TEXT("arrow"),TEXT("firepot")};\n    Sprinting=false; RecoveryDelay=0;'),
    ('    const auto J=Parse(Json);\n    if(J->HasField(TEXT("companionOrder")))', '''    const auto J=Parse(Json);
    if(J->HasTypedField<EJson::Array>(TEXT("quickItems")))
        for(int32 Slot=0;Slot<4;++Slot) QuickItems[Slot]=FName(*J->GetArrayField(TEXT("quickItems"))[Slot]->AsString());
    if(J->HasField(TEXT("companionOrder")))'''),
])
edit('Source/Hearthward/Survival/HearthwardSurvivalState.h', [
    ('    UPROPERTY() double HotRate = 0;', '    UPROPERTY() double HotRate = 0;\n    UPROPERTY() FName HotItem;'),
    ('    UPROPERTY() bool AutomaticMedicine = false;', '''    UPROPERTY() bool AutomaticMedicine = false;
    UPROPERTY(BlueprintReadOnly) FName FoodItem;
    UPROPERTY(BlueprintReadOnly) double FoodRemaining = 0;'''),
    ('HotRemaining=HotRate=0; }', 'HotRemaining=HotRate=0; HotItem=NAME_None; }'),
    ('DownRemaining=120; HotRemaining=HotRate=0; }', 'DownRemaining=120; HotRemaining=HotRate=0; HotItem=NAME_None; }'),
    ('            HotRemaining=FMath::Max(0.,HotRemaining-Active*Step);', '            HotRemaining=FMath::Max(0.,HotRemaining-Active*Step);\n            if(HotRemaining==0) HotItem=NAME_None;'),
])
edit('Source/Hearthward/Survival/HearthwardSurvivalComponent.h', [
    ('    bool Busy() const { return !State.Medicine.IsNone() || Rescue.IsValid(); }', '''    bool Busy() const { return !State.Medicine.IsNone() || !State.FoodItem.IsNone() || Rescue.IsValid(); }
    FName ActiveConsumable() const;'''),
])
edit('Source/Hearthward/Survival/HearthwardSurvivalComponent.cpp', [
    ('bool UHearthwardSurvivalComponent::BeginMedicine(FName Item,bool Automatic)', '''FName UHearthwardSurvivalComponent::ActiveConsumable() const
{
    if(!State.Medicine.IsNone()) return State.Medicine;
    if(!State.FoodItem.IsNone()) return State.FoodItem;
    if(State.HotRemaining>0) return State.HotItem.IsNone()?FName(TEXT("medicine")):State.HotItem;
    // Keep a same-frame cancelled dose reserved until damage or the next active tick settles it.
    return CancelledMedicine;
}
bool UHearthwardSurvivalComponent::BeginMedicine(FName Item,bool Automatic)'''),
    ('    if(Duration>0 && State.HotRemaining>0 && MaxHealth()*Fraction/Duration<State.HotRate) return false;', '''    if(!Automatic && !ActiveConsumable().IsNone()) return false;
    if(Duration>0 && State.HotRemaining>0 && MaxHealth()*Fraction/Duration<State.HotRate) return false;'''),
    ('''    if(!Enabled() || !Alive() || Settling || Food<=0 || Hunger()>=100 || (Automatic && !Permitted(Item))) return false;
    TGuardValue<bool> Guard(Settling,true);''', '''    if(!Enabled() || !Alive() || Settling || Busy() || Food<=0 || Hunger()>=100 || (Automatic && !Permitted(Item))) return false;
    if(!Automatic)
    {
        if(!ActiveConsumable().IsNone()) return false;
        if(const auto* C=GetOwner()->FindComponentByClass<UHearthwardCombatComponent>();C && (C->Busy() || C->Guarding() || C->MovementMultiplier()<1)) return false;
        if(!Bag() || !Bag()->Reserve(Item)) return false;
        State.FoodItem=Item; State.FoodRemaining=3;
        ActionEpoch=Epoch(); ActionOrigin=GetOwner()->GetActorLocation();
        if(auto* Timer=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>()) Timer->InterruptAction();
        Resting=false; SetStatus(TEXT("正在进食")); return true;
    }
    TGuardValue<bool> Guard(Settling,true);'''),
    ('    Rescue.Reset(); RescueRemaining=0; Resting=false; Treatment=false;\n    FName Item=State.Medicine;', '''    Rescue.Reset(); RescueRemaining=0; Resting=false; Treatment=false;
    if(!State.FoodItem.IsNone())
    {
        State.FoodItem=NAME_None; State.FoodRemaining=0;
        if(Bag()) Bag()->ReleaseReservation();
    }
    FName Item=State.Medicine;'''),
    ('    if(!Alive() || ActionEpoch!=Epoch() || FVector::Dist(ActionOrigin,GetOwner()->GetActorLocation())>5 || GetOwner()->GetVelocity().Size()>5)',
     '    if(!Alive() || ActionEpoch!=Epoch() || (State.FoodItem.IsNone() && (FVector::Dist(ActionOrigin,GetOwner()->GetActorLocation())>5 || GetOwner()->GetVelocity().Size()>5)))'),
    ('    if(State.Medicine.IsNone()) return;\n    State.MedicineRemaining=', '''    if(!State.FoodItem.IsNone())
    {
        State.FoodRemaining=FMath::Max(0.,State.FoodRemaining-Delta);
        if(State.FoodRemaining>0) return;
        if(Hunger()>=100) { CancelAction();return; }
        const FName Used=State.FoodItem;
        const float Food=Number(Find(TEXT("items"),Used.ToString()),TEXT("food"));
        TGuardValue<bool> Guard(Settling,true);
        const bool Committed=Bag()->CommitReservation(NAME_None,false);
        State.FoodItem=NAME_None;State.FoodRemaining=0;
        if(!Committed) {SetStatus(TEXT("进食已取消"));return;}
        Hunger()=FMath::Min(100.f,Hunger()+Food*(1+(Gameplay()?Gameplay()->Effect(TEXT("food")):0)));
        State.Food(Hunger());SetStatus(TEXT("进食完成"));
        if(auto* G=Gameplay()) G->Record(TEXT("consume"),Used);
        Bag()->OnInventoryChanged.Broadcast();return;
    }
    if(State.Medicine.IsNone()) return;
    State.MedicineRemaining='''),
    ('    if(Duration>0) { State.HotRemaining=Duration; State.HotRate=Heal/Duration; }',
     '    if(Duration>0) { State.HotRemaining=Duration; State.HotRate=Heal/Duration; State.HotItem=State.Medicine; }'),
    ('    if(!State.Medicine.IsNone()) Bag()->Reserve(State.Medicine);', '    if(!State.Medicine.IsNone()) Bag()->Reserve(State.Medicine);\n    else if(!State.FoodItem.IsNone()) Bag()->Reserve(State.FoodItem);'),
    ('    if(State.DrowningRemaining>=0) return FString::Printf', '    if(!State.FoodItem.IsNone()) return FString::Printf(TEXT("进食：剩余 %.1f 秒"),State.FoodRemaining);\n    if(State.DrowningRemaining>=0) return FString::Printf'),
])
edit('Source/Hearthward/Save/HearthwardSaveGame.cpp', [
    ('State.SafeSeconds,State.MedicineRemaining}', 'State.SafeSeconds,State.MedicineRemaining,State.FoodRemaining}'),
    ('State.MedicineRemaining<0 || State.MedicineRemaining>3)', 'State.MedicineRemaining<0 || State.MedicineRemaining>3 || State.FoodRemaining<0 || State.FoodRemaining>3)'),
    ('    for(FName Id:State.AutoPermissions)', '''    if(State.FoodItem.IsNone()!=(State.FoodRemaining==0) || (!State.Medicine.IsNone() && !State.FoodItem.IsNone())) return false;
    if(!State.FoodItem.IsNone() && (Bag.FindRef(State.FoodItem)<1 || HearthwardData::Number(HearthwardData::Find(TEXT("items"),State.FoodItem.ToString()),TEXT("food"))<=0)) return false;
    if(!State.HotItem.IsNone() && (State.HotRemaining<=0 || HearthwardData::Number(HearthwardData::Find(TEXT("items"),State.HotItem.ToString()),TEXT("healing"))<=0)) return false;
    for(FName Id:State.AutoPermissions)'''),
])
print('Gameplay, survival, and save changes applied')
