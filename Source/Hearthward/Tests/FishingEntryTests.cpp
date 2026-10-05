#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Interaction/HearthwardInteractionComponent.h"
#include "../Nature/HearthwardNatureActor.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
template<typename T> T* AddFishingTestComponent(AActor* Actor)
{
    auto* Component=NewObject<T>(Actor);
    Actor->AddInstanceComponent(Component);Component->RegisterComponent();return Component;
}

// PROTOTYPE_ONLY: real player lookup, inventory instances and the public nature action entry.
struct FFishingEntryWorld
{
    UWorld* World;
    APawn* Player;
    UHearthwardInventoryComponent* Bag;
    UHearthwardNatureSubsystem* Nature;
    FGuid Rod,Point,Epoch;

    FFishingEntryWorld()
    {
        World=UWorld::CreateWorld(EWorldType::Game,false);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        World->InitializeActorsForPlay(FURL());
        Player=World->SpawnActor<APawn>();
        auto* Root=NewObject<USceneComponent>(Player);
        Player->AddInstanceComponent(Root);Player->SetRootComponent(Root);Root->RegisterComponent();
        Bag=AddFishingTestComponent<UHearthwardInventoryComponent>(Player);
        auto* Gameplay=AddFishingTestComponent<UHearthwardGameplayComponent>(Player);
        Gameplay->Enabled=true;Gameplay->Health=100;
        AddFishingTestComponent<UHearthwardSurvivalComponent>(Player);
        World->SpawnActor<APlayerController>()->Possess(Player);
        Nature=World->GetSubsystem<UHearthwardNatureSubsystem>();
        Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
        Bag->TryAdd(TEXT("fishing_rod"),1);Bag->TryAdd(TEXT("bait"),2);
        Rod=Bag->FirstInstance(TEXT("fishing_rod"));
        FHearthwardNaturePoint Fish;Fish.Id=FGuid::NewGuid();Fish.Key=TEXT("task061_entry");
        Fish.Kind=TEXT("fish");Fish.Definition=TEXT("carp");Fish.Position=FVector(200,0,0);Fish.Remaining=24;
        Point=Fish.Id;Nature->State.Points.Add(Fish);
    }
    ~FFishingEntryWorld()
    {
        Nature->Cancel();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    bool Start(){return Nature->Act(TEXT("fish"),Point,NAME_None,Epoch);}
    int32 FishCount() const
    {
        int32 Count=0;for(const auto& Value:HearthwardNature::Rows(TEXT("fish")))
            Count+=Bag->GetItemCount(FName(*HearthwardData::Text(Value->AsObject(),TEXT("item"))));
        return Count;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingEquippedRodTest,"Hearthward.Fishing061.RequiresEquippedRod",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingEquippedRodTest::RunTest(const FString&)
{
    FFishingEntryWorld Fixture;
    if(!TestTrue(TEXT("Fixture pawn is the real player lookup"),UGameplayStatics::GetPlayerPawn(Fixture.World,0)==Fixture.Player))return false;
    if(!TestTrue(TEXT("Actual durable rod exists in the bag"),Fixture.Rod.IsValid() && Fixture.Bag->FindInstance(Fixture.Rod)->Durability>0))return false;
    TestFalse(TEXT("Bag rod is not equipped in the tool slot"),Fixture.Bag->EquippedInstance(TEXT("tool")).IsValid());
    TestFalse(TEXT("A rod carried without equipping cannot start fishing"),Fixture.Start());
    TestFalse(TEXT("Rejected equipment entry does not reserve the point"),Fixture.Nature->IsFishing());
    Fixture.Nature->Tick(1.05f);
    TestEqual(TEXT("Rejected equipment entry cannot consume bait at the cast boundary"),Fixture.Bag->GetItemCount(TEXT("bait")),2);

    Fixture.Nature->Cancel();
    if(!TestTrue(TEXT("Equip the actual rod instance into the tool slot"),Fixture.Bag->EquipInstance(Fixture.Rod)))return false;
    TestTrue(TEXT("The same valid rod works after equipping"),Fixture.Start());
    TestFalse(TEXT("A second start cannot reserve the same active point"),Fixture.Start());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingCastRangeTest,"Hearthward.Fishing061.FifteenMeterCastRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingCastRangeTest::RunTest(const FString&)
{
    FFishingEntryWorld Fixture;Fixture.Bag->EquipInstance(Fixture.Rod);
    Fixture.Nature->State.Points[0].Position=FVector(1200,0,0);
    TestTrue(TEXT("Equipped rod can cast to a stocked point twelve meters away"),Fixture.Start());
    Fixture.Nature->Tick(1.05f);
    TestTrue(TEXT("The same legal twelve-meter distance remains valid through casting"),Fixture.Nature->IsFishing());
    TestEqual(TEXT("Actual cast boundary consumes exactly one bait"),Fixture.Bag->GetItemCount(TEXT("bait")),1);
    TestEqual(TEXT("Casting alone does not reduce fish stock"),Fixture.Nature->State.Points[0].Remaining,24);
    TestEqual(TEXT("Casting alone does not wear the equipped rod"),Fixture.Bag->FindInstance(Fixture.Rod)->Durability,40.);
    Fixture.Nature->Cancel();
    Fixture.Nature->State.Points[0].Position=FVector(1500,0,0);
    TestTrue(TEXT("Exactly fifteen meters is included"),Fixture.Start());
    Fixture.Nature->Cancel();Fixture.Nature->State.Points[0].Position=FVector(1501,0,0);
    TestFalse(TEXT("More than fifteen meters is rejected"),Fixture.Start());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingFullBagTest,"Hearthward.Fishing061.FullBagRejectsBeforeBaitDebit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingFullBagTest::RunTest(const FString&)
{
    FFishingEntryWorld Fixture;Fixture.Bag->EquipInstance(Fixture.Rod);
    if(!TestTrue(TEXT("Real materials fill the bag legally"),Fixture.Bag->TryAdd(TEXT("stone"),98)==EHearthwardInventoryResult::Success
        && Fixture.Bag->TryAdd(TEXT("herb"),2)==EHearthwardInventoryResult::Success))return false;
    TestEqual(TEXT("Fixture reaches actual rank-one capacity"),Fixture.Bag->GetWeight(),Fixture.Bag->GetCapacity());
    FHearthwardInventoryState AfterCast;
    if(!TestTrue(TEXT("Full bag remains a valid inventory snapshot"),AfterCast.Restore(Fixture.Bag->Snapshot())))return false;
    AfterCast.Remove(TEXT("bait"),1);
    for(const auto& Value:HearthwardNature::Rows(TEXT("fish")))
    {
        auto Candidate=AfterCast;const FName Item(*HearthwardData::Text(Value->AsObject(),TEXT("item")));
        TestTrue(TEXT("Even spending one bait cannot fit any real fish in this fixture"),Candidate.Add(Item,1)==EHearthwardInventoryResult::CapacityExceeded);
    }
    TestFalse(TEXT("Known-full bag is rejected before starting fishing"),Fixture.Start());
    TestFalse(TEXT("Known-full bag does not hold a fishing reservation"),Fixture.Nature->IsFishing());
    Fixture.Nature->Tick(1.05f);
    TestEqual(TEXT("Rejected capacity entry preserves both bait"),Fixture.Bag->GetItemCount(TEXT("bait")),2);
    TestEqual(TEXT("Rejected capacity entry preserves the exact rod durability"),Fixture.Bag->FindInstance(Fixture.Rod)->Durability,40.);
    TestEqual(TEXT("Rejected capacity entry preserves fish stock"),Fixture.Nature->State.Points[0].Remaining,24);
    TestEqual(TEXT("Rejected capacity entry does not advance reward ordinal"),Fixture.Nature->State.Points[0].Successes,0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingRodSwitchTest,"Hearthward.Fishing061.RodInstanceSwitchCancels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingRodSwitchTest::RunTest(const FString&)
{
    FFishingEntryWorld Fixture;Fixture.Bag->EquipInstance(Fixture.Rod);
    Fixture.Bag->TryAdd(TEXT("fishing_rod"),1);
    const FGuid OtherRod=Fixture.Bag->Snapshot().Instances.Last().Id;
    if(!TestTrue(TEXT("Same-definition alternate rod is a different real instance"),OtherRod.IsValid() && OtherRod!=Fixture.Rod))return false;
    if(!TestTrue(TEXT("Equipped first rod starts fishing"),Fixture.Start()))return false;
    Fixture.Nature->Tick(1.05f);
    TestEqual(TEXT("Cast consumes one bait before equipment changes"),Fixture.Bag->GetItemCount(TEXT("bait")),1);
    Fixture.Bag->EquipInstance(OtherRod);Fixture.Nature->Tick(.05f);
    TestFalse(TEXT("Changing even to another valid rod cancels the active cast"),Fixture.Nature->IsFishing());
    TestEqual(TEXT("Equipment cancellation retains the spent bait"),Fixture.Bag->GetItemCount(TEXT("bait")),1);
    TestEqual(TEXT("Equipment cancellation produces no fish"),Fixture.FishCount(),0);
    TestEqual(TEXT("Original rod keeps exact durability"),Fixture.Bag->FindInstance(Fixture.Rod)->Durability,40.);
    TestEqual(TEXT("Replacement rod keeps exact durability"),Fixture.Bag->FindInstance(OtherRod)->Durability,40.);
    TestEqual(TEXT("Equipment cancellation preserves fish stock"),Fixture.Nature->State.Points[0].Remaining,24);
    TestEqual(TEXT("Equipment cancellation preserves the reward ordinal"),Fixture.Nature->State.Points[0].Successes,0);
    TestTrue(TEXT("Equipment cancellation cannot create a pending reward"),Fixture.Nature->State.Points[0].Pending.Stacks.IsEmpty()
        && Fixture.Nature->State.Points[0].Pending.Instances.IsEmpty() && Fixture.Nature->State.Rewards.IsEmpty());

    if(!TestTrue(TEXT("Replacement rod can start a new cast"),Fixture.Start()))return false;
    Fixture.Bag->EquipInstance(Fixture.Rod);Fixture.Nature->Tick(1.05f);
    TestFalse(TEXT("Equipment changes before casting also cancel"),Fixture.Nature->IsFishing());
    TestEqual(TEXT("Pre-cast equipment cancellation does not consume another bait"),Fixture.Bag->GetItemCount(TEXT("bait")),1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingLateCapacityTest,"Hearthward.Fishing061.CapacityCheckedAgainAtCatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingLateCapacityTest::RunTest(const FString&)
{
    FFishingEntryWorld Fixture;Fixture.Bag->EquipInstance(Fixture.Rod);
    if(!TestTrue(TEXT("Initially spacious bag starts fishing"),Fixture.Start()))return false;
    Fixture.Nature->Tick(1.05f);
    if(!TestTrue(TEXT("Real materials fill available space after bait was spent"),Fixture.Bag->TryAdd(TEXT("stone"),98)==EHearthwardInventoryResult::Success
        && Fixture.Bag->TryAdd(TEXT("herb"),2)==EHearthwardInventoryResult::Success))return false;
    for(int32 I=0;I<1300 && Fixture.Nature->IsFishing();++I)
    {
        Fixture.Nature->HoldLine(Fixture.Nature->FishingTension()<.45);
        Fixture.Nature->Tick(.01f);
    }
    TestFalse(TEXT("Catch attempt leaves fishing mode"),Fixture.Nature->IsFishing());
    TestTrue(TEXT("Actual terminal feedback identifies insufficient bag space"),Fixture.Nature->Feedback.Contains(TEXT("背包容量不足")));
    TestEqual(TEXT("Released fish does not enter inventory"),Fixture.FishCount(),0);
    TestEqual(TEXT("Late capacity rejection retains exactly one spent bait"),Fixture.Bag->GetItemCount(TEXT("bait")),1);
    TestEqual(TEXT("Late capacity rejection does not wear the rod"),Fixture.Bag->FindInstance(Fixture.Rod)->Durability,40.);
    TestEqual(TEXT("Late capacity rejection preserves fish stock"),Fixture.Nature->State.Points[0].Remaining,24);
    TestEqual(TEXT("Late capacity rejection preserves the reward ordinal"),Fixture.Nature->State.Points[0].Successes,0);
    TestTrue(TEXT("Released fish does not trigger a rare reward"),Fixture.Nature->State.Rewards.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingWorldInteractionTest,"Hearthward.Fishing061.WorldInteractionStartsCast",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingWorldInteractionTest::RunTest(const FString&)
{
    FFishingEntryWorld Fixture;Fixture.Bag->EquipInstance(Fixture.Rod);
    Fixture.Nature->State.Points[0].Position=FVector(1200,0,0);Fixture.Nature->RebuildActors();
    auto* FishActor=Fixture.Nature->Actor(Fixture.Point);
    if(!TestTrue(TEXT("Production fish point actor exists"),IsValid(FishActor)))return false;
    auto* Target=FishActor->FindComponentByClass<UHearthwardNatureInteraction>();
    auto* Interaction=AddFishingTestComponent<UHearthwardInteractionComponent>(Fixture.Player);
    TestTrue(TEXT("Real nearest-target lookup permits a twelve-meter fish point"),Interaction->GetNearestTarget()==Target);
    TestTrue(TEXT("Real E interaction dispatch executes"),Interaction->InteractNearest());
    TestTrue(TEXT("E enters fishing directly at the stocked point"),Fixture.Nature->IsFishing());
    TestEqual(TEXT("E entry has not consumed bait before casting"),Fixture.Bag->GetItemCount(TEXT("bait")),2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingCompanionToolTest,"Hearthward.Fishing061.CompanionUsesEquippedToolInstance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingCompanionToolTest::RunTest(const FString&)
{
    FFishingEntryWorld Fixture;Fixture.Nature->RebuildActors();
    auto* Floor=Fixture.World->SpawnActor<AStaticMeshActor>();
    auto* FloorMesh=Floor->GetStaticMeshComponent();FloorMesh->SetMobility(EComponentMobility::Movable);
    FloorMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    FloorMesh->SetCollisionProfileName(TEXT("BlockAll"));FloorMesh->SetCollisionObjectType(ECC_WorldStatic);
    Floor->SetActorLocation(FVector(0,0,-50));Floor->SetActorScale3D(FVector(10,10,1));Floor->Tags.Add(TEXT("Hearthward.NatureGround"));
    FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Brother=Fixture.World->SpawnActor<AHearthwardCompanionFixture>(AHearthwardCompanionFixture::StaticClass(),FVector(0,0,82),FRotator::ZeroRotator,Spawn);
    Fixture.World->BeginPlay();
    Fixture.Player->FindComponentByClass<UHearthwardGameplayComponent>()->SetComponentTickEnabled(false);
    if(!Brother->GetController())Brother->SpawnDefaultController();
    for(int32 I=0;I<40;++I)
    {
        Fixture.World->Tick(LEVELTICK_All,.025f);
        if(Brother->GetCharacterMovement()->IsMovingOnGround())break;
    }
    TestTrue(TEXT("Real world physics lands the companion on the collision floor"),Brother->GetCharacterMovement()->IsMovingOnGround());
    Brother->Bag->TryAdd(TEXT("fishing_rod"),1);Brother->Bag->TryAdd(TEXT("bait"),2);
    const FGuid BrotherRod=Brother->Bag->FirstInstance(TEXT("fishing_rod"));
    FHearthwardAgentGoal Goal;Goal.Intent=TEXT("fish");Goal.Item=TEXT("fish");Goal.Quantity=1;
    Goal.QuantityMode=TEXT("one_catch");Goal.SourceRef=TEXT("known_target");Goal.Station=Fixture.Point;
    TestEqual(TEXT("Companion preview requires an equipped rod"),Brother->PreviewGoal(Goal),FString(TEXT("FISHING_ROD_REQUIRED")));
    FName Caught;
    TestFalse(TEXT("Companion catch entry also rejects a bag-only rod"),Fixture.Nature->CatchCompanion(Brother,Fixture.Point,Fixture.Epoch,Caught));
    TestEqual(TEXT("Rejected companion entry preserves his bait"),Brother->Bag->GetItemCount(TEXT("bait")),2);
    Brother->Bag->EquipInstance(BrotherRod);
    TestTrue(TEXT("Companion preview accepts the actual equipped rod"),Brother->PreviewGoal(Goal).IsEmpty());
    auto* Survival=Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    const auto* Movement=Brother->GetCharacterMovement();
    const FString Preconditions=FString::Printf(TEXT("position=%s distance=%.2fcm movement=%d falling=%d swimming=%d alive=%d hp=%.2f safeToSave=%d"),
        *Brother->GetActorLocation().ToString(),FVector::Dist(Brother->GetActorLocation(),Fixture.Nature->State.Points[0].Position),
        int32(Movement->MovementMode),Movement->IsFalling(),Movement->IsSwimming(),Survival->Alive(),Survival->Health(),Survival->SafeToSave());
    AddInfo(Preconditions);
    TestTrue(FString(TEXT("Actual companion catch has safe standing preconditions: "))+Preconditions,Survival->SafeToSave());
    const bool CaughtSuccessfully=Fixture.Nature->CatchCompanion(Brother,Fixture.Point,Fixture.Epoch,Caught);
    TestTrue(FString(TEXT("Actual companion catch settles after equipping: "))+Preconditions+TEXT(" feedback=")+Fixture.Nature->Feedback,CaughtSuccessfully);
    TestTrue(TEXT("Companion receives his real caught fish"),!Caught.IsNone() && Brother->Bag->GetItemCount(Caught)==1);
    TestEqual(TEXT("Companion spends his own bait once"),Brother->Bag->GetItemCount(TEXT("bait")),1);
    TestEqual(TEXT("Companion success wears his exact equipped rod"),Brother->Bag->FindInstance(BrotherRod)->Durability,39.);
    TestEqual(TEXT("Companion catch does not consume player bait"),Fixture.Bag->GetItemCount(TEXT("bait")),2);
    TestEqual(TEXT("Companion catch does not wear the player's carried rod"),Fixture.Bag->FindInstance(Fixture.Rod)->Durability,40.);
    TestEqual(TEXT("Companion success reduces the same fish point once"),Fixture.Nature->State.Points[0].Remaining,23);
    TestEqual(TEXT("Companion success advances the same reward ordinal once"),Fixture.Nature->State.Points[0].Successes,1);
    return true;
}
#endif
