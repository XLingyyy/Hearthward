#include "HearthwardScreenWidget.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "GameFramework/Pawn.h"

using namespace HearthwardData;
void UHearthwardScreenWidget::ComposeRepair()
{
    auto* G=Gameplay();
    TArray<FName> Items;
    for(const auto& V:Rows(TEXT("repairRecipes")))
    {
        const FName Id(*Text(V->AsObject(),TEXT("id")));
        if(Inventory()->GetItemCount(Id)>0) Items.Add(Id);
    }
    auto Current=[&](FName Id) { const auto* D=G->Durability.Find(Id); return D?*D:float(Number(Find(TEXT("items"),Id.ToString()),TEXT("durability"))); };
    Items.StableSort([&](FName A,FName B) { return Current(A)/Number(Find(TEXT("items"),A.ToString()),TEXT("durability"))<Current(B)/Number(Find(TEXT("items"),B.ToString()),TEXT("durability")); });
    if(!Items.Contains(SelectedRepair)) SelectedRepair=Items.IsEmpty()?NAME_None:Items[0];
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Items.Num()-6));
    for(int32 I=Scroll;I<FMath::Min(Items.Num(),Scroll+6);++I)
    {
        const auto Item=Find(TEXT("items"),Items[I].ToString());
        Element(TEXT("button"),Text(Item,TEXT("name"))+FString::Printf(TEXT("   %.0f / %.0f"),Current(Items[I]),Number(Item,TEXT("durability"))),FVector2D(300,258+(I-Scroll)*65),FVector2D(410,53),21,TEXT("repairItem:")+Items[I].ToString(),TEXT(""),SelectedRepair==Items[I]);
        Elements.Last().Component=TEXT("repairing.items"); Elements.Last().LayoutId=TEXT("repairing.item.")+Items[I].ToString();
    }
    auto Identify=[&](const TCHAR* Id,const TCHAR* Component=TEXT("repairing.details")) { Elements.Last().Component=Component; Elements.Last().LayoutId=Id; };
    if(Items.IsEmpty())
    {
        Element(TEXT("text"),TEXT("背包中没有可维修的装备"),FVector2D(785,250),FVector2D(570,60),24); Identify(TEXT("repairing.empty")); return;
    }
    const auto Item=Find(TEXT("items"),SelectedRepair.ToString());
    const float Before=Current(SelectedRepair),Maximum=Number(Item,TEXT("durability"));
    Element(TEXT("image"),TEXT(""),FVector2D(785,185),FVector2D(105,105),18,TEXT(""),Text(Item,TEXT("icon"))); Identify(TEXT("repairing.icon"));
    Element(TEXT("text"),Text(Item,TEXT("name")),FVector2D(925,212),FVector2D(440,60),30); Identify(TEXT("repairing.name"));
    Element(TEXT("text"),Before<=0?TEXT("已损坏"):Before>=Maximum?TEXT("完好"):TEXT("需要维护"),FVector2D(800,328),FVector2D(560,40),22); Identify(TEXT("repairing.condition"));
    Element(TEXT("bar"),TEXT(""),FVector2D(800,385),FVector2D(510,12)); Identify(TEXT("repairing.durabilityBar")); Elements.Last().Value=Before/Maximum; Elements.Last().Color=Color(TEXT("gold"));
    Element(TEXT("text"),FString::Printf(TEXT("当前耐久 %.0f / %.0f\n修复后耐久 %.0f / %.0f"),Before,Maximum,Maximum,Maximum),FVector2D(800,415),FVector2D(560,85),23); Identify(TEXT("repairing.durability"));
    FString Cost=TEXT("整件修复材料   持有 / 消耗\n");
    for(const auto& M:Find(TEXT("repairRecipes"),SelectedRepair.ToString())->GetObjectField(TEXT("materials"))->Values)
        Cost+=Text(Find(TEXT("items"),FString(*M.Key)),TEXT("name"))+FString::Printf(TEXT("   %d / %.0f\n"),Inventory()->GetItemCount(FName(*M.Key)),M.Value->AsNumber());
    Element(TEXT("text"),Cost,FVector2D(800,522),FVector2D(560,110),20); Identify(TEXT("repairing.materials"));
    const FString Reason=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>()->RepairStatus(Workbench,SelectedRepair,CraftingEpoch);
    Element(TEXT("button"),TEXT("F 修复装备"),FVector2D(800,675),FVector2D(530,55),25,TEXT("repairEquipment")); Identify(TEXT("repairing.submit"),TEXT("repairing.actions")); Elements.Last().Enabled=Reason.IsEmpty();
    Element(TEXT("text"),Reason.IsEmpty()?TEXT("立即修复至满耐久 · 消耗背包中的材料"):Reason,FVector2D(800,750),FVector2D(570,50),18); Identify(TEXT("repairing.status"),TEXT("repairing.actions"));
}
