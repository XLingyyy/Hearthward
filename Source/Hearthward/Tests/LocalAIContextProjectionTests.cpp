#include "../AI/HearthwardNPCContextProjection.h"
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
TSharedPtr<FJsonObject> ProjectedContext(const FHearthwardNPCContextSnapshot& Snapshot,EHearthwardNPCContextTier Tier)
{
    TSharedPtr<FJsonObject> Object;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(HearthwardContextProjection::Project(Snapshot,Tier).Json),Object);
    return Object;
}
const TArray<EHearthwardNPCContextTier> ContextTiers={EHearthwardNPCContextTier::Full,EHearthwardNPCContextTier::Compact,EHearthwardNPCContextTier::Minimal};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalAIProjectionOwnershipTest,"Hearthward.Iteration.Task087.Projection.OwnershipEveryTier",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLocalAIProjectionOwnershipTest::RunTest(const FString&)
{
    FHearthwardNPCContextSnapshot S;S.Query=TEXT("把你背包里的货物交给我");
    for(auto Tier:ContextTiers)
    {
        const auto O=ProjectedContext(S,Tier);
        if(!TestTrue(TEXT("projection parses"),O.IsValid()))continue;
        FString Speaker,Listener,Pronouns;
        TestTrue(TEXT("player is the utterance speaker"),O->TryGetStringField(TEXT("speaker"),Speaker) && Speaker==TEXT("player"));
        TestTrue(TEXT("brother is the utterance listener"),O->TryGetStringField(TEXT("listener"),Listener) && Listener==TEXT("brother"));
        TestTrue(TEXT("input pronouns retain speaker perspective"),O->TryGetStringField(TEXT("pronouns"),Pronouns)
            && Pronouns.Contains(TEXT("我=player")) && Pronouns.Contains(TEXT("你=brother")));
        const TSharedPtr<FJsonObject>* Containers=nullptr;
        if(!TestTrue(TEXT("container ownership survives degradation"),O->TryGetObjectField(TEXT("containers"),Containers)))continue;
        TestEqual(TEXT("bag belongs to brother"),(*Containers)->GetStringField(TEXT("bag")),FString(TEXT("弟弟背包")));
        TestEqual(TEXT("player_bag belongs to player"),(*Containers)->GetStringField(TEXT("player_bag")),FString(TEXT("玩家背包")));
        TestEqual(TEXT("camp is the shared warehouse"),(*Containers)->GetStringField(TEXT("camp")),FString(TEXT("当前营地共享仓库")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalAIProjectionRepairMultiplicityTest,"Hearthward.Iteration.Task087.Projection.RepairMultiplicity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLocalAIProjectionRepairMultiplicityTest::RunTest(const FString&)
{
    FHearthwardNPCContextSnapshot S;S.Query=TEXT("修好你自己的斧头");
    S.bOwnBagViewAvailable=true;S.OwnBag.Add(TEXT("axe"),2);
    for(auto Tier:ContextTiers)
    {
        const auto O=ProjectedContext(S,Tier);
        const TSharedPtr<FJsonObject>* Owned=nullptr;
        if(!TestTrue(TEXT("colloquial repair retains owned candidates"),O.IsValid() && O->TryGetObjectField(TEXT("owned_repair_candidates"),Owned)))continue;
        TestTrue(TEXT("owned bag view is known"),(*Owned)->GetBoolField(TEXT("available")));
        TestEqual(TEXT("two instances never become one after trimming"),(*Owned)->GetObjectField(TEXT("counts"))->GetIntegerField(TEXT("axe")),2);
        TestFalse(TEXT("projection does not choose an equipment instance"),(*Owned)->HasField(TEXT("selected_instance")));
    }
    S.OwnBag[TEXT("axe")]=1;
    const auto Unique=ProjectedContext(S,EHearthwardNPCContextTier::Minimal);
    const TSharedPtr<FJsonObject>* Owned=nullptr;
    if(TestTrue(TEXT("new repair snapshot remains available"),Unique.IsValid() && Unique->TryGetObjectField(TEXT("owned_repair_candidates"),Owned)))
        TestEqual(TEXT("new snapshot updates the owned count"),(*Owned)->GetObjectField(TEXT("counts"))->GetIntegerField(TEXT("axe")),1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalAIProjectionKnownObjectsTest,"Hearthward.Iteration.Task087.Projection.KnownObjectsAndUnknownViews",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLocalAIProjectionKnownObjectsTest::RunTest(const FString&)
{
    FHearthwardNPCContextSnapshot S;S.Query=TEXT("照顾作物和鱼点并护送族人");
    S.bKnownTargetsViewAvailable=true;S.KnownTargetCounts.Add(TEXT("nature_care:water"),2);S.KnownTargetCounts.Add(TEXT("fish:fish"),0);
    S.KnownTargetCounts.Add(TEXT("hunt:boar"),3);
    S.bKnownPeopleViewAvailable=true;S.KnownPeople={TEXT("rescued_01")};
    for(auto Tier:ContextTiers)
    {
        const auto O=ProjectedContext(S,Tier);
        const TSharedPtr<FJsonObject>* Targets=nullptr;const TSharedPtr<FJsonObject>* People=nullptr;
        if(!TestTrue(TEXT("observed candidates survive every tier"),O.IsValid() && O->TryGetObjectField(TEXT("known_targets"),Targets)
            && O->TryGetObjectField(TEXT("known_people"),People)))continue;
        TestTrue(TEXT("candidate view is known"),(*Targets)->GetBoolField(TEXT("available")));
        TestEqual(TEXT("multiple observed crops remain multiple"),(*Targets)->GetObjectField(TEXT("counts"))->GetIntegerField(TEXT("nature_care:water")),2);
        TestEqual(TEXT("observed absence is explicit zero"),(*Targets)->GetObjectField(TEXT("counts"))->GetIntegerField(TEXT("fish:fish")),0);
        TestFalse(TEXT("unrelated candidates do not consume the projected budget"),(*Targets)->GetObjectField(TEXT("counts"))->HasField(TEXT("hunt:boar")));
        const auto Ids=(*People)->GetArrayField(TEXT("ids"));
        TestEqual(TEXT("only captured contacted people are included"),Ids.Num(),1);
        if(Ids.Num()==1)TestEqual(TEXT("known person's stable ID is retained"),Ids[0]->AsString(),FString(TEXT("rescued_01")));
    }
    S.bKnownTargetsViewAvailable=false;S.bKnownPeopleViewAvailable=false;
    const auto O=ProjectedContext(S,EHearthwardNPCContextTier::Minimal);
    const TSharedPtr<FJsonObject>* Targets=nullptr;const TSharedPtr<FJsonObject>* People=nullptr;
    if(TestTrue(TEXT("unavailable views retain explicit unknown frames"),O.IsValid() && O->TryGetObjectField(TEXT("known_targets"),Targets) && O->TryGetObjectField(TEXT("known_people"),People)))
    {
        TestFalse(TEXT("unavailable target view stays unknown"),(*Targets)->GetBoolField(TEXT("available")));
        TestEqual(TEXT("unknown view does not expose stale candidate counts"),(*Targets)->GetObjectField(TEXT("counts"))->Values.Num(),0);
        TestFalse(TEXT("unavailable people view stays unknown"),(*People)->GetBoolField(TEXT("available")));
        TestEqual(TEXT("unknown view does not expose stale people IDs"),(*People)->GetArrayField(TEXT("ids")).Num(),0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalAIProjectionOldStateTest,"Hearthward.Iteration.Task087.Projection.CurrentStateIsNotNewDirective",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLocalAIProjectionOldStateTest::RunTest(const FString&)
{
    FHearthwardNPCContextSnapshot S;S.Query=TEXT("帮我对付附近的威胁");
    S.bCombatViewAvailable=true;S.RequestedOrder=TEXT("wait");S.TacticalIntent=TEXT("hold");
    S.CampTeamStatus=TEXT("木材队伍正在工作");
    for(auto Tier:ContextTiers)
    {
        const auto O=ProjectedContext(S,Tier);
        if(!TestTrue(TEXT("projection parses"),O.IsValid()))continue;
        TestFalse(TEXT("unrelated old task is trimmed before new directive"),O->HasField(TEXT("current_task")));
        TestFalse(TEXT("unrelated work party does not compete with directive"),O->HasField(TEXT("camp_team")));
        const TSharedPtr<FJsonObject>* State=nullptr;FString Semantics;
        if(!TestTrue(TEXT("current combat view is retained as an observation"),O->TryGetObjectField(TEXT("companion_state"),State)))continue;
        TestTrue(TEXT("current order is explicitly different from new command"),(*State)->TryGetStringField(TEXT("semantics"),Semantics)
            && Semantics.Contains(TEXT("仅记录当前状态")));
        TestEqual(TEXT("observed old order is not rewritten"),(*State)->GetStringField(TEXT("requested_order")),FString(TEXT("wait")));
    }
    S.Query=TEXT("还差多少，为什么停了？");S.bHasActiveTask=true;
    S.PreviousGoalQuantity=8;S.PreviousGoalDelivered=3;S.TaskCarried=2;S.TaskBlockReason=TEXT("资源不足");
    for(auto Tier:ContextTiers)
    {
        const auto O=ProjectedContext(S,Tier);const TSharedPtr<FJsonObject>* Task=nullptr;
        if(!TestTrue(TEXT("status query retains task in every tier"),O.IsValid() && O->TryGetObjectField(TEXT("current_task"),Task)))continue;
        TestEqual(TEXT("status remaining retains authoritative arithmetic"),(*Task)->GetIntegerField(TEXT("remaining")),5);
        TestEqual(TEXT("status carried is not delivered"),(*Task)->GetIntegerField(TEXT("carried")),2);
        TestEqual(TEXT("status retains actual reason"),(*Task)->GetStringField(TEXT("block_reason")),S.TaskBlockReason);
    }
    return true;
}
#endif
