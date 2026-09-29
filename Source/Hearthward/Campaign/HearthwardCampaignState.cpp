#include "HearthwardCampaignState.h"
#include "../Gameplay/HearthwardGameData.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
using namespace HearthwardData;
const TArray<TSharedPtr<FJsonValue>>& HearthwardCampaign::Rows(const FString& Name)
{ return Catalog()->GetObjectField(TEXT("campaign"))->GetArrayField(Name); }
TSharedPtr<FJsonObject> HearthwardCampaign::Find(const FString& Table,FName Id)
{
    for(const auto& V:Rows(Table))if(Text(V->AsObject(),TEXT("id"))==Id.ToString())return V->AsObject();
    return nullptr;
}
FVector HearthwardCampaign::XY(const TSharedPtr<FJsonObject>& R,const FString& Key)
{ const auto& A=R->GetArrayField(Key);return FVector(A[0]->AsNumber()*100,A[1]->AsNumber()*100,0); }
double HearthwardCampaign::Health(FName Kind,int32 Stage)
{
    const auto& A=Catalog()->GetObjectField(TEXT("progression"))->GetArrayField(TEXT("enemyCalibration"));
    const auto R=A[FMath::Clamp(Stage-1,0,A.Num()-1)]->AsObject();
    return Number(R,TEXT("H"))*(Kind==TEXT("archer")?Number(R,TEXT("archer_hp_multiplier")):Kind==TEXT("heavy")?Number(R,TEXT("heavy_hp_multiplier")):1);
}
namespace
{
FHearthwardCampaignEnemy Enemy(const TSharedPtr<FJsonObject>& R,FName Group,FName Zone=NAME_None)
{
    FHearthwardCampaignEnemy E;E.Id=FName(*Text(R,TEXT("id")));E.Group=Group;E.Zone=Zone.IsNone()?FName(*Text(R,TEXT("zone"))):Zone;
    E.Kind=FName(*Text(R,TEXT("kind")));E.Stage=Number(R,TEXT("stage"),1);E.Home=HearthwardCampaign::XY(R,TEXT("spawn"));
    E.Combat.Id=E.Id;E.Combat.Region=E.Zone;E.Combat.Generation=1;E.Combat.Health=HearthwardCampaign::Health(E.Kind,E.Stage);E.Combat.Position=E.Home;
    return E;
}
}
int32 FHearthwardCampaignState::Total() const
{ return Enemies.FilterByPredicate([](const auto& E){return E.Group==TEXT("base") || E.Group==TEXT("reinforcement");}).Num(); }
int32 FHearthwardCampaignState::Cleared() const
{ return Enemies.FilterByPredicate([](const auto& E){return (E.Group==TEXT("base") || E.Group==TEXT("reinforcement")) && E.Combat.Health<=0;}).Num(); }
bool FHearthwardCampaignState::ZoneClear(FName Zone,bool BaseOnly) const
{
    int32 Count=0;
    for(const auto& E:Enemies)if(E.Zone==Zone && (E.Group==TEXT("base") || (!BaseOnly && E.Group==TEXT("reinforcement"))))
    {++Count;if(E.Combat.Health>0)return false;}
    return Count>0;
}
bool FHearthwardCampaignState::ReadyForVictory() const
{
    if(LegacyHometown || Total()<80 || Cleared()!=Total() || Flags.Num()!=4 || Reinforcements.Num()!=2)return false;
    for(const auto& R:Reinforcements)if(R.Value==TEXT("pending"))return false;
    return true;
}
void FHearthwardCampaignState::Initialize(bool Old,bool Home)
{
    *this={};Legacy=Old;LegacyHometown=Home;Phase=Old?FName(TEXT("occupied")):FName(TEXT("prologue"));Victory=Home;
    if(Old)Facts.Add(TEXT("prologue_skipped"));
    if(Home)Phase=TEXT("reclaimed");
    for(const auto& Z:HearthwardCampaign::Rows(TEXT("reinforcements")))Reinforcements.Add(FName(*Text(Z->AsObject(),TEXT("zone"))),Home?TEXT("cancelled"):TEXT("pending"));
    if(!Home)for(const auto& R:HearthwardCampaign::Rows(TEXT("enemies")))Enemies.Add(Enemy(R->AsObject(),TEXT("base")));
    if(!Old)for(const auto& R:HearthwardCampaign::Rows(TEXT("prologue_enemies")))Enemies.Add(Enemy(R->AsObject(),TEXT("prologue"),TEXT("prologue")));
    for(const auto& R:HearthwardCampaign::Rows(TEXT("field_enemies")))Enemies.Add(Enemy(R->AsObject(),TEXT("field"),TEXT("field_slice")));
    for(const auto& Q:HearthwardCampaign::Rows(TEXT("quests")))
    {
        const auto R=Q->AsObject();
        for(const auto& P:R->GetArrayField(TEXT("rescued_people")))
        {
            FHearthwardCampaignPerson Person;Person.Id=FName(*P->AsString());Person.Location=FName(*Text(R,TEXT("location")));
            Person.Position=HearthwardCampaign::XY(HearthwardCampaign::Find(TEXT("people"),Person.Id),TEXT("spawn"));
            People.Add(Person);
        }
    }
}
void FHearthwardCampaignState::ResolveUntriggered()
{ for(auto& R:Reinforcements)if(R.Value==TEXT("pending") && ZoneClear(R.Key,true))R.Value=TEXT("cancelled"); }
bool FHearthwardCampaignState::RegisterReinforcement(FName Zone)
{
    ResolveUntriggered();auto* Status=Reinforcements.Find(Zone);
    if(!Status || *Status!=TEXT("pending") || Victory || Phase!=TEXT("occupied"))return false;
    for(const auto& V:HearthwardCampaign::Rows(TEXT("reinforcements")))if(Text(V->AsObject(),TEXT("zone"))==Zone.ToString())
    {
        for(const auto& R:V->AsObject()->GetArrayField(TEXT("enemies")))Enemies.Add(Enemy(R->AsObject(),TEXT("reinforcement"),Zone));
        *Status=TEXT("spawned");return true;
    }
    return false;
}
bool FHearthwardCampaignState::Validate() const
{
    if(Phase.IsNone())return Enemies.IsEmpty() && People.IsEmpty() && Flags.IsEmpty() && Facts.IsEmpty() && Reinforcements.IsEmpty();
    if(Version!=1 || (Phase!=TEXT("prologue") && Phase!=TEXT("occupied") && Phase!=TEXT("reclaimed")) || People.Num()!=10 || Reinforcements.Num()!=2)return false;
    if((Phase==TEXT("reclaimed"))!=Victory || (LegacyHometown && (!Legacy || !Victory)))return false;
    TSet<FName> Ids;int32 Base=0;
    for(const auto& E:Enemies)
    {
        if(E.Id.IsNone() || Ids.Contains(E.Id) || E.Combat.Id!=E.Id || E.Home.ContainsNaN() || E.Combat.Position.ContainsNaN() || E.Combat.Rotation.ContainsNaN()
            || !FMath::IsFinite(E.Combat.Health) || E.Combat.Health<0 || E.Combat.Health>HearthwardCampaign::Health(E.Kind,E.Stage) || E.Combat.Generation<1
            || !FMath::IsFinite(E.Combat.ReportRemaining) || E.Combat.ReportRemaining<0 || E.Combat.ReportRemaining>3
            || !FMath::IsFinite(E.RefreshDue) || E.RefreshDue < -1 || E.PatrolPoint<0 || E.PatrolPoint>3)return false;
        TSharedPtr<FJsonObject> Definition;
        if(E.Group==TEXT("base")){Definition=HearthwardCampaign::Find(TEXT("enemies"),E.Id);++Base;}
        else if(E.Group==TEXT("field"))Definition=HearthwardCampaign::Find(TEXT("field_enemies"),E.Id);
        else if(E.Group==TEXT("prologue"))Definition=HearthwardCampaign::Find(TEXT("prologue_enemies"),E.Id);
        else if(E.Group==TEXT("reinforcement"))
        {
            for(const auto& V:HearthwardCampaign::Rows(TEXT("reinforcements")))if(Text(V->AsObject(),TEXT("zone"))==E.Zone.ToString())
                for(const auto& R:V->AsObject()->GetArrayField(TEXT("enemies")))if(Text(R->AsObject(),TEXT("id"))==E.Id.ToString())Definition=R->AsObject();
        }
        if(!Definition || Text(Definition,TEXT("kind"))!=E.Kind.ToString() || Number(Definition,TEXT("stage"),1)!=E.Stage)return false;
        if(E.Group==TEXT("base") && Text(Definition,TEXT("zone"))!=E.Zone.ToString())return false;
        if(E.Group!=TEXT("field") && (E.Combat.Generation!=1 || E.RefreshDue!=-1))return false;
        Ids.Add(E.Id);
        for(const auto& Detection:E.Combat.Detection)if(!FMath::IsFinite(Detection.Value) || Detection.Value<0 || Detection.Value>1)return false;
    }
    if(Base!=(LegacyHometown?0:80))return false;
    int32 Extra=0;
    for(const auto& R:Reinforcements)
    {
        if(R.Key!=TEXT("workshops") && R.Key!=TEXT("assembly"))return false;
        if(R.Value!=TEXT("pending") && R.Value!=TEXT("spawned") && R.Value!=TEXT("cancelled"))return false;
        int32 Count=0;for(const auto& E:Enemies)if(E.Group==TEXT("reinforcement") && E.Zone==R.Key){++Count;if(!E.Id.ToString().StartsWith(TEXT("reinforce_")+R.Key.ToString()+TEXT("_")))return false;}
        if(Count!=(R.Value==TEXT("spawned")?4:0))return false;Extra+=Count;
    }
    if(Total()!=Base+Extra)return false;
    Ids.Reset();
    for(const auto& P:People)
    {
        if(!P.Id.ToString().StartsWith(TEXT("rescued_")) || Ids.Contains(P.Id) || P.Position.ContainsNaN() || !HearthwardCampaign::Find(TEXT("locations"),P.Location)
            || (P.Stage!=TEXT("uncontacted") && P.Stage!=TEXT("waiting") && P.Stage!=TEXT("following") && P.Stage!=TEXT("arrived"))
            || (P.Escort!=TEXT("player") && P.Escort!=TEXT("brother")))return false;
        Ids.Add(P.Id);
    }
    for(int32 I=1;I<=10;++I)if(!Ids.Contains(FName(*FString::Printf(TEXT("rescued_%02d"),I))))return false;
    for(FName F:Flags)if(!HearthwardCampaign::Find(TEXT("zones"),F) || !ZoneClear(F))return false;
    for(const auto& P:Positions)if(P.Key.IsNone() || P.Value.ContainsNaN())return false;
    if(Victory && !LegacyHometown && !ReadyForVictory())return false;
    return true;
}
FString FHearthwardCampaignState::Snapshot() const
{ FString J;FJsonObjectConverter::UStructToJsonObjectString(*this,J);return J; }
bool FHearthwardCampaignState::Parse(const FString& Json,FHearthwardCampaignState& Out)
{
    Out={};if(Json.IsEmpty())return true;
    TSharedPtr<FJsonObject> R;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),R) || !R)return false;
    for(const TCHAR* Key:{TEXT("version"),TEXT("phase"),TEXT("enemies"),TEXT("people"),TEXT("reinforcements"),TEXT("flags"),TEXT("facts"),TEXT("positions"),TEXT("victory"),TEXT("legacy"),TEXT("legacyHometown")})if(!R->HasField(Key))return false;
    return FJsonObjectConverter::JsonObjectToUStruct(R.ToSharedRef(),&Out) && Out.Validate();
}
