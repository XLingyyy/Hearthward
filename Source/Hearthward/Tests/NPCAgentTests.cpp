#include "../AI/HearthwardAgentContract.h"
#include "../AI/HearthwardNPCMemory.h"
#include "../AI/HearthwardNPCPerception.h"
#include "../AI/HearthwardAgentPlan.h"
#include "../AI/HearthwardAgentRecovery.h"
#include "../AI/HearthwardNPCSuggestions.h"
#include "../AI/HearthwardNPCInitiative.h"
#include "../AI/HearthwardNPCEpisode.h"
#include "../AI/HearthwardNPCCoordination.h"
#include "../AI/HearthwardNPCContextProjection.h"
#include "../AI/HearthwardNPCRoutine.h"
#include "../Gameplay/HearthwardCompanionCombatPolicy.h"
#include "../Companion/HearthwardCompanionCommand.h"
#include "../Building/HearthwardWorkshopService.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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
    const auto* Store=HearthwardAgent::FindCapability(TEXT("store"));
    TestTrue(TEXT("Store exposes ordinary stackable cargo"),Store && Store->Items.Contains(TEXT("wood")));
    TestTrue(TEXT("Store identifies authorized player handoff separately"),Store && Store->Sources.Contains(TEXT("player_bag")));
    TestFalse(TEXT("Protected quest item cannot be delegated to storage"),Store && Store->Items.Contains(TEXT("amulet")));
    const auto* Retrieve=HearthwardAgent::FindCapability(TEXT("retrieve"));
    TestTrue(TEXT("Camp-to-player cargo is a distinct typed route"),Retrieve && Retrieve->Items.Contains(TEXT("wood"))
        && Retrieve->Sources==TArray<FString>{TEXT("camp")} && Retrieve->QuantityMode==TEXT("camp_to_player"));
    const auto* Give=HearthwardAgent::FindCapability(TEXT("give"));
    TestTrue(TEXT("Brother bag can be explicitly handed to player"),Give && Give->Items.Contains(TEXT("wood"))
        && Give->Sources==TArray<FString>{TEXT("bag")} && Give->QuantityMode==TEXT("bag_to_player"));
    const auto* Fetch=HearthwardAgent::FindCapability(TEXT("fetch"));
    TestTrue(TEXT("Camp stock can be explicitly kept in brother bag"),Fetch && Fetch->Items.Contains(TEXT("wood"))
        && Fetch->Sources==TArray<FString>{TEXT("camp")} && Fetch->QuantityMode==TEXT("camp_to_bag"));
    const auto* Receive=HearthwardAgent::FindCapability(TEXT("receive"));
    TestTrue(TEXT("Player cargo can be explicitly received into brother bag"),Receive && Receive->Items.Contains(TEXT("wood"))
        && Receive->Sources==TArray<FString>{TEXT("player_bag")} && Receive->QuantityMode==TEXT("player_to_bag"));
    FHearthwardAgentGoal Care;Care.Intent=TEXT("nature_care");Care.Item=TEXT("water");Care.Quantity=1;
    Care.QuantityMode=TEXT("action_count");Care.SourceRef=TEXT("known_target");
    TestTrue(TEXT("Single crop action has a typed contract"),HearthwardAgent::Validate(Care).IsEmpty());
    Care.Quantity=2;TestFalse(TEXT("Crop action cannot silently become a batch"),HearthwardAgent::Validate(Care).IsEmpty());
    FHearthwardAgentGoal Resource;Resource.Intent=TEXT("nature_collect");Resource.Item=TEXT("stone");Resource.Quantity=4;
    Resource.QuantityMode=TEXT("additional_acquired");Resource.SourceRef=TEXT("known_target");
    TestTrue(TEXT("Known resource has a typed collection contract"),HearthwardAgent::Validate(Resource).IsEmpty() && Resource.WritesWorld());
    Resource.Limits={TEXT("ban:stone")};TestEqual(TEXT("Collection ban covers nature resources"),HearthwardAgent::Validate(Resource),FString(TEXT("POLICY_CONFLICT")));
    TestTrue(TEXT("Explicit one-time allowance has a distinct type"),HearthwardAgent::ValidLimit(TEXT("once:herb")));
    TSharedPtr<FJsonObject> SchemaRoot;
    const TArray<TSharedPtr<FJsonValue>>* Branches=nullptr;
    TestTrue(TEXT("Schema parses structurally"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(HearthwardAgent::Schema()),SchemaRoot)
        && SchemaRoot.IsValid() && SchemaRoot->TryGetArrayField(TEXT("oneOf"),Branches));
    if(Branches)
    {
        TestEqual(TEXT("Every registered capability has one schema branch"),Branches->Num(),HearthwardAgent::Capabilities().Num());
        for(int32 I=0;I<FMath::Min(Branches->Num(),HearthwardAgent::Capabilities().Num());++I)
        {
            const auto& C=HearthwardAgent::Capabilities()[I];
            const auto Branch=(*Branches)[I]->AsObject();
            const TSharedPtr<FJsonObject>* Properties=nullptr;
            TestTrue(TEXT("Capability branch has properties"),Branch.IsValid() && Branch->TryGetObjectField(TEXT("properties"),Properties));
            if(!Properties)continue;
            const auto IntentSchema=(*Properties)->GetObjectField(TEXT("intent"));
            const auto ItemSchema=(*Properties)->GetObjectField(TEXT("item"));
            const auto ModeSchema=(*Properties)->GetObjectField(TEXT("mode"));
            const auto SourceSchema=(*Properties)->GetObjectField(TEXT("source"));
            const auto& IntentEnum=IntentSchema->GetArrayField(TEXT("enum"));
            const auto& ItemEnum=ItemSchema->GetArrayField(TEXT("enum"));
            const auto& ModeEnum=ModeSchema->GetArrayField(TEXT("enum"));
            const auto& SourceEnum=SourceSchema->GetArrayField(TEXT("enum"));
            TestTrue(TEXT("Schema intent comes from registry"),IntentEnum.Num()==1 && IntentEnum[0]->AsString()==C.Id.ToString());
            TestEqual(TEXT("Schema item count comes from registry"),ItemEnum.Num(),C.Items.Num());
            TestTrue(TEXT("Schema mode comes from registry"),ModeEnum.Num()==1 && ModeEnum[0]->AsString()==C.QuantityMode);
            TestEqual(TEXT("Schema source count comes from registry"),SourceEnum.Num(),C.Sources.Num());
        }
    }

    const auto* Order=HearthwardAgent::FindCapability(TEXT("companion_order"));
    TestTrue(TEXT("Companion order capability registered"),Order!=nullptr);
    if(Order)
    {
        const FString Prompt=HearthwardAgent::CompanionOrderPrompt();
        for(FName Directive:Order->Items)
        {
            TestTrue(TEXT("Prompt includes every registered directive"),Prompt.Contains(Directive.ToString()));
            FHearthwardAgentGoal OrderGoal;OrderGoal.Intent=TEXT("companion_order");OrderGoal.Item=Directive;OrderGoal.Quantity=1;
            OrderGoal.QuantityMode=Order->QuantityMode;OrderGoal.SourceRef=Order->Sources[0];
            TestTrue(TEXT("Every registered directive passes the same preflight contract"),HearthwardAgent::Validate(OrderGoal).IsEmpty());
        }
        TestTrue(TEXT("Routine is not prompt-forbidden drift"),Order->Items.Contains(TEXT("routine")) && Prompt.Contains(TEXT("routine")));
    }

    FHearthwardAgentGoal Report;Report.Intent=TEXT("inventory_report");Report.Item=TEXT("wood");Report.Quantity=20;
    Report.QuantityMode=TEXT("reported_exact");Report.SourceRef=TEXT("player");
    TestTrue(TEXT("Inventory report is valid cognition-only input"),HearthwardAgent::Validate(Report).IsEmpty() && !Report.WritesWorld());
    FHearthwardAgentGoal Query;Query.Intent=TEXT("inventory");Query.Item=TEXT("wood");Query.Quantity=0;
    Query.QuantityMode=TEXT("none");Query.SourceRef=TEXT("none");
    TestTrue(TEXT("Inventory query is distinct read-only contract"),HearthwardAgent::Validate(Query).IsEmpty() && !Query.WritesWorld()
        && Query.QuantityMode!=Report.QuantityMode && Query.SourceRef!=Report.SourceRef);
    const FString Description=HearthwardAgent::Describe();
    TestTrue(TEXT("Capability description keeps localized recipe identity"),Description.Contains(TEXT("箭矢[arrows]")));
    TestFalse(TEXT("Generic capability description does not inject recipe quantities"),Description.Contains(TEXT("每批消耗")) || Description.Contains(TEXT("每批产出")));
    TestFalse(TEXT("Real crafting materials"),HearthwardWorkshop::Materials(TEXT("craft"),TEXT("arrows"),1).IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentOriginalQuantityTest,"Hearthward.NPCAgent.OriginalQuantityBoundary",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentOriginalQuantityTest::RunTest(const FString&)
{
    FHearthwardAgentGoal G;G.Intent=TEXT("collect");G.Item=TEXT("wood");G.Quantity=32;
    G.QuantityMode=TEXT("additional_acquired");G.SourceRef=TEXT("S1");
    G.Original=TEXT("新采三十三份木材带回仓库");
    TestFalse(TEXT("Explicit Chinese 33 cannot become a legal 32-unit proposal"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("新采33份木材带回仓库");TestFalse(TEXT("ASCII 33 cannot become 32"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("新采132份木材带回仓库");TestFalse(TEXT("32 is not a substring quantity of 132"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("新采一百三十二份木材");TestFalse(TEXT("A large Chinese token is not reduced to its suffix"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("新采三十二份木材带回仓库");TestTrue(TEXT("The exact supported Chinese target remains valid"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("新采32份木材带回仓库");TestTrue(TEXT("The exact ASCII target remains valid"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Quantity=2;G.Original=TEXT("新采十二份木材");TestFalse(TEXT("Two is not the suffix of twelve"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("新采两份木头");TestTrue(TEXT("Existing item alias and Chinese two remain supported"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=HearthwardAgent::GoalText(G);TestTrue(TEXT("Structured GoalText item-times-quantity remains supported"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Quantity=3;G.Original=TEXT("采些木材，最多消耗三份木材");TestFalse(TEXT("Consumption quantity cannot fill a missing target"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("采些木材\n补充：三份");TestTrue(TEXT("Explicit quantity clarification fills the known item"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Quantity=4;G.Original=TEXT("采些木材\n补充：4");TestTrue(TEXT("Existing bare ASCII quantity clarification remains supported"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("采些木材\n补充：四份。");TestTrue(TEXT("A declarative period does not reject explicit quantity clarification"),HearthwardAgent::OriginalQuantityMatches(G));G.Quantity=3;
    G.Original=TEXT("请采木材，数量为三");TestTrue(TEXT("Existing explicit quantity statement remains supported"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("新采二份木材\n补充：三份");TestFalse(TEXT("Conflicting original and clarification quantities need review"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("新采二份木材，再采三份木材");TestFalse(TEXT("Multiple target quantities cannot be silently selected"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Intent=TEXT("craft");G.Item=TEXT("arrows");G.QuantityMode=TEXT("batches");G.SourceRef=TEXT("bag");
    G.Original=TEXT("制作三批箭矢，最多消耗三份木材");
    TestTrue(TEXT("Craft batches and a same-valued material limit are distinct slots"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("制作三批箭矢，最多消耗五份木材");TestTrue(TEXT("A different material limit does not replace the target batches"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Quantity=5;TestFalse(TEXT("The material budget cannot become five craft batches"),HearthwardAgent::OriginalQuantityMatches(G));G.Quantity=3;
    G.Original=TEXT("制作些箭矢，最多消耗三份木材");TestFalse(TEXT("Only a consumption number supplies no craft batches"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=TEXT("制作三支箭矢");TestFalse(TEXT("Product pieces cannot be relabeled as batches"),HearthwardAgent::OriginalQuantityMatches(G));
    G.Original=HearthwardAgent::GoalText(G);TestTrue(TEXT("Structured craft GoalText preserves exact batches"),HearthwardAgent::OriginalQuantityMatches(G));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentOriginalBudgetTest,"Hearthward.NPCAgent.OriginalMaterialBudget",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentOriginalBudgetTest::RunTest(const FString&)
{
    FHearthwardAgentGoal G;G.Intent=TEXT("craft");G.Item=TEXT("arrows");G.Quantity=3;
    G.QuantityMode=TEXT("batches");G.SourceRef=TEXT("bag");
    G.Original=TEXT("制作三批箭矢，最多消耗三份木材");
    TestTrue(TEXT("Target batches are independently valid before checking its material budget"),HearthwardAgent::OriginalQuantityMatches(G));
    TestFalse(TEXT("An explicit original material budget cannot disappear from a valid proposal"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("max:wood:4")};
    TestFalse(TEXT("An explicit original material budget cannot be enlarged by the proposal"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("max:wood:3")};
    TestTrue(TEXT("The exact explicit material budget remains a valid proposal"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=HearthwardAgent::GoalText(G);
    TestTrue(TEXT("The public structured GoalText retains a material budget"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("制作三批箭矢");
    TestTrue(TEXT("An applicable confirmed rule can still supply an unspoken material limit"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("制作三批箭矢，消耗最多三份木材");
    TestTrue(TEXT("Consumption before the ceiling word retains its explicit budget"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("制作三批箭矢，木材最多消耗三份");
    TestTrue(TEXT("Material before the ceiling word retains its explicit budget"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("用你背包的材料制作三批箭矢最多消耗三份木材");
    TestTrue(TEXT("A target before a ceiling without punctuation is not a second budget material"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("max:wood:3"),TEXT("max:stone:2")};G.Original=TEXT("制作三批箭矢，最多消耗三份木材和二份石材");
    TestTrue(TEXT("Two different materials bind their own complete quantities in one budget clause"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("max:wood:3")};
    TestFalse(TEXT("A second explicit material budget cannot disappear"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("max:wood:100")};G.Original=TEXT("制作三批箭矢，最多消耗一百份木材");
    TestTrue(TEXT("A valid hundred-unit Chinese material budget is not narrowed to ninety-nine"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("max:wood:100000")};G.Original=TEXT("制作三批箭矢，最多消耗十万份木材");
    TestTrue(TEXT("The existing maximum material budget remains valid in Chinese"),HearthwardAgent::Validate(G).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentOriginalCollectionMeaningTest,"Hearthward.NPCAgent.OriginalCollectionMeaning",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentOriginalCollectionMeaningTest::RunTest(const FString&)
{
    FHearthwardAgentGoal G;
    if(!TestTrue(TEXT("The actual model-shaped collection result parses before the original is bound"),HearthwardAgent::Parse(
        TEXT("{\"intent\":\"collect\",\"item\":\"wood\",\"quantity\":2,\"mode\":\"additional_acquired\",\"source\":\"S1\",")
        TEXT("\"limits\":[],\"unresolved\":[],\"npc_line\":\"哥，这张卡去已知安全点采集两份木材，请确认后再执行。\"}"),G)))return false;
    G.Original=TEXT("拿二份木材过来");
    if(!TestTrue(TEXT("The actual original has a valid exact quantity before collection meaning is checked"),HearthwardAgent::OriginalQuantityMatches(G)))return false;
    TestFalse(TEXT("An unspecified transfer source cannot become new collection from default S1"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("请新采两份木头并带回营地仓库");
    TestTrue(TEXT("Explicit new collection with the existing material alias remains valid"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("请采二份木材送进仓库");
    TestTrue(TEXT("A normal explicit collection verb does not require a new wording prefix"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=HearthwardAgent::GoalText(G);
    TestTrue(TEXT("Structured collection GoalText remains a valid confirmed contract"),HearthwardAgent::Validate(G).IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentOriginalLocationRestrictionTest,"Hearthward.NPCAgent.OriginalCollectionLocationRestriction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentOriginalLocationRestrictionTest::RunTest(const FString&)
{
    FHearthwardAgentGoal G;
    if(!TestTrue(TEXT("The actual model-shaped location omission parses before original binding"),HearthwardAgent::Parse(
        TEXT("{\"intent\":\"collect\",\"item\":\"wood\",\"quantity\":4,\"mode\":\"additional_acquired\",\"source\":\"S1\",")
        TEXT("\"limits\":[],\"unresolved\":[],\"npc_line\":\"哥，新采四份木材但避开原采集点，请确认路线与目标；若遇阻碍或危险将安全返营等待。\"}"),G)))return false;
    G.Original=TEXT("新采四份木材，但别去那里");
    if(!TestTrue(TEXT("The excluded-place clause preserves a legitimate exact target quantity"),HearthwardAgent::OriginalQuantityMatches(G)))return false;
    TestFalse(TEXT("An unrepresented excluded location cannot silently fall back to S1"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("去已知安全点新采四份木材，带回营地入库");G.Limits={TEXT("source:S1")};
    TestTrue(TEXT("The existing positive known-source restriction stays valid"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=HearthwardAgent::GoalText(G);
    TestTrue(TEXT("Canonical positive source GoalText is unaffected"),HearthwardAgent::Validate(G).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentOriginalCollectionVerbTest,"Hearthward.NPCAgent.OriginalCollectionVerbCompatibility",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentOriginalCollectionVerbTest::RunTest(const FString&)
{
    FHearthwardAgentGoal G;
    if(!TestTrue(TEXT("The model-shaped collection contract parses before checking the original wording"),HearthwardAgent::Parse(
        TEXT("{\"intent\":\"collect\",\"item\":\"wood\",\"quantity\":2,\"mode\":\"additional_acquired\",\"source\":\"S1\",")
        TEXT("\"limits\":[],\"unresolved\":[],\"npc_line\":\"请核对任务卡。\"}"),G)))return false;
    G.Original=TEXT("帮忙采两份木材并送入仓库");
    TestTrue(TEXT("An explicit collection request using help remains valid"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("给我采两份木材送入仓库");
    TestTrue(TEXT("An explicit collection request for the speaker remains valid"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("请勿采两份木材");
    TestFalse(TEXT("A negative collection request cannot authorize fresh collection"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("请把你采集好的二份木材送入仓库");
    if(!TestTrue(TEXT("The existing acquired cargo has an exact target quantity"),HearthwardAgent::OriginalQuantityMatches(G)))return false;
    TestFalse(TEXT("Collection describing acquired cargo cannot authorize another new harvest"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("新采二份木材，别往那里走");
    TestFalse(TEXT("An omitted excluded direction cannot default to the current safe source"),HearthwardAgent::Validate(G).IsEmpty());
    FHearthwardAgentGoal NegativeQuantity;
    if(!TestTrue(TEXT("The unchanged CPU U01 model output parses"),HearthwardAgent::Parse(
        TEXT(R"({"intent":"collect","item":"wood","quantity":3,"mode":"additional_acquired","source":"S1","limits":[],"unresolved":[],"npc_line":"哥，新采负三份木材带回仓库，数量逻辑异常，请确认具体需求或重新表述。"})"),NegativeQuantity)))return false;
    NegativeQuantity.Original=TEXT("新采负三份木材带回仓库");
    TestEqual(TEXT("An explicit negative quantity is refused before asking about collection source"),
        HearthwardAgent::Validate(NegativeQuantity),FString(TEXT("UNRESOLVED_CONSTRAINT")));
    NegativeQuantity.Original=TEXT("新采三份木材带回仓库");
    TestTrue(TEXT("The same positive collection proposal remains valid"),HearthwardAgent::Validate(NegativeQuantity).IsEmpty());
    NegativeQuantity.Original=TEXT("拿三份木材过来");
    TestEqual(TEXT("A genuinely unspecified acquisition source retains clarification"),
        HearthwardAgent::Validate(NegativeQuantity),FString(TEXT("UNRESOLVED_COLLECTION_SOURCE")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentOriginalNoLimitTest,"Hearthward.NPCAgent.OriginalMaterialProhibition",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentOriginalNoLimitTest::RunTest(const FString&)
{
    FHearthwardAgentGoal G;
    if(!TestTrue(TEXT("The model-shaped proposal parses with all eight fields"),HearthwardAgent::Parse(
        TEXT("{\"intent\":\"craft\",\"item\":\"rope\",\"quantity\":1,\"mode\":\"batches\",\"source\":\"bag\",")
        TEXT("\"limits\":[],\"unresolved\":[],\"npc_line\":\"哥，这张卡用我背包的材料制作一批绳索，请确认；现场条件会在执行前复核。\"}"),G)))return false;
    G.Original=TEXT("请用你背包的材料制作一批绳索，但不要消耗木材");
    if(!TestTrue(TEXT("The prohibition clause does not alter the actual target batches"),HearthwardAgent::OriginalQuantityMatches(G)))return false;
    const auto Cost=HearthwardWorkshop::Materials(G.Intent,G.Item,G.Quantity);
    if(!TestTrue(TEXT("The actual recipe has positive wood cost before testing the missing prohibition"),Cost.FindRef(TEXT("wood"))>0))return false;
    TestFalse(TEXT("An explicit original prohibition cannot disappear from a parsed model proposal"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("no:wood")};
    TestTrue(TEXT("An accurately preserved prohibition remains a valid constrained contract"),HearthwardAgent::Validate(G).IsEmpty());
    TestFalse(TEXT("The preserved prohibition blocks the actual recipe cost"),HearthwardAgent::AllowsCost(G.Limits,Cost,{}));
    G.Original=HearthwardAgent::GoalText(G);
    TestTrue(TEXT("Structured GoalText keeps its explicit material prohibition"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("制作一批绳索，不要消耗药草");G.Limits={TEXT("no:herb")};
    TestTrue(TEXT("An original prohibition of another material retains its existing alias"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("no:wood")};
    TestFalse(TEXT("A prohibition cannot bind a different material"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("制作一批绳索，不允许使用精矿");G.Limits={TEXT("no:refined_ore")};
    TestTrue(TEXT("The registered refined material name binds its exact item"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("制作一批绳索，不要使用refined_ore");G.Limits={TEXT("no:ore")};
    TestFalse(TEXT("The longer refined item id cannot be reduced to ore"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("no:refined_ore")};
    TestTrue(TEXT("The complete refined item id remains supported"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("制作一批绳索，不要消耗精炼矿石");G.Limits={TEXT("no:ore")};
    TestFalse(TEXT("An unregistered compound cannot authorize a suffix material match"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("no:herb"),TEXT("no:wood")};G.Original=HearthwardAgent::GoalText(G);
    TestTrue(TEXT("Canonical material prohibitions remain valid when joined as constraint labels"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("no:herb")};
    TestFalse(TEXT("A later canonical prohibition cannot disappear after a constraint-label separator"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={};G.Original=TEXT("这次授权使用营地仓库材料制作一批绳索");G.SourceRef=TEXT("camp");
    TestTrue(TEXT("A normal warehouse-source authorization adds no one-time material exception"),HearthwardAgent::Validate(G).IsEmpty());
    G.Intent=TEXT("repair");G.Item=TEXT("axe");G.QuantityMode=TEXT("one_owned");G.SourceRef=TEXT("bag");G.Original=TEXT("维修自己的石斧一件，不要用木头");
    TestFalse(TEXT("A repair proposal also preserves an explicit original material prohibition"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("no:wood")};
    TestTrue(TEXT("Repair accepts the existing wood alias when its prohibition is preserved"),HearthwardAgent::Validate(G).IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentOriginalRuleMaterialTest,"Hearthward.NPCAgent.OriginalRuleMaterialConstraint",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentOriginalRuleMaterialTest::RunTest(const FString&)
{
    FHearthwardAgentGoal G;G.Intent=TEXT("rule_proposal");G.Item=TEXT("none");G.Quantity=0;
    G.QuantityMode=TEXT("none");G.SourceRef=TEXT("none");G.Limits={TEXT("ban:stone")};
    G.Original=TEXT("以后不要消耗石材");
    TestFalse(TEXT("An original material-consumption prohibition cannot become a collection ban rule"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("no:stone")};
    TestTrue(TEXT("The exact material-consumption rule remains valid"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=HearthwardAgent::GoalText(G);
    TestTrue(TEXT("Canonical explicit material rules keep existing manual-card compatibility"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("ban:stone")};G.Original=TEXT("以后不要采集石材");
    TestTrue(TEXT("An explicit collection ban retains its distinct registered rule"),HearthwardAgent::Validate(G).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentOriginalOnceTest,"Hearthward.NPCAgent.OriginalOnceAuthorization",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentOriginalOnceTest::RunTest(const FString&)
{
    FHearthwardNPCMemory Rules;
    if(!TestTrue(TEXT("A lasting material prohibition is recorded through the existing public memory operation"),
        Rules.PutRule(TEXT("no:wood"),TEXT("以后制作和维修都不要消耗木材"),1)))return false;
    if(!TestTrue(TEXT("The lasting prohibition is applicable to this capability"),Rules.ApplicableRules(TEXT("craft")).Contains(TEXT("no:wood"))))return false;
    FHearthwardAgentGoal G;
    if(!TestTrue(TEXT("A model-proposed one-time exception has the normal eight-field shape"),HearthwardAgent::Parse(
        TEXT("{\"intent\":\"craft\",\"item\":\"rope\",\"quantity\":1,\"mode\":\"batches\",\"source\":\"bag\",")
        TEXT("\"limits\":[\"once:wood\"],\"unresolved\":[],\"npc_line\":\"哥，这次用我背包的材料制作一批绳索；请确认后执行，其他约定保持有效。\"}"),G)))return false;
    G.Original=TEXT("这次请用你背包的材料制作一批绳索");
    if(!TestTrue(TEXT("A current-only task still has an exact legitimate target"),HearthwardAgent::OriginalQuantityMatches(G)))return false;
    TestFalse(TEXT("Saying this time cannot authorize an unspoken material-specific exception"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("这次允许消耗木材，请用你背包的材料制作一批绳索");
    TestTrue(TEXT("An explicit original one-time wood allowance remains supported"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=HearthwardAgent::GoalText(G);
    TestTrue(TEXT("Structured GoalText retains its explicit one-time material allowance"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("这次允许使用木材粉，制作一批绳索");
    TestFalse(TEXT("A complete unknown material suffix cannot grant a one-time wood exception"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("这次允许使用木材的替代物，制作一批绳索");
    TestFalse(TEXT("Permission to use a wood substitute cannot grant a one-time wood exception"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("这次允许用石头，制作一批绳索");
    TestFalse(TEXT("A one-time stone allowance cannot lift the wood prohibition"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("once:stone")};
    TestTrue(TEXT("The explicit one-time allowance binds the actual aliased material"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("本次可以用木头，制作一批绳索");G.Limits={TEXT("once:wood")};
    TestTrue(TEXT("An explicit current-only wood allowance supports existing wording and alias"),HearthwardAgent::Validate(G).IsEmpty());
    G.Original=TEXT("不允许本次消耗木材，制作一批绳索");G.Limits={TEXT("once:wood"),TEXT("no:wood")};
    TestFalse(TEXT("Consent cannot start inside a negated authorization even when no is retained"),HearthwardAgent::Validate(G).IsEmpty());
    G.Limits={TEXT("no:wood")};
    TestTrue(TEXT("A negated current-only consumption clause retains its actual prohibition"),HearthwardAgent::Validate(G).IsEmpty());
    TestTrue(TEXT("A one-time task never revokes the lasting rule record"),Rules.ApplicableRules(TEXT("craft")).Contains(TEXT("no:wood")));
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
    FHearthwardCompanionCommand Delivery;const auto DeliveryTicket=Delivery.Request(Epoch);
    TestTrue(TEXT("Accept camp delivery"),Delivery.Accept(DeliveryTicket,Epoch,TEXT("wood"),5,
        {TEXT("collect"),TEXT("return"),TEXT("deposit")})==EHearthwardProposalResult::Accepted);
    TestTrue(TEXT("Withdraw cargo"),Delivery.RecordAcquisition(3));
    TestFalse(TEXT("Unfulfilled amount cannot exceed carried cargo"),Delivery.RecordUnfulfilled(4));
    TestTrue(TEXT("Returned warehouse cargo reverses acquisition"),Delivery.RecordUnfulfilled(3));
    TestEqual(TEXT("Returned cargo is not player delivery"),Delivery.GetDelivered(),0);
    TestEqual(TEXT("Returned cargo leaves no tracked load"),Delivery.GetCarried(),0);
    TestTrue(TEXT("Returned quantity can be withdrawn again"),Delivery.RecordAcquisition(5));
    Delivery.Carried-=5;TestTrue(TEXT("Only actual handoff completes goal"),Delivery.RecordDelivery(DeliveryTicket,5));
    TestEqual(TEXT("Five delivered"),Delivery.GetDelivered(),5);
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
    FHearthwardNPCEvent Replanned;Replanned.Id=FGuid::NewGuid();Replanned.Command=Command;Replanned.Campaign=M.Campaign;
    Replanned.Kind=TEXT("replanned");Replanned.Item=TEXT("wood");Replanned.Count=0;Replanned.Reason=TEXT("SOURCE_POSITION_CHANGED");M.RecordEvent(Replanned);
    TestTrue(TEXT("Adaptive replan event remains save-valid"),M.IsValid(0));
    TestTrue(TEXT("Valid memory"),M.IsValid(0));M.Events.Last().Campaign=FGuid::NewGuid();TestFalse(TEXT("Foreign campaign rejected"),M.IsValid(0));
    FHearthwardNPCMemory Recycled;
    for(int32 I=0;I<128;++I)
    {TestTrue(TEXT("Repeated insert"),Recycled.Put({},TEXT("claim"),TEXT("可撤销记录"),0));const auto Id=Recycled.Records[0].Id;TestTrue(TEXT("Repeated revoke"),Recycled.Revoke(Id));TestFalse(TEXT("No old handle reuse"),Recycled.Put(Id,TEXT("claim"),TEXT("已撤销"),0));}
    TestTrue(TEXT("All capacity reclaimed"),Recycled.Records.IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentBeliefStateTest,"Hearthward.NPCAgent.BeliefStateProvenance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentBeliefStateTest::RunTest(const FString&)
{
    FHearthwardNPCMemory M;M.Campaign=FGuid::NewGuid();const int64 StartRevision=M.Revision;
    TestTrue(TEXT("Firsthand camp fact stored"),HearthwardBeliefs::UpsertCampStock(M.Beliefs,M.Revision,M.Campaign,TEXT("wood"),5,EHearthwardNPCBeliefSource::Firsthand,10));
    FHearthwardNPCBeliefView V;
    TestTrue(TEXT("Firsthand resolves confirmed"),HearthwardBeliefs::ResolveCampStock(M.Beliefs,TEXT("wood"),V)
        && V.Value==5 && V.Source==EHearthwardNPCBeliefSource::Firsthand && V.IsConfirmed());
    const int64 FirsthandRevision=M.Revision;
    TestEqual(TEXT("Initial semantic change time"),V.RecordedAt,10.0);
    TestEqual(TEXT("Initial evidence time"),V.LastEvidenceAt,10.0);
    TestTrue(TEXT("Identical observation refreshes evidence"),HearthwardBeliefs::UpsertCampStock(M.Beliefs,M.Revision,M.Campaign,TEXT("wood"),5,EHearthwardNPCBeliefSource::Firsthand,11));
    HearthwardBeliefs::ResolveCampStock(M.Beliefs,TEXT("wood"),V);
    TestEqual(TEXT("Evidence refresh does not churn semantic revision"),M.Revision,FirsthandRevision);
    TestEqual(TEXT("Evidence refresh preserves semantic change time"),V.RecordedAt,10.0);
    TestEqual(TEXT("Evidence refresh advances LastEvidenceAt"),V.LastEvidenceAt,11.0);
    TestFalse(TEXT("Older evidence cannot move freshness backwards"),HearthwardBeliefs::UpsertCampStock(M.Beliefs,M.Revision,M.Campaign,TEXT("wood"),5,EHearthwardNPCBeliefSource::Firsthand,10.5));

    TestTrue(TEXT("Later player report is stored with provenance"),HearthwardBeliefs::UpsertCampStock(M.Beliefs,M.Revision,M.Campaign,TEXT("wood"),20,EHearthwardNPCBeliefSource::PlayerReport,12));
    HearthwardBeliefs::ResolveCampStock(M.Beliefs,TEXT("wood"),V);
    TestTrue(TEXT("Player report remains explicitly unconfirmed"),V.Value==20 && V.Source==EHearthwardNPCBeliefSource::PlayerReport && !V.IsConfirmed());

    TestTrue(TEXT("Firsthand evidence can correct report"),HearthwardBeliefs::UpsertCampStock(M.Beliefs,M.Revision,M.Campaign,TEXT("wood"),7,EHearthwardNPCBeliefSource::Firsthand,13));
    HearthwardBeliefs::ResolveCampStock(M.Beliefs,TEXT("wood"),V);
    TestTrue(TEXT("Corrected belief is confirmed"),V.Value==7 && V.Source==EHearthwardNPCBeliefSource::Firsthand && V.IsConfirmed());
    TestTrue(TEXT("Belief memory validates"),M.Revision>StartRevision && M.IsValid(13));

    FHearthwardNPCMemory Legacy;Legacy.Campaign=FGuid::NewGuid();Legacy.HasCampObservation=true;Legacy.CampObservedAt=4;Legacy.CampInventory.Add(TEXT("wood"),3);
    Legacy.Migrate(Legacy.Campaign);
    TestTrue(TEXT("Legacy camp snapshot migrates to typed belief"),HearthwardBeliefs::ResolveCampStock(Legacy.Beliefs,TEXT("wood"),V)
        && V.Value==3 && V.Source==EHearthwardNPCBeliefSource::Firsthand);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentInitiativePolicyTest,"Hearthward.NPCAgent.EventDrivenInitiativePolicy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentInitiativePolicyTest::RunTest(const FString&)
{
    const auto Evidence=FGuid::NewGuid();
    auto I=HearthwardInitiative::FromEvent(TEXT("completed"),TEXT("wood"),4,TEXT(""),Evidence,10);
    TestTrue(TEXT("Completed event becomes initiative"),I.IsValid() && I.Kind==TEXT("task_completed") && I.EvidenceId==Evidence && I.Message.Contains(TEXT("4")));

    I=HearthwardInitiative::FromEvent(TEXT("replanned"),TEXT("wood"),0,TEXT("SOURCE_POSITION_CHANGED"),FGuid::NewGuid(),11);
    TestTrue(TEXT("Replan event becomes initiative"),I.IsValid() && I.Kind==TEXT("task_replanned"));

    I=HearthwardInitiative::FromEvent(TEXT("blocked"),TEXT("wood"),0,TEXT("RETURN_UNREACHABLE"),FGuid::NewGuid(),12);
    TestTrue(TEXT("Blocked reason is grounded"),I.IsValid() && I.Kind==TEXT("task_blocked") && I.Message.Contains(TEXT("RETURN_UNREACHABLE")));

    I=HearthwardInitiative::FromEvent(TEXT("delivered"),TEXT("wood"),2,TEXT(""),FGuid::NewGuid(),13);
    TestFalse(TEXT("Routine partial delivery does not chatter"),I.IsValid());

    I=HearthwardInitiative::BeliefCorrection(TEXT("wood"),99,2,14);
    TestTrue(TEXT("Belief correction is proactive"),I.IsValid() && I.Kind==TEXT("belief_corrected") && I.Message.Contains(TEXT("99")) && I.Message.Contains(TEXT("2")));
    TestFalse(TEXT("No correction when report already matches"),HearthwardInitiative::BeliefCorrection(TEXT("wood"),2,2,15).IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentEpisodeProjectionTest,"Hearthward.NPCAgent.GroundedEpisodeProjection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentEpisodeProjectionTest::RunTest(const FString&)
{
    const FGuid Campaign=FGuid::NewGuid(),Command=FGuid::NewGuid();
    FHearthwardNPCMemory Memory;Memory.Campaign=Campaign;Memory.BeginCommand(Command);
    auto Add=[&](FHearthwardNPCMemory& Target,FGuid TargetCommand,FName Kind,int32 Count,double At,const FString& Reason=FString())
    {
        FHearthwardNPCEvent E;E.Id=FGuid::NewGuid();E.Command=TargetCommand;E.Campaign=Campaign;
        E.Kind=Kind;E.Item=TEXT("wood");E.Count=Count;E.At=At;E.Reason=Reason;Target.RecordEvent(E);
    };
    Add(Memory,Command,TEXT("acquired"),2,1);
    Add(Memory,Command,TEXT("replanned"),0,2,TEXT("SOURCE_POSITION_CHANGED"));
    Add(Memory,Command,TEXT("delivered"),2,3);
    Add(Memory,Command,TEXT("completed"),2,4);

    const FGuid OtherCommand=FGuid::NewGuid();
    Add(Memory,OtherCommand,TEXT("blocked"),0,5,TEXT("RETURN_UNREACHABLE"));

    const auto Episodes=HearthwardEpisodes::Build(Memory,3);
    TestEqual(TEXT("Two commands become two episodes"),Episodes.Num(),2);
    TestTrue(TEXT("Newest command sorted first"),Episodes[0].Command==OtherCommand && Episodes[0].Reasons.Contains(TEXT("RETURN_UNREACHABLE")));
    const auto* Completed=Episodes.FindByPredicate([&](const auto& X){return X.Command==Command;});
    TestTrue(TEXT("Completed episode found"),Completed!=nullptr);
    if(Completed)
    {
        TestTrue(TEXT("Accepted command with intact events has complete coverage"),Completed->Coverage==EHearthwardNPCEpisodeCoverage::Complete);
        TestTrue(TEXT("Episode aggregates real effects"),Completed->Acquired==2 && Completed->Delivered==2 && Completed->Replans==1 && Completed->Completed);
        TestTrue(TEXT("Episode keeps replan evidence"),Completed->Reasons.Contains(TEXT("SOURCE_POSITION_CHANGED")) && Completed->Evidence.Num()==4);
        const FString Text=HearthwardEpisodes::Describe(*Completed);
        TestTrue(TEXT("Complete description is evidence-grounded"),Text.Contains(TEXT("实际取得2")) && Text.Contains(TEXT("实际入库2"))
            && Text.Contains(TEXT("重规划1次")) && Text.Contains(TEXT("SOURCE_POSITION_CHANGED")) && Text.Contains(TEXT("记录覆盖完整")));
    }
    const auto CompatibilityEpisodes=HearthwardEpisodes::Build(Memory.Events,3);
    TestTrue(TEXT("Events without coverage metadata never claim completeness"),
        CompatibilityEpisodes.ContainsByPredicate([&](const auto& X){return X.Command==Command && X.Coverage==EHearthwardNPCEpisodeCoverage::Unknown;}));

    FHearthwardNPCMemory Truncated;Truncated.Campaign=Campaign;
    const FGuid Active=FGuid::NewGuid();Truncated.BeginCommand(Active);
    Add(Truncated,Active,TEXT("acquired"),1,10);
    for(int32 I=0;I<128;++I)
    {
        FHearthwardNPCEvent Directive;Directive.Id=FGuid::NewGuid();Directive.Command=FGuid::NewGuid();Directive.Campaign=Campaign;
        Directive.Kind=TEXT("directive");Directive.Item=TEXT("follow");Directive.Count=1;Directive.At=11+I;Directive.Reason=TEXT("coverage_pressure");
        Truncated.RecordEvent(Directive);
    }
    TestEqual(TEXT("Event ring remains bounded"),Truncated.Events.Num(),128);
    TestTrue(TEXT("Active command remains registered after all early events are evicted"),
        Truncated.CommandCoverage.ContainsByPredicate([&](const auto& C){return C.Command==Active && C.Active;}));
    TestTrue(TEXT("Eviction marks active command truncated"),Truncated.CoverageFor(Active)==EHearthwardNPCEpisodeCoverage::Truncated);

    Add(Truncated,Active,TEXT("delivered"),1,200);
    Add(Truncated,Active,TEXT("completed"),1,201);
    const auto TruncatedEpisodes=HearthwardEpisodes::Build(Truncated,3);
    const auto* TruncatedEpisode=TruncatedEpisodes.FindByPredicate([&](const auto& X){return X.Command==Active;});
    TestTrue(TEXT("Later events do not reset truncated coverage"),TruncatedEpisode && TruncatedEpisode->Coverage==EHearthwardNPCEpisodeCoverage::Truncated);
    if(TruncatedEpisode)
    {
        const FString Text=HearthwardEpisodes::Describe(*TruncatedEpisode);
        TestTrue(TEXT("Terminal state remains reportable without inventing the missing process"),
            TruncatedEpisode->Completed && Text.Contains(TEXT("记录已截断")) && Text.Contains(TEXT("不能据此确认全部过程或总量")));
    }
    TestTrue(TEXT("Coverage metadata remains bounded"),Truncated.CommandCoverage.Num()<=2 && Truncated.IsValid(201));

    FHearthwardNPCMemory Legacy;Legacy.Campaign=Campaign;
    FHearthwardNPCEvent LegacyCompleted;LegacyCompleted.Id=FGuid::NewGuid();LegacyCompleted.Command=FGuid::NewGuid();LegacyCompleted.Campaign=Campaign;
    LegacyCompleted.Kind=TEXT("completed");LegacyCompleted.Item=TEXT("wood");LegacyCompleted.Count=1;LegacyCompleted.At=5;Legacy.Events.Add(LegacyCompleted);
    Legacy.Migrate(Campaign,true);
    TestTrue(TEXT("Legacy events migrate to unknown rather than fabricated complete"),
        Legacy.CoverageFor(LegacyCompleted.Command)==EHearthwardNPCEpisodeCoverage::Unknown && Legacy.IsValid(5));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentContextProjectionTest,"Hearthward.NPCAgent.BoundedContextProjection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentContextProjectionTest::RunTest(const FString&)
{
    FHearthwardNPCContextSnapshot S;S.Query=TEXT("请去新采四份木材并带回仓库");S.InputSource=TEXT("free_text");
    S.bAtCamp=false;S.bCampAvailable=true;S.bCollectionSourceAvailable=true;S.bCollectionSourceTrustedSafe=true;
    S.CollectionSafety=TEXT("allowed");S.Memory.Campaign=FGuid::NewGuid();
    TestTrue(TEXT("Hard collection rule stored"),S.Memory.Put({},TEXT("collection_ban"),TEXT("以后不要采石头"),1,TEXT("stone")));
    TestTrue(TEXT("Typed source rule stored"),S.Memory.PutRule(TEXT("source:S1"),TEXT("采集只能使用已知来源"),2));
    for(int32 I=0;I<50;++I)
        TestTrue(TEXT("Pressure player record stored"),S.Memory.Put({},TEXT("claim"),FString::Printf(TEXT("无关记录%02d"),I),3+I));

    double At=100;
    for(const auto& Item:HearthwardBasicItems())
    {
        TestTrue(TEXT("Pressure belief stored"),HearthwardBeliefs::UpsertCampStock(S.Memory.Beliefs,S.Memory.Revision,S.Memory.Campaign,
            Item.Id,Item.Id==TEXT("wood")?10:1,EHearthwardNPCBeliefSource::Firsthand,At++));
        S.OwnBag.Add(Item.Id,Item.Id==TEXT("wood")?2:1);
    }
    S.Memory.WorkingGoal.Intent=TEXT("collect");S.Memory.WorkingGoal.Item=TEXT("wood");S.Memory.WorkingGoal.Quantity=4;
    S.Memory.WorkingGoal.QuantityMode=TEXT("additional_acquired");S.Memory.WorkingGoal.SourceRef=TEXT("S1");
    S.Memory.WorkingGoal.Original=TEXT("请去新采四份木材并带回仓库，但不要改变其它限制");
    S.Memory.WorkingGoal.Unresolved={TEXT("玩家明确限制必须保留")};

    const auto Full=HearthwardContextProjection::Project(S,EHearthwardNPCContextTier::Full);
    const auto Compact=HearthwardContextProjection::Project(S,EHearthwardNPCContextTier::Compact);
    const auto Minimal=HearthwardContextProjection::Project(S,EHearthwardNPCContextTier::Minimal);
    const auto MinimalAgain=HearthwardContextProjection::Project(S,EHearthwardNPCContextTier::Minimal);

    TestTrue(TEXT("Projection is deterministic for the same snapshot"),Minimal.Json==MinimalAgain.Json);
    TestTrue(TEXT("Minimal projection is smaller than full pressure projection"),Minimal.Json.Len()<Full.Json.Len());
    TestFalse(TEXT("Legacy duplicate camp inventory is not a model fact source"),Full.Json.Contains(TEXT("observed_camp_inventory"))
        || Full.Json.Contains(TEXT("last_seen_camp")) || Full.Json.Contains(TEXT("camp_knowledge")));
    TestTrue(TEXT("Hard rules survive every degradation tier"),Minimal.Json.Contains(TEXT("collection_prohibited_items"))
        && Minimal.Json.Contains(TEXT("stone")) && Minimal.Json.Contains(TEXT("source:S1")));
    TestTrue(TEXT("Unresolved original constraint survives minimal tier"),Minimal.Json.Contains(TEXT("玩家明确限制必须保留")));
    TestTrue(TEXT("Collect projection does not inject camp-stock beliefs"),Full.Json.Contains(TEXT("\"camp_stock_beliefs\":[]"))
        && Minimal.Json.Contains(TEXT("\"camp_stock_beliefs\":[]")));
    TestTrue(TEXT("Unneeded beliefs are reported as dropped"),Full.DroppedFields.Contains(TEXT("unrelated_beliefs"))
        && Minimal.DroppedFields.Contains(TEXT("unrelated_beliefs")));

    auto InventorySnapshot=S;
    InventorySnapshot.Query=TEXT("营地仓库还有多少木材？");
    InventorySnapshot.Memory.WorkingGoal={};
    const auto InventoryMinimal=HearthwardContextProjection::Project(InventorySnapshot,EHearthwardNPCContextTier::Minimal);
    TestTrue(TEXT("Inventory query keeps relevant wood belief"),InventoryMinimal.Json.Contains(TEXT("\"item\":\"wood\""))
        && InventoryMinimal.Json.Contains(TEXT("\"last_evidence_time\""))
        && InventoryMinimal.Json.Contains(TEXT("\"freshness\":\"possibly_stale\"")));
    TestFalse(TEXT("Inventory query still drops unrelated pressure beliefs"),InventoryMinimal.Json.Contains(TEXT("\"item\":\"stone\"")));
    TestTrue(TEXT("Compact/full are named diagnostics, not extra generations"),Full.Tier==TEXT("full_relevant")
        && Compact.Tier==TEXT("compact_relevant") && Minimal.Tier==TEXT("required_minimal"));

    TSharedPtr<FJsonObject> MinimalObject;
    TestTrue(TEXT("Projected JSON parses"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Minimal.Json),MinimalObject) && MinimalObject.IsValid());
    if(MinimalObject)
    {
        const TArray<TSharedPtr<FJsonValue>>* Records=nullptr;
        TestTrue(TEXT("Player records remain bounded under pressure"),MinimalObject->TryGetArrayField(TEXT("player_records"),Records) && Records && Records->Num()<=1);
    }
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

    O.PlayerHealthRatio=.3f;O.ProtectHealthRatio=.5f;O.ProtectRadius=300;O.RegroupThreatCount=2;
    O.Threats={Near};
    D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("Low health protects against the nearest close threat"),
        D.Intent==EHearthwardCompanionTacticalIntent::Protect && D.Target==TEXT("near_player") && D.Reason==TEXT("PROTECT_LOW_HEALTH"));

    FHearthwardCompanionThreat CloseA;CloseA.Id=TEXT("close_a");CloseA.RemainingHealth=10;CloseA.Position=FVector(100,0,0);
    FHearthwardCompanionThreat CloseB;CloseB.Id=TEXT("close_b");CloseB.RemainingHealth=10;CloseB.Position=FVector(0,120,0);
    O.Threats={CloseA,CloseB};
    D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("Low health under multiple close threats regroups"),
        D.Intent==EHearthwardCompanionTacticalIntent::Regroup && D.Target.IsNone() && D.Reason==TEXT("LOW_HEALTH_OVERWHELMED"));

    O.Threats={Outside};
    D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("Low health without a close protect target regroups"),
        D.Intent==EHearthwardCompanionTacticalIntent::Regroup && D.Target.IsNone() && D.Reason==TEXT("LOW_HEALTH_REGROUP"));

    O.PlayerHealthRatio=0;
    D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("Down player regroups instead of chasing"),
        D.Intent==EHearthwardCompanionTacticalIntent::Regroup && D.Target.IsNone() && D.Reason==TEXT("PLAYER_DOWN_REGROUP"));

    O.RequestedOrder=TEXT("wait");
    D=HearthwardCombatPolicy::Evaluate(O);
    TestTrue(TEXT("Explicit hold still wins even while down"),D.Intent==EHearthwardCompanionTacticalIntent::Hold);

    FHearthwardAgentGoal Goal;Goal.Intent=TEXT("companion_order");Goal.Item=TEXT("assist");Goal.Quantity=1;
    Goal.QuantityMode=TEXT("directive");Goal.SourceRef=TEXT("player");
    TestTrue(TEXT("Companion order is a confirmed world-write capability"),Goal.WritesWorld());
    TestTrue(TEXT("Valid companion directive passes contract"),HearthwardAgent::Validate(Goal).IsEmpty());
    Goal.Item=TEXT("routine");
    TestTrue(TEXT("Routine is an explicit high-level companion directive"),HearthwardAgent::Validate(Goal).IsEmpty());
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

    C.PreferredDirective=TEXT("follow");C.CoordinationConfidence=.75f;
    S=HearthwardSuggestions::Build(C);
    TestTrue(TEXT("Stable coordination prior changes suggestion only"),
        S.ContainsByPredicate([](const auto& X){return X.Kind==TEXT("coordination") && X.Message==TEXT("跟着我。");}));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentCoordinationPriorTest,"Hearthward.NPCAgent.CoordinationPrior",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentCoordinationPriorTest::RunTest(const FString&)
{
    TArray<FHearthwardNPCEvent> Events;
    const FGuid Campaign=FGuid::NewGuid();
    auto Add=[&](FName Directive,double At)
    {
        FHearthwardNPCEvent E;E.Id=FGuid::NewGuid();E.Command=FGuid::NewGuid();E.Campaign=Campaign;
        E.Kind=TEXT("directive");E.Item=Directive;E.Count=1;E.At=At;E.Reason=TEXT("test");Events.Add(E);
    };

    Add(TEXT("follow"),1);Add(TEXT("follow"),2);
    auto P=HearthwardCoordination::Build(Events);
    TestFalse(TEXT("Two samples never create a learned prior"),P.Stable);
    TestTrue(TEXT("Counts remain inspectable"),P.Samples==2 && P.FollowCount==2);

    Add(TEXT("follow"),3);
    P=HearthwardCoordination::Build(Events);
    TestTrue(TEXT("Three consistent confirmations establish follow prior"),
        P.Stable && P.PreferredDirective==TEXT("follow") && P.Confidence>=.99f);

    Add(TEXT("assist"),4);
    P=HearthwardCoordination::Build(Events);
    TestTrue(TEXT("One contradictory command does not instantly erase history"),
        P.Stable && P.PreferredDirective==TEXT("follow"));

    Add(TEXT("assist"),5);Add(TEXT("assist"),6);Add(TEXT("assist"),7);Add(TEXT("assist"),8);
    P=HearthwardCoordination::Build(Events);
    TestFalse(TEXT("Mixed transition becomes uncertain before flipping"),P.Stable);

    Add(TEXT("assist"),9);Add(TEXT("assist"),10);
    P=HearthwardCoordination::Build(Events);
    TestTrue(TEXT("Recent rolling window can adapt to a new stable habit"),
        P.Stable && P.PreferredDirective==TEXT("assist") && P.Confidence>=.67f);

    TestTrue(TEXT("Directive events remain valid memory facts"),[&]()
    {
        FHearthwardNPCMemory M;M.Campaign=Campaign;M.Events=Events;M.Revision=20;return M.IsValid(10);
    }());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentRoutinePolicyTest,"Hearthward.NPCAgent.CampRoutinePolicy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentRoutinePolicyTest::RunTest(const FString&)
{
    FHearthwardNPCRoutineContext C;C.Enabled=true;C.bOrderIdle=true;C.GameSeconds=0;C.CompanionToCampDistance=0;
    auto D=HearthwardRoutine::Evaluate(C);
    TestTrue(TEXT("Idle authorized companion gets a routine phase"),D.Active && D.Activity==TEXT("rest"));

    C.GameSeconds=7;
    D=HearthwardRoutine::Evaluate(C);
    TestTrue(TEXT("World time deterministically advances routine"),D.Active && D.Activity==TEXT("patrol"));

    C.CompanionToCampDistance=500;
    D=HearthwardRoutine::Evaluate(C);
    TestTrue(TEXT("Far companion returns to camp before routine"),D.Active && D.Activity==TEXT("return_camp"));

    C.CompanionToCampDistance=0;C.bPlayerInCombat=true;
    TestFalse(TEXT("Combat suspends routine"),HearthwardRoutine::Evaluate(C).Active);
    C.bPlayerInCombat=false;C.bPlayerDown=true;
    TestFalse(TEXT("Player down suspends routine"),HearthwardRoutine::Evaluate(C).Active);
    C.bPlayerDown=false;C.bTaskActive=true;
    TestFalse(TEXT("Typed task owns navigation over routine"),HearthwardRoutine::Evaluate(C).Active);
    C.bTaskActive=false;C.bOrderIdle=false;
    TestFalse(TEXT("Explicit follow/assist/hold ownership suppresses routine"),HearthwardRoutine::Evaluate(C).Active);
    C.bOrderIdle=true;C.Enabled=false;
    TestFalse(TEXT("Explicitly disabled routine stays off"),HearthwardRoutine::Evaluate(C).Active);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCAgentRecoveryPolicyTest,"Hearthward.NPCAgent.AdaptiveRecoveryPolicy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNPCAgentRecoveryPolicyTest::RunTest(const FString&)
{
    using A=EHearthwardAgentActionType;
    using T=EHearthwardAgentTarget;
    using M=EHearthwardAgentRecoveryMode;

    FHearthwardAgentRecoveryContext C;
    C.FailedAction.Type=A::Gather;C.FailedAction.Target=T::Source;
    C.FailureReason=TEXT("SOURCE_POSITION_CHANGED");
    C.bCampAvailable=true;C.bSourceAvailable=true;C.bStationAvailable=true;
    C.MaxAdaptiveAttempts=2;

    auto D=HearthwardRecovery::Decide(C);
    TestTrue(TEXT("Moved source rewinds to source move"),
        D.Mode==M::RewindToMove && D.RewindTarget==T::Source && D.Reason==TEXT("REACQUIRE_SOURCE"));

    C.FailedAction.Type=A::MoveTo;C.FailedAction.Target=T::Source;C.FailureReason=TEXT("去程受阻");
    D=HearthwardRecovery::Decide(C);
    TestTrue(TEXT("Transient source route retries same move"),D.Mode==M::RetryCurrent && D.Reason==TEXT("RETRY_ROUTE"));

    C.FailedAction.Type=A::CommitWorkshop;C.FailedAction.Target=T::Workshop;C.FailureReason=TEXT("PATH_BLOCKED");
    D=HearthwardRecovery::Decide(C);
    TestTrue(TEXT("Workshop path failure rewinds to workshop move"),
        D.Mode==M::RewindToMove && D.RewindTarget==T::Workshop);

    C.bHasCargo=true;
    D=HearthwardRecovery::Decide(C);
    TestTrue(TEXT("Physical cargo overrides adaptive recovery"),D.Mode==M::ReturnToCamp && D.Reason==TEXT("PRESERVE_CARGO"));

    C.bHasCargo=false;C.AdaptiveAttempts=2;
    D=HearthwardRecovery::Decide(C);
    TestTrue(TEXT("Adaptive budget is bounded"),D.Mode==M::ReturnToCamp && D.Reason==TEXT("REPLAN_BUDGET_EXHAUSTED"));

    C.bCampAvailable=false;
    D=HearthwardRecovery::Decide(C);
    TestTrue(TEXT("No camp after exhausted budget holds safely"),D.Mode==M::Hold);

    C.AdaptiveAttempts=0;C.bCampAvailable=true;C.bSourceAvailable=false;
    C.FailedAction.Type=A::Gather;C.FailedAction.Target=T::Source;C.FailureReason=TEXT("实际资源不足");
    D=HearthwardRecovery::Decide(C);
    TestTrue(TEXT("Depleted resource is not invented into a replan"),D.Mode==M::ReturnToCamp && D.Reason==TEXT("HARD_BLOCK"));

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

    FHearthwardAgentGoal Store=Goal(TEXT("store"),TEXT("bag"));Store.QuantityMode=TEXT("held_to_camp");
    TestTrue(TEXT("Held cargo store plan builds"),HearthwardPlan::Build(Store,Plan,Error));
    TestEqual(TEXT("Store has only travel and deposit"),Plan.Actions.Num(),2);
    TestTrue(TEXT("Store begins with camp travel"),Plan.Actions[0].Matches(EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Camp));
    TestTrue(TEXT("Store finishes with deposit"),Plan.Actions[1].Matches(EHearthwardAgentActionType::Deposit,EHearthwardAgentTarget::Camp));

    FHearthwardAgentGoal Retrieve=Goal(TEXT("retrieve"),TEXT("camp"));Retrieve.QuantityMode=TEXT("camp_to_player");
    TestTrue(TEXT("Camp-to-player delivery plan builds"),HearthwardPlan::Build(Retrieve,Plan,Error));
    TestEqual(TEXT("Delivery has four real steps"),Plan.Actions.Num(),4);
    TestTrue(TEXT("Delivery withdraws from camp"),Plan.Actions[1].Matches(EHearthwardAgentActionType::Withdraw,EHearthwardAgentTarget::Camp));
    TestTrue(TEXT("Delivery navigates to player"),Plan.Actions[2].Matches(EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Player));
    TestTrue(TEXT("Only player handoff counts"),Plan.Actions[3].Matches(EHearthwardAgentActionType::Handoff,EHearthwardAgentTarget::Player));

    FHearthwardAgentGoal Give=Goal(TEXT("give"),TEXT("bag"));Give.QuantityMode=TEXT("bag_to_player");
    TestTrue(TEXT("Brother-to-player delivery plan builds"),HearthwardPlan::Build(Give,Plan,Error));
    TestEqual(TEXT("Held cargo only needs approach and handoff"),Plan.Actions.Num(),2);
    TestTrue(TEXT("Held cargo handoff targets player"),Plan.Actions[1].Matches(EHearthwardAgentActionType::Handoff,EHearthwardAgentTarget::Player));
    FHearthwardAgentGoal Fetch=Goal(TEXT("fetch"),TEXT("camp"));Fetch.QuantityMode=TEXT("camp_to_bag");
    TestTrue(TEXT("Camp-to-brother delivery plan builds"),HearthwardPlan::Build(Fetch,Plan,Error));
    TestEqual(TEXT("Camp-to-brother ends after physical withdrawal"),Plan.Actions.Num(),2);
    TestTrue(TEXT("Camp-to-brother withdraws at camp"),Plan.Actions[1].Matches(EHearthwardAgentActionType::Withdraw,EHearthwardAgentTarget::Camp));
    FHearthwardAgentGoal Receive=Goal(TEXT("receive"),TEXT("player_bag"));Receive.QuantityMode=TEXT("player_to_bag");
    TestTrue(TEXT("Player-to-brother delivery plan builds"),HearthwardPlan::Build(Receive,Plan,Error));
    TestEqual(TEXT("Player-to-brother uses physical handoff"),Plan.Actions.Num(),2);
    TestTrue(TEXT("Player-to-brother handoff targets player"),Plan.Actions[1].Matches(EHearthwardAgentActionType::Handoff,EHearthwardAgentTarget::Player));

    FHearthwardAgentGoal Care;Care.Intent=TEXT("nature_care");Care.Item=TEXT("water");Care.Quantity=1;
    Care.QuantityMode=TEXT("action_count");Care.SourceRef=TEXT("known_target");
    TestFalse(TEXT("Nature action requires an identified target"),HearthwardPlan::Build(Care,Plan,Error));
    Care.Station=FGuid::NewGuid();
    TestTrue(TEXT("Identified nature action has an executable plan"),HearthwardPlan::Build(Care,Plan,Error));
    TestEqual(TEXT("Nature plan action count"),Plan.Actions.Num(),2);
    TestTrue(TEXT("Nature plan commits through one domain action"),Plan.Actions[1].Matches(EHearthwardAgentActionType::CommitNature,EHearthwardAgentTarget::Nature));

    FHearthwardAgentGoal Resource=Goal(TEXT("nature_collect"),TEXT("known_target"));Resource.Item=TEXT("stone");
    TestFalse(TEXT("Resource collection requires an identified point"),HearthwardPlan::Build(Resource,Plan,Error));
    Resource.Station=FGuid::NewGuid();
    TestTrue(TEXT("Identified resource point builds"),HearthwardPlan::Build(Resource,Plan,Error));
    TestEqual(TEXT("Resource collection has return and deposit"),Plan.Actions.Num(),4);
    TestTrue(TEXT("Resource collection commits through Nature"),Plan.Actions[1].Matches(EHearthwardAgentActionType::CommitNature,EHearthwardAgentTarget::Nature));
    TestTrue(TEXT("Resource collection deposits at camp"),Plan.Actions[3].Matches(EHearthwardAgentActionType::Deposit,EHearthwardAgentTarget::Camp));

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
    TestTrue(TEXT("NPC craft debits own bag"),HearthwardWorkshop::Commit(NPC,TEXT("craft"),TEXT("arrows"),2));
    TestEqual(TEXT("Actual NPC arrows"),NPC->GetItemCount(TEXT("arrow")),8);TestEqual(TEXT("Player wood unchanged"),Player->GetItemCount(TEXT("wood")),10);
    TestFalse(TEXT("Material disappearance rejects craft"),HearthwardWorkshop::Commit(NPC,TEXT("craft"),TEXT("arrows"),2));TestEqual(TEXT("No partial output"),NPC->GetItemCount(TEXT("arrow")),8);
    NPC->TryAdd(TEXT("axe"),1);NPC->TryAdd(TEXT("wood"),1);
    const FGuid Axe=NPC->FirstInstance(TEXT("axe"));NPC->WearInstance(Axe,60);
    TestFalse(TEXT("Missing stone leaves durability and wood intact"),HearthwardWorkshop::Commit(NPC,TEXT("repair"),TEXT("axe"),1));
    TestEqual(TEXT("Original instance durability"),NPC->FindInstance(Axe)->Durability,20.);TestEqual(TEXT("No partial repair cost"),NPC->GetItemCount(TEXT("wood")),2);
    NPC->TryAdd(TEXT("stone"),3);TestTrue(TEXT("Own selected equipment repaired"),HearthwardWorkshop::Commit(NPC,TEXT("repair"),TEXT("axe"),1));
    TestEqual(TEXT("Repair wood cost"),NPC->GetItemCount(TEXT("wood")),0);TestEqual(TEXT("Maximum restored"),NPC->FindInstance(Axe)->Durability,80.);
    TestFalse(TEXT("Completed repair cannot settle again"),HearthwardWorkshop::Commit(NPC,TEXT("repair"),TEXT("axe"),1));
    NPC->TryAdd(TEXT("axe"),1);FGuid SecondAxe;
    for(const auto& Instance:NPC->Snapshot().Instances)if(Instance.Definition==TEXT("axe") && Instance.Id!=Axe)SecondAxe=Instance.Id;
    TestTrue(TEXT("Second instance exists"),SecondAxe.IsValid());
    NPC->WearInstance(SecondAxe,40);NPC->TryAdd(TEXT("wood"),6);NPC->TryAdd(TEXT("stone"),6);
    TestFalse(TEXT("Two same-type instances need a target"),HearthwardWorkshop::Commit(NPC,TEXT("repair"),TEXT("axe"),1));
    TestTrue(TEXT("Explicit second instance repairs"),HearthwardWorkshop::Commit(NPC,TEXT("repair"),TEXT("axe"),1,SecondAxe));
    TestEqual(TEXT("First instance unchanged"),NPC->FindInstance(Axe)->Durability,80.);
    TestEqual(TEXT("Selected second instance repaired"),NPC->FindInstance(SecondAxe)->Durability,80.);
    TestEqual(TEXT("Still no player debit"),Player->GetItemCount(TEXT("wood")),10);return true;
}
#endif
