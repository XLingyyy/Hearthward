#include "HearthwardCampaignSubsystem.h"
#include "HearthwardCampaignActor.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "EngineUtils.h"
using namespace HearthwardData;
FName UHearthwardCampaignSubsystem::Nearest() const
{
    if(!Active() || !Player())return NAME_None;
    double Best=260;FName Result;
    auto Consider=[&](FName Id,FVector P){const double D=FVector::Distance(Player()->GetActorLocation(),P);if(D<Best){Best=D;Result=Id;}};
    for(const auto& P:State.People)if(P.Stage!=TEXT("arrived") && Actor(P.Id))Consider(P.Id,Actor(P.Id)->GetActorLocation());
    for(const auto& V:HearthwardCampaign::Rows(TEXT("locations")))
    {
        const FName Id(*Text(V->AsObject(),TEXT("id")));
        const FString Kind=Text(V->AsObject(),TEXT("kind"));
        if(Kind==TEXT("resource") || Kind==TEXT("rescue") || Kind==TEXT("entry") || Id==TEXT("route_fork"))continue;
        if(Id==TEXT("camp_relic") && !State.Legacy)continue;
        const bool Prologue=Id==TEXT("prologue_relic") || Id==TEXT("prologue_exit");
        if((State.Phase==TEXT("prologue"))!=Prologue)continue;
        if(Id==TEXT("hometown") && !State.Victory)continue;
        if(State.Positions.Contains(Id))Consider(Id,Position(Id));
    }
    return Result;
}
FString UHearthwardCampaignSubsystem::Prompt() const
{
    if(!Active())return {};
    if(IntroRemaining>0)return TEXT("前街的火光照进了窗户。\n弟弟：哥，他们已经进来了！\n拿上母亲的护符，从后巷去找族人。");
    if(TravelDestination==TEXT("camp") && State.Phase==TEXT("prologue"))return TEXT("弟弟：前面就是林地。族人在隐蔽的地方等我们。\n天亮后，再想办法回来。");
    if(!TravelDestination.IsNone())return TEXT("正在加载目的地区域……");
    if(!PendingFlag.IsNone())return FString::Printf(TEXT("占领旗帜 %.1f 秒；移动或受袭中断"),FlagRemaining);
    const FName Id=Nearest();if(Id.IsNone())return Feedback;
    if(Id.ToString().StartsWith(TEXT("rescued_")))return TEXT("E 与族人交谈：跟随 / 原地等待");
    const auto R=HearthwardCampaign::Find(TEXT("locations"),Id);
    FString Status=State.Phase==TEXT("prologue")?TEXT("夜袭 · 带弟弟撤离") : State.Victory?TEXT("故乡已夺回") : FString::Printf(TEXT("故乡控制 %d/4"),State.Flags.Num());
    if(State.ShowRemaining())Status+=FString::Printf(TEXT(" · 剩余驻军 %d"),State.Total()-State.Cleared());
    return Status+TEXT("\nE ")+Text(R,TEXT("name"));
}
bool UHearthwardCampaignSubsystem::Interact()
{
    const FName Id=Nearest();if(Id.IsNone())return false;
    if(!Safe() || Busy()){Feedback=TEXT("请先脱离战斗并结束当前动作");return true;}
    Use(Id);Gameplay()->Feedback=Feedback;Gameplay()->OnChanged.Broadcast();return true;
}
bool UHearthwardCampaignSubsystem::Use(FName Id)
{
    auto* G=Gameplay();auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    const FGuid Epoch=Store->GetTimelineEpoch();
    if(auto* P=State.People.FindByPredicate([&](const auto& V){return V.Id==Id;}))
    {
        if(P->Stage==TEXT("arrived"))return false;
        P->Stage=P->Stage==TEXT("following")?FName(TEXT("waiting")):FName(TEXT("following"));
        P->Escort=TEXT("player");
        Feedback=P->Stage==TEXT("following")?TEXT("我会跟在你后面。遇到危险就先躲起来，记得回来接我。"):TEXT("我在这里等你。");return true;
    }
    if(Id==TEXT("prologue_relic") || (Id==TEXT("camp_relic") && State.Legacy))
    {
        if(!State.Facts.Contains(TEXT("relic")))
        {
            auto* Bag=Player()->FindComponentByClass<UHearthwardInventoryComponent>();
            if(!G->RewardFacts.Contains(TEXT("campaign_start_amulet")) && Bag->GetItemCount(TEXT("amulet"))==0)
            {
                if(Bag->TryAdd(TEXT("amulet"),1)!=EHearthwardInventoryResult::Success){Feedback=TEXT("请先腾出护符的行装位置");return false;}
                G->RewardFacts.Add(TEXT("campaign_start_amulet"));
            }
            Record(TEXT("relic"));
        }
        Feedback=TEXT("母亲留下的护符还在。叫上弟弟，沿后巷走。护符已放入行装。");return true;
    }
    if(Id==TEXT("prologue_exit"))
    {
        bool Brother=false;for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)Brother=It->FindComponentByClass<UHearthwardSurvivalComponent>()->Alive() && FVector::Dist2D(It->GetActorLocation(),Player()->GetActorLocation())<1000;
        if(!State.Facts.Contains(TEXT("relic")) || !State.Facts.Contains(TEXT("prologue_order")) || !Brother){Feedback=TEXT("先拿上护符，向弟弟下达指令，等他到达后巷。");return false;}
        Feedback=TEXT("弟弟：前面就是林地。族人在隐蔽的地方等我们。天亮后，再想办法回来。");return BeginTravel(TEXT("camp"));
    }
    if(Id.ToString().StartsWith(TEXT("loc_")))
    {
        const FName Zone(*Id.ToString().RightChop(4));
        if(State.Flags.Contains(Zone)){Feedback=TEXT("此区已控制");return true;}
        if(!State.ZoneClear(Zone) || ZoneOccupied(Zone)){Feedback=TEXT("此区仍有敌军，暂时不能占旗");return false;}
        PendingFlag=Zone;ActionPosition=Player()->GetActorLocation();ActionHealth=G->Health;FlagRemaining=5;ActionEpoch=Epoch;return true;
    }
    if(Id==TEXT("route_ford") || Id==TEXT("camp") || Id==TEXT("hometown"))
    {G->Activated.Add(Id);G->Discovered.Add(Id);Feedback=TEXT("路标已激活，可从地图选择传送");return true;}
    if(Id==TEXT("route_watch")){Record(TEXT("watch_survey"));Feedback=TEXT("河门、工坊、住区、议场；可以选择进攻顺序。先确认撤离路线。");return true;}
    if(Id==TEXT("route_ridge")){Record(TEXT("ridge_survey"));Feedback=TEXT("已记下山路与故乡方向。");return true;}
    if(Id==TEXT("loot_river_gate")){Record(TEXT("old_carving"));Feedback=TEXT("取回旧门楣雕饰，可放到营地纪念处。");return true;}
    if(Id==TEXT("loot_workshops")){Record(TEXT("workshop_cache"));Feedback=TEXT("旧料已登记；在锻造设施查看目录后领取。");return true;}
    if(Id==TEXT("loot_dwellings")){Record(TEXT("family_letter"));Record(TEXT("hunter_record"));Feedback=TEXT("匣里有一封家信和猎弓图纸记录。带回营地核对。");return true;}
    if(Id==TEXT("loot_assembly")){Record(TEXT("watch_record"));Feedback=TEXT("取回巡哨记录。回营地记录台抄录。");return true;}
    if(Id==TEXT("camp_memorial") && State.Facts.Contains(TEXT("old_carving"))){Record(TEXT("placed_carving"));Feedback=TEXT("旧物安放好了。这里也会成为家。");return true;}
    if(Id==TEXT("civilian_initial_01") && State.Facts.Contains(TEXT("family_letter"))){Record(TEXT("delivered_letter"));Feedback=TEXT("这是留给我的。谢谢你把它带回来。");return true;}
    if(Id==TEXT("camp_records") && State.Facts.Contains(TEXT("watch_record"))){Record(TEXT("copied_record"));Feedback=TEXT("巡哨见闻已抄录，只代表当时情况。");return true;}
    if(Id==TEXT("camp_hunter") && State.Facts.Contains(TEXT("hunter_record")))
    {G->KnownRecipes.Add(TEXT("craft_bow_rare"));Record(TEXT("hunter_confirmed"));Feedback=TEXT("猎弓结构核对完毕，制作知识已登记。");return true;}
    Feedback=TEXT("已查看此处。按 J 查看任务记录。");return true;
}
bool UHearthwardCampaignSubsystem::AssignEscort(FName Person,AHearthwardCompanionFixture* Brother,FGuid Epoch)
{
    auto* P=State.People.FindByPredicate([&](const auto& X){return X.Id==Person;});
    auto* A=Actor(Person);
    if(!P || !A || P->Stage==TEXT("uncontacted") || P->Stage==TEXT("arrived")
        || !IsValid(Brother) || !Player() || !Safe() || Busy()
        || Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()
        || FVector::Dist2D(Brother->GetActorLocation(),A->GetActorLocation())>300
        || FVector::Dist2D(Brother->GetActorLocation(),Player()->GetActorLocation())>3000)return false;
    P->Stage=TEXT("following");P->Escort=TEXT("brother");return true;
}
