#include "HearthwardPersistentInteractionState.h"

namespace
{
TMap<FName, FHearthwardPersistentDefinition>& Definitions()
{
    static TMap<FName, FHearthwardPersistentDefinition> Value=[]
    {
        TMap<FName,FHearthwardPersistentDefinition> Result;
        auto Add=[&](const TCHAR* Id,EHearthwardPersistentKind Kind){FHearthwardPersistentDefinition D;D.Kind=Kind;Result.Add(FName(Id),D);};
        for(const TCHAR* Id:{TEXT("workshops_lamp_entry"),TEXT("workshops_lamp_yard"),TEXT("cellar_lamp_entry"),TEXT("cellar_lamp_store")})Add(Id,EHearthwardPersistentKind::Lamp);
        for(const TCHAR* Id:{TEXT("workshops_inner_latch"),TEXT("workshops_drop_ladder"),TEXT("cellar_inner_latch"),TEXT("cellar_drop_ladder")})Add(Id,EHearthwardPersistentKind::Shortcut);
        for(const TCHAR* Id:{TEXT("workshops_winch"),TEXT("workshops_beam"),TEXT("cellar_winch"),TEXT("cellar_beam")})Add(Id,EHearthwardPersistentKind::Cooperation);
        for(const TCHAR* Id:{TEXT("civilian_initial_02"),TEXT("civilian_initial_03"),TEXT("civilian_initial_04")})
        {
            Add(Id,EHearthwardPersistentKind::Person);
            Result[FName(Id)].Steps={TEXT("met"),TEXT("visited"),TEXT("returned"),TEXT("remembered")};
            Result[FName(Id)].Choices={TEXT("camp"),TEXT("hometown")};
        }
        Add(TEXT("world_light_rain"),EHearthwardPersistentKind::Weather);
        for(const TCHAR* Species:{TEXT("deer"),TEXT("boar")})for(int32 I=0;I<2;++I)
        {
            FHearthwardPersistentDefinition D;D.Kind=EHearthwardPersistentKind::Trail;D.MaxTrailSamples=24;D.TrailSpecies=FName(Species);
            Result.Add(FName(*FString::Printf(TEXT("nature_%s%d"),Species,I)),D);
        }
        for(const TCHAR* Camp:{TEXT("camp"),TEXT("hometown")})for(int32 Slot=1;Slot<=3;++Slot)
        {
            const FName Id(*FString::Printf(TEXT("display_%s_%d"),Camp,Slot));
            FHearthwardPersistentDefinition D;D.Kind=EHearthwardPersistentKind::Display;D.Camp=FName(Camp);D.Slot=FName(*FString::FromInt(Slot));
            D.DisplayFacts={TEXT("craftsman_tools"),TEXT("herbalist_record"),TEXT("watcher_memento")};
            D.DisplayOwners={{TEXT("craftsman_tools"),TEXT("civilian_initial_02")},
                {TEXT("herbalist_record"),TEXT("civilian_initial_03")},{TEXT("watcher_memento"),TEXT("civilian_initial_04")}};
            Result.Add(Id,D);
        }
        return Result;
    }();
    return Value;
}
}
bool HearthwardPersistence::Register(FName Id, const FHearthwardPersistentDefinition& Definition)
{
    if(Id.IsNone() || Definitions().Contains(Id))return false;
    Definitions().Add(Id,Definition);
    return true;
}
const FHearthwardPersistentDefinition* HearthwardPersistence::Find(FName Id, EHearthwardPersistentKind Kind)
{
    const auto* D=Definitions().Find(Id);
    return D && D->Kind==Kind?D:nullptr;
}
#if WITH_DEV_AUTOMATION_TESTS
void HearthwardPersistence::UnregisterFixture(FName Id)
{
    if(Id.ToString().StartsWith(TEXT("PROTOTYPE_ONLY_")))Definitions().Remove(Id);
}
#endif
bool FHearthwardPersistentInteractionState::IsEmpty() const
{
    return Revision==0 && Lamps.IsEmpty() && Shortcuts.IsEmpty() && Cooperation.IsEmpty() && People.IsEmpty()
        && Weather.IsEmpty() && Trails.IsEmpty() && Displays.IsEmpty();
}
bool FHearthwardPersistentInteractionState::Serialize(FArchive& Ar)
{
    if(Ar.IsLoading())Version=0;
    StaticStruct()->SerializeTaggedProperties(Ar,reinterpret_cast<uint8*>(this),StaticStruct(),nullptr);
    return true;
}
bool FHearthwardPersistentInteractionState::Validate(double CalendarMinutes, FString& Error) const
{
    auto Fail=[&](const TCHAR* Message){Error=Message;return false;};
    if(Version!=1 || Revision<0 || !FMath::IsFinite(CalendarMinutes) || CalendarMinutes<0)
        return Fail(TEXT("新增交互状态版本或时间无效，原档已保留"));
    TSet<FName> Seen;
    auto Definition=[&](FName Id,EHearthwardPersistentKind Kind)
    {
        const auto* D=HearthwardPersistence::Find(Id,Kind);
        if(!D || Seen.Contains(Id))return static_cast<const FHearthwardPersistentDefinition*>(nullptr);
        Seen.Add(Id);return D;
    };
    for(const auto& S:Lamps)if(!Definition(S.Id,EHearthwardPersistentKind::Lamp))return Fail(TEXT("未知或重复灯具身份"));
    for(const auto& S:Shortcuts)if(!Definition(S.Id,EHearthwardPersistentKind::Shortcut))return Fail(TEXT("未知或重复捷径身份"));
    for(const auto& S:Cooperation)
        if(!Definition(S.Id,EHearthwardPersistentKind::Cooperation) || uint8(S.Result)>uint8(EHearthwardCooperationResult::Cleared))return Fail(TEXT("协作结果无效"));
    for(const auto& S:People)
    {
        const auto* D=Definition(S.Id,EHearthwardPersistentKind::Person);
        if(!D || (!S.Step.IsNone() && !D->Steps.Contains(S.Step)) || (!S.Choice.IsNone() && !D->Choices.Contains(S.Choice)))return Fail(TEXT("人物步骤或选择无定义"));
    }
    for(const auto& S:Weather)
        if(!Definition(S.Id,EHearthwardPersistentKind::Weather) || S.Seed==0 || uint8(S.Phase)>uint8(EHearthwardWeatherPhase::Rain)
            || !FMath::IsFinite(S.PhaseStartedAt) || S.PhaseStartedAt<0 || S.PhaseStartedAt>CalendarMinutes)return Fail(TEXT("天气状态或时间无效"));
    for(const auto& S:Trails)
    {
        const auto* D=Definition(S.Id,EHearthwardPersistentKind::Trail);
        if(!D || S.Generation<0 || D->MaxTrailSamples<=0 || S.Samples.Num()>D->MaxTrailSamples)return Fail(TEXT("踪迹身份或样本上限无效"));
        if(!D->TrailSpecies.IsNone() && (!S.Animal.IsValid() || S.Species!=D->TrailSpecies || S.Generation<1))return Fail(TEXT("踪迹物种或个体身份无效"));
        double Previous=-1;
        for(const auto& Sample:S.Samples)
        {
            if(Sample.Position.ContainsNaN() || !FMath::IsFinite(Sample.CalendarMinutes) || Sample.CalendarMinutes<0
                || Sample.CalendarMinutes<Previous || Sample.CalendarMinutes>CalendarMinutes)return Fail(TEXT("踪迹样本位置或时间无效"));
            Previous=Sample.CalendarMinutes;
        }
    }
    TSet<FGuid> Instances;
    TSet<FName> DisplayFacts;
    for(const auto& S:Displays)
    {
        const auto* D=Definition(S.Id,EHearthwardPersistentKind::Display);
        if(!D || S.Camp!=D->Camp || S.Slot!=D->Slot || S.Camp.IsNone() || S.Slot.IsNone())return Fail(TEXT("陈列营地或槽位无效"));
        if(S.Instance.IsValid())
        {
            if(!S.Fact.IsNone() || !D->DisplayDefinitions.Contains(S.Definition) || Instances.Contains(S.Instance))return Fail(TEXT("陈列装备重复或身份无效"));
            Instances.Add(S.Instance);
        }
        else if(!S.Definition.IsNone() || (!S.Fact.IsNone() && (!D->DisplayFacts.Contains(S.Fact) || DisplayFacts.Contains(S.Fact))))return Fail(TEXT("陈列事实无效或重复"));
        if(const FName* Owner=D->DisplayOwners.Find(S.Fact))
        {
            const auto* Person=People.FindByPredicate([&](const auto& P){return P.Id==*Owner;});
            if(!Person || Person->Step!=TEXT("remembered") || Person->Choice!=S.Camp)
                return Fail(TEXT("陈列纪念与人物完成记录或营地选择不符，原档已保留"));
        }
        if(!S.Fact.IsNone())DisplayFacts.Add(S.Fact);
    }
    return true;
}
