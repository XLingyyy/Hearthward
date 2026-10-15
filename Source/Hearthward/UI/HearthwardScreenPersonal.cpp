#include "HearthwardScreenWidget.h"
#include "../Campaign/HearthwardPersonalStory.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "GameFramework/Pawn.h"
using namespace HearthwardData;

void UHearthwardScreenWidget::OpenPersonal(UHearthwardPersonalInteraction* Target)
{
    PersonalTarget=Target;PersonalEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();OpenPage(TEXT("personal"));
}
void UHearthwardScreenWidget::ComposePersonal()
{
    auto* Target=PersonalTarget.Get();
    if(Target && Target->IsDisplay())
    {
        Elements.Reset();Element(TEXT("inventorySurface"),TEXT(""),{110,555},{1450,360});
        Element(TEXT("text"),TEXT("留下的纪念 · 预览在眼前的纪念台上"),{150,575},{1050,50},25);
        int32 Index=0;
        for(const auto& V:HearthwardPersonalStory::Rows())
        {
            const auto R=V->AsObject();Element(TEXT("button"),Text(R,TEXT("name"))+TEXT("的纪念"),{150.+Index++*410,645},{390,50},21,TEXT("personal.preview:")+Text(R,TEXT("fact")));
        }
        Element(TEXT("button"),TEXT("撤下 · 预览"),{150,715},{270,50},21,TEXT("personal.preview:"));
        Element(TEXT("button"),TEXT("确认摆放"),{450,715},{300,50},21,TEXT("personal.confirm"));
        Element(TEXT("button"),TEXT("取消预览"),{780,715},{300,50},21,TEXT("personal.cancel"));
        Element(TEXT("button"),TEXT("返回"),{1110,715},{350,50},21,TEXT("page:hud"));
        Element(TEXT("text"),Message.IsEmpty()?Target->Describe():Message,{150,795},{1300,90},20);return;
    }
    Element(TEXT("text"),Target && Target->IsDisplay()?TEXT("留下的纪念"):TEXT("归乡人的话"),{180,90},{1050,60},32);
    Element(TEXT("button"),TEXT("返回"),{1250,98},{210,45},20,TEXT("page:hud"));
    if(!Target || PersonalEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())
    {Element(TEXT("text"),TEXT("交互已结束，请重新靠近人物或纪念台。"),{180,200},{1260,140},24);return;}
    if(!Target->IsDisplay())
    {
        const auto Row=HearthwardPersonalStory::Row(Target->Identity);const auto State=HearthwardPersonalStory::State(GetWorld(),Target->Identity);
        Element(TEXT("text"),Text(Row,TEXT("name"))+TEXT(" · ")+Text(Row,TEXT("title")),{180,170},{1250,50},24);
        Element(TEXT("text"),Target->Describe(),{180,245},{1250,260},26);
        const auto* Gameplay=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardGameplayComponent>();
        const bool Unknown=Target->Identity==TEXT("civilian_initial_03") && !Gameplay->Discovered.Contains(TEXT("poi_herb_01"));
        if(State.Step.IsNone() && !Unknown)Element(TEXT("button"),TEXT("听完并记下这件事"),{180,550},{530,60},22,TEXT("personal.begin"));
        else if(State.Step==TEXT("visited") && State.Choice.IsNone())
        {
            Element(TEXT("button"),TEXT("把纪念留在营地"),{180,550},{530,60},22,TEXT("personal.camp"));
            Element(TEXT("button"),TEXT("把纪念留在故乡生活区"),{800,550},{600,60},22,TEXT("personal.hometown"));
            Element(TEXT("text"),TEXT("完成现场观察并返回后可选择。守望者的结尾在故乡真正夺回后开放。选择不会改动岗位或产量。"),{180,640},{1260,100},20);
        }
        else if(!State.Choice.IsNone())Element(TEXT("text"),TEXT("记录已经留下。这段话可以重看，其他主支线仍可继续。"),{180,580},{1260,100},22);
        else Element(TEXT("text"),Unknown?TEXT("发现旧药圃后再来交谈。当前没有记入新的观察。")
            :TEXT("这件事已记下。完成现场观察，再回来交谈。"),{180,580},{1260,100},22);
    }
    if(!Message.IsEmpty())Element(TEXT("text"),Message,{180,755},{1260,110},22);
}
bool UHearthwardScreenWidget::ExecutePersonalAction(const FString& Action)
{
    if(Page!=TEXT("personal") || !PersonalTarget.IsValid())return false;
    auto* Target=PersonalTarget.Get();
    if(Action==TEXT("personal.confirm"))Message=Target->ConfirmDisplay(GetOwningPlayerPawn(),PersonalEpoch);
    else if(Action==TEXT("personal.cancel")){Target->CancelPreview();Message=TEXT("预览已取消，原陈列保留");}
    else if(Action.StartsWith(TEXT("personal.preview:")))Message=Target->Preview(GetOwningPlayerPawn(),FName(*Action.Mid(17)),PersonalEpoch);
    else Message=Target->Choose(GetOwningPlayerPawn(),FName(*Action.Mid(9)),PersonalEpoch);
    MessageUntil=FPlatformTime::Seconds()+12;Refresh();return true;
}
