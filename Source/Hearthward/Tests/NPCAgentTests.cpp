#include "../AI/HearthwardAgentContract.h"
#include "../AI/HearthwardNPCMemory.h"
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
