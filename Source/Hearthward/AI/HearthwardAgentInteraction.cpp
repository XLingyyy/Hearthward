#include "HearthwardLocalAISubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Building/HearthwardWorkshopService.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Internationalization/Regex.h"

namespace
{
FString Json(const TSharedPtr<FJsonObject>& O){FString S;FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&S));return S;}
bool Object(FHttpResponsePtr R,TSharedPtr<FJsonObject>& O)
{return R.IsValid() && R->GetResponseCode()==200 && R->GetContentLength()<256*1024 && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(R->GetContentAsString()),O) && O.IsValid();}
}
void UHearthwardLocalAISubsystem::RestoreMemory(const FHearthwardNPCMemory& Snapshot)
{
    // Save decoding/migration and validation happen before world mutation. Do not repair a current-format
    // snapshot here, otherwise damaged new fields could be mistaken for legacy data.
    Memory=Snapshot;
}
void UHearthwardLocalAISubsystem::CountRequest(const TArray<TSharedPtr<FJsonObject>>& Bodies,
    const TArray<FHearthwardNPCContextProjectionResult>& Projections,int32 TierIndex)
{
    if(!StillCurrent())return;
    if(Bodies.Num()!=Projections.Num() || !Bodies.IsValidIndex(TierIndex))
    {Fail(TEXT("上下文投影配置无效，未生成提案"));return;}

    const auto Body=Bodies[TierIndex];
    const uint64 Expected=Serial;
    Request=FHttpModule::Get().CreateRequest();
    Request->SetURL(Runtime.GetBaseUrl()+TEXT("/apply-template"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Authorization"),TEXT("Bearer ")+Runtime.GetApiKey());
    Request->SetHeader(TEXT("Content-Type"),TEXT("application/json"));
    Request->SetContentAsString(Json(Body));
    Request->SetTimeout(10);
    Request->OnProcessRequestComplete().BindWeakLambda(this,
        [this,Expected,Bodies,Projections,TierIndex,Body](FHttpRequestPtr,FHttpResponsePtr R,bool Ok)
    {
        if(Expected!=Serial)return;
        Request.Reset();
        if(!StillCurrent())return;

        TSharedPtr<FJsonObject> O;FString Prompt;
        if(!Ok || !Object(R,O) || !O->TryGetStringField(TEXT("prompt"),Prompt))
        {Fail(TEXT("模型模板计数失败，未生成提案"));return;}

        auto T=MakeShared<FJsonObject>();
        T->SetStringField(TEXT("content"),Prompt);
        T->SetBoolField(TEXT("add_special"),true);
        T->SetBoolField(TEXT("parse_special"),true);

        Request=FHttpModule::Get().CreateRequest();
        Request->SetURL(Runtime.GetBaseUrl()+TEXT("/tokenize"));
        Request->SetVerb(TEXT("POST"));
        Request->SetHeader(TEXT("Authorization"),TEXT("Bearer ")+Runtime.GetApiKey());
        Request->SetHeader(TEXT("Content-Type"),TEXT("application/json"));
        Request->SetContentAsString(Json(T));
        Request->SetTimeout(10);
        Request->OnProcessRequestComplete().BindWeakLambda(this,
            [this,Expected,Bodies,Projections,TierIndex,Body](FHttpRequestPtr,FHttpResponsePtr Response,bool Success)
        {
            if(Expected!=Serial)return;
            Request.Reset();
            if(!StillCurrent())return;

            TSharedPtr<FJsonObject> Counts;const TArray<TSharedPtr<FJsonValue>>* Tokens=nullptr;
            if(!Success || !Object(Response,Counts) || !Counts->TryGetArrayField(TEXT("tokens"),Tokens))
            {Fail(TEXT("模型token计数失败，未生成提案"));return;}

            InputTokens=Tokens->Num();
            const int32 Limit=HearthwardAgent::Policy(TEXT("max_input_tokens"));
            if(InputTokens>Limit)
            {
                if(Bodies.IsValidIndex(TierIndex+1))
                {
                    UE_LOG(LogTemp,Display,TEXT("Local AI context tier %s over budget: %d>%d; degrading once to %s"),
                        *Projections[TierIndex].Tier,InputTokens,Limit,*Projections[TierIndex+1].Tier);
                    CountRequest(Bodies,Projections,TierIndex+1);
                    return;
                }

                LastFilteredContext=Projections[TierIndex].Json;
                ContextTier=Projections[TierIndex].Tier;
                DroppedContextFields=Projections[TierIndex].DroppedFields;
                Fail(TEXT("固定规则、当前原话和未解决限制已超过内部上下文预算；未发送生成请求，请结束旧澄清或重新开始交流。"),
                    TEXT("CONTEXT_OVERFLOW"));
                return;
            }

            LastFilteredContext=Projections[TierIndex].Json;
            ContextTier=Projections[TierIndex].Tier;
            DroppedContextFields=Projections[TierIndex].DroppedFields;
            UE_LOG(LogTemp,Display,TEXT("Local AI context accepted: tier=%s tokens=%d dropped=%s"),
                *ContextTier,InputTokens,*FString::Join(DroppedContextFields,TEXT(",")));
            Generate(Body);
        });
        if(!Request->ProcessRequest()){Request.Reset();Fail(TEXT("无法检查输入长度"));}
    });

    Status=TEXT("正在检查对话长度（")+Projections[TierIndex].Tier+TEXT("）");
    if(!Request->ProcessRequest()){Request.Reset();Fail(TEXT("无法检查输入长度"));}
}
void UHearthwardLocalAISubsystem::StageCandidate(FHearthwardAgentGoal Goal)
{
    CandidateId.Invalidate();Goal.Original=Input;Goal.RuleRevision=Memory.Revision;
    if(!Memory.WorkingGoal.Original.IsEmpty())Goal.Original=Memory.WorkingGoal.Original+TEXT("\n补充：")+Input;
    if(Goal.WritesWorld())
    {
        // Schema-constrained generation can discard a sign or round a fraction. Preserve the player's numeric boundary.
        FRegexMatcher InvalidQuantity(FRegexPattern(TEXT("[-−负]\\s*[0-9一二两三四五六七八九十]|[0-9]+[.．][0-9]+|[零一二两三四五六七八九十]+点[零一二两三四五六七八九十]+")),Goal.Original);
        if(InvalidQuantity.FindNext())Goal.Unresolved.AddUnique(TEXT("原话含负数或小数数量，不能改写成正整数任务"));
        if(Goal.Intent==TEXT("collect") && (Input.Contains(TEXT("改成")) || Input.Contains(TEXT("改为")) || Input.Contains(TEXT("换成")) || Input.Contains(TEXT("不是"))))
        {
            const FString Correction=HearthwardAgent::Normalize(Input);
            for(const auto& I:HearthwardBasicItems())
                if(I.Id!=Goal.Item && Correction.Contains(I.DisplayName.ToString()))
                {
                    Goal.Item=I.Id;
                    Goal.Unresolved.AddUnique(TEXT("物品已更改，请核对当前已注册的采集能力；不能沿用旧物品"));
                }
        }
        if(Goal.Intent==TEXT("collect") && !HearthwardAgent::Normalize(Goal.Original).Contains(HearthwardAgent::ItemText(Goal.Item))
            && !Goal.Original.Contains(Goal.Item.ToString()))Goal.Unresolved.AddUnique(TEXT("item"));
        if(Goal.Intent==TEXT("craft") && !Goal.Original.Contains(TEXT("批")))
            Goal.Unresolved.AddUnique(TEXT("请明确制作批数，不能把成品件数直接当批数"));
        if(Goal.Intent==TEXT("repair") && (Input.Contains(TEXT("我的")) || Input.Contains(TEXT("我背包"))
            || Input.Contains(TEXT("我装备")) || Input.Contains(TEXT("我身上")) || Input.Contains(TEXT("玩家")) || Input.Contains(TEXT("我穿")) || Input.Contains(TEXT("装备全"))))
            Goal.Unresolved.AddUnique(TEXT("只能维修弟弟自己的唯一装备，不能代换玩家装备或多件目标"));
        if(Input.Contains(TEXT("再来")) || Input.Contains(TEXT("补到")) || Input.Contains(TEXT("凑够")))
            Goal.Unresolved.AddUnique(TEXT("请明确新取得数量；追加量和最终总量不能混用"));
        if(Goal.Intent==TEXT("collect") && (Input.Contains(TEXT("然后")) || Input.Contains(TEXT("再去")) || Input.Contains(TEXT("再修"))))
        {
            if(Input.Contains(TEXT("制作")) || Input.Contains(TEXT("修")) || Input.Contains(TEXT("工作台")) || Input.Contains(TEXT("建造")) || Input.Contains(TEXT("敌营")))
                Goal.Unresolved.AddUnique(TEXT("包含第二个任务，请分别安排并确认"));
        }
        if(Goal.Intent!=TEXT("collect") && (Input.Contains(TEXT("然后")) || Input.Contains(TEXT("再去")) || Input.Contains(TEXT("先"))))
        {
            if(Input.Contains(TEXT("采")) || Input.Contains(TEXT("收集")) || (Goal.Intent==TEXT("craft") && Input.Contains(TEXT("修"))))
                Goal.Unresolved.AddUnique(TEXT("包含第二个任务，请分别安排并确认"));
        }
    }
    if(Goal.Intent==TEXT("collect") || Goal.Intent==TEXT("craft"))
    {
        FRegexMatcher Number(FRegexPattern(TEXT("([0-9]+|[一二两三四五六七八九十]+)\\s*(份|个|根|单位|块|批)|[0-9]+|数量[为是： ]*[一二两三四五六七八九十]+")),Goal.Original);
        if(!Number.FindNext())
        {
            Goal.Quantity=0;Goal.Unresolved.AddUnique(TEXT("quantity"));Memory.WorkingGoal=Goal;
            NPCLine=TEXT("需要多少？请明确数量，我再准备任务卡。");Status=TEXT("等待补充信息");LastAppliedIntent=TEXT("clarify");ReasonCode=TEXT("AMBIGUOUS_TARGET");
            Memory.AddClarification(Input,NPCLine);return;
        }
    }
    for(const auto& U:Memory.WorkingGoal.Unresolved)
    {
        // Only explicit slots can be filled by a later candidate. Arbitrary conditions remain unresolved.
        if(U==TEXT("数量") || U==TEXT("缺少数量") || U==TEXT("quantity"))continue;
        if(U==TEXT("物品") || U==TEXT("缺少物品") || U==TEXT("item"))continue;
        Goal.Unresolved.AddUnique(U);
    }
    for(const auto& L:Memory.ApplicableRules(Goal.Intent))Goal.Limits.AddUnique(L);
    if(Goal.Original.Len()>1000 || Goal.Unresolved.Num()>4 || Goal.Limits.Num()>4)
    {ReasonCode=TEXT("CONTEXT_OVERFLOW");NPCLine=TEXT("本次条件超出任务卡容量，请保留全部要求重新整理。草稿仍保留。");Status=NPCLine;LastAppliedIntent=TEXT("refuse");return;}
    ReasonCode=HearthwardAgent::Validate(Goal);
    if(Goal.WritesWorld())
    {
        if(Goal.Intent==TEXT("craft") || Goal.Intent==TEXT("repair"))
        {
            auto* P=UGameplayStatics::GetPlayerPawn(GetWorld(),0);auto* B=P?P->FindComponentByClass<UHearthwardBuildingComponent>():nullptr;
            Goal.Station=B?B->KnownWorkbench(PendingCompanion.Get()):FGuid();
        }
        if(ReasonCode.IsEmpty())
        {
            if(Goal.Intent==TEXT("companion_order"))
            {
                auto* Gameplay=PendingSpeaker.IsValid()?PendingSpeaker->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
                ReasonCode=Gameplay?Gameplay->PreviewCompanionDirective(PendingSpeaker.Get(),Goal.Item):TEXT("PLAYER_UNAVAILABLE");
            }
            else ReasonCode=PendingCompanion->PreviewGoal(Goal);
        }
    }
    Memory.WorkingGoal=Goal;
    if(!ReasonCode.IsEmpty())
    {
        NPCLine=ReasonCode==TEXT("POLICY_CONFLICT")?TEXT("这项任务与已确认的规则冲突，请先修改规则或任务。"):
            ReasonCode==TEXT("UNRESOLVED_CONSTRAINT")?TEXT("还有未解决的限制，不能确认执行。请明确地点或结束本次澄清后重新安排。"):
            ReasonCode==TEXT("AMBIGUOUS_TARGET")?TEXT("没有找到我自己持有的唯一目标，不能代用你的装备。"):
            ReasonCode==TEXT("ALREADY_REPAIRED")?TEXT("这件装备已经完好，无需维修。"):TEXT("当前能力或工作台条件不满足，未改变正在执行的任务。");
        Status=TEXT("任务未就绪：")+ReasonCode;LastAppliedIntent=TEXT("refuse");return;
    }
    Candidate=Goal;CandidateId=FGuid::NewGuid();CandidateMemoryRevision=Memory.Revision;
    NPCLine=TEXT("请核对下面的任务卡，确认后我再开始。");Status=TEXT("等待确认");LastAppliedIntent=TEXT("proposal");
}
bool UHearthwardLocalAISubsystem::ConfirmCandidate(FGuid Id)
{
    if(!Id.IsValid() || Id!=CandidateId || bPending || !PendingCompanion.IsValid()
        || !PendingCompanion->IsProposalCurrent(PendingSpeaker.Get(),Ticket) || CandidateMemoryRevision!=Memory.Revision
        || GetWorld()->IsPaused() || GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring())
    {ReasonCode=TEXT("STALE_CONFIRMATION");Status=TEXT("这张任务卡已失效，请重新交流");return false;}
    ReasonCode=HearthwardAgent::Validate(Candidate);if(!ReasonCode.IsEmpty())return false;
    if(Candidate.Intent==TEXT("rule_proposal"))
    {
        const double Now=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds;
        if(!Memory.PutRule(Candidate.Limits[0],Candidate.Original,Now)){ReasonCode=TEXT("MEMORY_CAPACITY");Status=TEXT("规则未保存，请缩短原话或管理记录容量");return false;}
        PendingCompanion->DiscardProposal(Ticket);NPCLine=TEXT("长期规则已确认，将用于后续接受的任务。当前任务保持原状。");
    }
    else if(Candidate.Intent==TEXT("companion_order"))
    {
        auto* Gameplay=PendingSpeaker.IsValid()?PendingSpeaker->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
        ReasonCode=Gameplay?Gameplay->PreviewCompanionDirective(PendingSpeaker.Get(),Candidate.Item):TEXT("PLAYER_UNAVAILABLE");
        if(!ReasonCode.IsEmpty()){Status=TEXT("条件已变化，未执行：")+ReasonCode;return false;}
        if(!Gameplay->ApplyCompanionDirective(PendingSpeaker.Get(),Candidate.Item))
        {ReasonCode=TEXT("STALE_CONFIRMATION");Status=TEXT("伙伴指令已失效，请重新交流");return false;}
        RecordCoordinationDirective(Candidate.Item,TEXT("dialogue_confirm"));
        NPCLine=Candidate.Item==TEXT("hold")?TEXT("好，我先原地等待。"):
            Candidate.Item==TEXT("follow")?TEXT("好，我跟着你。"):
            Candidate.Item==TEXT("routine")?TEXT("好，我在营地附近自己活动，有事你再叫我。"):
            TEXT("好，我会协助处理你附近的有效威胁。");
    }
    else
    {
        ReasonCode=PendingCompanion->PreviewGoal(Candidate);if(!ReasonCode.IsEmpty()){Status=TEXT("条件已变化，未执行：")+ReasonCode;return false;}
        if(PendingCompanion->SubmitGoal(PendingSpeaker.Get(),Ticket,Candidate)!=EHearthwardProposalResult::Accepted){ReasonCode=TEXT("STALE_CONFIRMATION");return false;}
        NPCLine=Candidate.Intent==TEXT("repair")?TEXT("维修任务已接受，完成后装备仍由我持有。"):TEXT("任务已接受，完成数量以实际交付为准。");
    }
    LastAppliedIntent=Candidate.Intent.ToString();CandidateId.Invalidate();Memory.Clarification.Reset();Memory.WorkingGoal={};Status=NPCLine;return true;
}
FString UHearthwardLocalAISubsystem::GetCandidateText() const
{
    if(!HasCandidate())return Memory.WorkingGoal.Unresolved.IsEmpty()?FString():TEXT("原话：")+Memory.WorkingGoal.Original+TEXT("\n未解决的条件：")+FString::Join(Memory.WorkingGoal.Unresolved,TEXT("；"));
    FString S=TEXT("原话：")+Candidate.Original+TEXT("\n")+HearthwardAgent::GoalText(Candidate);
    if(Candidate.Intent==TEXT("craft") || Candidate.Intent==TEXT("repair"))
    {
        S+=TEXT("\n实际消耗：");for(const auto& C:HearthwardWorkshop::Materials(Candidate.Intent,Candidate.Item,Candidate.Quantity))
        {const auto* I=HearthwardBasicItems().FindByPredicate([&](const auto& X){return X.Id==C.Key;});S+=FString::Printf(TEXT("%s%d "),*I->DisplayName.ToString(),C.Value);}
        if(Candidate.Intent==TEXT("craft")){S+=TEXT("\n实际产量：");for(const auto& C:HearthwardWorkshop::Outputs(Candidate.Item,Candidate.Quantity))S+=FString::Printf(TEXT("%s %d "),*HearthwardAgent::ItemText(C.Key),C.Value);}
        if(Candidate.Intent==TEXT("repair") && PendingCompanion.IsValid())S+=FString::Printf(TEXT("\n当前耐久：%.0f；修好后仍由弟弟持有"),PendingCompanion->OwnedDurability.FindRef(Candidate.Item));
    }
    if(PendingCompanion.IsValid() && PendingCompanion->GetRequested()>PendingCompanion->GetDelivered())S+=FString::Printf(TEXT("\n将替换任务 %s（已交付%d）"),*PendingCompanion->GetCommandId().ToString().Left(8),PendingCompanion->GetDelivered());
    return S;
}
bool UHearthwardLocalAISubsystem::AdjustCandidate(FGuid Id,int32 Delta)
{
    if(Id!=CandidateId || !HasCandidate() || (Delta!=1 && Delta!=-1) || !PendingCompanion.IsValid()
        || !PendingCompanion->IsProposalCurrent(PendingSpeaker.Get(),Ticket) || CandidateMemoryRevision!=Memory.Revision)return false;
    auto G=Candidate;G.Quantity+=Delta;if(!HearthwardAgent::Validate(G).IsEmpty())return false;
    Candidate=G;Memory.WorkingGoal=G;CandidateId=FGuid::NewGuid();return true;
}
bool UHearthwardLocalAISubsystem::SetStructuredGoal(AActor* Speaker,AHearthwardCompanionFixture* Companion,const FHearthwardAgentGoal& Goal)
{
    if(!IsValid(Companion) || !Companion->CanCommunicate(Speaker) || GetWorld()->IsPaused())return false;
    CancelPending();Memory.Clarification.Reset();Memory.WorkingGoal={};PendingSpeaker=Speaker;PendingCompanion=Companion;Input=HearthwardAgent::GoalText(Goal);Ticket=Companion->Request(Speaker,Input);
    if(!Ticket.Id.IsValid())return false;StageCandidate(Goal);ReasonCode=TEXT("deterministic_fallback");return HasCandidate();
}
bool UHearthwardLocalAISubsystem::QueryInventory(AActor* Speaker,AHearthwardCompanionFixture* Companion,FName Item)
{
    if(!IsValid(Companion) || !Companion->CanCommunicate(Speaker) || !HearthwardBasicItems().ContainsByPredicate([&](const auto& I){return I.Id==Item;}))return false;
    CancelPending();PendingSpeaker=Speaker;PendingCompanion=Companion;Ticket=Companion->Request(Speaker,TEXT("显式库存查询"));if(!Ticket.Id.IsValid())return false;
    Proposal={};Proposal.Intent=TEXT("inventory");Proposal.Item=Item;Proposal.QuantityMode=TEXT("none");Proposal.SourceRef=TEXT("none");bPending=true;ApplyProposal();ReasonCode=TEXT("deterministic_fallback");return true;
}

bool UHearthwardLocalAISubsystem::RecordPlayerCampReport(FName Item,int32 Count)
{
    if(Count<0 || Count>100000 || !HearthwardBasicItems().ContainsByPredicate([&](const auto& I){return I.Id==Item;}))return false;
    const double Now=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds;
    return HearthwardBeliefs::UpsertCampStock(Memory.Beliefs,Memory.Revision,Memory.Campaign,Item,Count,EHearthwardNPCBeliefSource::PlayerReport,Now);
}

bool UHearthwardLocalAISubsystem::ReportCampInventory(AActor* Speaker,AHearthwardCompanionFixture* Companion,FName Item,int32 Count)
{
    if(!IsValid(Companion) || !Companion->CanCommunicate(Speaker) || GetWorld()->IsPaused()
        || GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring())return false;
    CancelPending();
    if(!RecordPlayerCampReport(Item,Count))return false;
    const auto* Def=HearthwardBasicItems().FindByPredicate([&](const auto& I){return I.Id==Item;});
    NPCLine=FString::Printf(TEXT("我记下你说营地现在有 %d 份%s；这是你的报告，我还没有亲自确认。"),Count,*Def->DisplayName.ToString());
    Status=NPCLine;LastAppliedIntent=TEXT("inventory_report");ReasonCode=TEXT("player_report");return true;
}

FHearthwardNPCBeliefView UHearthwardLocalAISubsystem::GetCampStockBelief(FName Item) const
{
    FHearthwardNPCBeliefView Out;HearthwardBeliefs::ResolveCampStock(Memory.Beliefs,Item,Out);return Out;
}

void UHearthwardLocalAISubsystem::RecordCoordinationDirective(FName Directive,const FString& Source)
{
    if(Directive!=TEXT("hold") && Directive!=TEXT("follow") && Directive!=TEXT("assist"))return;
    FHearthwardNPCEvent E;
    E.Id=FGuid::NewGuid();
    E.Command=FGuid::NewGuid();
    E.Campaign=Memory.Campaign;
    E.Kind=TEXT("directive");
    E.Item=Directive;
    E.Count=1;
    E.At=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds;
    E.Reason=Source.Left(200);
    RecordEvent(E);
}

void UHearthwardLocalAISubsystem::RecordCampStockReceipt(FName Item,int32 ExactCount)
{
    if(GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring())return;
    const double Now=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds;
    HearthwardBeliefs::UpsertCampStock(Memory.Beliefs,Memory.Revision,Memory.Campaign,Item,ExactCount,EHearthwardNPCBeliefSource::Receipt,Now);
}
FString UHearthwardLocalAISubsystem::BuildEpisodeRecall(FName Item) const
{
    FString Out;int32 Added=0;
    for(const auto& Episode:HearthwardEpisodes::Build(Memory,8))
    {
        if(!Item.IsNone() && Episode.Item!=Item)continue;
        const FString Line=HearthwardEpisodes::Describe(Episode);
        if(Line.IsEmpty())continue;
        Out+=Line+TEXT("\n");
        if(++Added>=3)break;
    }
    return Out;
}

bool UHearthwardLocalAISubsystem::QueryRecentHistory(AActor* Speaker,AHearthwardCompanionFixture* Companion,FName Item)
{
    if(!IsValid(Companion) || !Companion->CanCommunicate(Speaker))return false;
    CancelPending();Initiatives.Reset();PendingSpeaker=Speaker;PendingCompanion=Companion;
    NPCLine=BuildEpisodeRecall(Item);
    if(NPCLine.IsEmpty())NPCLine=TEXT("我没有找到对应的实际行动记录。");
    Status=TEXT("弟弟的实际经历");LastAppliedIntent=TEXT("recall");ReasonCode=TEXT("deterministic_fallback");return true;
}

bool UHearthwardLocalAISubsystem::CancelExecution(AActor* Speaker,AHearthwardCompanionFixture* Companion)
{
    if(!IsValid(Companion) || !Companion->Cancel(Speaker))return false;
    ClearClarification();PendingSpeaker=Speaker;PendingCompanion=Companion;
    NPCLine=TEXT("已停止当前委托，实际取得的物资保留。");Status=NPCLine;LastAppliedIntent=TEXT("cancel");ReasonCode=TEXT("deterministic_fallback");return true;
}
