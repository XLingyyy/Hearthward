#include "HearthwardScreenWidget.h"
#include "Engine/GameInstance.h"

FHearthwardUIElement& UHearthwardScreenWidget::MenuElement(FString Type,FString Text,FVector2D Position,FVector2D Size,float Font,FString Action,bool Selected)
{
    Element(Type,Text,Position,Size,Font,Action,TEXT(""),Selected);
    auto& E=Elements.Last();
    E.Font=Font*GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f;
    E.FontRole=TEXT("display"); E.TextInset=0; E.Tracking=20;
    E.Component=Page.ToString()+(Position.Y<212?TEXT(".header"):Position.Y>=854?TEXT(".footer"):TEXT(".sheet"));
    if(!Action.IsEmpty()) E.LayoutId=Page.ToString()+TEXT(".action.")+Action;
    return E;
}

float UHearthwardScreenWidget::MenuRowHeight() const
{ return 56*GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f; }

int32 UHearthwardScreenWidget::MenuPageSize() const
{ return FMath::Max(3,FMath::FloorToInt(400/MenuRowHeight())); }

void UHearthwardScreenWidget::ComposeMenuChrome(const FString& Heading,const FString& Subtitle)
{
    MenuElement(TEXT("menuSurface"),TEXT(""),{64,212},{1544,634});
    MenuElement(TEXT("text"),Heading,{88,43},{1050,56},36).Color=Color(TEXT("gold"));
    auto& Brand=MenuElement(TEXT("text"),TEXT("HEARTHWARD"),{1190,64},{394,30},18);
    Brand.Align=TEXT("right"); Brand.Tracking=180; Brand.Color=Color(TEXT("muted"));
    MenuElement(TEXT("text"),Message.IsEmpty()?Subtitle:Message,{88,111},{1496,30},18).Color=Color(TEXT("muted"));
    MenuElement(TEXT("line"),TEXT(""),{64,144},{1544,1}).Color=Color(TEXT("bronze"));
    MenuElement(TEXT("line"),TEXT(""),{64,854},{1544,1}).Color=Color(TEXT("bronze"));
    MenuElement(TEXT("menuAction"),TEXT("Esc  返回"),{100,872},{225,43},21,TEXT("back"));
}
