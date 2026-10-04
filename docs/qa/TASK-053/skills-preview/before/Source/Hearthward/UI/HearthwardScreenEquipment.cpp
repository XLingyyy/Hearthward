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
void UHearthwardScreenWidget::ComposeEquipment()
{
    auto* Bag=EquipmentBag(GetWorld(),EquipmentOwner,Inventory());
    const auto& Items=Bag?Bag->Snapshot():GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->InventorySnapshot();
    Element(TEXT("text"),TEXT("行装管理"),FVector2D(180,95),FVector2D(900,55),32);
    Element(TEXT("button"),TEXT("返回背包"),FVector2D(1280,95),FVector2D(210,45),20,TEXT("page:inventory"));
    int32 X=180;
    for(const auto Who:{TEXT("player"),TEXT("brother"),TEXT("storage")})
    {
        Element(TEXT("button"),FString(Who)==TEXT("player")?TEXT("我的背包"):FString(Who)==TEXT("brother")?TEXT("弟弟背包"):TEXT("营地仓储"),FVector2D(X,165),FVector2D(200,44),20,TEXT("gear.owner:")+FString(Who),TEXT(""),EquipmentOwner==Who);X+=215;
    }
    if(Bag)Element(TEXT("text"),FString::Printf(TEXT("负重 %.2f / %.0f  ·  背包 %d 阶"),Bag->GetWeight(),Bag->GetCapacity(),Bag->BackpackRank()),FVector2D(890,170),FVector2D(620,35),20);
    TArray<FHearthwardItemInstance> Instances=Items.Instances;
    Instances.Sort([](const auto& A,const auto& B){return A.Definition==B.Definition?A.Id<B.Id:A.Definition.LexicalLess(B.Definition);});
    TArray<FName> Stacks;Items.Stacks.GetKeys(Stacks);Stacks.Sort(FNameLexicalLess());
    const int32 Total=Instances.Num()+Stacks.Num();Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Total-8));
    if(!Items.Instances.ContainsByPredicate([&](const auto& I){return I.Id==EquipmentSelection;}) && !Items.Stacks.Contains(EquipmentStack))
    {EquipmentSelection=Instances.IsEmpty()?FGuid():Instances[0].Id;EquipmentStack=Instances.IsEmpty() && !Stacks.IsEmpty()?Stacks[0]:NAME_None;}
    for(int32 Index=Scroll;Index<FMath::Min(Total,Scroll+8);++Index)
    {
        const bool Instance=Index<Instances.Num();const auto* I=Instance?&Instances[Index]:nullptr;
        const FName Item=I?I->Definition:Stacks[Index-Instances.Num()];const auto Row=Find(TEXT("items"),Item.ToString());
        FString Label=Text(Row,TEXT("name"));
        if(I)Label+=FString::Printf(TEXT("  ·  第%d件  %s"),Index+1,Bag && Bag->IsEquipped(I->Id)?TEXT("已装备"):TEXT(""));
        else Label+=FString::Printf(TEXT("  × %d"),Items.Stacks[Item]);
        if(I && Number(Row,TEXT("durability"))>0)Label+=FString::Printf(TEXT("  %.2f / %.0f"),I->Durability,Number(Row,TEXT("durability")));
        Element(TEXT("button"),Label,FVector2D(180,235+(Index-Scroll)*62),FVector2D(670,53),18,I?TEXT("gear.select:")+I->Id.ToString():TEXT("gear.stack:")+Item.ToString(),TEXT(""),I?EquipmentSelection==I->Id:EquipmentStack==Item);
    }
    Element(TEXT("button"),TEXT("上一页"),FVector2D(180,745),FVector2D(160,42),18,TEXT("gear.prev"));
    Element(TEXT("button"),TEXT("下一页"),FVector2D(355,745),FVector2D(160,42),18,TEXT("gear.next"));
    const auto* Selected=Items.Instances.FindByPredicate([&](const auto& I){return I.Id==EquipmentSelection;});
    Element(TEXT("text"),TEXT("转交需双方在3米内，脱战且结束动作。\n营地仓储需靠近存取设施。"),FVector2D(895,235),FVector2D(600,70),18);
    X=895;
    for(const auto Who:{TEXT("player"),TEXT("brother"),TEXT("storage")})if(EquipmentOwner!=Who)
    {Element(TEXT("button"),FString(Who)==TEXT("player")?TEXT("交给我"):FString(Who)==TEXT("brother")?TEXT("交给弟弟"):TEXT("放入仓储"),FVector2D(X,320),FVector2D(190,44),18,TEXT("gear.to:")+FString(Who));X+=205;}
    if(!Selected)
    {
        Element(TEXT("button"),TEXT("−"),FVector2D(895,385),FVector2D(70,42),20,TEXT("gear.minus"));
        Element(TEXT("text"),FString::FromInt(Quantity),FVector2D(995,390),FVector2D(120,35),20);
        Element(TEXT("button"),TEXT("＋"),FVector2D(1130,385),FVector2D(70,42),20,TEXT("gear.plus"));
    }
    else if(Bag)
    {
        Element(TEXT("button"),Bag->IsEquipped(Selected->Id)?TEXT("卸下选中装备"):TEXT("装备选中物品"),FVector2D(895,385),FVector2D(320,46),20,TEXT("gear.equip"));
        if(EquipmentOwner==TEXT("player"))
        {
            X=895;
            for(double Fraction:{.25,.5,1.})
            {
                const FString Percent=FString::FromInt(FMath::RoundToInt(Fraction*100));
                Element(TEXT("button"),Fraction==1?TEXT("全部修复"):TEXT("修复 ")+Percent+TEXT("%"),FVector2D(X,455),FVector2D(180,44),18,TEXT("gear.repair:")+Percent);X+=195;
            }
            FString Quotes;
            for(double Fraction:{.25,.5,1.})
            {
                TMap<FName,int32> Cost;double Restored;
                if(HearthwardWorkshop::RepairQuote(Bag,Selected->Id,Fraction,Cost,Restored))
                {
                    Quotes+=FString::Printf(TEXT("%.0f%%  恢复%.2f："),Fraction*100,Restored);
                    for(const auto& M:Cost)Quotes+=Text(Find(TEXT("items"),M.Key.ToString()),TEXT("name"))+FString::Printf(TEXT("×%d "),M.Value);
                    Quotes+=TEXT("\n");
                }
            }
            Element(TEXT("text"),Quotes,FVector2D(895,515),FVector2D(600,100),17);
            Element(TEXT("button"),TEXT("将选中装备放到地面"),FVector2D(895,635),FVector2D(320,44),18,Selected->UniqueClaim.IsNone()?TEXT("gear.drop"):TEXT("ask:gear.dropConfirmed"));
        }
    }
    if(Bag)
    {
        const auto Next=Find(TEXT("backpacks"),FString::FromInt(Bag->BackpackRank()+1));
        FString Cost=TEXT("背包已满级");
        if(Next)
        {
            Cost=FString::Printf(TEXT("下一阶 %.0f 容量 · 营地 %.0f 阶\n"),Number(Next,TEXT("capacity")),Number(Next,TEXT("minimum_camp_tier")));
            for(const auto& M:Next->GetObjectField(TEXT("incremental_cost"))->Values)Cost+=Text(Find(TEXT("items"),FString(*M.Key)),TEXT("name"))+FString::Printf(TEXT("×%.0f  "),M.Value->AsNumber());
        }
        Element(TEXT("text"),Cost,FVector2D(895,695),FVector2D(600,55),16);
        Element(TEXT("button"),TEXT("找后勤员升级此背包"),FVector2D(895,765),FVector2D(320,42),19,TEXT("gear.upgrade"));
    }
}
bool UHearthwardScreenWidget::ExecuteEquipmentAction(const FString& Action)
{
    if(Page!=TEXT("equipment"))return false;
    auto* G=Gameplay();bool OK=true;
    if(Action.StartsWith(TEXT("gear.owner:"))){EquipmentOwner=FName(*Action.Mid(11));Scroll=0;EquipmentSelection.Invalidate();EquipmentStack=NAME_None;}
    else if(Action.StartsWith(TEXT("gear.select:"))){FGuid::Parse(Action.Mid(12),EquipmentSelection);EquipmentStack=NAME_None;}
    else if(Action.StartsWith(TEXT("gear.stack:"))){EquipmentStack=FName(*Action.Mid(11));EquipmentSelection.Invalidate();Quantity=1;}
    else if(Action==TEXT("gear.prev"))Scroll-=8;
    else if(Action==TEXT("gear.next"))Scroll+=8;
    else if(Action==TEXT("gear.minus"))Quantity=FMath::Max(1,Quantity-1);
    else if(Action==TEXT("gear.plus"))Quantity=FMath::Min(9999,Quantity+1);
    else if(Action.StartsWith(TEXT("gear.to:")))OK=G->TransferInventory(EquipmentOwner,FName(*Action.Mid(8)),EquipmentStack,Quantity,EquipmentSelection,EquipmentEpoch);
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
