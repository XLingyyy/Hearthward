#include "HearthwardAgentContract.h"
#include "../Inventory/HearthwardInventoryState.h"
#include "../Gameplay/HearthwardGameData.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
TSharedPtr<FJsonObject> PolicyData()
{
    static TSharedPtr<FJsonObject> Data;
    if(!Data)
    {
        FString Text; FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("config/npc-agent.policy.json")));
        checkf(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Data) && Data,TEXT("Missing NPC policy"));
    }
    return Data;
}
TArray<TSharedPtr<FJsonValue>> Strings(const TArray<FString>& Values)
{ TArray<TSharedPtr<FJsonValue>> Out; for(const auto& V:Values) Out.Add(MakeShared<FJsonValueString>(V)); return Out; }
TSharedPtr<FJsonObject> Enum(const TArray<FString>& Values)
{ auto O=MakeShared<FJsonObject>(); O->SetStringField(TEXT("type"),TEXT("string")); O->SetArrayField(TEXT("enum"),Strings(Values)); return O; }
bool StringArray(const TSharedPtr<FJsonObject>& O,const TCHAR* Key,TArray<FString>& Out,int32 Max,int32 Length)
{
    const TArray<TSharedPtr<FJsonValue>>* A;
    if(!O->TryGetArrayField(Key,A) || A->Num()>Max) return false;
    for(const auto& V:*A) { FString S; if(!V->TryGetString(S) || S.IsEmpty() || S.Len()>Length) return false; Out.Add(S); }
    return true;
}
}
int32 HearthwardAgent::Policy(const TCHAR* Key) { return PolicyData()->GetIntegerField(Key); }
bool HearthwardAgent::Settle(TArray<FHearthwardAgentReceipt>& Receipts,FGuid Id,FGuid Command,
    FGuid Epoch,FGuid CurrentEpoch,const FString& Payload,TFunctionRef<bool()> Effect)
{
    if(!Id.IsValid() || !Command.IsValid() || Epoch!=CurrentEpoch)return false;
    if(const auto* R=Receipts.FindByPredicate([&](const auto& X){return X.Id==Id;}))
        return R->Command==Command && R->Payload==Payload;
    if(!Effect())return false;
    FHearthwardAgentReceipt R;R.Id=Id;R.Command=Command;R.Payload=Payload;Receipts.Add(R);return true;
}
FString HearthwardAgent::Normalize(const FString& Text)
{
    FString Out=Text.ToLower();
    for(const auto& A:PolicyData()->GetObjectField(TEXT("aliases"))->Values) Out.ReplaceInline(*A.Key,*A.Value->AsString());
    return Out;
}
const TArray<FHearthwardAgentCapability>& HearthwardAgent::Capabilities()
{
    static const TArray<FHearthwardAgentCapability> C=[]
    {
        TArray<FName> All,Recipes,Repair;
        for(const auto& I:HearthwardBasicItems()) All.Add(I.Id);
        for(const auto& R:HearthwardData::Rows(TEXT("craftingRecipes"))) Recipes.Add(FName(*HearthwardData::Text(R->AsObject(),TEXT("id"))));
        for(const auto& R:HearthwardData::Rows(TEXT("repairRecipes"))) Repair.Add(FName(*HearthwardData::Text(R->AsObject(),TEXT("id"))));
        TArray<FHearthwardAgentCapability> Result={
            {TEXT("collect"),TEXT("采集木材→返营→入库；数量是新采集份数；S1为当前已知安全点"),{TEXT("wood")},Policy(TEXT("max_collect")),TEXT("additional_acquired"),{TEXT("S1")},{TEXT("ban"),TEXT("source")},true},
            {TEXT("craft"),TEXT("取得授权材料→到工作台制作→产物入库；quantity是批数；默认弟弟背包bag，明确授权才用camp仓库"),Recipes,Policy(TEXT("max_craft_batches")),TEXT("batches"),{TEXT("bag"),TEXT("camp")},{TEXT("no"),TEXT("max")},true},
            {TEXT("repair"),TEXT("到工作台修理自己背包中唯一一件装备；quantity=1；不操作玩家装备"),Repair,1,TEXT("one_owned"),{TEXT("bag"),TEXT("camp")},{TEXT("no"),TEXT("max")},true},
            {TEXT("companion_order"),TEXT("高层伙伴指令；hold原地等待，follow跟随玩家，assist在玩家附近协助有效威胁，routine恢复营地低权限自由活动；UE决定目标、导航、攻击时机和伤害"),{TEXT("hold"),TEXT("follow"),TEXT("assist"),TEXT("routine")},1,TEXT("directive"),{TEXT("player")},{},true},
            {TEXT("inventory"),TEXT("只读营地当前或已有belief库存；回复必须说明来源与是否亲自确认"),All,0,TEXT("none"),{TEXT("none")},{},false},
            {TEXT("inventory_report"),TEXT("玩家明确报告营地某物品当前数量；只更新弟弟的belief，不修改实际仓库；quantity为玩家报告的精确数量"),All,100000,TEXT("reported_exact"),{TEXT("player")},{},false},
            {TEXT("recall"),TEXT("只读有效原话和本人实际事件"),{TEXT("none")},0,TEXT("none"),{TEXT("none")},{},false},
            {TEXT("rule_proposal"),TEXT("提出长期规则卡，确认后生效；limits一条ban:wood或source:S1或no:物品或max:物品:整数"),{TEXT("none")},0,TEXT("none"),{TEXT("none")},{TEXT("ban"),TEXT("source"),TEXT("no"),TEXT("max")},false}
        };
        for(FName Id:{FName(TEXT("cancel")),FName(TEXT("clarify")),FName(TEXT("dialogue")),FName(TEXT("refuse"))})
            Result.Add({Id,TEXT("取消/澄清/闲聊/拒绝；无物品参数"),{TEXT("none")},0,TEXT("none"),{TEXT("none")},{},false});
        return Result;
    }();
    return C;
}
const FHearthwardAgentCapability* HearthwardAgent::FindCapability(FName Id)
{
    return Capabilities().FindByPredicate([&](const auto& C){return C.Id==Id;});
}

bool HearthwardAgent::IsCapabilityItem(FName Capability,FName Item)
{
    const auto* C=FindCapability(Capability);
    return C && C->Items.Contains(Item);
}

FString HearthwardAgent::CompanionOrderPrompt()
{
    const auto* C=FindCapability(TEXT("companion_order"));
    if(!C) return TEXT("companion_order当前未注册，不得输出。");
    TArray<FString> Items;for(FName Item:C->Items)Items.Add(Item.ToString());
    return TEXT("companion_order仅允许目录中的高层指令：")+FString::Join(Items,TEXT("/"))
        +TEXT("；quantity=1,mode=directive,source=player。模型不选择敌人、坐标、路径、攻击时机或伤害。");
}

FString HearthwardAgent::Describe()
{
    FString Out=TEXT("目录v2；只有这些已注册能力。站点或物资不足可暂时不可用；战斗仅允许companion_order高层指令，不能生成逐帧战术、建造或未知物品能力。\n");
    for(const auto& C:Capabilities())
    {
        TArray<FString> I; for(FName Id:C.Items) I.Add(Id.ToString());
        Out+=C.Id.ToString()+TEXT(": ")+C.Description+TEXT("；item=")+FString::Join(I,TEXT(","))+FString::Printf(TEXT("；quantity上限%d；mode="),C.MaxQuantity)+C.QuantityMode+TEXT("\n");
    }
    for(const auto& V:HearthwardData::Rows(TEXT("craftingRecipes")))
    {
        const auto R=V->AsObject();Out+=HearthwardData::Text(R,TEXT("id"))+TEXT(" 每批消耗");
        for(const auto& M:R->GetObjectField(TEXT("materials"))->Values)Out+=FString::Printf(TEXT(" %s:%d"),*M.Key,int32(M.Value->AsNumber()));
        Out+=TEXT("，每批产出");for(const auto& M:R->GetObjectField(TEXT("outputs"))->Values)Out+=FString::Printf(TEXT(" %s:%d"),*M.Key,int32(M.Value->AsNumber()));Out+=TEXT("\n");
    }
    Out+=TEXT("制作请求已明确批数和配方即可提卡。背包材料默认有使用权，无须额外确认；实际库存、配方扣料、到站距离由UE再校验，不能把这些自动检查列成玩家未解决问题。\n");
    return Out;
}
FString HearthwardAgent::Schema()
{
    auto Root=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Branches;
    for(const auto& C:Capabilities())
    {
        auto B=MakeShared<FJsonObject>(),P=MakeShared<FJsonObject>();B->SetStringField(TEXT("type"),TEXT("object"));B->SetBoolField(TEXT("additionalProperties"),false);
        P->SetObjectField(TEXT("intent"),Enum({C.Id.ToString()}));
        TArray<FString> Items;for(FName I:C.Items) Items.Add(I.ToString());P->SetObjectField(TEXT("item"),Enum(Items));
        auto Q=MakeShared<FJsonObject>();Q->SetStringField(TEXT("type"),TEXT("integer"));Q->SetNumberField(TEXT("minimum"),C.Writes?1:0);Q->SetNumberField(TEXT("maximum"),C.MaxQuantity);P->SetObjectField(TEXT("quantity"),Q);
        P->SetObjectField(TEXT("mode"),Enum({C.QuantityMode}));P->SetObjectField(TEXT("source"),Enum(C.Sources));
        for(const auto& Key:{TEXT("limits"),TEXT("unresolved")})
        {
            auto A=MakeShared<FJsonObject>(),S=MakeShared<FJsonObject>();A->SetStringField(TEXT("type"),TEXT("array"));A->SetNumberField(TEXT("maxItems"),4);S->SetStringField(TEXT("type"),TEXT("string"));S->SetNumberField(TEXT("maxLength"),120);A->SetObjectField(TEXT("items"),S);P->SetObjectField(Key,A);
        }
        auto Line=MakeShared<FJsonObject>();Line->SetStringField(TEXT("type"),TEXT("string"));Line->SetNumberField(TEXT("maxLength"),150);P->SetObjectField(TEXT("npc_line"),Line);
        B->SetObjectField(TEXT("properties"),P);B->SetArrayField(TEXT("required"),Strings({TEXT("intent"),TEXT("item"),TEXT("quantity"),TEXT("mode"),TEXT("source"),TEXT("limits"),TEXT("unresolved"),TEXT("npc_line")}));Branches.Add(MakeShared<FJsonValueObject>(B));
    }
    Root->SetArrayField(TEXT("oneOf"),Branches);FString Out;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Out));return Out;
}
bool HearthwardAgent::ValidLimit(const FString& Limit)
{
    TArray<FString> T;Limit.ParseIntoArray(T,TEXT(":"));
    if(T.Num()==2 && T[0]==TEXT("source")) return T[1]==TEXT("S1");
    if(T.Num()<2 || !HearthwardBasicItems().ContainsByPredicate([&](const auto& I){return I.Id==FName(*T[1]);})) return false;
    if(T.Num()==2) return T[0]==TEXT("ban") || T[0]==TEXT("no");
    if(T.Num()!=3 || T[0]!=TEXT("max") || !T[2].IsNumeric() || T[2].Contains(TEXT("."))) return false;
    int64 N=FCString::Atoi64(*T[2]);return N>=0 && N<=100000 && FString::Printf(TEXT("%lld"),N)==T[2];
}
FString HearthwardAgent::Validate(const FHearthwardAgentGoal& G)
{
    const auto* C=Capabilities().FindByPredicate([&](const auto& X){return X.Id==G.Intent;});
    if(!C || G.CapabilityVersion!=2 || !C->Items.Contains(G.Item)) return TEXT("UNSUPPORTED_CAPABILITY");
    if(G.Quantity<(C->Writes?1:0) || G.Quantity>C->MaxQuantity || G.QuantityMode!=C->QuantityMode || !C->Sources.Contains(G.SourceRef)) return TEXT("AMBIGUOUS_TARGET");
    if(G.Limits.Num()>4 || G.Unresolved.Num()>4) return TEXT("UNRESOLVED_CONSTRAINT");
    for(const auto& L:G.Limits)
    {
        FString Type,Rest;L.Split(TEXT(":"),&Type,&Rest);
        if(!ValidLimit(L) || !C->Constraints.Contains(Type)) return TEXT("UNRESOLVED_CONSTRAINT");
        if(Type==TEXT("ban") && Rest==G.Item.ToString() && G.Intent==TEXT("collect")) return TEXT("POLICY_CONFLICT");
    }
    if(C->Writes && !G.Unresolved.IsEmpty()) return TEXT("UNRESOLVED_CONSTRAINT");
    if(G.Intent==TEXT("rule_proposal") && (G.Limits.Num()!=1 || !G.Unresolved.IsEmpty())) return TEXT("UNRESOLVED_CONSTRAINT");
    return {};
}
bool HearthwardAgent::Parse(const FString& Json,FHearthwardAgentGoal& Out)
{
    TSharedPtr<FJsonObject> O;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),O) || !O || O->Values.Num()!=8) return false;
    FHearthwardAgentGoal G;FString Intent,Item;double Q;
    if(!O->TryGetStringField(TEXT("intent"),Intent) || !O->TryGetStringField(TEXT("item"),Item) || !O->TryGetNumberField(TEXT("quantity"),Q)
        || !FMath::IsFinite(Q) || Q<0 || Q>MAX_int32 || FMath::FloorToDouble(Q)!=Q
        || !O->TryGetStringField(TEXT("mode"),G.QuantityMode) || !O->TryGetStringField(TEXT("source"),G.SourceRef)
        || !O->TryGetStringField(TEXT("npc_line"),G.Line) || G.Line.Len()>150
        || !StringArray(O,TEXT("limits"),G.Limits,4,120) || !StringArray(O,TEXT("unresolved"),G.Unresolved,4,120)) return false;
    G.Intent=FName(*Intent);G.Item=FName(*Item);G.Quantity=int32(Q);
    // Semantic failures are preserved for a specific refusal, never silently repaired into an executable goal.
    if(!Capabilities().ContainsByPredicate([&](const auto& C){return C.Id==G.Intent;})) return false;
    Out=G;return true;
}
bool HearthwardAgent::AllowsCost(const TArray<FString>& Limits,const TMap<FName,int32>& Cost,const TMap<FName,int32>& Spent)
{
    for(const auto& L:Limits)
    {
        if(!ValidLimit(L)) return false;TArray<FString> T;L.ParseIntoArray(T,TEXT(":"));const FName Item(*T[1]);
        if(T[0]==TEXT("no") && Cost.FindRef(Item)>0) return false;
        if(T[0]==TEXT("max") && int64(Cost.FindRef(Item))+Spent.FindRef(Item)>FCString::Atoi64(*T[2])) return false;
    }
    return true;
}
FString HearthwardAgent::ItemText(FName Item)
{
    if(Item==TEXT("hold"))return TEXT("原地等待");
    if(Item==TEXT("follow"))return TEXT("跟随玩家");
    if(Item==TEXT("assist"))return TEXT("协助战斗");
    if(Item==TEXT("routine"))return TEXT("营地自由活动");
    if(const auto* I=HearthwardBasicItems().FindByPredicate([&](const auto& X){return X.Id==Item;}))return I->DisplayName.ToString();
    if(auto R=HearthwardData::Find(TEXT("craftingRecipes"),Item.ToString()))return HearthwardData::Text(R,TEXT("name"));
    return TEXT("未指定物品");
}
FString HearthwardAgent::LimitText(const FString& L)
{
    TArray<FString> T;L.ParseIntoArray(T,TEXT(":"));if(!ValidLimit(L))return L;
    if(T[0]==TEXT("source"))return TEXT("仅当前已知安全采集点");
    const FString Name=ItemText(FName(*T[1]));
    if(T[0]==TEXT("ban"))return TEXT("禁止采集")+Name;
    if(T[0]==TEXT("no"))return TEXT("不得消耗")+Name;
    return FString::Printf(TEXT("累计最多消耗%s %s份"),*Name,*T[2]);
}
FString HearthwardAgent::EventText(FName Kind)
{
    if(Kind==TEXT("collect"))return TEXT("采集");
    if(Kind==TEXT("acquired"))return TEXT("实际采集");if(Kind==TEXT("delivered"))return TEXT("实际入库");
    if(Kind==TEXT("craft"))return TEXT("完成制作");if(Kind==TEXT("repair"))return TEXT("完成维修");
    if(Kind==TEXT("completed"))return TEXT("任务完成");if(Kind==TEXT("materials_taken"))return TEXT("按授权领取材料");
    if(Kind==TEXT("cancelled"))return TEXT("任务取消");return TEXT("任务受阻");
}
FString HearthwardAgent::GoalText(const FHearthwardAgentGoal& G)
{
    if(G.Intent.IsNone())return TEXT("暂无待补充任务");
    const FString Name=ItemText(G.Item);
    FString Action=G.Intent==TEXT("collect")?TEXT("新采集"):G.Intent==TEXT("craft")?TEXT("制作"):G.Intent==TEXT("repair")?TEXT("维修自己的"):G.Intent==TEXT("inventory_report")?TEXT("玩家报告库存"):TEXT("新增长期规则");
    FString Unit=G.Intent==TEXT("craft")?TEXT("批"):G.Intent==TEXT("repair")?TEXT("件"):TEXT("份");
    FString Text=FString::Printf(TEXT("%s %s × %d %s\n来源：%s；目的地：营地仓库"),*Action,*Name,G.Quantity,*Unit,G.SourceRef==TEXT("camp")?TEXT("授权共享仓库材料"):G.SourceRef==TEXT("bag")?TEXT("弟弟背包"):TEXT("当前安全采集点"));
    if(G.Intent==TEXT("repair")) Text=FString::Printf(TEXT("维修弟弟自己的 %s × 1 件；保留在弟弟背包\n材料：%s"),*Name,G.SourceRef==TEXT("camp")?TEXT("授权共享仓库"):TEXT("弟弟背包"));
    if(G.Intent==TEXT("companion_order"))
        Text=FString::Printf(TEXT("伙伴高层指令：%s\n战术目标、导航、攻击时机与伤害由UE按当前世界状态决定"),*Name);
    if(G.Intent==TEXT("inventory_report"))Text=FString::Printf(TEXT("玩家报告：营地仓库当前有 %d 份%s；仅更新弟弟认知，不改变实际仓库"),G.Quantity,*Name);
    if(G.Intent==TEXT("rule_proposal"))Text=TEXT("新增长期规则；确认后影响新接受的任务");
    TArray<FString> Labels;for(const auto& L:G.Limits)Labels.Add(LimitText(L));
    if(!Labels.IsEmpty())Text+=TEXT("\n限制：")+FString::Join(Labels,TEXT("、"));
    if(!G.Unresolved.IsEmpty())Text+=TEXT("\n待补充：")+FString::Join(G.Unresolved,TEXT("、"));
    return Text;
}
