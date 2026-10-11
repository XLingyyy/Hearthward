#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../UI/HearthwardQuestGuidance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "TimerManager.h"

namespace
{
struct FM2World
{
    UWorld* World;
    ACharacter* Player;
    APlayerController* Controller;
    UHearthwardGameplayComponent* Gameplay;
    UHearthwardInventoryComponent* Bag;
    UHearthwardCampaignSubsystem* Campaign;
    UHearthwardStorageSubsystem* Store;
    FM2World()
    {
        UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Player=World->SpawnActor<ACharacter>();Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
        Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
        Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize(true);
        Store=World->GetSubsystem<UHearthwardStorageSubsystem>();
        Gameplay->Discovered.Reset();Gameplay->Claimed.Reset();Gameplay->RewardFacts.Reset();Gameplay->Experience=0;
    }
    bool Use(FName Id)
    {
        const FVector At=Campaign->Position(Id);Campaign->State.Positions.Add(Id,At);
        Player->SetActorLocation(At,false,nullptr,ETeleportType::TeleportPhysics);
        return Campaign->InteractionTarget()==Id && Campaign->Interact();
    }
    ~FM2World()
    {
        World->EndPlay(EEndPlayReason::Quit);
        GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM2QuestRoundTrip,"Hearthward.Content.M2.QuestRoundTripAndLegacyFacts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM2QuestRoundTrip::RunTest(const FString&)
{
    FM2World F;
    struct FRoute{FName Quest,Observe,Take,Deliver,Fact;};
    const FRoute Routes[]={{TEXT("side_exp_well_01"),TEXT("poi_well_01"),TEXT("poi_well_keepsake"),TEXT("m2_camp_well"),TEXT("m2:well_delivered")},
        {TEXT("side_exp_signal_01"),TEXT("poi_signal_01"),TEXT("poi_signal_banner"),TEXT("m2_camp_signal"),TEXT("m2:signal_delivered")}};
    F.Campaign->Record(TEXT("m1:river_delivered"));
    for(const auto& R:Routes)
    {
        TestFalse(TEXT("Unknown quest hidden"),F.Campaign->Available(R.Quest));
        TestFalse(TEXT("No remote delivery"),F.Use(R.Deliver));
        TestFalse(TEXT("Read marks before opening concealed storage"),F.Use(R.Take));
        TestTrue(TEXT("Read actual marks"),F.Use(R.Observe));
        TestTrue(TEXT("Take protected token"),F.Use(R.Take));
        TestTrue(TEXT("Taking discovers this quest"),F.Campaign->Available(R.Quest));
        TestEqual(TEXT("Return marker is camp"),F.Campaign->QuestLocation(R.Quest),FName(TEXT("camp")));
        TestEqual(TEXT("One real step before delivery"),F.Campaign->Progress(R.Quest),1);
        FHearthwardCampaignState Saved;
        TestTrue(TEXT("Taken state saves"),FHearthwardCampaignState::Parse(F.Campaign->State.Snapshot(),Saved));F.Campaign->State=Saved;
        TestFalse(TEXT("No duplicate token after restore"),F.Use(R.Take));
        TestTrue(TEXT("Deliver token"),F.Use(R.Deliver));
        TestTrue(TEXT("World change is persistent"),F.Campaign->State.Facts.Contains(R.Fact));
        TestFalse(TEXT("No repeated delivery"),F.Use(R.Deliver));
        const int32 XP=F.Gameplay->Experience;
        TestTrue(TEXT("Claim through existing transaction"),F.Campaign->Claim(R.Quest,F.Store->GetTimelineEpoch()));
        TestEqual(TEXT("Exactly one sidequest reward"),F.Gameplay->Experience,XP+600);
        TestFalse(TEXT("Replay cannot award XP"),F.Campaign->Claim(R.Quest,F.Store->GetTimelineEpoch()));
    }
    TestTrue(TEXT("Existing M1 fact survives new content"),F.Campaign->State.Facts.Contains(TEXT("m1:river_delivered")));
    TestEqual(TEXT("No rescue added"),F.Campaign->State.People.Num(),10);
    TestEqual(TEXT("Garrison denominator unchanged"),F.Campaign->State.Total(),80);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM2Cellar,"Hearthward.Content.M2.FiniteCellarCargo",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM2Cellar::RunTest(const FString&)
{
    FM2World F;
    const int32 Fill=int32(F.Bag->GetCapacity()*100)/FHearthwardInventoryState::FindItem(TEXT("wood"))->WeightHundredths;
    F.Bag->TryAdd(TEXT("wood"),Fill);F.Use(TEXT("poi_cellar_food_1"));
    TestFalse(TEXT("Rejected bundle remains"),F.Campaign->State.Facts.Contains(TEXT("m2:cellar_food_1")));
    TestEqual(TEXT("No partial food"),F.Bag->GetItemCount(TEXT("cooked_food")),0);
    F.Bag->TryRemove(TEXT("wood"),Fill);TestTrue(TEXT("First bundle fits"),F.Use(TEXT("poi_cellar_food_1")));
    FHearthwardCampaignState Saved;TestTrue(TEXT("Save consumed bundle"),FHearthwardCampaignState::Parse(F.Campaign->State.Snapshot(),Saved));F.Campaign->State=Saved;
    TestFalse(TEXT("Restored bundle stays empty"),F.Use(TEXT("poi_cellar_food_1")));
    TestTrue(TEXT("Independent second bundle"),F.Use(TEXT("poi_cellar_food_2")));
    TestEqual(TEXT("Finite total food"),F.Bag->GetItemCount(TEXT("cooked_food")),4);
    TestEqual(TEXT("Nothing appears in storage"),F.Store->GetItemCount(TEXT("cooked_food")),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM2Knowledge,"Hearthward.Content.M2.LocalKnowledge",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM2Knowledge::RunTest(const FString&)
{
    FM2World F;F.Gameplay->TrackedQuest=TEXT("side_exp_well_01");
    TestFalse(TEXT("Unknown well has no marker"),HearthwardQuestGuidance::Resolve(F.Controller).Visible);
    TestTrue(TEXT("Read local path"),F.Use(TEXT("poi_backpath_01")));
    TestEqual(TEXT("One nearby point revealed"),F.Gameplay->Discovered.Num(),1);
    TestTrue(TEXT("Well known"),F.Gameplay->Discovered.Contains(TEXT("poi_well_01")));
    TestTrue(TEXT("Now guidance is available"),HearthwardQuestGuidance::Resolve(F.Controller).Visible);
    TestTrue(TEXT("Observation grants no travel"),F.Gameplay->Activated.IsEmpty());
    TestFalse(TEXT("Other new quest still unknown"),F.Campaign->Available(TEXT("side_exp_signal_01")));
    const auto Facts=F.Campaign->State.Facts;const int32 Known=F.Gameplay->Discovered.Num();
    TestTrue(TEXT("Can inspect tents"),F.Use(TEXT("poi_tents_01")));
    TestTrue(TEXT("Registry does not disclose unloaded enemies"),F.Campaign->Feedback.Contains(TEXT("暂时看不到")));
    TestEqual(TEXT("No lasting enemy fact"),F.Campaign->State.Facts.Num(),Facts.Num());
    TestEqual(TEXT("No map leakage"),F.Gameplay->Discovered.Num(),Known);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM2Events,"Hearthward.Content.M2.CampEventsRespectWorldState",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM2Events::RunTest(const FString&)
{
    FM2World F;
    TestFalse(TEXT("Owner cannot react to missing objects"),F.Use(TEXT("m2_event_owner")));
    F.Campaign->Record(TEXT("m2:well_delivered"));TestTrue(TEXT("Owner reacts to actual delivery"),F.Use(TEXT("m2_event_owner")));
    TestTrue(TEXT("Correct object named"),F.Campaign->Feedback.Contains(TEXT("三道刻痕")));
    TestFalse(TEXT("Departure waits for stage"),F.Use(TEXT("m2_event_departure")));
    F.Gameplay->Claimed.Add(TEXT("main_04"));TestTrue(TEXT("Stage permits short departure line"),F.Use(TEXT("m2_event_departure")));
    FHearthwardCampaignState Saved;TestTrue(TEXT("Events persist"),FHearthwardCampaignState::Parse(F.Campaign->State.Snapshot(),Saved));F.Campaign->State=Saved;
    TestFalse(TEXT("Owner reply does not repeat after load"),F.Use(TEXT("m2_event_owner")));
    TestFalse(TEXT("Departure reply does not repeat after load"),F.Use(TEXT("m2_event_departure")));
    F.Use(TEXT("poi_home_steps_01"));TestFalse(TEXT("Pre-victory steps are not home"),F.Campaign->State.Facts.Contains(TEXT("m2:home_evening")));
    F.Campaign->State.Victory=true;F.Use(TEXT("poi_home_steps_01"));TestFalse(TEXT("Victory alone cannot invent second camp"),F.Campaign->State.Facts.Contains(TEXT("m2:home_evening")));
    auto& Camp=F.World->GetSubsystem<UHearthwardCampSubsystem>()->State;Camp.Hometown=true;Camp.AddCamp(TEXT("hometown"),F.Campaign->Position(TEXT("hometown")));
    F.Use(TEXT("poi_home_steps_01"));TestTrue(TEXT("Usable second camp enables home response"),F.Campaign->State.Facts.Contains(TEXT("m2:home_evening")));
    const FString First=F.Campaign->Feedback;F.Use(TEXT("poi_home_steps_01"));TestTrue(TEXT("Later response changes without another reward"),First!=F.Campaign->Feedback);
    TestEqual(TEXT("Events never grant XP"),F.Gameplay->Experience,0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM2WorkEvent,"Hearthward.Content.M2.WorkInterruptionClearsOnRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM2WorkEvent::RunTest(const FString&)
{
    FM2World F;auto& Camp=F.World->GetSubsystem<UHearthwardCampSubsystem>()->State;
    TestFalse(TEXT("No fabricated incident"),F.Use(TEXT("m2_event_work")));
    Camp.AddCamp(TEXT("camp"),F.Campaign->Position(TEXT("camp")));Camp.Assign(TEXT("camp_wood"),0);
    auto* R=Camp.Regions.FindByPredicate([](const auto& Row){return Row.Id==TEXT("camp_wood");});
    if(!TestNotNull(TEXT("Existing wood job"),R))return false;R->Enabled=true;
    Camp.Advance(1,true,[](const auto&,const auto&){return true;});
    TestTrue(TEXT("Real missing source reports block"),F.Use(TEXT("m2_event_work")));
    TestTrue(TEXT("Specific recovery explanation"),F.Campaign->Feedback.Contains(TEXT("等待可采集来源")));
    TestTrue(TEXT("Record that explanation occurred"),F.Campaign->State.Facts.Contains(TEXT("m2:work_explained")));
    FHearthwardCampSource Source;Source.Id=TEXT("m2-test-tree");Source.Camp=TEXT("camp");Source.Item=TEXT("wood");Source.Capacity=Source.Remaining=10;Camp.Sources.Add(Source);
    Camp.Advance(1,true,[](const auto&,const auto&){return true;});
    TestTrue(TEXT("Same job resumes a real batch"),R->Batch.Active);
    TestFalse(TEXT("Recovered job no longer offers stop explanation"),F.Use(TEXT("m2_event_work")));
    TestEqual(TEXT("Only reserved production input consumed"),Camp.Sources.Last().Remaining,8);
    return true;
}
#endif
