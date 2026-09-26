#include "HearthwardSaveGame.h"
#include "../Camp/HearthwardCampState.h"
#include "Serialization/JsonSerializer.h"
#include "../Interaction/HearthwardHarvestSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

int32 HearthwardSave::SelectSlot(const TArray<FHearthwardSavePoint>& Points)
{
    if (Points.Num() < MaxPoints) return Points.Num();
    int32 Oldest = INDEX_NONE;
    for (int32 I = 0; I < Points.Num(); ++I)
        if (!Points[I].Manual && !Points[I].Locked && (Oldest == INDEX_NONE || Points[I].Created < Points[Oldest].Created)) Oldest = I;
    return Oldest;
}

namespace
{
bool ValidCounts(const TMap<FName, int32>& Counts, bool Unlimited)
{
    FHearthwardInventoryState State(Unlimited);
    for (const auto& Pair : Counts)
        if (State.Add(Pair.Key, Pair.Value) != EHearthwardInventoryResult::Success) return false;
    return true;
}
bool ValidTimer(const FHearthwardSavedTimer& Timer)
{
    return uint8(Timer.Status) <= uint8(EHearthwardTimedActionStatus::Completed)
        && FMath::IsFinite(Timer.Elapsed) && Timer.Elapsed >= 0 && Timer.Elapsed <= 5;
}
bool ValidSurvival(const FHearthwardSurvivalState& State,const TMap<FName,int32>& Bag,double Calendar)
{
    if(State.Life!=EHearthwardLife::Alive || State.DownRemaining!=0 || State.DrowningRemaining!=-1) return false;
    for(double Value:{State.SevereDue,State.HotRemaining,State.HotRate,State.RecoveryDelay,State.SafeSeconds,State.MedicineRemaining})
        if(!FMath::IsFinite(Value)) return false;
    if((State.SevereDue!=-1 && State.SevereDue<=Calendar) || State.HotRemaining<0 || State.HotRemaining>15 || State.HotRate<0
        || State.RecoveryDelay<0 || State.RecoveryDelay>.5 || State.SafeSeconds<0 || State.MedicineRemaining<0 || State.MedicineRemaining>3) return false;
    if(State.Medicine.IsNone()!= (State.MedicineRemaining==0)) return false;
    if(!State.Medicine.IsNone() && (Bag.FindRef(State.Medicine)<1 || HearthwardData::Number(HearthwardData::Find(TEXT("items"),State.Medicine.ToString()),TEXT("healing"))<=0)) return false;
    for(FName Id:State.AutoPermissions) if(!HearthwardData::Find(TEXT("items"),Id.ToString())) return false;
    return true;
}
bool Decode(const TArray<uint8>& Bytes, UHearthwardSaveGame*& Out, FString& Error)
{
    constexpr uint32 LegacyMagic = 0x48575331, PreviousMagic = 0x48575332, Magic = 0x48575335;
    uint32 Header[3] = {};
    if (Bytes.Num() < sizeof(Header) || Bytes.Num() > 64 * 1024 * 1024) { Error = TEXT("存档大小无效"); return false; }
    FMemory::Memcpy(Header, Bytes.GetData(), sizeof(Header));
    const int32 Length = Bytes.Num() - sizeof(Header);
    if ((Header[0] != Magic && Header[0]!=LegacyMagic && Header[0]!=PreviousMagic) || Header[1] != uint32(Length) || Header[2] != FCrc::MemCrc32(Bytes.GetData() + sizeof(Header), Length))
    { Error = TEXT("存档完整性校验失败"); return false; }
    TArray<uint8> Payload;
    Payload.Append(Bytes.GetData() + sizeof(Header), Length);
    Out = Cast<UHearthwardSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));

    // Historical files may omit a property that matched the class default of that build.
    // Only the legacy envelope may use this fallback; current-format damaged files never enter it.
    if(Out && Header[0]==LegacyMagic && Out->Schema==HearthwardSave::CurrentSchema
        && Out->Points.ContainsByPredicate([](const auto& P){return P.World.NPCStateVersion==0;}))
        Out->Schema=1;

    if(Out && Header[0]==PreviousMagic && Out->Schema==HearthwardSave::CurrentSchema
        && !Out->Points.ContainsByPredicate([](const auto& P){return P.World.SurvivalVersion!=0;})) Out->Schema=3;

    if(Out && Header[0]==LegacyMagic && Out->Schema==1)
    {
        for(auto& P:Out->Points)
        {
            auto& S=P.World;
            S.NPCMemory.Migrate(P.CampaignId);
            S.NPCStateVersion=2;
            S.Acquired=S.Delivered+FMath::Min(S.Bag.FindRef(S.Item),FMath::Max(0,S.Requested-S.Delivered));
            S.Carried=S.Acquired-S.Delivered;
            if(S.Requested>0)
            {
                S.AgentGoal.Intent=TEXT("collect");S.AgentGoal.Item=S.Item;S.AgentGoal.Quantity=S.Requested;
                S.AgentGoal.QuantityMode=TEXT("additional_acquired");S.AgentGoal.SourceRef=TEXT("S1");
                S.CommandId=FGuid::NewGuid();
            }
        }
        Out->Schema=2;
    }

    // Schema 2 is the real pre-TASK-040 vNext format. It has no LastEvidenceAt or coverage metadata.
    // Migrate that explicit format once, then validate the new format strictly.
    if(Out && Header[0]!=Magic && Out->Schema==2)
    {
        for(auto& P:Out->Points)
        {
            auto& S=P.World;
            if(S.NPCStateVersion!=2) { Error=TEXT("旧版认知快照版本无效"); Out=nullptr; return false; }
            S.NPCMemory.Migrate(P.CampaignId,true,S.CommandActive?S.CommandId:FGuid());
            S.NPCStateVersion=HearthwardSave::NPCStateVersion;
        }
        Out->Schema=3;
    }

    if(Out && Header[0]!=Magic && Out->Schema==3)
    {
        for(auto& P:Out->Points)
        {
            auto& S=P.World;
            TSharedPtr<FJsonObject> G;
            if(!S.Gameplay.IsEmpty()) FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S.Gameplay),G);
            if(G && HearthwardData::Number(G,TEXT("health"))<=0)
            { Error=TEXT("旧存档生命为零，缺少可迁移生存状态；原文件保留"); Out=nullptr; return false; }
            S.SurvivalVersion=1; S.CalendarMinutes=S.ActiveSeconds;
            S.PlayerSurvival={}; S.BrotherSurvival={};
            S.BrotherHealth=S.BrotherHunger=S.BrotherStamina=100;
            // No invented historical deadline: old zero-hunger state starts its new period at the saved boundary.
            if(G && HearthwardData::Number(G,TEXT("hunger"))==0 && HearthwardData::Number(G,TEXT("health"))<=10)
                S.PlayerSurvival.SevereDue=S.CalendarMinutes+4320;
        }
        Out->Schema=HearthwardSave::CurrentSchema;
    }

    if (!Out || !HearthwardSave::Validate(*Out)) { Error = TEXT("存档版本或快照状态无效"); Out = nullptr; return false; }
    return true;
}
}

bool HearthwardSave::Validate(const UHearthwardSaveGame& Pool)
{
    if (Pool.Schema != CurrentSchema || Pool.Points.Num() > MaxPoints) return false;
    TSet<FGuid> Ids;
    for (const auto& P : Pool.Points)
    {
        const auto& S = P.World;
        if(S.SurvivalVersion!=1 || !FMath::IsFinite(S.CalendarMinutes) || S.CalendarMinutes<S.ActiveSeconds
            || !ValidSurvival(S.PlayerSurvival,S.Inventory,S.CalendarMinutes) || !ValidSurvival(S.BrotherSurvival,S.Bag,S.CalendarMinutes)
            || !FMath::IsFinite(S.BrotherHealth) || S.BrotherHealth<=0 || S.BrotherHealth>100
            || !FMath::IsFinite(S.BrotherHunger) || S.BrotherHunger<0 || S.BrotherHunger>100
            || !FMath::IsFinite(S.BrotherStamina) || S.BrotherStamina<0 || S.BrotherStamina>100) return false;
        if(S.NPCStateVersion!=NPCStateVersion || S.Acquired<0 || S.Carried<0 || S.Acquired<S.Delivered || S.Acquired>S.Requested || S.Carried!=S.Acquired-S.Delivered
            || S.Carried>S.Bag.FindRef(S.Item) || S.NPCOperations.Num()>512 || S.NPCMemory.Campaign!=P.CampaignId) return false;
        if(S.CommandActive && (S.AgentGoal.Intent.IsNone() || !S.CommandId.IsValid()))return false;
        if(!S.AgentGoal.Intent.IsNone() && !HearthwardAgent::Validate(S.AgentGoal).IsEmpty())return false;
        if(!ValidCounts(S.NPCSpent,true))return false;
        TSet<FGuid> Operations;
        for(const auto& Id:S.NPCOperations){if(!Id.IsValid() || Operations.Contains(Id))return false;Operations.Add(Id);}
        Operations.Reset();if(S.NPCReceipts.Num()>512)return false;
        for(const auto& R:S.NPCReceipts)
        {if(!R.Id.IsValid() || R.Command!=S.CommandId || R.Payload.IsEmpty() || R.Payload.Len()>200 || Operations.Contains(R.Id))return false;Operations.Add(R.Id);}
        for(const auto& D:S.NPCDurability)
        {
            auto Item=HearthwardData::Find(TEXT("items"),D.Key.ToString());
            if(!Item || !FMath::IsFinite(D.Value) || D.Value<0 || D.Value>HearthwardData::Number(Item,TEXT("durability")))return false;
        }
        if (!S.NPCMemory.IsValid(S.ActiveSeconds)) return false;
        FHearthwardCampState Camp;
        if(!S.CampEconomy.IsEmpty() && (!FHearthwardCampState::Parse(S.CampEconomy,Camp) || FMath::Abs(Camp.Calendar-S.CalendarMinutes)>1.e-4 || !Camp.ValidateBuildings(S.Gameplay))) return false;
        if(!UHearthwardGameplayComponent::ValidateSnapshot(S.Gameplay) || !UHearthwardHarvestSubsystem::Validate(S.HarvestedResources)) return false;
        if (!P.SaveId.IsValid() || !P.CampaignId.IsValid() || Ids.Contains(P.SaveId) || S.Map.IsEmpty()
            || !FMath::IsFinite(S.ActiveSeconds) || S.ActiveSeconds < 0 || S.KnowledgeRevision != S.Knowledge.Num()
            || S.AutoMinutes < 1 || S.AutoMinutes > 60 || !S.Safety.CanSave()
            || !S.Player.IsValid() || !S.Companion.IsValid() || !S.Source.IsValid() || !S.Camp.IsValid() || S.View.ContainsNaN()
            || !ValidCounts(S.Inventory, false) || !ValidCounts(S.Storage, true) || !ValidCounts(S.Bag, false) || !ValidCounts(S.Resource, false)
            || !ValidTimer(S.PlayerTimer) || !ValidTimer(S.CompanionTimer)
            || uint8(S.Phase) > uint8(EHearthwardCompanionPhase::HoldingSafely)
            || S.Requested < 0 || S.Delivered < 0 || S.Delivered > S.Requested) return false;
        if (S.Requested > 0 && !HearthwardBasicItems().ContainsByPredicate([&S](const auto& I) { return I.Id == S.Item; })) return false;
        if (S.CommandActive && (S.Requested == 0 || S.Delivered == S.Requested)) return false;
        const bool Executing = S.Phase == EHearthwardCompanionPhase::GoingToSource || S.Phase == EHearthwardCompanionPhase::Gathering
            || S.Phase == EHearthwardCompanionPhase::Returning || S.Phase == EHearthwardCompanionPhase::ReturningBlocked
            || S.Phase == EHearthwardCompanionPhase::GoingToWorkshop || S.Phase == EHearthwardCompanionPhase::TakingMaterials || S.Phase == EHearthwardCompanionPhase::HoldingSafely;
        if (Executing && !S.CommandActive) return false;
        if (S.Phase == EHearthwardCompanionPhase::Completed && (S.Requested == 0 || S.Delivered != S.Requested || S.CommandActive)) return false;
        if (S.Phase == EHearthwardCompanionPhase::Gathering && S.CompanionTimer.Status != EHearthwardTimedActionStatus::Running
            && S.CompanionTimer.Status != EHearthwardTimedActionStatus::Completed) return false;
        Ids.Add(P.SaveId);
    }
    return true;
}

bool HearthwardSave::Read(const FString& Path, UHearthwardSaveGame*& Out, FString& Error)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path)) { Error = TEXT("无法读取存档池"); return false; }
    return Decode(Bytes, Out, Error);
}

bool HearthwardSave::Write(const FString& Path, UHearthwardSaveGame* Pool, FString& Error)
{
    if (!Pool || !Validate(*Pool)) { Error = TEXT("拒绝写入无效快照"); return false; }
    TArray<uint8> Payload, Bytes;
    if (!UGameplayStatics::SaveGameToMemory(Pool, Payload)) { Error = TEXT("快照序列化失败"); return false; }
    const uint32 Header[] = {0x48575335, uint32(Payload.Num()), FCrc::MemCrc32(Payload.GetData(), Payload.Num())};
    Bytes.Append(reinterpret_cast<const uint8*>(Header), sizeof(Header));
    Bytes.Append(Payload);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
    const FString Temp = Path + TEXT(".pending");
    if (!FFileHelper::SaveArrayToFile(Bytes, *Temp)) { Error = TEXT("临时存档写入失败，原档未替换"); return false; }
    UHearthwardSaveGame* Verified = nullptr;
    if (!Read(Temp, Verified, Error)) return false;
#if PLATFORM_WINDOWS
    const FString From = FPaths::ConvertRelativePathToFull(Temp), To = FPaths::ConvertRelativePathToFull(Path);
    if (MoveFileExW(*From, *To, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
#endif
    Error = TEXT("存档原子替换失败，原档未替换");
    return false;
}
