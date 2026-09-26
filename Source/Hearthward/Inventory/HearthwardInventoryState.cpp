#include "HearthwardInventoryState.h"
#include "../Gameplay/HearthwardGameData.h"

const TArray<FHearthwardItemDefinition>& HearthwardBasicItems()
{
    static const TArray<FHearthwardItemDefinition> Items=[]
    {
        TArray<FHearthwardItemDefinition> Out;
        for(const auto& V:HearthwardData::Rows(TEXT("items")))
        {
            const auto R=V->AsObject();
            FHearthwardItemDefinition D;
            D.Id=FName(*HearthwardData::Text(R,TEXT("id")));
            D.WeightHundredths=int32(HearthwardData::Number(R,TEXT("weight")));
            D.DisplayName=FText::FromString(HearthwardData::Text(R,TEXT("name")));
            D.Slot=FName(*HearthwardData::Text(R,TEXT("slot")));
            D.MaximumDurability=HearthwardData::Number(R,TEXT("durability"));
            D.UniqueClaim=FName(*HearthwardData::Text(R,TEXT("uniqueClaim")));
            Out.Add(D);
        }
        return Out;
    }();
    return Items;
}
const FHearthwardItemDefinition* FHearthwardInventoryState::FindItem(FName Item)
{ return HearthwardBasicItems().FindByPredicate([&](const auto& D){return D.Id==Item;}); }
int32 FHearthwardInventoryState::GetCount(FName Item) const
{
    int32 Count=Data.Stacks.FindRef(Item);
    for(const auto& I:Data.Instances) if(I.Definition==Item) ++Count;
    return Count;
}
int64 FHearthwardInventoryState::GetWeightHundredths() const
{
    int64 Weight=0;
    for(const auto& D:HearthwardBasicItems()) Weight+=int64(GetCount(D.Id))*D.WeightHundredths;
    return Weight;
}
bool FHearthwardInventoryState::UpgradeBackpack()
{ if(Unlimited || Data.BackpackRank>=5)return false; ++Data.BackpackRank;return true; }
const FHearthwardItemInstance* FHearthwardInventoryState::FindInstance(FGuid Id) const
{ return Data.Instances.FindByPredicate([&](const auto& I){return I.Id==Id;}); }
bool FHearthwardInventoryState::IsEquipped(FGuid Id) const
{ for(const auto& E:Data.Equipped)if(E.Value==Id)return true;return false; }
FGuid FHearthwardInventoryState::FirstInstance(FName Item,bool PreferUnequipped) const
{
    FGuid First;
    for(const auto& I:Data.Instances)if(I.Definition==Item)
    { if(!First.IsValid())First=I.Id;if(!PreferUnequipped || !IsEquipped(I.Id))return I.Id; }
    return First;
}
FName FHearthwardInventoryState::EquippedItem(FName Slot) const
{ const auto* I=FindInstance(EquippedInstance(Slot));return I?I->Definition:NAME_None; }
bool FHearthwardInventoryState::Equip(FGuid Id)
{
    const auto* I=FindInstance(Id);const auto* D=I?FindItem(I->Definition):nullptr;
    if(!D || D->Slot.IsNone())return false;
    if(Data.Equipped.FindRef(D->Slot)==Id)Data.Equipped.Remove(D->Slot);
    else Data.Equipped.Add(D->Slot,Id);
    return true;
}
EHearthwardInventoryResult FHearthwardInventoryState::InsertInstance(const FHearthwardItemInstance& I)
{
    const auto* D=FindItem(I.Definition);
    if(!D || !D->IsInstance())return EHearthwardInventoryResult::UnknownItem;
    if(!I.Id.IsValid() || FindInstance(I.Id) || !FMath::IsFinite(I.Durability) || I.Durability<0 || I.Durability>D->MaximumDurability || I.UniqueClaim!=D->UniqueClaim)return EHearthwardInventoryResult::InvalidArgument;
    if(!Unlimited && GetWeightHundredths()+D->WeightHundredths>GetCapacityHundredths())return EHearthwardInventoryResult::CapacityExceeded;
    Data.Instances.Add(I);return EHearthwardInventoryResult::Success;
}
bool FHearthwardInventoryState::RemoveInstance(FGuid Id)
{
    if(!FindInstance(Id))return false;
    for(auto It=Data.Equipped.CreateIterator();It;++It)if(It.Value()==Id)It.RemoveCurrent();
    Data.Instances.RemoveAll([&](const auto& I){return I.Id==Id;});return true;
}
EHearthwardInventoryResult FHearthwardInventoryState::Add(FName Item,int32 Count)
{
    if(Count<=0)return EHearthwardInventoryResult::InvalidCount;
    const auto* D=FindItem(Item);if(!D)return EHearthwardInventoryResult::UnknownItem;
    if(!Unlimited && D->WeightHundredths>0 && Count>(GetCapacityHundredths()-GetWeightHundredths())/D->WeightHundredths)return EHearthwardInventoryResult::CapacityExceeded;
    if(Count>MAX_int32-GetCount(Item))return EHearthwardInventoryResult::QuantityOverflow;
    if(!D->UniqueClaim.IsNone() && (Count!=1 || Data.Instances.ContainsByPredicate([&](const auto& I){return I.UniqueClaim==D->UniqueClaim;})))return EHearthwardInventoryResult::InvalidArgument;
    if(D->IsInstance())for(int32 N=0;N<Count;++N)
    { FHearthwardItemInstance I;I.Id=FGuid::NewGuid();I.Definition=Item;I.Durability=D->MaximumDurability;I.UniqueClaim=D->UniqueClaim;Data.Instances.Add(I); }
    else Data.Stacks.FindOrAdd(Item)+=Count;
    return EHearthwardInventoryResult::Success;
}
EHearthwardInventoryResult FHearthwardInventoryState::Remove(FName Item,int32 Count)
{
    if(Count<=0)return EHearthwardInventoryResult::InvalidCount;
    const auto* D=FindItem(Item);if(!D)return EHearthwardInventoryResult::UnknownItem;
    if(Count>GetCount(Item))return EHearthwardInventoryResult::InsufficientItems;
    if(!D->UniqueClaim.IsNone() && (Count!=1 || Data.Instances.ContainsByPredicate([&](const auto& I){return I.UniqueClaim==D->UniqueClaim;})))return EHearthwardInventoryResult::InvalidArgument;
    if(D->IsInstance())for(int32 N=0;N<Count;++N)RemoveInstance(FirstInstance(Item,true));
    else { Data.Stacks[Item]-=Count;if(Data.Stacks[Item]==0)Data.Stacks.Remove(Item); }
    return EHearthwardInventoryResult::Success;
}
EHearthwardInventoryResult FHearthwardInventoryState::Exchange(const TMap<FName,int32>& Materials,const TMap<FName,int32>& Outputs,int32 Batches)
{
    if(Batches<=0)return EHearthwardInventoryResult::InvalidCount;
    if(Materials.IsEmpty() || Outputs.IsEmpty())return EHearthwardInventoryResult::InvalidArgument;
    auto After=*this;
    for(const auto* Map:{&Materials,&Outputs})for(const auto& E:*Map)
    { if(E.Value<=0)return EHearthwardInventoryResult::InvalidCount;if(int64(E.Value)*Batches>MAX_int32)return EHearthwardInventoryResult::QuantityOverflow; }
    for(const auto& E:Materials){auto R=After.Remove(E.Key,E.Value*Batches);if(R!=EHearthwardInventoryResult::Success)return R;}
    for(const auto& E:Outputs){auto R=After.Add(E.Key,E.Value*Batches);if(R!=EHearthwardInventoryResult::Success)return R;}
    *this=MoveTemp(After);return EHearthwardInventoryResult::Success;
}
EHearthwardInventoryResult FHearthwardInventoryState::TransferInstanceTo(FHearthwardInventoryState& Target,FGuid Id)
{
    if(this==&Target)return EHearthwardInventoryResult::InvalidArgument;
    const auto* I=FindInstance(Id);if(!I)return EHearthwardInventoryResult::InsufficientItems;
    const auto R=Target.InsertInstance(*I);
    if(R==EHearthwardInventoryResult::Success)RemoveInstance(Id);
    return R;
}
EHearthwardInventoryResult FHearthwardInventoryState::TransferTo(FHearthwardInventoryState& Target,FName Item,int32 Count)
{
    if(this==&Target)return EHearthwardInventoryResult::InvalidArgument;
    if(Count<=0)return EHearthwardInventoryResult::InvalidCount;
    const auto* D=FindItem(Item);if(!D)return EHearthwardInventoryResult::UnknownItem;
    if(Count>GetCount(Item))return EHearthwardInventoryResult::InsufficientItems;
    auto SourceAfter=*this,TargetAfter=Target;
    if(D->IsInstance())
    {
        for(int32 N=0;N<Count;++N)
        { const auto R=SourceAfter.TransferInstanceTo(TargetAfter,SourceAfter.FirstInstance(Item,true));if(R!=EHearthwardInventoryResult::Success)return R; }
    }
    else
    {
        auto R=SourceAfter.Remove(Item,Count);if(R!=EHearthwardInventoryResult::Success)return R;
        R=TargetAfter.Add(Item,Count);if(R!=EHearthwardInventoryResult::Success)return R;
    }
    *this=MoveTemp(SourceAfter);Target=MoveTemp(TargetAfter);return EHearthwardInventoryResult::Success;
}
bool FHearthwardInventoryState::Wear(FGuid Id,double Amount)
{
    auto* I=Data.Instances.FindByPredicate([&](const auto& E){return E.Id==Id;});
    const auto* D=I?FindItem(I->Definition):nullptr;
    if(!D || D->MaximumDurability<=0 || !FMath::IsFinite(Amount) || Amount<=0 || I->Durability<=0)return false;
    I->Durability=FMath::Max(0.,I->Durability-Amount);return true;
}
bool FHearthwardInventoryState::RestoreDurability(FGuid Id,double Amount)
{
    auto* I=Data.Instances.FindByPredicate([&](const auto& E){return E.Id==Id;});
    const auto* D=I?FindItem(I->Definition):nullptr;
    if(!D || D->MaximumDurability<=0 || !FMath::IsFinite(Amount) || Amount<=0 || I->Durability>=D->MaximumDurability)return false;
    I->Durability=FMath::Min(D->MaximumDurability,I->Durability+Amount);return true;
}
bool FHearthwardInventoryState::Validate(const FHearthwardInventorySnapshot& S,bool InUnlimited)
{
    if(S.Version!=1 || S.BackpackRank<1 || S.BackpackRank>5)return false;
    TSet<FGuid> Ids;TSet<FName> Unique;int64 Weight=0;
    for(const auto& E:S.Stacks)
    { const auto* D=FindItem(E.Key);if(!D || D->IsInstance() || E.Value<=0)return false;Weight+=int64(E.Value)*D->WeightHundredths; }
    for(const auto& I:S.Instances)
    {
        const auto* D=FindItem(I.Definition);
        if(!D || !D->IsInstance() || !I.Id.IsValid() || Ids.Contains(I.Id) || !FMath::IsFinite(I.Durability) || I.Durability<0 || I.Durability>D->MaximumDurability || I.UniqueClaim!=D->UniqueClaim)return false;
        if(!I.UniqueClaim.IsNone()){if(Unique.Contains(I.UniqueClaim))return false;Unique.Add(I.UniqueClaim);}
        Ids.Add(I.Id);Weight+=D->WeightHundredths;
    }
    for(const auto& E:S.Equipped)
    {
        const auto* I=S.Instances.FindByPredicate([&](const auto& Entry){return Entry.Id==E.Value;});
        if(!I || E.Key.IsNone() || FindItem(I->Definition)->Slot!=E.Key)return false;
    }
    return InUnlimited || Weight<=int64(100+50*(S.BackpackRank-1))*100;
}
bool FHearthwardInventoryState::Restore(const FHearthwardInventorySnapshot& S)
{ if(!Validate(S,Unlimited))return false;Data=S;return true; }
