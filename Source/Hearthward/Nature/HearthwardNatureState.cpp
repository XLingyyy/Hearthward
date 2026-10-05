#include "HearthwardNatureState.h"
#include "../Gameplay/HearthwardGameData.h"
#include "JsonObjectConverter.h"
#include "Math/RandomStream.h"
using namespace HearthwardData;

const TArray<TSharedPtr<FJsonValue>>& HearthwardNature::Rows(const TCHAR* Table)
{
    static const TArray<TSharedPtr<FJsonValue>> Empty;
    const TSharedPtr<FJsonObject>* Root=nullptr;const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
    return Catalog()->TryGetObjectField(TEXT("nature"),Root) && (*Root)->TryGetArrayField(Table,Values)?*Values:Empty;
}
TSharedPtr<FJsonObject> HearthwardNature::Definition(const TCHAR* Table,FName Id)
{
    for(const auto& V:Rows(Table)){auto R=V->AsObject();if(Text(R,TEXT("id"))==Id.ToString())return R;}return nullptr;
}
int32 HearthwardNature::Capacity(int32 Level){return Level==1?6:Level==2?10:Level==3?14:0;}
FName HearthwardNature::Reward(int32 Seed,FName Point,int32 Success)
{
    FRandomStream R(HashCombineFast(uint32(Seed),HashCombineFast(FCrc::StrCrc32(*Point.ToString()),uint32(Success))));
    if(R.FRand()>=.02f)return NAME_None;
    int32 Roll=R.RandRange(1,100);
    for(const auto& V:Rows(TEXT("rewards"))) {const auto D=V->AsObject();Roll-=Number(D,TEXT("weight"));if(Roll<=0)return FName(Text(D,TEXT("id")));}
    return NAME_None;
}
void HearthwardNature::FFishing::Advance(double Seconds,bool Held)
{
    // Integrate at event edges and small fixed steps, so low frame rates cannot skip the safe band.
    while(Seconds>1.e-8 && !Done && !Failed)
    {
        const double Step=FMath::Min(Seconds,.01);Seconds-=Step;Elapsed+=Step;
        if(Elapsed>=1)Cast=true;
        if(Elapsed>=1+Bite)
        {
            const double Force=int32((Elapsed-1-Bite)/1.5)%2==0?.05:-.05;
            Tension+=(Held?.30:-.25)*Step+Force*Step;
            if(Tension<=0 || Tension>=1){Failed=true;break;}
            if(Tension>=.15 && Tension<=.85)Progress+=Step;
            if(Progress+1.e-6>=Required){Done=true;break;}
        }
        if(Elapsed>=12)Failed=true;
    }
}
int32 FHearthwardNatureState::Occupants(FGuid Pen,bool Reserved) const
{int32 N=0;for(const auto& A:Animals)if(A.Health>0 && (A.Pen==Pen || (Reserved && A.ReservedPen==Pen)))++N;return N;}
bool FHearthwardNatureState::Ready(const FHearthwardCrop& C) const
{return Calendar+1.e-6>=C.Planted+Number(HearthwardNature::Definition(TEXT("crops"),C.Definition),TEXT("days"))*1440;}
int32 FHearthwardNatureState::Yield(const FHearthwardCrop& C) const
{return FMath::FloorToInt(Number(HearthwardNature::Definition(TEXT("crops"),C.Definition),TEXT("base_yield"))*(1+.25*int(C.Watered)+.25*int(C.Fertilized)));}
void FHearthwardNatureState::Advance(double Minutes)
{
    if(!FMath::IsFinite(Minutes) || Minutes<=0)return;
    double Left=Minutes;
    while(Left>1.e-7)
    {
        // Charge complete feeding windows before allocating this interval's biological time.
        Animals.Sort([](const auto& A,const auto& B){if(A.Juvenile!=B.Juvenile)return !A.Juvenile;return A.Id.ToString()<B.Id.ToString();});
        for(auto& A:Animals)if(A.Domestic && A.Health>0 && A.Pen.IsValid() && A.FedRemaining<=1.e-7)
        {
            auto* P=Pens.FindByPredicate([&](const auto& X){return X.Id==A.Pen;});
            const auto D=HearthwardNature::Definition(TEXT("domestic"),A.Definition);
            const int32 Cost=Number(D,A.Juvenile?TEXT("young_feed_daily"):TEXT("adult_feed_daily"));
            if(P && P->Feed>=Cost){P->Feed-=Cost;A.FedRemaining=1440;}
        }
        double Step=Left;
        for(const auto& A:Animals)if(A.Health>0 && A.Domestic && A.Pen.IsValid() && A.FedRemaining>1.e-7)
        {Step=FMath::Min(Step,A.FedRemaining);if(A.Juvenile)Step=FMath::Min(Step,FMath::Max(1.e-7,2880-A.Growth));}
        struct FPair{FGuid Pen,A,B;FString Key;};TArray<FPair> Pairs;
        for(auto& P:Pens)
        {
            TArray<const FHearthwardAnimal*> Adults;
            for(const auto& A:Animals)if(A.Health>0 && A.Pen==P.Id && !A.Juvenile)Adults.Add(&A);
            TSet<FString> Keys;
            if(Occupants(P.Id)<HearthwardNature::Capacity(P.Level))for(int32 I=0;I+1<Adults.Num();I+=2)
            {
                const auto* A=Adults[I];const auto* B=Adults[I+1];const FString Key=A->Id.ToString()+TEXT(":")+B->Id.ToString();Keys.Add(Key);
                double& Progress=P.Pairs.FindOrAdd(Key);
                if(A->FedRemaining>1.e-7 && B->FedRemaining>1.e-7){Pairs.Add({P.Id,A->Id,B->Id,Key});Step=FMath::Min(Step,FMath::Max(1.e-7,2880-Progress));}
            }
            for(auto It=P.Pairs.CreateIterator();It;++It)if(!Keys.Contains(It.Key()))It.RemoveCurrent();
        }
        for(auto& A:Animals)if(A.Health>0 && A.Domestic && A.Pen.IsValid() && A.FedRemaining>1.e-7)
        {
            const bool Producing=!A.Juvenile && !Text(HearthwardNature::Definition(TEXT("domestic"),A.Definition),TEXT("product")).IsEmpty();
            if(A.Juvenile){A.Growth=FMath::Min(2880.,A.Growth+Step);if(A.Growth>=2880-1.e-6)A.Juvenile=false;}
            if(Producing)
            {
                A.ProductMinutes+=Step;
                if(A.ProductMinutes>=1440-1.e-6)
                {
                    A.ProductMinutes=0;
                    if(auto* Pen=Pens.FindByPredicate([&](const auto& P){return P.Id==A.Pen;}))
                        Pen->Products=FMath::Min(HearthwardNature::ProductCapacity,Pen->Products+1);
                }
            }
            A.FedRemaining=FMath::Max(0.,A.FedRemaining-Step);
        }
        for(const auto& Pair:Pairs)
        {
            auto& P=*Pens.FindByPredicate([&](const auto& X){return X.Id==Pair.Pen;});double& Progress=P.Pairs.FindChecked(Pair.Key);Progress+=Step;
            if(Progress>=2880-1.e-6)
            {
                Progress=0;
                if(Occupants(P.Id)<HearthwardNature::Capacity(P.Level))
                {
                    FHearthwardAnimal A;A.Id=FGuid::NewGuid();A.Domestic=true;A.Captured=true;A.Juvenile=true;A.Pen=P.Id;A.Definition=P.Definition;A.Position=P.Position;
                    A.Health=Number(HearthwardNature::Definition(TEXT("domestic"),A.Definition),TEXT("health"));Animals.Add(A);
                }
            }
        }
        Left-=Step;Calendar+=Step;
    }
    for(auto& P:Points)if(P.Kind==TEXT("fish") && P.Remaining==0 && P.Due>=0 && P.Due<=Calendar){P.Remaining=24;P.Due=-1;}
}
FString FHearthwardNatureState::Snapshot() const
{FString Json;FJsonObjectConverter::UStructToJsonObjectString(*this,Json);return Json;}
bool FHearthwardNatureState::Parse(const FString& Json,FHearthwardNatureState& Out)
{
    if(Json.IsEmpty()){Out={};return true;}
    TSharedPtr<FJsonObject> Root;
    return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) && Root && Root->HasTypedField<EJson::Number>(TEXT("version"))
        && FJsonObjectConverter::JsonObjectToUStruct(Root.ToSharedRef(),&Out) && Out.Valid();
}
bool FHearthwardNatureState::Valid() const
{
    if(Version!=1 || !FMath::IsFinite(Calendar) || Calendar<0)return false;
    TSet<FGuid> Ids;TSet<FName> Keys;TSet<FGuid> ItemIds;TSet<FName> Unique;
    auto Id=[&](FGuid V,FVector P){if(!V.IsValid() || Ids.Contains(V) || P.ContainsNaN())return false;Ids.Add(V);return true;};
    for(const auto& P:Points)
    {
        if(!Id(P.Id,P.Position) || P.Key.IsNone() || Keys.Contains(P.Key) || P.Successes<0 || P.Remaining<0 || !FMath::IsFinite(P.Due) || P.Due< -1)return false;Keys.Add(P.Key);
        if(P.Kind!=TEXT("resource") && P.Kind!=TEXT("fish") && P.Kind!=TEXT("treasure"))return false;
        if(P.Kind==TEXT("resource") && !HearthwardNature::Definition(TEXT("resources"),P.Definition))return false;
        if(P.Kind==TEXT("fish") && (P.Remaining>24 || (P.Remaining>0 && P.Due!=-1)))return false;
        if(!FHearthwardInventoryState::Validate(P.Pending,true))return false;
        for(const auto& I:P.Pending.Instances){if(ItemIds.Contains(I.Id) || (!I.UniqueClaim.IsNone() && Unique.Contains(I.UniqueClaim)))return false;ItemIds.Add(I.Id);if(!I.UniqueClaim.IsNone())Unique.Add(I.UniqueClaim);}
    }
    for(const auto& C:Crops)if(!Id(C.Id,C.Position) || !HearthwardNature::Definition(TEXT("crops"),C.Definition) || !FMath::IsFinite(C.Planted) || C.Planted<0 || C.Planted>Calendar)return false;
    for(const auto& P:Pens)
    {
        const auto D=HearthwardNature::Definition(TEXT("domestic"),P.Definition);
        if(!Id(P.Id,P.Position) || !D || !HearthwardNature::Capacity(P.Level) || P.Feed<0 || P.Products<0
            || P.Products>HearthwardNature::ProductCapacity || (P.Products>0 && Text(D,TEXT("product")).IsEmpty())
            || Occupants(P.Id)>HearthwardNature::Capacity(P.Level))return false;
        for(const auto& Pair:P.Pairs)if(!FMath::IsFinite(Pair.Value) || Pair.Value<0 || Pair.Value>=2880+1.e-6)return false;
        for(const auto& Cost:P.Paid)if(Cost.Value<=0 || !Find(TEXT("items"),Cost.Key.ToString()))return false;
    }
    for(const auto& A:Animals)
    {
        const auto D=HearthwardNature::Definition(A.Domestic?TEXT("domestic"):TEXT("wildlife"),A.Definition);
        if(!Id(A.Id,A.Position) || !D || A.Destination.ContainsNaN() || !FMath::IsFinite(A.Health) || A.Health<0 || A.Health>Number(D,TEXT("health")) || !FMath::IsFinite(A.Growth) || A.Growth<0 || A.Growth>2880 || !FMath::IsFinite(A.FedRemaining) || A.FedRemaining<0 || A.FedRemaining>1440 || !FMath::IsFinite(A.ProductMinutes) || A.ProductMinutes<0 || A.ProductMinutes>=1440 || !FMath::IsFinite(A.AlertRemaining) || A.AlertRemaining<0)return false;
        if(!A.Threat.IsNone() && A.Threat!=TEXT("player") && A.Threat!=TEXT("brother"))return false;
        for(FGuid Home:{A.Pen,A.ReservedPen})if(Home.IsValid() && !Pens.ContainsByPredicate([&](const auto& P){return P.Id==Home && P.Definition==A.Definition;}))return false;
        if(A.Pen.IsValid() && A.ReservedPen.IsValid())return false;
        if(A.FollowingBrother && (!A.Domestic || !A.Captured || !A.Following || !A.ReservedPen.IsValid()))return false;
        for(const auto& Loot:A.Loot)if(Loot.Value<=0 || !Find(TEXT("items"),Loot.Key.ToString()))return false;
    }
    TSet<FName> SlotsSeen;
    for(const auto& S:Slots)
    {if(S.Id.IsNone() || SlotsSeen.Contains(S.Id) || S.Position.ContainsNaN() || S.Generation<1 || !FMath::IsFinite(S.Due) || S.Due< -1 || !Animals.ContainsByPredicate([&](const auto& A){return A.Id==S.Current && A.Slot==S.Id && A.Definition==S.Definition;}))return false;SlotsSeen.Add(S.Id);}
    for(FName R:Rewards)if(!HearthwardNature::Definition(TEXT("rewards"),R))return false;
    for(FName M:Maps)if(!Rewards.Contains(M) || !M.ToString().StartsWith(TEXT("treasure_map_")))return false;
    for(FName M:Opened)if(!Maps.Contains(M))return false;
    return true;
}
