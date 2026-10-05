#include "HearthwardSaveGame.h"
#include "HearthwardSaveCompatibility.h"
#include "../Update/HearthwardUpdateSubsystem.h"
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
#include "../Time/HearthwardClockState.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

bool FHearthwardWorldSave::Serialize(FArchive& Ar)
{
    // An absent version must stay absent when loading historical tagged properties.
    // nullptr defaults writes the clock metadata even when it equals today's defaults.
    if(Ar.IsLoading()){ClockVersion=0;InitialDay=0;InitialMinute=-1;CommandRevision=0;}
    StaticStruct()->SerializeTaggedProperties(Ar,reinterpret_cast<uint8*>(this),StaticStruct(),nullptr);
    return true;
}
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
    for(double Value:{State.SevereDue,State.HotRemaining,State.HotRate,State.RecoveryDelay,State.SafeSeconds,State.MedicineRemaining,State.FoodRemaining})
        if(!FMath::IsFinite(Value)) return false;
    if((State.SevereDue!=-1 && State.SevereDue<=Calendar) || State.HotRemaining<0 || State.HotRemaining>15 || State.HotRate<0
        || State.RecoveryDelay<0 || State.RecoveryDelay>.5 || State.SafeSeconds<0 || State.MedicineRemaining<0 || State.MedicineRemaining>3 || State.FoodRemaining<0 || State.FoodRemaining>3) return false;
    if(State.Medicine.IsNone()!= (State.MedicineRemaining==0)) return false;
    if(!State.Medicine.IsNone() && (Bag.FindRef(State.Medicine)<1 || HearthwardData::Number(HearthwardData::Find(TEXT("items"),State.Medicine.ToString()),TEXT("healing"))<=0)) return false;
    if(State.FoodItem.IsNone()!=(State.FoodRemaining==0) || (!State.Medicine.IsNone() && !State.FoodItem.IsNone())) return false;
    if(!State.FoodItem.IsNone() && (Bag.FindRef(State.FoodItem)<1 || HearthwardData::Number(HearthwardData::Find(TEXT("items"),State.FoodItem.ToString()),TEXT("food"))<=0)) return false;
    if(!State.HotItem.IsNone() && (State.HotRemaining<=0 || HearthwardData::Number(HearthwardData::Find(TEXT("items"),State.HotItem.ToString()),TEXT("healing"))<=0)) return false;
    for(FName Id:State.AutoPermissions) if(!HearthwardData::Find(TEXT("items"),Id.ToString())) return false;
    return true;
}
bool Decode(const TArray<uint8>& Bytes, UHearthwardSaveGame*& Out, FString& Error)
{
    constexpr uint32 LegacyMagic = 0x48575331, PreviousMagic = 0x48575332, Schema5Magic = 0x48575335, Schema6Magic = 0x48575336, Schema7Magic = 0x48575337, Schema8Magic=0x48575338, Magic = 0x48575339;
    uint32 Header[3] = {};
    if (Bytes.Num() < sizeof(Header) || Bytes.Num() > 64 * 1024 * 1024) { Error = TEXT("存档大小无效"); return false; }
    FMemory::Memcpy(Header, Bytes.GetData(), sizeof(Header));
    const int32 Length = Bytes.Num() - sizeof(Header);
    if (Header[1] != uint32(Length) || Header[2] != FCrc::MemCrc32(Bytes.GetData() + sizeof(Header), Length))
    { Error = TEXT("存档完整性校验失败"); return false; }
    if(Header[0]!=Magic && Header[0]!=Schema8Magic && Header[0]!=Schema7Magic && Header[0]!=Schema6Magic && Header[0]!=Schema5Magic && Header[0]!=LegacyMagic && Header[0]!=PreviousMagic)
    {Error=TEXT("此存档格式不受当前版本支持。请同步更新；不要删除进度，原档已保留。");return false;}
    TArray<uint8> Payload;
    Payload.Append(Bytes.GetData() + sizeof(Header), Length);
    Out = Cast<UHearthwardSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
    if(Out && HearthwardVersion::IsNewer(Out->WriterVersion,HearthwardVersion::Current))
    {Error=FString::Printf(TEXT("此进度由较新版本 %s 保存，请同步更新后继续；原档已保留。"),*Out->WriterVersion);Out=nullptr;return false;}

    // A missing historical Schema property loads the current class default; explicit older versions must match the envelope.
    if(Out && Out->Schema!=HearthwardSave::CurrentSchema
        && !((Header[0]==LegacyMagic && Out->Schema==1)
            || (Header[0]==PreviousMagic && (Out->Schema==2 || Out->Schema==3))
            || (Header[0]==Schema5Magic && Out->Schema==5)
            || (Header[0]==Schema6Magic && Out->Schema==6)
            || (Header[0]==Schema7Magic && Out->Schema==7)
            || (Header[0]==Schema8Magic && Out->Schema==8)))
    {Error=TEXT("存档封装与数据版本不一致，原档已保留；请同步更新或恢复备份");Out=nullptr;return false;}

    if(Out && Header[0]!=Magic && Out->Schema==HearthwardSave::CurrentSchema
        && Out->Points.ContainsByPredicate([](const auto& P){return P.World.ClockVersion!=0 || P.World.InitialDay!=0 || P.World.InitialMinute!=-1;}))
    {Error=TEXT("存档封装与时间格式不一致，原档已保留；请同步更新或恢复备份");Out=nullptr;return false;}
    if(Out && Header[0]==Schema8Magic && Out->Schema==HearthwardSave::CurrentSchema)Out->Schema=8;
    if(Out && Header[0]==Schema7Magic && Out->Schema==HearthwardSave::CurrentSchema)Out->Schema=7;
    if(Out && Header[0]==Schema6Magic && Out->Schema==HearthwardSave::CurrentSchema)Out->Schema=6;
    if(Out && Header[0]==Schema5Magic && Out->Schema==HearthwardSave::CurrentSchema)Out->Schema=5;

    // Historical files may omit a property that matched the class default of that build.
    // Only the legacy envelope may use this fallback; current-format damaged files never enter it.
    if(Out && Header[0]==LegacyMagic && Out->Schema==HearthwardSave::CurrentSchema
        && Out->Points.ContainsByPredicate([](const auto& P){return P.World.NPCStateVersion==0;}))
        Out->Schema=1;

    if(Out && Header[0]==PreviousMagic && Out->Schema==HearthwardSave::CurrentSchema
        && !Out->Points.ContainsByPredicate([](const auto& P){return P.World.SurvivalVersion!=0;}))
        Out->Schema=!Out->Points.IsEmpty() && !Out->Points.ContainsByPredicate([](const auto& P){return P.World.NPCStateVersion!=2;})?2:3;

    // Before schema 6, the camp was stored as gameplay origin/tier and building
    // actors. Preserve that restore path; absence is not a damaged economy blob.
    const bool LegacyCampFormat = Out && Header[0]!=Magic && Out->Schema>=1 && Out->Schema<=5;

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
            if(LegacyCampFormat && P.World.CampEconomy.IsEmpty())
            {
                TSharedPtr<FJsonObject> Gameplay;
                FVector Position=P.World.Player.GetLocation();
                if(!P.World.Gameplay.IsEmpty())
                {
                    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(P.World.Gameplay),Gameplay) || !Gameplay
                        || !Position.InitFromString(HearthwardData::Text(Gameplay,TEXT("origin"))))
                    {Error=TEXT("旧营地位置无效，原文件已保留");Out=nullptr;return false;}
                    Position+=HearthwardData::Position(HearthwardData::Find(TEXT("locations"),TEXT("camp")));
                    Camp.Tier=int32(HearthwardData::Number(Gameplay,TEXT("campTier"),1));
                }
                Camp.Calendar=P.World.CalendarMinutes;
                Camp.AddCamp(TEXT("camp"),Position);
                if(!Camp.Validate()){Error=FString::Printf(TEXT("存档点 %s 的旧营地等级、位置或世界时间不符合当前规则；原档已保留，请更新或恢复备份。"),*P.SaveId.ToString());Out=nullptr;return false;}
                // Keep CampEconomy absent: Restore rebuilds legacy facilities from
                // their saved actors with zero paid cost, without inventing refunds.
            }
            else if(!FHearthwardCampState::Parse(P.World.CampEconomy,Camp) || Camp.Camps.IsEmpty())
            {Error=FString::Printf(TEXT("存档点 %s 的营地经济记录缺失或损坏，无法安全恢复营地、设施与生产队列；原档已保留，请更新或恢复备份。"),*P.SaveId.ToString());Out=nullptr;return false;}
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
    if(Out && Header[0]!=Magic && Out->Schema==8)
    {
        for(auto& P:Out->Points)
        {
            auto& S=P.World;
            S.ClockVersion=1;S.InitialDay=1;S.InitialMinute=0;
            FHearthwardCampaignState Campaign;
            if(!FHearthwardCampaignState::Parse(S.Campaign,Campaign))
            {Error=TEXT("旧档战役记录损坏，原档已保留");Out=nullptr;return false;}
            for(auto& E:Campaign.Enemies)if(E.Group==TEXT("field") && E.RefreshDue>=0)
            {
                const double DeathW=E.RefreshDue-5760;
                if(!FMath::IsFinite(DeathW) || DeathW<0 || DeathW>S.CalendarMinutes)
                {Error=TEXT("旧档野外敌人死亡时间无法确定，原到期记录已保留；请更新或恢复备份");Out=nullptr;return false;}
                E.RefreshDue=FHearthwardClockState::FieldRefreshDue(DeathW);
            }
            if(!S.Campaign.IsEmpty())S.Campaign=Campaign.Snapshot();
        }
        Out->Schema=9;
    }
    if (!Out || !HearthwardSave::Validate(*Out)) { Error = Out?HearthwardSave::Diagnose(*Out):TEXT("存档无法解析，请恢复备份；原档已保留。"); Out = nullptr; return false; }
    return true;
}
}

bool HearthwardSave::ResolveCommandRevision(const FHearthwardWorldSave& S,int64& Revision)
{
    Revision=S.CommandRevision;
    if(Revision<0)return false;
    for(const auto& Receipt:S.NPCReceipts)
    {
        if(Receipt.Command!=S.CommandId)continue;
        bool Versioned=false;
        for(const TCHAR* Prefix:{TEXT("take:"),TEXT("craft:"),TEXT("repair:"),TEXT("deposit:"),TEXT("withdraw:"),TEXT("handoff:")})
            if(Receipt.Payload.StartsWith(Prefix,ESearchCase::CaseSensitive)){Versioned=true;break;}
        if(!Versioned)continue;
        int32 Separator;
        if(!Receipt.Payload.FindLastChar(TEXT(':'),Separator))return false;
        const FString Suffix=Receipt.Payload.Mid(Separator+1);
        if(!Suffix.StartsWith(TEXT("r"),ESearchCase::CaseSensitive))return false;
        const FString Number=Suffix.Mid(1);
        const int64 Value=FCString::Strtoi64(*Number,nullptr,10);
        if(Value<=0 || FString::Printf(TEXT("%lld"),Value)!=Number || (Revision!=0 && Revision!=Value))return false;
        Revision=Value;
    }
    if(Revision==0)Revision=1;
    return true;
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
        if(S.ClockVersion!=1 || S.InitialDay<1 || !FMath::IsFinite(S.InitialMinute) || S.InitialMinute<0 || S.InitialMinute>=1440
            || S.SurvivalVersion!=1 || !FMath::IsFinite(S.CalendarMinutes) || S.CalendarMinutes<S.ActiveSeconds
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
        int64 CommandRevision;if(!ResolveCommandRevision(S,CommandRevision))return false;
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
        if (!S.NPCMemory.IsValid(S.ActiveSeconds)
            || (S.NPCMemory.ConversationClockStarted && S.NPCMemory.LastConversationCalendar>S.CalendarMinutes)) return false;
        if(!UHearthwardGameplayComponent::ValidateSnapshot(S.Gameplay) || !UHearthwardHarvestSubsystem::Validate(S.HarvestedResources)) return false;
        if (!P.SaveId.IsValid() || !P.CampaignId.IsValid() || Ids.Contains(P.SaveId) || S.Map.IsEmpty()
            || !FMath::IsFinite(S.ActiveSeconds) || S.ActiveSeconds < 0 || S.KnowledgeRevision != S.Knowledge.Num()
            || S.AutoMinutes < 1 || S.AutoMinutes > 60 || !S.Safety.CanSave()
            || !S.Player.IsValid() || !S.Companion.IsValid() || !S.Source.IsValid() || !S.Camp.IsValid() || S.View.ContainsNaN()
            || !ValidCounts(S.Resource, false)
            || !ValidTimer(S.PlayerTimer) || !ValidTimer(S.CompanionTimer)
            || uint8(S.Phase) > uint8(EHearthwardCompanionPhase::CampBatchWorking)
            || S.Requested < 0 || S.Delivered < 0 || S.Delivered > S.Requested) return false;
        if (S.Requested > 0 && !HearthwardBasicItems().ContainsByPredicate([&S](const auto& I) { return I.Id == S.Item; })
            && !(S.AgentGoal.Item==S.Item && ((S.AgentGoal.Intent==TEXT("nature_care") && HearthwardAgent::IsCapabilityItem(TEXT("nature_care"),S.Item))
                || (S.AgentGoal.Intent==TEXT("escort") && HearthwardAgent::IsCapabilityItem(TEXT("escort"),S.Item))
                || (S.AgentGoal.Intent==TEXT("hunt") && HearthwardAgent::IsCapabilityItem(TEXT("hunt"),S.Item))
                || (S.AgentGoal.Intent==TEXT("fish") && HearthwardAgent::IsCapabilityItem(TEXT("fish"),S.Item))
                || (S.AgentGoal.Intent==TEXT("capture") && HearthwardAgent::IsCapabilityItem(TEXT("capture"),S.Item))
                || (S.AgentGoal.Intent==TEXT("camp_batch") && HearthwardAgent::IsCapabilityItem(TEXT("camp_batch"),S.Item))))) return false;
        if (S.CommandActive && (S.Requested == 0 || S.Delivered == S.Requested)) return false;
        const bool Executing = S.Phase == EHearthwardCompanionPhase::GoingToSource || S.Phase == EHearthwardCompanionPhase::Gathering
            || S.Phase == EHearthwardCompanionPhase::Returning || S.Phase == EHearthwardCompanionPhase::ReturningBlocked
            || S.Phase == EHearthwardCompanionPhase::GoingToWorkshop || S.Phase == EHearthwardCompanionPhase::TakingMaterials || S.Phase == EHearthwardCompanionPhase::HoldingSafely
            || S.Phase == EHearthwardCompanionPhase::TakingCargo || S.Phase == EHearthwardCompanionPhase::GoingToPlayer || S.Phase == EHearthwardCompanionPhase::HandingOff
            || S.Phase == EHearthwardCompanionPhase::LeadingAnimal || S.Phase == EHearthwardCompanionPhase::CampBatchWorking;
        if (Executing && !S.CommandActive) return false;
        if(S.CampBatchBaseline< -1 || (S.Phase==EHearthwardCompanionPhase::CampBatchWorking && S.CampBatchBaseline<0))return false;
        if (S.Phase == EHearthwardCompanionPhase::Completed && (S.Requested == 0 || S.Delivered != S.Requested || S.CommandActive)) return false;
        if (S.Phase == EHearthwardCompanionPhase::Gathering && S.CompanionTimer.Status != EHearthwardTimedActionStatus::Running
            && S.CompanionTimer.Status != EHearthwardTimedActionStatus::Completed) return false;
        Ids.Add(P.SaveId);
    }
    return true;
}

bool HearthwardSave::Read(const FString& Path, UHearthwardSaveGame*& Out, FString& Error)
{
    Out=nullptr;
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path)) { Error = TEXT("无法读取存档池"); return false; }
    return Decode(Bytes,Out,Error);
}
bool HearthwardSave::ReadBytes(const TArray<uint8>& Bytes,UHearthwardSaveGame*& Out,FString& Error)
{ Out=nullptr;return Decode(Bytes,Out,Error); }

bool HearthwardSave::Write(const FString& Path, UHearthwardSaveGame* Pool, FString& Error)
{
    if (!Pool || !Validate(*Pool)) { Error = TEXT("拒绝写入无效快照"); return false; }
    if(HearthwardVersion::IsNewer(Pool->WriterVersion,HearthwardVersion::Current))
    {Error=TEXT("请同步更新后保存，当前程序不能覆盖较新版本进度。");return false;}
    Pool=DuplicateObject<UHearthwardSaveGame>(Pool,GetTransientPackage());
    Pool->WriterVersion=HearthwardVersion::Current;
    TArray<uint8> Payload, Bytes;
    if (!UGameplayStatics::SaveGameToMemory(Pool, Payload)) { Error = TEXT("快照序列化失败"); return false; }
    const uint32 Header[] = {0x48575339, uint32(Payload.Num()), FCrc::MemCrc32(Payload.GetData(), Payload.Num())};
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
        UHearthwardSaveGame* OriginalPool=nullptr;
        if(Original.Num()>12)
        {
            TArray<uint8> OldPayload;OldPayload.Append(Original.GetData()+12,Original.Num()-12);
            OriginalPool=Cast<UHearthwardSaveGame>(UGameplayStatics::LoadGameFromMemory(OldPayload));
        }
        // One content-addressed backup on upgrade, not one new file per autosave.
        FString VersionBackup;
        if((Previous!=0x48575339 || !OriginalPool || OriginalPool->WriterVersion!=HearthwardVersion::Current)
            && !HearthwardSave::Backup(Path,VersionBackup,Error))return false;
        const FString Backup=Path+(Previous==0x48575338?TEXT(".pre-schema9"):((Previous==0x48575336 || Previous==0x48575337)?TEXT(".pre-schema8"):TEXT(".pre-schema6")));
        if(Previous!=0x48575339 && !IFileManager::Get().FileExists(*Backup) && !FFileHelper::SaveArrayToFile(Original,*Backup))
        {Error=TEXT("无法保留旧存档备份，原档未替换");return false;}
    }
#if PLATFORM_WINDOWS
    const FString From = FPaths::ConvertRelativePathToFull(Temp), To = FPaths::ConvertRelativePathToFull(Path);
    if (MoveFileExW(*From, *To, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
#endif
    Error = TEXT("存档原子替换失败，原档未替换");
    return false;
}
