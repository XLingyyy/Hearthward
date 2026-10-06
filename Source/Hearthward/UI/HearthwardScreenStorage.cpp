#include "HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Engine/World.h"

using namespace HearthwardData;

bool UHearthwardScreenWidget::UsesSimpleUI() const
{
    return Page==TEXT("storage") || Page==TEXT("crafting") || Page==TEXT("repairing")
        || Page==TEXT("memory") || Page==TEXT("camp") || Page==TEXT("nature");
}

void UHearthwardScreenWidget::ApplySimpleUIStyle()
{
    if(!UsesSimpleUI()) return;
    for(auto& E:Elements)
    {
        if(E.Type==TEXT("panel")) E.Type=TEXT("inventorySurface");
        else if(E.Type==TEXT("button") || E.Type==TEXT("choice")) E.Type=TEXT("menuAction");
        else if(E.Type==TEXT("tab")) E.Type=TEXT("menuTab");
        else if(E.Type==TEXT("notice")) { E.Type=TEXT("text"); E.Color=Color(TEXT("gold")); }
        E.FontRole=TEXT("body"); E.Tracking=0;
        if(E.Type==TEXT("text")) E.TextInset=0;
    }
}

void UHearthwardScreenWidget::ComposeStorage()
{
    const auto* Bag=Inventory(); const auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    const float Scale=FMath::Clamp(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f,1.f,1.5f);
    auto Add=[&](const TCHAR* Type,FString Label,FVector2D P,FVector2D Size,float Font,const FString& Id,const TCHAR* Component,FString Action=FString(),FString Asset=FString(),bool Selected=false)->FHearthwardUIElement&
    {
        Element(Type,Label,P,Size,Font,Action,Asset,Selected);
        auto& E=Elements.Last(); E.Font=Font*Scale; E.FontRole=TEXT("body"); E.Tracking=0; E.TextInset=0;
        E.LayoutId=Id; E.Component=Component; return E;
    };
    auto Rule=[&](FVector2D P,float Width,const FString& Id,const TCHAR* Component)
    { Add(TEXT("line"),TEXT(""),P,{Width,1},18,Id,Component).Color=Color(TEXT("bronze"))*.65f; };
    Add(TEXT("text"),TEXT("储物箱"),{64,36},{188,56},28,TEXT("storage.heading"),TEXT("storage.header")).Color=Color(TEXT("gold"));
    const TCHAR* Categories[]={TEXT("全部"),TEXT("装备"),TEXT("材料"),TEXT("食物"),TEXT("工具"),TEXT("任务")};
    const TCHAR* Icons[]={TEXT("category_all"),TEXT("category_gear"),TEXT("category_material"),TEXT("category_food"),TEXT("category_tool"),TEXT("category_quest")};
    if(Category.IsEmpty()) Category=TEXT("全部");
    for(int32 I=0;I<6;++I)
    {
        const float X=280+218*I;
        auto& Tab=Add(TEXT("menuTab"),Categories[I],{X,28},{192,62},20,FString::Printf(TEXT("storage.tab.%d"),I),TEXT("storage.header"),TEXT("filter:")+FString(Categories[I]),TEXT(""),Category==Categories[I]); Tab.TextInset=52;
        Add(TEXT("image"),TEXT(""),{X+10,42},{30,30},18,FString::Printf(TEXT("storage.tab.icon.%d"),I),TEXT("storage.header"),TEXT(""),Icons[I]);
    }
    Rule({40,102},1592,TEXT("storage.header.rule"),TEXT("storage.header"));
    Add(TEXT("text"),Message,{64,108},{1544,28},18,TEXT("storage.message"),TEXT("storage.header")).Color=Color(TEXT("gold"));
    Add(TEXT("inventorySurface"),TEXT(""),{40,140},{548,700},18,TEXT("storage.bag.surface"),TEXT("storage.bag"));
    Add(TEXT("inventorySurface"),TEXT(""),{600,140},{472,700},18,TEXT("storage.detail.surface"),TEXT("storage.detail"));
    Add(TEXT("inventorySurface"),TEXT(""),{1084,140},{548,700},18,TEXT("storage.stock.surface"),TEXT("storage.stock"));
    Add(TEXT("text"),TEXT("背包"),{64,164},{160,42},24,TEXT("storage.bag.heading"),TEXT("storage.bag"));
    Add(TEXT("text"),TEXT("储物箱"),{1108,164},{180,42},24,TEXT("storage.stock.heading"),TEXT("storage.stock"));
    Add(TEXT("text"),FString::Printf(TEXT("%.1f / %.0f"),Bag->GetWeight(),Bag->GetCapacity()),{304,168},{254,36},18,TEXT("storage.bag.weight"),TEXT("storage.bag")).Align=TEXT("right");
    Add(TEXT("text"),FString::Printf(TEXT("%.1f / 无限"),Store->GetWeight()),{1354,168},{248,36},18,TEXT("storage.stock.weight"),TEXT("storage.stock")).Align=TEXT("right");
    TArray<TSharedPtr<FJsonObject>> Lists[2];
    for(const auto& V:Rows(TEXT("items")))
    {
        const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id")));
        const FString ItemCategory=Text(R,TEXT("category"))==TEXT("药品")?TEXT("食物"):Text(R,TEXT("category"));
        if(Category!=TEXT("全部") && Category!=ItemCategory) continue;
        if(Bag->GetItemCount(Id)>0) Lists[0].Add(R);
        if(Store->GetItemCount(Id)>0) Lists[1].Add(R);
    }
    for(auto& List:Lists) List.StableSort([](const auto& A,const auto& B){return Number(A,TEXT("displayOrder"))<Number(B,TEXT("displayOrder"));});
    const auto& SelectedList=Lists[StorageToCamp?0:1];
    if(!SelectedList.ContainsByPredicate([&](const auto& R){return FName(*Text(R,TEXT("id")))==SelectedItem;}))
    {SelectedItem=SelectedList.IsEmpty()?NAME_None:FName(*Text(SelectedList[0],TEXT("id"))); Quantity=1;}
    const int32 RowCount=FMath::DivideAndRoundUp(FMath::Max(Lists[0].Num(),Lists[1].Num()),4);
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,RowCount-4));
    for(int32 Side=0;Side<2;++Side)
    {
        const TCHAR* Component=Side?TEXT("storage.stock"):TEXT("storage.bag"); const float X=Side?1108:64;
        Rule({X,212},500,FString::Printf(TEXT("storage.%d.rule"),Side),Component);
        for(int32 Cell=0;Cell<16;++Cell)
        {
            const int32 Index=Scroll*4+Cell;
            const auto R=Lists[Side].IsValidIndex(Index)?Lists[Side][Index]:nullptr;
            const FName Id=R?FName(*Text(R,TEXT("id"))):NAME_None;
            auto& CellElement=Add(TEXT("inventorySlot"),R?FString::FromInt(Side?Store->GetItemCount(Id):Bag->GetItemCount(Id)):TEXT(""),
                {X+(Cell%4)*124,226.+(Cell/4)*124},{112,112},18,FString::Printf(TEXT("storage.%s.cell.%d"),Side?TEXT("stock"):TEXT("bag"),Cell),Component,
                R?(Side?TEXT("withdraw:"):TEXT("deposit:"))+Id.ToString():TEXT(""),R?Text(R,TEXT("icon")):TEXT(""),Id==SelectedItem && !Id.IsNone() && StorageToCamp==(Side==0));
            CellElement.Value=0;
        }
        if(Lists[Side].IsEmpty()) Add(TEXT("text"),TEXT("暂无物品"),{X+96,460},{308,50},24,FString::Printf(TEXT("storage.%d.empty"),Side),Component).Align=TEXT("center");
        Add(TEXT("text"),FString::Printf(TEXT("%d 种物品"),Lists[Side].Num()),{X,768},{200,40},18,FString::Printf(TEXT("storage.%d.total"),Side),Component).Color=Color(TEXT("muted"));
    }
    auto& Prev=Add(TEXT("menuAction"),TEXT("上一页"),{360,758},{98,48},18,TEXT("storage.prev"),TEXT("storage.bag"),TEXT("storage.prev")); Prev.Enabled=Scroll>0;
    auto& Next=Add(TEXT("menuAction"),TEXT("下一页"),{466,758},{98,48},18,TEXT("storage.next"),TEXT("storage.bag"),TEXT("storage.next")); Next.Enabled=Scroll+4<RowCount;
    const auto R=Find(TEXT("items"),SelectedItem.ToString());
    if(R)
    {
        Add(TEXT("text"),Text(R,TEXT("name")),{626,166},{420,52},26,TEXT("storage.detail.name"),TEXT("storage.detail")).Color=Color(TEXT("gold"));
        Add(TEXT("image"),TEXT(""),{762,230},{148,136},18,TEXT("storage.detail.icon"),TEXT("storage.detail"),TEXT(""),Text(R,TEXT("icon")));
        const FSlateFontInfo Font(Typeface,FMath::RoundToInt(18*Scale*.75f));
        const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        TArray<FString> Lines; FString Line=Text(R,TEXT("description"));
        while(Line.Len()>1 && Measure->Measure(Line,Font).X>420)
        {const int32 N=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,420)+1,1,Line.Len());Lines.Add(Line.Left(N));Line=Line.Mid(N);} Lines.Add(Line);
        Add(TEXT("text"),FString::Join(Lines,TEXT("\n")),{626,388},{420,144},18,TEXT("storage.detail.description"),TEXT("storage.detail"));
        const int32 Count=StorageToCamp?Bag->GetItemCount(SelectedItem):Store->GetItemCount(SelectedItem);
        FString Properties=FString::Printf(TEXT("分类   %s\n单重   %.2f\n%s   %d"),*Text(R,TEXT("category")),Number(R,TEXT("weight"))/100,StorageToCamp?TEXT("背包持有"):TEXT("仓储持有"),Count);
        if(!Text(R,TEXT("slot")).IsEmpty())
        {
            const auto& Items=StorageToCamp?Bag->Snapshot().Instances:Store->InventorySnapshot().Instances;
            const auto* Instance=Items.FindByPredicate([&](const auto& I){return I.Definition==SelectedItem;});
            Properties=FString::Printf(TEXT("%s   %d\n%s   %.0f\n耐久   %.0f / %.0f\n单重   %.2f"),StorageToCamp?TEXT("背包持有"):TEXT("仓储持有"),Count,
                Number(R,TEXT("attack"))>0?TEXT("攻击力"):TEXT("防御力"),Number(R,TEXT("attack"))>0?Number(R,TEXT("attack")):Number(R,TEXT("defense")),
                Instance?Instance->Durability:Number(R,TEXT("durability")),Number(R,TEXT("durability")),Number(R,TEXT("weight"))/100);
        }
        Add(TEXT("text"),Properties,{626,520},{420,154},18,TEXT("storage.detail.properties"),TEXT("storage.detail"));
        Add(TEXT("menuAction"),TEXT("−"),{626,686},{64,52},26,TEXT("storage.quantity.less"),TEXT("storage.detail"),TEXT("quantity:-1")).Enabled=Quantity>1;
        Add(TEXT("text"),FString::FromInt(Quantity),{714,694},{242,40},24,TEXT("storage.quantity"),TEXT("storage.detail")).Align=TEXT("center");
        Add(TEXT("menuAction"),TEXT("+"),{980,686},{64,52},26,TEXT("storage.quantity.more"),TEXT("storage.detail"),TEXT("quantity:1")).Enabled=Quantity<Count;
        auto& Transfer=Add(TEXT("menuAction"),StorageToCamp?TEXT("E 存入储物箱"):TEXT("E 取到背包"),{626,764},{420,56},24,TEXT("storage.transfer"),TEXT("storage.detail"),TEXT("transfer")); Transfer.TextInset=14;
    }
    else Add(TEXT("text"),TEXT("选择物品"),{626,388},{420,54},24,TEXT("storage.detail.empty"),TEXT("storage.detail")).Align=TEXT("center");
    Rule({40,854},1592,TEXT("storage.footer.rule"),TEXT("storage.footer"));
    Add(TEXT("menuAction"),TEXT("Esc 返回"),{64,868},{180,50},20,TEXT("storage.back"),TEXT("storage.footer"),TEXT("back"));
    Add(TEXT("text"),TEXT("鼠标选择物品   ·   − / + 调整数量   ·   滚轮浏览"),{650,876},{958,36},18,TEXT("storage.hint"),TEXT("storage.footer")).Align=TEXT("right");
}
