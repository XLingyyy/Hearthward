#include "HearthwardNPCMemory.h"
#include "../Inventory/HearthwardInventoryState.h"

namespace
{
bool ValidKind(FName Kind)
{ return Kind == TEXT("claim") || Kind == TEXT("preference") || Kind == TEXT("agreement") || Kind == TEXT("collection_ban") || Kind == TEXT("typed_constraint"); }

bool ValidDirective(FName Item)
{ return Item==TEXT("hold") || Item==TEXT("follow") || Item==TEXT("assist"); }
int32 Relevance(const FString& Query, const FString& Text)
{
    TSet<FString> Terms;
    for (int32 I=0; I+1<Query.Len(); ++I)
        if (!FChar::IsWhitespace(Query[I]) && !FChar::IsPunct(Query[I]) && !FChar::IsPunct(Query[I+1])) Terms.Add(Query.Mid(I,2).ToLower());
    int32 Score=0;
    for (const auto& Term:Terms) if (Text.Contains(Term,ESearchCase::IgnoreCase)) ++Score;
    return Score;
}
}
bool FHearthwardNPCMemory::Put(FGuid Id,FName Kind,const FString& Text,double Now,FName BlockedItem)
{
    if(Kind==TEXT("typed_constraint")) return false; // Only the confirmed rule-card path can create this kind.
    Records.RemoveAll([](const auto& R){return R.Revoked;});
    const FString Clean=Text.TrimStartAndEnd();
    if (!ValidKind(Kind) || Clean.IsEmpty() || Clean.Len()>MaxText || !FMath::IsFinite(Now) || Now<0) return false;
    if (Kind==TEXT("collection_ban") && !HearthwardBasicItems().ContainsByPredicate([&](const auto& I){return I.Id==BlockedItem;})) return false;
    auto* Existing=Records.FindByPredicate([&](const auto& R){return R.Id==Id && !R.Revoked;});
    if (Id.IsValid() && !Existing) return false;
    if (!Id.IsValid() && Records.Num()>=MaxRecords) return false;
    if (Kind==TEXT("agreement"))
    {
        int32 Count=0;
        for (const auto& R:Records) if (!R.Revoked && R.Kind==Kind && R.Id!=Id) ++Count;
        if (Count>=MaxAgreements) return false;
    }
    if (!Existing) { Existing=&Records.AddDefaulted_GetRef(); Existing->Id=FGuid::NewGuid(); }
    Existing->Kind=Kind; Existing->Text=Clean; Existing->RecordedAt=Now;
    Existing->Campaign=Campaign; Existing->Revision=++Revision;
    Existing->BlockedItem=Kind==TEXT("collection_ban")?BlockedItem:NAME_None;
    // Editing a record invalidates any pending interpretation that used its old text.
    Clarification.Reset(); WorkingGoal={};
    return true;
}
bool FHearthwardNPCMemory::Revoke(FGuid Id)
{
    auto* R=Records.FindByPredicate([&](const auto& Entry){return Entry.Id==Id && !Entry.Revoked;});
    if (!R) return false;
    Records.RemoveAll([&](const auto& Entry){return Entry.Id==Id;});
    ++Revision; Clarification.Reset(); WorkingGoal={}; return true;
}
bool FHearthwardNPCMemory::AddClarification(const FString& Player,const FString& Question)
{
    int32 Characters=Player.Len()+Question.Len();
    for (const auto& T:Clarification) Characters+=T.Player.Len()+T.Question.Len();
    if (Clarification.Num()>=4 || Characters>MaxClarificationCharacters) return false;
    Clarification.Add({Player,Question}); return true;
}
bool FHearthwardNPCMemory::BlocksCollection(FName Item) const
{
    return ApplicableRules(TEXT("collect")).Contains(TEXT("ban:")+Item.ToString());
}
TArray<FHearthwardPlayerMemory> FHearthwardNPCMemory::Retrieve(const FString& Query,bool IncludeAgreements) const
{
    TArray<FHearthwardPlayerMemory> Out, Candidates;
    for (const auto& R:Records)
    {
        if (R.Revoked) continue;
        if (R.Kind==TEXT("collection_ban") || R.Kind==TEXT("typed_constraint")) continue;
        if (IncludeAgreements && R.Kind==TEXT("agreement")) Out.Add(R);
        else if (Relevance(HearthwardAgent::Normalize(Query),HearthwardAgent::Normalize(R.Text))>0) Candidates.Add(R);
    }
    Candidates.StableSort([&](const auto& A,const auto& B)
    {
        const int32 AS=Relevance(HearthwardAgent::Normalize(Query),HearthwardAgent::Normalize(A.Text)), BS=Relevance(HearthwardAgent::Normalize(Query),HearthwardAgent::Normalize(B.Text));
        return AS!=BS ? AS>BS : A.RecordedAt>B.RecordedAt;
    });
    for (int32 I=0; I<FMath::Min(3,Candidates.Num()); ++I) Out.Add(Candidates[I]);
    return Out;
}
bool FHearthwardNPCMemory::IsValid(double Now) const
{
    if(Revision<1 || Events.Num()>HearthwardAgent::Policy(TEXT("max_events"))) return false;
    if (Records.Num()>MaxRecords || Clarification.Num()>4 || !FMath::IsFinite(CampObservedAt)
        || CampObservedAt<0 || CampObservedAt>Now) return false;
    TSet<FGuid> Ids; int32 Agreements=0, Characters=0;
    for (const auto& R:Records)
    {
        if (!R.Id.IsValid() || Ids.Contains(R.Id) || !ValidKind(R.Kind) || R.Text.TrimStartAndEnd().IsEmpty()
            || R.Text.Len()>MaxText || !FMath::IsFinite(R.RecordedAt) || R.RecordedAt<0 || R.RecordedAt>Now) return false;
        Ids.Add(R.Id);
        if(R.Revision<1 || R.Revision>Revision || R.Campaign!=Campaign || (R.Kind==TEXT("typed_constraint") && !HearthwardAgent::ValidLimit(R.Constraint))) return false;
        if (R.Kind==TEXT("collection_ban"))
        { if (!HearthwardBasicItems().ContainsByPredicate([&](const auto& I){return I.Id==R.BlockedItem;})) return false; }
        else if (!R.BlockedItem.IsNone()) return false;
        if (!R.Revoked && R.Kind==TEXT("agreement")) ++Agreements;
    }
    for (const auto& T:Clarification)
    {
        if (T.Player.IsEmpty() || T.Question.IsEmpty()) return false;
        Characters+=T.Player.Len()+T.Question.Len();
    }
    if (Agreements>MaxAgreements || Characters>MaxClarificationCharacters) return false;
    if (!HasCampObservation && (!CampInventory.IsEmpty() || CampObservedAt!=0)) return false;
    for (const auto& Entry:CampInventory)
        if (Entry.Value<0 || !HearthwardBasicItems().ContainsByPredicate([&](const auto& I){return I.Id==Entry.Key;})) return false;
    if(!HearthwardBeliefs::Validate(Beliefs,Revision,Campaign,Now)) return false;
    TSet<FGuid> EventIds;
    for(const auto& E:Events)
    {
        if(!E.Id.IsValid() || !E.Command.IsValid() || E.Campaign!=Campaign || E.At<0 || E.At>Now || !FMath::IsFinite(E.At) || E.Count<0 || E.Reason.Len()>200 || EventIds.Contains(E.Id)) return false;
        if(!TArray<FName>{TEXT("acquired"),TEXT("delivered"),TEXT("craft"),TEXT("repair"),TEXT("completed"),TEXT("materials_taken"),TEXT("cancelled"),TEXT("blocked"),TEXT("replanned"),TEXT("directive")}.Contains(E.Kind))return false;
        const bool BasicItem=HearthwardBasicItems().ContainsByPredicate([&](const auto& I){return I.Id==E.Item;});
        const bool CraftItem=E.Kind==TEXT("craft") && HearthwardAgent::Capabilities().ContainsByPredicate([&](const auto& C){return C.Id==TEXT("craft") && C.Items.Contains(E.Item);});
        const bool DirectiveItem=E.Kind==TEXT("directive") && ValidDirective(E.Item);
        if(!BasicItem && !CraftItem && !DirectiveItem)return false;
        EventIds.Add(E.Id);
    }
    return WorkingGoal.Original.Len()<=1000 && WorkingGoal.Unresolved.Num()<=4 && WorkingGoal.Limits.Num()<=4;
}

void FHearthwardNPCMemory::Migrate(FGuid CampaignId)
{
    Campaign=CampaignId;Revision=FMath::Max<int64>(1,Revision);
    Records.RemoveAll([](const auto& R){return R.Revoked;});
    for(auto& R:Records) {R.Campaign=Campaign;R.Revision=FMath::Max<int64>(1,R.Revision);}
    for(auto& B:Beliefs){B.Campaign=Campaign;B.Revision=FMath::Clamp<int64>(B.Revision,1,Revision);}
    if(Beliefs.IsEmpty() && HasCampObservation)
        for(const auto& Entry:CampInventory)
            HearthwardBeliefs::UpsertCampStock(Beliefs,Revision,Campaign,Entry.Key,Entry.Value,EHearthwardNPCBeliefSource::Firsthand,CampObservedAt);
}
bool FHearthwardNPCMemory::PutRule(const FString& Constraint,const FString& Original,double Now)
{
    if(!HearthwardAgent::ValidLimit(Constraint) || Records.Num()>=MaxRecords || Original.IsEmpty() || Original.Len()>MaxText || !FMath::IsFinite(Now) || Now<0) return false;
    FHearthwardPlayerMemory R;R.Id=FGuid::NewGuid();R.Kind=TEXT("typed_constraint");R.Text=Original;R.Constraint=Constraint;R.RecordedAt=Now;R.Campaign=Campaign;R.Revision=++Revision;Records.Add(R);Clarification.Reset();WorkingGoal={};return true;
}
TArray<FString> FHearthwardNPCMemory::ApplicableRules(FName Capability) const
{
    TArray<FString> Out;
    for(const auto& R:Records)
    {
        if(R.Revoked) continue;
        if(Capability==TEXT("collect") && R.Kind==TEXT("collection_ban")) Out.AddUnique(TEXT("ban:")+R.BlockedItem.ToString());
        if(R.Kind!=TEXT("typed_constraint")) continue;
        if((Capability==TEXT("collect") && (R.Constraint.StartsWith(TEXT("ban:")) || R.Constraint.StartsWith(TEXT("source:"))))
            || ((Capability==TEXT("craft") || Capability==TEXT("repair")) && (R.Constraint.StartsWith(TEXT("no:")) || R.Constraint.StartsWith(TEXT("max:"))))) Out.AddUnique(R.Constraint);
    }
    return Out;
}
void FHearthwardNPCMemory::RecordEvent(const FHearthwardNPCEvent& Event)
{
    if(Events.ContainsByPredicate([&](const auto& E){return E.Id==Event.Id;})) return;
    if(Events.Num()>=HearthwardAgent::Policy(TEXT("max_events"))) Events.RemoveAt(0);
    Events.Add(Event);
}
