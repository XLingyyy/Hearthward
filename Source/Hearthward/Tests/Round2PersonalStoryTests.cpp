#include "../Campaign/HearthwardPersonalStory.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2PersonalTest,"Hearthward.Round2.Personal.ChoicesVictoryUniqueDisplayAndStaleUI",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2PersonalTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Player=World->SpawnActor<ACharacter>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Game=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Game);Game->RegisterComponent();
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Director=World->SpawnActor<AHearthwardPersonalStoryDirector>();
    auto* Save=World->GetSubsystem<UHearthwardSaveSubsystem>();auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();const auto Epoch=Storage->GetTimelineEpoch();
    auto* NPC=World->SpawnActor<AActor>();
    auto* Talk=NewObject<UHearthwardPersonalInteraction>(NPC);NPC->AddInstanceComponent(Talk);NPC->SetRootComponent(Talk);Talk->RegisterComponent();Talk->Identity=TEXT("civilian_initial_02");
    Talk->Choose(Player,TEXT("camp"),Epoch);TestTrue(TEXT("Cannot skip site inspection"),Save->InteractionState().People.IsEmpty());
    Talk->Choose(Player,TEXT("begin"),Epoch);TestEqual(TEXT("Meeting is recorded for the existing person"),HearthwardPersonalStory::State(World,Talk->Identity).Step,FName(TEXT("met")));
    TestTrue(TEXT("Fixture supplies completed site observation"),Director->SetStep(Talk->Identity,TEXT("visited")));
    Talk->Choose(Player,TEXT("camp"),Epoch);TestEqual(TEXT("Choice commits"),HearthwardPersonalStory::State(World,Talk->Identity).Choice,FName(TEXT("camp")));
    const auto Revision=Save->InteractionState().Revision;Talk->Choose(Player,TEXT("camp"),Epoch);TestEqual(TEXT("Replay does not grant another result"),Save->InteractionState().Revision,Revision);
    Talk->Identity=TEXT("civilian_initial_04");Director->SetStep(Talk->Identity,TEXT("visited"));Talk->Choose(Player,TEXT("hometown"),Epoch);
    TestTrue(TEXT("Watcher ending cannot bypass victory"),HearthwardPersonalStory::State(World,Talk->Identity).Choice.IsNone());
    auto MakeSlot=[&](const TCHAR* Id,const TCHAR* Slot)
    {
        auto* Owner=World->SpawnActor<AActor>();auto* Use=NewObject<UHearthwardPersonalInteraction>(Owner);Owner->AddInstanceComponent(Use);Owner->SetRootComponent(Use);Use->RegisterComponent();
        Use->Identity=FName(Id);Use->DisplayCamp=TEXT("camp");Use->DisplaySlot=FName(Slot);return Use;
    };
    auto* First=MakeSlot(TEXT("display_camp_1"),TEXT("1"));auto* Second=MakeSlot(TEXT("display_camp_2"),TEXT("2"));
    First->Preview(Player,TEXT("craftsman_tools"),Epoch);TestTrue(TEXT("Preview does not save"),Save->InteractionState().Displays.IsEmpty());
    First->ConfirmDisplay(Player,Epoch);TestEqual(TEXT("Confirmed display references fact"),Save->InteractionState().Displays[0].Fact,FName(TEXT("craftsman_tools")));
    Second->Preview(Player,TEXT("craftsman_tools"),Epoch);Second->ConfirmDisplay(Player,Epoch);
    int32 Count=0;for(const auto& D:Save->InteractionState().Displays)Count+=D.Fact==TEXT("craftsman_tools");TestEqual(TEXT("Move leaves exactly one display"),Count,1);
    Second->Preview(Player,NAME_None,Epoch);Storage->AdvanceTimeline();Second->ConfirmDisplay(Player,Epoch);
    TestEqual(TEXT("Stale UI cannot remove current display"),Save->InteractionState().Displays[1].Fact,FName(TEXT("craftsman_tools")));
    TestTrue(TEXT("Story and display add no inventory copies"),Bag->Snapshot().Instances.IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2StoryConservationTest,"Hearthward.Round2.Personal.AllEndingsPreserveEconomyAndGarrison",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2StoryConservationTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT{GEngine->DestroyWorldContext(World);World->DestroyWorld(false);};
    auto* Player=World->SpawnActor<ACharacter>();
    auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();Bag->TryAdd(TEXT("herb"),2);
    auto* Game=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Game);Game->RegisterComponent();Game->Discovered.Add(TEXT("poi_herb_01"));
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Director=World->SpawnActor<AHearthwardPersonalStoryDirector>();
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();Campaign->State.Victory=true;
    auto* Dead=Campaign->Enemy(TEXT("home_workshops_18"));Dead->Combat.Health=0;Dead->Combat.bStunned=true;
    auto* Camp=World->GetSubsystem<UHearthwardCampSubsystem>();auto* Nature=World->GetSubsystem<UHearthwardNatureSubsystem>();
    const FString CampaignBefore=Campaign->State.Snapshot(),CampBefore=Camp->State.Snapshot(),NatureBefore=Nature->State.Snapshot();
    const auto BagBefore=Bag->Snapshot();
    const auto Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    for(const FName Person:{FName(TEXT("civilian_initial_02")),FName(TEXT("civilian_initial_03")),FName(TEXT("civilian_initial_04"))})
    {
        auto* NPC=World->SpawnActor<AActor>();auto* Talk=NewObject<UHearthwardPersonalInteraction>(NPC);NPC->AddInstanceComponent(Talk);NPC->SetRootComponent(Talk);Talk->RegisterComponent();Talk->Identity=Person;
        NPC->SetActorHiddenInGame(true);Talk->Choose(Player,TEXT("begin"),Epoch);
        TestTrue(TEXT("Hidden person cannot start story"),HearthwardPersonalStory::State(World,Person).Step.IsNone());NPC->SetActorHiddenInGame(false);
        Talk->Choose(Player,TEXT("begin"),Epoch);TestEqual(TEXT("Visible existing person begins once"),HearthwardPersonalStory::State(World,Person).Step,FName(TEXT("met")));
        TestTrue(TEXT("Fixture supplies site observation"),Director->SetStep(Person,TEXT("visited")));
        Talk->Choose(Player,TEXT("hometown"),Epoch);Talk->Choose(Player,TEXT("hometown"),Epoch);
        TestEqual(TEXT("Ending belongs to this original person"),HearthwardPersonalStory::State(World,Person).Step,FName(TEXT("remembered")));
    }
    const auto BagAfter=Bag->Snapshot();
    TestTrue(TEXT("All endings preserve every inventory field"),FHearthwardInventorySnapshot::StaticStruct()->CompareScriptStruct(&BagBefore,&BagAfter,0));
    TestEqual(TEXT("Stories cannot change garrison, permanent deaths or rewards"),Campaign->State.Snapshot(),CampaignBefore);
    TestEqual(TEXT("Stories cannot create labor or camp production"),Camp->State.Snapshot(),CampBefore);
    TestEqual(TEXT("Stories cannot create harvests or change refresh schedules"),Nature->State.Snapshot(),NatureBefore);
    return true;
}
#endif
