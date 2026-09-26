#include "HearthwardCampState.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryState.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"

namespace HearthwardCamp
{
const TSharedPtr<FJsonObject>& Table() { return HearthwardData::Catalog()->GetObjectField(TEXT("campEconomy")); }
TSharedPtr<FJsonObject> Tier(int32 Level)
{
    for(const auto& V:Table()->GetArrayField(TEXT("camp_tiers")))
        if(HearthwardData::Number(V->AsObject(),TEXT("tier"))==Level) return V->AsObject();
    return nullptr;
}
TSharedPtr<FJsonObject> Facility(FName Kind,int32 Level)
{
    for(const auto& V:Table()->GetArrayField(TEXT("facilities")))
        if(HearthwardData::Text(V->AsObject(),TEXT("id"))==Kind.ToString() && HearthwardData::Number(V->AsObject(),TEXT("level"))==Level) return V->AsObject();
    return nullptr;
}
TSharedPtr<FJsonObject> Recipe(FName Id)
{
    for(const auto& V:Table()->GetArrayField(TEXT("recipes")))
        if(HearthwardData::Text(V->AsObject(),TEXT("id"))==Id.ToString()) return V->AsObject();
    return nullptr;
}
TMap<FName,int32> Counts(const TSharedPtr<FJsonObject>& Row,const TCHAR* Field)
{
    TMap<FName,int32> Result;
    if(Row) for(const auto& V:Row->GetObjectField(Field)->Values) Result.Add(FName(*V.Key),int32(V.Value->AsNumber()));
    return Result;
}
TMap<FName,int32> BuildCost(FName Kind,int32 Level)
{
    if(auto R=Facility(Kind,Level)) return Counts(R,TEXT("incremental_cost"));
    if(Level==1) for(const auto& V:Table()->GetArrayField(TEXT("other_buildings")))
        if(HearthwardData::Text(V->AsObject(),TEXT("id"))==Kind.ToString()) return Counts(V->AsObject(),TEXT("cost"));
    return {};
}
int32 RequiredTier(FName Kind,int32 Level)
{
    if(auto R=Facility(Kind,Level)) return HearthwardData::Number(R,TEXT("minimum_camp_tier"));
    if(Level==1) for(const auto& V:Table()->GetArrayField(TEXT("other_buildings")))
        if(HearthwardData::Text(V->AsObject(),TEXT("id"))==Kind.ToString()) return HearthwardData::Number(V->AsObject(),TEXT("minimum_camp_tier"));
    return MAX_int32;
}
double FoodPoints(FName Item)
{ return HearthwardData::Number(Table()->GetObjectField(TEXT("rations"))->GetObjectField(TEXT("food_points")),*Item.ToString()); }
TMap<FName,int32> Refund(const TMap<FName,int32>& Paid)
{
    TMap<FName,int32> Result;
    for(const auto& Entry:Paid) if(const int32 N=int64(Entry.Value)*4/5;N>0) Result.Add(Entry.Key,N);
    return Result;
}
}

double FHearthwardCampState::Radius() const { return HearthwardData::Number(HearthwardCamp::Tier(Tier),TEXT("radius_m"))*100; }
float FHearthwardCampState::Bonus(const TCHAR* Field) const { return HearthwardData::Number(HearthwardCamp::Tier(Tier),Field); }
FName FHearthwardCampState::CampAt(FVector Position) const
{
    for(const auto& C:Camps) if(FVector::Dist2D(Position,C.Position)<=Radius()) return C.Id;
    return NAME_None;
}
void FHearthwardCampState::AddCamp(FName Id,FVector Position)
{
    if(Id.IsNone() || Camps.ContainsByPredicate([&](const auto& C){return C.Id==Id;})) return;
    FHearthwardCampSite Site;Site.Id=Id;Site.Position=Position;Camps.Add(Site);
    int32 Priority=0;
    for(const FName Job:{FName(TEXT("forage")),FName(TEXT("wood")),FName(TEXT("stone")),FName(TEXT("ore"))})
    {
        FHearthwardCampRegion R;R.Id=FName(*(Id.ToString()+TEXT("_")+Job.ToString()));R.Camp=Id;R.Job=Job;
        R.Priority=Priority++;Regions.Add(R);
    }
}
bool FHearthwardCampState::Assign(FName Region,int32 Person)
{
    auto* Target=Regions.FindByPredicate([&](const auto& R){return R.Id==Region;});
    if(!Target || Person<0 || Person>31 || (Person<30 && Person>=Population())) return false;
    const bool Already=Person==30?Target->Player:Person==31?Target->Brother:Target->Workers.Contains(Person);
    if(!Already && Target->Workers.Num()+int(Target->Player)+int(Target->Brother)>=5) return false;
    for(auto& R:Regions)
    {
        if(Person==30) R.Player=false;
        else if(Person==31) R.Brother=false;
        else R.Workers.Remove(Person);
    }
    if(!Already)
    {
        if(Person==30) Target->Player=true;
        else if(Person==31) Target->Brother=true;
        else Target->Workers.Add(Person);
    }
    return true;
}
bool FHearthwardCampState::Rescue(FName Person)
{
    if(Person.IsNone() || Rescued.Num()>=10 || Rescued.Contains(Person)) return false;
    Rescued.Add(Person);return true;
}
FString FHearthwardCampState::UpgradeReason() const
{
    if(Tier>=8) return TEXT("营地已满阶");
    for(const auto& V:HearthwardCamp::Tier(Tier+1)->GetArrayField(TEXT("conditions")))
    {
        FString Name,Count;V->AsString().Split(TEXT(":"),&Name,&Count);const int32 N=FCString::Atoi(*Count);
        if(Name==TEXT("rescued_population")) { if(Rescued.Num()<N) return TEXT("需要实际救回一名族人"); }
        else if(Name==TEXT("public_food_deposited_points")) { if(DonatedPoints<N) return TEXT("累计存入口粮不足40点"); }
        else if(Name==TEXT("completed_distinct_production_regions"))
        { int32 Done=0;for(const auto& R:Regions) Done+=R.Completed>0;if(Done<N) return TEXT("需要两个不同生产区域各完成一批"); }
        else if(Name==TEXT("hometown_reclaimed")) { if(!Hometown) return TEXT("需要永久夺回故乡"); }
        else if(!Facilities.ContainsByPredicate([&](const auto& B){return B.Kind==FName(*Name) && B.Level>=N;}))
            return FString::Printf(TEXT("需要设施 %s 达到%d级"),*HearthwardData::Text(HearthwardData::Find(TEXT("buildings"),Name),TEXT("name")),N);
    }
    return {};
}
void FHearthwardCampState::Drain(double Minutes)
{
    const double Time=RationDrainMinutes+Minutes;
    const int64 Units=FMath::FloorToInt64(Time/30.0+1.e-9);
    RationHalfPoints=FMath::Max(int64(0),RationHalfPoints-Units);
    RationDrainMinutes=RationHalfPoints>0?FMath::Max(0.,Time-Units*30):0;
}
bool FHearthwardCampState::Donate(FName Item,int32 Count)
{
    const double Points=HearthwardCamp::FoodPoints(Item)*Count;
    if(Count<=0 || Points<=0 || Points>double(MAX_int64-RationHalfPoints)/2) return false;
    RationHalfPoints+=FMath::RoundToInt64(Points*2);DonatedPoints+=Points;return true;
}
bool FHearthwardCampState::Eat(float& Hunger)
{
    if(Hunger>=100 || Rations()<5) return false;
    RationHalfPoints-=10;Hunger=FMath::Min(100.f,Hunger+40);return true;
}
void FHearthwardCampState::Advance(double Minutes,bool Sleeping,const FExchange& Exchange)
{
    if(!FMath::IsFinite(Minutes) || Minutes<=0) return;
    const double End=Calendar+Minutes;
    TArray<int32> Order;for(int32 I=0;I<Regions.Num();++I) Order.Add(I);
    Order.StableSort([&](int32 A,int32 B){return Regions[A].Priority<Regions[B].Priority;});
    while(Calendar<End-1.e-8)
    {
        for(auto& S:Sources) if(S.Remaining==0 && S.Due>=0 && S.Due<=Calendar+1.e-8 && !S.Blocked)
        { S.Remaining=S.Capacity;S.Due=-1; }
        TArray<double> Rates;Rates.SetNumZeroed(Regions.Num());
        for(int32 Index:Order)
        {
            auto& R=Regions[Index];R.Status.Reset();
            if(!R.Enabled || !R.Safe) {R.Status=R.Safe?TEXT("已暂停"):TEXT("区域不安全");continue;}
            auto* Facility=Facilities.FindByPredicate([&](const auto& B){return B.Id==R.Facility;});
            const bool Processing=!R.Facility.IsValid()?false:true;
            if(Processing && (!Facility || Facility->Paused || Facility->Camp!=R.Camp)) {R.Status=TEXT("设施不可用");continue;}
            double Rate=R.Workers.Num()+(!Sleeping?(R.Player?R.PlayerEfficiency:0)+(R.Brother?R.BrotherEfficiency:0):0);
            if(Processing) Rate*=HearthwardData::Number(HearthwardCamp::Facility(Facility->Kind,Facility->Level),TEXT("labor_speed_percent"),100)/100;
            if(Rate<=0) {R.Status=TEXT("没有正在工作的族人");continue;}
            Rates[Index]=Rate;
            if(R.Batch.Active) continue;
            FHearthwardCampBatch Batch;Batch.Active=true;Batch.ToRations=R.ToRations;
            if(Processing)
            {
                const auto Recipe=HearthwardCamp::Recipe(R.Job);
                if(!Recipe || HearthwardData::Text(Recipe,TEXT("facility"))!=Facility->Kind.ToString()
                    || HearthwardData::Number(Recipe,TEXT("level"))>Facility->Level) {R.Status=TEXT("请选择已解锁配方");continue;}
                Batch.Inputs=HearthwardCamp::Counts(Recipe,TEXT("inputs"));Batch.Outputs=HearthwardCamp::Counts(Recipe,TEXT("outputs"));Batch.Required=360;
                if(!Exchange(Batch.Inputs,{})) {R.Status=TEXT("等待共享仓储补料");continue;}
            }
            else
            {
                const FName Item=R.Job==TEXT("forage")?FName(TEXT("wild_food")):R.Job;
                const int32 Yield=R.Job==TEXT("forage")?1:2;
                // Finish a partially harvested patch before switching to a newly refreshed full patch.
                FHearthwardCampSource* Source=nullptr;
                for(auto& S:Sources) if(S.Camp==R.Camp && S.Item==Item && !S.Blocked && S.Remaining>=Yield
                    && (!Source || S.Remaining<Source->Remaining)) Source=&S;
                if(!Source) {R.Status=TEXT("等待可采集来源／刷新");continue;}
                Source->Remaining-=Yield;
                if(Source->Remaining==0 && Source->RefreshMinutes>0) Source->Due=Calendar+Source->RefreshMinutes;
                Batch.Inputs.Add(Item,Yield);Batch.Outputs.Add(Item,Yield);Batch.Required=R.Job==TEXT("forage")?360:15;
            }
            R.Batch=MoveTemp(Batch);
        }
        double Step=End-Calendar;
        for(const auto& S:Sources) if(!S.Blocked && S.Remaining==0 && S.Due>Calendar+1.e-8) Step=FMath::Min(Step,S.Due-Calendar);
        for(int32 I:Order) if(Regions[I].Batch.Active && Rates[I]>0 && Regions[I].Batch.Work<Regions[I].Batch.Required-1.e-8)
            Step=FMath::Min(Step,(Regions[I].Batch.Required-Regions[I].Batch.Work)/Rates[I]);
        Drain(Step);Calendar+=Step;
        for(int32 I:Order)
        {
            auto& R=Regions[I];if(!R.Batch.Active || Rates[I]<=0) continue;
            R.Batch.Work=FMath::Min(R.Batch.Required,R.Batch.Work+Step*Rates[I]);
            if(R.Batch.Work<R.Batch.Required-1.e-7) continue;
            bool Food=R.Batch.ToRations;
            for(const auto& O:R.Batch.Outputs) if(HearthwardCamp::FoodPoints(O.Key)<=0) Food=false;
            if(!Food && !Exchange({},R.Batch.Outputs)) {R.Status=TEXT("仓储数量达到上限，产物待入库");continue;}
            if(Food) for(const auto& O:R.Batch.Outputs) Donate(O.Key,O.Value);
            R.Batch={};R.Completed++;R.Status=TEXT("生产中");
        }
    }
}

namespace
{
bool ValidCampCounts(const TMap<FName,int32>& Counts)
{
    for(const auto& P:Counts) if(P.Value<=0 || !HearthwardBasicItems().ContainsByPredicate([&](const auto& I){return I.Id==P.Key;})) return false;
    return true;
}
}
bool FHearthwardCampState::Validate() const
{
    if(Version!=1 || Tier<1 || Tier>8 || RationHalfPoints<0 || !FMath::IsFinite(RationDrainMinutes) || RationDrainMinutes<0 || RationDrainMinutes>=30
        || !FMath::IsFinite(DonatedPoints) || DonatedPoints<0 || !FMath::IsFinite(Calendar) || Calendar<0 || Rescued.Num()>10 || Camps.Num()>2) return false;
    TSet<FName> People,CampsSeen,RegionsSeen;TSet<FGuid> Buildings;TSet<int32> Workers;TSet<FString> Points;
    for(FName P:Rescued) {if(P.IsNone() || People.Contains(P))return false;People.Add(P);}
    for(const auto& C:Camps) {if(C.Id.IsNone() || CampsSeen.Contains(C.Id) || C.Position.ContainsNaN())return false;CampsSeen.Add(C.Id);}
    for(const auto& B:Facilities)
    {
        if(!B.Id.IsValid() || Buildings.Contains(B.Id) || (!B.Camp.IsNone() && !CampsSeen.Contains(B.Camp))
            || HearthwardCamp::BuildCost(B.Kind,B.Level).IsEmpty() || !ValidCampCounts(B.Paid)) return false;
        Buildings.Add(B.Id);
    }
    for(const auto& S:Sources)
    {
        if(S.Id.IsEmpty() || Points.Contains(S.Id) || !CampsSeen.Contains(S.Camp) || S.Position.ContainsNaN() || S.Capacity<=0 || S.Remaining<0 || S.Remaining>S.Capacity
            || !FMath::IsFinite(S.RefreshMinutes) || S.RefreshMinutes<0 || !FMath::IsFinite(S.Due) || S.Due< -1 || (S.Remaining>0 && S.Due!=-1)
            || !HearthwardBasicItems().ContainsByPredicate([&](const auto& I){return I.Id==S.Item;})) return false;
        Points.Add(S.Id);
    }
    for(const auto& R:Regions)
    {
        if(R.Id.IsNone() || RegionsSeen.Contains(R.Id) || !CampsSeen.Contains(R.Camp) || R.Completed<0 || R.Priority<0
            || R.Workers.Num()+int(R.Player)+int(R.Brother)>5 || (R.Facility.IsValid() && !Buildings.Contains(R.Facility))) return false;
        RegionsSeen.Add(R.Id);
        TArray<int32> Assigned=R.Workers;if(R.Player) Assigned.Add(30);if(R.Brother) Assigned.Add(31);
        for(int32 P:Assigned) {if(P<0 || P>31 || (P<30 && P>=Population()) || Workers.Contains(P))return false;Workers.Add(P);}
        const auto& B=R.Batch;
        if(!FMath::IsFinite(B.Work) || !FMath::IsFinite(B.Required) || B.Work<0 || B.Required<0 || B.Work>B.Required
            || !ValidCampCounts(B.Inputs) || !ValidCampCounts(B.Outputs) || (B.Active && (B.Required<=0 || B.Inputs.IsEmpty() || B.Outputs.IsEmpty()))
            || (!B.Active && (B.Work!=0 || B.Required!=0 || !B.Inputs.IsEmpty() || !B.Outputs.IsEmpty()))) return false;
        if(B.Active)
        {
            TMap<FName,int32> Inputs,Outputs;double Work=0;
            if(R.Facility.IsValid())
            {
                const auto Recipe=HearthwardCamp::Recipe(R.Job);if(!Recipe)return false;
                const auto* F=Facilities.FindByPredicate([&](const auto& Entry){return Entry.Id==R.Facility;});
                if(HearthwardData::Text(Recipe,TEXT("facility"))!=F->Kind.ToString() || HearthwardData::Number(Recipe,TEXT("level"))>F->Level)return false;
                Inputs=HearthwardCamp::Counts(Recipe,TEXT("inputs"));Outputs=HearthwardCamp::Counts(Recipe,TEXT("outputs"));Work=360;
            }
            else
            {
                if(R.Job!=TEXT("forage") && R.Job!=TEXT("wood") && R.Job!=TEXT("stone") && R.Job!=TEXT("ore"))return false;
                Inputs.Add(R.Job==TEXT("forage")?FName(TEXT("wild_food")):R.Job,R.Job==TEXT("forage")?1:2);Outputs=Inputs;Work=R.Job==TEXT("forage")?360:15;
            }
            if(B.Required!=Work || !Inputs.OrderIndependentCompareEqual(B.Inputs) || !Outputs.OrderIndependentCompareEqual(B.Outputs))return false;
        }
    }
    return true;
}
bool FHearthwardCampState::ValidateBuildings(const FString& Gameplay) const
{
    if(Gameplay.IsEmpty())return Facilities.IsEmpty();
    TSharedPtr<FJsonObject> Root;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Gameplay),Root) || !Root || HearthwardData::Number(Root,TEXT("campTier"))!=Tier)return false;
    const TArray<TSharedPtr<FJsonValue>>* Buildings;
    if(!Root->TryGetArrayField(TEXT("buildings"),Buildings))return Facilities.IsEmpty();
    if(Buildings->Num()!=Facilities.Num())return false;
    for(const auto& V:*Buildings)
    {
        if(!V || V->Type!=EJson::Object)return false;
        FGuid Id;const auto Row=V->AsObject();if(!FGuid::Parse(HearthwardData::Text(Row,TEXT("id")),Id))return false;
        const auto* B=Facilities.FindByPredicate([&](const auto& F){return F.Id==Id;});
        if(!B || B->Kind.ToString()!=HearthwardData::Text(Row,TEXT("recipe")) || B->Paused)return false;
    }
    return true;
}
FString FHearthwardCampState::Snapshot() const
{ FString Json;FJsonObjectConverter::UStructToJsonObjectString(*this,Json);return Json; }
bool FHearthwardCampState::Parse(const FString& Json,FHearthwardCampState& Out)
{
    TSharedPtr<FJsonObject> Object;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Object) || !Object) return false;
    for(const TCHAR* Key:{TEXT("version"),TEXT("tier"),TEXT("rationHalfPoints"),TEXT("rationDrainMinutes"),TEXT("donatedPoints"),TEXT("calendar"),TEXT("hometown"),TEXT("rescued"),TEXT("camps"),TEXT("facilities"),TEXT("regions"),TEXT("sources")})
        if(!Object->HasField(Key)) return false;
    FHearthwardCampState Candidate;
    if(!FJsonObjectConverter::JsonObjectToUStruct(Object.ToSharedRef(),&Candidate) || !Candidate.Validate()) return false;
    Out=MoveTemp(Candidate);return true;
}
