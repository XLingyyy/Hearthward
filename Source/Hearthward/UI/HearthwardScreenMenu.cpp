#include "HearthwardScreenWidget.h"
#include "Engine/GameInstance.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

FString UHearthwardScreenWidget::InventoryCategory(const TSharedPtr<FJsonObject>& Item) const
{
    const FName Tab=UHearthwardGameplayComponent::InventoryTab(FName(*HearthwardData::Text(Item,TEXT("id"))));
    if(Tab==TEXT("gear"))return TEXT("装备");
    if(Tab==TEXT("material"))return TEXT("材料");
    if(Tab==TEXT("consumable"))return TEXT("消耗品");
    if(Tab==TEXT("tool"))return TEXT("工具");
    return FString();
}

void UHearthwardScreenWidget::ComposeInventoryScreen()
{
    const auto* G=Gameplay(); const auto* Bag=Inventory();
    const float TextScale=FMath::Clamp(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f,1.f,1.5f);
    const FString Categories[]={TEXT("装备"),TEXT("材料"),TEXT("消耗品"),TEXT("工具")};
    const FName TabIds[]={TEXT("gear"),TEXT("material"),TEXT("consumable"),TEXT("tool")};
    const FString Icons[]={TEXT("category_gear"),TEXT("category_material"),TEXT("category_food"),TEXT("category_tool")};
    if(Category!=Categories[0] && Category!=Categories[1] && Category!=Categories[2] && Category!=Categories[3]) Category=Categories[0];
    auto Inv=[&](FString Type,FString Label,FVector2D P,FVector2D Size,float Font,FString Id,FString Component,FString Action=FString(),FString Asset=FString(),bool Selected=false)->FHearthwardUIElement&
    {
        Element(Type,Label,P,Size,Font,Action,Asset,Selected);
        auto& E=Elements.Last(); E.Font=Font*TextScale; E.FontRole=TEXT("body"); E.Tracking=0; E.TextInset=0;
        E.LayoutId=Id; E.Component=Component; return E;
    };
    auto Rule=[&](float X,float Y,float Width,const FString& Id,const FString& Component)
    { Inv(TEXT("line"),TEXT(""),{X,Y},{Width,1},18,Id,Component).Color=Color(TEXT("bronze"))*.65f; };
    Inv(TEXT("inventorySurface"),TEXT(""),{40,140},{548,700},18,TEXT("inventory.bag.surface"),TEXT("inventory.bag"));
    Inv(TEXT("inventorySurface"),TEXT(""),{600,140},{566,700},18,TEXT("inventory.detail.surface"),TEXT("inventory.detail"));
    Inv(TEXT("inventorySurface"),TEXT(""),{1178,140},{454,700},18,TEXT("inventory.stats.surface"),TEXT("inventory.stats"));
    Inv(TEXT("text"),TEXT("背包"),{64,36},{132,56},28,TEXT("inventory.heading"),TEXT("inventory.header")).Color=Color(TEXT("gold"));
    for(int32 Tab=0;Tab<4;++Tab)
    {
        const float X=220+Tab*192;
        auto& E=Inv(TEXT("menuTab"),Categories[Tab],{X,28},{168,62},22,TEXT("inventory.tab.")+Categories[Tab],TEXT("inventory.header"),TEXT("filter:")+Categories[Tab],TEXT(""),Category==Categories[Tab]);
        E.TextInset=54;
        Inv(TEXT("image"),TEXT(""),{X+10,42},{30,30},18,TEXT("inventory.tab.icon.")+Categories[Tab],TEXT("inventory.header"),TEXT(""),Icons[Tab]).Color=Category==Categories[Tab]?Color(TEXT("gold")):Color(TEXT("muted"));
    }
    Inv(TEXT("text"),FString::Printf(TEXT("负重  %.1f / %.0f"),Bag->GetWeight(),Bag->GetCapacity()),{1048,43},{392,40},18,TEXT("inventory.header.weight"),TEXT("inventory.header")).Align=TEXT("right");
    Inv(TEXT("menuAction"),TEXT("设置"),{1480,31},{128,56},21,TEXT("inventory.settings"),TEXT("inventory.header"),TEXT("page:settings")).Align=TEXT("center");
    Rule(40,102,1592,TEXT("inventory.header.rule"),TEXT("inventory.header"));
    Inv(TEXT("text"),Message.IsEmpty()?TEXT("HEARTHWARD"):Message,{64,108},{1544,28},14,TEXT("inventory.message"),TEXT("inventory.header")).Color=Color(TEXT("muted"));

    const bool Materials=Category==TEXT("材料"),Gear=Category==TEXT("装备");
    if(Gear)
    {
        Inv(TEXT("text"),TEXT("当前装备"),{64,154},{490,46},24,TEXT("inventory.equipment.heading"),TEXT("inventory.bag")).Color=Color(TEXT("gold"));
        Rule(64,204,500,TEXT("inventory.equipment.rule"),TEXT("inventory.bag"));
        auto EquipmentSlots=Theme->GetObjectField(TEXT("inventory"))->GetArrayField(TEXT("equipmentSlots"));
        for(const auto& Extra:TArray<TPair<FString,FString>>{{TEXT("hands"),TEXT("手部")},{TEXT("tool"),TEXT("工具")}})
        {
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),Extra.Key);Row->SetStringField(TEXT("name"),Extra.Value);
            EquipmentSlots.Add(MakeShared<FJsonValueObject>(Row));
        }
        for(int32 SlotIndex=0;SlotIndex<EquipmentSlots.Num();++SlotIndex)
        {
            const auto SlotRow=EquipmentSlots[SlotIndex]->AsObject(); const FString SlotId=HearthwardData::Text(SlotRow,TEXT("id"));
            const float X=64+(SlotIndex%6)*84,Y=245+(SlotIndex/6)*140;
            auto& Label=Inv(TEXT("text"),HearthwardData::Text(SlotRow,TEXT("name")),{X-3,Y-33},{80,34},16,TEXT("inventory.equipment.")+SlotId+TEXT(".label"),TEXT("inventory.bag")); Label.Align=TEXT("center");
            const FGuid Instance=Bag->EquippedInstance(FName(*SlotId));const auto* Equipped=Bag->FindInstance(Instance);
            const FName ItemId=Equipped?Equipped->Definition:NAME_None;const auto Item=HearthwardData::Find(TEXT("items"),ItemId.ToString());
            auto& Cell=Inv(TEXT("inventorySlot"),TEXT(""),{X,Y},{74,80},18,TEXT("inventory.equipment.")+SlotId,TEXT("inventory.bag"),Item?TEXT("item:")+ItemId.ToString():TEXT(""),Item?HearthwardData::Text(Item,TEXT("icon")):TEXT(""),Item && ItemId==SelectedItem);
            Cell.Value=Item?1:0;Cell.InventoryItem=ItemId;Cell.InventoryInstance=Instance;Cell.EquipmentSlot=FName(*SlotId);
        }
    }
    else if(!Materials)
    {
        Inv(TEXT("text"),TEXT("物品栏"),{64,154},{490,46},24,TEXT("inventory.quick.heading"),TEXT("inventory.bag")).Color=Color(TEXT("gold"));
        Rule(64,204,500,TEXT("inventory.quick.rule"),TEXT("inventory.bag"));
        const TCHAR* Roles[]={TEXT("medicine"),TEXT("food"),TEXT("ammunition"),TEXT("throwable")};
        const TCHAR* Names[]={TEXT("药品"),TEXT("食物"),TEXT("弓箭"),TEXT("投掷物")};
        const FName Bindings[]={TEXT("survival.medicine"),TEXT("survival.food"),TEXT("combat.ammunition"),TEXT("combat.throw")};
        const auto& Keys=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings;
        for(int32 QuickIndex=0;QuickIndex<4;++QuickIndex)
        {
            const float X=72+QuickIndex*125;const FString Id=FString(TEXT("inventory.quick."))+Roles[QuickIndex];
            auto& Label=Inv(TEXT("text"),Names[QuickIndex],{X-16,223},{116,36},16,Id+TEXT(".label"),TEXT("inventory.bag"));Label.Align=TEXT("center");
            const FName ItemId=G->QuickItem(QuickIndex);const auto Item=HearthwardData::Find(TEXT("items"),ItemId.ToString());
            auto& Cell=Inv(TEXT("inventorySlot"),Item?FString::FromInt(Bag->GetItemCount(ItemId)):TEXT(""),{X,269},{84,80},18,Id,TEXT("inventory.bag"),Item?TEXT("item:")+ItemId.ToString():TEXT(""),Item?HearthwardData::Text(Item,TEXT("icon")):TEXT(""),Item && ItemId==SelectedItem);
            Cell.Value=0;Cell.InventoryItem=ItemId;Cell.QuickSlot=QuickIndex;
            const auto& RoleKeys=Keys.FindChecked(Bindings[QuickIndex]);
            Cell.Shortcut=RoleKeys[0].Key.IsValid()?RoleKeys[0].Label():(RoleKeys[1].Key.IsValid()?RoleKeys[1].Label():TEXT("—"));
            auto& Name=Inv(TEXT("text"),Item?HearthwardData::Text(Item,TEXT("name")):TEXT("未配置"),{X-16,362},{116,50},14,Id+TEXT(".name"),TEXT("inventory.bag"));Name.Align=TEXT("center");Name.Color=Color(TEXT("muted"));
        }
        Inv(TEXT("text"),TEXT("左键拖入配置 · 拖出清空"),{64,432},{500,37},16,TEXT("inventory.quick.help"),TEXT("inventory.bag")).Color=Color(TEXT("muted"));
    }
    int32 Tab=0;for(int32 Index=0;Index<4;++Index)if(Category==Categories[Index])Tab=Index;
    const auto Slots=G->InventorySlots(TabIds[Tab]);
    TArray<FName> Owned;for(FName Item:Slots)if(!Item.IsNone())Owned.Add(Item);
    TArray<FName> Selectable=Owned;
    if(Gear)for(const auto& Equipped:Bag->Snapshot().Equipped)
        if(const auto* Instance=Bag->FindInstance(Equipped.Value))Selectable.AddUnique(Instance->Definition);
    if(!Selectable.Contains(SelectedItem))SelectedItem=Selectable.IsEmpty()?NAME_None:Selectable[0];
    constexpr int32 Columns=5;const int32 VisibleRows=Materials?6:3,PageSize=Columns*VisibleRows;
    const int32 LastRow=FMath::Max(0,FMath::DivideAndRoundUp(Slots.Num(),Columns)-VisibleRows);
    Scroll=FMath::Clamp(Scroll,0,LastRow); const int32 First=Scroll*Columns;
    const float HeadingY=Materials?154:486,GridY=Materials?226:548,StepY=Materials?92:86;
    Inv(TEXT("text"),Category,{64,HeadingY},{240,44},22,TEXT("inventory.bag.heading"),TEXT("inventory.bag")).Color=Color(TEXT("gold"));
    auto& Range=Inv(TEXT("text"),Owned.IsEmpty()?TEXT("0 件"):Slots.Num()!=Owned.Num()?FString::Printf(TEXT("%d 件 · %d–%d格"),Owned.Num(),First+1,FMath::Min(First+PageSize,Slots.Num())):FString::Printf(TEXT("%d–%d / %d"),First+1,FMath::Min(First+PageSize,Slots.Num()),Slots.Num()),{334,HeadingY+6},{224,34},17,TEXT("inventory.bag.range"),TEXT("inventory.bag")); Range.Align=TEXT("right");
    Rule(64,Materials?204:535,500,TEXT("inventory.bag.rule"),TEXT("inventory.bag"));
    for(int32 Index=0;Index<PageSize;++Index)
    {
        const FVector2D P(64+(Index%Columns)*99,GridY+(Index/Columns)*StepY);
        const FName ItemId=Slots.IsValidIndex(First+Index)?Slots[First+Index]:NAME_None;const auto Item=HearthwardData::Find(TEXT("items"),ItemId.ToString());
        auto& Cell=Inv(TEXT("inventorySlot"),Item?FString::FromInt(G->BackpackItemCount(ItemId)):TEXT(""),P,{84,78},18,
            Item?TEXT("inventory.bag.item.")+ItemId.ToString():TEXT("inventory.bag.empty.")+FString::FromInt(Index),TEXT("inventory.bag"),Item?TEXT("item:")+ItemId.ToString():TEXT(""),Item?HearthwardData::Text(Item,TEXT("icon")):TEXT(""),Item && SelectedItem==ItemId);
        Cell.Value=0;
        Cell.InventoryItem=ItemId;Cell.InventoryPosition=First+Index;Cell.InventoryGroup=TabIds[Tab];
        if(Item)Cell.InventoryInstance=Bag->FirstInstance(ItemId,true);
    }
    auto& Prev=Inv(TEXT("menuAction"),TEXT("‹ 上一页"),{72,804},{174,34},16,TEXT("inventory.bag.prev"),TEXT("inventory.bag"),TEXT("inventory.prev")); Prev.Enabled=Scroll>0;
    auto& Next=Inv(TEXT("menuAction"),TEXT("下一页 ›"),{380,804},{174,34},16,TEXT("inventory.bag.next"),TEXT("inventory.bag"),TEXT("inventory.next")); Next.Enabled=Scroll<LastRow; Next.Align=TEXT("right");

    const auto Selected=HearthwardData::Find(TEXT("items"),SelectedItem.ToString());
    Inv(TEXT("text"),Selected?HearthwardData::Text(Selected,TEXT("name")):TEXT("物品详情"),{624,154},{518,62},26,TEXT("inventory.detail.name"),TEXT("inventory.detail")).Color=Color(TEXT("gold"));
    Rule(624,220,518,TEXT("inventory.detail.titleRule"),TEXT("inventory.detail"));
    if(Selected)
    {
        const auto* DetailInstance=EquipmentOwner==TEXT("player")?Bag->FindInstance(EquipmentSelection):nullptr;
        if(DetailInstance && DetailInstance->Definition!=SelectedItem)DetailInstance=nullptr;
        const bool MissingSelectedInstance=EquipmentOwner==TEXT("player") && EquipmentSelection.IsValid() && !Bag->FindInstance(EquipmentSelection);
        if(!DetailInstance && !MissingSelectedInstance)
        {
            for(const auto& Equipped:Bag->Snapshot().Equipped)
                if(const auto* Instance=Bag->FindInstance(Equipped.Value);Instance && Instance->Definition==SelectedItem){DetailInstance=Instance;break;}
            if(!DetailInstance)DetailInstance=Bag->FindInstance(Bag->FirstInstance(SelectedItem,true));
            if(DetailInstance){EquipmentSelection=DetailInstance->Id;EquipmentOwner=TEXT("player");EquipmentStack=NAME_None;}
        }
        Inv(TEXT("text"),TEXT("类型  ")+HearthwardData::Text(Selected,TEXT("category")),{624,238},{252,38},17,TEXT("inventory.detail.category"),TEXT("inventory.detail")).Color=Color(TEXT("muted"));
        auto& Count=Inv(TEXT("text"),FString::Printf(TEXT("持有 %d · 可用 %d"),Bag->GetItemCount(SelectedItem),Bag->Available(SelectedItem)),{888,238},{254,38},17,TEXT("inventory.detail.quantity"),TEXT("inventory.detail")); Count.Align=TEXT("right");
        Inv(TEXT("image"),TEXT(""),{754,282},{260,182},18,TEXT("inventory.detail.art"),TEXT("inventory.detail"),TEXT(""),SelectedItem==TEXT("axe")?TEXT("axeLarge"):HearthwardData::Text(Selected,TEXT("icon")));
        Inv(TEXT("text"),TEXT("物品说明"),{624,468},{518,42},21,TEXT("inventory.detail.descriptionHeading"),TEXT("inventory.detail")).Color=Color(TEXT("gold"));
        Rule(624,513,518,TEXT("inventory.detail.descriptionRule"),TEXT("inventory.detail"));
        const FString Description=(DetailInstance?TEXT("实例 ")+DetailInstance->Id.ToString(EGuidFormats::Digits).Left(8)
            +(Bag->IsEquipped(DetailInstance->Id)?TEXT(" · 已装备\n"):TEXT(" · 未装备\n")):FString())+HearthwardData::Text(Selected,TEXT("description"));
        const float DescriptionFont=19*TextScale;
        const FSlateFontInfo Font(Typeface,FMath::RoundToInt(DescriptionFont*.75f));
        const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        TArray<FString> SourceLines; Description.ParseIntoArrayLines(SourceLines,false); int32 DescriptionLines=0;
        for(FString Line:SourceLines)
        {
            while(Line.Len()>1 && Measure->Measure(Line,Font).X>518)
            {
                const int32 WrappedCount=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,518)+1,1,Line.Len());
                Line=Line.Mid(WrappedCount); ++DescriptionLines;
            }
            ++DescriptionLines;
        }
        const float DescriptionHeight=FMath::Max(80.f,DescriptionLines*DescriptionFont*1.6f);
        const float PropertiesY=FMath::Max(630.f,529+DescriptionHeight+16);
        Inv(TEXT("text"),Description,{624,529},{518,DescriptionHeight},19,TEXT("inventory.detail.description"),TEXT("inventory.detail"));
        TArray<TPair<FString,FString>> Properties;
        auto Property=[&](FString Label,FString Value){Properties.Emplace(Label,Value);};
        if(!HearthwardData::Text(Selected,TEXT("slot")).IsEmpty())
        {
            if(HearthwardData::Number(Selected,TEXT("attack"))>0) Property(TEXT("攻击力"),FString::Printf(TEXT("%.0f"),HearthwardData::Number(Selected,TEXT("attack"))));
            if(HearthwardData::Number(Selected,TEXT("defense"))>0) Property(TEXT("防御力"),FString::Printf(TEXT("%.0f"),HearthwardData::Number(Selected,TEXT("defense"))));
            const float Maximum=HearthwardData::Number(Selected,TEXT("durability"),100);
            Property(TEXT("耐久度"),DetailInstance?FString::Printf(TEXT("%.2f / %.0f"),DetailInstance->Durability,Maximum):TEXT("所选实例不可用"));
        }
        else
        {
            if(HearthwardData::Number(Selected,TEXT("food"))>0) Property(TEXT("饱食恢复"),FString::Printf(TEXT("+%.0f"),HearthwardData::Number(Selected,TEXT("food"))));
            if(HearthwardData::Number(Selected,TEXT("healing"))>0) Property(TEXT("生命恢复"),FString::Printf(TEXT("+%.0f"),HearthwardData::Number(Selected,TEXT("healing"))));
            if(HearthwardData::Number(Selected,TEXT("throwDamage"))>0) Property(TEXT("投掷伤害"),FString::Printf(TEXT("%.0f"),HearthwardData::Number(Selected,TEXT("throwDamage"))));
        }
        Property(TEXT("单重"),FString::Printf(TEXT("%.2f"),HearthwardData::Number(Selected,TEXT("weight"))/100));
        Inv(TEXT("text"),TEXT("物品属性"),{624,PropertiesY},{518,40},21,TEXT("inventory.detail.propertiesHeading"),TEXT("inventory.detail")).Color=Color(TEXT("gold"));
        for(int32 PropertyIndex=0;PropertyIndex<Properties.Num();++PropertyIndex)
        {
            const float Y=PropertiesY+49+PropertyIndex*38*TextScale;
            Inv(TEXT("text"),Properties[PropertyIndex].Key,{624,Y},{248,37},18,TEXT("inventory.detail.property.")+FString::FromInt(PropertyIndex)+TEXT(".label"),TEXT("inventory.detail"));
            auto& Value=Inv(TEXT("text"),Properties[PropertyIndex].Value,{894,Y},{248,37},18,TEXT("inventory.detail.property.")+FString::FromInt(PropertyIndex)+TEXT(".value"),TEXT("inventory.detail")); Value.Align=TEXT("right");
            Rule(624,Y+37,518,TEXT("inventory.detail.property.")+FString::FromInt(PropertyIndex)+TEXT(".rule"),TEXT("inventory.detail"));
        }
    }
    else Inv(TEXT("text"),TEXT("选择左侧物品查看说明"),{624,340},{518,64},22,TEXT("inventory.detail.emptyState"),TEXT("inventory.detail")).Color=Color(TEXT("muted"));

    Inv(TEXT("text"),TEXT("角色资料"),{1202,154},{406,46},24,TEXT("inventory.stats.heading"),TEXT("inventory.stats")).Color=Color(TEXT("gold"));
    Rule(1202,204,406,TEXT("inventory.stats.rule"),TEXT("inventory.stats"));
    Inv(TEXT("text"),TEXT("荒行者"),{1202,219},{406,43},22,TEXT("inventory.stats.role"),TEXT("inventory.stats"));
    Inv(TEXT("text"),TEXT("行过黑暗，守住火种。"),{1202,270},{406,33},16,TEXT("inventory.stats.flavor"),TEXT("inventory.stats")).Color=Color(TEXT("muted"));
    const auto Tuning=HearthwardData::Catalog()->GetObjectField(TEXT("tuning"));
    int32 LevelExperience=G->Experience;
    for(int32 LevelIndex=1;LevelIndex<G->Level();++LevelIndex) LevelExperience-=HearthwardData::Number(Tuning,TEXT("xpBase"))+(LevelIndex-1)*HearthwardData::Number(Tuning,TEXT("xpGrowth"));
    const int32 NextLevel=HearthwardData::Number(Tuning,TEXT("xpBase"))+(G->Level()-1)*HearthwardData::Number(Tuning,TEXT("xpGrowth"));
    const TPair<FString,FString> Stats[]={
        {TEXT("等级"),FString::FromInt(G->Level())},{TEXT("经验"),FString::Printf(TEXT("%d / %d"),LevelExperience,NextLevel)},
        {TEXT("生命值"),FString::Printf(TEXT("%.0f / %.0f"),G->Health,G->MaxHealth())},{TEXT("饱食度"),FString::Printf(TEXT("%.0f / 100"),G->Hunger)},
        {TEXT("体力"),FString::Printf(TEXT("%.0f / %.0f"),G->Stamina,G->MaxStamina())},{TEXT("攻击力"),FString::Printf(TEXT("%.0f"),G->AttackPower())},
        {TEXT("全身减伤"),FString::Printf(TEXT("%.0f%%"),FMath::Min(85.f,G->Effect(TEXT("defense"))*100))},
        {TEXT("重击增伤"),FString::Printf(TEXT("+%.0f%%"),G->Effect(TEXT("heavy_damage"))*100)},
        {TEXT("耐力消耗"),FString::Printf(TEXT("−%.0f%%"),G->Effect(TEXT("cost"))*100)},
        {TEXT("负重"),FString::Printf(TEXT("%.1f / %.0f"),Bag->GetWeight(),Bag->GetCapacity())},
        {TEXT("移动速度"),FString::Printf(TEXT("%.0f%%"),Bag->GetMoveSpeedMultiplier()*100)},
        {TEXT("头 / 胸减伤"),FString::Printf(TEXT("%.0f%% / %.0f%%"),G->ArmorReduction(TEXT("head")),G->ArmorReduction(TEXT("chest")))},
        {TEXT("腿 / 脚减伤"),FString::Printf(TEXT("%.0f%% / %.0f%%"),G->ArmorReduction(TEXT("legs")),G->ArmorReduction(TEXT("feet")))}};
    for(int32 Stat=0;Stat<UE_ARRAY_COUNT(Stats);++Stat)
    {
        const float Y=330+Stat*38; const FString Id=TEXT("inventory.stats.")+FString::FromInt(Stat);
        Inv(TEXT("text"),Stats[Stat].Key,{1202,Y},{184,38},18,Id+TEXT(".label"),TEXT("inventory.stats"));
        auto& Value=Inv(TEXT("text"),Stats[Stat].Value,{1400,Y},{208,38},18,Id+TEXT(".value"),TEXT("inventory.stats")); Value.Align=TEXT("right");
        if(Stat>=2 && Stat<=4) Value.Color=Color(Stat==2?TEXT("health"):Stat==3?TEXT("hunger"):TEXT("stamina"));
        Rule(1202,Y+37,406,Id+TEXT(".rule"),TEXT("inventory.stats"));
    }
    Rule(40,854,1592,TEXT("inventory.footer.rule"),TEXT("inventory.footer"));
    Inv(TEXT("menuAction"),TEXT("Esc  返回"),{64,868},{170,50},20,TEXT("inventory.back"),TEXT("inventory.footer"),TEXT("back"));
    auto& Drop=Inv(TEXT("menuAction"),TEXT("R  丢弃"),{246,868},{170,50},20,TEXT("inventory.drop"),TEXT("inventory.footer"),TEXT("drop")); Drop.Enabled=Selected.IsValid();
    auto& Use=Inv(TEXT("menuAction"),TEXT("F  装备 / 使用"),{428,868},{242,50},20,TEXT("inventory.use"),TEXT("inventory.footer"),TEXT("use")); Use.Enabled=Selected.IsValid();
    Inv(TEXT("menuAction"),TEXT("逐件装备 · 行装管理"),{700,868},{338,50},18,TEXT("inventory.equipment.manage"),TEXT("inventory.footer"),TEXT("page:equipment"));
    bool Rare=false,KeyItem=false;
    if(Selected)
    {
        Selected->TryGetBoolField(TEXT("rare"),Rare); Selected->TryGetBoolField(TEXT("key"),KeyItem);
        if(const auto Base=HearthwardData::Find(TEXT("items"),HearthwardData::Text(Selected,TEXT("medicineBase"))))
        {bool BaseRare=false,BaseKey=false;Base->TryGetBoolField(TEXT("rare"),BaseRare);Base->TryGetBoolField(TEXT("key"),BaseKey);Rare|=BaseRare;KeyItem|=BaseKey;}
    }
    if(Selected && (Rare || KeyItem) && (HearthwardData::Number(Selected,TEXT("healing"))>0 || HearthwardData::Number(Selected,TEXT("food"))>0))
        for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        {
            const auto* Survival=It->FindComponentByClass<UHearthwardSurvivalComponent>();
            Inv(TEXT("menuAction"),Survival->Permitted(SelectedItem)?TEXT("撤销弟弟自动使用授权"):TEXT("允许弟弟自动使用此物"),{1070,868},{538,50},17,TEXT("inventory.autoPermission"),TEXT("inventory.footer"),TEXT("autoPermission")); break;
        }
}

FHearthwardUIElement& UHearthwardScreenWidget::MenuElement(FString Type,FString Text,FVector2D Position,FVector2D Size,float Font,FString Action,bool Selected)
{
    Element(Type,Text,Position,Size,Font,Action,TEXT(""),Selected);
    auto& E=Elements.Last();
    E.Font=Font*GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f;
    E.FontRole=TEXT("display"); E.TextInset=0; E.Tracking=20;
    E.Component=Page.ToString()+(Position.Y<212?TEXT(".header"):Position.Y>=854?TEXT(".footer"):TEXT(".sheet"));
    if(!Action.IsEmpty()) E.LayoutId=Page.ToString()+TEXT(".action.")+Action;
    return E;
}

float UHearthwardScreenWidget::MenuRowHeight() const
{ return 56*GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f; }

int32 UHearthwardScreenWidget::MenuPageSize() const
{ return FMath::Max(3,FMath::FloorToInt(400/MenuRowHeight())); }

void UHearthwardScreenWidget::ComposeMenuChrome(const FString& Heading,const FString& Subtitle)
{
    MenuElement(TEXT("menuSurface"),TEXT(""),{64,212},{1544,634});
    MenuElement(TEXT("text"),Heading,{88,43},{1050,56},36).Color=Color(TEXT("gold"));
    auto& Brand=MenuElement(TEXT("text"),TEXT("HEARTHWARD"),{1190,64},{394,30},18);
    Brand.Align=TEXT("right"); Brand.Tracking=180; Brand.Color=Color(TEXT("muted"));
    MenuElement(TEXT("text"),Message.IsEmpty()?Subtitle:Message,{88,111},{1496,30},18).Color=Color(TEXT("muted"));
    MenuElement(TEXT("line"),TEXT(""),{64,144},{1544,1}).Color=Color(TEXT("bronze"));
    MenuElement(TEXT("line"),TEXT(""),{64,854},{1544,1}).Color=Color(TEXT("bronze"));
    MenuElement(TEXT("menuAction"),TEXT("Esc  返回"),{100,872},{225,43},21,TEXT("back"));
}
