#include "../AI/HearthwardAgentContract.h"
#include "../AI/HearthwardNPCMemory.h"
#include "../AI/HearthwardNPCPerception.h"
#include "../AI/HearthwardAgentPlan.h"
#include "../AI/HearthwardNPCSuggestions.h"
#include "../Gameplay/HearthwardCompanionCombatPolicy.h"
#include "../Companion/HearthwardCompanionCommand.h"
#include "../Building/HearthwardWorkshopService.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentContractTest,"Hearthward.NPCAgent.CapabilitiesAndLimits",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentContractTest::RunTest(const FString&)
{
    FHearthwardAgentGoal G;
    const FString Json=TEXT(R"({"intent":"collect","item":"wood","quantity":3,"mode":"additional_acquired","source":"S1","limits":[],"unresolved":[],"npc_line":"请确认"})");
    TestTrue(TEXT("Parse registered proposal"),HearthwardAgent::Parse(Json,G));
    TestTrue(TEXT("Semantically valid"),HearthwardAgent::Validate(G).IsEmpty());
    TestFalse(TEXT("No fractional quantities"),HearthwardAgent::Parse(Json.Replace(TEXT(":3,"),TEXT(":1.5,")),G));
    TestFalse(TEXT("No added authority field"),HearthwardAgent::Parse(Json.Replace(TEXT("{\"intent\""),TEXT("{\"execute\":true,\"intent\"")),G));
    G.SourceRef=TEXT("hidden_mountain");TestFalse(TEXT("Unknown source not remapped"),HearthwardAgent::Validate(G).IsEmpty());G.SourceRef=TEXT("S1");
    G.Unresolved={TEXT("只去北山")};TestEqual(TEXT("Unresolved blocks execution"),HearthwardAgent::Validate(G),FString(TEXT("UNRESOLVED_CONSTRAINT")));G.Unresolved.Reset();
    G.Limits={TEXT("ban:wood")};TestEqual(TEXT("Typed ban"),HearthwardAgent::Validate(G),FString(TEXT("POLICY_CONFLICT")));
    TestTrue(TEXT("Budget permits exact cumulative limit"),HearthwardAgent::AllowsCost({TEXT("max:wood:6")},{{TEXT("wood"),2}},{{TEXT("wood"),4}}));
    TestFalse(TEXT("Budget rejects cumulative excess"),HearthwardAgent::AllowsCost({TEXT("max:wood:6")},{{TEXT("wood"),3}},{{TEXT("wood"),4}}));
    TestFalse(TEXT("Forbidden material"),HearthwardAgent::AllowsCost({TEXT("no:herb")},{{TEXT("herb"),1}},{}));
    TestFalse(TEXT("Malformed constraint"),HearthwardAgent::ValidLimit(TEXT("max:wood:-1")));
    TestFalse(TEXT("Unknown material"),HearthwardAgent::ValidLimit(TEXT("no:secret")));
    for(const auto& C:HearthwardAgent::Capabilities())
        TestTrue(TEXT("Registry drives grammar"),HearthwardAgent::Schema().Contains(C.Id.ToString()));
    TestFalse(TEXT("Real crafting materials"),HearthwardWorkshop::Materials(TEXT("craft"),TEXT("arrows"),1).IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentReceiptTest,"Hearthward.NPCAgent.EffectReceiptsAndProgress",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentReceiptTest::RunTest(const FString&)
{
    TArray<FHearthwardAgentReceipt> R;const auto Id=FGuid::NewGuid(),Cmd=FGuid::NewGuid(),Epoch=FGuid::NewGuid();int32 Effects=0;
    auto Effect=[&]{++Effects;return true;};
    TestTrue(TEXT("First settlement"),HearthwardAgent::Settle(R,Id,Cmd,Epoch,Epoch,TEXT("wood:2"),Effect));
    for(int32 I=0;I<3;++I)TestTrue(TEXT("Same payload replay acknowledged"),HearthwardAgent::Settle(R,Id,Cmd,Epoch,Epoch,TEXT("wood:2"),Effect));
    TestEqual(TEXT("One effect"),Effects,1);
    TestFalse(TEXT("Id collision with other payload rejected"),HearthwardAgent::Settle(R,Id,Cmd,Epoch,Epoch,TEXT("wood:3"),Effect));
    TestFalse(TEXT("Old runtime epoch rejected even for receipt"),HearthwardAgent::Settle(R,Id,Cmd,Epoch,FGuid::NewGuid(),TEXT("wood:2"),Effect));
    auto Restored=R;TestTrue(TEXT("Restored receipt prevents replay in fresh valid epoch"),HearthwardAgent::Settle(Restored,Id,Cmd,Epoch,Epoch,TEXT("wood:2"),Effect));TestEqual(TEXT("Still one effect"),Effects,1);
    TestFalse(TEXT("Failed effect no receipt"),HearthwardAgent::Settle(R,FGuid::NewGuid(),Cmd,Epoch,Epoch,TEXT("fail"),[]{return false;}));TestEqual(TEXT("Only committed receipt"),R.Num(),1);
    FHearthwardCompanionCommand C;auto T=C.Request(Epoch);C.Accept(T,Epoch,TEXT("wood"),6,{TEXT("collect"),TEXT("return"),TEXT("deposit")});
    TestTrue(TEXT("Acquire four real units"),C.RecordAcquisition(4));TestEqual(TEXT("Acquiring not delivery"),C.GetDelivered(),0);
    TestFalse(TEXT("Cannot acquire over remainder"),C.RecordAcquisition(3));
    TestTrue(TEXT("Deposit real amount"),C.RecordDelivery(T,4));C.Carried-=4;TestTrue(TEXT("Remaining goal persists"),C.IsCurrent(Epoch));
    TestTrue(TEXT("Acquire remaining"),C.RecordAcquisition(2));TestEqual(TEXT("Additional units target"),C.GetAcquired(),6);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentMemoryTest,"Hearthward.NPCAgent.MemoryRevisionCapacityAndEvents",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentMemoryTest::RunTest(const FString&)
{
    FHearthwardNPCMemory M;M.Campaign=FGuid::NewGuid();
    for(int32 I=0;I<64;++I)TestTrue(TEXT("Active record capacity"),M.Put({},TEXT("claim"),FString::Printf(TEXT("记录%d"),I),0));
    auto Old=M.Records[0].Id;const auto Rev=M.Revision;TestTrue(TEXT("Revoke at capacity"),M.Revoke(Old));
    TestTrue(TEXT("Capacity immediately recycled"),M.Put({},TEXT("preference"),TEXT("清晨喜欢烤肉"),0));
    TestTrue(TEXT("Revision advances"),M.Revision>Rev);TestFalse(TEXT("Revoked ID never resurrected"),M.Put(Old,TEXT("claim"),TEXT("旧记录"),0));
    TestTrue(TEXT("Alias retrieval"),!M.Retrieve(TEXT("早晨爱吃什么"),false).IsEmpty());
    const auto Command=FGuid::NewGuid();FGuid Last;
    for(int32 I=0;I<140;++I){FHearthwardNPCEvent E;E.Id=FGuid::NewGuid();Last=E.Id;E.Command=Command;E.Campaign=M.Campaign;E.Kind=TEXT("acquired");E.Item=TEXT("wood");E.Count=1;M.RecordEvent(E);}
    TestEqual(TEXT("Bounded event view"),M.Events.Num(),128);M.RecordEvent(M.Events.Last());TestEqual(TEXT("No repeated last event"),M.Events.Num(),128);
    TestTrue(TEXT("Valid memory"),M.IsValid(0));M.Events.Last().Campaign=FGuid::NewGuid();TestFalse(TEXT("Foreign campaign rejected"),M.IsValid(0));
    FHearthwardNPCMemory Recycled;
    for(int32 I=0;I<128;++I)
    {TestTrue(TEXT("Repeated insert"),Recycled.Put({},TEXT("claim"),TEXT("可撤销记录"),0));const auto Id=Recycled.Records[0].Id;TestTrue(TEXT("Repeated revoke"),Recycled.Revoke(Id));TestFalse(TEXT("No old handle reuse"),Recycled.Put(Id,TEXT("claim"),TEXT("已撤销"),0));}
    TestTrue(TEXT("All capacity reclaimed"),Recycled.Records.IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentCombatPolicyTest,"Hearthward.NPCAgent.CompanionCombatPolicy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentCombatPolicyTest::RunTest(const FString&)
{
    FHearthwardCompanionCombatObservation O;
    O.CommandRange=1000;O.PlayerHealthRatio=1.0f;

    auto D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("Wait holds"),D.Intent==EHearthwardCompanionTacticalIntent::Hold && D.Reason==TEXT("EXPLICIT_HOLD"));

    O.RequestedOrder=TEXT("follow");
    D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("Follow ignores enemies"),D.Intent==EHearthwardCompanionTacticalIntent::Follow && D.Target.IsNone());

    O.RequestedOrder=TEXT("attack");O.CompanionToPlayerDistance=1200;
    FHearthwardCompanionThreat Wolf;Wolf.Id=TEXT("wolf");Wolf.RemainingHealth=10;Wolf.Position=FVector(100,0,0);
    O.Threats={Wolf};
    D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("Leash overrides assist"),D.Intent==EHearthwardCompanionTacticalIntent::Follow && D.Target.IsNone()
        && D.Reason==TEXT("RETURN_TO_PLAYER_LEASH"));

    O.CompanionToPlayerDistance=100;O.PlayerPosition=FVector::ZeroVector;O.CompanionPosition=FVector(500,0,0);
    FHearthwardCompanionThreat Far;Far.Id=TEXT("far_from_player");Far.RemainingHealth=10;Far.Position=FVector(700,0,0);
    FHearthwardCompanionThreat Near;Near.Id=TEXT("near_player");Near.RemainingHealth=10;Near.Position=FVector(200,0,0);
    O.Threats={Far,Near};
    D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("Threat nearest player has priority"),D.Target==TEXT("near_player") && D.Intent==EHearthwardCompanionTacticalIntent::Assist);

    FHearthwardCompanionThreat Dead;Dead.Id=TEXT("dead");Dead.RemainingHealth=0;Dead.Position=FVector(10,0,0);
    FHearthwardCompanionThreat Live;Live.Id=TEXT("live");Live.RemainingHealth=10;Live.Position=FVector(200,0,0);
    FHearthwardCompanionThreat Outside;Outside.Id=TEXT("outside");Outside.RemainingHealth=10;Outside.Position=FVector(1200,0,0);
    O.Threats={Dead,Live,Outside};
    D=HearthwardCombatPolicy::Evaluate(O);
    TestEqual(TEXT("Dead and out-of-range targets excluded"),D.Target,FName(TEXT("live")));

    FHearthwardCompanionThreat North;North.Id=TEXT("north");North.RemainingHealth=10;North.Position=FVector(0,200,0);
    FHearthwardCompanionThreat East;East.Id=TEXT("east");East.RemainingHealth=10;East.Position=FVector(200,0,0);
    O.Threats={North,East};
    D=HearthwardCombatPolicy::Evaluate(O);
    TestEqual(TEXT("Equal player distance prefers threat nearer companion"),D.Target,FName(TEXT("east")));

    FHearthwardCompanionThreat Beta;Beta.Id=TEXT("beta");Beta.RemainingHealth=10;Beta.Position=FVector(200,0,0);
    FHearthwardCompanionThreat Alpha;Alpha.Id=TEXT("alpha");Alpha.RemainingHealth=10;Alpha.Position=FVector(200,0,0);
    O.Threats={Beta,Alpha};
    D=HearthwardCombatPolicy::Evaluate(O);
    TestEqual(TEXT("Stable ID tie-break"),D.Target,FName(TEXT("alpha")));

    O.Threats={Outside};
    D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("No legal threat falls back to follow"),D.Intent==EHearthwardCompanionTacticalIntent::Follow && D.Target.IsNone());

    O.PlayerHealthRatio=0;
    D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("Down player stops combat directive"),D.Intent==EHearthwardCompanionTacticalIntent::Hold);

    FHearthwardAgentGoal Goal;Goal.Intent=TEXT("companion_order");Goal.Item=TEXT("assist");Goal.Quantity=1;
    Goal.QuantityMode=TEXT("directive");Goal.SourceRef=TEXT("player");
    TestTrue(TEXT("Companion order is a confirmed world-write capability"),Goal.WritesWorld());
    TestTrue(TEXT("Valid companion directive passes contract"),HearthwardAgent::Validate(Goal).IsEmpty());
    Goal.Item=TEXT("wolf");
    TestEqual(TEXT("Model cannot name an arbitrary target"),HearthwardAgent::Validate(Goal),FString(TEXT("UNSUPPORTED_CAPABILITY")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentSuggestionTest,"Hearthward.NPCAgent.ContextualSuggestions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentSuggestionTest::RunTest(const FString&)
{
    FHearthwardSuggestionContext C;
    C.bCanCollectWood=true;C.bHasCampWood=true;C.CampWood=7;
    auto S=HearthwardSuggestions::Build(C);
    TestEqual(TEXT("Exactly three suggestions"),S.Num(),3);
    TestEqual(TEXT("Idle safe context suggests collection"),S[0].Kind,FName(TEXT("collect")));
    TestTrue(TEXT("Camp fact stays in global suggestion text"),S[1].Message.Contains(TEXT("7")));
    TestEqual(TEXT("Camp suggestion tagged"),S[1].Kind,FName(TEXT("camp_stock")));

    C.bHasActiveCommand=true;
    S=HearthwardSuggestions::Build(C);
    TestEqual(TEXT("Active command gets progress suggestion"),S[0].Kind,FName(TEXT("progress")));
    TestFalse(TEXT("Active command never suggests replacement collect"),S.ContainsByPredicate([](const auto& X){return X.Kind==TEXT("collect");}));

    C={};
    S=HearthwardSuggestions::Build(C);
    TestEqual(TEXT("No world facts still produces bounded safe set"),S.Num(),3);
    TestFalse(TEXT("Unknown camp stock is not fabricated"),S.ContainsByPredicate([](const auto& X){return X.Kind==TEXT("camp_stock");}));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentPlanTest,"Hearthward.NPCAgent.TypedPlanCompilation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentPlanTest::RunTest(const FString&)
{
    auto Goal=[](FName Intent,const TCHAR* Source)
    {
        FHearthwardAgentGoal G;G.Intent=Intent;G.Item=Intent==TEXT("repair")?TEXT("axe"):Intent==TEXT("craft")?TEXT("arrows"):TEXT("wood");
        G.Quantity=Intent==TEXT("repair")?1:2;G.QuantityMode=Intent==TEXT("repair")?TEXT("one_owned"):Intent==TEXT("craft")?TEXT("batches"):TEXT("additional_acquired");
        G.SourceRef=Source;return G;
    };
    FString Error;FHearthwardAgentPlan Plan;

    TestTrue(TEXT("Collect plan builds"),HearthwardPlan::Build(Goal(TEXT("collect"),TEXT("S1")),Plan,Error));
    TestEqual(TEXT("Collect action count"),Plan.Actions.Num(),4);
    TestTrue(TEXT("Collect move source"),Plan.Actions[0].Matches(EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Source));
    TestTrue(TEXT("Collect gather"),Plan.Actions[1].Matches(EHearthwardAgentActionType::Gather,EHearthwardAgentTarget::Source));
    TestTrue(TEXT("Collect return"),Plan.Actions[2].Matches(EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Camp));
    TestTrue(TEXT("Collect deposit"),Plan.Actions[3].Matches(EHearthwardAgentActionType::Deposit,EHearthwardAgentTarget::Camp));

    TestTrue(TEXT("Bag craft plan builds"),HearthwardPlan::Build(Goal(TEXT("craft"),TEXT("bag")),Plan,Error));
    TestEqual(TEXT("Bag craft count"),Plan.Actions.Num(),4);
    TestTrue(TEXT("Bag craft starts at workshop"),Plan.Actions[0].Matches(EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Workshop));
    TestTrue(TEXT("Bag craft commits"),Plan.Actions[1].Matches(EHearthwardAgentActionType::CommitWorkshop,EHearthwardAgentTarget::Workshop));
    TestTrue(TEXT("Bag craft deposits output"),Plan.Actions[3].Matches(EHearthwardAgentActionType::Deposit,EHearthwardAgentTarget::Camp));

    TestTrue(TEXT("Camp craft plan builds"),HearthwardPlan::Build(Goal(TEXT("craft"),TEXT("camp")),Plan,Error));
    TestEqual(TEXT("Camp craft count"),Plan.Actions.Num(),6);
    TestTrue(TEXT("Camp craft first returns"),Plan.Actions[0].Matches(EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Camp));
    TestTrue(TEXT("Camp craft takes authorized materials"),Plan.Actions[1].Matches(EHearthwardAgentActionType::TakeMaterials,EHearthwardAgentTarget::Camp));
    TestEqual(TEXT("Camp craft delivery uses final camp move"),HearthwardPlan::FindLast(Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Camp),4);

    TestTrue(TEXT("Bag repair plan builds"),HearthwardPlan::Build(Goal(TEXT("repair"),TEXT("bag")),Plan,Error));
    TestEqual(TEXT("Bag repair count"),Plan.Actions.Num(),2);
    TestTrue(TEXT("Bag repair has no deposit"),HearthwardPlan::Find(Plan,EHearthwardAgentActionType::Deposit)==INDEX_NONE);

    TestTrue(TEXT("Camp repair plan builds"),HearthwardPlan::Build(Goal(TEXT("repair"),TEXT("camp")),Plan,Error));
    TestEqual(TEXT("Camp repair count"),Plan.Actions.Num(),4);
    TestTrue(TEXT("Camp repair takes materials"),Plan.Actions[1].Matches(EHearthwardAgentActionType::TakeMaterials,EHearthwardAgentTarget::Camp));

    auto Unsupported=Goal(TEXT("collect"),TEXT("S1"));Unsupported.Intent=TEXT("dialogue");
    TestFalse(TEXT("Read-only intent has no executable plan"),HearthwardPlan::Build(Unsupported,Plan,Error));
    TestTrue(TEXT("Unsupported plan returns reason"),!Error.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentPerceptionSafetyTest,"Hearthward.NPCAgent.PerceptionSafety",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentPerceptionSafetyTest::RunTest(const FString&)
{
    FHearthwardNPCObservation O;
    O.bWorldAvailable=true;O.bCombatStateAvailable=true;O.bCampAvailable=true;O.bCollectionSourceAvailable=true;O.bCollectionSourceTrustedSafe=true;

    FHearthwardAgentGoal Collect;
    Collect.Intent=TEXT("collect");Collect.Item=TEXT("wood");Collect.Quantity=4;
    Collect.QuantityMode=TEXT("additional_acquired");Collect.SourceRef=TEXT("S1");
    auto D=HearthwardPerception::Evaluate(O,Collect);
    TestTrue(TEXT("Known safe collection is allowed"),D.IsAllowed());

    O.bCombatActive=true;
    D=HearthwardPerception::Evaluate(O,Collect);
    TestTrue(TEXT("Active combat is unsafe"),D.Verdict==EHearthwardNPCSafetyVerdict::Unsafe);
    TestEqual(TEXT("Combat reason is deterministic"),D.Reason,FString(TEXT("ACTIVE_COMBAT")));
    O.bCombatActive=false;

    O.bCollectionSourceTrustedSafe=false;
    D=HearthwardPerception::Evaluate(O,Collect);
    TestTrue(TEXT("Untrusted source is unsafe"),D.Verdict==EHearthwardNPCSafetyVerdict::Unsafe);
    O.bCollectionSourceTrustedSafe=true;
    O.bNavigationRebuilding=true;
    D=HearthwardPerception::Evaluate(O,Collect);
    TestTrue(TEXT("Navigation rebuild is observed but executor may wait"),D.IsAllowed());
    O.bNavigationRebuilding=false;O.bCollectionSourceAvailable=false;
    D=HearthwardPerception::Evaluate(O,Collect);
    TestTrue(TEXT("Missing collection source is unavailable"),D.Verdict==EHearthwardNPCSafetyVerdict::Unavailable);
    TestEqual(TEXT("Missing source reason"),D.Reason,FString(TEXT("SOURCE_UNAVAILABLE")));

    O.bCampAvailable=true;
    FHearthwardAgentGoal Craft;
    Craft.Intent=TEXT("craft");Craft.Item=TEXT("arrows");Craft.Quantity=2;
    Craft.QuantityMode=TEXT("batches");Craft.SourceRef=TEXT("bag");
    D=HearthwardPerception::Evaluate(O,Craft);
    TestTrue(TEXT("Own-bag craft does not depend on collection source"),D.IsAllowed());

    O.bCampAvailable=false;
    D=HearthwardPerception::Evaluate(O,Craft);
    TestTrue(TEXT("Craft still requires the delivery camp"),D.Verdict==EHearthwardNPCSafetyVerdict::Unavailable);
    TestEqual(TEXT("Missing craft camp reason"),D.Reason,FString(TEXT("CAMP_UNAVAILABLE")));

    FHearthwardAgentGoal Repair;
    Repair.Intent=TEXT("repair");Repair.Item=TEXT("axe");Repair.Quantity=1;
    Repair.QuantityMode=TEXT("one_owned");Repair.SourceRef=TEXT("bag");
    D=HearthwardPerception::Evaluate(O,Repair);
    TestTrue(TEXT("Own-bag repair does not depend on collection source or camp"),D.IsAllowed());
    Repair.SourceRef=TEXT("camp");
    D=HearthwardPerception::Evaluate(O,Repair);
    TestTrue(TEXT("Camp-authorized repair materials require a camp"),D.Verdict==EHearthwardNPCSafetyVerdict::Unavailable);
    TestEqual(TEXT("Missing repair camp reason"),D.Reason,FString(TEXT("CAMP_UNAVAILABLE")));

    O.bPaused=true;
    Craft.SourceRef=TEXT("bag");
    D=HearthwardPerception::Evaluate(O,Craft);
    TestTrue(TEXT("Paused world blocks new world writes"),D.Verdict==EHearthwardNPCSafetyVerdict::Unavailable);
    O.bPaused=false;O.bCombatStateAvailable=false;
    D=HearthwardPerception::Evaluate(O,Craft);
    TestTrue(TEXT("Unknown combat state fails closed"),D.Verdict==EHearthwardNPCSafetyVerdict::Unavailable);
    TestEqual(TEXT("Missing safety-state reason"),D.Reason,FString(TEXT("SAFETY_STATE_UNAVAILABLE")));

    FHearthwardAgentGoal ReadOnly;ReadOnly.Intent=TEXT("inventory");
    D=HearthwardPerception::Evaluate(O,ReadOnly);
    TestTrue(TEXT("Read-only reasoning is not promoted to a world write"),D.IsAllowed());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentWorkshopTest,"Hearthward.NPCAgent.OwnBagWorkshopTransactions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentWorkshopTest::RunTest(const FString&)
{
    auto* NPC=NewObject<UHearthwardInventoryComponent>();auto* Player=NewObject<UHearthwardInventoryComponent>();
    Player->TryAdd(TEXT("wood"),10);NPC->TryAdd(TEXT("wood"),3);
    TestTrue(TEXT("NPC craft debits own bag"),HearthwardWorkshop::Commit(NPC,nullptr,TEXT("craft"),TEXT("arrows"),2));
    TestEqual(TEXT("Actual NPC arrows"),NPC->GetItemCount(TEXT("arrow")),8);TestEqual(TEXT("Player wood unchanged"),Player->GetItemCount(TEXT("wood")),10);
    TestFalse(TEXT("Material disappearance rejects craft"),HearthwardWorkshop::Commit(NPC,nullptr,TEXT("craft"),TEXT("arrows"),2));TestEqual(TEXT("No partial output"),NPC->GetItemCount(TEXT("arrow")),8);
    NPC->TryAdd(TEXT("axe"),1);NPC->TryAdd(TEXT("wood"),1);TMap<FName,float> Dur={{TEXT("axe"),20}};
    TestFalse(TEXT("Missing rope rolls back durability and wood"),HearthwardWorkshop::Commit(NPC,&Dur,TEXT("repair"),TEXT("axe"),1));TestEqual(TEXT("Original durability"),Dur[TEXT("axe")],20.f);TestEqual(TEXT("No partial repair cost"),NPC->GetItemCount(TEXT("wood")),2);
    NPC->TryAdd(TEXT("rope"),1);TestTrue(TEXT("Own equipment repaired"),HearthwardWorkshop::Commit(NPC,&Dur,TEXT("repair"),TEXT("axe"),1));TestEqual(TEXT("Repair wood cost"),NPC->GetItemCount(TEXT("wood")),0);
    TestFalse(TEXT("Completed repair cannot settle again"),HearthwardWorkshop::Commit(NPC,&Dur,TEXT("repair"),TEXT("axe"),1));
    TestEqual(TEXT("Still no player debit"),Player->GetItemCount(TEXT("wood")),10);return true;
}
#endif
