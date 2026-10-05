#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FScopedCampaignEndStateWorld
{
    UWorld* World=nullptr;
    ACharacter* Player=nullptr;
    UHearthwardGameplayComponent* Gameplay=nullptr;
    UHearthwardSurvivalComponent* Survival=nullptr;
    UHearthwardBuildingComponent* Building=nullptr;
    UHearthwardCampaignSubsystem* Campaign=nullptr;
    UHearthwardCampSubsystem* Camp=nullptr;
    UHearthwardStorageSubsystem* Storage=nullptr;
    UHearthwardWorldClockSubsystem* Clock=nullptr;
    FVector FloorPosition=FVector::ZeroVector;
    double InitialA=0;

    template<typename T> T* Attach()
    {
        auto* Component=NewObject<T>(Player);
        Player->AddInstanceComponent(Component);Component->RegisterComponent();return Component;
    }

    explicit FScopedCampaignEndStateWorld(FName Location)
    {
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        FloorPosition=HearthwardCampaign::XY(HearthwardCampaign::Find(TEXT("locations"),Location),TEXT("xy"));
        auto* Floor=World->SpawnActor<AActor>();Floor->Tags.Add(TEXT("Hearthward.NatureGround"));
        auto* Box=NewObject<UBoxComponent>(Floor);Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(1000,1000,10));Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent();Floor->SetActorLocation(FloorPosition-FVector(0,0,10));
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Player=World->SpawnActor<ACharacter>(FloorPosition+FVector(0,0,100),FRotator::ZeroRotator,Params);
        auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
        Attach<UHearthwardInventoryComponent>();Gameplay=Attach<UHearthwardGameplayComponent>();Gameplay->Enabled=true;
        Survival=Attach<UHearthwardSurvivalComponent>();Attach<UHearthwardTimedActionComponent>();
        auto* Combat=Attach<UHearthwardCombatComponent>();Building=Attach<UHearthwardBuildingComponent>();
        World->GetWorldSettings()->NotifyBeginPlay();
        Gameplay->SetComponentTickEnabled(false);Combat->SetComponentTickEnabled(false);Building->SetComponentTickEnabled(false);
        // Only the fixture settles its physical capsule before the campaign and timed assertions start.
        for(int32 I=0;I<40 && !Player->GetCharacterMovement()->IsMovingOnGround();++I)World->Tick(LEVELTICK_All,.025f);
        Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Camp=World->GetSubsystem<UHearthwardCampSubsystem>();
        Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
        InitialA=Clock->GetSnapshot().ActivePlaySeconds;Camp->State.AddCamp(TEXT("camp"),FVector(-98000,-75000,0));
        Campaign->State.Initialize();Campaign->State.Phase=TEXT("occupied");
        // Cleared, unloaded identities isolate end-state rules from combat and unrelated streamed scenery.
        for(auto& Enemy:Campaign->State.Enemies)
        {Enemy.Combat.Health=0;Enemy.Combat.Position=FVector(1000000,1000000,80);Enemy.Located=true;}
        for(auto& Person:Campaign->State.People)
        {Person.Position=FVector(1000000,1000000,80);Person.Located=true;}
        Campaign->State.ResolveUntriggered();Campaign->State.Positions.Add(Location,FloorPosition+FVector(0,0,80));
    }

    ~FScopedCampaignEndStateWorld(){GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}

    bool Ready(FAutomationTestBase& Test)
    {
        FVector Ground;
        const bool Resolved=Test.TestTrue(TEXT("GameplayStatics resolves the registered real player"),UGameplayStatics::GetPlayerPawn(World,0)==Player);
        const bool Landed=Test.TestTrue(TEXT("The actual capsule settles on the fixture's collision floor"),Player->GetCharacterMovement()->IsMovingOnGround());
        const bool Supported=Test.TestTrue(TEXT("Production campaign Ground resolves the tagged support"),Campaign->Ground(FloorPosition,Ground) && FMath::Abs(Ground.Z)<1);
        const bool Valid=Test.TestTrue(TEXT("All eighty cleared stable identities and terminal reinforcement states validate"),Campaign->State.Validate());
        const bool Alive=Test.TestTrue(TEXT("Production Survival is enabled and alive before the action"),Survival->Enabled() && Survival->Alive());
        Test.AddInfo(FString::Printf(TEXT("Campaign067 fixture: pos=%s walking=%d ground=%s paused=%d A=%.6f N=%d K=%d"),
            *Player->GetActorLocation().ToString(),Player->GetCharacterMovement()->IsMovingOnGround(),*Ground.ToString(),World->IsPaused(),
            Clock->GetSnapshot().ActivePlaySeconds,Campaign->State.Total(),Campaign->State.Cleared()));
        return Resolved && Landed && Supported && Valid && Alive;
    }

    void Step(float Delta)
    {
        Clock->Tick(Delta);
        if(Campaign->IsTickable())Campaign->Tick(Delta);
    }

    void ReadyForVictory()
    {
        for(const auto& Zone:HearthwardCampaign::Rows(TEXT("zones")))
            Campaign->State.Flags.Add(FName(*Zone->AsObject()->GetStringField(TEXT("id"))));
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignFlagActiveClock067Test,"Hearthward.Campaign067.FlagUsesActualActiveSeconds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignFlagActiveClock067Test::RunTest(const FString&)
{
    for(const float Delta:{1.f,.1f})
    {
        FScopedCampaignEndStateWorld Fixture(TEXT("loc_river_gate"));if(!Fixture.Ready(*this))return false;
        TestTrue(TEXT("The real control zone has no surviving owned or generated enemies"),Fixture.Campaign->State.ZoneClear(TEXT("river_gate")));
        if(!TestTrue(TEXT("The actual nearest-flag interaction begins the production action"),Fixture.Campaign->Interact())
            || !TestTrue(TEXT("Interaction creates a pending flag action"),Fixture.Campaign->Busy()))return false;
        const int32 PerSecond=FMath::RoundToInt(1.f/Delta);
        for(int32 I=0;I<4*PerSecond;++I)Fixture.Step(Delta);
        TestTrue(TEXT("Four active seconds came from the production world clock"),FMath::Abs(Fixture.Clock->GetSnapshot().ActivePlaySeconds-(Fixture.InitialA+4))<1.e-5);
        TestFalse(TEXT("Four active seconds cannot complete a five-second flag"),Fixture.Campaign->State.Flags.Contains(TEXT("river_gate")));
        for(int32 I=0;I<PerSecond;++I)Fixture.Step(Delta);
        TestTrue(TEXT("Five active seconds came from the same production clock"),FMath::Abs(Fixture.Clock->GetSnapshot().ActivePlaySeconds-(Fixture.InitialA+5))<1.e-5);
        TestTrue(FString::Printf(TEXT("Five actual active seconds complete the flag with %.1f-second updates"),Delta),Fixture.Campaign->State.Flags.Contains(TEXT("river_gate")));
        TestFalse(TEXT("Successful flag completion ends the action"),Fixture.Campaign->Busy());
    }
    FScopedCampaignEndStateWorld Moved(TEXT("loc_river_gate"));if(!Moved.Ready(*this))return false;
    if(!TestTrue(TEXT("A fresh real flag interaction begins"),Moved.Campaign->Interact()))return false;
    Moved.Step(.05f);const FVector Origin=Moved.Player->GetActorLocation();
    Moved.Player->GetCharacterMovement()->MoveUpdatedComponent(FVector(60,0,0),Moved.Player->GetActorQuat(),true);
    if(!TestTrue(TEXT("The swept real capsule moves beyond the forty-centimeter interruption limit"),FVector::Dist2D(Origin,Moved.Player->GetActorLocation())>40))return false;
    Moved.Step(.05f);
    TestFalse(TEXT("Movement interrupts before the next scenery-refresh interval"),Moved.Campaign->Busy());
    Moved.Player->GetCharacterMovement()->MoveUpdatedComponent(Origin-Moved.Player->GetActorLocation(),Moved.Player->GetActorQuat(),true);
    for(int32 I=0;I<6;++I)Moved.Step(1.f);
    TestFalse(TEXT("Returning to the start cannot resurrect the interrupted flag action"),Moved.Campaign->State.Flags.Contains(TEXT("river_gate")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignVictoryActivation067Test,"Hearthward.Campaign067.VictoryActivatesActualSecondCamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignVictoryActivation067Test::RunTest(const FString&)
{
    FScopedCampaignEndStateWorld Fixture(TEXT("hometown"));if(!Fixture.Ready(*this))return false;
    Fixture.ReadyForVictory();if(!TestTrue(TEXT("Actual identities, four flags and reinforcement terminal states qualify"),Fixture.Campaign->State.ReadyForVictory()))return false;
    TestFalse(TEXT("The station was not manually activated before victory"),Fixture.Gameplay->Activated.Contains(TEXT("hometown")));
    Fixture.Step(.1f);
    if(!TestTrue(TEXT("Production Tick commits permanent victory on the real support"),Fixture.Campaign->State.Victory)
        || !TestTrue(TEXT("The same transaction opens the actual second camp"),Fixture.Camp->State.Hometown))return false;
    TestEqual(TEXT("Both camp sites share the existing state"),Fixture.Camp->State.Camps.Num(),2);
    TestEqual(TEXT("The real building component receives all four zero-material gifts"),Fixture.Building->GetBuildings().Num(),4);
    TestEqual(TEXT("The unique blade is credited by the actual shared-storage transaction"),Fixture.Storage->GetItemCount(TEXT("hearth_blade")),1);
    TestTrue(TEXT("The unique reward ledger is committed"),Fixture.Gameplay->RewardFacts.Contains(TEXT("reward:hometown")));
    TestTrue(TEXT("Victory automatically activates the hometown travel station"),Fixture.Gameplay->Activated.Contains(TEXT("hometown")));
    Fixture.Step(.5f);
    TestEqual(TEXT("Another world update does not duplicate the camp"),Fixture.Camp->State.Camps.Num(),2);
    TestEqual(TEXT("Another update does not duplicate the gift facilities"),Fixture.Building->GetBuildings().Num(),4);
    TestEqual(TEXT("Another update does not duplicate the unique blade"),Fixture.Storage->GetItemCount(TEXT("hearth_blade")),1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignFailedVictory067Test,"Hearthward.Campaign067.FailedSurvivalRejectsAutomaticVictory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignFailedVictory067Test::RunTest(const FString&)
{
    FScopedCampaignEndStateWorld Fixture(TEXT("hometown"));if(!Fixture.Ready(*this))return false;
    Fixture.ReadyForVictory();if(!TestTrue(TEXT("The otherwise legitimate victory transaction is ready"),Fixture.Campaign->State.ReadyForVictory()))return false;
    if(!TestTrue(TEXT("Production fatal damage commits the real player's failure"),Fixture.Survival->ReceiveDamage(Fixture.Survival->MaxHealth(),FGuid::NewGuid(),Fixture.Storage->GetTimelineEpoch(),true)))return false;
    TestTrue(TEXT("The production global failure predicate is true"),UHearthwardSurvivalComponent::HasFailed(Fixture.World));
    TestFalse(TEXT("This is the domain interval before failure UI applies pause; no pause was lifted"),Fixture.World->IsPaused());
    Fixture.Step(.5f);
    TestEqual(TEXT("Production failure freezes the actual active clock"),Fixture.Clock->GetSnapshot().ActivePlaySeconds,Fixture.InitialA);
    TestFalse(TEXT("Automatic world ticking cannot commit victory after real failure"),Fixture.Campaign->State.Victory);
    TestFalse(TEXT("Failure cannot open the second-camp ledger"),Fixture.Camp->State.Hometown);
    TestEqual(TEXT("Failure retains just the original camp"),Fixture.Camp->State.Camps.Num(),1);
    TestEqual(TEXT("Failure creates no gift building instances"),Fixture.Building->GetBuildings().Num(),0);
    TestEqual(TEXT("Failure grants no unique blade to shared storage"),Fixture.Storage->GetItemCount(TEXT("hearth_blade")),0);
    TestFalse(TEXT("Failure cannot publish the unique reward receipt"),Fixture.Gameplay->RewardFacts.Contains(TEXT("reward:hometown")));
    TestFalse(TEXT("Failure cannot activate a travel station"),Fixture.Gameplay->Activated.Contains(TEXT("hometown")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignWeightedPrompt067Test,"Hearthward.Campaign067.SecondStage.PromptUsesWeightedControlProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignWeightedPrompt067Test::RunTest(const FString&)
{
    FScopedCampaignEndStateWorld Fixture(TEXT("loc_river_gate"));if(!Fixture.Ready(*this))return false;
    int32 Cleared=0;
    for(auto& Enemy:Fixture.Campaign->State.Enemies)if(Enemy.Group==TEXT("base"))
    {
        Enemy.Combat.Health=HearthwardCampaign::Health(Enemy.Kind,Enemy.Stage);
        if(Enemy.Zone==TEXT("river_gate")){Enemy.Combat.Health=0;++Cleared;}
    }
    for(auto& Enemy:Fixture.Campaign->State.Enemies)
        if(Enemy.Group==TEXT("base") && Enemy.Combat.Health>0 && Cleared<40){Enemy.Combat.Health=0;++Cleared;}
    for(auto& Trigger:Fixture.Campaign->State.Reinforcements)Trigger.Value=TEXT("pending");
    Fixture.Campaign->State.ResolveUntriggered();Fixture.Campaign->State.Flags.Add(TEXT("river_gate"));
    if(!TestTrue(TEXT("The forty-cleared, one-flag campaign fixture is a valid persistent state"),Fixture.Campaign->State.Validate()))return false;
    TestEqual(TEXT("Actual denominator remains eighty stable identities"),Fixture.Campaign->State.Total(),80);
    TestEqual(TEXT("Actual cleared identities total forty"),Fixture.Campaign->State.Cleared(),40);
    TestEqual(TEXT("Actual controlled flags total one"),Fixture.Campaign->State.Flags.Num(),1);
    const FString Prompt=Fixture.Campaign->Prompt();AddInfo(TEXT("Campaign067 actual weighted prompt: ")+Prompt);
    TestTrue(TEXT("The actual player-facing prompt displays 70 percent of 40/80 plus 30 percent of 1/4 as 42.5 percent"),Prompt.Contains(TEXT("42.5%")));
    TestTrue(TEXT("The same actual prompt retains its one-of-four flag count"),Prompt.Contains(TEXT("1/4")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignOccupiedGift067Test,"Hearthward.Campaign067.SecondStage.OccupiedGiftsStayOnLegalGroundOrRollback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignOccupiedGift067Test::RunTest(const FString&)
{
    FScopedCampaignEndStateWorld Fixture(TEXT("hometown"));if(!Fixture.Ready(*this))return false;
    auto* Bag=Fixture.Player->FindComponentByClass<UHearthwardInventoryComponent>();
    auto* Timer=Fixture.Player->FindComponentByClass<UHearthwardTimedActionComponent>();
    if(!TestTrue(TEXT("Campfire is genuinely permitted outside the initial camp"),HearthwardData::Find(TEXT("buildings"),TEXT("campfire"))->GetBoolField(TEXT("wilderness"))))return false;
    TestTrue(TEXT("The future hometown is outside the current camp's building boundary"),Fixture.Camp->State.CampAt(Fixture.FloorPosition).IsNone());
    if(!TestTrue(TEXT("Fixture supplies the real four wood for normal construction"),Bag->TryAdd(TEXT("wood"),4)==EHearthwardInventoryResult::Success)
        || !TestTrue(TEXT("Fixture supplies the real four stone for normal construction"),Bag->TryAdd(TEXT("stone"),4)==EHearthwardInventoryResult::Success))return false;
    Fixture.Player->GetCharacterMovement()->MoveUpdatedComponent(FVector(60,0,0),Fixture.Player->GetActorQuat(),true);
    if(!TestTrue(TEXT("The player selects a campfire through the normal building entry"),Fixture.Building->SelectBuilding(TEXT("campfire"))))return false;
    Fixture.Building->TickComponent(.01f,LEVELTICK_All,nullptr);
    const FVector OriginalPlacement=Fixture.Building->Placement;
    if(!TestTrue(TEXT("The normal preview projects onto the currently occupied gift candidate"),FVector::Dist2D(OriginalPlacement,Fixture.FloorPosition+FVector(300,0,0))<1)
        || !TestTrue(TEXT("Normal support, occupancy, distance and material checks accept the campfire"),Fixture.Building->ConfirmPlacement()))return false;
    const double Started=Fixture.Clock->GetSnapshot().ActivePlaySeconds;
    Fixture.Clock->Tick(5.f);Timer->TickComponent(5.f,LEVELTICK_All,nullptr);
    if(!TestTrue(TEXT("Five actual active seconds finish the normal construction timer"),FMath::Abs(Fixture.Clock->GetSnapshot().ActivePlaySeconds-Started-5)<1.e-5)
        || !TestEqual(TEXT("The production completion creates one actual building"),Fixture.Building->BuildingCount(),1)
        || !TestEqual(TEXT("The same completion publishes one facility ledger"),Fixture.Camp->State.Facilities.Num(),1))return false;
    TestEqual(TEXT("Normal construction spends its four wood"),Bag->GetItemCount(TEXT("wood")),0);
    TestEqual(TEXT("Normal construction spends its four stone"),Bag->GetItemCount(TEXT("stone")),0);
    const FGuid ExistingId=Fixture.Camp->State.Facilities[0].Id;
    auto* Existing=Fixture.Building->ResolveFacility(ExistingId);
    if(!TestNotNull(TEXT("The constructed campfire is a real registered actor"),Existing))return false;
    auto* ExistingBox=Cast<UBoxComponent>(Existing->GetRootComponent());
    if(!TestNotNull(TEXT("The existing campfire has its production collision box"),ExistingBox))return false;
    const FBox ExistingBounds=ExistingBox->Bounds.GetBox();
    const auto BeforeBuildings=Fixture.Building->Snapshot();
    Fixture.ReadyForVictory();
    if(!TestTrue(TEXT("The actual victory prerequisites are ready after normal construction"),Fixture.Campaign->State.ReadyForVictory())
        || !TestTrue(TEXT("The production campaign domain can tick after completed construction"),Fixture.Campaign->IsTickable() && !Fixture.Clock->Suspended()))return false;
    TestFalse(TEXT("The actual building action and preview ended before victory settlement"),Fixture.Building->IsBuilding() || Fixture.Building->IsPlacing());
    Fixture.Step(.5f);
    AddInfo(FString::Printf(TEXT("Campaign067 occupied gift: victory=%d hometown=%d facilities=%d buildings=%d feedback=%s"),
        Fixture.Campaign->State.Victory,Fixture.Camp->State.Hometown,Fixture.Camp->State.Facilities.Num(),Fixture.Building->BuildingCount(),*Fixture.Camp->Feedback));
    if(!Fixture.Campaign->State.Victory)
    {
        TestFalse(TEXT("A rejected layout rolls back second-camp ownership"),Fixture.Camp->State.Hometown);
        TestEqual(TEXT("A rejected layout retains only the original camp"),Fixture.Camp->State.Camps.Num(),1);
        TestEqual(TEXT("A rejected layout retains exactly the paid existing facility"),Fixture.Camp->State.Facilities.Num(),1);
        const auto AfterBuildings=Fixture.Building->Snapshot();
        if(TestEqual(TEXT("A rejected layout rolls back all gift actor records"),AfterBuildings.Num(),1))
        {
            TestEqual(TEXT("Rollback preserves the existing building's stable identity"),AfterBuildings[0]->AsObject()->GetStringField(TEXT("id")),BeforeBuildings[0]->AsObject()->GetStringField(TEXT("id")));
            TestEqual(TEXT("Rollback preserves the existing building's paid location"),AfterBuildings[0]->AsObject()->GetStringField(TEXT("position")),BeforeBuildings[0]->AsObject()->GetStringField(TEXT("position")));
        }
        const auto* Paid=Fixture.Camp->State.Facilities.FindByPredicate([&](const auto& F){return F.Id==ExistingId;});
        if(TestNotNull(TEXT("Rollback retains the existing facility ledger"),Paid))
        {
            TestEqual(TEXT("Rollback preserves the actual paid wood"),Paid->Paid.FindRef(TEXT("wood")),4);
            TestEqual(TEXT("Rollback preserves the actual paid stone"),Paid->Paid.FindRef(TEXT("stone")),4);
        }
        TestEqual(TEXT("A rejected layout grants no unique blade"),Fixture.Storage->GetItemCount(TEXT("hearth_blade")),0);
        TestFalse(TEXT("A rejected layout publishes no unique reward receipt"),Fixture.Gameplay->RewardFacts.Contains(TEXT("reward:hometown")));
        TestFalse(TEXT("A rejected layout activates no hometown station"),Fixture.Gameplay->Activated.Contains(TEXT("hometown")));
        return true;
    }
    TestTrue(TEXT("A legal alternative layout publishes second-camp ownership"),Fixture.Camp->State.Hometown);
    TestEqual(TEXT("A legal layout adds all four gifts to the existing paid building"),Fixture.Building->BuildingCount(),5);
    int32 Gifts=0;
    for(const auto& Facility:Fixture.Camp->State.Facilities)if(Facility.Id!=ExistingId)
    {
        ++Gifts;auto* Actor=Fixture.Building->ResolveFacility(Facility.Id);
        if(!TestNotNull(TEXT("Each committed gift has an actual building actor"),Actor))continue;
        auto* Box=Cast<UBoxComponent>(Actor->GetRootComponent());
        if(!TestNotNull(TEXT("Each committed gift has its production collision box"),Box))continue;
        FVector Ground;const bool Supported=Fixture.Campaign->Ground(Actor->GetActorLocation(),Ground);
        const double BaseZ=Actor->GetActorLocation().Z-Box->GetScaledBoxExtent().Z;
        const double Tolerance=HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("construction")),TEXT("supportTolerance"));
        TestTrue(FString::Printf(TEXT("Committed gift %s stands on actual terrain rather than the existing campfire: base=%.2f ground=%.2f"),*Facility.Kind.ToString(),BaseZ,Ground.Z),Supported && FMath::Abs(BaseZ-Ground.Z)<=Tolerance);
        const FBox Overlap=ExistingBounds.Overlap(Box->Bounds.GetBox());const FVector Size=Overlap.GetSize();
        TestFalse(TEXT("A committed gift does not penetrate the actual existing campfire collider"),Overlap.IsValid && Size.X>1 && Size.Y>1 && Size.Z>1);
        TestEqual(TEXT("Each legal gift belongs to the new local camp"),Facility.Camp,FName(TEXT("hometown")));
        TestTrue(TEXT("Each victory gift records zero paid construction material"),Facility.Paid.IsEmpty());
    }
    TestEqual(TEXT("A successful transaction creates all four free gift ledgers"),Gifts,4);
    TestEqual(TEXT("A successful legal layout credits its unique blade once"),Fixture.Storage->GetItemCount(TEXT("hearth_blade")),1);
    return true;
}
#endif
