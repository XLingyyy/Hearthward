#include "HearthwardSaveGame.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Nature/HearthwardNatureState.h"
#include "../Gameplay/HearthwardProgression.h"
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
    constexpr uint32 LegacyMagic = 0x48575331, PreviousMagic = 0x48575332, Schema5Magic = 0x48575335, Schema6Magic = 0x48575336, Schema7Magic = 0x48575337, Magic = 0x48575338;
    uint32 Header[3] = {};
    if (Bytes.Num() < sizeof(Header) || Bytes.Num() > 64 * 1024 * 1024) { Error = TEXT("存档大小无效"); return false; }
    FMemory::Memcpy(Header, Bytes.GetData(), sizeof(Header));
    const int32 Length = Bytes.Num() - sizeof(Header);
    if ((Header[0] != Magic && Header[0]!=Schema7Magic && Header[0]!=Schema6Magic && Header[0]!=Schema5Magic && Header[0]!=LegacyMagic && Header[0]!=PreviousMagic) || Header[1] != uint32(Length) || Header[2] != FCrc::MemCrc32(Bytes.GetData() + sizeof(Header), Length))
    { Error = TEXT("存档完整性校验失败"); return false; }
    TArray<uint8> Payload;
    Payload.Append(Bytes.GetData() + sizeof(Header), Length);
    Out = Cast<UHearthwardSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));

    if(Out && Header[0]==Schema7Magic && Out->Schema==HearthwardSave::CurrentSchema)Out->Schema=7;
    if(Out && Header[0]==Schema6Magic && Out->Schema==HearthwardSave::CurrentSchema)Out->Schema=6;
    if(Out && Header[0]==Schema5Magic && Out->Schema==HearthwardSave::CurrentSchema)Out->Schema=5;

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
        Out->Schema=5;
    }

    if(Out && Header[0]!=Magic && Out->Schema==5 && !HearthwardSave::MigrateInventory(*Out,Error)){Out=nullptr;return false;}
    if(Out && Header[0]!=Magic && Out->Schema==6)
    {
        for(auto& P:Out->Points)
        {
            P.World.Nature.Reset();
            FHearthwardCampState Camp;
            if(!P.World.CampEconomy.IsEmpty() && FHearthwardCampState::Parse(P.World.CampEconomy,Camp))
            {
                for(auto& Source:Camp.Sources)
                {
                    if(Source.RefreshMinutes==0)Source.RefreshMinutes=Source.Item==TEXT("refined_ore")?5760:2880;
                    if(Source.Remaining==0 && Source.Due<0)Source.Due=P.World.CalendarMinutes+Source.RefreshMinutes;
                }
                P.World.CampEconomy=Camp.Snapshot();
            }
        }
        Out->Schema=7;
    }
    if(Out && Header[0]!=Magic && Out->Schema==7)
    {
        for(auto& P:Out->Points)
        {
            if(!P.World.NaturalWorld)continue;
            FHearthwardCampState Camp;
            if(!FHearthwardCampState::Parse(P.World.CampEconomy,Camp) || Camp.Camps.IsEmpty()){Error=TEXT("旧营地快照无效");Out=nullptr;return false;}
            FHearthwardCampaignState Campaign;Campaign.Initialize(true,Camp.Hometown);
            for(const auto& Site:Camp.Camps)Campaign.Positions.Add(Site.Id,Site.Position);
            for(FName Id:Camp.Rescued)
            {
                auto* Person=Campaign.People.FindByPredicate([&](const auto& V){return V.Id==Id;});
                if(!Person){Error=TEXT("旧档包含未映射的救援身份，原文件已保留");Out=nullptr;return false;}
                Person->Stage=TEXT("arrived");
                Person->Position=Camp.Camps[0].Position+FVector(200,200+Campaign.People.IndexOfByPredicate([&](const auto& V){return V.Id==Id;})*100,0);Person->Located=true;
            }
            P.World.Campaign=Campaign.Snapshot();
        }
        Out->Schema=8;
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
        FHearthwardCampaignState Campaign;
        if(!FHearthwardCampaignState::Parse(S.Campaign,Campaign))return false;
        FHearthwardNatureState Nature;
        if(!FHearthwardNatureState::Parse(S.Nature,Nature) || (!S.Nature.IsEmpty() && FMath::Abs(Nature.Calendar-S.CalendarMinutes)>1.e-4))return false;
        FHearthwardCampState Camp;
        if(!S.CampEconomy.IsEmpty() && (!FHearthwardCampState::Parse(S.CampEconomy,Camp) || FMath::Abs(Camp.Calendar-S.CalendarMinutes)>1.e-4 || !Camp.ValidateBuildings(S.Gameplay))) return false;
        if(!Campaign.Phase.IsNone())
        {
            if(Campaign.Victory!=Camp.Hometown)return false;
            for(const auto& Person:Campaign.People)if((Person.Stage==TEXT("arrived"))!=Camp.Rescued.Contains(Person.Id))return false;
        }
        TSharedPtr<FJsonObject> Gameplay;
        if(!S.Gameplay.IsEmpty()) FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S.Gameplay),Gameplay);
        const int32 Tier=Gameplay?int32(HearthwardData::Number(Gameplay,TEXT("campTier"),1)):1;
        const auto Growth=HearthwardCamp::Tier(Tier);
        if(!Growth)return false;
        const int32 Level=HearthwardProgression::Level(Gameplay?HearthwardData::Number(Gameplay,TEXT("experience")):0);
        // Match the float caps used by the live survival component, including growth rounding.
        const float BrotherMaxHealth=100+HearthwardProgression::Attribute(Level,TEXT("hp_bonus"))+HearthwardData::Number(Growth,TEXT("cumulative_hp_bonus"));
        const float BrotherMaxStamina=100+HearthwardProgression::Attribute(Level,TEXT("stamina_bonus"))+HearthwardData::Number(Growth,TEXT("cumulative_stamina_bonus"));
        if(S.SurvivalVersion!=1 || !FMath::IsFinite(S.CalendarMinutes) || S.CalendarMinutes<S.ActiveSeconds
            || !ValidSurvival(S.PlayerSurvival,S.PlayerItems.Stacks,S.CalendarMinutes) || !ValidSurvival(S.BrotherSurvival,S.BrotherItems.Stacks,S.CalendarMinutes)
            || !FMath::IsFinite(S.BrotherHealth) || S.BrotherHealth<=0 || S.BrotherHealth>BrotherMaxHealth
            || !FMath::IsFinite(S.BrotherHunger) || S.BrotherHunger<0 || S.BrotherHunger>100
            || !FMath::IsFinite(S.BrotherStamina) || S.BrotherStamina<0 || S.BrotherStamina>BrotherMaxStamina) return false;
        if(S.NPCStateVersion!=NPCStateVersion || S.Acquired<0 || S.Carried<0 || S.Acquired<S.Delivered || S.Acquired>S.Requested || S.Carried!=S.Acquired-S.Delivered
            || S.Carried>S.BrotherItems.Stacks.FindRef(S.Item)+S.BrotherItems.Instances.FilterByPredicate([&](const auto& I){return I.Definition==S.Item;}).Num() || S.NPCOperations.Num()>512 || S.NPCMemory.Campaign!=P.CampaignId) return false;
        if(S.CommandActive && (S.AgentGoal.Intent.IsNone() || !S.CommandId.IsValid()))return false;
        if(!S.AgentGoal.Intent.IsNone() && !HearthwardAgent::Validate(S.AgentGoal).IsEmpty())return false;
        if(!ValidCounts(S.NPCSpent,true))return false;
        TSet<FGuid> Operations;
        for(const auto& Id:S.NPCOperations){if(!Id.IsValid() || Operations.Contains(Id))return false;Operations.Add(Id);}
        Operations.Reset();if(S.NPCReceipts.Num()>512)return false;
        for(const auto& R:S.NPCReceipts)
        {if(!R.Id.IsValid() || R.Command!=S.CommandId || R.Payload.IsEmpty() || R.Payload.Len()>200 || Operations.Contains(R.Id))return false;Operations.Add(R.Id);}
        if(!S.Inventory.IsEmpty() || !S.Storage.IsEmpty() || !S.Bag.IsEmpty() || !S.NPCDurability.IsEmpty())return false;
        TSet<FGuid> InstanceIds;TSet<FName> UniqueClaims;
        for(const auto* Inventory:{&S.PlayerItems,&S.BrotherItems,&S.StorageItems})
        {
            if(!FHearthwardInventoryState::Validate(*Inventory,Inventory==&S.StorageItems))return false;
            for(const auto& I:Inventory->Instances)
            {
                if(InstanceIds.Contains(I.Id) || (!I.UniqueClaim.IsNone() && UniqueClaims.Contains(I.UniqueClaim)))return false;
                InstanceIds.Add(I.Id);if(!I.UniqueClaim.IsNone())UniqueClaims.Add(I.UniqueClaim);
            }
        }
        for(const auto& Ground:S.GroundEquipment)
        {
            FHearthwardInventorySnapshot Single;Single.Instances.Add(Ground.Item);
            const auto& I=Ground.Item;
            if(!Ground.Transform.IsValid() || !FHearthwardInventoryState::Validate(Single,true) || InstanceIds.Contains(I.Id)
                || (!I.UniqueClaim.IsNone() && UniqueClaims.Contains(I.UniqueClaim)))return false;
            InstanceIds.Add(I.Id);if(!I.UniqueClaim.IsNone())UniqueClaims.Add(I.UniqueClaim);
        }
        for(const auto& Point:Nature.Points)for(const auto& I:Point.Pending.Instances)
        {
            if(InstanceIds.Contains(I.Id) || (!I.UniqueClaim.IsNone() && UniqueClaims.Contains(I.UniqueClaim)))return false;
            InstanceIds.Add(I.Id);if(!I.UniqueClaim.IsNone())UniqueClaims.Add(I.UniqueClaim);
        }
        if (!S.NPCMemory.IsValid(S.ActiveSeconds)) return false;
        if(!UHearthwardGameplayComponent::ValidateSnapshot(S.Gameplay) || !UHearthwardHarvestSubsystem::Validate(S.HarvestedResources)) return false;
        if (!P.SaveId.IsValid() || !P.CampaignId.IsValid() || Ids.Contains(P.SaveId) || S.Map.IsEmpty()
            || !FMath::IsFinite(S.ActiveSeconds) || S.ActiveSeconds < 0 || S.KnowledgeRevision != S.Knowledge.Num()
            || S.AutoMinutes < 1 || S.AutoMinutes > 60 || !S.Safety.CanSave()
            || !S.Player.IsValid() || !S.Companion.IsValid() || !S.Source.IsValid() || !S.Camp.IsValid() || S.View.ContainsNaN()
            || !ValidCounts(S.Resource, false)
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
    return Decode(Bytes,Out,Error);
}

bool HearthwardSave::Write(const FString& Path, UHearthwardSaveGame* Pool, FString& Error)
{
    if (!Pool || !Validate(*Pool)) { Error = TEXT("拒绝写入无效快照"); return false; }
    TArray<uint8> Payload, Bytes;
    if (!UGameplayStatics::SaveGameToMemory(Pool, Payload)) { Error = TEXT("快照序列化失败"); return false; }
    const uint32 Header[] = {0x48575338, uint32(Payload.Num()), FCrc::MemCrc32(Payload.GetData(), Payload.Num())};
    Bytes.Append(reinterpret_cast<const uint8*>(Header), sizeof(Header));
    Bytes.Append(Payload);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
    const FString Temp = Path + TEXT(".pending");
    if (!FFileHelper::SaveArrayToFile(Bytes, *Temp)) { Error = TEXT("临时存档写入失败，原档未替换"); return false; }
    UHearthwardSaveGame* Verified = nullptr;
    if (!Read(Temp, Verified, Error)) return false;
    if(IFileManager::Get().FileExists(*Path))
    {
        TArray<uint8> Original;if(!FFileHelper::LoadFileToArray(Original,*Path))return false;
        uint32 Previous=0;if(Original.Num()>=4)FMemory::Memcpy(&Previous,Original.GetData(),4);
        const FString Backup=Path+((Previous==0x48575336 || Previous==0x48575337)?TEXT(".pre-schema8"):TEXT(".pre-schema6"));
        if(Previous!=0x48575338 && !IFileManager::Get().FileExists(*Backup) && !FFileHelper::SaveArrayToFile(Original,*Backup))
        {Error=TEXT("无法保留旧存档备份，原档未替换");return false;}
    }
#if PLATFORM_WINDOWS
    const FString From = FPaths::ConvertRelativePathToFull(Temp), To = FPaths::ConvertRelativePathToFull(Path);
    if (MoveFileExW(*From, *To, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
#endif
    Error = TEXT("存档原子替换失败，原档未替换");
    return false;
}
