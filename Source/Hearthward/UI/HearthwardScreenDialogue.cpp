#include "HearthwardScreenWidget.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

namespace
{
AHearthwardCompanionFixture* DialogueBrother(UWorld* World)
{for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It)return *It;return nullptr;}
const FName TransportIntents[]={TEXT("store"),TEXT("retrieve"),TEXT("give"),TEXT("fetch"),TEXT("receive")};
const TCHAR* TransportRoutes[]={TEXT("弟弟背包 → 营地仓库"),TEXT("营地仓库 → 我的背包"),TEXT("弟弟背包 → 我的背包"),TEXT("营地仓库 → 弟弟背包"),TEXT("我的背包 → 弟弟背包")};
}
void UHearthwardScreenWidget::ComposeDialogue()
{
    auto* AI=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();
    auto* Brother=DialogueBrother(GetWorld());auto* Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    const bool Card=AI->HasCandidate();
    const float Scale=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f;
    auto Add=[&](FString Type,FString Label,float X,float Y,float W,float H,float Font,FString Action=FString(),bool Selected=false)->FHearthwardUIElement&
    {
        Element(Type,Label,{X,Y},{W,H},Font,Action,TEXT(""),Selected);auto& E=Elements.Last();
        E.Font=Font*Scale;E.FontRole=TEXT("body");E.Tracking=0;E.TextInset=0;
        E.Component=TEXT("dialogue.redesign");return E;
    };
    auto Text=[&](FString Label,float Y,float H=38,float Font=22)->FHearthwardUIElement&
    {return Add(TEXT("text"),Label,1070,Y,532,H,Font);};
    auto Button=[&](FString Label,float Y,FString Action,bool Primary=false)->FHearthwardUIElement&
    {auto& E=Add(Primary?TEXT("dialoguePrimary"):TEXT("dialogueButton"),Label,1070,Y,532,54,23,Action);E.Align=TEXT("center");return E;};
    auto Row=[&](FString Label,FString Sub,float Y,FString Action)
    {
        Add(TEXT("dialogueRow"),TEXT(""),1070,Y,532,80,22,Action);
        Add(TEXT("text"),Label,1090,Y+7,468,38,25);
        Add(TEXT("text"),Sub,1090,Y+46,468,29,18).Color=Color(TEXT("muted"));
        Add(TEXT("text"),TEXT("›"),1573,Y+20,24,40,28).Color=Color(TEXT("gold"));
    };
    auto ScrolledText=[&](const FString& Content,float Y,float Height)
    {
        TArray<FString> Paragraphs,Lines;Content.ParseIntoArrayLines(Paragraphs,false);
        const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        const FSlateFontInfo Font(Typeface,FMath::RoundToInt(21*Scale*.75f));
        for(FString P:Paragraphs)
        {
            while(P.Len()>1 && Measure->Measure(P,Font).X>532)
            {
                const int32 Count=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(P),Font,532)+1,1,P.Len());
                Lines.Add(P.Left(Count));P=P.Mid(Count);
            }
            Lines.Add(P);
        }
        const int32 Visible=FMath::Max(1,FMath::FloorToInt(Height/(21*Scale*1.6f)));
        Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Lines.Num()-Visible));TArray<FString> Slice;
        for(int32 I=Scroll;I<FMath::Min(Scroll+Visible,Lines.Num());++I)Slice.Add(Lines[I]);
        Text(FString::Join(Slice,TEXT("\n")),Y,Height,21);
        if(Lines.Num()>Visible)Text(TEXT("滚轮查看完整内容"),Y+Height+6,28,17).Color=Color(TEXT("muted"));
    };
    Add(TEXT("portrait"),TEXT(""),1070,64,64,64,20).Asset=TEXT("portrait");
    Add(TEXT("text"),TEXT("弟弟"),1154,62,225,45,34).Color=Color(TEXT("gold"));
    const FString Team=Camp->DescribeWorkParty();
    const bool Active=Brother && Brother->GetRequested()>Brother->GetDelivered() && Brother->GetPhase()!=EHearthwardCompanionPhase::Cancelled;
    Add(TEXT("text"),!Team.IsEmpty()?TEXT("营地采集队"):Active?TEXT("有个人委托"):TEXT("暂无委托"),1154,108,288,32,18).Color=Color(TEXT("muted"));
    Add(TEXT("menuAction"),TEXT("Esc 关闭"),1457,66,145,40,18,TEXT("page:hud")).Align=TEXT("right");
    Add(TEXT("dialogueRule"),TEXT(""),1070,156,532,1,18);
    if(Card)
    {
        Text(TEXT("确认这项安排"),184,48,29).Color=Color(TEXT("gold"));
        Text(AI->GetNPCLine(),238,72,21);
        ScrolledText(AI->GetCandidateText(),330,356);
        const auto Goal=AI->GetCandidate();const auto* Cap=HearthwardAgent::FindCapability(Goal.Intent);
        if(Cap && Cap->MaxQuantity>1 && Goal.Intent!=TEXT("camp_team"))
        {
            Add(TEXT("dialogueButton"),TEXT("数量 −"),1390,695,96,40,18,TEXT("agentLess:")+AI->GetCandidateId().ToString()).Align=TEXT("center");
            Add(TEXT("dialogueButton"),TEXT("数量 +"),1500,695,102,40,18,TEXT("agentMore:")+AI->GetCandidateId().ToString()).Align=TEXT("center");
        }
        Button(TEXT("确认委托"),752,TEXT("agentConfirm:")+AI->GetCandidateId().ToString(),true);
        Button(TEXT("暂不安排，返回对话"),820,TEXT("dialogue.home"));
    }
    else if(DialogueView==TEXT("home"))
    {
        FString Reply=AI->CanDisplay()?AI->GetNPCLine():FString();
        Text(Reply.IsEmpty()?TEXT("哥，我在。今天需要我帮你做什么？"):Reply,188,85,24);
        if(Reply.Len()>45)Add(TEXT("menuAction"),TEXT("查看完整回复 ›"),1070,278,532,34,18,TEXT("dialogue.reply")).Color=Color(TEXT("gold"));
        Row(TEXT("帮我采集物资"),TEXT("选择数量，采好后送回营地仓库"),322,TEXT("dialogue.gather"));
        Row(TEXT("帮我搬运物资"),TEXT("选清从哪里拿、送到谁的背包"),408,TEXT("dialogue.routes"));
        Row(TEXT("带族人一起工作"),TEXT("安排空闲族人，一起采集木材或石头"),494,TEXT("dialogue.team"));
        Row(TEXT("查看当前工作"),TEXT("查看进度、停工原因，或暂停采集队"),580,TEXT("dialogue.status"));
        Add(TEXT("menuAction"),TEXT("更多委托"),1070,675,220,38,20,TEXT("dialogue.more"));
        Add(TEXT("menuAction"),TEXT("记忆与约定"),1380,675,222,38,20,TEXT("page:memory")).Align=TEXT("right");
        Text(TEXT("也可以直接告诉我"),732,35,19).Color=Color(TEXT("muted"));
        auto& Send=Add(TEXT("dialoguePrimary"),AI->IsBusy()?TEXT("取消"):TEXT("发送"),1492,777,110,56,22,AI->IsBusy()?TEXT("cancelReply"):TEXT("send"));Send.Align=TEXT("center");
        FString Status=AI->IsBusy()?FString::Printf(TEXT("弟弟正在思考 · %.0f秒"),AI->GetElapsedSeconds()):AI->GetStatus();
        Text(Status,846,32,17).Color=Color(TEXT("muted"));
    }
    else
    {
        Add(TEXT("menuAction"),TEXT("‹ 返回对话"),1070,178,532,40,20,TEXT("dialogue.home"));
        if(DialogueView==TEXT("reply"))
        {Text(TEXT("弟弟的回复"),237,54,29);ScrolledText(AI->GetNPCLine(),314,460);}
        else if(DialogueView==TEXT("routes"))
        {
            Text(TEXT("物资从哪里拿，送到哪里？"),237,54,27);
            for(int32 I=0;I<5;++I)Row(TransportRoutes[I],I>=1?TEXT("按实际取出与交付数量结算"):TEXT("搬运弟弟已经持有的物资"),314+I*86,TEXT("dialogue.route:")+FString::FromInt(I));
        }
        else if(DialogueView==TEXT("status"))
        {
            Text(TEXT("当前工作"),236,50,29).Color=Color(TEXT("gold"));
            FString Detail=Team;
            if(Active)Detail+=FString::Printf(TEXT("\n个人委托：%s %d / %d，携带%d\n%s"),*HearthwardAgent::ItemText(Brother->GetGoal().Item),Brother->GetDelivered(),Brother->GetRequested(),Brother->GetCarried(),*Brother->BlockReason);
            Text(Detail.IsEmpty()?TEXT("当前没有进行中的工作。"):Detail,307,304,23);
            if(!Team.IsEmpty())
            {
                const FName Id=Camp->State.CampAt(GetOwningPlayerPawn()->GetActorLocation());
                const auto* R=Camp->State.Regions.FindByPredicate([&](const auto& V){return V.Camp==Id && V.Brother && !V.Facility.IsValid();});
                Button(R && R->Enabled?TEXT("暂停采集队"):TEXT("继续采集队"),658,R && R->Enabled?TEXT("dialogue.stopTeam"):TEXT("dialogue.resumeTeam"));
            }
            else if(Active)
            {
                Button(TEXT("尝试继续原委托"),588,TEXT("agentRetryPath"));
                Button(TEXT("取消个人委托"),658,TEXT("cancelTask"));
            }
            Button(TEXT("打开营地分工"),736,TEXT("page:camp"),true);
            Text(Message,805,70,18).Color=Color(TEXT("gold"));
        }
        else if(DialogueView==TEXT("more"))
        {
            Text(TEXT("其他安排"),237,54,29);
            Row(TEXT("制作、维修与其他委托"),TEXT("选择任务种类、物品和具体装备"),324,TEXT("dialogue.advanced"));
            Row(TEXT("查看木材库存"),TEXT("询问弟弟已知的仓储情况"),420,TEXT("agentInventory"));
            Row(TEXT("田野与牧场"),TEXT("指认自然资源、作物或动物后安排工作"),516,TEXT("page:nature"));
        }
        else if(DialogueView==TEXT("advanced"))
        {
            Text(TEXT("其他委托"),237,54,29);
            TArray<const FHearthwardAgentCapability*> Caps;
            for(const auto& C:HearthwardAgent::Capabilities())if(C.Id==TEXT("collect") || C.Id==TEXT("store") || C.Id==TEXT("retrieve") || C.Id==TEXT("give") || C.Id==TEXT("fetch") || C.Id==TEXT("receive") || C.Id==TEXT("craft") || C.Id==TEXT("repair") || C.Id==TEXT("escort"))Caps.Add(&C);
            AgentCapabilityIndex=FMath::Clamp(AgentCapabilityIndex,0,Caps.Num()-1);const auto& C=*Caps[AgentCapabilityIndex];
            AgentItemIndex=FMath::Clamp(AgentItemIndex,0,C.Items.Num()-1);
            FHearthwardAgentGoal Goal;Goal.Intent=C.Id;Goal.Item=C.Items[AgentItemIndex];Goal.Quantity=1;Goal.SourceRef=C.Sources[0];
            Button(TEXT("切换任务种类"),316,TEXT("agentTypeNext"));
            Text(HearthwardAgent::GoalText(Goal),395,150,22);
            Button(TEXT("选择物品：")+HearthwardAgent::ItemText(Goal.Item),566,TEXT("agentItemNext"));
            if(C.Id==TEXT("repair"))Button(FString::Printf(TEXT("切换装备实例：第%d件"),AgentInstanceIndex+1),635,TEXT("agentInstanceNext"));
            if(C.Id==TEXT("store"))Button(AgentSourceIndex==0?TEXT("来源：弟弟背包"):TEXT("来源：我的背包"),635,TEXT("agentSourceNext"));
            Button(TEXT("生成待确认任务单"),756,TEXT("agentCollectCard"),true);
        }
        else
        {
            const bool Party=DialogueView==TEXT("team"),Transport=DialogueView==TEXT("transport");
            Text(Party?TEXT("带族人一起采集"):Transport?TEXT("帮我搬运物资"):TEXT("帮我采集物资"),238,54,29).Color=Color(TEXT("gold"));
            Text(Party?TEXT("只安排空闲族人，持续采集至暂停"):Transport?TransportRoutes[DialogueTransport]:TEXT("采好后送回营地仓库"),299,42,20).Color=Color(TEXT("muted"));
            Text(Party?TEXT("一起采集什么"):Transport?TEXT("搬运什么"):TEXT("采集什么"),368,36,21);
            Button(HearthwardAgent::ItemText(DialogueJob)+(Party || Transport?TEXT("  › 点击切换"):TEXT(" · 当前安全采集点")),409,Party || Transport?TEXT("dialogue.item"):FString());
            Text(Party?TEXT("带几名族人（不含弟弟）"):TEXT("需要多少份"),492,36,21);
            Add(TEXT("dialogueButton"),TEXT("−"),1070,536,72,54,26,TEXT("dialogue.less")).Align=TEXT("center");
            Add(TEXT("text"),FString::FromInt(Party?DialogueWorkers:DialogueQuantity)+(Party?TEXT(" 名"):TEXT(" 份")),1170,546,320,44,28).Align=TEXT("center");
            Add(TEXT("dialogueButton"),TEXT("+"),1530,536,72,54,26,TEXT("dialogue.moreQuantity")).Align=TEXT("center");
            FString Summary=Party?FString::Printf(TEXT("弟弟 + %d名族人 → %s采集岗位\n产物自动存入营地仓库。"),DialogueWorkers,*HearthwardAgent::ItemText(DialogueJob)):
                Transport?FString::Printf(TEXT("搬运%d份%s\n%s"),DialogueQuantity,*HearthwardAgent::ItemText(DialogueJob),TransportRoutes[DialogueTransport]):
                FString::Printf(TEXT("弟弟采集%d份木材，送回营地仓库。\n此处还剩%d份；采尽后暂停并报告。"),DialogueQuantity,Brother && Brother->Source?Brother->Source->GetItemCount(TEXT("wood")):0);
            Text(Summary,621,110,22);
            Button(TEXT("核对委托"),756,TEXT("dialogue.propose"),true);
            Text(Message.IsEmpty()?TEXT("确认前不会改变人员分工或现有委托。") : Message,821,66,18).Color=Color(TEXT("muted"));
        }
    }
    Text(Card || DialogueView!=TEXT("home")?TEXT("Esc 返回上一层"):TEXT("Enter 发送    ·    Esc 返回游戏"),887,26,16).Color=Color(TEXT("muted"));
}
bool UHearthwardScreenWidget::ExecuteDialogueAction(const FString& Action)
{
    if(Page!=TEXT("dialogue"))return false;
    auto* AI=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();auto* Brother=DialogueBrother(GetWorld());
    const FString Verb=Action.Mid(9);bool Success=true;
    if(Verb==TEXT("home"))
    {
        if(AI->HasCandidate() || AI->IsBusy() || AI->GetClarificationTurns()>0)AI->ClearClarification();
        DialogueView=TEXT("home");Scroll=0;Message.Reset();
    }
    else if(Verb==TEXT("less") || Verb==TEXT("moreQuantity"))
    {int32& N=DialogueView==TEXT("team")?DialogueWorkers:DialogueQuantity;N=FMath::Clamp(N+(Verb==TEXT("less")?-1:1),1,DialogueView==TEXT("team")?4:HearthwardAgent::Policy(TEXT("max_collect")));}
    else if(Verb==TEXT("item"))
    {
        if(DialogueView==TEXT("team"))DialogueJob=DialogueJob==TEXT("wood")?TEXT("stone"):TEXT("wood");
        else {const auto* C=HearthwardAgent::FindCapability(TransportIntents[DialogueTransport]);DialogueJob=C->Items[(C->Items.IndexOfByKey(DialogueJob)+1)%C->Items.Num()];}
    }
    else if(Verb.StartsWith(TEXT("route:")))
    {DialogueTransport=FMath::Clamp(FCString::Atoi(*Verb.Mid(6)),0,4);DialogueView=TEXT("transport");DialogueJob=TEXT("wood");}
    else if(Verb==TEXT("propose") || Verb==TEXT("stopTeam") || Verb==TEXT("resumeTeam"))
    {
        FHearthwardAgentGoal Goal;Goal.Item=DialogueJob;Goal.Quantity=DialogueQuantity;
        Goal.Intent=DialogueView==TEXT("team")?FName(TEXT("camp_team")):DialogueView==TEXT("transport")?TransportIntents[DialogueTransport]:FName(TEXT("collect"));
        if(Goal.Intent==TEXT("camp_team"))Goal.Quantity=DialogueWorkers;
        if(Verb==TEXT("stopTeam") || Verb==TEXT("resumeTeam"))
        {
            Goal.Intent=Verb==TEXT("stopTeam")?TEXT("camp_team_stop"):TEXT("camp_team");Goal.Quantity=1;
            auto* Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();const FName Id=Camp->State.CampAt(GetOwningPlayerPawn()->GetActorLocation());
            const auto* Region=Camp->State.Regions.FindByPredicate([&](const auto& R){return R.Camp==Id && R.Brother && !R.Facility.IsValid();});
            if(!Region)return false;Goal.Item=Region->Job;
            if(Verb==TEXT("resumeTeam"))Goal.Quantity=FMath::Max(1,Region->Workers.Num());
        }
        const auto* C=HearthwardAgent::FindCapability(Goal.Intent);Goal.QuantityMode=C->QuantityMode;Goal.SourceRef=C->Sources[0];
        Success=AI->SetStructuredGoal(GetOwningPlayerPawn(),Brother,Goal);Message=AI->GetNPCLine();Scroll=0;
    }
    else if(Verb==TEXT("gather") || Verb==TEXT("team") || Verb==TEXT("routes") || Verb==TEXT("status") || Verb==TEXT("more") || Verb==TEXT("advanced") || Verb==TEXT("reply"))
    {DialogueView=FName(*Verb);DialogueJob=TEXT("wood");Message.Reset();Scroll=0;}
    else return false;
    Refresh();return Success;
}
