#include "HearthwardSaveGame.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardProgression.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/SecureHash.h"
using namespace HearthwardData;
namespace
{
FGuid LegacyInstanceId(FGuid Save,FName Container,FName Definition,int32 Index)
{
    // A stable identity makes retrying the same old snapshot idempotent.
    const FString Key=Save.ToString()+TEXT(":")+Container.ToString()+TEXT(":")+Definition.ToString()+FString::FromInt(Index);
    const auto Bytes=StringCast<UTF8CHAR>(*Key);FMD5 Digest;Digest.Update(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());uint8 Out[16];Digest.Final(Out);
    uint32 Parts[4];FMemory::Memcpy(Parts,Out,sizeof(Out));return FGuid(Parts[0],Parts[1],Parts[2],Parts[3]);
}
bool Convert(FGuid Save,FName Container,const TMap<FName,int32>& Counts,const TMap<FName,float>& Own,const TSharedPtr<FJsonObject>& Global,const TSharedPtr<FJsonObject>& Equipment,FHearthwardInventorySnapshot& Out,bool Unlimited)
{
    Out={};
    for(const auto& E:Counts)
    {
        const auto* D=FHearthwardInventoryState::FindItem(E.Key);if(!D || E.Value<=0)return false;
        if(!D->IsInstance()){Out.Stacks.Add(E);continue;}
        double Percent=1;
        if(const auto* Wear=Own.Find(E.Key))Percent=*Wear/100.;
        else if(Global && Global->HasField(E.Key.ToString()))Percent=Number(Global,E.Key.ToString())/100.;
        if(!FMath::IsFinite(Percent) || Percent<0 || Percent>1)return false;
        for(int32 N=0;N<E.Value;++N)
        {
            FHearthwardItemInstance I;I.Id=LegacyInstanceId(Save,Container,E.Key,N);I.Definition=E.Key;I.Durability=Percent*D->MaximumDurability;I.UniqueClaim=D->UniqueClaim;
            Out.Instances.Add(I);
            if(N==0 && Equipment && Text(Equipment,D->Slot.ToString())==E.Key.ToString())Out.Equipped.Add(D->Slot,I.Id);
        }
    }
    return FHearthwardInventoryState::Validate(Out,Unlimited);
}
}
bool HearthwardSave::MigrateInventory(UHearthwardSaveGame& Pool,FString& Error)
{
    if(Pool.Schema!=5)return false;
    auto Points=Pool.Points;
    for(auto& P:Points)
    {
        auto& S=P.World;TSharedPtr<FJsonObject> G;
        if(!S.Gameplay.IsEmpty() && !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S.Gameplay),G))return false;
        const TSharedPtr<FJsonObject>* Global=nullptr;const TSharedPtr<FJsonObject>* Equipment=nullptr;
        if(G){G->TryGetObjectField(TEXT("durability"),Global);G->TryGetObjectField(TEXT("equipment"),Equipment);}
        if(!Convert(P.SaveId,TEXT("player"),S.Inventory,{},Global?*Global:nullptr,Equipment?*Equipment:nullptr,S.PlayerItems,false)
            || !Convert(P.SaveId,TEXT("brother"),S.Bag,S.NPCDurability,Global?*Global:nullptr,nullptr,S.BrotherItems,false)
            || !Convert(P.SaveId,TEXT("storage"),S.Storage,{},Global?*Global:nullptr,nullptr,S.StorageItems,true))
        {Error=TEXT("旧库存或耐久无效，原档保留");return false;}
        S.Inventory.Reset();S.Bag.Reset();S.Storage.Reset();S.NPCDurability.Reset();
        if(G)
        {
            G->SetNumberField(TEXT("experience"),FMath::Min(double(HearthwardProgression::MaximumExperience()),Number(G,TEXT("experience"))));
            G->SetObjectField(TEXT("skills"),MakeShared<FJsonObject>());G->SetObjectField(TEXT("equipment"),MakeShared<FJsonObject>());G->SetObjectField(TEXT("durability"),MakeShared<FJsonObject>());
            TArray<TSharedPtr<FJsonValue>> Facts;
            const TArray<TSharedPtr<FJsonValue>>* Claimed;
            if(G->TryGetArrayField(TEXT("claimed"),Claimed))for(const auto& Q:*Claimed)Facts.Add(MakeShared<FJsonValueString>(TEXT("quest:")+Q->AsString()));
            if(G->TryGetArrayField(TEXT("discovered"),Claimed))for(const auto& Q:*Claimed)Facts.Add(MakeShared<FJsonValueString>(TEXT("discover:")+Q->AsString()));
            if(S.PlayerItems.Instances.ContainsByPredicate([](const auto& I){return I.Definition==TEXT("amulet");}))Facts.Add(MakeShared<FJsonValueString>(TEXT("claim:campaign_start_amulet")));
            G->SetArrayField(TEXT("rewardFacts"),Facts);G->SetArrayField(TEXT("knownRecipes"),{});
            const int32 Level=HearthwardProgression::Level(Number(G,TEXT("experience")));
            const auto Growth=Find(TEXT("levels"),FString::FromInt(Level));
            const auto& Tiers=Catalog()->GetObjectField(TEXT("campEconomy"))->GetArrayField(TEXT("camp_tiers"));
            const int32 Tier=Number(G,TEXT("campTier"),1);if(!Tiers.IsValidIndex(Tier-1))return false;
            G->SetNumberField(TEXT("health"),FMath::Min(Number(G,TEXT("health")),100+Number(Growth,TEXT("hp_bonus"))+Number(Tiers[Tier-1]->AsObject(),TEXT("cumulative_hp_bonus"))));
            G->SetNumberField(TEXT("stamina"),FMath::Min(Number(G,TEXT("stamina")),100+Number(Growth,TEXT("stamina_bonus"))+Number(Tiers[Tier-1]->AsObject(),TEXT("cumulative_stamina_bonus"))));
            S.Gameplay.Reset();FJsonSerializer::Serialize(G.ToSharedRef(),TJsonWriterFactory<>::Create(&S.Gameplay));
        }
    }
    Pool.Points=MoveTemp(Points);Pool.Schema=6;return true;
}
