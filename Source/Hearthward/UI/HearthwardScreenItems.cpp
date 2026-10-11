#include "HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameData.h"

void UHearthwardScreenWidget::ItemLine(FName Item,const FString& Label,FVector2D Position,float Width,
    const FString& LayoutId,const FString& Component,float Font,float IconSize)
{
    const auto Row=HearthwardData::Find(TEXT("items"),Item.ToString());
    Element(TEXT("image"),TEXT(""),Position,{IconSize,IconSize},18,TEXT(""),HearthwardData::Text(Row,TEXT("icon")));
    Elements.Last().LayoutId=LayoutId+TEXT(".icon");Elements.Last().Component=Component;
    Element(TEXT("text"),Label,Position+FVector2D(IconSize+8,1),{Width-IconSize-8,FMath::Max(IconSize,28.f)},Font);
    Elements.Last().LayoutId=LayoutId;Elements.Last().Component=Component;
    Elements.Last().Font=Font*GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f;
    Elements.Last().Tracking=0;Elements.Last().TextInset=0;
}
