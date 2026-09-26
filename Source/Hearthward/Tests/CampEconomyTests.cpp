#include "../Camp/HearthwardCampState.h"
#include "../Inventory/HearthwardInventoryState.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "../Save/HearthwardSaveGame.h"
#include "../Gameplay/HearthwardGameplayComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FHearthwardCampState CampForage(int32 Workers,int32 Patches)
{
    FHearthwardCampState S;S.AddCamp(TEXT("camp"),FVector::ZeroVector);
    for(int32 I=0;I<Workers;++I)S.Assign(TEXT("camp_forage"),I);
    S.Regions[0].Enabled=true;S.Regions[0].ToRations=true;
    for(int32 I=0;I<Patches;++I)
    {FHearthwardCampSource P;P.Id=FString::FromInt(I);P.Camp=TEXT("camp");P.Item=TEXT("wild_food");P.Capacity=P.Remaining=16;P.RefreshMinutes=2880;S.Sources.Add(P);}
    return S;
}
bool CampExchange(FHearthwardInventoryState& Store,const TMap<FName,int32>& In,const TMap<FName,int32>& Out)
{
    auto After=Store;for(const auto& C:In)if(After.Remove(C.Key,C.Value)!=EHearthwardInventoryResult::Success)return false;
    for(const auto& C:Out)if(After.Add(C.Key,C.Value)!=EHearthwardInventoryResult::Success)return false;Store=MoveTemp(After);return true;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampFiniteBudgetTest,"Hearthward.Camp.FiniteBudgetAndWholeMeals",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampFiniteBudgetTest::RunTest(const FString&)
{
    for(int32 Activity:{1,2})
    {
        auto S=CampForage(Activity==1?4:5,Activity==1?3:4);FHearthwardInventoryState Store(true);
        float Hunger[2]={25,25};int32 Meals=0;bool Short=false;
        for(int32 I=0;I<960;++I)
        {
            S.Advance(90,false,[&](const auto& In,const auto& Out){return CampExchange(Store,In,Out);});
            for(float& H:Hunger){H-=float(90.*100/2880*Activity);if(H<25){if(!S.Eat(H))Short=true;else Meals++;}}
        }
        TestEqual(TEXT("Sixty days actual finite production"),S.Regions[0].Completed,Activity==1?960:1200);
        TestFalse(TEXT("Sixty days do not miss a whole meal"),Short);
        TestEqual(TEXT("Actual integer meal count"),Meals,Activity==1?150:300);
        TestTrue(TEXT("Approved finite-source net balance"),FMath::IsNearlyEqual(S.Rations(),Activity==1?310.:160.,1.e-6));
        TestTrue(TEXT("Whole state remains valid"),S.Validate());
    }
    auto Missing=CampForage(4,0);FHearthwardInventoryState Store(true);
    Missing.Advance(1440,true,[&](const auto& In,const auto& Out){return CampExchange(Store,In,Out);});
    TestEqual(TEXT("No source creates no food"),Missing.Regions[0].Completed,0);TestEqual(TEXT("Base consumption still occurs"),Missing.Rations(),76.);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampClockTest,"Hearthward.Camp.PartitionSleepAndRestoration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampClockTest::RunTest(const FString&)
{
    auto Whole=CampForage(4,3),Split=Whole;FHearthwardInventoryState Store(true);
    auto Exchange=[&](const auto& In,const auto& Out){return CampExchange(Store,In,Out);};
    Whole.Advance(480,true,Exchange);for(int32 I=0;I<4800;++I)Split.Advance(.1,true,Exchange);
    TestEqual(TEXT("Sleep gives five completed real batches"),Whole.Regions[0].Completed,5);
    TestEqual(TEXT("One reserved input remains in flight"),Whole.Sources[0].Remaining,10);
    TestTrue(TEXT("Fractional labor is retained"),FMath::IsNearlyEqual(Whole.Regions[0].Batch.Work,120.));
    TestTrue(TEXT("Small updates preserve rations"),FMath::IsNearlyEqual(Whole.Rations(),Split.Rations(),1.e-6));
    TestTrue(TEXT("Small updates preserve production progress"),FMath::IsNearlyEqual(Whole.Regions[0].Batch.Work,Split.Regions[0].Batch.Work,1.e-6));
    FHearthwardCampState Restored;TestTrue(TEXT("Economy snapshot parses"),FHearthwardCampState::Parse(Whole.Snapshot(),Restored));
    Whole.Advance(4000,true,Exchange);Restored.Advance(4000,true,Exchange);
    TestEqual(TEXT("Reload does not duplicate reserved inputs or refreshes"),Whole.Snapshot(),Restored.Snapshot());
    auto Blocked=CampForage(4,1);Blocked.Sources[0].Remaining=0;Blocked.Sources[0].Due=0;Blocked.Sources[0].Blocked=true;
    Blocked.Advance(10000,true,Exchange);TestEqual(TEXT("Occupied refresh site waits"),Blocked.Regions[0].Completed,0);
    Blocked.Sources[0].Blocked=false;Blocked.Advance(90,true,Exchange);TestEqual(TEXT("Unblocked site replenishes once"),Blocked.Regions[0].Completed,1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampLaborTest,"Hearthward.Camp.UniqueLaborAndBrotherContribution",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampLaborTest::RunTest(const FString&)
{
    auto S=CampForage(4,3);S.AddCamp(TEXT("hometown"),FVector(100000,0,0));
    TestTrue(TEXT("Brother uses the fifth physical slot"),S.Assign(TEXT("camp_forage"),31));
    TestFalse(TEXT("Cannot silently replace a worker in a full region"),S.Assign(TEXT("camp_forage"),30));
    S.Regions[0].BrotherEfficiency=3;FHearthwardInventoryState Store(true);auto Exchange=[&](const auto& In,const auto& Out){return CampExchange(Store,In,Out);};
    S.Advance(360./7,false,Exchange);TestEqual(TEXT("Four plus a working brother gives seven labor"),S.Regions[0].Completed,1);
    TestTrue(TEXT("Transfer same person to second camp"),S.Assign(TEXT("hometown_forage"),0));
    TestFalse(TEXT("Old region loses that person"),S.Regions[0].Workers.Contains(0));TestEqual(TEXT("Second camp adds no population"),S.Population(),20);
    auto Sleeping=CampForage(0,1);Sleeping.Assign(TEXT("camp_forage"),31);Sleeping.Regions[0].BrotherEfficiency=3;
    Sleeping.Advance(480,true,Exchange);TestEqual(TEXT("Sleeping brother supplies no labor"),Sleeping.Regions[0].Completed,0);
    TestEqual(TEXT("No labor consumes no source"),Sleeping.Sources[0].Remaining,16);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampProcessingTest,"Hearthward.Camp.InputEscrowAndFacilitySpeed",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampProcessingTest::RunTest(const FString&)
{
    auto S=CampForage(0,0);FHearthwardInventoryState Store(true);Store.Add(TEXT("wood"),2);
    FHearthwardCampFacility B;B.Id=FGuid::NewGuid();B.Kind=TEXT("workbench");B.Camp=TEXT("camp");B.Level=2;S.Facilities.Add(B);
    auto& R=S.Regions.AddDefaulted_GetRef();R.Id=TEXT("processing");R.Camp=TEXT("camp");R.Facility=B.Id;R.Job=TEXT("rope");R.Enabled=true;S.Assign(R.Id,0);
    auto Exchange=[&](const auto& In,const auto& Out){return CampExchange(Store,In,Out);};
    S.Advance(100,false,Exchange);TestEqual(TEXT("Batch input is spent exactly once at start"),Store.GetCount(TEXT("wood")),0);
    TestEqual(TEXT("No early output"),Store.GetCount(TEXT("rope")),0);
    R.Enabled=false;S.Advance(500,true,Exchange);TestEqual(TEXT("Pause retains consumed input and labor"),R.Batch.Work,125.);
    R.Enabled=true;S.Advance(188,true,Exchange);TestEqual(TEXT("Level II completes in 288 worker minutes"),Store.GetCount(TEXT("rope")),1);
    S.Advance(1440,true,Exchange);TestEqual(TEXT("Missing input stops next batch"),Store.GetCount(TEXT("rope")),1);
    Store.Add(TEXT("wood"),2);S.Advance(1,true,Exchange);R.Batch={};R.Enabled=false;
    TestEqual(TEXT("Explicit batch cancellation does not mint refunded input"),Store.GetCount(TEXT("wood")),0);
    TestEqual(TEXT("Cancellation gives no unfinished output"),Store.GetCount(TEXT("rope")),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampUpgradeTest,"Hearthward.Camp.GrowthRefundAndSnapshotRejection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampUpgradeTest::RunTest(const FString&)
{
    auto S=CampForage(0,0);TestFalse(TEXT("Missing gates prevent first growth"),S.UpgradeReason().IsEmpty());
    FHearthwardCampFacility B;B.Id=FGuid::NewGuid();B.Camp=TEXT("camp");B.Kind=TEXT("workbench");S.Facilities.Add(B);
    TestTrue(TEXT("Rescue is an actual unique fact"),S.Rescue(TEXT("rescued_1")));TestFalse(TEXT("Duplicate rescue cannot duplicate population"),S.Rescue(TEXT("rescued_1")));
    TestTrue(TEXT("Both first growth gates satisfied"),S.UpgradeReason().IsEmpty());
    TMap<FName,int32> Paid;for(int32 Level=1;Level<=3;++Level)for(const auto& M:HearthwardCamp::BuildCost(TEXT("workbench"),Level))Paid.FindOrAdd(M.Key)+=M.Value;
    const auto Refund=HearthwardCamp::Refund(Paid);TestEqual(TEXT("Refund cumulative actual wood with floor"),Refund.FindRef(TEXT("wood")),217);
    TestTrue(TEXT("Gift or migrated facility refunds nothing"),HearthwardCamp::Refund({}).IsEmpty());
    FHearthwardCampState Parsed;TestTrue(TEXT("Valid paid state roundtrip"),FHearthwardCampState::Parse(S.Snapshot(),Parsed));
    Parsed.Regions[0].Workers={0};Parsed.Regions[1].Workers={0};TestFalse(TEXT("Duplicate worker in snapshot rejected"),FHearthwardCampState::Parse(Parsed.Snapshot(),S));
    TestFalse(TEXT("Missing fields rejected"),FHearthwardCampState::Parse(TEXT("{}"),S));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampReservationTest,"Hearthward.Camp.SharedAndPersonalConstructionReservations",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampReservationTest::RunTest(const FString&)
{
    auto* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
    auto* Store=W->GetSubsystem<UHearthwardStorageSubsystem>();auto* Actor=W->SpawnActor<AActor>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Actor);Actor->AddInstanceComponent(Bag);Bag->RegisterComponent();
    Store->Adjust({},{{TEXT("wood"),100}});const FGuid Ticket=FGuid::NewGuid();
    TestTrue(TEXT("Build reserves shared cost"),Store->Reserve(Ticket,{{TEXT("wood"),72}}));
    TestFalse(TEXT("Processing cannot steal reserved materials"),Store->Adjust({{TEXT("wood"),29}},{{TEXT("rope"),1}}));
    TestEqual(TEXT("Visible shared quantity unchanged before completion"),Store->GetItemCount(TEXT("wood")),100);
    TestEqual(TEXT("Transfer cannot take reserved quantity"),Store->Transfer(Bag,false,TEXT("wood"),29,FGuid::NewGuid(),Store->GetTimelineEpoch()).MovedCount,0);
    Store->Release(Ticket);TestEqual(TEXT("Cancellation releases all materials"),Store->Available(TEXT("wood")),100);
    Bag->TryAdd(TEXT("wood"),10);TestTrue(TEXT("Reserve personal build cost"),Bag->ReserveMaterials({{TEXT("wood"),8}}));
    TestFalse(TEXT("Another recipe cannot steal personal materials"),Bag->TryConsume({{TEXT("wood"),3}})==EHearthwardInventoryResult::Success);
    Bag->ReleaseMaterials();TestEqual(TEXT("Personal cancellation preserves inventory"),Bag->Available(TEXT("wood")),10);
    W->DestroyWorld(false);GEngine->DestroyWorldContext(W);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampGrowthSaveTest,"Hearthward.Camp.GrowthSaveLimits",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampGrowthSaveTest::RunTest(const FString&)
{
    auto* Pool=NewObject<UHearthwardSaveGame>();auto& Point=Pool->Points.AddDefaulted_GetRef();
    Point.SaveId=FGuid::NewGuid();Point.CampaignId=FGuid::NewGuid();auto& S=Point.World;
    S.Map=TEXT("PROTOTYPE_ONLY");S.SurvivalVersion=1;S.NPCStateVersion=HearthwardSave::NPCStateVersion;S.NPCMemory.Campaign=Point.CampaignId;
    auto* Gameplay=NewObject<UHearthwardGameplayComponent>();Gameplay->CampTier=3;S.Gameplay=Gameplay->SaveSnapshot();
    FHearthwardCampState Camp;Camp.Tier=3;Camp.AddCamp(TEXT("camp"),FVector::ZeroVector);S.CampEconomy=Camp.Snapshot();
    S.BrotherHealth=120;S.BrotherStamina=110;
    TestTrue(TEXT("Tier three brother maxima are valid in the full save pool"),HearthwardSave::Validate(*Pool));
    S.BrotherHealth=121;TestFalse(TEXT("Reject health above tier maximum"),HearthwardSave::Validate(*Pool));
    S.BrotherHealth=120;S.BrotherStamina=111;TestFalse(TEXT("Reject stamina above tier maximum"),HearthwardSave::Validate(*Pool));
    S.BrotherStamina=110;S.CampEconomy.Reset();TestTrue(TEXT("Legacy tier bonus migrates with the same maxima"),HearthwardSave::Validate(*Pool));
    return true;
}
#endif
