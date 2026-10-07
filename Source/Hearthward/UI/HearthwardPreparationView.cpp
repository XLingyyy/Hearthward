#include "HearthwardPreparationView.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "Engine/World.h"

FHearthwardPreparationView HearthwardPreparation::Read(const UHearthwardGameplayComponent* G,
    const UHearthwardCampaignSubsystem* Campaign,FName Quest)
{
    using namespace HearthwardData;
    FHearthwardPreparationView View;View.Quest=Quest;
    const auto Row=Find(TEXT("quests"),Quest.ToString());
    if(!G || !Row || !G->QuestAvailable(Quest) || G->Claimed.Contains(Quest))return View;
    View.Visible=true;View.Heading=Text(Row,TEXT("name"));
    const bool InCampaign=Campaign && Campaign->Active() && HearthwardCampaign::Find(TEXT("quests"),Quest);
    View.Location=InCampaign?Campaign->QuestLocation(Quest):FName(*Text(Row,TEXT("location")));
    const int32 Required=Number(Row,TEXT("required"));
    View.ReadyToClaim=Required>0 && G->QuestProgress(Quest)>=Required;
    View.Action=TEXT("preparation.journal:")+Quest.ToString();
    if(View.ReadyToClaim){View.NextStep=TEXT("条件已达成，打开日志领取奖励。");View.Location=NAME_None;return View;}
    View.NextStep=Text(Row,TEXT("objective"));
    if(!InCampaign)return View;
    const auto& Camp=G->GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    const bool Workbench=Camp.Facilities.ContainsByPredicate([](const auto& F){return F.Kind==TEXT("workbench");});
    if(Quest==TEXT("main_01"))
    {
        if(!Campaign->State.Facts.Contains(TEXT("relic")))View.NextStep=TEXT("靠近家中的遗物包，按交互键取回遗物。");
        else if(!Campaign->State.Facts.Contains(TEXT("prologue_order")))
        {View.NextStep=TEXT("靠近弟弟，打开交流并确认跟随，然后沿西侧通道撤离。");View.Action=TEXT("page:dialogue");}
        else View.NextStep=TEXT("带弟弟沿西侧通道下楼，从出城口撤离。");
    }
    else if(Quest==TEXT("main_02"))
    {
        if(G->Events.FindRef(TEXT("harvest:wood"))<=0 || G->Events.FindRef(TEXT("harvest:stone"))<=0)View.NextStep=TEXT("先实际采集木材和石材，准备营地建设材料。");
        else if(!Workbench || !Campaign->State.Facts.Contains(TEXT("worker_assigned")))
        {View.NextStep=TEXT("建成工作台 I，并在营地页安排族人岗位；查看制作和分工入口。营地建设按原成本扣料。");View.Action=TEXT("page:camp");}
        else if(!Campaign->State.Facts.Contains(TEXT("camp_order")))
        {View.NextStep=TEXT("靠近弟弟，确认一次营地分工。个人委托与持续采集队在工作页分别查看。");View.Action=TEXT("page:dialogue");}
    }
    else if(Quest==TEXT("main_03"))
    {
        const auto* Person=Campaign->State.People.FindByPredicate([](const auto& P){return P.Id==TEXT("rescued_01");});
        if(Camp.Rescued.Contains(TEXT("rescued_01")))View.NextStep=TEXT("获救者已到营地；打开日志检查剩余条件并领取奖励。");
        else if(Person && Person->Stage==TEXT("waiting"))
        {
            View.Location=NAME_None;View.LocationLabel=TEXT("等待中的获救者");
            View.NextStep=TEXT("回到获救者身边，检查附近威胁和路线后恢复跟随，再带他回营。");
        }
        else if(Person && Person->Stage!=TEXT("uncontacted"))View.NextStep=TEXT("带已联系的获救者回营；若停止跟随，先检查附近威胁和路线。");
        else View.NextStep=TEXT("前往已知救援地点，排除威胁后联系获救者。建议出发前检查食物、药品和装备。");
    }
    else if(Quest==TEXT("main_04"))
    {
        View.Action=TEXT("page:camp");
        View.NextStep=!Workbench || !Camp.Rescued.Contains(TEXT("rescued_01"))?TEXT("先带一名获救者回营并建成工作台 I。"):
            Camp.Tier<2?TEXT("打开营地页检查 S2 条件与材料缺口，满足后手动确认升阶。"):
            TEXT("营地已升至 S2，打开营地管理查看新增能力。之后在日志领取奖励。");
    }
    return View;
}
