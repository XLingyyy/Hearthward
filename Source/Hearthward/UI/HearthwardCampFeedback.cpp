#include "HearthwardCampFeedback.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardStorageSubsystem.h"

using namespace HearthwardData;

FString HearthwardCampFeedback::Unlocks(int32 Tier,bool Cumulative)
{
    TArray<FString> Names;
    for(int32 I=Cumulative?1:Tier;I<=Tier;++I)
        for(const auto& V:HearthwardCamp::Tier(I)->GetArrayField(TEXT("unlocks")))Names.AddUnique(V->AsString());
    return FString::Join(Names,TEXT(" / "));
}

FHearthwardCampUpgradeView HearthwardCampFeedback::ReadUpgrade(const FHearthwardCampState& State,
    const UHearthwardStorageSubsystem* Storage)
{
    FHearthwardCampUpgradeView V;V.MaxTier=State.Tier>=8;V.CurrentUnlocks=Unlocks(State.Tier,true);
    if(V.MaxTier){V.Conditions=TEXT("营地已满阶");return V;}
    V.ConditionsMet=true;V.MaterialsMet=Storage!=nullptr;V.NextUnlocks=Unlocks(State.Tier+1);
    const auto Next=HearthwardCamp::Tier(State.Tier+1);
    TArray<FString> Conditions;
    for(const auto& Entry:Next->GetArrayField(TEXT("conditions")))
    {
        FString Kind,Count;Entry->AsString().Split(TEXT(":"),&Kind,&Count);
        const int32 Required=FCString::Atoi(*Count);double Current=0;FString Label;
        if(Kind==TEXT("rescued_population")){Current=State.Rescued.Num();Label=TEXT("实际获救族人");}
        else if(Kind==TEXT("public_food_deposited_points")){Current=State.DonatedPoints;Label=TEXT("累计存入口粮点");}
        else if(Kind==TEXT("completed_distinct_production_regions"))
        {for(const auto& R:State.Regions)Current+=R.Completed>0;Label=TEXT("已完成批次的不同区域");}
        else if(Kind==TEXT("hometown_reclaimed")){Current=State.Hometown?1:0;Label=TEXT("永久夺回故乡");}
        else
        {
            for(const auto& B:State.Facilities)if(B.Kind==FName(*Kind))Current=FMath::Max(Current,double(B.Level));
            Label=Text(Find(TEXT("buildings"),Kind),TEXT("name"))+TEXT("等级（任一营地）");
        }
        const bool Met=Current>=Required;V.ConditionsMet&=Met;
        Conditions.Add(FString::Printf(TEXT("%s：%.0f / %d%s"),*Label,Current,Required,Met?TEXT(" · 满足"):TEXT(" · 未满足")));
    }
    V.Conditions=FString::Join(Conditions,TEXT("\n"));
    if(!Storage){V.Materials=TEXT("共享仓储当前不可用");return V;}
    TArray<FString> Materials;const auto Cost=HearthwardCamp::Counts(Next,TEXT("cost"));
    TArray<FName> Items;Cost.GetKeys(Items);Items.Sort(FNameLexicalLess());
    for(FName Item:Items)
    {
        const int32 Needed=Cost.FindChecked(Item),Available=Storage->Available(Item),Missing=FMath::Max(0,Needed-Available);
        V.MaterialsMet&=Missing==0;
        Materials.Add(FString::Printf(TEXT("%s：可用 %d / 需 %d%s"),*Text(Find(TEXT("items"),Item.ToString()),TEXT("name")),Available,Needed,
            Missing>0?*FString::Printf(TEXT(" · 缺 %d"),Missing):TEXT(" · 足够")));
    }
    V.Materials=FString::Join(Materials,TEXT("\n"));return V;
}

int32 HearthwardCampFeedback::AssignedWorkers(const FHearthwardCampState& State)
{
    TSet<int32> Assigned;for(const auto& R:State.Regions)for(int32 Person:R.Workers)Assigned.Add(Person);
    return Assigned.Num();
}

FString HearthwardCampFeedback::RegionStatus(const FHearthwardCampState& State,const FHearthwardCampRegion& R)
{
    if(R.Workers.IsEmpty() && !R.Player && !R.Brother)return TEXT("无人");
    if(!R.Enabled)return TEXT("已暂停");
    if(!R.Safe)return TEXT("区域不安全");
    if(R.Facility.IsValid())
    {
        const auto* B=State.Facilities.FindByPredicate([&](const auto& F){return F.Id==R.Facility;});
        if(!B)return TEXT("设施不可用");
        if(B->Paused)return TEXT("设施已暂停");
    }
    else if(!R.Batch.Active && State.SourceIndex(R)==INDEX_NONE)return TEXT("待资源刷新");
    if(R.Workers.IsEmpty() && R.PlayerEfficiency<=0 && R.BrotherEfficiency<=0)return TEXT("等待兄弟到岗");
    return R.Status.IsEmpty()?FString(TEXT("工作中")):R.Status;
}

FString HearthwardCampFeedback::UpgradeToken(FGuid Epoch,int32 Tier)
{return TEXT("camp.upgrade:")+Epoch.ToString(EGuidFormats::Digits)+TEXT(":")+FString::FromInt(Tier);}

void FHearthwardCampFeedback::Add(const FString& Text)
{
    Entries.Add(Text);if(Entries.Num()>3)Entries.RemoveAt(0);
    Notice=Text;++Revision;
}

void FHearthwardCampFeedback::Observe(FGuid CurrentEpoch,FGuid CurrentCampaign,
    const FHearthwardCampState& State,const TArray<FHearthwardNPCEvent>& Events)
{
    if(!CurrentEpoch.IsValid())return;
    if(Epoch!=CurrentEpoch || Campaign!=CurrentCampaign)
    {
        Epoch=CurrentEpoch;Campaign=CurrentCampaign;Population=State.Population();LastTier=State.Tier;
        People.Reset();for(FName Person:State.Rescued)People.Add(Person);
        Tiers.Reset();for(int32 I=1;I<=State.Tier;++I)Tiers.Add(I);
        EventIds.Reset();for(const auto& E:Events)EventIds.Add(E.Id);
        Entries.Reset();Notice.Reset();++Revision;return;
    }
    int32 NewPeople=0;for(FName Person:State.Rescued)if(!People.Contains(Person)){People.Add(Person);++NewPeople;}
    if(NewPeople>0)Add(FString::Printf(TEXT("族人回营：人口 %d → %d（救回 %d 名）"),Population,State.Population(),NewPeople));
    Population=State.Population();
    if(State.Tier>LastTier && !Tiers.Contains(State.Tier))
    {Tiers.Add(State.Tier);Add(FString::Printf(TEXT("营地已升至 S%d · 开放 %s"),State.Tier,*HearthwardCampFeedback::Unlocks(State.Tier)));}
    LastTier=State.Tier;
    for(const auto& E:Events)
    {
        if(!E.Id.IsValid() || EventIds.Contains(E.Id))continue;
        EventIds.Add(E.Id);
        if(E.Campaign!=Campaign || E.Kind!=TEXT("delivered") || !E.Reason.IsEmpty() || E.Count<=0 || !E.Command.IsValid())continue;
        Add(FString::Printf(TEXT("弟弟实际入库：%s ×%d"),*Text(Find(TEXT("items"),E.Item.ToString()),TEXT("name")),E.Count));
    }
}

FString FHearthwardCampFeedback::Summary() const
{return Entries.IsEmpty()?FString(TEXT("当前营地状态 · 本次会话尚无新回营回执")):TEXT("本次会话回营记录\n")+FString::Join(Entries,TEXT("\n"));}
