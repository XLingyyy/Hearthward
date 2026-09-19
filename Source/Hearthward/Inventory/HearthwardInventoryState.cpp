#include "HearthwardInventoryState.h"
#include "../Gameplay/HearthwardGameData.h"
const TArray<FHearthwardItemDefinition>& HearthwardBasicItems()
{
    static const TArray<FHearthwardItemDefinition> Items=[]
    {
        TArray<FHearthwardItemDefinition> Out;
        for (const auto& Value : HearthwardData::Rows(TEXT("items")))
        {
            const auto R=Value->AsObject();
            Out.Add({FName(*HearthwardData::Text(R,TEXT("id"))),int32(HearthwardData::Number(R,TEXT("weight"))),FText::FromString(HearthwardData::Text(R,TEXT("name")))});
        }
        return Out;
    }();
    return Items;
}
