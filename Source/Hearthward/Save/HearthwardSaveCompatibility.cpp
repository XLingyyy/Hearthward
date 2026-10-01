#include "HearthwardSaveCompatibility.h"
#include "HearthwardSaveGame.h"
#include "../Update/HearthwardUpdateSubsystem.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardProgression.h"
#include "../Camp/HearthwardCampState.h"
#include "../Nature/HearthwardNatureState.h"
#include "../Campaign/HearthwardCampaignState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Misc/Crc.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FString Hash(const TArray<uint8>& Bytes)
{ uint8 Digest[20];FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Digest);return BytesToHex(Digest,20); }
bool Raw(const TArray<uint8>& Bytes,UHearthwardSaveGame*& Pool,uint32& Magic,FString& Error)
{
    Pool=nullptr;
    if(Bytes.Num()<12 || Bytes.Num()>64*1024*1024){Error=TEXT("存档文件大小异常；请恢复备份，不能自动删除玩家进度。");return false;}
    uint32 H[3];FMemory::Memcpy(H,Bytes.GetData(),12);Magic=H[0];
    if(H[1]!=uint32(Bytes.Num()-12) || H[2]!=FCrc::MemCrc32(Bytes.GetData()+12,Bytes.Num()-12))
    {Error=TEXT("存档完整性校验失败。这是文件损坏，不是玩家操作冲突；请恢复备份。");return false;}
    if(Magic!=0x48575331 && Magic!=0x48575332 && Magic!=0x48575335 && Magic!=0x48575336 && Magic!=0x48575337 && Magic!=0x48575338)
    {Error=TEXT("此存档格式不受当前版本支持，请同步更新。原档保留，不可通过删除记录降级。");return false;}
    TArray<uint8> Payload;Payload.Append(Bytes.GetData()+12,Bytes.Num()-12);
    Pool=Cast<UHearthwardSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
    if(!Pool){Error=TEXT("无法解析存档数据，请恢复备份。原文件保留。");return false;}
    if(HearthwardVersion::IsNewer(Pool->WriterVersion,HearthwardVersion::Current))
    {Error=FString::Printf(TEXT("进度由较新版本 %s 保存。存在新版本，请同步更新；不删除进度。"),*Pool->WriterVersion);return false;}
    return true;
}
FString Label(const FHearthwardSavePoint& P)
{ return FString::Printf(TEXT("%s · %s · 节点%s"),*P.Created.ToString(TEXT("%Y-%m-%d %H:%M")),*P.Location,*P.SaveId.ToString().Left(8)); }
void Json(const TSharedPtr<FJsonObject>& Object,FString& Text)
{ Text.Reset();FJsonSerializer::Serialize(Object.ToSharedRef(),TJsonWriterFactory<>::Create(&Text)); }
}
bool HearthwardSave::Backup(const FString& Path,FString& BackupPath,FString& Error)
{
    TArray<uint8> Bytes,Verified;
    if(!FFileHelper::LoadFileToArray(Bytes,*Path)){Error=TEXT("无法读取原档以创建备份；未修改进度。");return false;}
    BackupPath=FPaths::GetPath(Path)/TEXT("Backups")/(FPaths::GetCleanFilename(Path)+TEXT(".")+Hash(Bytes)+TEXT(".hws"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(BackupPath),true);
    if(!IFileManager::Get().FileExists(*BackupPath) && !FFileHelper::SaveArrayToFile(Bytes,*BackupPath))
    {Error=TEXT("备份写入失败；未修改进度。");return false;}
    if(!FFileHelper::LoadFileToArray(Verified,*BackupPath) || Verified!=Bytes)
    {Error=TEXT("备份校验失败；未修改进度。");return false;}
    return true;
}
FString HearthwardSave::Diagnose(const UHearthwardSaveGame& Pool)
{
    for(const auto& P:Pool.Points)
    {
        const auto& S=P.World;TArray<FString> Areas;
        FHearthwardCampState Camp;FHearthwardNatureState Nature;FHearthwardCampaignState Campaign;
        if(!S.CampEconomy.IsEmpty() && (!FHearthwardCampState::Parse(S.CampEconomy,Camp) || !Camp.ValidateBuildings(S.Gameplay)
            || FMath::Abs(Camp.Calendar-S.CalendarMinutes)>1.e-4))Areas.Add(TEXT("营地设施、生产队列或时间线"));
        if(!FHearthwardNatureState::Parse(S.Nature,Nature))Areas.Add(TEXT("自然资源、种养或捕获记录"));
        if(!FHearthwardCampaignState::Parse(S.Campaign,Campaign))Areas.Add(TEXT("剧情、救援或据点记录"));
        if(!UHearthwardGameplayComponent::ValidateSnapshot(S.Gameplay))Areas.Add(TEXT("技能、配方、建筑或角色属性"));
        if(!FHearthwardInventoryState::Validate(S.PlayerItems,false))Areas.Add(TEXT("玩家背包与装备"));
        if(!FHearthwardInventoryState::Validate(S.BrotherItems,false))Areas.Add(TEXT("弟弟背包与装备"));
        if(!FHearthwardInventoryState::Validate(S.StorageItems,true))Areas.Add(TEXT("共享仓储"));
        if(!S.NPCMemory.IsValid(S.ActiveSeconds))Areas.Add(TEXT("弟弟记忆与任务时间线"));
        if(!Areas.IsEmpty())return Label(P)+TEXT("：")+FString::Join(Areas,TEXT("、"))+TEXT("校验未通过。请查看兼容详情；原档保留。");
    }
    return TEXT("存档身份、任务状态或跨系统关联校验未通过。原档保留；无法安全推断应删除的内容，请更新或恢复备份。");
}
bool HearthwardSave::InspectCompatibility(const FString& Path,FHearthwardSaveCompatibility& Report,UHearthwardSaveGame*& Candidate)
{
    Report={};Report.SourcePath=Path;Candidate=nullptr;
    TArray<uint8> Bytes;
    if(!FFileHelper::LoadFileToArray(Bytes,*Path)){Report.Summary=TEXT("无法读取存档；请检查文件权限或恢复备份。");return false;}
    Report.SourceHash=Hash(Bytes);
    FString Error;
    if(ReadBytes(Bytes,Candidate,Error)){Report.Summary=TEXT("存档兼容。原始进度保留，首次保存前将备份旧档。");return true;}
    Candidate=nullptr;Report.Summary=Error;
    UHearthwardSaveGame* Work=nullptr;uint32 Magic=0;
    if(!Raw(Bytes,Work,Magic,Report.Summary))return false;
    for(auto& P:Work->Points)
    {
        auto& S=P.World;const FString Prefix=Label(P)+TEXT("\n");
        auto Change=[&](const FString& Description){Report.Changes.Add(Prefix+Description);};
        auto Counts=[&](TMap<FName,int32>& Values,const FString& Place)
        {
            TArray<FName> Remove;
            for(const auto& V:Values)if(!FHearthwardInventoryState::FindItem(V.Key))
            {Change(Place+TEXT("：物品 ")+V.Key.ToString()+FString::Printf(TEXT(" ×%d 已无定义，须删除此记录"),V.Value));Remove.Add(V.Key);}
            for(FName Id:Remove)Values.Remove(Id);
        };
        auto Inventory=[&](FHearthwardInventorySnapshot& I,const FString& Place)
        {
            Counts(I.Stacks,Place);
            for(int32 N=I.Instances.Num()-1;N>=0;--N)if(!FHearthwardInventoryState::FindItem(I.Instances[N].Definition))
            {
                const auto V=I.Instances[N];Change(Place+TEXT("：装备 ")+V.Definition.ToString()+TEXT("（")+V.Id.ToString()+TEXT("）已无定义，须删除并解除穿戴"));
                for(auto It=I.Equipped.CreateIterator();It;++It)if(It.Value()==V.Id)It.RemoveCurrent();I.Instances.RemoveAt(N);
            }
        };
        Counts(S.Inventory,TEXT("旧玩家背包"));Counts(S.Bag,TEXT("旧弟弟背包"));Counts(S.Storage,TEXT("旧共享仓储"));
        Inventory(S.PlayerItems,TEXT("玩家背包"));Inventory(S.BrotherItems,TEXT("弟弟背包"));Inventory(S.StorageItems,TEXT("共享仓储"));
        for(int32 N=S.GroundEquipment.Num()-1;N>=0;--N)if(!FHearthwardInventoryState::FindItem(S.GroundEquipment[N].Item.Definition))
        {Change(TEXT("地面装备：")+S.GroundEquipment[N].Item.Definition.ToString()+TEXT(" @ ")+S.GroundEquipment[N].Transform.GetLocation().ToString()+TEXT(" 已无定义，须删除"));S.GroundEquipment.RemoveAt(N);}
        TSharedPtr<FJsonObject> G,Camp;
        if(!S.CampEconomy.IsEmpty())FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S.CampEconomy),Camp);
        if(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S.Gameplay),G) && G)
        {
            bool Changed=false;TSet<FString> RemovedBuildings;
            const double Experience=HearthwardData::Number(G,TEXT("experience"));
            if(FMath::IsFinite(Experience) && Experience>HearthwardProgression::MaximumExperience())
            {
                Change(FString::Printf(TEXT("角色经验 %.0f 超出新版上限 %d，须删除超出的 %.0f 点；上限内经验保留"),Experience,HearthwardProgression::MaximumExperience(),Experience-HearthwardProgression::MaximumExperience()));
                G->SetNumberField(TEXT("experience"),HearthwardProgression::MaximumExperience());Changed=true;
            }
            const TArray<TSharedPtr<FJsonValue>>* Rows;
            if(G->TryGetArrayField(TEXT("buildings"),Rows))
            {
                auto Buildings=*Rows;
                Buildings.RemoveAll([&](const auto& V)
                {
                    if(!V || V->Type!=EJson::Object)return false;
                    auto B=V->AsObject();const FString Kind=HearthwardData::Text(B,TEXT("recipe"));
                    if(HearthwardData::Find(TEXT("buildings"),Kind))return false;
                    const FString Id=HearthwardData::Text(B,TEXT("id"));RemovedBuildings.Add(Id);
                    Change(TEXT("已建建筑 ")+Kind+TEXT("（")+Id+TEXT("）@ ")+HearthwardData::Text(B,TEXT("position"))+TEXT(" 已无定义；须删除建筑及关联生产队列，队列已投入材料不返还"));return Changed=true;
                });G->SetArrayField(TEXT("buildings"),Buildings);
            }
            if(Camp && !RemovedBuildings.IsEmpty())
            {
                for(const auto* Field:{TEXT("facilities"),TEXT("regions")})if(Camp->TryGetArrayField(Field,Rows))
                {
                    auto Kept=*Rows;Kept.RemoveAll([&](const auto& V){return V && V->Type==EJson::Object && RemovedBuildings.Contains(HearthwardData::Text(V->AsObject(),FCString::Strcmp(Field,TEXT("facilities"))==0?TEXT("id"):TEXT("facility")));});Camp->SetArrayField(Field,Kept);
                }
                Json(Camp,S.CampEconomy);
            }
            const TSharedPtr<FJsonObject>* Skills;
            if(G->TryGetObjectField(TEXT("skills"),Skills))
            {
                bool Again=true;
                while(Again)
                {
                    Again=false;TArray<FString> Remove;
                    for(const auto& Skill:(*Skills)->Values)
                    {
                        const FString SkillId(*Skill.Key);
                        const auto Def=HearthwardData::Find(TEXT("skills"),SkillId);double Rank=0;
                        const FString Parent=Def?HearthwardData::Text(Def,TEXT("requires")):FString();
                        if(!Def || (Skill.Value->TryGetNumber(Rank) && (Rank>HearthwardData::Number(Def,TEXT("maxRank"))
                            || (Rank>0 && !Parent.IsEmpty() && HearthwardData::Number(*Skills,Parent)<=0))))
                        {Change(TEXT("已学技能 ")+SkillId+TEXT(" 的定义、等级或前置技能与新版冲突，须删除该技能记录（经验保留）"));Remove.Add(SkillId);}
                    }
                    for(const auto& Key:Remove){(*Skills)->RemoveField(Key);Again=Changed=true;}
                }
            }
            if(G->TryGetArrayField(TEXT("knownRecipes"),Rows))
            {
                auto Kept=*Rows;Kept.RemoveAll([&](const auto& V)
                {FString Id;if(!V || !V->TryGetString(Id) || HearthwardData::Find(TEXT("craftingRecipes"),Id))return false;
                    Change(TEXT("已解锁配方 ")+Id+TEXT(" 已无定义，须删除此解锁记录"));return Changed=true;});G->SetArrayField(TEXT("knownRecipes"),Kept);
            }
            if(Changed)Json(G,S.Gameplay);
        }
    }
    if(Report.Changes.IsEmpty())
    {Report.Changes.Add(Report.Summary+TEXT("\n无法确定可安全删除的具体记录。请同步更新或恢复备份，不会自动清空进度。"));return false;}
    TArray<uint8> Payload,Trial;
    if(!UGameplayStatics::SaveGameToMemory(Work,Payload))return false;
    const uint32 Header[]={Magic,uint32(Payload.Num()),FCrc::MemCrc32(Payload.GetData(),Payload.Num())};
    Trial.Append(reinterpret_cast<const uint8*>(Header),12);Trial.Append(Payload);
    Report.CanRepair=ReadBytes(Trial,Candidate,Error);
    Report.Changes.Sort();
    if(!Report.CanRepair){Report.Summary=TEXT("仍有无法安全处理的关联冲突：")+Error+TEXT("。未允许删除，请更新或恢复备份。");Candidate=nullptr;}
    else Report.Summary=TEXT("若要在当前版本继续，必须删除下列冲突记录；其他兼容进度保留。可返回并等待支持旧记录的新版本。");
    return false;
}
bool HearthwardSave::ResolveCompatibility(const FHearthwardSaveCompatibility& Preview,const FString& Destination,FString& Status)
{
    FHearthwardSaveCompatibility Now;UHearthwardSaveGame* Candidate=nullptr;
    InspectCompatibility(Preview.SourcePath,Now,Candidate);
    if(!Preview.CanRepair || !Now.CanRepair || !Candidate || Now.SourceHash!=Preview.SourceHash || Now.Changes!=Preview.Changes)
    {Status=TEXT("存档或冲突清单已变化，请重新检查；未修改进度。");return false;}
    if(Destination!=Preview.SourcePath && IFileManager::Get().FileExists(*Destination))
    {Status=TEXT("新版存档已由其他会话创建，请重新检查；未覆盖已有进度。");return false;}
    FString BackupPath;if(!Backup(Preview.SourcePath,BackupPath,Status))return false;
    if(!Write(Destination,Candidate,Status))return false;
    Status=TEXT("已备份原档并删除确认的冲突记录；兼容进度已保留。");return true;
}
