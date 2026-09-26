#include "HearthwardScreenWidget.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameData.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

using namespace HearthwardData;
namespace
{
FString CampCost(const TMap<FName,int32>& Cost)
{FString Text;for(const auto& C:Cost)Text+=HearthwardData::Text(Find(TEXT("items"),C.Key.ToString()),TEXT("name"))+FString::Printf(TEXT(" %d  "),C.Value);return Text.IsEmpty()?TEXT("无"):Text;}
FString RegionName(const FHearthwardCampRegion& R)
{
    const FString Prefix=R.Camp==TEXT("hometown")?TEXT("故乡 · "):TEXT("营地 · ");
    const FString Id=R.Id.ToString();
    if(R.Facility.IsValid())
    {
        const auto Recipe=HearthwardCamp::Recipe(R.Job);
        return Prefix+(Recipe?Text(Find(TEXT("buildings"),Text(Recipe,TEXT("facility"))),TEXT("name")):TEXT("待设置加工区"));
    }
    return Prefix+(Id.EndsWith(TEXT("forage"))?TEXT("采食区"):Id.EndsWith(TEXT("wood"))?TEXT("伐木区"):Id.EndsWith(TEXT("stone"))?TEXT("采石区"):Id.EndsWith(TEXT("ore"))?TEXT("采矿区"):TEXT("加工区"));
}
TArray<FName> CampFoods()
{
    TArray<FName> Result;for(const auto& V:Rows(TEXT("items")))if(HearthwardCamp::FoodPoints(FName(*Text(V->AsObject(),TEXT("id"))))>0)Result.Add(FName(*Text(V->AsObject(),TEXT("id"))));return Result;
}
}
void UHearthwardScreenWidget::ComposeCamp()
{
    auto* E=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();const auto& State=E->State;
    auto Button=[&](FString Label,FString Action,float X,float Y,float Width=210){Element(TEXT("button"),Label,FVector2D(X,Y),FVector2D(Width,46),19,Action);};
    auto Label=[&](FString Text,float X,float Y,float Width=1100,float Height=40){Element(TEXT("text"),Text,FVector2D(X,Y),FVector2D(Width,Height),20);};
    Label(FString::Printf(TEXT("营地 %d / 8 阶     半径 %.0f 米     族人 %d     公共口粮 %.1f"),State.Tier,State.Radius()/100,State.Population(),State.Rations()),180,95);
    int32 Tab=0;for(const auto& Pair:TArray<TPair<FString,FString>>{{TEXT("发展"),TEXT("growth")},{TEXT("分工与生产"),TEXT("workers")},{TEXT("设施管理"),TEXT("facilities")},{TEXT("公共口粮"),TEXT("food")}})
        Button(Pair.Key,TEXT("camp.tab:")+Pair.Value,180+Tab++*270,155,240);
    if(Category.IsEmpty())Category=TEXT("growth");
    if(Category==TEXT("growth"))
    {
        Label(FString::Printf(TEXT("营地成长：生命上限 +%.0f，耐力上限 +%.0f，两兄弟各享一份"),State.Bonus(TEXT("cumulative_hp_bonus")),State.Bonus(TEXT("cumulative_stamina_bonus"))),180,240);
        Label(TEXT("满足条件、提交共享仓储材料后升阶；建筑等级独立。"),180,295);
        if(State.Tier<8)
        {
            Label(TEXT("下一阶材料：")+CampCost(HearthwardCamp::Counts(HearthwardCamp::Tier(State.Tier+1),TEXT("cost"))),180,360,1260,80);
            const FString Reason=State.UpgradeReason();Label(Reason.IsEmpty()?TEXT("发展条件已满足；仍需足额材料"):Reason,180,460);
            Button(TEXT("提交材料升阶"),TEXT("camp.upgrade"),180,535,300);
        }
        Label(TEXT("首次工作台需木材72；建造优先用背包，差额从共享仓储预留。"),180,620);
        Button(TEXT("打开建造目录"),TEXT("page:building"),180,680,300);
    }
    if(Category==TEXT("workers"))
    {
        if(State.Regions.IsEmpty()){Label(TEXT("尚未建立营地"),180,250);return;}
        auto* R=State.Regions.FindByPredicate([&](const auto& Entry){return Entry.Id==CampRegion;});if(!R){CampRegion=State.Regions[0].Id;R=&State.Regions[0];}
        Label(RegionName(*R),180,235);Button(TEXT("切换生产区域"),TEXT("camp.regionNext"),1050,225,320);
        FString People;for(int32 P:R->Workers)People+=FString::Printf(TEXT("族人%d  "),P+1);if(R->Player)People+=TEXT("主角  ");if(R->Brother)People+=TEXT("弟弟");
        Label(TEXT("岗位（最多5人）：")+(People.IsEmpty()?TEXT("未分配"):People),180,295);
        Label(TEXT("选中人员：")+(CampPerson==30?TEXT("主角，劳动等效3人"):CampPerson==31?TEXT("弟弟，劳动等效3人"):FString::Printf(TEXT("族人%d"),CampPerson+1)),180,350);
        Button(TEXT("上一人"),TEXT("camp.personPrev"),750,340,170);Button(TEXT("下一人"),TEXT("camp.personNext"),950,340,170);Button(TEXT("分配 / 移出"),TEXT("camp.assign"),1150,340,210);
        Button(R->Enabled?TEXT("暂停生产"):TEXT("开始生产"),TEXT("camp.toggle"),180,410,240);
        Button(R->ToRations?TEXT("产物入粮：开"):TEXT("产物入粮：关"),TEXT("camp.rationToggle"),450,410,260);
        Button(TEXT("优先取料"),TEXT("camp.priority"),740,410,240);
        Button(TEXT("取消批次…"),TEXT("camp.cancelAsk"),1010,410,300);
        FString Details=R->Enabled?TEXT("已启用"):TEXT("已暂停");Details+=TEXT("   ")+R->Status;
        if(R->Batch.Active)Details+=FString::Printf(TEXT("\n本批劳动 %.1f / %.1f；已投入："),R->Batch.Work,R->Batch.Required)+CampCost(R->Batch.Inputs);
        Label(Details,180,485,1200,110);
        if(R->Facility.IsValid())
        {
            Label(TEXT("在“设施管理”选设施和配方，再点击“设为本营地后台生产”。"),180,630);
        }
        else Label(TEXT("需要区域内真实资源点；缺少来源时等待，不会凭空产出。"),180,630);
        Label(TEXT("兄弟须到达对应设施／源点旁并停止其他工作；睡眠不计兄弟劳动。"),180,680);
    }
    if(Category==TEXT("facilities"))
    {
        Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,State.Facilities.Num()-5));
        for(int32 I=Scroll;I<FMath::Min(Scroll+5,State.Facilities.Num());++I)
        {
            const auto& B=State.Facilities[I];
            Button(Text(Find(TEXT("buildings"),B.Kind.ToString()),TEXT("name"))+FString::Printf(TEXT("  %d级"),B.Level),TEXT("camp.facility:")+B.Id.ToString(),180,230+(I-Scroll)*64,330);
        }
        Button(TEXT("上一组"),TEXT("camp.prev"),180,565,150);Button(TEXT("下一组"),TEXT("camp.next"),355,565,155);
        const auto* B=State.Facilities.FindByPredicate([&](const auto& Entry){return Entry.Id==CampFacility;});
        if(!B && !State.Facilities.IsEmpty()){CampFacility=State.Facilities[0].Id;B=&State.Facilities[0];}
        if(B)
        {
            Label(TEXT("升级费用：")+CampCost(HearthwardCamp::BuildCost(B->Kind,B->Level+1)),570,235,820,75);
            Button(TEXT("靠近后升级"),TEXT("camp.facilityUpgrade"),570,320,240);Button(TEXT("免费移动"),TEXT("camp.move"),840,320,230);Button(TEXT("拆除…"),TEXT("camp.demolishAsk"),1100,320,220);
            Label(TEXT("拆除返还：")+CampCost(HearthwardCamp::Refund(B->Paid)),570,390,830,85);
            TArray<TSharedPtr<FJsonObject>> Recipes;
            for(const auto& V:HearthwardCamp::Table()->GetArrayField(TEXT("recipes")))if(Text(V->AsObject(),TEXT("facility"))==B->Kind.ToString() && Number(V->AsObject(),TEXT("level"))<=B->Level)Recipes.Add(V->AsObject());
            if(!Recipes.IsEmpty())
            {
                auto Selected=Recipes.FindByPredicate([&](const auto& R){return Text(R,TEXT("id"))==CampRecipe.ToString();});
                const auto Recipe=Selected?*Selected:Recipes[0];CampRecipe=FName(*Text(Recipe,TEXT("id")));
                Label(TEXT("配方：")+CampCost(HearthwardCamp::Counts(Recipe,TEXT("outputs"))),570,490,850);
                Label(TEXT("每批投入：")+CampCost(HearthwardCamp::Counts(Recipe,TEXT("inputs"))),570,545,850);
                Button(TEXT("切换配方"),TEXT("camp.recipeNext"),570,605,240);Button(TEXT("即时加工1批"),TEXT("camp.craft"),840,605,320);
                Button(TEXT("设为本营地后台生产"),TEXT("camp.configure"),570,675,590);
            }
            else Label(TEXT("该设施当前没有已解锁的加工配方。"),570,520,850);
            if(B->Kind==TEXT("bed"))Button(TEXT("睡眠八小时"),TEXT("camp.sleep"),570,625,400);
        }
        else Label(TEXT("尚无已建造设施，请先打开建造目录。"),570,260,850);
    }
    if(Category==TEXT("food"))
    {
        Label(TEXT("固定消耗24点／游戏日，两营地只扣一份。每餐5点，恢复最多40饱食。"),180,245);
        Label(TEXT("公共口粮用尽不影响普通族人劳动；兄弟仍需进食。"),180,305);
        const auto Foods=CampFoods();if(!Foods.IsEmpty())
        {
            CampFood=FMath::Clamp(CampFood,0,Foods.Num()-1);const FName Item=Foods[CampFood];
            Label(Text(Find(TEXT("items"),Item.ToString()),TEXT("name"))+FString::Printf(TEXT("  共享仓储%d份；每份 %.1f口粮点"),GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Available(Item),HearthwardCamp::FoodPoints(Item)),180,390);
            Button(TEXT("切换食物"),TEXT("camp.foodNext"),180,455,280);Button(TEXT("存入1份食物"),TEXT("camp.donate"),500,455,280);
        }
        Button(TEXT("主角吃一餐"),TEXT("camp.eatPlayer"),180,555,300);Button(TEXT("请营地里的弟弟吃一餐"),TEXT("camp.eatBrother"),540,555,420);
        Label(TEXT("食物不会自动从共享仓储转成口粮；公共口粮不能反向取成背包食物。"),180,665);
    }
    Button(TEXT("返回游戏"),TEXT("page:hud"),1120,765,300);
}
bool UHearthwardScreenWidget::ExecuteCampAction(const FString& Action)
{
    if(Page!=TEXT("camp"))return false;
    auto* E=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();auto* Builder=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>();bool Success=true;
    auto* R=E->State.Regions.FindByPredicate([&](const auto& Entry){return Entry.Id==CampRegion;});
    auto* B=E->State.Facilities.FindByPredicate([&](const auto& Entry){return Entry.Id==CampFacility;});
    E->Feedback.Reset();
    if(Action.StartsWith(TEXT("camp.tab:"))) {Category=Action.Mid(9);Scroll=0;}
    else if(Action==TEXT("camp.upgrade"))Success=E->UpgradeCamp(CampEpoch);
    else if(Action==TEXT("camp.regionNext")) {int32 I=E->State.Regions.IndexOfByPredicate([&](const auto& V){return V.Id==CampRegion;});if(!E->State.Regions.IsEmpty())CampRegion=E->State.Regions[(I+1)%E->State.Regions.Num()].Id;}
    else if(Action==TEXT("camp.personNext") || Action==TEXT("camp.personPrev"))
    {TArray<int32> People;for(int32 I=0;I<E->State.Population();++I)People.Add(I);People.Add(30);People.Add(31);int32 I=People.IndexOfByKey(CampPerson);CampPerson=People[(I+(Action.EndsWith(TEXT("Next"))?1:People.Num()-1))%People.Num()];}
    else if(Action==TEXT("camp.assign"))Success=E->AssignWorker(CampRegion,CampPerson,CampEpoch);
    else if(Action==TEXT("camp.toggle") && R)Success=E->SetProduction(CampRegion,!R->Enabled,R->ToRations,CampEpoch);
    else if(Action==TEXT("camp.rationToggle") && R)Success=E->SetProduction(CampRegion,R->Enabled,!R->ToRations,CampEpoch);
    else if(Action==TEXT("camp.priority"))Success=E->Prioritize(CampRegion,CampEpoch);
    else if(Action==TEXT("camp.cancelAsk")){ConfirmAction=TEXT("camp.cancel");ConfirmMessage=TEXT("取消后不会返还已投入材料：\n")+(R?CampCost(R->Batch.Inputs):TEXT("无"));}
    else if(Action==TEXT("camp.cancel"))Success=E->CancelBatch(CampRegion,true,CampEpoch);
    else if(Action.StartsWith(TEXT("camp.facility:"))){FGuid::Parse(Action.Mid(14),CampFacility);CampRecipe=NAME_None;}
    else if(Action==TEXT("camp.prev"))Scroll-=5;
    else if(Action==TEXT("camp.next"))Scroll+=5;
    else if(Action==TEXT("camp.demolishAsk")){ConfirmAction=TEXT("camp.demolish");TMap<FName,int32> Lost;
        for(const auto& Region:E->State.Regions)if(Region.Facility==CampFacility)for(const auto& Input:Region.Batch.Inputs)Lost.FindOrAdd(Input.Key)+=Input.Value;
        ConfirmMessage=TEXT("未完成投入损失：")+CampCost(Lost)+TEXT("\n返还建材：")+(B?CampCost(HearthwardCamp::Refund(B->Paid)):TEXT("无"));}
    else if(Action==TEXT("camp.demolish")){Success=Builder->DemolishFacility(CampFacility,true,CampEpoch);E->Feedback=Builder->Feedback;}
    else if(Action==TEXT("camp.facilityUpgrade") || Action==TEXT("camp.move"))
    {const FGuid Epoch=CampEpoch,Id=CampFacility;OpenPage(TEXT("hud"));Success=Action==TEXT("camp.move")?Builder->MoveFacility(Id,Epoch):Builder->UpgradeFacility(Id,Epoch);E->Feedback=Builder->Feedback;}
    else if(Action==TEXT("camp.craft"))Success=E->Craft(CampFacility,CampRecipe,1,CampEpoch);
    else if(Action==TEXT("camp.sleep")){const FGuid Epoch=CampEpoch,Id=CampFacility;OpenPage(TEXT("hud"));Success=E->Sleep(Id,Epoch);}
    else if(Action==TEXT("camp.configure") && B)
    {const FName Region(*(TEXT("facility_")+B->Id.ToString()));Success=E->SelectProduction(Region,B->Id,CampRecipe,CampEpoch);if(Success){CampRegion=Region;Category=TEXT("workers");}}
    else if(Action==TEXT("camp.recipeNext") && B)
    {TArray<FName> Choices;for(const auto& V:HearthwardCamp::Table()->GetArrayField(TEXT("recipes")))if(Text(V->AsObject(),TEXT("facility"))==B->Kind.ToString() && Number(V->AsObject(),TEXT("level"))<=B->Level)Choices.Add(FName(*Text(V->AsObject(),TEXT("id"))));if(!Choices.IsEmpty())CampRecipe=Choices[(Choices.IndexOfByKey(CampRecipe)+1)%Choices.Num()];}
    else if(Action==TEXT("camp.foodNext")){const auto Foods=CampFoods();if(!Foods.IsEmpty())CampFood=(CampFood+1)%Foods.Num();}
    else if(Action==TEXT("camp.donate")){const auto Foods=CampFoods();Success=Foods.IsValidIndex(CampFood) && E->DonateFood(Foods[CampFood],1,CampEpoch);}
    else if(Action==TEXT("camp.eatPlayer") || Action==TEXT("camp.eatBrother"))Success=E->EatMeal(Action.EndsWith(TEXT("Brother")),CampEpoch);
    else Success=false;
    Message=E->Feedback;if(!Success && Message.IsEmpty())Message=TEXT("操作不可用，请检查位置、材料与当前时间线");Refresh();return Success;
}
