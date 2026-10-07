#include "HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Building/HearthwardWorkshopService.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
using namespace HearthwardData;
namespace
{
UHearthwardInventoryComponent* EquipmentBag(UWorld* World,FName Who,UHearthwardInventoryComponent* Player)
{
    if(Who==TEXT("player"))return Player;
    if(Who==TEXT("brother"))for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It)return It->Bag;
    return nullptr;
}
}
FString UHearthwardScreenWidget::EquipmentActionToken(const FString& Action) const
{
    return Action+TEXT("|")+EquipmentEpoch.ToString(EGuidFormats::Digits)+TEXT("|")+EquipmentOwner.ToString()+TEXT("|")
        +(EquipmentSelection.IsValid()?EquipmentSelection.ToString(EGuidFormats::Digits):EquipmentStack.ToString())+TEXT("|")
        +FString::FromInt(Quantity)+TEXT("|")+EquipmentOperation.ToString(EGuidFormats::Digits);
}
void UHearthwardScreenWidget::ComposeEquipment()
{
    auto* Bag=EquipmentBag(GetWorld(),EquipmentOwner,Inventory());
    const auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();const FHearthwardInventorySnapshot Empty;
    const auto& Items=Bag?Bag->Snapshot():EquipmentOwner==TEXT("storage")?Store->InventorySnapshot():Empty;
    const float TextScale=FMath::Clamp(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f,1.f,1.5f);
    const int32 PageSize=TextScale>1.25f?6:8;
    auto Gear=[&](FString Type,FString Label,FVector2D P,FVector2D Size,float Font,FString Id,FString Component,FString Action=FString(),FString Asset=FString(),bool Selected=false)->FHearthwardUIElement&
    {
        Element(Type,Label,P,Size,Font,Action,Asset,Selected);
        auto& E=Elements.Last();E.Font=Font*TextScale;E.FontRole=TEXT("body");E.Tracking=0;E.TextInset=0;
        E.LayoutId=Id;E.Component=Component;return E;
    };
    auto Rule=[&](float X,float Y,float Width,const FString& Id,const FString& Component)
    {Gear(TEXT("line"),TEXT(""),{X,Y},{Width,1},18,Id,Component).Color=Color(TEXT("bronze"))*.65f;};
    Gear(TEXT("inventorySurface"),TEXT(""),{40,140},{548,700},18,TEXT("equipment.list.surface"),TEXT("equipment.list"));
    Gear(TEXT("inventorySurface"),TEXT(""),{600,140},{566,700},18,TEXT("equipment.detail.surface"),TEXT("equipment.detail"));
    Gear(TEXT("inventorySurface"),TEXT(""),{1178,140},{454,700},18,TEXT("equipment.actions.surface"),TEXT("equipment.actions"));
    Gear(TEXT("text"),TEXT("行装管理"),{64,36},{196,56},28,TEXT("equipment.heading"),TEXT("equipment.header")).Color=Color(TEXT("gold"));
    int32 Tab=0;
    for(const auto Who:{TEXT("player"),TEXT("brother"),TEXT("storage")})
    {
        Gear(TEXT("menuTab"),FString(Who)==TEXT("player")?TEXT("我的背包"):FString(Who)==TEXT("brother")?TEXT("弟弟背包"):TEXT("营地仓储"),
            {280.f+Tab++*214,28},{188,62},21,FString(TEXT("equipment.owner."))+Who,TEXT("equipment.header"),TEXT("gear.owner:")+FString(Who),TEXT(""),EquipmentOwner==Who).Align=TEXT("center");
    }
    Gear(TEXT("text"),Bag?FString::Printf(TEXT("负重 %.2f / %.0f · 背包 %d 阶"),Bag->GetWeight(),Bag->GetCapacity(),Bag->BackpackRank()):TEXT("共享仓储"),
        {984,43},{428,40},17,TEXT("equipment.weight"),TEXT("equipment.header")).Align=TEXT("right");
    Gear(TEXT("menuAction"),TEXT("返回背包"),{1432,31},{176,56},20,TEXT("equipment.return"),TEXT("equipment.header"),TEXT("page:inventory")).Align=TEXT("center");
    Rule(40,102,1592,TEXT("equipment.header.rule"),TEXT("equipment.header"));
    Gear(TEXT("text"),Message.IsEmpty()?TEXT("HEARTHWARD"):Message,{64,108},{1544,28},14,TEXT("equipment.message"),TEXT("equipment.header")).Color=Color(TEXT("muted"));

    TArray<FHearthwardItemInstance> Instances=Items.Instances;
    Instances.Sort([](const auto& A,const auto& B){return A.Definition==B.Definition?A.Id<B.Id:A.Definition.LexicalLess(B.Definition);});
    TArray<FName> Stacks;Items.Stacks.GetKeys(Stacks);Stacks.Sort(FNameLexicalLess());
    const int32 Total=Instances.Num()+Stacks.Num();Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Total-PageSize));
    if(!EquipmentOperation.IsValid())EquipmentOperation=FGuid::NewGuid();
    if(!EquipmentSelection.IsValid() && EquipmentStack.IsNone())
    {EquipmentSelection=Instances.IsEmpty()?FGuid():Instances[0].Id;EquipmentStack=Instances.IsEmpty() && !Stacks.IsEmpty()?Stacks[0]:NAME_None;}
    Gear(TEXT("text"),TEXT("装备与物资"),{64,154},{292,46},24,TEXT("equipment.list.heading"),TEXT("equipment.list")).Color=Color(TEXT("gold"));
    Gear(TEXT("text"),Total?FString::Printf(TEXT("%d–%d / %d"),Scroll+1,FMath::Min(Total,Scroll+PageSize),Total):TEXT("0 件"),
        {358,163},{200,34},17,TEXT("equipment.list.range"),TEXT("equipment.list")).Align=TEXT("right");
    Rule(64,204,500,TEXT("equipment.list.rule"),TEXT("equipment.list"));
    for(int32 Index=Scroll;Index<FMath::Min(Total,Scroll+PageSize);++Index)
    {
        const auto* I=Index<Instances.Num()?&Instances[Index]:nullptr;
        const FName Item=I?I->Definition:Stacks[Index-Instances.Num()];const auto Row=Find(TEXT("items"),Item.ToString());
        const float Y=228+(Index-Scroll)*(PageSize==6?88:68);
        const bool Selected=I?EquipmentSelection==I->Id:EquipmentStack==Item;
        const FString Id=TEXT("equipment.list.item.")+(I?I->Id.ToString():Item.ToString());
        auto& Entry=Gear(TEXT("menuRow"),TEXT(""),{64,Y},{500,64},18,Id,TEXT("equipment.list"),
            I?TEXT("gear.select:")+I->Id.ToString():TEXT("gear.stack:")+Item.ToString(),TEXT(""),Selected);
        Entry.InventoryItem=Item;Entry.InventoryInstance=I?I->Id:FGuid();
        Gear(TEXT("image"),TEXT(""),{72,Y+5},{50,50},18,Id+TEXT(".icon"),TEXT("equipment.list"),TEXT(""),Text(Row,TEXT("icon")));
        Gear(TEXT("text"),Text(Row,TEXT("name")),{136,Y+2},{422,28},18,Id+TEXT(".name"),TEXT("equipment.list")).Color=Color(Selected?TEXT("gold"):TEXT("text"));
        FString State=I?FString::Printf(TEXT("实例 %s · %s"),*I->Id.ToString(EGuidFormats::Digits).Left(8),Bag && Bag->IsEquipped(I->Id)?TEXT("已装备"):TEXT("未装备")):FString::Printf(TEXT("持有 ×%d"),Items.Stacks[Item]);
        if(I && Number(Row,TEXT("durability"))>0)State+=FString::Printf(TEXT(" · %.2f / %.0f"),I->Durability,Number(Row,TEXT("durability")));
        Gear(TEXT("text"),State,{136,Y+32},{422,28},14,Id+TEXT(".state"),TEXT("equipment.list")).Color=Color(TEXT("muted"));
        Rule(64,Y+65,500,Id+TEXT(".rule"),TEXT("equipment.list"));
    }
    if(!Total)Gear(TEXT("text"),TEXT("尚无物品"),{64,244},{500,50},20,TEXT("equipment.list.empty"),TEXT("equipment.list")).Color=Color(TEXT("muted"));
    auto& Prev=Gear(TEXT("menuAction"),TEXT("‹ 上一页"),{72,804},{174,34},16,TEXT("equipment.prev"),TEXT("equipment.list"),TEXT("gear.prev"));Prev.Enabled=Scroll>0;
    auto& Next=Gear(TEXT("menuAction"),TEXT("下一页 ›"),{380,804},{174,34},16,TEXT("equipment.next"),TEXT("equipment.list"),TEXT("gear.next"));Next.Enabled=Scroll+PageSize<Total;Next.Align=TEXT("right");

    const auto* Selected=Items.Instances.FindByPredicate([&](const auto& I){return I.Id==EquipmentSelection;});
    const FName ManagedItem=Selected?Selected->Definition:EquipmentStack;
    const auto Row=Find(TEXT("items"),ManagedItem.ToString());
    Gear(TEXT("text"),Row?Text(Row,TEXT("name")):TEXT("物品详情"),{624,154},{518,62},26,TEXT("equipment.detail.name"),TEXT("equipment.detail")).Color=Color(TEXT("gold"));
    Rule(624,220,518,TEXT("equipment.detail.rule"),TEXT("equipment.detail"));
    if(Row)
    {
        Gear(TEXT("text"),TEXT("类型  ")+Text(Row,TEXT("category")),{624,238},{252,38},17,TEXT("equipment.detail.category"),TEXT("equipment.detail")).Color=Color(TEXT("muted"));
        const int32 TotalCount=Bag?Bag->GetItemCount(ManagedItem):GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetItemCount(ManagedItem);
        const int32 Available=Bag?Bag->Available(ManagedItem):GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Available(ManagedItem);
        Gear(TEXT("text"),FString::Printf(TEXT("持有 %d · 可用 %d"),TotalCount,Available),
            {888,238},{254,38},17,TEXT("equipment.detail.state"),TEXT("equipment.detail")).Align=TEXT("right");
        Gear(TEXT("image"),TEXT(""),{754,282},{260,182},18,TEXT("equipment.detail.art"),TEXT("equipment.detail"),TEXT(""),ManagedItem==TEXT("axe")?TEXT("axeLarge"):Text(Row,TEXT("icon")));
        Gear(TEXT("text"),TEXT("物品说明"),{624,468},{518,42},21,TEXT("equipment.detail.description.heading"),TEXT("equipment.detail")).Color=Color(TEXT("gold"));
        Rule(624,513,518,TEXT("equipment.detail.description.rule"),TEXT("equipment.detail"));
        const FString Description=(Selected?TEXT("实例 ")+Selected->Id.ToString(EGuidFormats::Digits).Left(8)
            +(Bag && Bag->IsEquipped(Selected->Id)?TEXT(" · 已装备\n"):TEXT(" · 未装备\n")):FString())+Text(Row,TEXT("description"));
        Gear(TEXT("text"),Description,{624,529},{518,96},17,TEXT("equipment.detail.description"),TEXT("equipment.detail"));
        Gear(TEXT("text"),TEXT("物品属性"),{624,638},{518,40},21,TEXT("equipment.detail.properties.heading"),TEXT("equipment.detail")).Color=Color(TEXT("gold"));
        TArray<TPair<FString,FString>> Properties;
        if(Number(Row,TEXT("attack"))>0)Properties.Emplace(TEXT("攻击力"),FString::Printf(TEXT("%.0f"),Number(Row,TEXT("attack"))));
        if(Number(Row,TEXT("defense"))>0)Properties.Emplace(TEXT("防御力"),FString::Printf(TEXT("%.0f"),Number(Row,TEXT("defense"))));
        if(Selected && Number(Row,TEXT("durability"))>0)Properties.Emplace(TEXT("耐久度"),FString::Printf(TEXT("%.2f / %.0f"),Selected->Durability,Number(Row,TEXT("durability"))));
        Properties.Emplace(TEXT("单重"),FString::Printf(TEXT("%.2f"),Number(Row,TEXT("weight"))/100));
        for(int32 Index=0;Index<Properties.Num();++Index)
        {
            const float Y=682+Index*38;
            Gear(TEXT("text"),Properties[Index].Key,{624,Y},{248,37},18,TEXT("equipment.detail.property.")+FString::FromInt(Index)+TEXT(".label"),TEXT("equipment.detail"));
            Gear(TEXT("text"),Properties[Index].Value,{894,Y},{248,37},18,TEXT("equipment.detail.property.")+FString::FromInt(Index)+TEXT(".value"),TEXT("equipment.detail")).Align=TEXT("right");
            Rule(624,Y+37,518,TEXT("equipment.detail.property.")+FString::FromInt(Index)+TEXT(".rule"),TEXT("equipment.detail"));
        }
    }
    else Gear(TEXT("text"),TEXT("选择左侧物品查看说明"),{624,292},{518,64},21,TEXT("equipment.detail.empty"),TEXT("equipment.detail")).Color=Color(TEXT("muted"));

    Gear(TEXT("text"),TEXT("操作与背包"),{1202,154},{406,46},24,TEXT("equipment.actions.heading"),TEXT("equipment.actions")).Color=Color(TEXT("gold"));
    Rule(1202,204,406,TEXT("equipment.actions.rule"),TEXT("equipment.actions"));
    Gear(TEXT("text"),EquipmentOwner==TEXT("player")?TEXT("从我的背包转出"):EquipmentOwner==TEXT("brother")?TEXT("从弟弟背包转出"):TEXT("从仓储转出"),{1202,222},{406,36},19,TEXT("equipment.transfer.heading"),TEXT("equipment.actions")).Color=Color(TEXT("gold"));
    FString Preview;
    const int32 TransferCount=Selected?1:Quantity;
    for(const FName Who:{FName(TEXT("player")),FName(TEXT("brother")),FName(TEXT("storage"))})if(Who!=EquipmentOwner)
    {
        const auto* TargetBag=EquipmentBag(GetWorld(),Who,Inventory());
        const auto& TargetItems=TargetBag?TargetBag->Snapshot():Who==TEXT("storage")?Store->InventorySnapshot():Empty;
        FHearthwardInventoryState SourceAfter(EquipmentOwner==TEXT("storage")),TargetAfter(Who==TEXT("storage"));
        const int32 Available=Bag?Bag->Available(ManagedItem):EquipmentOwner==TEXT("storage")?Store->Available(ManagedItem):0;
        EHearthwardInventoryResult Result=EHearthwardInventoryResult::InsufficientItems;
        if(Who!=TEXT("storage") && !TargetBag)Result=EHearthwardInventoryResult::InvalidArgument;
        else if(TransferCount<=Available && SourceAfter.Restore(Items) && TargetAfter.Restore(TargetItems))
            Result=Selected?SourceAfter.TransferInstanceTo(TargetAfter,Selected->Id):SourceAfter.TransferTo(TargetAfter,ManagedItem,TransferCount);
        const FString TargetName=Who==TEXT("player")?TEXT("我的背包"):Who==TEXT("brother")?TEXT("弟弟背包"):TEXT("仓储");
        Preview+=FString::Printf(TEXT("→ %s：%d件 · %s\n"),*TargetName,TransferCount,
            Result!=EHearthwardInventoryResult::Success?(Result==EHearthwardInventoryResult::InvalidArgument?TEXT("目标不可用"):Result==EHearthwardInventoryResult::CapacityExceeded?TEXT("容量不足"):TEXT("可用不足")):
            Who==TEXT("storage")?TEXT("不限重量"):*FString::Printf(TEXT("%.1f/%.0f"),TargetAfter.GetWeightHundredths()/100.,TargetBag?TargetBag->GetCapacity():0));
    }
    Gear(TEXT("text"),Preview,{1202,262},{406,72},14,TEXT("equipment.transfer.help"),TEXT("equipment.actions")).Color=Color(TEXT("muted"));
    int32 Target=0;
    if(EquipmentAppliedTransfer==EquipmentActionToken(TEXT("gear.transfer")))
        Gear(TEXT("menuAction"),TEXT("新转移"),{1202,347},{406,44},18,TEXT("equipment.newTransfer"),TEXT("equipment.actions"),TEXT("gear.newTransfer")).Align=TEXT("center");
    else for(const auto Who:{TEXT("player"),TEXT("brother"),TEXT("storage")})if(EquipmentOwner!=Who)
        Gear(TEXT("menuAction"),FString(Who)==TEXT("player")?TEXT("交给我"):FString(Who)==TEXT("brother")?TEXT("交给弟弟"):TEXT("放入仓储"),
            {1202.f+Target++*209,347},{197,44},18,FString(TEXT("equipment.transfer."))+Who,TEXT("equipment.actions"),EquipmentActionToken(TEXT("gear.to:")+FString(Who))).Align=TEXT("center");
    if(Row && !Selected)
    {
        Gear(TEXT("menuAction"),TEXT("−"),{1202,410},{70,44},20,TEXT("equipment.quantity.minus"),TEXT("equipment.actions"),TEXT("gear.minus")).Align=TEXT("center");Elements.Last().Enabled=Quantity>1;
        Gear(TEXT("text"),FString::FromInt(Quantity),{1284,414},{240,36},20,TEXT("equipment.quantity"),TEXT("equipment.actions")).Align=TEXT("center");
        Gear(TEXT("menuAction"),TEXT("＋"),{1538,410},{70,44},20,TEXT("equipment.quantity.plus"),TEXT("equipment.actions"),TEXT("gear.plus")).Align=TEXT("center");
        Elements.Last().Enabled=Quantity<(Bag?Bag->Available(ManagedItem):GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Available(ManagedItem));
    }
    else if(Selected && Bag)
    {
        Gear(TEXT("menuAction"),Bag->IsEquipped(Selected->Id)?TEXT("卸下选中装备"):TEXT("装备选中物品"),{1202,410},{406,44},20,TEXT("equipment.equip"),TEXT("equipment.actions"),EquipmentActionToken(TEXT("gear.equip"))).Align=TEXT("center");
        if(EquipmentOwner==TEXT("player"))
        {
            Gear(TEXT("text"),TEXT("装备维修"),{1202,472},{406,36},19,TEXT("equipment.repair.heading"),TEXT("equipment.actions")).Color=Color(TEXT("gold"));
            int32 Index=0;
            for(double Fraction:{.25,.5,1.})
            {
                const FString Percent=FString::FromInt(FMath::RoundToInt(Fraction*100));
                const FString Label=Fraction==1?HearthwardInput::Label(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings,TEXT("repair.commit"))+TEXT(" 全部修复"):TEXT("修复 ")+Percent+TEXT("%");
                Gear(TEXT("menuAction"),Label,{1202.f+Index++*139,510},{128,44},16,TEXT("equipment.repair.")+Percent,TEXT("equipment.actions"),EquipmentActionToken(TEXT("gear.repair:")+Percent)).Align=TEXT("center");
            }
            FString Quotes;
            for(double Fraction:{.25,.5,1.})
            {
                TMap<FName,int32> Cost;double Restored;
                if(HearthwardWorkshop::RepairQuote(Bag,Selected->Id,Fraction,Cost,Restored))
                {
                    Quotes+=FString::Printf(TEXT("%.0f%% 恢复%.2f："),Fraction*100,Restored);
                    for(const auto& M:Cost)Quotes+=Text(Find(TEXT("items"),M.Key.ToString()),TEXT("name"))+FString::Printf(TEXT("×%d "),M.Value);
                    Quotes+=TEXT("\n");
                }
            }
            Gear(TEXT("text"),Quotes.IsEmpty()?TEXT("当前装备无需修复或暂无维修配方"):Quotes,{1202,564},{406,104},14,TEXT("equipment.repair.quote"),TEXT("equipment.actions")).Color=Color(TEXT("muted"));
            Gear(TEXT("menuAction"),TEXT("将选中装备放到地面"),{1202,682},{406,42},18,TEXT("equipment.drop"),TEXT("equipment.actions"),Selected->UniqueClaim.IsNone()?EquipmentActionToken(TEXT("gear.drop")):TEXT("ask:")+EquipmentActionToken(TEXT("gear.dropConfirmed"))).Align=TEXT("center");
        }
    }
    else if(Selected)Gear(TEXT("text"),TEXT("仓储装备需先转入个人背包"),{1202,410},{406,64},18,TEXT("equipment.storage.help"),TEXT("equipment.actions")).Color=Color(TEXT("muted"));
    if(Bag)
    {
        const auto NextRank=Find(TEXT("backpacks"),FString::FromInt(Bag->BackpackRank()+1));
        FString Cost=TEXT("背包已满级");
        if(NextRank)
        {
            Cost=FString::Printf(TEXT("下一阶 %.0f 容量 · 营地 %.0f 阶\n"),Number(NextRank,TEXT("capacity")),Number(NextRank,TEXT("minimum_camp_tier")));
            for(const auto& M:NextRank->GetObjectField(TEXT("incremental_cost"))->Values)Cost+=Text(Find(TEXT("items"),FString(*M.Key)),TEXT("name"))+FString::Printf(TEXT("×%.0f  "),M.Value->AsNumber());
        }
        Gear(TEXT("text"),Cost,{1202,734},{406,50},14,TEXT("equipment.upgrade.cost"),TEXT("equipment.actions")).Color=Color(TEXT("muted"));
        Gear(TEXT("menuAction"),TEXT("找后勤员升级此背包"),{1202,792},{406,36},16,TEXT("equipment.upgrade"),TEXT("equipment.actions"),EquipmentActionToken(TEXT("gear.upgrade"))).Align=TEXT("center");
    }
    Rule(40,854,1592,TEXT("equipment.footer.rule"),TEXT("equipment.footer"));
    Gear(TEXT("menuAction"),TEXT("Esc 返回背包"),{64,868},{260,50},20,TEXT("equipment.back"),TEXT("equipment.footer"),TEXT("back"));
    Gear(TEXT("text"),TEXT("逐件装备 · 转交与维修"),{1128,875},{480,40},17,TEXT("equipment.footer.help"),TEXT("equipment.footer")).Align=TEXT("right");
}
bool UHearthwardScreenWidget::ExecuteEquipmentAction(const FString& InAction)
{
    if(Page!=TEXT("equipment"))return false;
    const int32 Separator=InAction.Find(TEXT("|"));const FString Action=Separator==INDEX_NONE?InAction:InAction.Left(Separator);
    if(Separator!=INDEX_NONE && InAction!=EquipmentActionToken(Action))
    {Message=TEXT("装备操作卡片已过期，请重新选择实例");Refresh();return false;}
    auto* G=Gameplay();bool OK=true;
    if(EquipmentOwner==TEXT("brother") && !EquipmentBag(GetWorld(),EquipmentOwner,Inventory()) && !Action.StartsWith(TEXT("gear.owner:")))
    {Message=TEXT("弟弟背包不可用，请重新选择容器");Refresh();return false;}
    const auto NewPreview=[&](){EquipmentOperation=FGuid::NewGuid();EquipmentAppliedTransfer.Reset();};
    if(Action.StartsWith(TEXT("gear.owner:")))
    {
        const FName Owner(*Action.Mid(11));if(Owner!=TEXT("player") && Owner!=TEXT("brother") && Owner!=TEXT("storage"))return false;
        EquipmentOwner=Owner;Scroll=0;EquipmentSelection.Invalidate();EquipmentStack=NAME_None;NewPreview();
    }
    else if(Action.StartsWith(TEXT("gear.select:")))
    {
        FGuid Id;if(!FGuid::Parse(Action.Mid(12),Id))return false;
        const auto* Bag=EquipmentBag(GetWorld(),EquipmentOwner,Inventory());
        const auto& Snapshot=Bag?Bag->Snapshot():GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->InventorySnapshot();
        if(!Snapshot.Instances.ContainsByPredicate([&](const auto& I){return I.Id==Id;}))return false;
        EquipmentSelection=Id;EquipmentStack=NAME_None;Quantity=1;NewPreview();
    }
    else if(Action.StartsWith(TEXT("gear.stack:")))
    {
        const FName Item(*Action.Mid(11));const auto* Bag=EquipmentBag(GetWorld(),EquipmentOwner,Inventory());
        const auto& Snapshot=Bag?Bag->Snapshot():GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->InventorySnapshot();
        if(!Snapshot.Stacks.Contains(Item))return false;
        EquipmentStack=Item;EquipmentSelection.Invalidate();Quantity=1;NewPreview();
    }
    else if(Action==TEXT("gear.prev"))Scroll-=(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale>125?6:8);
    else if(Action==TEXT("gear.next"))Scroll+=(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale>125?6:8);
    else if(Action==TEXT("gear.minus") || Action==TEXT("gear.plus"))
    {
        const auto* Bag=EquipmentBag(GetWorld(),EquipmentOwner,Inventory());
        const int32 Available=Bag?Bag->Available(EquipmentStack):GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Available(EquipmentStack);
        Quantity=FMath::Clamp(Quantity+(Action==TEXT("gear.plus")?1:-1),1,FMath::Max(1,Available));NewPreview();
    }
    else if(Action==TEXT("gear.newTransfer"))NewPreview();
    else if(Action.StartsWith(TEXT("gear.to:")))
    {
        const FString Key=EquipmentActionToken(TEXT("gear.transfer"));
        if(EquipmentAppliedTransfer==Key){Message=TEXT("本次转交已处理，请重新选择物品或新转移");Refresh();return false;}
        const FName Target(*Action.Mid(8));if(Target!=TEXT("player") && Target!=TEXT("brother") && Target!=TEXT("storage"))return false;
        auto* TargetBag=EquipmentBag(GetWorld(),Target,Inventory());auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        const auto* SourceBag=EquipmentBag(GetWorld(),EquipmentOwner,Inventory());
        const auto& SourceItems=SourceBag?SourceBag->Snapshot():Store->InventorySnapshot();
        const auto* Selected=SourceItems.Instances.FindByPredicate([&](const auto& I){return I.Id==EquipmentSelection;});
        const FName Item=Selected?Selected->Definition:EquipmentStack;
        const int32 Before=TargetBag?TargetBag->GetItemCount(Item):Store->GetItemCount(Item);
        OK=G->TransferInventory(EquipmentOwner,Target,EquipmentStack,Quantity,EquipmentSelection,EquipmentEpoch);
        if(OK)
        {
            EquipmentAppliedTransfer=Key;
            const int32 Moved=(TargetBag?TargetBag->GetItemCount(Item):Store->GetItemCount(Item))-Before;
            Message=FString::Printf(TEXT("已转移 %s ×%d · 实例与耐久保留"),*Text(Find(TEXT("items"),Item.ToString()),TEXT("name")),Moved);
            Refresh();return true;
        }
    }
    else if(Action==TEXT("gear.upgrade"))OK=G->UpgradeBackpack(EquipmentOwner==TEXT("brother"),EquipmentEpoch);
    else if(Action==TEXT("gear.equip"))
    {
        if(EquipmentEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())return false;
        if(EquipmentOwner==TEXT("brother"))OK=G->EquipBrother(EquipmentSelection,EquipmentEpoch);
        else {const FGuid Selected=EquipmentSelection;OpenPage(TEXT("hud"));OK=G->EquipInstance(Selected);}
    }
    else if(Action.StartsWith(TEXT("gear.repair:")))
    {
        auto* B=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>();FGuid Station;
        const auto* I=Inventory()->FindInstance(EquipmentSelection);
        const auto R=I?Find(TEXT("repairRecipes"),I->Definition.ToString()):nullptr;
        if(R)for(const auto& F:GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Facilities)
            if(B->MatchesFacility(F.Id,FName(*Text(R,TEXT("facility"))),Number(R,TEXT("facilityLevel")))){Station=F.Id;break;}
        OK=B->RepairInstance(Station,EquipmentSelection,FCString::Atod(*Action.Mid(12))/100,EquipmentEpoch);Message=B->Feedback;Refresh();return OK;
    }
    else if(Action==TEXT("gear.drop") || Action==TEXT("gear.dropConfirmed"))OK=G->DropEquipment(EquipmentSelection,EquipmentEpoch,Action==TEXT("gear.dropConfirmed"));
    else return false;
    Message=G->Feedback;Refresh();return OK;
}
