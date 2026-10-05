#include "HearthwardAgentContract.h"
#include "../Nature/HearthwardNatureState.h"
#include "../Camp/HearthwardCampState.h"
#include "../Campaign/HearthwardCampaignState.h"
#include "../Inventory/HearthwardInventoryState.h"
#include "../Gameplay/HearthwardGameData.h"
#include "Dom/JsonObject.h"
#include "Internationalization/Regex.h"
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
        if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Data) || !Data)
            UE_LOG(LogTemp,Fatal,TEXT("Missing NPC policy"));
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
bool OriginalMaterialBudgetsMatch(const FHearthwardAgentGoal& Goal)
{
    const FString Text=HearthwardAgent::Normalize(Goal.Original);
    FRegexMatcher Budget(FRegexPattern(TEXT("(最多|至多|不超过)([^，,。；;\\n]*?)(?=最多|至多|不超过|[，,。；;\\n]|$)")),Text);
    while(Budget.FindNext())
    {
        int32 Start=Budget.GetMatchBeginning();
        while(Start>0 && !FString(TEXT("，,。；;\n")).Contains(Text.Mid(Start-1,1)))--Start;
        FString Prefix=Text.Mid(Start,Budget.GetMatchBeginning()-Start).TrimEnd();
        const FString Tail=Budget.GetCaptureGroup(2);
        if(!Prefix.Contains(TEXT("耗")) && !Prefix.Contains(TEXT("用")) && !Tail.Contains(TEXT("耗")) && !Tail.Contains(TEXT("用")))continue;
        Prefix.ReplaceInline(TEXT("消耗"),TEXT(""));Prefix.ReplaceInline(TEXT("使用"),TEXT(""));Prefix.ReplaceInline(TEXT("耗"),TEXT(""));Prefix.ReplaceInline(TEXT("用"),TEXT(""));Prefix.TrimEndInline();
        bool TailHasItem=false;
        for(const auto& Item:HearthwardBasicItems())
            if(Tail.Contains(HearthwardAgent::Normalize(HearthwardAgent::ItemText(Item.Id))) || Tail.Contains(Item.Id.ToString())){TailHasItem=true;break;}
        FRegexMatcher Number(FRegexPattern(TEXT("(?<![-−负.．点0-9零〇一二两三四五六七八九十百千万亿])([0-9]+|[零〇一二两三四五六七八九十百千万亿]+)(?![.．点0-9零〇一二两三四五六七八九十百千万亿])\\s*(份|个|根|单位|块|件)")),Tail);
        bool Found=false;TSet<FName> BoundItems;
        while(Number.FindNext())
        {
            FString Before=Tail.Left(Number.GetMatchBeginning()).TrimEnd();
            Before.ReplaceInline(TEXT("消耗"),TEXT(""));Before.ReplaceInline(TEXT("使用"),TEXT(""));Before.ReplaceInline(TEXT("耗"),TEXT(""));Before.ReplaceInline(TEXT("用"),TEXT(""));Before.TrimEndInline();
            const FString After=Tail.Mid(Number.GetMatchEnding()).TrimStart();
            FName Material;int32 BestLength=0;bool Ambiguous=false;
            for(const auto& Item:HearthwardBasicItems())
            {
                const FString Name=HearthwardAgent::Normalize(HearthwardAgent::ItemText(Item.Id)),Id=Item.Id.ToString();
                const int32 Length=FMath::Max(After.StartsWith(Name) || Before.EndsWith(Name) || (!TailHasItem && Prefix.EndsWith(Name))?Name.Len():0,
                    After.StartsWith(Id) || Before.EndsWith(Id) || (!TailHasItem && Prefix.EndsWith(Id))?Id.Len():0);
                if(Length>BestLength){Material=Item.Id;BestLength=Length;Ambiguous=false;}
                else if(Length>0 && Length==BestLength && Material!=Item.Id)Ambiguous=true;
            }
            if(Material.IsNone() || Ambiguous || BoundItems.Contains(Material))return false;
            BoundItems.Add(Material);bool Matches=false;
            for(const auto& Limit:Goal.Limits)
            {
                TArray<FString> Parts;Limit.ParseIntoArray(Parts,TEXT(":"));
                if(Parts.Num()!=3 || Parts[0]!=TEXT("max") || Parts[1]!=Material.ToString())continue;
                FHearthwardAgentGoal Amount;Amount.Intent=TEXT("collect");Amount.Item=Material;Amount.Quantity=FCString::Atoi(*Parts[2]);
                Amount.Original=TEXT("数量：")+Number.GetCaptureGroup(1)+Number.GetCaptureGroup(2)+HearthwardAgent::ItemText(Material);
                if(HearthwardAgent::OriginalQuantityMatches(Amount)){Matches=true;break;}
            }
            if(!Matches)return false;
            Found=true;
        }
        if(!Found)return false;
    }
    return true;
}
bool OriginalMaterialPermissionsMatch(const FHearthwardAgentGoal& Goal)
{
    const FString Text=HearthwardAgent::Normalize(Goal.Original);
    const auto BindMaterials=[](FString Tail,TSet<FName>& Items)
    {
        for(;;)
        {
            Tail.TrimStartInline();FName Material;int32 BestLength=0;bool Ambiguous=false;
            for(const auto& Item:HearthwardBasicItems())
            {
                const FString Name=HearthwardAgent::Normalize(HearthwardAgent::ItemText(Item.Id)),Id=Item.Id.ToString();int32 Length=0;
                for(const auto& Word:{Name,Id})
                {
                    if(Word.IsEmpty() || !Tail.StartsWith(Word))continue;
                    if(Word==Id && Tail.Len()>Word.Len() && Tail[Word.Len()]<128
                        && (FChar::IsAlnum(Tail[Word.Len()]) || Tail[Word.Len()]==TEXT('_')))continue;
                    Length=FMath::Max(Length,Word.Len());
                }
                if(Length>BestLength){Material=Item.Id;BestLength=Length;Ambiguous=false;}
                else if(Length>0 && Length==BestLength && Material!=Item.Id)Ambiguous=true;
            }
            if(Material.IsNone() || Ambiguous)return false;
            Items.Add(Material);Tail=Tail.Mid(BestLength).TrimStart();bool Joined=false;
            for(const auto* Join:{TEXT("和"),TEXT("与"),TEXT("及"),TEXT("、")})
                if(Tail.StartsWith(Join)){Tail=Tail.Mid(FCString::Strlen(Join));Joined=true;break;}
            if(!Joined)return Tail.IsEmpty();
        }
    };
    TSet<FName> Prohibited;
    FRegexMatcher No(FRegexPattern(TEXT("(?:(?:不允许|不得|禁止|不能|不要|不许|不准|别|不)\\s*(?:(?:仅)?(?:这次|本次)\\s*)?(?:消耗|使用|耗费|耗|用)|禁耗)\\s*([^，,。；;\\n]*?)(?=、(?:不得消耗|仅本次允许消耗|累计最多消耗)|[，,。；;\\n]|$)")),Text);
    while(No.FindNext())if(!BindMaterials(No.GetCaptureGroup(1),Prohibited))return false;
    for(FName Item:Prohibited)if(!Goal.Limits.Contains(TEXT("no:")+Item.ToString()))return false;
    if(Goal.Limits.ContainsByPredicate([](const auto& L){return L.StartsWith(TEXT("once:"));}))
    {
        TSet<FName> Authorized;
        FRegexMatcher Once(FRegexPattern(TEXT("(?:^|[，,。；;：:、\\n])\\s*(?:(?:仅)?(?:这次|本次)\\s*(?:允许|可以|可|准许|同意|授权)|(?:允许|准许|同意|授权)\\s*(?:仅)?(?:这次|本次))\\s*(?:消耗|使用|耗费|耗|用)\\s*([^，,。；;\\n]*?)(?=、(?:不得消耗|仅本次允许消耗|累计最多消耗)|[，,。；;\\n]|$)")),Text);
        while(Once.FindNext())
        {
            TSet<FName> Materials;
            if(BindMaterials(Once.GetCaptureGroup(1),Materials))for(FName Item:Materials)Authorized.Add(Item);
        }
        for(const auto& L:Goal.Limits)
            if(L.StartsWith(TEXT("once:")) && !Authorized.Contains(FName(*L.RightChop(5))))return false;
    }
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
bool HearthwardAgent::OriginalQuantityMatches(const FHearthwardAgentGoal& Goal)
{
    const FString Text=Normalize(Goal.Original),Item=Normalize(ItemText(Goal.Item));
    FString Chinese;
    if(Goal.Quantity>=0 && Goal.Quantity<=100000)
    {
        const FString Digits=TEXT("零一二三四五六七八九");
        const auto GroupText=[&](int32 Value,bool OmitLeadingOne)
        {
            FString Out;bool Zero=false;
            const int32 Places[]={1000,100,10,1};
            const TCHAR* Units[]={TEXT("千"),TEXT("百"),TEXT("十"),TEXT("")};
            for(int32 Index=0;Index<4;++Index)
            {
                const int32 Digit=Value/Places[Index]%10;
                if(Digit>0)
                {
                    if(Zero){Out+=TEXT("零");Zero=false;}
                    if(Digit!=1 || Places[Index]!=10 || !Out.IsEmpty() || !OmitLeadingOne)Out+=Digits.Mid(Digit,1);
                    Out+=Units[Index];
                }
                else if(!Out.IsEmpty() && Value%Places[Index]>0)Zero=true;
            }
            return Out;
        };
        if(Goal.Quantity==0)Chinese=TEXT("零");
        else if(Goal.Quantity>=10000)
        {
            Chinese=GroupText(Goal.Quantity/10000,true)+TEXT("万");
            const int32 Low=Goal.Quantity%10000;
            if(Low>0)Chinese+=(Low<1000?TEXT("零"):TEXT(""))+GroupText(Low,false);
        }
        else Chinese=GroupText(Goal.Quantity,true);
    }
    FRegexMatcher Number(FRegexPattern(TEXT("(数量[为是： ]*)?(?<![-−负.．点0-9零〇一二两三四五六七八九十百千万亿])([0-9]+|[零〇一二两三四五六七八九十百千万亿]+)(?![.．点0-9零〇一二两三四五六七八九十百千万亿])\\s*(份|个|根|单位|块|件|批)?")),Text);
    const int32 Supplement=Text.Find(TEXT("\n补充："),ESearchCase::CaseSensitive,ESearchDir::FromEnd);
    bool Found=false;
    while(Number.FindNext())
    {
        const FString Unit=Number.GetCaptureGroup(3);
        if(Goal.Intent==TEXT("craft")?Unit!=TEXT("批"):Unit==TEXT("批"))continue;
        const int32 Begin=Number.GetMatchBeginning(),End=Number.GetMatchEnding();
        int32 ClauseStart=Begin;
        while(ClauseStart>0 && !FString(TEXT("，,。；;\n")).Contains(Text.Mid(ClauseStart-1,1)))--ClauseStart;
        const FString Prefix=Text.Mid(ClauseStart,Begin-ClauseStart);
        if(Prefix.Contains(TEXT("耗")) || Prefix.Contains(TEXT("最多")) || Prefix.Contains(TEXT("至多")) || Prefix.Contains(TEXT("不超过")))continue;
        FString Before=Text.Left(Begin).TrimEnd();
        if(Before.EndsWith(TEXT("×")))Before=Before.LeftChop(1).TrimEnd();
        const FString After=Text.Mid(End).TrimStart();
        const bool Clarification=Supplement!=INDEX_NONE && Begin>=Supplement+4
            && Text.Mid(Supplement+4,Begin-Supplement-4).TrimStartAndEnd().IsEmpty()
            && (After.IsEmpty() || After==TEXT("。") || After==TEXT(".")) && Text.Left(Supplement).Contains(Item);
        if(Unit.IsEmpty() && Number.GetCaptureGroup(1).IsEmpty()
            && !(Clarification && Number.GetCaptureGroup(2).IsNumeric()))continue;
        if(Goal.Intent!=TEXT("craft") && !After.StartsWith(Item) && !After.StartsWith(Goal.Item.ToString())
            && !Before.EndsWith(Item) && !Before.EndsWith(Goal.Item.ToString())
            && !(Number.GetCaptureGroup(1).Len()>0 && Text.Contains(Item)) && !Clarification)continue;
        FString Token=Number.GetCaptureGroup(2);Token.ReplaceInline(TEXT("两"),TEXT("二"));Token.ReplaceInline(TEXT("〇"),TEXT("零"));
        while(Token.Len()>1 && Token[0]==TEXT('0'))Token.RightChopInline(1);
        const bool Matches=Token.IsNumeric()?Token==FString::FromInt(Goal.Quantity):!Chinese.IsEmpty() && Token==Chinese;
        if(!Matches || Found)return false;
        Found=true;
    }
    return Found;
}
const TArray<FHearthwardAgentCapability>& HearthwardAgent::Capabilities()
{
    static const TArray<FHearthwardAgentCapability> C=[]
    {
        TArray<FName> All,Stored,Resources,Recipes,Repair,People,Wildlife,Domestic,CampRecipes;
        for(const auto& I:HearthwardBasicItems())
        {
            All.Add(I.Id);
            const auto D=HearthwardData::Find(TEXT("items"),I.Id.ToString());
            if(!I.IsInstance() && I.WeightHundredths>0 && I.UniqueClaim.IsNone()
                && HearthwardData::Text(D,TEXT("rarity"))==TEXT("common"))Stored.Add(I.Id);
        }
        for(const auto& R:HearthwardData::Rows(TEXT("craftingRecipes"))) Recipes.Add(FName(*HearthwardData::Text(R->AsObject(),TEXT("id"))));
        for(const auto& R:HearthwardData::Rows(TEXT("repairRecipes"))) Repair.Add(FName(*HearthwardData::Text(R->AsObject(),TEXT("id"))));
        for(const auto& R:HearthwardNature::Rows(TEXT("resources")))
            Resources.AddUnique(FName(*HearthwardData::Text(R->AsObject(),TEXT("item"))));
        for(const auto& R:HearthwardNature::Rows(TEXT("domestic")))
        {
            Domestic.Add(FName(*HearthwardData::Text(R->AsObject(),TEXT("id"))));
            const FName Product(*HearthwardData::Text(R->AsObject(),TEXT("product")));
            if(!Product.IsNone())Resources.AddUnique(Product);
        }
        for(const auto& R:HearthwardCampaign::Rows(TEXT("people")))
        {
            const FName Person(*HearthwardData::Text(R->AsObject(),TEXT("id")));
            if(Person.ToString().StartsWith(TEXT("rescued_")))People.Add(Person);
        }
        for(const auto& R:HearthwardNature::Rows(TEXT("wildlife")))
            Wildlife.Add(FName(*HearthwardData::Text(R->AsObject(),TEXT("id"))));
        for(const auto& R:HearthwardCamp::Table()->GetArrayField(TEXT("recipes")))
            CampRecipes.Add(FName(*HearthwardData::Text(R->AsObject(),TEXT("id"))));
        TArray<FHearthwardAgentCapability> Result={
            {TEXT("collect"),TEXT("已知安全S1新采木材→返营入库；量是新增。"),{TEXT("wood")},Policy(TEXT("max_collect")),TEXT("additional_acquired"),{TEXT("S1")},{TEXT("ban"),TEXT("source")},true},
            {TEXT("nature_collect"),TEXT("唯一已知同营地安全资源/栏舍普通产物→返营入库；量是新增。"),Resources,Policy(TEXT("max_collect")),TEXT("additional_acquired"),{TEXT("known_target")},{TEXT("ban")},true},
            {TEXT("store"),TEXT("弟弟已有普通货物→仓库；玩家3米同营地明确交付时可收货再入库。"),Stored,Policy(TEXT("max_collect")),TEXT("held_to_camp"),{TEXT("bag"),TEXT("player_bag")},{},true},
            {TEXT("retrieve"),TEXT("同营地仓库→玩家背包；实际交给玩家才完成。"),Stored,Policy(TEXT("max_collect")),TEXT("camp_to_player"),{TEXT("camp")},{},true},
            {TEXT("give"),TEXT("同营地弟弟背包→玩家；中断时货物留弟弟。"),Stored,Policy(TEXT("max_collect")),TEXT("bag_to_player"),{TEXT("bag")},{},true},
            {TEXT("fetch"),TEXT("同营地仓库→弟弟背包；实际取入才完成。"),Stored,Policy(TEXT("max_collect")),TEXT("camp_to_bag"),{TEXT("camp")},{},true},
            {TEXT("receive"),TEXT("同营地玩家背包→弟弟背包；需明确交付。"),Stored,Policy(TEXT("max_collect")),TEXT("player_to_bag"),{TEXT("player_bag")},{},true},
            {TEXT("nature_care"),TEXT("唯一已知地块/栏舍；water/fertilize/harvest各1次，deposit_feed按份数。"),{TEXT("water"),TEXT("fertilize"),TEXT("harvest"),TEXT("deposit_feed")},32,TEXT("action_count"),{TEXT("known_target")},{},true},
            {TEXT("hunt"),TEXT("玩家同行指认单只野生猎物；弟弟真实击杀才完成；战利品留尸体。"),Wildlife,1,TEXT("one_animal"),{TEXT("known_target")},{},true},
            {TEXT("fish"),TEXT("玩家同行已指认鱼点钓1条；自有竿/饵/包；按实际鱼群和稀有奖励结算。"),{TEXT("fish")},1,TEXT("one_catch"),{TEXT("known_target")},{},true},
            {TEXT("capture"),TEXT("玩家同行指认单只家畜→当前同种栏舍；首次捕获消耗自有绳索/饲料，继续牵引不重复消耗。"),Domestic,1,TEXT("one_animal"),{TEXT("known_target")},{},true},
            {TEXT("camp_batch"),TEXT("已配置且只分配弟弟的设施岗位；仓库结算有限批次，到数停产。"),CampRecipes,Policy(TEXT("max_craft_batches")),TEXT("batches"),{TEXT("assigned_region")},{},true},
            {TEXT("craft"),TEXT("授权制作配方批数→产物入库。"),Recipes,Policy(TEXT("max_craft_batches")),TEXT("batches"),{TEXT("bag"),TEXT("camp")},{TEXT("no"),TEXT("max"),TEXT("once")},true},
            {TEXT("repair"),TEXT("工作台维修自有装备；量1。"),Repair,1,TEXT("one_owned"),{TEXT("bag"),TEXT("camp")},{TEXT("no"),TEXT("max"),TEXT("once")},true},
            {TEXT("escort"),TEXT("同行护送已接触指定族人→当前营地报到；玩家负责交谈，不能寻找未知人。"),People,1,TEXT("one_person"),{TEXT("known_person")},{},true},
            {TEXT("companion_order"),TEXT("伙伴高层指令，UE决定战术细节。"),{TEXT("hold"),TEXT("follow"),TEXT("assist"),TEXT("routine")},1,TEXT("directive"),{TEXT("player")},{},true},
            {TEXT("inventory"),TEXT("只读库存认知。"),All,0,TEXT("none"),{TEXT("none")},{},false},
            {TEXT("inventory_report"),TEXT("陈述库存报告。"),All,100000,TEXT("reported_exact"),{TEXT("player")},{},false},
            {TEXT("recall"),TEXT("仅有效原话和本人事件。"),{TEXT("none")},0,TEXT("none"),{TEXT("none")},{},false},
            {TEXT("rule_proposal"),TEXT("长期1条规则卡，确认生效：ban/source/no/max/allow。"),{TEXT("none")},0,TEXT("none"),{TEXT("none")},{TEXT("ban"),TEXT("source"),TEXT("no"),TEXT("max"),TEXT("allow")},false}
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
    const auto& Catalog=Capabilities();TArray<FName> Items;
    for(const auto& C:Catalog)for(FName Id:C.Items)Items.AddUnique(Id);
    TArray<TArray<FName>> GroupIntents,GroupItems;
    for(FName Id:Items)
    {
        TArray<FName> Intents;
        for(const auto& C:Catalog)if(C.Items.Contains(Id))Intents.Add(C.Id);
        int32 Group=GroupIntents.IndexOfByPredicate([&](const auto& Existing){return Existing==Intents;});
        if(Group==INDEX_NONE){Group=GroupIntents.Add(Intents);GroupItems.Add(TArray<FName>());}
        GroupItems[Group].Add(Id);
    }
    FString Out=TEXT("能力|语义|quantity上限|mode|source（斜线分隔可选值）\n");
    for(const auto& C:Catalog)
        Out+=C.Id.ToString()+TEXT("|")+C.Description+FString::Printf(TEXT("|%d|"),C.MaxQuantity)
            +C.QuantityMode+TEXT("|")+FString::Join(C.Sources,TEXT("/"))+TEXT("\n");
    Out+=TEXT("item允许关系（左侧意图允许右侧全部物品；中文/别名[id]，同名仅id）：\n");
    const auto Aliases=PolicyData()->GetObjectField(TEXT("aliases"));
    for(int32 Group=0;Group<GroupIntents.Num();++Group)
    {
        TArray<FString> Intents,Labels;
        for(FName Intent:GroupIntents[Group])Intents.Add(Intent.ToString());
        for(FName Id:GroupItems[Group])
        {
            const FString Label=ItemText(Id);
            if(Label.IsEmpty() || Label==Id.ToString()) Labels.Add(Id.ToString());
            else
            {
                TArray<FString> Names={Label};
                for(const auto& Alias:Aliases->Values)if(Alias.Value->AsString()==Label) Names.AddUnique(FString(Alias.Key));
                Labels.Add(FString::Join(Names,TEXT("/"))+TEXT("[")+Id.ToString()+TEXT("]"));
            }
        }
        Out+=FString::Join(Intents,TEXT("/"))+TEXT(":")+FString::Join(Labels,TEXT(","))+TEXT("\n");
    }
    return Out;
}
FString HearthwardAgent::Schema()
{
    auto Root=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Branches;
    TArray<FString> RegisteredItems;for(const auto& I:HearthwardBasicItems()) RegisteredItems.Add(I.Id.ToString());
    const FString LimitItems=TEXT("(")+FString::Join(RegisteredItems,TEXT("|"))+TEXT(")");
    for(const auto& C:Capabilities())
    {
        auto B=MakeShared<FJsonObject>(),P=MakeShared<FJsonObject>();B->SetStringField(TEXT("type"),TEXT("object"));B->SetBoolField(TEXT("additionalProperties"),false);
        P->SetObjectField(TEXT("intent"),Enum({C.Id.ToString()}));
        TArray<FString> Items;for(FName I:C.Items) Items.Add(I.ToString());P->SetObjectField(TEXT("item"),Enum(Items));
        auto Q=MakeShared<FJsonObject>();Q->SetStringField(TEXT("type"),TEXT("integer"));Q->SetNumberField(TEXT("minimum"),C.Writes?1:0);Q->SetNumberField(TEXT("maximum"),C.MaxQuantity);P->SetObjectField(TEXT("quantity"),Q);
        P->SetObjectField(TEXT("mode"),Enum({C.QuantityMode}));P->SetObjectField(TEXT("source"),Enum(C.Sources));
        for(const auto& Key:{TEXT("limits"),TEXT("unresolved")})
        {
            auto A=MakeShared<FJsonObject>(),S=MakeShared<FJsonObject>();A->SetStringField(TEXT("type"),TEXT("array"));A->SetNumberField(TEXT("maxItems"),4);S->SetStringField(TEXT("type"),TEXT("string"));S->SetNumberField(TEXT("maxLength"),120);
            if(FCString::Strcmp(Key,TEXT("unresolved"))==0 && (C.Writes || C.Id==TEXT("rule_proposal"))) A->SetNumberField(TEXT("maxItems"),0);
            if(FCString::Strcmp(Key,TEXT("limits"))==0)
            {
                A->SetNumberField(TEXT("maxItems"),C.Constraints.IsEmpty()?0:4);
                if(C.Id==TEXT("rule_proposal")){A->SetNumberField(TEXT("minItems"),1);A->SetNumberField(TEXT("maxItems"),1);}
                if(!C.Constraints.IsEmpty())
                {
                    TArray<FString> Patterns,ItemTypes;
                    for(const auto& Type:C.Constraints)
                    {
                        if(Type==TEXT("source")) Patterns.Add(TEXT("source:S1"));
                        else if(Type==TEXT("max")) Patterns.Add(TEXT("max:")+LimitItems+TEXT(":(0|[1-9][0-9]{0,4}|100000)"));
                        else ItemTypes.Add(Type);
                    }
                    if(!ItemTypes.IsEmpty()) Patterns.Add(TEXT("(")+FString::Join(ItemTypes,TEXT("|"))+TEXT("):")+LimitItems);
                    S->SetStringField(TEXT("pattern"),TEXT("^(")+FString::Join(Patterns,TEXT("|"))+TEXT(")$"));
                }
            }
            A->SetObjectField(TEXT("items"),S);P->SetObjectField(Key,A);
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
    if(T.Num()==2) return T[0]==TEXT("ban") || T[0]==TEXT("no") || T[0]==TEXT("allow") || T[0]==TEXT("once");
    if(T.Num()!=3 || T[0]!=TEXT("max") || !T[2].IsNumeric() || T[2].Contains(TEXT("."))) return false;
    int64 N=FCString::Atoi64(*T[2]);return N>=0 && N<=100000 && FString::Printf(TEXT("%lld"),N)==T[2];
}
FString HearthwardAgent::Validate(const FHearthwardAgentGoal& G)
{
    const auto* C=Capabilities().FindByPredicate([&](const auto& X){return X.Id==G.Intent;});
    if(!C || G.CapabilityVersion!=2 || !C->Items.Contains(G.Item)) return TEXT("UNSUPPORTED_CAPABILITY");
    if(G.Quantity<(C->Writes?1:0) || G.Quantity>C->MaxQuantity || G.QuantityMode!=C->QuantityMode || !C->Sources.Contains(G.SourceRef)) return TEXT("AMBIGUOUS_TARGET");
    if(G.Limits.Num()>4 || G.Unresolved.Num()>4) return TEXT("UNRESOLVED_CONSTRAINT");
    if(C->Writes && !G.Original.IsEmpty())
    {
        FRegexMatcher InvalidQuantity(FRegexPattern(TEXT("[-−负]\\s*[0-9一二两三四五六七八九十]|[0-9]+[.．][0-9]+|[零一二两三四五六七八九十]+点[零一二两三四五六七八九十]+")),G.Original);
        if(InvalidQuantity.FindNext())return TEXT("UNRESOLVED_CONSTRAINT");
    }
    if(G.Intent==TEXT("nature_care") && G.Item!=TEXT("deposit_feed") && G.Quantity!=1)return TEXT("AMBIGUOUS_TARGET");
    for(const auto& L:G.Limits)
    {
        FString Type,Rest;L.Split(TEXT(":"),&Type,&Rest);
        if(!ValidLimit(L) || !C->Constraints.Contains(Type)) return TEXT("UNRESOLVED_CONSTRAINT");
        if(Type==TEXT("ban") && Rest==G.Item.ToString() && (G.Intent==TEXT("collect") || G.Intent==TEXT("nature_collect"))) return TEXT("POLICY_CONFLICT");
    }
    if((G.Intent==TEXT("collect") || G.Intent==TEXT("nature_collect")) && !G.Original.IsEmpty())
    {
        const FString Text=Normalize(G.Original);
        FRegexMatcher Excluded(FRegexPattern(TEXT("(?:别|不要|不许|不准|禁止|不得|不能|勿|不)\\s*(?:去|到|往|前往|进入|靠近)|(?:避开|绕开)\\s*[^，,。；;\\n]+")),Text);
        if(Excluded.FindNext())return TEXT("UNRESOLVED_COLLECTION_LOCATION");
        if(G.Intent==TEXT("collect"))
        {
            FRegexMatcher Acquisition(FRegexPattern(TEXT("(?:^|[，,。；;：:\\n])\\s*(?:(?:请|帮我|帮忙|给我|去|从|到|麻烦)(?:(?!不|别|勿|禁止|无需|(?:之前|已经|曾经|已)\\s*(?:新采(?:集)?|采集|收集|采))[^，,。；;\\n])*?)?(?:新采(?:集)?|采集|收集|采)(?=\\s*(?:[0-9零一二两三四五六七八九十百千万些点]|")+ItemText(G.Item)+TEXT("|")+G.Item.ToString()+TEXT("))")),Text);
            if(!Acquisition.FindNext())return TEXT("UNRESOLVED_COLLECTION_SOURCE");
        }
    }
    if((G.Intent==TEXT("craft") || G.Intent==TEXT("repair") || G.Intent==TEXT("rule_proposal"))
        && (!OriginalMaterialBudgetsMatch(G) || !OriginalMaterialPermissionsMatch(G)))return TEXT("UNRESOLVED_CONSTRAINT");
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
    if(Item==TEXT("water"))return TEXT("地块浇水");
    if(Item==TEXT("fertilize"))return TEXT("地块施肥");
    if(Item==TEXT("harvest"))return TEXT("成熟作物收获");
    if(Item==TEXT("deposit_feed"))return TEXT("栏舍喂料");
    if(Item==TEXT("fish"))return TEXT("鱼点渔获");
    if(auto R=HearthwardNature::Definition(TEXT("domestic"),Item))return HearthwardData::Text(R,TEXT("name"));
    if(Item.ToString().StartsWith(TEXT("rescued_")))return TEXT("族人")+Item.ToString().Right(2);
    if(auto R=HearthwardNature::Definition(TEXT("wildlife"),Item))return HearthwardData::Text(R,TEXT("name"));
    if(const auto* I=HearthwardBasicItems().FindByPredicate([&](const auto& X){return X.Id==Item;}))return I->DisplayName.ToString();
    if(auto R=HearthwardData::Find(TEXT("craftingRecipes"),Item.ToString()))return HearthwardData::Text(R,TEXT("name"));
    if(auto R=HearthwardCamp::Recipe(Item))return HearthwardData::Text(R,TEXT("name"));
    return TEXT("未指定物品");
}
FString HearthwardAgent::LimitText(const FString& L)
{
    TArray<FString> T;L.ParseIntoArray(T,TEXT(":"));if(!ValidLimit(L))return L;
    if(T[0]==TEXT("source"))return TEXT("仅当前已知安全采集点");
    const FString Name=ItemText(FName(*T[1]));
    if(T[0]==TEXT("ban"))return TEXT("禁止采集")+Name;
    if(T[0]==TEXT("no"))return TEXT("不得消耗")+Name;
    if(T[0]==TEXT("allow"))return TEXT("解除以后不得消耗")+Name+TEXT("的约定（不授权取仓库）");
    if(T[0]==TEXT("once"))return TEXT("仅本次允许消耗")+Name+TEXT("；其他约定仍有效");
    return FString::Printf(TEXT("累计最多消耗%s %s份"),*Name,*T[2]);
}
FString HearthwardAgent::EventText(FName Kind)
{
    if(Kind==TEXT("collect"))return TEXT("采集");
    if(Kind==TEXT("acquired"))return TEXT("实际采集");if(Kind==TEXT("delivered"))return TEXT("实际交付");
    if(Kind==TEXT("craft"))return TEXT("完成制作");if(Kind==TEXT("repair"))return TEXT("完成维修");
    if(Kind==TEXT("completed"))return TEXT("任务完成");if(Kind==TEXT("materials_taken"))return TEXT("按授权领取材料");if(Kind==TEXT("handoff"))return TEXT("当面收到玩家物品");
    if(Kind==TEXT("withdrawn"))return TEXT("从营地仓库取货");if(Kind==TEXT("returned"))return TEXT("未交付货物返仓");
    if(Kind==TEXT("cargo_missing"))return TEXT("携带货物不在背包");
    if(Kind==TEXT("cancelled"))return TEXT("任务取消");return TEXT("任务受阻");
}
FString HearthwardAgent::GoalText(const FHearthwardAgentGoal& G)
{
    if(G.Intent.IsNone())return TEXT("暂无待补充任务");
    const FString Name=ItemText(G.Item);
    FString Action=G.Intent==TEXT("collect") || G.Intent==TEXT("nature_collect")?TEXT("新采集"):G.Intent==TEXT("store")?TEXT("搬运已有"):G.Intent==TEXT("retrieve")?TEXT("仓库取出并交付"):G.Intent==TEXT("give")?TEXT("交给玩家"):G.Intent==TEXT("fetch")?TEXT("仓库取到弟弟背包"):G.Intent==TEXT("receive")?TEXT("从玩家背包接收"):G.Intent==TEXT("nature_care")?TEXT("照料"):
        G.Intent==TEXT("craft")?TEXT("制作"):G.Intent==TEXT("repair")?TEXT("维修自己的"):G.Intent==TEXT("inventory_report")?TEXT("玩家报告库存"):TEXT("新增长期规则");
    FString Unit=G.Intent==TEXT("craft")?TEXT("批"):G.Intent==TEXT("repair")?TEXT("件"):TEXT("份");
    FString Text=FString::Printf(TEXT("%s %s × %d %s\n来源：%s；目的地：营地仓库"),*Action,*Name,G.Quantity,*Unit,G.SourceRef==TEXT("camp")?TEXT("授权共享仓库材料"):G.SourceRef==TEXT("bag")?TEXT("弟弟背包"):G.SourceRef==TEXT("player_bag")?TEXT("玩家背包，确认时当面交付"):TEXT("当前安全采集点"));
    if(G.Intent==TEXT("repair")) Text=FString::Printf(TEXT("维修弟弟自己的 %s × 1 件；保留在弟弟背包\n装备实例：%s；材料：%s"),*Name,
        G.EquipmentId.IsValid()?*G.EquipmentId.ToString().Left(8):TEXT("待确认唯一实例"),G.SourceRef==TEXT("camp")?TEXT("授权共享仓库"):TEXT("弟弟背包"));
    if(G.Intent==TEXT("companion_order"))
        Text=FString::Printf(TEXT("伙伴高层指令：%s\n战术目标、导航、攻击时机与伤害由UE按当前世界状态决定"),*Name);
    if(G.Intent==TEXT("nature_care"))Text=FString::Printf(TEXT("在目标[%s]执行%s × %d；实际完成由地块／栏舍状态决定"),*G.Station.ToString().Left(8),*G.Item.ToString(),G.Quantity);
    if(G.Intent==TEXT("nature_collect"))Text=FString::Printf(TEXT("从已知安全目标[%s]取得%s × %d份并送入当前营地仓库；仅计实际取得与入库"),*G.Station.ToString().Left(8),*Name,G.Quantity);
    if(G.Intent==TEXT("escort"))Text=FString::Printf(TEXT("同行护送已接触的%s回当前营地；只有族人真实到营报到才计完成。玩家需在弟弟30米内同行。"),*Name);
    if(G.Intent==TEXT("hunt"))Text=FString::Printf(TEXT("同行狩猎已指认的%s[%s]；弟弟真实击杀才计完成，战利品留在尸体；玩家需在弟弟30米内同行。"),*Name,*G.Station.ToString().Left(8));
    if(G.Intent==TEXT("fish"))Text=FString::Printf(TEXT("在已指认鱼点[%s]同行钓获一条；消耗弟弟鱼饵并磨损鱼竿，渔获留在弟弟背包；玩家需在弟弟30米内同行。"),*G.Station.ToString().Left(8));
    if(G.Intent==TEXT("capture"))Text=FString::Printf(TEXT("同行捕获或继续牵引已指认的%s[%s]回当前营地栏舍；只有动物实际入栏才计完成；新捕获消耗弟弟1份饲料和1条绳索。"),*Name,*G.Station.ToString().Left(8));
    if(G.Intent==TEXT("camp_batch"))Text=FString::Printf(TEXT("在当前营地设施[%s]执行%s × %d批；共享仓储按现有生产配方真实扣料与入库，到限定批数自动停产。"),*G.Station.ToString().Left(8),*Name,G.Quantity);
    if(G.Intent==TEXT("retrieve"))Text=FString::Printf(TEXT("从当前营地仓库取出%s × %d份，交入同营地玩家背包；仅计实际交付，受阻返仓不计完成"),*Name,G.Quantity);
    if(G.Intent==TEXT("give"))Text=FString::Printf(TEXT("从弟弟背包取已有%s × %d份，交入同营地玩家背包；仅计实际交付，受阻时留在弟弟背包"),*Name,G.Quantity);
    if(G.Intent==TEXT("fetch"))Text=FString::Printf(TEXT("从当前营地仓库取出%s × %d份，留在弟弟背包；仅计实际取入，仓库库存相应减少"),*Name,G.Quantity);
    if(G.Intent==TEXT("receive"))Text=FString::Printf(TEXT("从同营地玩家背包接收%s × %d份，留在弟弟背包；仅计实际接收，玩家背包相应减少"),*Name,G.Quantity);
    if(G.Intent==TEXT("inventory_report"))Text=FString::Printf(TEXT("玩家报告：营地仓库当前有 %d 份%s；仅更新弟弟认知，不改变实际仓库"),G.Quantity,*Name);
    if(G.Intent==TEXT("rule_proposal"))Text=TEXT("新增长期规则；确认后影响新接受的任务");
    TArray<FString> Labels;for(const auto& L:G.Limits)Labels.Add(LimitText(L));
    if(!Labels.IsEmpty())Text+=TEXT("\n限制：")+FString::Join(Labels,TEXT("、"));
    if(!G.Unresolved.IsEmpty())Text+=TEXT("\n待补充：")+FString::Join(G.Unresolved,TEXT("、"));
    return Text;
}
