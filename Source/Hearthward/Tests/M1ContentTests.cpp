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
struct FM1World
{
    UWorld* World;
    ACharacter* Player;
    APlayerController* Controller;
    UHearthwardGameplayComponent* Gameplay;
    UHearthwardInventoryComponent* Bag;
    UHearthwardCampaignSubsystem* Campaign;
    UHearthwardStorageSubsystem* Store;
    FM1World()
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
    ~FM1World()
    {
        World->EndPlay(EEndPlayReason::Quit);
        GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM1QuestRoundTrip,"Hearthward.Content.M1.QuestRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM1QuestRoundTrip::RunTest(const FString&)
{
    FM1World F;
    struct FRoute{FName Quest,Take,Deliver,Fact;};
    const FRoute Routes[]={{TEXT("side_exp_river_01"),TEXT("poi_river_relic"),TEXT("m1_camp_relic"),TEXT("m1:river_delivered")},
        {TEXT("side_exp_workshop_01"),TEXT("poi_workshop_tools"),TEXT("m1_camp_tools"),TEXT("m1:tools_delivered")}};
    for(const auto& Route:Routes)
    {
        TestFalse(TEXT("Undiscovered content has no journal answer"),F.Campaign->Available(Route.Quest));
        TestFalse(TEXT("Cannot deliver before taking"),F.Use(Route.Deliver));
        TestTrue(TEXT("Normal campaign interaction takes unique protected object"),F.Use(Route.Take));
        TestTrue(TEXT("Taking object discovers its own quest"),F.Campaign->Available(Route.Quest));
        TestEqual(TEXT("Taking is one step, no automatic reward"),F.Campaign->Progress(Route.Quest),1);
        TestEqual(TEXT("Return leg points to known camp"),F.Campaign->QuestLocation(Route.Quest),FName(TEXT("camp")));
        const FString BeforeDelivery=F.Campaign->State.Snapshot();FHearthwardCampaignState Restored;
        TestTrue(TEXT("Taken state parses under unchanged schema"),FHearthwardCampaignState::Parse(BeforeDelivery,Restored));
        F.Campaign->State=Restored;
        TestFalse(TEXT("Taken source cannot be used twice after restore"),F.Use(Route.Take));
        TestTrue(TEXT("Camp interaction commits the visible result"),F.Use(Route.Deliver));
        TestTrue(TEXT("Delivered fact exists"),F.Campaign->State.Facts.Contains(Route.Fact));
        TestFalse(TEXT("Old timeline cannot claim"),F.Campaign->Claim(Route.Quest,FGuid::NewGuid()));
        const int32 BeforeXP=F.Gameplay->Experience;
        TestTrue(TEXT("Existing reward transaction claims quest"),F.Campaign->Claim(Route.Quest,F.Store->GetTimelineEpoch()));
        TestEqual(TEXT("Existing side quest experience only"),F.Gameplay->Experience-BeforeXP,600);
        TestFalse(TEXT("No second reward"),F.Campaign->Claim(Route.Quest,F.Store->GetTimelineEpoch()));
        TestTrue(TEXT("Delivered state round trips"),FHearthwardCampaignState::Parse(F.Campaign->State.Snapshot(),Restored));
        TestTrue(TEXT("Visual event survives restore"),Restored.Facts.Contains(Route.Fact));
    }
    TestEqual(TEXT("No extra rescued population"),F.Campaign->State.People.Num(),10);
    TestEqual(TEXT("No extra occupation enemies"),F.Campaign->State.Total(),80);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM1CargoAtomic,"Hearthward.Content.M1.CargoCapacityAndPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM1CargoAtomic::RunTest(const FString&)
{
    FM1World F;
    const int32 Fill=int32(F.Bag->GetCapacity()*100)/FHearthwardInventoryState::FindItem(TEXT("wood"))->WeightHundredths;
    TestTrue(TEXT("Fill the real capacity"),F.Bag->TryAdd(TEXT("wood"),Fill)==EHearthwardInventoryResult::Success);
    TestTrue(TEXT("Full bag still receives an interaction reply"),F.Use(TEXT("poi_cart_wood")));
    TestFalse(TEXT("Rejected cargo stays at site"),F.Campaign->State.Facts.Contains(TEXT("m1:cart_wood")));
    TestEqual(TEXT("No partial cargo"),F.Bag->GetItemCount(TEXT("wood")),Fill);
    F.Bag->TryRemove(TEXT("wood"),Fill);
    TestTrue(TEXT("Retry with space collects the cargo"),F.Use(TEXT("poi_cart_wood")));
    TestEqual(TEXT("Exactly four wood in personal bag"),F.Bag->GetItemCount(TEXT("wood")),4);
    TestEqual(TEXT("No invented storage deposit"),F.Store->GetItemCount(TEXT("wood")),0);
    FHearthwardCampaignState Saved;TestTrue(TEXT("Cargo fact serializes"),FHearthwardCampaignState::Parse(F.Campaign->State.Snapshot(),Saved));F.Campaign->State=Saved;
    TestFalse(TEXT("No repeated cargo after restore"),F.Use(TEXT("poi_cart_wood")));
    TestTrue(TEXT("Other bundle remains available"),F.Use(TEXT("poi_cart_rope")));
    TestEqual(TEXT("Second bundle is real rope"),F.Bag->GetItemCount(TEXT("rope")),2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM1Knowledge,"Hearthward.Content.M1.LocalKnowledgeAndNoFreeMeat",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM1Knowledge::RunTest(const FString&)
{
    FM1World F;F.Gameplay->TrackedQuest=TEXT("side_exp_workshop_01");
    TestFalse(TEXT("Unknown M1 quest has no world marker"),HearthwardQuestGuidance::Resolve(F.Controller).Visible);
    TestTrue(TEXT("Survey sign can be read"),F.Use(TEXT("poi_ridge_01")));
    TestEqual(TEXT("Only one visible destination revealed"),F.Gameplay->Discovered.Num(),1);
    TestTrue(TEXT("The observed shed is known"),F.Gameplay->Discovered.Contains(TEXT("poi_workshop_01")));
    TestFalse(TEXT("Survey does not activate travel"),F.Gameplay->Activated.Contains(TEXT("poi_workshop_01")));
    TestTrue(TEXT("Known shed becomes guidable"),HearthwardQuestGuidance::Resolve(F.Controller).Visible);
    TestTrue(TEXT("Survey preserves old side_08 progress"),F.Campaign->State.Facts.Contains(TEXT("ridge_survey")));
    TestTrue(TEXT("Briefing first page"),F.Use(TEXT("m1_camp_board")));
    const FString First=F.Campaign->Feedback;
    TestTrue(TEXT("Briefing is short enough for HUD"),First.Len()<85);
    F.Use(TEXT("m1_camp_board"));
    TestTrue(TEXT("Repeated use advances briefing"),F.Campaign->Feedback!=First);
    for(int32 I=0;I<4;++I)F.Use(TEXT("m1_camp_board"));
    TestEqual(TEXT("Briefing wraps without losing first page"),F.Campaign->Feedback,First);
    const int32 Meat=F.Bag->GetItemCount(TEXT("meat"));
    TestTrue(TEXT("Tracks with no live animal report absence"),F.Use(TEXT("poi_tracks_01")));
    TestTrue(TEXT("Explicit no-animal reply"),F.Campaign->Feedback.Contains(TEXT("暂时没有")));
    TestEqual(TEXT("Tracks do not grant meat"),F.Bag->GetItemCount(TEXT("meat")),Meat);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM1DepositEvent,"Hearthward.Content.M1.DepositEventUsesCommittedTransfer",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM1DepositEvent::RunTest(const FString&)
{
    FM1World F;const FGuid Op=FGuid::NewGuid(),Epoch=F.Store->GetTimelineEpoch();
    F.Store->Transfer(F.Bag,true,TEXT("wood"),2,FGuid::NewGuid(),Epoch);
    TestFalse(TEXT("Failed transfer cannot furnish storage"),F.Campaign->State.Facts.Contains(TEXT("m1:materials_stored")));
    F.Bag->TryAdd(TEXT("wood"),2);
    const auto Moved=F.Store->Transfer(F.Bag,true,TEXT("wood"),2,Op,Epoch);
    TestEqual(TEXT("Real materials deposited"),Moved.MovedCount,2);
    TestTrue(TEXT("Event follows committed transfer"),F.Campaign->State.Facts.Contains(TEXT("m1:materials_stored")));
    F.Store->Transfer(F.Bag,true,TEXT("wood"),2,Op,Epoch);
    TestEqual(TEXT("Replay creates no decoration reward"),F.Store->GetItemCount(TEXT("wood")),2);
    FHearthwardCampaignState Saved;TestTrue(TEXT("Event persists"),FHearthwardCampaignState::Parse(F.Campaign->State.Snapshot(),Saved));
    TestTrue(TEXT("Restored event remains set"),Saved.Facts.Contains(TEXT("m1:materials_stored")));
    return true;
}

#if WITH_EDITOR
#include "../Time/HearthwardWorldClockSubsystem.h"
static FAutoConsoleCommandWithWorldAndArgs FM1StageCommand(TEXT("Hearthward.Test106.Stage"),
    TEXT("Isolated GUID save-pool only: load a production M1 location for scene/input review. Does not complete quests."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* World)
    {
        FString Pool;FGuid PoolId;
        if(!World || (Args.Num()!=1 && Args.Num()!=2) || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,PoolId))return;
        auto* Player=Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(World,0));
        auto* C=World->GetSubsystem<UHearthwardCampaignSubsystem>();const FName Id(*Args[0]);
        if(!Player || !C->HasLocation(Id))return;
        auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();if(!G || !G->Enabled)return;
        if(C->State.Phase==TEXT("prologue")){FHearthwardCampaignState Fixture;Fixture.Initialize(true);C->Restore(Fixture.Snapshot());}
        if(Args.Num()==2 && Args[1]==TEXT("day"))
        {
            auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
            const double Delta=FMath::Fmod(720.-Clock->GetSnapshot().MinuteOfDay+1440.,1440.);
            Clock->AdvanceCalendar(Delta);
        }
        const FVector Destination=C->Position(Id);
        auto* Source=World->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Source);Source->AddInstanceComponent(Root);Source->SetRootComponent(Root);Root->RegisterComponent();Source->SetActorLocation(Destination);
        auto* Streaming=NewObject<UWorldPartitionStreamingSourceComponent>(Source);Source->AddInstanceComponent(Streaming);Streaming->RegisterComponent();Streaming->EnableStreamingSource();
        const auto Attempts=MakeShared<int32>(0);const auto Timer=MakeShared<FTimerHandle>();
        World->GetTimerManager().SetTimer(*Timer,FTimerDelegate::CreateLambda([World,Player,C,Destination,Source,Attempts,Timer,Id]()
        {
            FVector Floor;const bool Ready=C->Ground(Destination,Floor);
            if(!Ready && ++*Attempts<90)return;
            World->GetTimerManager().ClearTimer(*Timer);
            if(Ready)
            {
                Player->SetActorLocation(Floor+FVector(0,-180,100),false,nullptr,ETeleportType::TeleportPhysics);
                Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Player->GetCharacterMovement()->StopMovementImmediately();
                Player->GetController()->SetControlRotation(FRotator(-12,90,0));
                C->State.Positions.Add(Id,Floor+FVector(0,0,80));
                FString Terrain=TEXT("x_cm,y_cm,z_cm\n");
                for(int32 X=-5;X<=5;++X)for(int32 Y=-5;Y<=5;++Y)
                {FVector Point;if(C->Ground(Floor+FVector(X*500,Y*500,0),Point))Terrain+=FString::Printf(TEXT("%.0f,%.0f,%.1f\n"),Point.X,Point.Y,Point.Z);}
                FFileHelper::SaveStringToFile(Terrain,*FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Task106/terrain.csv")));
                UE_LOG(LogTemp,Display,TEXT("M1 isolated staging %s: %s; input and route validation still required"),*Id.ToString(),*Floor.ToString());
            }
            else UE_LOG(LogTemp,Error,TEXT("M1 isolated staging failed: terrain unavailable %s"),*Id.ToString());
            Source->Destroy();
        }),.5f,true);
    }));
static FAutoConsoleCommandWithWorld FM1InspectCommand(TEXT("Hearthward.Test106.Inspect"),
    TEXT("Read-only M1 scene, quest and inventory snapshot for an isolated test pool."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        FString Pool;FGuid PoolId;
        if(!World || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,PoolId))return;
        auto* C=World->GetSubsystem<UHearthwardCampaignSubsystem>();auto* Player=UGameplayStatics::GetPlayerPawn(World,0);if(!Player)return;
        FString Report=TEXT("Layer: isolated production-map fixture; staging is not a player route.\nPlayer: ")+Player->GetActorLocation().ToString()+TEXT("\nPrompt: ")+C->InteractionPrompt()+TEXT("\nFeedback: ")+C->Feedback;
        Report+=TEXT("\nCampaign: ")+C->Snapshot();
        if(auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>())Report+=TEXT("\nGameplay: ")+G->SaveSnapshot();
        if(auto* Bag=Player->FindComponentByClass<UHearthwardInventoryComponent>())Report+=TEXT("\nInventory: ")+Bag->DescribeInventory();
        FFileHelper::SaveStringToFile(Report,*FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Task106/scene.txt")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }));
#endif
#endif
