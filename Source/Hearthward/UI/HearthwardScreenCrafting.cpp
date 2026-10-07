#include "HearthwardScreenWidget.h"
#include "HearthwardCraftingTracker.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

using namespace HearthwardData;
void UHearthwardScreenWidget::CraftingSearchChanged(const FText& Value)
{
    if(Page!=TEXT("crafting") || CraftingSearch==Value.ToString())return;
    CraftingSearch=Value.ToString();Scroll=0;Refresh();
}
void UHearthwardScreenWidget::ComposeCrafting()
{
    const auto Recipes=FilteredCraftingRecipes();
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Recipes.Num()-4));
    for(int32 I=Scroll;I<FMath::Min(Recipes.Num(),Scroll+4);++I)
    {
        const auto R=Recipes[I]; const FString Id=Text(R,TEXT("id"));
        Element(TEXT("button"),Text(R,TEXT("name")),FVector2D(318,340+(I-Scroll)*67),FVector2D(370,58),22,TEXT("recipe:")+Id,TEXT(""),SelectedRecipe==FName(*Id));
        Elements.Last().Component=TEXT("crafting.recipes"); Elements.Last().LayoutId=TEXT("crafting.recipe.")+Id;
    }
    auto Filter=[&](const TCHAR* Id,FString Label,FVector2D Position,FVector2D Size,const FString& Action,bool Selected=false)
    {
        Element(TEXT("button"),Label,Position,Size,18,Action,TEXT(""),Selected);
        Elements.Last().Component=TEXT("crafting.recipes");Elements.Last().LayoutId=Id;
    };
    Filter(TEXT("crafting.category"),TEXT("分类：")+(CraftingCategory.IsEmpty()?TEXT("全部"):CraftingCategory),{318,285},{220,42},TEXT("craftCategory"));
    Filter(TEXT("crafting.available"),TEXT("可制作"),{550,285},{138,42},TEXT("craftAvailable"),CraftingOnlyAvailable);
    Filter(TEXT("crafting.clearFilters"),TEXT("清空筛选"),{318,687},{370,44},TEXT("craftClearFilters"));
    if(Recipes.IsEmpty())
    {
        Element(TEXT("text"),TEXT("没有符合筛选的配方\n可清空筛选后继续查看"),{318,355},{370,130},20);
        Elements.Last().Component=TEXT("crafting.recipes");Elements.Last().LayoutId=TEXT("crafting.empty");
    }
    const auto R=Find(TEXT("craftingRecipes"),SelectedRecipe.ToString()); if(!R) return;
    auto Identify=[&](const TCHAR* Id,const TCHAR* Component=TEXT("crafting.details")) { Elements.Last().Component=Component; Elements.Last().LayoutId=Id; };
    Element(TEXT("text"),Text(R,TEXT("name")),FVector2D(905,210),FVector2D(450,55),32); Identify(TEXT("crafting.name"));
    Element(TEXT("image"),TEXT(""),FVector2D(792,200),FVector2D(95,95),18,TEXT(""),Text(R,TEXT("icon"))); Identify(TEXT("crafting.icon"));
    Element(TEXT("text"),Text(R,TEXT("description")),FVector2D(792,315),FVector2D(580,60),19); Identify(TEXT("crafting.description"));
    const auto* Bag=Inventory();
    const auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    const bool UseStorage=!GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(GetOwningPlayerPawn()->GetActorLocation()).IsNone();
    FString Ingredients=TEXT("所需材料   可用 / 本次消耗\n");
    FString Products=TEXT("本次获得\n"); double Delta=0;
    for(const auto& Need:HearthwardCraftingDiscovery::Materials(R,CraftingBatches,Bag,Storage,UseStorage))
    {
        const auto Item=Find(TEXT("items"),Need.Item.ToString());
        Ingredients+=Text(Item,TEXT("name"))+(UseStorage
            ?FString::Printf(TEXT("   背包可用 %d + 仓储可用 %d / 消耗 %d · 缺 %d\n"),Need.InBag,Need.InStorage,Need.Needed,Need.Missing)
            :FString::Printf(TEXT("   背包可用 %d / 消耗 %d · 缺 %d（仓储参考 %d）\n"),Need.InBag,Need.Needed,Need.Missing,Need.InStorage));
        Delta-=Number(Item,TEXT("weight"))*FMath::Min(Need.InBag,Need.Needed);
    }
    for(const auto& O:R->GetObjectField(TEXT("outputs"))->Values)
    {
        const auto Item=Find(TEXT("items"),FString(*O.Key)); const int32 Made=int32(O.Value->AsNumber())*CraftingBatches;
        Products+=Text(Item,TEXT("name"))+FString::Printf(TEXT(" × %d   （持有 %d）\n"),Made,Inventory()->GetItemCount(FName(*O.Key)));
        Delta+=Number(Item,TEXT("weight"))*Made;
    }
    Element(TEXT("text"),Ingredients,FVector2D(792,370),FVector2D(580,105),18); Identify(TEXT("crafting.ingredients"));
    Element(TEXT("text"),Products,FVector2D(792,478),FVector2D(580,65),18); Identify(TEXT("crafting.outputs"));
    const FString Reason=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>()->CraftingStatus(Workbench,SelectedRecipe,CraftingBatches,CraftingEpoch);
    const FString Weight=Reason.IsEmpty()
        ?FString::Printf(TEXT("负重 %.2f → %.2f / %.0f"),Inventory()->GetWeight(),Inventory()->GetWeight()+Delta/100,Inventory()->GetCapacity())
        :FString::Printf(TEXT("当前负重 %.2f / %.0f"),Inventory()->GetWeight(),Inventory()->GetCapacity());
    Element(TEXT("text"),Weight,FVector2D(792,547),FVector2D(580,30),18); Identify(TEXT("crafting.weight"));
    Element(TEXT("text"),CraftingTrackerText(false),{792,582},{580,80},16);Identify(TEXT("crafting.tracker"));
    Element(TEXT("button"),TEXT("−"),FVector2D(800,675),FVector2D(65,50),26,TEXT("craftLess")); Identify(TEXT("crafting.less"),TEXT("crafting.actions"));
    Element(TEXT("text"),FString::Printf(TEXT("%d 批"),CraftingBatches),FVector2D(895,685),FVector2D(140,35),22); Identify(TEXT("crafting.quantity"),TEXT("crafting.actions"));
    Element(TEXT("button"),TEXT("+"),FVector2D(1040,675),FVector2D(65,50),26,TEXT("craftMore")); Identify(TEXT("crafting.more"),TEXT("crafting.actions"));
    Element(TEXT("button"),TEXT("F 制作"),FVector2D(1140,675),FVector2D(210,50),24,TEXT("craft")); Identify(TEXT("crafting.submit"),TEXT("crafting.actions")); Elements.Last().Enabled=Reason.IsEmpty();
    const FString Ready=UseStorage?TEXT("即时完成 · 背包优先，缺额使用营地仓储"):TEXT("即时完成 · 使用背包可用材料");
    Element(TEXT("text"),Reason.IsEmpty()?Ready:Reason,FVector2D(800,733),FVector2D(570,35),17); Identify(TEXT("crafting.status"),TEXT("crafting.actions"));
    Element(TEXT("button"),CraftingTracker.IsCurrent(Storage->GetTimelineEpoch())?TEXT("替换追踪目标"):TEXT("追踪材料"),{792,780},{280,44},19,TEXT("craftTrack"));Identify(TEXT("crafting.track"),TEXT("crafting.actions"));
    Element(TEXT("button"),TEXT("取消材料追踪"),{1092,780},{280,44},19,TEXT("craftUntrack"));Identify(TEXT("crafting.untrack"),TEXT("crafting.actions"));
    Elements.Last().Enabled=CraftingTracker.IsCurrent(Storage->GetTimelineEpoch());
}

TArray<TSharedPtr<FJsonObject>> UHearthwardScreenWidget::FilteredCraftingRecipes() const
{
    TArray<TSharedPtr<FJsonObject>> Result;
    const auto* Building=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>();
    for(const auto& Value:Rows(TEXT("craftingRecipes")))
    {
        const auto Recipe=Value->AsObject();const FName Id(*Text(Recipe,TEXT("id")));
        if(!CraftingSearch.IsEmpty() && !Text(Recipe,TEXT("name")).Contains(CraftingSearch))continue;
        if(!CraftingCategory.IsEmpty() && HearthwardCraftingDiscovery::Category(Recipe)!=CraftingCategory)continue;
        if(CraftingOnlyAvailable && !Building->CraftingStatus(Workbench,Id,CraftingBatches,CraftingEpoch).IsEmpty())continue;
        Result.Add(Recipe);
    }
    return Result;
}
FString UHearthwardScreenWidget::CraftingTrackerText(bool Compact)
{
    const auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if(!CraftingTracker.IsCurrent(Storage->GetTimelineEpoch())){CraftingTracker.Clear();return Compact?FString():TEXT("材料追踪仅在本次会话保留，读档或新游戏后清空。");}
    const auto Recipe=Find(TEXT("craftingRecipes"),CraftingTracker.Recipe.ToString());
    const bool UseStorage=!GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(GetOwningPlayerPawn()->GetActorLocation()).IsNone();
    const auto Needs=HearthwardCraftingDiscovery::Materials(Recipe,CraftingTracker.Batches,Inventory(),Storage,UseStorage);
    const bool Ready=!Needs.ContainsByPredicate([](const auto& N){return N.Missing>0;});
    FString Label=FString::Printf(TEXT("材料追踪：%s × %d批 · %s"),*Text(Recipe,TEXT("name")),CraftingTracker.Batches,Ready?TEXT("材料齐备"):TEXT("准备中"));
    if(Compact)return Label;
    Label+=TEXT("\n");
    for(const auto& Need:Needs)
        Label+=Text(Find(TEXT("items"),Need.Item.ToString()),TEXT("name"))+FString::Printf(TEXT(" %d/%d（缺%d）  "),Need.Available,Need.Needed,Need.Missing);
    Label+=UseStorage?TEXT("\n营地内计入共享仓储；设施等其他条件仍按制作状态检查。"):TEXT("\n离营仅计背包；仓储数量仅供参考。");
    return Label;
}
FString UHearthwardScreenWidget::GetCraftingTrackerText()
{
    const FString Label=CraftingTrackerText(false);
    return CraftingTracker.Recipe.IsNone()?FString():Label;
}
