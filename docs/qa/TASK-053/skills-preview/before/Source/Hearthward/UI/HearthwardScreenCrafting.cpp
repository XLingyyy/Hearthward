#include "HearthwardScreenWidget.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "GameFramework/Pawn.h"

using namespace HearthwardData;
void UHearthwardScreenWidget::ComposeCrafting()
{
    const auto& Recipes=Rows(TEXT("craftingRecipes"));
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Recipes.Num()-4));
    for(int32 I=Scroll;I<FMath::Min(Recipes.Num(),Scroll+4);++I)
    {
        const auto R=Recipes[I]->AsObject(); const FString Id=Text(R,TEXT("id"));
        Element(TEXT("button"),Text(R,TEXT("name")),FVector2D(318,267+(I-Scroll)*80),FVector2D(370,62),25,TEXT("recipe:")+Id,TEXT(""),SelectedRecipe==FName(*Id));
        Elements.Last().Component=TEXT("crafting.recipes"); Elements.Last().LayoutId=TEXT("crafting.recipe.")+Id;
    }
    const auto R=Find(TEXT("craftingRecipes"),SelectedRecipe.ToString()); if(!R) return;
    auto Identify=[&](const TCHAR* Id,const TCHAR* Component=TEXT("crafting.details")) { Elements.Last().Component=Component; Elements.Last().LayoutId=Id; };
    Element(TEXT("text"),Text(R,TEXT("name")),FVector2D(905,210),FVector2D(450,55),32); Identify(TEXT("crafting.name"));
    Element(TEXT("image"),TEXT(""),FVector2D(792,200),FVector2D(95,95),18,TEXT(""),Text(R,TEXT("icon"))); Identify(TEXT("crafting.icon"));
    Element(TEXT("text"),Text(R,TEXT("description")),FVector2D(792,315),FVector2D(580,60),19); Identify(TEXT("crafting.description"));
    FString Ingredients=TEXT("所需材料   持有 / 消耗\n");
    FString Products=TEXT("本次获得\n"); double Delta=0;
    for(const auto& M:R->GetObjectField(TEXT("materials"))->Values)
    {
        const auto Item=Find(TEXT("items"),FString(*M.Key)); const int32 Needed=int32(M.Value->AsNumber())*CraftingBatches;
        Ingredients+=Text(Item,TEXT("name"))+FString::Printf(TEXT("   %d / %d\n"),Inventory()->GetItemCount(FName(*M.Key)),Needed);
        Delta-=Number(Item,TEXT("weight"))*Needed;
    }
    for(const auto& O:R->GetObjectField(TEXT("outputs"))->Values)
    {
        const auto Item=Find(TEXT("items"),FString(*O.Key)); const int32 Made=int32(O.Value->AsNumber())*CraftingBatches;
        Products+=Text(Item,TEXT("name"))+FString::Printf(TEXT(" × %d   （持有 %d）\n"),Made,Inventory()->GetItemCount(FName(*O.Key)));
        Delta+=Number(Item,TEXT("weight"))*Made;
    }
    Element(TEXT("text"),Ingredients,FVector2D(792,392),FVector2D(580,90),21); Identify(TEXT("crafting.ingredients"));
    Element(TEXT("text"),Products,FVector2D(792,500),FVector2D(580,90),21); Identify(TEXT("crafting.outputs"));
    const FString Reason=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>()->CraftingStatus(Workbench,SelectedRecipe,CraftingBatches,CraftingEpoch);
    const FString Weight=Reason.IsEmpty()
        ?FString::Printf(TEXT("负重 %.2f → %.2f / %.0f"),Inventory()->GetWeight(),Inventory()->GetWeight()+Delta/100,Inventory()->GetCapacity())
        :FString::Printf(TEXT("当前负重 %.2f / %.0f"),Inventory()->GetWeight(),Inventory()->GetCapacity());
    Element(TEXT("text"),Weight,FVector2D(792,600),FVector2D(580,35),18); Identify(TEXT("crafting.weight"));
    Element(TEXT("button"),TEXT("−"),FVector2D(800,675),FVector2D(65,50),26,TEXT("craftLess")); Identify(TEXT("crafting.less"),TEXT("crafting.actions"));
    Element(TEXT("text"),FString::Printf(TEXT("%d 批"),CraftingBatches),FVector2D(895,685),FVector2D(140,35),22); Identify(TEXT("crafting.quantity"),TEXT("crafting.actions"));
    Element(TEXT("button"),TEXT("+"),FVector2D(1040,675),FVector2D(65,50),26,TEXT("craftMore")); Identify(TEXT("crafting.more"),TEXT("crafting.actions"));
    Element(TEXT("button"),TEXT("F 制作"),FVector2D(1140,675),FVector2D(210,50),24,TEXT("craft")); Identify(TEXT("crafting.submit"),TEXT("crafting.actions")); Elements.Last().Enabled=Reason.IsEmpty();
    Element(TEXT("text"),Reason.IsEmpty()?TEXT("即时完成 · 仅使用背包中的材料"):Reason,FVector2D(800,748),FVector2D(570,48),18); Identify(TEXT("crafting.status"),TEXT("crafting.actions"));
}
