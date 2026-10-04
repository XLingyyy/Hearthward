#include "HearthwardScreenWidget.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
using namespace HearthwardData;
void UHearthwardScreenWidget::OpenNature(FGuid Target)
{
    NatureSelection=Target;NatureEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();OpenPage(TEXT("nature"));
}
void UHearthwardScreenWidget::ComposeNature()
{
    auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
    if(!NatureEpoch.IsValid())NatureEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    Element(TEXT("text"),TEXT("田野与牧场"),{180,95},{900,55},32);
    Element(TEXT("button"),TEXT("返回"),{1280,95},{210,45},20,TEXT("page:hud"));
    Element(TEXT("text"),TEXT("种植和建栏使用身前空地；目标操作需在三米内。操作耗时五秒，移动或受伤会中断。"),{180,155},{1320,45},18);
    int32 X=180;
    for(const auto& V:HearthwardNature::Rows(TEXT("crops")))
    {const auto D=V->AsObject();Element(TEXT("button"),TEXT("播种")+Text(D,TEXT("name")),{double(X),210},{195,42},18,TEXT("nature.plant:")+Text(D,TEXT("id")));X+=205;}
    for(const auto& V:HearthwardNature::Rows(TEXT("domestic")))
    {const auto D=V->AsObject();Element(TEXT("button"),TEXT("建")+Text(D,TEXT("name"))+TEXT("栏"),{double(X),210},{195,42},18,TEXT("nature.build_pen:")+Text(D,TEXT("id")));X+=205;}
    struct FRow{FGuid Id;FString Name;double Distance;};TArray<FRow> List;
    auto Add=[&](FGuid Id,FVector At,FString Name){const double Dist=FVector::Dist2D(GetOwningPlayerPawn()->GetActorLocation(),At);if(Dist<10000 || Id==NatureSelection)List.Add({Id,Name,Dist});};
    for(const auto& C:N->State.Crops)Add(C.Id,C.Position,Text(HearthwardNature::Definition(TEXT("crops"),C.Definition),TEXT("name"))+(N->State.Ready(C)?TEXT(" · 成熟"):TEXT(" · 生长中")));
    for(const auto& P:N->State.Pens)Add(P.Id,P.Position,Text(HearthwardNature::Definition(TEXT("domestic"),P.Definition),TEXT("name"))+TEXT("栏舍"));
    for(const auto& A:N->State.Animals)if(A.Health>0 || !A.Loot.IsEmpty())Add(A.Id,A.Position,Text(HearthwardNature::Definition(A.Domestic?TEXT("domestic"):TEXT("wildlife"),A.Definition),TEXT("name"))+(A.Health>0?TEXT(""):TEXT(" · 尸体")));
    auto* Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    for(const auto& R:N->State.Points)if(R.Kind==TEXT("resource")
        && FVector::Dist2D(GetOwningPlayerPawn()->GetActorLocation(),R.Position)<=3000)
        Add(R.Id,R.Position,Text(HearthwardNature::Definition(TEXT("resources"),R.Definition),TEXT("name")));
    for(const auto& P:N->State.Points)if(P.Kind==TEXT("fish") || (P.Kind==TEXT("treasure") && !N->State.Opened.Contains(P.Definition)))Add(P.Id,P.Position,P.Kind==TEXT("fish")?TEXT("钓鱼点"):TEXT("藏宝箱"));
    List.Sort([](const auto& A,const auto& B){return A.Distance<B.Distance;});Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,List.Num()-7));
    for(int32 I=Scroll;I<FMath::Min(Scroll+7,List.Num());++I)
    {const auto& R=List[I];Element(TEXT("button"),FString::Printf(TEXT("%s  ·  %.0f米"),*R.Name,R.Distance/100),{180.,290.+(I-Scroll)*60},{610,50},18,TEXT("nature.select:")+R.Id.ToString(),TEXT(""),R.Id==NatureSelection);}
    Element(TEXT("button"),TEXT("上一页"),{180,730},{180,42},18,TEXT("nature.prev"));Element(TEXT("button"),TEXT("下一页"),{380,730},{180,42},18,TEXT("nature.next"));
    const auto* C=N->State.Crops.FindByPredicate([&](const auto& R){return R.Id==NatureSelection;});
    const auto* P=N->State.Pens.FindByPredicate([&](const auto& R){return R.Id==NatureSelection;});
    const auto* A=N->State.Animals.FindByPredicate([&](const auto& R){return R.Id==NatureSelection;});
    const auto* F=N->State.Points.FindByPredicate([&](const auto& R){return R.Id==NatureSelection;});
    double Y=360;auto Button=[&](FString Text,FString Action){Element(TEXT("button"),Text,{880,Y},{590,40},19,Action);Y+=48;};
    FString Info;
    if(C)
    {
        Info=FString::Printf(TEXT("预计产量 %d  ·  浇水 %s  ·  施肥 %s"),N->State.Yield(*C),C->Watered?TEXT("完成"):TEXT("未做"),C->Fertilized?TEXT("完成"):TEXT("未做"));
        Button(TEXT("浇水"),TEXT("nature.water:"));Button(TEXT("施肥"),TEXT("nature.fertilize:"));Button(TEXT("收获并取回一粒种子"),TEXT("nature.harvest:"));
        Button(TEXT("请弟弟浇水"),TEXT("nature.brother_water:"));Button(TEXT("请弟弟施肥"),TEXT("nature.brother_fertilize:"));Button(TEXT("请弟弟收获"),TEXT("nature.brother_harvest:"));
    }
    if(P)
    {
        const FName Product(*Text(HearthwardNature::Definition(TEXT("domestic"),P->Definition),TEXT("product")));
        const auto ProductDef=Find(TEXT("items"),Product.ToString());
        Info=FString::Printf(TEXT("%d级栏舍  ·  动物及预留 %d / %d  ·  饲料 %d  ·  %s %d / %d"),P->Level,N->State.Occupants(P->Id),HearthwardNature::Capacity(P->Level),P->Feed,
            Product.IsNone()?TEXT("普通产物"):*Text(ProductDef,TEXT("name")),P->Products,HearthwardNature::ProductCapacity);
        Button(TEXT("放入10份饲料"),TEXT("nature.deposit_feed:"));Button(TEXT("取出全部饲料"),TEXT("nature.withdraw_feed:"));
        Button(TEXT("请弟弟放入10份饲料"),TEXT("nature.brother_deposit_feed:"));
        if(!Product.IsNone() && P->Products>0)
        {
            Button(TEXT("收取1份产物"),TEXT("nature.collect_product:"));
            Button(FString::Printf(TEXT("收取全部%d份产物"),P->Products),TEXT("nature.collect_product_all:"));
            Button(FString::Printf(TEXT("请弟弟收取并入库%d份产物"),P->Products),FString::Printf(TEXT("nature.brother_collect_product:%d"),P->Products));
        }
        Button(TEXT("升级：下一级材料为 60木 / 20石 / 10绳 × 等级"),TEXT("nature.upgrade_pen:"));
        Button(TEXT("移动空栏舍至前方"),TEXT("nature.move_pen:"));Button(TEXT("拆除空栏舍，返还累计材料80%"),TEXT("ask:nature.demolish_pen:confirmed"));
    }
    if(A)
    {
        Info=FString::Printf(TEXT("生命 %.0f  ·  %s  ·  已付饲养时间 %.1f 小时"),A->Health,A->Juvenile?TEXT("幼年"):TEXT("成年"),A->FedRemaining/60);
        if(A->Health<=0)Button(TEXT("取走肉与皮革"),TEXT("nature.loot:"));
        else if(A->Domestic)
        {Button(A->Captured?TEXT("继续牵引 / 转移到另一空栏"):TEXT("捕捉：消耗1饲料和1绳索"),A->Captured?TEXT("nature.lead:"):TEXT("nature.capture:"));Button(TEXT("请弟弟同行捕捉 / 牵引入栏"),TEXT("nature.brother_capture:"));if(A->Captured)Button(TEXT("屠宰（确认后执行）"),TEXT("ask:nature.slaughter:confirmed"));}
        else Button(TEXT("请弟弟同行狩猎此目标"),TEXT("nature.brother_hunt:"));
    }
    if(F)
    {
        Info=F->Kind==TEXT("fish")?FString::Printf(TEXT("鱼群剩余 %d / 24  ·  耗尽后两天恢复"),F->Remaining)
            : F->Kind==TEXT("resource")?FString::Printf(TEXT("资源点剩余 %d  ·  以实际采得和入库计数"),Camps->Source(F->Key.ToString())?Camps->Source(F->Key.ToString())->Remaining:0)
            : TEXT("藏宝奖励一次性领取，容量不足时保留全部内容");
        if(F->Kind==TEXT("fish"))
        {Button(TEXT("开始钓鱼：1鱼饵，成功磨损1耐久"),TEXT("nature.fish:"));Button(TEXT("请弟弟同行钓获1条"),TEXT("nature.brother_fish:"));}
        if(F->Kind==TEXT("resource"))
        {Button(TEXT("请弟弟采集并入库 4 份"),TEXT("nature.brother_collect:4"));Button(TEXT("请弟弟采集并入库 1 份"),TEXT("nature.brother_collect:1"));}
        else Button(TEXT("领取留存物品"),TEXT("nature.claim:"));
    }
    Element(TEXT("text"),Info,{880,290},{610,60},19);
    Element(TEXT("text"),Message.IsEmpty()?N->Feedback:Message,{180,815},{1310,70},18);
}
bool UHearthwardScreenWidget::ExecuteNatureAction(const FString& Action)
{
    if(Action==TEXT("nature.next") || Action==TEXT("nature.prev")){Scroll+=Action==TEXT("nature.next")?7:-7;Refresh();return true;}
    FString Command,Option;if(!Action.Mid(7).Split(TEXT(":"),&Command,&Option))return false;
    if(Command==TEXT("select")){FGuid::Parse(Option,NatureSelection);Refresh();return true;}
    if(Command.StartsWith(TEXT("brother_")))
    {
        const FName CareAction(*Command.Mid(8));
        const bool Collect=CareAction==TEXT("collect") || CareAction==TEXT("collect_product");
        const bool Hunt=CareAction==TEXT("hunt");
        const bool Fish=CareAction==TEXT("fish");
        const bool Capture=CareAction==TEXT("capture");
        auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        const auto* Point=CareAction==TEXT("collect")?N->State.Points.FindByPredicate([&](const auto& P){return P.Id==NatureSelection && P.Kind==TEXT("resource");}):nullptr;
        const auto* Pen=CareAction==TEXT("collect_product")?N->State.Pens.FindByPredicate([&](const auto& P){return P.Id==NatureSelection;}):nullptr;
        const auto* Animal=(Hunt || Capture)?N->State.Animals.FindByPredicate([&](const auto& A){return A.Id==NatureSelection && A.Domestic==Capture;}):nullptr;
        auto* AI=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();
        AHearthwardCompanionFixture* Brother=nullptr;
        for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It){Brother=*It;break;}
        FHearthwardAgentGoal Goal;
        Goal.Intent=Hunt?FName(TEXT("hunt")):Fish?FName(TEXT("fish")):Capture?FName(TEXT("capture")):Collect?FName(TEXT("nature_collect")):FName(TEXT("nature_care"));
        Goal.Item=Point?FName(*Text(HearthwardNature::Definition(TEXT("resources"),Point->Definition),TEXT("item")))
            :Pen?FName(*Text(HearthwardNature::Definition(TEXT("domestic"),Pen->Definition),TEXT("product"))):Animal?Animal->Definition:Fish?FName(TEXT("fish")):CareAction;
        Goal.Quantity=Collect?FCString::Atoi(*Option):(CareAction==TEXT("deposit_feed")?10:1);
        Goal.QuantityMode=Hunt || Capture?TEXT("one_animal"):Fish?TEXT("one_catch"):Collect?TEXT("additional_acquired"):TEXT("action_count");Goal.SourceRef=TEXT("known_target");Goal.Station=NatureSelection;
        OpenPage(TEXT("dialogue"));
        const bool OK=AI && AI->SetStructuredGoal(GetOwningPlayerPawn(),Brother,Goal);
        const FString Feedback=AI?AI->GetStatus():TEXT("伙伴对话未就绪");
        if(OK)Refresh();else {OpenPage(TEXT("nature"));Message=Feedback;Refresh();}
        return OK;
    }
    auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();int32 Count=Command==TEXT("deposit_feed")?10:1;
    if(Command==TEXT("withdraw_feed") || Command==TEXT("collect_product_all"))
    {const auto* P=N->State.Pens.FindByPredicate([&](const auto& X){return X.Id==NatureSelection;});Count=P?(Command==TEXT("withdraw_feed")?P->Feed:P->Products):0;}
    const FGuid Target=Command==TEXT("plant") || Command==TEXT("build_pen")?FGuid():NatureSelection;
    const FGuid Epoch=NatureEpoch;OpenPage(TEXT("hud"));
    const bool OK=N->Act(FName(Command==TEXT("collect_product_all")?TEXT("collect_product"):*Command),Target,FName(Option),Epoch,Count);Message=N->Feedback;
    if(!OK)OpenPage(TEXT("nature"));return OK;
}
