#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"

namespace
{
void StageVictory(FHearthwardCampaignState& State)
{
    State.Phase=TEXT("reclaimed");State.Victory=true;
    for(auto& E:State.Enemies)E.Combat.Health=0;
    for(const auto& Z:HearthwardCampaign::Rows(TEXT("zones")))State.Flags.Add(FName(*HearthwardData::Text(Z->AsObject(),TEXT("id"))));
    State.ResolveUntriggered();
}
struct FM3World
{
    UWorld* World;
    ACharacter* Player;
    UHearthwardGameplayComponent* Gameplay;
    UHearthwardSurvivalComponent* Survival;
    UHearthwardCampaignSubsystem* Campaign;
    UHearthwardCampSubsystem* Camp;
    UHearthwardStorageSubsystem* Store;
    FM3World()
    {
        UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Player=World->SpawnActor<ACharacter>();auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
        auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
        Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
        Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize(true);
        Camp=World->GetSubsystem<UHearthwardCampSubsystem>();Store=World->GetSubsystem<UHearthwardStorageSubsystem>();
        Gameplay->Discovered.Reset();Gameplay->Claimed.Reset();Gameplay->RewardFacts.Reset();Gameplay->Experience=0;
    }
    void Home(){StageVictory(Campaign->State);Camp->State.Hometown=true;Camp->State.AddCamp(TEXT("hometown"),Campaign->Position(TEXT("hometown")));}
    bool Use(FName Id)
    {
        const FVector At=Campaign->Position(Id);Campaign->State.Positions.Add(Id,At);Player->SetActorLocation(At,false,nullptr,ETeleportType::TeleportPhysics);
        return Campaign->InteractionTarget()==Id && Campaign->Interact();
    }
    ~FM3World(){World->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM3Lamp,"Hearthward.Content.M3.LampTransactionAndLegacy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM3Lamp::RunTest(const FString&)
{
    FM3World F;
    TestFalse(TEXT("No premature home interaction"),F.Use(TEXT("m3_home_lamp")));
    F.Campaign->State.Victory=true;TestFalse(TEXT("Victory alone is insufficient"),F.Use(TEXT("m3_home_lamp")));
    F.Home();TestTrue(TEXT("Inspect actual lamp"),F.Use(TEXT("m3_home_lamp")));
    TestTrue(TEXT("Quest is discoverable"),F.Campaign->Available(TEXT("side_home_light_01")));
    TestEqual(TEXT("Guidance remains in hometown"),F.Campaign->QuestLocation(TEXT("side_home_light_01")),FName(TEXT("m3_home_lamp")));
    F.Store->Adjust({},{{TEXT("wood"),2}});F.Use(TEXT("m3_home_lamp_repair"));
    TestEqual(TEXT("Missing rope does not partly consume wood"),F.Store->GetItemCount(TEXT("wood")),2);
    TestFalse(TEXT("No unearned repair"),F.Campaign->State.Facts.Contains(TEXT("m3:lamp_repaired")));
    F.Store->Adjust({},{{TEXT("rope"),2}});const FGuid Reservation=FGuid::NewGuid();F.Store->Reserve(Reservation,{{TEXT("rope"),2}});
    F.Use(TEXT("m3_home_lamp_repair"));TestEqual(TEXT("Reserved materials protected"),F.Store->GetItemCount(TEXT("wood")),2);
    F.Store->Release(Reservation);TestTrue(TEXT("Repair at actual anchor"),F.Use(TEXT("m3_home_lamp_repair")));
    TestTrue(TEXT("Persistent lamp"),F.Campaign->State.Facts.Contains(TEXT("m3:lamp_repaired")));
    TestEqual(TEXT("Exact wood cost"),F.Store->GetItemCount(TEXT("wood")),0);TestEqual(TEXT("Exact rope cost"),F.Store->GetItemCount(TEXT("rope")),0);
    FHearthwardCampaignState Saved;TestTrue(TEXT("Round trip"),FHearthwardCampaignState::Parse(F.Campaign->State.Snapshot(),Saved));F.Campaign->State=Saved;
    TestFalse(TEXT("Load cannot repeat repair"),F.Use(TEXT("m3_home_lamp_repair")));
    TestFalse(TEXT("Stale epoch cannot claim"),F.Campaign->Claim(TEXT("side_home_light_01"),FGuid::NewGuid()));
    TestTrue(TEXT("Claim reward"),F.Campaign->Claim(TEXT("side_home_light_01"),F.Store->GetTimelineEpoch()));
    TestFalse(TEXT("No double reward"),F.Campaign->Claim(TEXT("side_home_light_01"),F.Store->GetTimelineEpoch()));
    TestEqual(TEXT("600 XP once"),F.Gameplay->Experience,600);
    TestTrue(TEXT("Discovered lamp and claimed quest pass save validation"),UHearthwardGameplayComponent::ValidateSnapshot(F.Gameplay->SaveSnapshot()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM3Meal,"Hearthward.Content.M3.RealMealAndPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM3Meal::RunTest(const FString&)
{
    FM3World F;F.Home();F.Campaign->Record(TEXT("public_meal"));
    TestEqual(TEXT("Old meals cannot finish new quest"),F.Campaign->Progress(TEXT("side_home_meal_01")),0);
    TestFalse(TEXT("Must prepare table"),F.Use(TEXT("m3_home_meal")));
    TestTrue(TEXT("Prepare table"),F.Use(TEXT("m3_home_table")));F.Survival->Hunger()=100;
    const double Rations=F.Camp->State.Rations();F.Use(TEXT("m3_home_meal"));
    TestEqual(TEXT("Full hunger consumes nothing"),F.Camp->State.Rations(),Rations);
    TestFalse(TEXT("No fake meal"),F.Campaign->State.Facts.Contains(TEXT("m3:home_meal")));
    F.Survival->Hunger()=50;F.Camp->State.RationHalfPoints=8;F.Use(TEXT("m3_home_meal"));
    TestEqual(TEXT("Short rations consume nothing"),F.Camp->State.RationHalfPoints,int64(8));
    F.Camp->State.RationHalfPoints=200;TestTrue(TEXT("Eat through local table"),F.Use(TEXT("m3_home_meal")));
    TestEqual(TEXT("Existing cost of five points"),F.Camp->State.Rations(),95.);
    TestEqual(TEXT("Existing hunger restoration"),F.Survival->Hunger(),90.f);
    TestTrue(TEXT("Specific home meal recorded"),F.Campaign->State.Facts.Contains(TEXT("m3:home_meal")));
    FHearthwardCampaignState Saved;TestTrue(TEXT("Meal fact saves"),FHearthwardCampaignState::Parse(F.Campaign->State.Snapshot(),Saved));F.Campaign->State=Saved;
    FHearthwardCampState Economy;TestTrue(TEXT("Rations save"),FHearthwardCampState::Parse(F.Camp->State.Snapshot(),Economy));F.Camp->State=Economy;
    TestFalse(TEXT("No repeated table charge after restore"),F.Use(TEXT("m3_home_meal")));
    TestTrue(TEXT("Quest reward"),F.Campaign->Claim(TEXT("side_home_meal_01"),F.Store->GetTimelineEpoch()));
    TestFalse(TEXT("Reward once"),F.Campaign->Claim(TEXT("side_home_meal_01"),F.Store->GetTimelineEpoch()));
    TestTrue(TEXT("Discovered table and claimed quest pass save validation"),UHearthwardGameplayComponent::ValidateSnapshot(F.Gameplay->SaveSnapshot()));
    TestEqual(TEXT("Population unchanged"),F.Camp->State.Population(),20);TestEqual(TEXT("Hostile denominator unchanged"),F.Campaign->State.Total(),80);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM3MenuMeal,"Hearthward.Content.M3.MenuMealUsesActualCamp",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM3MenuMeal::RunTest(const FString&)
{
    FM3World F;F.Home();F.Use(TEXT("m3_home_table"));F.Survival->Hunger()=30;
    F.Camp->State.AddCamp(TEXT("camp"),F.Campaign->Position(TEXT("camp")));F.Player->SetActorLocation(F.Campaign->Position(TEXT("camp")));
    TestTrue(TEXT("Original camp meal still works"),F.Camp->EatMeal(false,F.Store->GetTimelineEpoch()));
    TestFalse(TEXT("Original camp cannot finish home quest"),F.Campaign->State.Facts.Contains(TEXT("m3:home_meal")));
    F.Player->SetActorLocation(F.Campaign->Position(TEXT("hometown")));
    TestTrue(TEXT("Ordinary hometown menu also counts"),F.Camp->EatMeal(false,F.Store->GetTimelineEpoch()));
    TestTrue(TEXT("Same home fact"),F.Campaign->State.Facts.Contains(TEXT("m3:home_meal")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FM3Visuals,"Hearthward.Content.M3.ThreeTiersAndFacilityState",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FM3Visuals::RunTest(const FString&)
{
    FM3World F;F.Home();const FVector Home=F.Campaign->Position(TEXT("hometown"));F.Player->SetActorLocation(Home);
    auto* Floor=F.World->SpawnActor<AActor>();Floor->Tags.Add(TEXT("Hearthward.NatureGround"));
    auto* Mesh=NewObject<UStaticMeshComponent>(Floor);Floor->AddInstanceComponent(Mesh);Floor->SetRootComponent(Mesh);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Mesh->SetWorldScale3D(FVector(200,200,1));Mesh->SetCollisionObjectType(ECC_WorldStatic);Mesh->RegisterComponent();Floor->SetActorLocation(Home-FVector(0,0,150));
    auto Visible=[&](FName Tag){int32 Count=0;for(TActorIterator<AActor> It(F.World);It;++It)if(It->ActorHasTag(TEXT("CampLife:hometown"))){TInlineComponentArray<UStaticMeshComponent*> Parts(*It);for(auto* P:Parts)if(P->ComponentHasTag(Tag) && P->IsVisible())++Count;}return Count;};
    F.Campaign->Tick(1);TestEqual(TEXT("Tier one no storage display"),Visible(TEXT("CampTier2")),0);
    F.Camp->State.Tier=4;F.Campaign->Tick(1);TestEqual(TEXT("Tier four storage display"),Visible(TEXT("CampTier2")),1);
    TestEqual(TEXT("No invented workbench"),Visible(TEXT("CampWork")),0);
    FHearthwardCampFacility Work;Work.Id=FGuid::NewGuid();Work.Camp=TEXT("hometown");Work.Kind=TEXT("workbench");F.Camp->State.Facilities.Add(Work);
    F.Campaign->Tick(1);TestEqual(TEXT("Actual workbench adds work corner"),Visible(TEXT("CampWork")),2);
    F.Camp->State.Facilities.Last().Paused=true;F.Campaign->Tick(1);TestEqual(TEXT("Paused facility hides work corner"),Visible(TEXT("CampWork")),0);
    F.Camp->State.Tier=7;F.Campaign->Tick(1);TestEqual(TEXT("Unrepaired home has no expanded lamps"),Visible(TEXT("CampTier3")),0);
    F.Campaign->Record(TEXT("m3:lamp_repaired"));F.Campaign->Tick(1);TestEqual(TEXT("Tier seven restored lighting"),Visible(TEXT("CampTier3")),4);
    return true;
}

// Fixture commands are compiled only for development and require an isolated save pool.
static FAutoConsoleCommandWithWorldAndArgs FM3Setup(TEXT("Hearthward.Test108.Setup"),TEXT("Stage M3 endgame fixture; not evidence of a conquest route."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* World)
    {
        FString Pool;FGuid Id;if(!World || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id))return;
        auto* P=UGameplayStatics::GetPlayerPawn(World,0);auto* G=P?P->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;if(!G)return;
        auto* C=World->GetSubsystem<UHearthwardCampaignSubsystem>();auto* Camp=World->GetSubsystem<UHearthwardCampSubsystem>();
        StageVictory(C->State);
        Camp->State.Hometown=true;Camp->State.AddCamp(TEXT("hometown"),C->Position(TEXT("hometown")));
        World->GetSubsystem<UHearthwardStorageSubsystem>()->Adjust({},{{TEXT("wood"),2},{TEXT("rope"),2}});
        Camp->State.RationHalfPoints=200;if(auto* S=P->FindComponentByClass<UHearthwardSurvivalComponent>())S->Hunger()=50;
        UE_LOG(LogTemp,Display,TEXT("M3 isolated endgame fixture ready; materials provisioned, normal input required."));
    }));
static FAutoConsoleCommandWithWorldAndArgs FM3Tier(TEXT("Hearthward.Test108.Tier"),TEXT("Set isolated visual tier 1, 4 or 7; not a paid upgrade."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* World)
    {
        FString Pool;FGuid Id;if(!World || Args.Num()!=1 || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id))return;
        const int32 Tier=FCString::Atoi(*Args[0]);if(Tier==1 || Tier==4 || Tier==7)World->GetSubsystem<UHearthwardCampSubsystem>()->State.Tier=Tier;
    }));
static FAutoConsoleCommandWithWorld FM3Inspect(TEXT("Hearthward.Test108.Inspect"),TEXT("Snapshot isolated M3 camp and shared storage."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        FString Pool;FGuid Id;if(!World || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id))return;
        FString Report=TEXT("Isolated M3 fixture; not full route evidence.\n")+World->GetSubsystem<UHearthwardCampSubsystem>()->State.Snapshot();
        auto* Store=World->GetSubsystem<UHearthwardStorageSubsystem>();Report+=FString::Printf(TEXT("\nwood=%d rope=%d"),Store->GetItemCount(TEXT("wood")),Store->GetItemCount(TEXT("rope")));
        FFileHelper::SaveStringToFile(Report,*FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Task108/camp.txt")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }));
#endif
