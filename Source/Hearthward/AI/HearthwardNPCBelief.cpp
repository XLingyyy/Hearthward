#include "HearthwardNPCBelief.h"
#include "../Inventory/HearthwardInventoryState.h"

namespace
{
bool ValidItem(FName Item)
{
    return HearthwardBasicItems().ContainsByPredicate([&](const auto& Def){ return Def.Id == Item; });
}
}

bool HearthwardBeliefs::UpsertCampStock(TArray<FHearthwardNPCBelief>& Beliefs, int64& MemoryRevision, FGuid Campaign,
    FName Item, int32 Value, EHearthwardNPCBeliefSource Source, double Now)
{
    if(!Campaign.IsValid() || !ValidItem(Item) || Value < 0 || !FMath::IsFinite(Now) || Now < 0) return false;
    auto* Existing=Beliefs.FindByPredicate([&](const auto& B){ return B.Kind==TEXT("camp_stock") && B.Item==Item; });
    if(Existing && Existing->Value==Value && Existing->Source==Source && Existing->Campaign==Campaign) return true;
    if(!Existing)
    {
        Existing=&Beliefs.AddDefaulted_GetRef();
        Existing->Id=FGuid::NewGuid();
        Existing->Kind=TEXT("camp_stock");
        Existing->Item=Item;
    }
    Existing->Value=Value;
    Existing->Source=Source;
    Existing->RecordedAt=Now;
    Existing->Campaign=Campaign;
    Existing->Revision=++MemoryRevision;
    return true;
}

bool HearthwardBeliefs::ResolveCampStock(const TArray<FHearthwardNPCBelief>& Beliefs, FName Item, FHearthwardNPCBeliefView& Out)
{
    Out={};
    const auto* B=Beliefs.FindByPredicate([&](const auto& X){ return X.Kind==TEXT("camp_stock") && X.Item==Item; });
    if(!B) return false;
    Out.Known=true;Out.Value=B->Value;Out.Source=B->Source;Out.RecordedAt=B->RecordedAt;Out.Revision=B->Revision;
    return true;
}

bool HearthwardBeliefs::Validate(const TArray<FHearthwardNPCBelief>& Beliefs, int64 MemoryRevision, FGuid Campaign, double Now)
{
    if(Beliefs.Num()>HearthwardBasicItems().Num()) return false;
    TSet<FGuid> Ids;TSet<FName> Items;
    for(const auto& B:Beliefs)
    {
        if(!B.Id.IsValid() || Ids.Contains(B.Id) || B.Kind!=TEXT("camp_stock") || !ValidItem(B.Item)
            || Items.Contains(B.Item) || B.Value<0 || !FMath::IsFinite(B.RecordedAt) || B.RecordedAt<0 || B.RecordedAt>Now
            || B.Revision<1 || B.Revision>MemoryRevision || B.Campaign!=Campaign) return false;
        Ids.Add(B.Id);Items.Add(B.Item);
    }
    return true;
}

FString HearthwardBeliefs::SourceName(EHearthwardNPCBeliefSource Source)
{
    switch(Source)
    {
    case EHearthwardNPCBeliefSource::Firsthand:return TEXT("firsthand");
    case EHearthwardNPCBeliefSource::PlayerReport:return TEXT("player_report");
    case EHearthwardNPCBeliefSource::Receipt:return TEXT("receipt");
    default:return TEXT("unknown");
    }
}
