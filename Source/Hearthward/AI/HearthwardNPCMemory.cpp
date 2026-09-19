#include "HearthwardNPCMemory.h"
#include "../Inventory/HearthwardInventoryState.h"

namespace
{
bool ValidKind(FName Kind)
{ return Kind == TEXT("claim") || Kind == TEXT("preference") || Kind == TEXT("agreement") || Kind == TEXT("collection_ban"); }
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
    Existing->BlockedItem=Kind==TEXT("collection_ban")?BlockedItem:NAME_None;
    // Editing a record invalidates any pending interpretation that used its old text.
    Clarification.Reset();
    return true;
}
bool FHearthwardNPCMemory::Revoke(FGuid Id)
{
    auto* R=Records.FindByPredicate([&](const auto& Entry){return Entry.Id==Id && !Entry.Revoked;});
    if (!R) return false;
    R->Revoked=true; Clarification.Reset(); return true;
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
    return Records.ContainsByPredicate([&](const auto& R){return !R.Revoked && R.Kind==TEXT("collection_ban") && R.BlockedItem==Item;});
}
TArray<FHearthwardPlayerMemory> FHearthwardNPCMemory::Retrieve(const FString& Query,bool IncludeAgreements) const
{
    TArray<FHearthwardPlayerMemory> Out, Candidates;
    for (const auto& R:Records)
    {
        if (R.Revoked) continue;
        if (R.Kind==TEXT("collection_ban")) continue;
        if (IncludeAgreements && R.Kind==TEXT("agreement")) Out.Add(R);
        else if (Relevance(Query,R.Text)>0) Candidates.Add(R);
    }
    Candidates.StableSort([&](const auto& A,const auto& B)
    {
        const int32 AS=Relevance(Query,A.Text), BS=Relevance(Query,B.Text);
        return AS!=BS ? AS>BS : A.RecordedAt>B.RecordedAt;
    });
    for (int32 I=0; I<FMath::Min(3,Candidates.Num()); ++I) Out.Add(Candidates[I]);
    return Out;
}
bool FHearthwardNPCMemory::IsValid(double Now) const
{
    if (Records.Num()>MaxRecords || Clarification.Num()>4 || !FMath::IsFinite(CampObservedAt)
        || CampObservedAt<0 || CampObservedAt>Now) return false;
    TSet<FGuid> Ids; int32 Agreements=0, Characters=0;
    for (const auto& R:Records)
    {
        if (!R.Id.IsValid() || Ids.Contains(R.Id) || !ValidKind(R.Kind) || R.Text.TrimStartAndEnd().IsEmpty()
            || R.Text.Len()>MaxText || !FMath::IsFinite(R.RecordedAt) || R.RecordedAt<0 || R.RecordedAt>Now) return false;
        Ids.Add(R.Id);
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
    return true;
}
