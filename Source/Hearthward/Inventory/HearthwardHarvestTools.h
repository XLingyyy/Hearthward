#pragma once
#include "HearthwardInventoryComponent.h"
#include "../Gameplay/HearthwardGameData.h"
namespace HearthwardHarvestTools
{
inline int32 Yield(const UHearthwardInventoryComponent* Bag,FName Item,FGuid& Tool)
{
    using namespace HearthwardData;
    Tool.Invalidate();
    const FString Kind=Item==TEXT("wood")?TEXT("axe"):(Item==TEXT("stone") || Item==TEXT("ore") || Item==TEXT("refined_ore"))?TEXT("pickaxe"):Item==TEXT("fish")?TEXT("fishing_rod"):TEXT("");
    if(Kind.IsEmpty())return 2;
    int32 Grade=0;
    for(const auto& I:Bag->Snapshot().Instances)
    {
        const auto R=Find(TEXT("items"),I.Definition.ToString());
        if(I.Durability>0 && Text(R,TEXT("toolKind"))==Kind && Number(R,TEXT("toolGrade"))>Grade)
        {Grade=Number(R,TEXT("toolGrade"));Tool=I.Id;}
    }
    if(Item==TEXT("refined_ore") && Grade<2)return 0;
    return Grade?Grade+1:0;
}
}
