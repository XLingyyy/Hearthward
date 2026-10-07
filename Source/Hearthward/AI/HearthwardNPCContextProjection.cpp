#include "HearthwardNPCContextProjection.h"
#include "HearthwardNPCEpisode.h"
#include "HearthwardAgentContract.h"
#include "../Inventory/HearthwardInventoryState.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FString ContextProjectionJson(const TSharedPtr<FJsonObject>& Value)
{
    FString Result;
    FJsonSerializer::Serialize(Value.ToSharedRef(),TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Result));
    return Result;
}

bool ContextProjectionContainsAny(const FString& Text,std::initializer_list<const TCHAR*> Terms)
{
    for(const auto* Term:Terms) if(Text.Contains(Term,ESearchCase::IgnoreCase)) return true;
    return false;
}

bool ContextProjectionMentionsItem(const FString& Query,FName Item)
{
    const FString Normalized=HearthwardAgent::Normalize(Query);
    return Normalized.Contains(Item.ToString(),ESearchCase::IgnoreCase)
        || Normalized.Contains(HearthwardAgent::Normalize(HearthwardAgent::ItemText(Item)),ESearchCase::IgnoreCase);
}

int32 ContextProjectionTextRelevance(const FString& Query,const FString& Text)
{
    TSet<FString> Terms;
    for(int32 I=0;I+1<Query.Len();++I)
        if(!FChar::IsWhitespace(Query[I]) && !FChar::IsPunct(Query[I]) && !FChar::IsPunct(Query[I+1]))
            Terms.Add(Query.Mid(I,2).ToLower());
    int32 Score=0;
    for(const auto& Term:Terms)if(Text.Contains(Term,ESearchCase::IgnoreCase))++Score;
    return Score;
}

TArray<FName> ContextProjectionRelevantItems(const FString& Query)
{
    TArray<FName> Out;
    for(const auto& Item:HearthwardBasicItems()) if(ContextProjectionMentionsItem(Query,Item.Id)) Out.Add(Item.Id);
    for(const auto& C:HearthwardAgent::Capabilities())
        for(FName Item:C.Items)
            if(!Item.IsNone() && Item!=TEXT("none") && ContextProjectionMentionsItem(Query,Item)) Out.AddUnique(Item);
    return Out;
}

TSharedPtr<FJsonObject> ContextProjectionEpisodeJson(const FHearthwardNPCEpisode& E,int32& EvidenceAlias)
{
    auto Row=MakeShared<FJsonObject>();
    Row->SetStringField(TEXT("command"),FString::Printf(TEXT("cmd%d"),EvidenceAlias));
    Row->SetStringField(TEXT("item"),E.Item.ToString());
    Row->SetStringField(TEXT("coverage"),HearthwardEpisodes::CoverageName(E.Coverage));
    const bool Complete=E.Coverage==EHearthwardNPCEpisodeCoverage::Complete;
    const FString Prefix=Complete?FString():TEXT("retained_");
    if(E.Acquired>0)Row->SetNumberField(Prefix+TEXT("acquired"),E.Acquired);
    if(E.Delivered>0)Row->SetNumberField(Prefix+TEXT("delivered"),E.Delivered);
    if(E.Crafted>0)Row->SetNumberField(Prefix+TEXT("crafted"),E.Crafted);
    if(E.Repaired>0)Row->SetNumberField(Prefix+TEXT("repaired"),E.Repaired);
    if(E.Replans>0)Row->SetNumberField(Prefix+TEXT("replans"),E.Replans);
    Row->SetBoolField(TEXT("completed"),E.Completed);
    Row->SetBoolField(TEXT("cancelled"),E.Cancelled);
    Row->SetNumberField(TEXT("last_at_game_seconds"),E.LastAt);
    TArray<TSharedPtr<FJsonValue>> Reasons;for(const auto& X:E.Reasons)Reasons.Add(MakeShared<FJsonValueString>(X));
    Row->SetArrayField(TEXT("reasons"),Reasons);
    TArray<TSharedPtr<FJsonValue>> Evidence;
    for(int32 I=0;I<E.Evidence.Num();++I) Evidence.Add(MakeShared<FJsonValueString>(FString::Printf(TEXT("ev%d"),EvidenceAlias++)));
    Row->SetArrayField(TEXT("evidence"),Evidence);
    return Row;
}

void ContextProjectionAddDropped(TArray<FString>& Dropped,const TCHAR* Field)
{
    Dropped.AddUnique(Field);
}
}

FString HearthwardContextProjection::TierName(EHearthwardNPCContextTier Tier)
{
    switch(Tier)
    {
    case EHearthwardNPCContextTier::Full:return TEXT("full_relevant");
    case EHearthwardNPCContextTier::Compact:return TEXT("compact_relevant");
    default:return TEXT("required_minimal");
    }
}

FHearthwardNPCContextProjectionResult HearthwardContextProjection::Project(const FHearthwardNPCContextSnapshot& S,EHearthwardNPCContextTier Tier)
{
    FHearthwardNPCContextProjectionResult Result;
    Result.Tier=TierName(Tier);
    auto Facts=MakeShared<FJsonObject>();
    Facts->SetStringField(TEXT("source"),TEXT("UE_authoritative_captured_snapshot"));
    Facts->SetStringField(TEXT("projection_tier"),Result.Tier);
    Facts->SetStringField(TEXT("input_source"),S.InputSource);
    Facts->SetStringField(TEXT("speaker"),TEXT("player"));
    Facts->SetStringField(TEXT("listener"),TEXT("brother"));
    Facts->SetStringField(TEXT("pronouns"),TEXT("原话我=player，你=brother"));
    auto Containers=MakeShared<FJsonObject>();
    Containers->SetStringField(TEXT("bag"),TEXT("弟弟背包"));
    Containers->SetStringField(TEXT("player_bag"),TEXT("玩家背包"));
    Containers->SetStringField(TEXT("camp"),TEXT("当前营地共享仓库"));
    Facts->SetObjectField(TEXT("containers"),Containers);

    const FString Query=HearthwardAgent::Normalize(S.Query+TEXT(" ")+S.Memory.WorkingGoal.Original);
    const bool HistoryQuery=ContextProjectionContainsAny(Query,{TEXT("上次"),TEXT("过去"),TEXT("经历"),TEXT("为什么"),TEXT("受阻"),TEXT("实际交付"),TEXT("完成"),TEXT("任务"),TEXT("委托"),TEXT("replan"),TEXT("history")});
    const bool OrderQuery=ContextProjectionContainsAny(Query,{TEXT("跟着"),TEXT("等待"),TEXT("威胁"),TEXT("协助"),TEXT("自由活动"),TEXT("routine"),TEXT("follow"),TEXT("assist"),TEXT("hold")});
    const bool InventoryQuery=ContextProjectionContainsAny(Query,{TEXT("库存"),TEXT("报告")})
        || (Query.Contains(TEXT("仓库")) && ContextProjectionContainsAny(Query,{TEXT("多少"),TEXT("几份"),TEXT("数量"),TEXT("还有"),TEXT("里面有")}));
    const bool CollectionQuery=ContextProjectionContainsAny(Query,{TEXT("采"),TEXT("收集"),TEXT("木材"),TEXT("collect")});
    const bool RepairQuery=ContextProjectionContainsAny(Query,{TEXT("修"),TEXT("repair")});
    const bool TaskQuery=HistoryQuery || ContextProjectionContainsAny(Query,{TEXT("进度"),TEXT("还差"),TEXT("剩余"),TEXT("停了"),TEXT("继续"),TEXT("resume"),TEXT("task_status")});
    const bool TeamQuery=ContextProjectionContainsAny(Query,{TEXT("一起"),TEXT("队"),TEXT("人数"),TEXT("集体"),TEXT("camp_team")})
        || (Query.Contains(TEXT("带")) && Query.Contains(TEXT("采")));
    const bool CareQuery=ContextProjectionContainsAny(Query,{TEXT("照顾"),TEXT("地块"),TEXT("作物"),TEXT("浇"),TEXT("施肥"),TEXT("收获"),TEXT("喂"),TEXT("nature_care")});
    const bool NatureCollectQuery=ContextProjectionContainsAny(Query,{TEXT("采"),TEXT("收集"),TEXT("产物"),TEXT("nature_collect")});
    const bool FishQuery=ContextProjectionContainsAny(Query,{TEXT("钓"),TEXT("鱼点"),TEXT("fish")});
    const bool HuntQuery=ContextProjectionContainsAny(Query,{TEXT("猎"),TEXT("狩"),TEXT("杀"),TEXT("hunt")});
    const bool CaptureQuery=ContextProjectionContainsAny(Query,{TEXT("捕"),TEXT("捉"),TEXT("牵"),TEXT("家畜"),TEXT("capture")});
    const bool PeopleQuery=ContextProjectionContainsAny(Query,{TEXT("护送"),TEXT("族人"),TEXT("escort")})
        || (Query.Contains(TEXT("带")) && Query.Contains(TEXT("人")));
    const TArray<FName> Relevant=ContextProjectionRelevantItems(Query);

    if(TaskQuery)
    {
        auto Task=MakeShared<FJsonObject>();
        Task->SetStringField(TEXT("semantics"),TEXT("仅记录当前任务"));
        Task->SetBoolField(TEXT("available"),S.bHasActiveTask);
        Task->SetStringField(TEXT("intent"),S.ActiveGoal.Intent.ToString());
        Task->SetStringField(TEXT("item"),S.ActiveGoal.Item.ToString());
        Task->SetStringField(TEXT("source"),S.ActiveGoal.SourceRef);
        Task->SetStringField(TEXT("phase"),S.ExecutionPhase);
        Task->SetNumberField(TEXT("requested"),S.PreviousGoalQuantity);
        Task->SetNumberField(TEXT("delivered"),S.PreviousGoalDelivered);
        Task->SetNumberField(TEXT("remaining"),FMath::Max(0,S.PreviousGoalQuantity-S.PreviousGoalDelivered));
        Task->SetNumberField(TEXT("carried"),S.TaskCarried);
        Task->SetStringField(TEXT("block_reason"),S.TaskBlockReason);
        Facts->SetObjectField(TEXT("current_task"),Task);
    }
    else ContextProjectionAddDropped(Result.DroppedFields,TEXT("unrelated_current_task"));
    if(TeamQuery && !S.CampTeamStatus.IsEmpty())Facts->SetStringField(TEXT("camp_team"),S.CampTeamStatus);
    else if(!S.CampTeamStatus.IsEmpty())ContextProjectionAddDropped(Result.DroppedFields,TEXT("unrelated_camp_team"));

    auto Perception=MakeShared<FJsonObject>();
    Perception->SetBoolField(TEXT("paused"),S.bPaused);
    if(Tier==EHearthwardNPCContextTier::Full)
    {
        Perception->SetBoolField(TEXT("combat_state_available"),S.bCombatStateAvailable);
        Perception->SetBoolField(TEXT("combat_active"),S.bCombatActive);
        Perception->SetBoolField(TEXT("camp_available"),S.bCampAvailable);
        Perception->SetBoolField(TEXT("collection_source_available"),S.bCollectionSourceAvailable);
        Perception->SetBoolField(TEXT("collection_source_trusted_safe"),S.bCollectionSourceTrustedSafe);
        Perception->SetBoolField(TEXT("navigation_rebuilding"),S.bNavigationRebuilding);
        Perception->SetBoolField(TEXT("at_camp"),S.bAtCamp);
        Perception->SetNumberField(TEXT("camp_distance_cm"),S.CampDistanceCm);
        Perception->SetNumberField(TEXT("collection_source_distance_cm"),S.CollectionSourceDistanceCm);
        Perception->SetStringField(TEXT("execution_phase"),S.ExecutionPhase);
        Perception->SetStringField(TEXT("execution_action"),S.ExecutionAction);
    }
    else
    {
        if(CollectionQuery)
        {
            Perception->SetBoolField(TEXT("collection_source_available"),S.bCollectionSourceAvailable);
            Perception->SetBoolField(TEXT("collection_source_trusted_safe"),S.bCollectionSourceTrustedSafe);
            Perception->SetStringField(TEXT("collection_safety"),S.CollectionSafety);
            if(!S.CollectionSafetyReason.IsEmpty())Perception->SetStringField(TEXT("collection_safety_reason"),S.CollectionSafetyReason);
        }
        if(InventoryQuery)Perception->SetBoolField(TEXT("at_camp"),S.bAtCamp);
        if(!S.ExecutionPhase.IsEmpty() && S.ExecutionPhase!=TEXT("Idle"))Perception->SetStringField(TEXT("execution_phase"),S.ExecutionPhase);
        ContextProjectionAddDropped(Result.DroppedFields,TEXT("perception_debug_details"));
    }
    Facts->SetObjectField(TEXT("npc_observation"),Perception);

    auto Bag=MakeShared<FJsonObject>();int32 BagFields=0;
    for(const auto& Pair:S.OwnBag)
    {
        if(Pair.Value<=0)continue;
        const bool Keep=Tier==EHearthwardNPCContextTier::Full || Relevant.Contains(Pair.Key);
        if(Keep){Bag->SetNumberField(Pair.Key.ToString(),Pair.Value);++BagFields;}
    }
    if(BagFields>0 && Tier!=EHearthwardNPCContextTier::Minimal)Facts->SetObjectField(TEXT("own_bag"),Bag);
    else ContextProjectionAddDropped(Result.DroppedFields,TEXT("own_bag_optional"));

    if(RepairQuery)
    {
        // A colloquial equipment name must not hide multiple owned repair candidates during trimming.
        auto Owned=MakeShared<FJsonObject>();
        auto Counts=MakeShared<FJsonObject>();
        const auto* Repair=HearthwardAgent::FindCapability(TEXT("repair"));
        if(S.bOwnBagViewAvailable && Repair)
            for(FName Item:Repair->Items)
                if(S.OwnBag.FindRef(Item)>0)Counts->SetNumberField(Item.ToString(),S.OwnBag.FindRef(Item));
        Owned->SetBoolField(TEXT("available"),S.bOwnBagViewAvailable);
        Owned->SetObjectField(TEXT("counts"),Counts);
        Owned->SetStringField(TEXT("rule"),TEXT("自有实例数；多件需明确选择；false为unknown"));
        Facts->SetObjectField(TEXT("owned_repair_candidates"),Owned);
    }

    if(CareQuery || NatureCollectQuery || FishQuery || HuntQuery || CaptureQuery)
    {
        auto Targets=MakeShared<FJsonObject>();
        auto Counts=MakeShared<FJsonObject>();
        if(S.bKnownTargetsViewAvailable)
        {
            TArray<FName> Keys;S.KnownTargetCounts.GetKeys(Keys);
            Keys.Sort([](FName A,FName B){return A.LexicalLess(B);});
            for(FName Key:Keys)
            {
                FString Intent,Item;
                if(!Key.ToString().Split(TEXT(":"),&Intent,&Item))continue;
                const bool KindRelevant=(Intent==TEXT("nature_care") && CareQuery)
                    || (Intent==TEXT("nature_collect") && NatureCollectQuery)
                    || (Intent==TEXT("fish") && FishQuery)
                    || (Intent==TEXT("hunt") && HuntQuery)
                    || (Intent==TEXT("capture") && CaptureQuery);
                const bool ItemRelevant=Relevant.IsEmpty() || Relevant.Contains(FName(*Item))
                    || Intent==TEXT("nature_care") || Intent==TEXT("fish");
                if(KindRelevant && ItemRelevant)Counts->SetNumberField(Key.ToString(),S.KnownTargetCounts.FindRef(Key));
            }
        }
        Targets->SetBoolField(TEXT("available"),S.bKnownTargetsViewAvailable);
        Targets->SetObjectField(TEXT("counts"),Counts);
        Targets->SetStringField(TEXT("rule"),TEXT("intent:item最小量候选数；非执行授权；false或缺key为unknown"));
        Facts->SetObjectField(TEXT("known_targets"),Targets);
    }
    if(PeopleQuery)
    {
        auto People=MakeShared<FJsonObject>();
        TArray<TSharedPtr<FJsonValue>> Ids;
        if(S.bKnownPeopleViewAvailable)
            for(FName Id:S.KnownPeople)Ids.Add(MakeShared<FJsonValueString>(Id.ToString()));
        People->SetBoolField(TEXT("available"),S.bKnownPeopleViewAvailable);
        People->SetArrayField(TEXT("ids"),Ids);
        People->SetStringField(TEXT("rule"),TEXT("仅已接触可观察人；非执行授权；false为unknown"));
        Facts->SetObjectField(TEXT("known_people"),People);
    }

    if(Tier==EHearthwardNPCContextTier::Full || OrderQuery)
    {
        auto Combat=MakeShared<FJsonObject>();
        Combat->SetStringField(TEXT("semantics"),TEXT("仅记录当前状态"));
        Combat->SetBoolField(TEXT("available"),S.bCombatViewAvailable);
        if(S.bCombatViewAvailable)
        {
            Combat->SetStringField(TEXT("requested_order"),S.RequestedOrder.ToString());
            Combat->SetStringField(TEXT("tactical_intent"),S.TacticalIntent.ToString());
            Combat->SetStringField(TEXT("reason"),S.CombatReason);
            Combat->SetBoolField(TEXT("player_in_combat"),S.bPlayerInCombat);
            Combat->SetBoolField(TEXT("routine_enabled"),S.bRoutineEnabled);
            Combat->SetStringField(TEXT("routine_activity"),S.RoutineActivity.ToString());
        }
        Facts->SetObjectField(TEXT("companion_state"),Combat);
    }
    else ContextProjectionAddDropped(Result.DroppedFields,TEXT("companion_state"));

    if(Tier==EHearthwardNPCContextTier::Full || (Tier==EHearthwardNPCContextTier::Compact && OrderQuery))
    {
        auto Profile=MakeShared<FJsonObject>();
        Profile->SetBoolField(TEXT("stable"),S.Coordination.Stable);
        Profile->SetNumberField(TEXT("samples"),S.Coordination.Samples);
        Profile->SetStringField(TEXT("preferred_directive"),S.Coordination.PreferredDirective.ToString());
        Profile->SetNumberField(TEXT("confidence"),S.Coordination.Confidence);
        Profile->SetStringField(TEXT("semantics"),TEXT("derived_recent_behavior_not_explicit_player_preference"));
        Facts->SetObjectField(TEXT("coordination_profile"),Profile);
    }
    else ContextProjectionAddDropped(Result.DroppedFields,TEXT("coordination_profile"));

    TArray<TSharedPtr<FJsonValue>> Beliefs;
    int32 KeptBeliefs=0;
    for(const auto& B:S.Memory.Beliefs)
    {
        const bool IsRelevant=Relevant.Contains(B.Item);
        // Camp-stock beliefs are authority-bearing evidence for inventory/report questions. A collect
        // request does not need the current camp quantity to interpret "newly acquire N", and injecting
        // that quantity makes the model conflate existing stock with the requested acquisition amount.
        bool Keep=InventoryQuery && IsRelevant;
        // "full_relevant" means full detail for relevant facts, not an unbounded dump of every belief.
        // For generic inventory questions without an item, keep a small deterministic sample.
        if(InventoryQuery && Relevant.IsEmpty() && Tier==EHearthwardNPCContextTier::Full)Keep=KeptBeliefs<8;
        if(InventoryQuery && Relevant.IsEmpty() && Tier==EHearthwardNPCContextTier::Compact)Keep=KeptBeliefs<6;
        if(Tier==EHearthwardNPCContextTier::Minimal && Relevant.IsEmpty())Keep=false;
        if(!Keep)continue;
        auto Row=MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("item"),B.Item.ToString());
        Row->SetNumberField(TEXT("quantity"),B.Value);
        Row->SetStringField(TEXT("source"),HearthwardBeliefs::SourceName(B.Source));
        Row->SetNumberField(TEXT("changed_at_game_seconds"),B.RecordedAt);
        Row->SetNumberField(TEXT("last_evidence_time"),B.LastEvidenceAt);
        const FString Freshness=B.Source==EHearthwardNPCBeliefSource::PlayerReport?TEXT("unverified_report"):(S.bAtCamp?TEXT("current_observation"):TEXT("possibly_stale"));
        Row->SetStringField(TEXT("freshness"),Freshness);
        Beliefs.Add(MakeShared<FJsonValueObject>(Row));++KeptBeliefs;
    }
    Facts->SetArrayField(TEXT("camp_stock_beliefs"),Beliefs);
    Facts->SetStringField(TEXT("belief_rule"),TEXT("player_report未核实；离营后firsthand/receipt也可能过期；未知数量保持unknown，不从世界真值偷看。"));
    if(KeptBeliefs<S.Memory.Beliefs.Num())ContextProjectionAddDropped(Result.DroppedFields,TEXT("unrelated_beliefs"));

    auto Episodes=HearthwardEpisodes::Build(S.Memory,3);
    TArray<TSharedPtr<FJsonValue>> EpisodeRows;
    int32 MaxEpisodes=Tier==EHearthwardNPCContextTier::Full?3:(Tier==EHearthwardNPCContextTier::Compact?2:1);
    if(!HistoryQuery && Tier==EHearthwardNPCContextTier::Minimal)MaxEpisodes=0;
    int32 Alias=1,AddedEpisodes=0;
    for(const auto& E:Episodes)
    {
        if(!Relevant.IsEmpty() && !Relevant.Contains(E.Item) && HistoryQuery)continue;
        if(AddedEpisodes>=MaxEpisodes)break;
        EpisodeRows.Add(MakeShared<FJsonValueObject>(ContextProjectionEpisodeJson(E,Alias)));++AddedEpisodes;
    }
    Facts->SetArrayField(TEXT("recent_episodes"),EpisodeRows);
    Facts->SetStringField(TEXT("episode_rule"),TEXT("complete才可把聚合数称为全过程总量；truncated/unknown只能说明保留记录可确认的部分。"));
    if(AddedEpisodes<Episodes.Num())ContextProjectionAddDropped(Result.DroppedFields,TEXT("older_episodes"));

    TArray<TSharedPtr<FJsonValue>> Records,Agreements;
    const auto Retrieved=S.Memory.Retrieve(Query,true);
    int32 RecordLimit=Tier==EHearthwardNPCContextTier::Full?3:(Tier==EHearthwardNPCContextTier::Compact?2:1);
    int32 RecordCount=0;
    for(const auto& R:Retrieved)
    {
        if(R.Kind==TEXT("agreement"))
        {
            if(Tier==EHearthwardNPCContextTier::Full || ContextProjectionTextRelevance(Query,HearthwardAgent::Normalize(R.Text))>0)
                Agreements.Add(MakeShared<FJsonValueString>(R.Text));
            continue;
        }
        if(RecordCount>=RecordLimit)continue;
        auto Row=MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("kind"),R.Kind.ToString());
        Row->SetStringField(TEXT("source"),TEXT("player_statement_unverified"));
        Row->SetStringField(TEXT("text"),R.Text);
        Row->SetNumberField(TEXT("recorded_at"),R.RecordedAt);
        Records.Add(MakeShared<FJsonValueObject>(Row));++RecordCount;
    }
    Facts->SetArrayField(TEXT("player_records"),Records);
    Facts->SetArrayField(TEXT("active_agreements"),Agreements);
    if(Records.Num()+Agreements.Num()<Retrieved.Num())ContextProjectionAddDropped(Result.DroppedFields,TEXT("unrelated_player_records"));

    // Hard rules and unresolved player constraints are never part of ordinary Top-K trimming.
    TArray<TSharedPtr<FJsonValue>> Prohibited;
    for(const auto& Item:HearthwardBasicItems())if(S.Memory.BlocksCollection(Item.Id))Prohibited.Add(MakeShared<FJsonValueString>(Item.Id.ToString()));
    Facts->SetArrayField(TEXT("collection_prohibited_items"),Prohibited);
    TArray<TSharedPtr<FJsonValue>> Rules;
    for(FName C:{FName(TEXT("collect")),FName(TEXT("craft")),FName(TEXT("repair"))})
        for(const auto& R:S.Memory.ApplicableRules(C))Rules.Add(MakeShared<FJsonValueString>(R));
    Facts->SetArrayField(TEXT("confirmed_rules"),Rules);
    Facts->SetStringField(TEXT("current_goal"),HearthwardAgent::GoalText(S.Memory.WorkingGoal));
    TArray<TSharedPtr<FJsonValue>> Unresolved;
    for(const auto& U:S.Memory.WorkingGoal.Unresolved)Unresolved.Add(MakeShared<FJsonValueString>(U));
    Facts->SetArrayField(TEXT("unresolved_original_constraints"),Unresolved);

    Facts->SetBoolField(TEXT("known_workbench"),S.bKnownWorkbench);
    Facts->SetStringField(TEXT("source_refs"),TEXT("S1=当前已知采集点；仓库材料取用仍需明确授权"));
    Facts->SetStringField(TEXT("capabilities_version"),TEXT("npc-v2"));

    Result.Json=ContextProjectionJson(Facts);
    return Result;
}
