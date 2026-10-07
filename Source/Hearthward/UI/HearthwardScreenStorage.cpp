#include "HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Engine/World.h"

using namespace HearthwardData;
namespace
{
FString TransferReason(EHearthwardInventoryResult Result)
{
    using R=EHearthwardInventoryResult;
    switch(Result)
    {
    case R::Success:return FString();
    case R::InvalidCount:return TEXT("数量必须为正整数");
    case R::InsufficientItems:return TEXT("当前可用数量不足，物品保留原处");
    case R::CapacityExceeded:return TEXT("转移后超过背包容量，物品保留原处");
    case R::QuantityOverflow:return TEXT("目标数量已达上限，物品保留原处");
    case R::StaleTimeline:return TEXT("存档时间线已变化，请重新打开仓储");
    case R::OperationConflict:return TEXT("转移预览已变化，请重新选择");
    default:return TEXT("物品或实例已失效，请重新选择");
    }
}
}

void UHearthwardScreenWidget::ResetStorageTransfer()
{
    StorageOperation=FGuid::NewGuid();StorageTransferCommitted=false;StorageDetailScroll=0;
}

FString UHearthwardScreenWidget::StorageTransferToken() const
{
    return TEXT("transfer:")+StorageEpoch.ToString(EGuidFormats::Digits)+TEXT(":")+StorageOperation.ToString(EGuidFormats::Digits);
}

FString UHearthwardScreenWidget::StorageTransferStatus(double& AfterWeight) const
{
    const auto* Bag=Inventory();const auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    AfterWeight=Bag?Bag->GetWeight():0;
    if(!Bag || !Store)return TEXT("背包或仓储不可用");
    if(StorageEpoch!=Store->GetTimelineEpoch())return TEXT("存档时间线已变化，请重新打开仓储");
    if(!Gameplay()->NearStorage())return TEXT("请靠近可用的营地仓储设施");
    if(!Gameplay()->CanChangeSkills())return TEXT("请先脱战并结束当前动作，再转移物品");
    if(SelectedItem.IsNone())return TEXT("选择要转移的物品或具体装备");
    if(StorageTransferCommitted)return TEXT("本次转移已处理；选择物品或点击新转移后可再提交");
    if(Quantity<=0 || (StorageSelection.IsValid() && Quantity!=1))return TEXT("数量必须为正整数，实例装备每次1件");
    const int32 Available=StorageToCamp?Bag->Available(SelectedItem):Store->Available(SelectedItem);
    if(Quantity>Available)return TEXT("当前可用数量不足，物品保留原处");
    FHearthwardInventoryState BagAfter,StockAfter(true);
    if(!BagAfter.Restore(Bag->Snapshot()) || !StockAfter.Restore(Store->InventorySnapshot()))return TEXT("容器状态不可用");
    auto& Source=StorageToCamp?BagAfter:StockAfter;auto& Target=StorageToCamp?StockAfter:BagAfter;
    const auto* Definition=FHearthwardInventoryState::FindItem(SelectedItem);
    if(Definition && Definition->IsInstance())
    {
        const auto* Instance=Source.FindInstance(StorageSelection);
        if(!Instance || Instance->Definition!=SelectedItem)return TEXT("所选装备已离开此容器，请重新选择具体实例");
    }
    const auto Result=StorageSelection.IsValid()?Source.TransferInstanceTo(Target,StorageSelection):Source.TransferTo(Target,SelectedItem,Quantity);
    if(Result!=EHearthwardInventoryResult::Success)return TransferReason(Result);
    AfterWeight=BagAfter.GetWeightHundredths()/100.;return FString();
}

bool UHearthwardScreenWidget::ExecuteStorageAction(const FString& Action)
{
    if(Page!=TEXT("storage"))return false;
    auto* Bag=Inventory();auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if(StorageEpoch!=Store->GetTimelineEpoch())
    {Message=TEXT("存档时间线已变化，请重新打开仓储");Refresh();return false;}
    if(Action==TEXT("storage.prev") || Action==TEXT("storage.next"))
        Scroll=FMath::Max(0,Scroll+(Action==TEXT("storage.next")?4:-4));
    else if(Action.StartsWith(TEXT("depositInstance:")) || Action.StartsWith(TEXT("withdrawInstance:")))
    {
        const bool ToCamp=Action.StartsWith(TEXT("depositInstance:"));FGuid Id;
        if(!FGuid::Parse(Action.Mid(ToCamp?16:17),Id))return false;
        const auto& Instances=ToCamp?Bag->Snapshot().Instances:Store->InventorySnapshot().Instances;
        const auto* Instance=Instances.FindByPredicate([&](const auto& I){return I.Id==Id;});
        if(!Instance){Message=TEXT("所选装备已离开此容器，请重新选择");Refresh();return false;}
        SelectedItem=Instance->Definition;StorageSelection=Id;StorageToCamp=ToCamp;Quantity=1;ResetStorageTransfer();Message.Reset();
    }
    else if(Action.StartsWith(TEXT("deposit:")) || Action.StartsWith(TEXT("withdraw:")))
    {
        const bool ToCamp=Action.StartsWith(TEXT("deposit:"));const FName Item(*Action.Mid(ToCamp?8:9));
        const auto* Definition=FHearthwardInventoryState::FindItem(Item);
        if(!Definition || (ToCamp?Bag->GetItemCount(Item):Store->GetItemCount(Item))<=0)return false;
        if(Definition->IsInstance())
        {Message=TEXT("请点击具体装备格子，逐件选择实例");Refresh();return false;}
        SelectedItem=Item;StorageSelection.Invalidate();StorageToCamp=ToCamp;Quantity=1;ResetStorageTransfer();Message.Reset();
    }
    else if(Action.StartsWith(TEXT("quantity:")))
    {
        if(StorageSelection.IsValid())return false;
        const int32 Available=StorageToCamp?Bag->Available(SelectedItem):Store->Available(SelectedItem);
        const int64 Requested=int64(Quantity)+FCString::Atoi64(*Action.Mid(9));
        Quantity=int32(FMath::Clamp<int64>(Requested,1,FMath::Max(1,Available)));ResetStorageTransfer();
    }
    else if(Action==TEXT("storage.detail.prev") || Action==TEXT("storage.detail.next"))
        StorageDetailScroll=FMath::Max(0,StorageDetailScroll+(Action==TEXT("storage.detail.next")?3:-3));
    else if(Action==TEXT("storage.newTransfer"))ResetStorageTransfer();
    else if(Action==TEXT("transfer") || Action.StartsWith(TEXT("transfer:")))
    {
        if(Action!=TEXT("transfer") && Action!=StorageTransferToken())
        {Message=TEXT("转移卡片已过期，请重新选择");Refresh();return false;}
        double AfterWeight;const FString Reason=StorageTransferStatus(AfterWeight);
        if(!Reason.IsEmpty()){Message=Reason;Refresh();return false;}
        const auto Reply=StorageSelection.IsValid()?Store->TransferInstance(Bag,StorageToCamp,StorageSelection,StorageOperation,StorageEpoch)
            :Store->Transfer(Bag,StorageToCamp,SelectedItem,Quantity,StorageOperation,StorageEpoch);
        const bool Success=Reply.Result==EHearthwardInventoryResult::Success;
        StorageTransferCommitted=Success;
        Message=Success?(Reply.Replayed?TEXT("本次转移已处理，未重复移动物品"):FString::Printf(TEXT("%s %s ×%d · 当前负重 %.2f / %.0f"),
            StorageToCamp?TEXT("已存入"):TEXT("已取出"),*Text(Find(TEXT("items"),SelectedItem.ToString()),TEXT("name")),Reply.MovedCount,Bag->GetWeight(),Bag->GetCapacity())):TransferReason(Reply.Result);
        Refresh();return Success;
    }
    else return false;
    Refresh();return true;
}

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
    struct FEntry {TSharedPtr<FJsonObject> Row;FGuid Instance;int32 Count;};
    TArray<FEntry> Lists[2];
    for(const auto& V:Rows(TEXT("items")))
    {
        const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id")));
        const FString ItemCategory=Text(R,TEXT("category"))==TEXT("药品")?TEXT("食物"):Text(R,TEXT("category"));
        if(Category!=TEXT("全部") && Category!=ItemCategory) continue;
        for(int32 Side=0;Side<2;++Side)
        {
            const auto& Snapshot=Side?Store->InventorySnapshot():Bag->Snapshot();
            if(const auto* Count=Snapshot.Stacks.Find(Id);Count && *Count>0)Lists[Side].Add({R,FGuid(),*Count});
            for(const auto& Instance:Snapshot.Instances)if(Instance.Definition==Id)Lists[Side].Add({R,Instance.Id,1});
        }
    }
    for(auto& List:Lists)List.StableSort([](const auto& A,const auto& B)
    {const double OrderA=Number(A.Row,TEXT("displayOrder")),OrderB=Number(B.Row,TEXT("displayOrder"));return OrderA==OrderB?A.Instance<B.Instance:OrderA<OrderB;});
    if(StorageViewEpoch!=StorageEpoch)
    {
        StorageViewEpoch=StorageEpoch;StorageToCamp=true;StorageSelection.Invalidate();SelectedItem=NAME_None;Quantity=1;ResetStorageTransfer();
        if(!Lists[0].IsEmpty()){SelectedItem=FName(*Text(Lists[0][0].Row,TEXT("id")));StorageSelection=Lists[0][0].Instance;}
    }
    const int32 RowCount=FMath::DivideAndRoundUp(FMath::Max(Lists[0].Num(),Lists[1].Num()),4);
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,RowCount-4));
    for(int32 Side=0;Side<2;++Side)
    {
        const TCHAR* Component=Side?TEXT("storage.stock"):TEXT("storage.bag"); const float X=Side?1108:64;
        Rule({X,212},500,FString::Printf(TEXT("storage.%d.rule"),Side),Component);
        for(int32 Cell=0;Cell<16;++Cell)
        {
            const int32 Index=Scroll*4+Cell;
            const auto* Entry=Lists[Side].IsValidIndex(Index)?&Lists[Side][Index]:nullptr;
            const auto R=Entry?Entry->Row:nullptr;
            const FName Id=R?FName(*Text(R,TEXT("id"))):NAME_None;
            const FGuid Instance=Entry?Entry->Instance:FGuid();
            const FString Action=Entry?(Instance.IsValid()?(Side?TEXT("withdrawInstance:"):TEXT("depositInstance:"))+Instance.ToString()
                :(Side?TEXT("withdraw:"):TEXT("deposit:"))+Id.ToString()):FString();
            auto& CellElement=Add(TEXT("inventorySlot"),Entry?FString::FromInt(Entry->Count):TEXT(""),
                {X+(Cell%4)*124,226.+(Cell/4)*124},{112,112},18,FString::Printf(TEXT("storage.%s.cell.%d"),Side?TEXT("stock"):TEXT("bag"),Cell),Component,
                Action,R?Text(R,TEXT("icon")):TEXT(""),Id==SelectedItem && !Id.IsNone() && Instance==StorageSelection && StorageToCamp==(Side==0));
            CellElement.Value=0;CellElement.InventoryItem=Id;CellElement.InventoryInstance=Instance;
        }
        if(Lists[Side].IsEmpty()) Add(TEXT("text"),TEXT("暂无物品"),{X+96,460},{308,50},24,FString::Printf(TEXT("storage.%d.empty"),Side),Component).Align=TEXT("center");
        Add(TEXT("text"),FString::Printf(TEXT("%d 格 · 装备逐件"),Lists[Side].Num()),{X,768},{240,40},18,FString::Printf(TEXT("storage.%d.total"),Side),Component).Color=Color(TEXT("muted"));
    }
    auto& Prev=Add(TEXT("menuAction"),TEXT("上一页"),{360,758},{98,48},18,TEXT("storage.prev"),TEXT("storage.bag"),TEXT("storage.prev")); Prev.Enabled=Scroll>0;
    auto& Next=Add(TEXT("menuAction"),TEXT("下一页"),{466,758},{98,48},18,TEXT("storage.next"),TEXT("storage.bag"),TEXT("storage.next")); Next.Enabled=Scroll+4<RowCount;
    const auto R=Find(TEXT("items"),SelectedItem.ToString());
    if(R)
    {
        Add(TEXT("text"),Text(R,TEXT("name")),{626,166},{420,52},26,TEXT("storage.detail.name"),TEXT("storage.detail")).Color=Color(TEXT("gold"));
        const FSlateFontInfo Font(Typeface,FMath::RoundToInt(18*Scale*.75f));
        const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        const int32 Count=StorageToCamp?Bag->GetItemCount(SelectedItem):Store->GetItemCount(SelectedItem);
        const int32 Available=StorageToCamp?Bag->Available(SelectedItem):Store->Available(SelectedItem);
        FString Properties=FString::Printf(TEXT("分类 %s · 单重 %.2f\n同类持有 %d · 可用 %d"),*Text(R,TEXT("category")),Number(R,TEXT("weight"))/100,Count,Available);
        if(StorageSelection.IsValid())
        {
            const auto& Items=StorageToCamp?Bag->Snapshot().Instances:Store->InventorySnapshot().Instances;
            const auto* Instance=Items.FindByPredicate([&](const auto& I){return I.Id==StorageSelection;});
            Properties+=TEXT("\n实例 ")+StorageSelection.ToString(EGuidFormats::Digits).Left(8);
            Properties+=Instance?FString::Printf(TEXT(" · %s\n耐久 %.2f / %.0f"),StorageToCamp && Bag->IsEquipped(Instance->Id)?TEXT("已装备"):TEXT("未装备"),Instance->Durability,Number(R,TEXT("durability"))):TEXT(" · 已离开此容器");
        }
        double AfterWeight;const FString Reason=StorageTransferStatus(AfterWeight);
        const FString Preview=FString::Printf(TEXT("%s · 本次 %d 件\n当前可用 %d · %s"),StorageToCamp?TEXT("背包 → 仓储"):TEXT("仓储 → 背包"),Quantity,Available,
            Reason.IsEmpty()?*FString::Printf(TEXT("转移后负重 %.2f / %.0f"),AfterWeight,Bag->GetCapacity()):*Reason);
        TArray<FString> Paragraphs,DetailLines;
        (Text(R,TEXT("description"))+TEXT("\n\n")+Properties+TEXT("\n\n")+Preview).ParseIntoArrayLines(Paragraphs,false);
        for(FString Paragraph:Paragraphs)
        {
            while(Paragraph.Len()>1 && Measure->Measure(Paragraph,Font).X>420)
            {const int32 N=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Paragraph),Font,420)+1,1,Paragraph.Len());DetailLines.Add(Paragraph.Left(N));Paragraph=Paragraph.Mid(N);}
            DetailLines.Add(Paragraph);
        }
        const int32 Visible=FMath::Max(1,FMath::FloorToInt(416/(18*Scale*1.6f)));
        StorageDetailScroll=FMath::Clamp(StorageDetailScroll,0,FMath::Max(0,DetailLines.Num()-Visible));TArray<FString> Slice;
        for(int32 I=StorageDetailScroll;I<FMath::Min(DetailLines.Num(),StorageDetailScroll+Visible);++I)Slice.Add(DetailLines[I]);
        Add(TEXT("text"),FString::Join(Slice,TEXT("\n")),{626,226},{420,416},18,TEXT("storage.detail.properties"),TEXT("storage.detail"));
        if(DetailLines.Num()>Visible)
        {
            Add(TEXT("menuAction"),TEXT("详情上移"),{626,644},{200,30},16,TEXT("storage.detail.prev"),TEXT("storage.detail"),TEXT("storage.detail.prev")).Enabled=StorageDetailScroll>0;
            Add(TEXT("menuAction"),TEXT("详情下移"),{844,644},{200,30},16,TEXT("storage.detail.next"),TEXT("storage.detail"),TEXT("storage.detail.next")).Enabled=StorageDetailScroll+Visible<DetailLines.Num();
        }
        Add(TEXT("menuAction"),TEXT("−"),{626,682},{64,42},22,TEXT("storage.quantity.less"),TEXT("storage.detail"),TEXT("quantity:-1")).Enabled=!StorageSelection.IsValid() && Quantity>1;
        Add(TEXT("text"),FString::FromInt(Quantity),{714,686},{242,36},21,TEXT("storage.quantity"),TEXT("storage.detail")).Align=TEXT("center");
        Add(TEXT("menuAction"),TEXT("+"),{980,682},{64,42},22,TEXT("storage.quantity.more"),TEXT("storage.detail"),TEXT("quantity:1")).Enabled=!StorageSelection.IsValid() && Quantity<Available;
        const FString Key=HearthwardInput::Label(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings,TEXT("storage.transfer"));
        auto& Transfer=Add(TEXT("menuAction"),Key+(StorageToCamp?TEXT(" 存入储物箱"):TEXT(" 取到背包")),{626,738},{420,44},21,TEXT("storage.transfer"),TEXT("storage.detail"),StorageTransferToken()); Transfer.Enabled=Reason.IsEmpty();Transfer.TextInset=14;
        if(StorageTransferCommitted)Add(TEXT("menuAction"),TEXT("新转移"),{626,794},{420,36},18,TEXT("storage.newTransfer"),TEXT("storage.detail"),TEXT("storage.newTransfer"));
    }
    else Add(TEXT("text"),TEXT("选择物品"),{626,388},{420,54},24,TEXT("storage.detail.empty"),TEXT("storage.detail")).Align=TEXT("center");
    Rule({40,854},1592,TEXT("storage.footer.rule"),TEXT("storage.footer"));
    Add(TEXT("menuAction"),TEXT("Esc 返回"),{64,868},{180,50},20,TEXT("storage.back"),TEXT("storage.footer"),TEXT("back"));
    Add(TEXT("text"),TEXT("鼠标选择物品   ·   − / + 调整数量   ·   滚轮浏览"),{650,876},{958,36},18,TEXT("storage.hint"),TEXT("storage.footer")).Align=TEXT("right");
}
