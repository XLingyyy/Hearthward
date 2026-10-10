#include "../Experience/HearthwardPresentationComponent.h"
#include "../Experience/HearthwardPlayerSettings.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../AI/HearthwardAgentContract.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Building/HearthwardWorkshopService.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Interaction/HearthwardHarvestSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Experience/HearthwardTraversalComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Audio.h"
#include "Sound/SoundWaveProcedural.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if WITH_DEV_AUTOMATION_TESTS
struct FAudioFeedbackLifecycleAccess
{
    static int32 Events(const UHearthwardPresentationComponent* P){return P->PlayedEvents.Num();}
    static bool Observed(const UHearthwardPresentationComponent* P,FGuid Id){return P->ObservedNPCEvents.Contains(Id);}
    static bool Played(const UHearthwardPresentationComponent* P,FGuid Id){return P->PlayedEvents.Contains(Id);}
    static int32 Count(const UHearthwardPresentationComponent* P,FName Key){return P->ObservedPlayerEvents.FindRef(Key);}
    static bool HasKey(const UHearthwardPresentationComponent* P,FName Key){return P->ObservedPlayerEvents.Contains(Key);}
    static int32 MovementCount(const UHearthwardPresentationComponent* P,FName Key){return P->ObservedMovementEvents.FindRef(Key);}
    static int32 CombatCount(const UHearthwardPresentationComponent* P){return P->ObservedCombatEvents.Num();}
    static bool CombatObserved(const UHearthwardPresentationComponent* P,FGuid Id){return P->ObservedCombatEvents.Contains(Id);}
    static FVector CombatPosition(const UHearthwardPresentationComponent* P,FGuid Id){return P->ObservedCombatEvents.FindRef(Id);}
    static UAudioComponent* LatestSource(const UHearthwardPresentationComponent* P){return P->Effects.IsEmpty()?nullptr:P->Effects.Last().Get();}
    static bool CueBound(const UHearthwardPresentationComponent* P,FName Event){return P->SoundCues.Contains(Event);}
    static FString CueFile(const UHearthwardPresentationComponent* P,FName Event)
    {const auto* Cue=P->SoundCues.Find(Event);return Cue?Cue->File:FString();}
    static FName CueChannel(const UHearthwardPresentationComponent* P,FName Event)
    {const auto* Cue=P->SoundCues.Find(Event);return Cue?Cue->Channel:NAME_None;}
    static float LongestEffectRemaining(const UHearthwardPresentationComponent* P)
    {float Result=0;for(const float Remaining:P->EffectRemaining)Result=FMath::Max(Result,Remaining);return Result;}
    static void KeepStorageCueOnly(UHearthwardPresentationComponent* P)
    {for(auto It=P->SoundCues.CreateIterator();It;++It)if(It.Key()!=TEXT("storage.transfer"))It.RemoveCurrent();}
    static int32 Sources(AActor* Actor)
    {
        TArray<UAudioComponent*> Components;Actor->GetComponents(Components);
        return Components.FilterByPredicate([](const auto* C){return IsValid(C) && C->ComponentTags.Contains(TEXT("Hearthward.Audio.effects"));}).Num();
    }
};
namespace
{
struct FAudioFeedbackWorld
{
    UGameInstance* Instance;
    UWorld* World;
    ACharacter* Player;
    UHearthwardInventoryComponent* Bag;
    UHearthwardStorageSubsystem* Storage;
    UHearthwardPresentationComponent* Presentation;
    FAudioFeedbackWorld()
    {
        Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();World=Instance->GetWorld();
        Player=World->SpawnActor<ACharacter>();Player->SetActorLocation({0,0,90});
        Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        auto* G=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(G);G->RegisterComponent();
        auto* S=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(S);S->RegisterComponent();
        auto* Warehouse=World->SpawnActor<AActor>();
        auto* Access=NewObject<UHearthwardResourceInteractionComponent>(Warehouse);Warehouse->AddInstanceComponent(Access);Warehouse->SetRootComponent(Access);
        Access->RegisterComponent();Access->InitializeResource(true);
        Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();
        Presentation=NewObject<UHearthwardPresentationComponent>(Player);Player->AddInstanceComponent(Presentation);Presentation->RegisterComponent();
        Presentation->RegisterAllComponentTickFunctions(true);Presentation->BeginPlay();
        Bag->TryAdd(TEXT("wood"),8);
    }
    ~FAudioFeedbackWorld()
    {
        if(World->HasBegunPlay())World->EndPlay(EEndPlayReason::Quit);
        else if(Presentation->HasBegunPlay())Presentation->EndPlay(EEndPlayReason::Destroyed);
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    FHearthwardTransferResult Deposit(FGuid Id,int32 Count=1)
    {return Storage->Transfer(Bag,true,TEXT("wood"),Count,Id,Storage->GetTimelineEpoch());}
};
struct FAudioProducerWorld : FAudioFeedbackWorld
{
    UHearthwardGameplayComponent* Gameplay;
    UHearthwardBuildingComponent* Building;
    UHearthwardInventoryComponent* Stock;
    AHearthwardCompanionFixture* Brother;
    FGuid Station;
    bool Ready=false;
    FString Failure;
    FAudioProducerWorld(bool BoundCues=false)
    {
        if(!BoundCues)FAudioFeedbackLifecycleAccess::KeepStorageCueOnly(Presentation);
        Gameplay=Player->FindComponentByClass<UHearthwardGameplayComponent>();Gameplay->Enabled=true;Gameplay->CompanionRoutineEnabled=false;
        auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
        auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));Controller->SetPlayer(LocalPlayer);
        auto* Timer=NewObject<UHearthwardTimedActionComponent>(Player);Player->AddInstanceComponent(Timer);Timer->RegisterComponent();
        Building=NewObject<UHearthwardBuildingComponent>(Player);Player->AddInstanceComponent(Building);Building->RegisterComponent();
        auto* Floor=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);
        Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);Box->SetBoxExtent(FVector(1000,1000,10));
        Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Floor->SetActorLocation(FVector(0,0,-10));
        auto* Camps=World->GetSubsystem<UHearthwardCampSubsystem>();Camps->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
        const bool Built=Building->AddGift(TEXT("workbench"),FVector(200,0,0));
        const auto* Facility=Camps->State.Facilities.FindByPredicate([](const auto& F){return F.Kind==TEXT("workbench");});
        if(Facility)Station=Facility->Id;
        const FVector CampPosition(200,-100,80);
        auto* Camp=World->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Camp);
        Camp->AddInstanceComponent(Root);Camp->SetRootComponent(Root);Root->RegisterComponent();Camp->SetActorLocation(CampPosition);
        Stock=NewObject<UHearthwardInventoryComponent>(Camp);Camp->AddInstanceComponent(Stock);Stock->RegisterComponent();
        FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Brother=World->SpawnActor<AHearthwardCompanionFixture>(CampPosition,FRotator::ZeroRotator,Spawn);
        Brother->InitializeCompanion(Stock,Camp);
        World->InitializeActorsForPlay(FURL(),false);World->GetWorldSettings()->NotifyBeginPlay();
        World->Tick(LEVELTICK_All,.025f);
        // Synchronous world ticks share GFrameCounter; advance the actual capsule physics explicitly.
        for(int32 I=1;I<80 && (!Player->GetCharacterMovement()->IsMovingOnGround() || !Brother->GetCharacterMovement()->IsMovingOnGround());++I)
        {
            Player->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
            Brother->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
        }
        Ready=Built && Station.IsValid() && Building->CanUseWorkbench(Station) && Brother->IsAtCamp() && Brother->CanCommunicate(Player)
            && Player->IsLocallyControlled() && Player->GetCharacterMovement()->IsMovingOnGround() && Brother->GetCharacterMovement()->IsMovingOnGround();
        Failure=FString::Printf(TEXT("Player mode=%d local=%d; Brother mode=%d"),int32(Player->GetCharacterMovement()->MovementMode),Player->IsLocallyControlled(),int32(Brother->GetCharacterMovement()->MovementMode));
        if(BoundCues)
        {
            // Setup really lands the player. Settle and expire that legitimate cue before testing a new operation.
            Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
            Presentation->TickComponent(FAudioFeedbackLifecycleAccess::LongestEffectRemaining(Presentation)+.001f,LEVELTICK_All,nullptr);
        }
    }
    bool Execute(FHearthwardAgentGoal Goal,FGuid& Command)
    {
        Goal.Original=HearthwardAgent::GoalText(Goal);
        const auto Ticket=Brother->Request(Player,Goal.Original);Command=Ticket.Id;
        const auto Accepted=Brother->SubmitGoal(Player,Ticket,Goal);
        if(Accepted!=EHearthwardProposalResult::Accepted)
        {Failure=FString::Printf(TEXT("Submit=%d preview=%s"),int32(Accepted),*Brother->PreviewGoal(Goal));return false;}
        auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
        for(int32 Step=0;Step<64 && Brother->GetPhase()!=EHearthwardCompanionPhase::Completed;++Step)
        {
            Clock->Tick(.25f);
            if(Brother->Action->GetStatus()==EHearthwardTimedActionStatus::Running)Brother->Action->TickComponent(.25f,LEVELTICK_All,nullptr);
            Brother->Tick(.25f);
        }
        Failure=Brother->BlockReason;
        return Brother->GetPhase()==EHearthwardCompanionPhase::Completed;
    }
    bool UpgradeWorkbenchToLevel2()
    {
        auto* Camps=World->GetSubsystem<UHearthwardCampSubsystem>();const FGuid Epoch=Storage->GetTimelineEpoch();
        if(Camps->State.Rescued.IsEmpty()){Failure=TEXT("A real rescued person is required before upgrading the camp");return false;}
        if(!Building->AddGift(TEXT("cooking"),FVector(-300,-300,0))){Failure=TEXT("Cooking fixture registration failed");return false;}
        const int32 Food=FMath::CeilToInt(40./HearthwardCamp::FoodPoints(TEXT("wild_food")));
        if(!Storage->Adjust({},{{TEXT("wild_food"),Food}}) || !Camps->DonateFood(TEXT("wild_food"),Food,Epoch))
        {Failure=Camps->Feedback;return false;}
        const int32 Required=HearthwardCamp::RequiredTier(TEXT("workbench"),2);
        while(Camps->State.Tier<Required)
        {
            if(!Storage->Adjust({},HearthwardCamp::Counts(HearthwardCamp::Tier(Camps->State.Tier+1),TEXT("cost"))) || !Camps->UpgradeCamp(Epoch))
            {Failure=Camps->Feedback;return false;}
        }
        if(!Storage->Adjust({},HearthwardCamp::BuildCost(TEXT("workbench"),2)) || !Building->UpgradeFacility(Station,Epoch))
        {Failure=Building->Feedback;return false;}
        auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();auto* Timer=Player->FindComponentByClass<UHearthwardTimedActionComponent>();
        for(int32 I=0;I<40 && Building->IsBuilding();++I)
        {Clock->Tick(.25f);Timer->TickComponent(.25f,LEVELTICK_All,nullptr);Building->TickComponent(.25f,LEVELTICK_All,nullptr);}
        const auto* Facility=Camps->State.Facilities.FindByPredicate([&](const auto& Entry){return Entry.Id==Station;});
        Failure=Building->Feedback;return !Building->IsBuilding() && Facility && Facility->Level==2;
    }
};
struct FAudioCombatWorld : FAudioProducerWorld
{
    UHearthwardCombatComponent* Combat;
    UHearthwardSurvivalComponent* Survival;
    FAudioCombatWorld(bool BoundCues=false):FAudioProducerWorld(BoundCues)
    {
        Combat=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
        Survival=Player->FindComponentByClass<UHearthwardSurvivalComponent>();
        Ready=Ready && Bag->TryAdd(TEXT("axe"),1)==EHearthwardInventoryResult::Success && Gameplay->CommitEquipment(TEXT("axe"));
        Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    }
    UHearthwardCombatTargetComponent* Target(FName Id,FVector Position)
    {
        auto* Actor=World->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Actor);
        Actor->AddInstanceComponent(Root);Actor->SetRootComponent(Root);Root->RegisterComponent();Actor->SetActorLocation(Position);
        auto* Target=NewObject<UHearthwardCombatTargetComponent>(Actor);Actor->AddInstanceComponent(Target);Target->RegisterComponent();
        Target->Id=Id;Target->Health=Target->MaximumHealth=100;Gameplay->Opponents.Add(Id,100);return Target;
    }
};

bool ReadMovementCueDuration(const TCHAR* File,float& Duration)
{
    TArray<uint8> Data;FWaveModInfo Info;
    if(!FFileHelper::LoadFileToArray(Data,*(FPaths::ProjectDir()/TEXT("Resources/Audio")/File))
        || !Info.ReadWaveInfo(Data.GetData(),Data.Num()) || *Info.pFormatTag!=1 || *Info.pBitsPerSample!=16
        || *Info.pChannels!=1 || *Info.pSamplesPerSec==0 || Info.SampleDataSize==0)return false;
    Duration=Info.SampleDataSize/float(*Info.pSamplesPerSec*2);return true;
}
bool LaunchAndLand(FAudioProducerWorld& Fixture)
{
    auto* Movement=Fixture.Player->GetCharacterMovement();Fixture.Player->LaunchCharacter(FVector(0,0,240),false,true);bool Fell=false;
    for(int32 I=0;I<80;++I)
    {
        Movement->TickComponent(.025f,LEVELTICK_All,nullptr);Fell|=Movement->IsFalling();
        if(Fell && Movement->IsMovingOnGround())return true;
    }
    return false;
}

UHearthwardCampaignSubsystem* PrepareProgressQuest(FAudioProducerWorld& Fixture)
{
    auto* Campaign=Fixture.World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();Campaign->State.Phase=TEXT("occupied");
    // Arrange diagnostic narrative preconditions; Claim below must still commit its own rewards and identity.
    Campaign->Record(TEXT("old_carving"));Campaign->Record(TEXT("placed_carving"));
    const auto* Camps=Fixture.World->GetSubsystem<UHearthwardCampSubsystem>();
    for(auto& Person:Campaign->State.People)if(Camps->State.Rescued.Contains(Person.Id))
    {Person.Stage=TEXT("arrived");Person.Position=Fixture.Brother->Camp->GetActorLocation();Person.Located=true;}
    return Campaign;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioCommittedTransfer099Test,"Hearthward.Iteration.Task099.CommittedTransferAndIdentity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioCommittedTransfer099Test::RunTest(const FString&)
{
    FAudioFeedbackWorld F;const FGuid First=FGuid::NewGuid();
    TestEqual(TEXT("Actual first warehouse transfer commits"),F.Deposit(First).MovedCount,1);
    TestEqual(TEXT("Committed receipt enters the sound event ledger"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),1);
    TestTrue(TEXT("A PCM sound source is created for the effects channel"),FAudioFeedbackLifecycleAccess::Sources(F.Player)>0);
    TestTrue(TEXT("Same operation really replays instead of settling twice"),F.Deposit(First).Replayed);
    F.Storage->OnTransferred.Broadcast(First,true,TEXT("wood"),1);
    TestEqual(TEXT("Duplicate delivery of the same receipt cannot play again"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),1);
    TestEqual(TEXT("A separate rapid legal transfer commits"),F.Deposit(FGuid::NewGuid()).MovedCount,1);
    TestEqual(TEXT("Distinct operations are not swallowed by global throttling"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),2);
    TestEqual(TEXT("Sound observes stock and never credits its own items"),F.Storage->GetItemCount(TEXT("wood")),2);
    TestEqual(TEXT("A rejected transfer has no moved items"),F.Deposit(FGuid::NewGuid(),100).MovedCount,0);
    TestEqual(TEXT("Rejection cannot produce a success sound"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),2);
    F.Presentation->TickComponent(1.f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Procedural one-shots are destroyed after their PCM duration"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    TestEqual(TEXT("Expiry does not erase receipt identity or replay success"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioTimeline099Test,"Hearthward.Iteration.Task099.TimelineAndShutdown",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioTimeline099Test::RunTest(const FString&)
{
    FAudioFeedbackWorld F;const FGuid OldEpoch=F.Storage->GetTimelineEpoch(),OldId=FGuid::NewGuid();F.Deposit(OldId);
    TestEqual(TEXT("Pre-restore success is observed"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),1);
    F.Storage->AdvanceTimeline();F.World->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.Broadcast();
    TestEqual(TEXT("Restore clears old audio sources"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    TestEqual(TEXT("Restore drops old presentation event history"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    const auto Stale=F.Storage->Transfer(F.Bag,true,TEXT("wood"),1,OldId,OldEpoch);
    TestTrue(TEXT("Old timeline request is rejected by the real transaction"),Stale.Result==EHearthwardInventoryResult::StaleTimeline);
    TestEqual(TEXT("A stale request cannot replay historical success"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    F.Deposit(FGuid::NewGuid());TestEqual(TEXT("New timeline observes only its own fresh operation"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),1);
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);
    TestEqual(TEXT("Exit destroys its effects sources"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    const int32 Before=FAudioFeedbackLifecycleAccess::Events(F.Presentation);F.Deposit(FGuid::NewGuid());
    TestEqual(TEXT("Ended presentation no longer consumes transaction delegates"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),Before);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioPause099Test,"Hearthward.Iteration.Task099.PauseAndLocalAccess",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioPause099Test::RunTest(const FString&)
{
    FAudioFeedbackWorld F;auto* Pauser=F.World->SpawnActor<APlayerState>();F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestTrue(TEXT("Fixture world is actually paused"),F.World->IsPaused());
    const FGuid PausedId=FGuid::NewGuid();
    TestEqual(TEXT("Diagnostic direct transaction commits while paused"),F.Deposit(PausedId).MovedCount,1);
    TestEqual(TEXT("Paused presentation does not enqueue a completion"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    F.Storage->OnTransferred.Broadcast(PausedId,true,TEXT("wood"),1);
    TestEqual(TEXT("Resume does not replay the completed paused transaction"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    const FGuid DistantId=FGuid::NewGuid();F.Player->SetActorLocation({1000,0,90});F.Deposit(DistantId);
    TestEqual(TEXT("Distant warehouse activity is not a global interface sound"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    F.Player->SetActorLocation({0,0,90});F.Storage->OnTransferred.Broadcast(DistantId,true,TEXT("wood"),1);
    TestEqual(TEXT("Returning near storage cannot replay distant history"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    F.Deposit(FGuid::NewGuid());
    TestEqual(TEXT("Fresh nearby transaction remains audible in the new context"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),1);
    const auto* Settings=F.Instance->GetSubsystem<UHearthwardPlayerSettings>();TArray<UAudioComponent*> Components;F.Player->GetComponents(Components);
    for(const auto* C:Components)if(C->ComponentTags.Contains(TEXT("Hearthward.Audio.effects")))
        TestEqual(TEXT("Source uses the actual effects volume setting"),C->VolumeMultiplier,Settings->Volume(TEXT("effects")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioPlayerSuccess099Test,"Hearthward.Iteration.Task099.PlayerCommittedSuccessUnbound",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioPlayerSuccess099Test::RunTest(const FString&)
{
    FAudioProducerWorld F;if(!TestTrue(TEXT("Actual controlled player can use the registered workbench"),F.Ready))return false;
    const FGuid Epoch=F.Storage->GetTimelineEpoch();
    if(!TestTrue(TEXT("First player craft commits through the actual workshop"),F.Building->Craft(F.Station,TEXT("rope"),1,Epoch))
        || !TestTrue(TEXT("A separate rapid craft commits through the same workshop"),F.Building->Craft(F.Station,TEXT("rope"),2,Epoch)))return false;
    TestEqual(TEXT("Three batches of the approved one-rope recipe enter the player's inventory"),F.Bag->GetItemCount(TEXT("rope")),3);
    TestEqual(TEXT("Only committed batch counts enter the observed craft ledger"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("craft:rope")),3);
    TestFalse(TEXT("An invalid batch request does not commit"),F.Building->Craft(F.Station,TEXT("rope"),100,Epoch));
    TestEqual(TEXT("A rejected craft does not advance the observed count"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("craft:rope")),3);

    if(!TestTrue(TEXT("An actual owned axe is available for repair"),F.Bag->TryAdd(TEXT("axe"),1)==EHearthwardInventoryResult::Success))return false;
    const FGuid Axe=F.Bag->FirstInstance(TEXT("axe"));
    if(!TestTrue(TEXT("The actual axe is damaged through the inventory API"),F.Bag->WearInstance(Axe,10)))return false;
    const double Worn=F.Bag->FindInstance(Axe)->Durability;TMap<FName,int32> Cost;double Restored=0;
    if(!TestTrue(TEXT("Actual repair quote identifies its payable cost"),HearthwardWorkshop::RepairQuote(F.Bag,Axe,1,Cost,Restored)))return false;
    for(const auto& Input:Cost)if(!TestTrue(TEXT("Repair inputs are supplied through the inventory API"),F.Bag->TryAdd(Input.Key,Input.Value)==EHearthwardInventoryResult::Success))return false;
    if(!TestTrue(TEXT("Player repair commits through the actual workshop"),F.Building->RepairInstance(F.Station,Axe,1,Epoch)))return false;
    TestEqual(TEXT("The real instance receives its quoted durability"),F.Bag->FindInstance(Axe)->Durability,Worn+Restored);
    TestEqual(TEXT("Committed repair is observed once"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("repair:axe")),1);
    TestFalse(TEXT("A fully repaired axe cannot settle again"),F.Building->RepairInstance(F.Station,Axe,1,Epoch));
    TestEqual(TEXT("Rejected repair produces no new observed success"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("repair:axe")),1);

    auto* Resource=F.World->SpawnActor<AActor>();auto* Target=NewObject<UHearthwardHarvestTargetComponent>(Resource);
    Resource->AddInstanceComponent(Target);Resource->SetRootComponent(Target);Target->ResourceKey=TEXT("task099_player_herb");
    Target->Item=TEXT("herb");Target->Capacity=4;Target->MaxDistance=240;Target->RegisterComponent();Target->SetWorldLocation(F.Player->GetActorLocation());
    auto* Camps=F.World->GetSubsystem<UHearthwardCampSubsystem>();Camps->RegisterSource(Target->ResourceKey,TEXT("herb"),4,4,Target->GetComponentLocation(),2880);
    auto* Harvest=F.World->GetSubsystem<UHearthwardHarvestSubsystem>();Harvest->Harvest(Target,F.Player);
    TestEqual(TEXT("Actual harvest deposits its committed yield"),F.Bag->GetItemCount(TEXT("herb")),2);
    TestEqual(TEXT("Actual resource depletion matches the committed yield"),Harvest->Remaining(Target->ResourceKey,4),2);
    TestEqual(TEXT("Committed harvest yield is observed"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("harvest:herb")),2);
    F.Player->SetActorLocation(FVector(5000,0,90));Harvest->Harvest(Target,F.Player);
    TestEqual(TEXT("Out-of-range harvest changes neither success count nor stock"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("harvest:herb")),2);
    TestEqual(TEXT("Rejected harvest retains the source"),Harvest->Remaining(Target->ResourceKey,4),2);
    for(const TCHAR* Key:{TEXT("craft:any"),TEXT("repair:any"),TEXT("harvest:any")})
        TestFalse(TEXT("Aggregate any counters never create a duplicate event route"),FAudioFeedbackLifecycleAccess::HasKey(F.Presentation,FName(Key)));
    F.Gameplay->OnChanged.Broadcast();
    TestEqual(TEXT("A repeated notification retains the actual craft count"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("craft:rope")),3);
    TestEqual(TEXT("Unbound successes do not claim an audible event"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    TestEqual(TEXT("Unbound successes create no replacement sound source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioNPCSuccess099Test,"Hearthward.Iteration.Task099.NPCCommittedSuccessUnbound",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioNPCSuccess099Test::RunTest(const FString&)
{
    FAudioProducerWorld F;if(!TestTrue(TEXT("Actual brother is initialized at the registered camp and workbench"),F.Ready))return false;
    auto* AI=F.World->GetSubsystem<UHearthwardLocalAISubsystem>();
    if(!TestTrue(TEXT("Actual collection source holds the contract's legal wood quantity"),F.Stock->TryAdd(TEXT("wood"),4)==EHearthwardInventoryResult::Success)
        || !TestTrue(TEXT("Brother owns the actual axe required for wood collection"),F.Brother->Bag->TryAdd(TEXT("axe"),1)==EHearthwardInventoryResult::Success))return false;
    FHearthwardAgentGoal Collect;Collect.Intent=TEXT("collect");Collect.Item=TEXT("wood");Collect.Quantity=2;
    Collect.QuantityMode=TEXT("additional_acquired");Collect.SourceRef=TEXT("S1");FGuid CollectCommand;
    if(!TestTrue(*(TEXT("Public collection command reaches committed completion: ")+F.Failure),F.Execute(Collect,CollectCommand)))
    {AddInfo(F.Failure);return false;}
    TestEqual(TEXT("Actual collection removes source stock"),F.Stock->GetItemCount(TEXT("wood")),2);
    TestEqual(TEXT("Actual delivery credits the shared warehouse"),F.Storage->GetItemCount(TEXT("wood")),2);
    auto Events=AI->GetEvents();const auto* Acquired=Events.FindByPredicate([&](const auto& E){return E.Command==CollectCommand && E.Kind==TEXT("acquired");});
    const auto* Delivered=Events.FindByPredicate([&](const auto& E){return E.Command==CollectCommand && E.Kind==TEXT("delivered");});
    if(!TestTrue(TEXT("Real settlement publishes acquired and delivered GUIDs"),Acquired && Delivered && Acquired->Id.IsValid() && Delivered->Id.IsValid()))return false;
    const FGuid AcquiredId=Acquired->Id,DeliveredId=Delivered->Id;
    TestTrue(TEXT("Actual warehouse receipt remains audible before the NPC event consumer"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,DeliveredId));
    const int32 TransferSounds=FAudioFeedbackLifecycleAccess::Sources(F.Player);
    auto* Pauser=F.World->SpawnActor<APlayerState>();F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Paused observation consumes the real acquired event"),FAudioFeedbackLifecycleAccess::Observed(F.Presentation,AcquiredId));
    TestTrue(TEXT("Paused observation consumes the real delivered event"),FAudioFeedbackLifecycleAccess::Observed(F.Presentation,DeliveredId));
    TestFalse(TEXT("An unbound acquired cue is not marked as played"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,AcquiredId));
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Resume cannot add a second source for the same formal delivery"),FAudioFeedbackLifecycleAccess::Sources(F.Player),TransferSounds);
    const auto Receipt=F.Storage->Transfer(F.Brother->Bag,true,TEXT("wood"),Delivered->Count,DeliveredId,F.Storage->GetTimelineEpoch());
    TestTrue(TEXT("The exact legal delivery operation replays its committed receipt"),Receipt.Replayed);
    TestEqual(TEXT("Legal storage receipt replay does not create another source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),TransferSounds);

    FHearthwardAgentGoal Craft;Craft.Intent=TEXT("craft");Craft.Item=TEXT("rope");Craft.Quantity=1;
    Craft.QuantityMode=TEXT("batches");Craft.SourceRef=TEXT("bag");Craft.Station=F.Station;
    for(const auto& Input:HearthwardWorkshop::Materials(TEXT("craft"),TEXT("rope"),1))
        if(!TestTrue(TEXT("Brother receives real recipe inputs"),F.Brother->Bag->TryAdd(Input.Key,Input.Value)==EHearthwardInventoryResult::Success))return false;
    FGuid CraftCommand;if(!TestTrue(TEXT("Public craft command reaches actual completion"),F.Execute(Craft,CraftCommand))){AddInfo(F.Failure);return false;}
    TestEqual(TEXT("Actual crafted products are delivered to camp"),F.Storage->GetItemCount(TEXT("rope")),HearthwardWorkshop::Outputs(TEXT("rope"),1).FindRef(TEXT("rope")));
    Events=AI->GetEvents();const auto* Crafted=Events.FindByPredicate([&](const auto& E){return E.Command==CraftCommand && E.Kind==TEXT("craft");});
    if(!TestTrue(TEXT("Actual craft publishes its own success GUID"),Crafted && Crafted->Id.IsValid()))return false;
    const FGuid CraftId=Crafted->Id;F.Player->SetActorLocation(FVector(5000,0,90));F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Distant real craft history is observed without enqueuing a return sound"),FAudioFeedbackLifecycleAccess::Observed(F.Presentation,CraftId));
    TestFalse(TEXT("Unbound distant craft never claims playback"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,CraftId));
    F.Player->SetActorLocation(FVector(0,0,90));F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestFalse(TEXT("Returning near brother does not replay old craft"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,CraftId));

    if(!TestTrue(TEXT("The actual collection axe remains owned for repair"),F.Brother->Bag->FirstInstance(TEXT("axe")).IsValid()))return false;
    const FGuid Axe=F.Brother->Bag->FirstInstance(TEXT("axe"));F.Brother->Bag->WearInstance(Axe,10);
    TMap<FName,int32> Cost;double Restored=0;const double Worn=F.Brother->Bag->FindInstance(Axe)->Durability;
    if(!TestTrue(TEXT("Actual brother repair quote succeeds"),HearthwardWorkshop::RepairQuote(F.Brother->Bag,Axe,1,Cost,Restored)))return false;
    for(const auto& Input:Cost)if(!TestTrue(TEXT("Brother repair materials are actually owned"),F.Brother->Bag->TryAdd(Input.Key,Input.Value)==EHearthwardInventoryResult::Success))return false;
    FHearthwardAgentGoal Repair;Repair.Intent=TEXT("repair");Repair.Item=TEXT("axe");Repair.Quantity=1;
    Repair.QuantityMode=TEXT("one_owned");Repair.SourceRef=TEXT("bag");Repair.Station=F.Station;Repair.EquipmentId=Axe;FGuid RepairCommand;
    if(!TestTrue(TEXT("Public repair command commits the exact owned instance"),F.Execute(Repair,RepairCommand))){AddInfo(F.Failure);return false;}
    TestEqual(TEXT("Brother's real axe receives the committed durability"),F.Brother->Bag->FindInstance(Axe)->Durability,Worn+Restored);
    Events=AI->GetEvents();const auto* Repaired=Events.FindByPredicate([&](const auto& E){return E.Command==RepairCommand && E.Kind==TEXT("repair");});
    if(!TestTrue(TEXT("Actual repair publishes its own success GUID"),Repaired && Repaired->Id.IsValid()))return false;
    const FGuid RepairId=Repaired->Id;
    const FGuid FreshStorage=FGuid::NewGuid();F.Deposit(FreshStorage);
    TestTrue(TEXT("Fresh legal storage remains audible after unbound NPC observations"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,FreshStorage));
    auto* Survival=F.Player->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!TestTrue(TEXT("Actual fatal damage establishes the dead-player suppression boundary"),Survival->ReceiveDamage(1,FGuid::NewGuid(),F.Storage->GetTimelineEpoch(),true))
        || !TestFalse(TEXT("The player is actually dead before consuming the fresh NPC event"),Survival->Alive()))return false;
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Fresh real repair is observed even when the player is dead"),FAudioFeedbackLifecycleAccess::Observed(F.Presentation,RepairId));
    TestFalse(TEXT("Unbound repair does not consume the audible event identity"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,RepairId));
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    const FGuid DeadStorage=FGuid::NewGuid();
    TestEqual(TEXT("Diagnostic actual transfer still commits under the dead-player boundary"),F.Deposit(DeadStorage).MovedCount,1);
    TestFalse(TEXT("Dead-player success is observed without claiming audible playback"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,DeadStorage));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioActualLoad099Test,"Hearthward.Iteration.Task099.ActualLoadSeedsHistory",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioActualLoad099Test::RunTest(const FString&)
{
    FString Pool;FGuid PoolId;
    if(!TestTrue(TEXT("Actual load fixture requires the runner's isolated save pool"),FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) && FGuid::Parse(Pool,PoolId)))return false;
    FAudioProducerWorld F;if(!TestTrue(TEXT("Actual load participants and registered workshop are ready"),F.Ready))return false;
    auto* Save=F.World->GetSubsystem<UHearthwardSaveSubsystem>();auto* AI=F.World->GetSubsystem<UHearthwardLocalAISubsystem>();
    if(!TestTrue(TEXT("Saved collection source uses the legal wood contract"),F.Stock->TryAdd(TEXT("wood"),4)==EHearthwardInventoryResult::Success)
        || !TestTrue(TEXT("Saved brother owns the actual wood collection tool"),F.Brother->Bag->TryAdd(TEXT("axe"),1)==EHearthwardInventoryResult::Success))return false;
    if(!TestTrue(*(TEXT("Actual save coordination enables: ")+Save->GetStatus()),Save->EnablePrototype())
        || !TestTrue(*(TEXT("Actual new campaign starts: ")+Save->GetStatus()),Save->StartNewProgress()))return false;
    const FGuid FirstEpoch=F.Storage->GetTimelineEpoch();
    if(!TestTrue(TEXT("Saved player craft is an actual committed operation"),F.Building->Craft(F.Station,TEXT("rope"),1,FirstEpoch)))return false;
    FHearthwardAgentGoal Collect;Collect.Intent=TEXT("collect");Collect.Item=TEXT("wood");Collect.Quantity=2;
    Collect.QuantityMode=TEXT("additional_acquired");Collect.SourceRef=TEXT("S1");FGuid Command;
    if(!TestTrue(TEXT("Saved NPC facts come from a real public collection command"),F.Execute(Collect,Command))){AddInfo(F.Failure);return false;}
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TSet<FGuid> Before;for(const auto& Point:Save->GetPoints())Before.Add(Point.SaveId);
    if(!TestTrue(*(TEXT("Actual disk-backed point saves committed facts: ")+Save->GetStatus()),Save->SavePoint(true)))return false;
    const auto Points=Save->GetPoints();const auto* Saved=Points.FindByPredicate([&](const auto& Point){return !Before.Contains(Point.SaveId);});
    if(!TestTrue(TEXT("The actual save publishes its own new point"),Saved!=nullptr))return false;
    const FGuid SavedId=Saved->SaveId;
    if(!TestTrue(TEXT("Unsaved later player craft commits independently"),F.Building->Craft(F.Station,TEXT("rope"),1,FirstEpoch)))return false;
    TestEqual(TEXT("Later real operation advances the live count"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("craft:rope")),2);
    if(!TestTrue(*(TEXT("Actual LoadPoint restores the committed history: ")+Save->GetStatus()),Save->LoadPoint(SavedId)))return false;
    TestTrue(TEXT("Real load advances the transaction epoch"),FirstEpoch!=F.Storage->GetTimelineEpoch());
    TestEqual(TEXT("Historical player success count is seeded from the actual snapshot"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("craft:rope")),1);
    TestEqual(TEXT("Load clears pre-load effects sources"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    TestEqual(TEXT("Historical effects never claim playback in the new epoch"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    const auto History=AI->GetEvents();TestTrue(TEXT("Actual load restores nonempty NPC success history"),History.ContainsByPredicate([](const auto& E){return E.Kind==TEXT("acquired");}));
    for(const auto& Event:History)if(Event.Id.IsValid())TestTrue(TEXT("Loaded NPC GUID history is seeded before any new frame"),FAudioFeedbackLifecycleAccess::Observed(F.Presentation,Event.Id));
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("First post-load frame cannot replay historical effects"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);F.Presentation->BeginPlay();
    for(const auto& Event:History)if(Event.Id.IsValid())TestTrue(TEXT("BeginPlay consumes existing NPC history without inventing playback"),FAudioFeedbackLifecycleAccess::Observed(F.Presentation,Event.Id));
    TestEqual(TEXT("BeginPlay seeds current player counters"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("craft:rope")),1);
    if(!TestTrue(TEXT("A fresh operation after real load still commits"),F.Building->Craft(F.Station,TEXT("rope"),1,F.Storage->GetTimelineEpoch())))return false;
    TestEqual(TEXT("Fresh post-load success advances only the current counter"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("craft:rope")),2);
    const FGuid FreshStorage=FGuid::NewGuid();F.Deposit(FreshStorage);
    TestTrue(TEXT("Fresh legal warehouse success remains audible after real load"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,FreshStorage));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioMovement099Test,"Hearthward.Iteration.Task099.ActualLandingAndWaterTransitions",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioMovement099Test::RunTest(const FString&)
{
    FAudioProducerWorld F;if(!TestTrue(*(TEXT("Actual movement participants are grounded: ")+F.Failure),F.Ready))return false;
    auto* Movement=F.Player->GetCharacterMovement();const int32 Before=FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.landed"));
    F.Player->LaunchCharacter(FVector(0,0,240),false,true);bool Fell=false;
    for(int32 I=0;I<80;++I)
    {
        Movement->TickComponent(.025f,LEVELTICK_All,nullptr);Fell|=Movement->IsFalling();
        if(Fell && Movement->IsMovingOnGround())break;
    }
    if(!TestTrue(TEXT("Actual launch physics falls and collides with the fixture floor"),Fell && Movement->IsMovingOnGround()))return false;
    TestEqual(TEXT("The actual capsule landing emits one observed event"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.landed")),Before+1);
    Movement->TickComponent(.025f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Remaining grounded does not fabricate another landing"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.landed")),Before+1);

    // The real default physics volume supplies the standalone water fixture, without event injection.
    auto* Volume=F.World->GetDefaultPhysicsVolume();Volume->bWaterVolume=true;
    auto* Bounds=NewObject<UBoxComponent>(Volume);Volume->AddInstanceComponent(Bounds);Bounds->SetupAttachment(Volume->GetRootComponent());
    Bounds->SetBoxExtent(FVector(1000,1000,100));Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bounds->RegisterComponent();Bounds->SetWorldLocation(FVector(0,0,500));
    auto* Traversal=NewObject<UHearthwardTraversalComponent>(F.Player);F.Player->AddInstanceComponent(Traversal);Traversal->RegisterComponent();
    if(!Traversal->HasBegunPlay())Traversal->BeginPlay();
    F.Player->SetActorLocation(FVector(0,0,300));float Surface=0;
    if(!TestTrue(TEXT("Production WaterSurface resolves the actual fixture physics volume"),F.Player->GetPhysicsVolume()==Volume && Traversal->WaterSurface(F.Player->GetActorLocation(),Surface) && Surface>300))return false;
    Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    if(!TestTrue(TEXT("Actual traversal chooses Swimming for an unsupported capsule below the water surface"),Movement->IsSwimming()))return false;
    TestEqual(TEXT("Production water entry is observed once"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.enter")),1);
    Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Remaining in Swimming does not fabricate another water entry"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.enter")),1);
    F.Player->SetActorLocation(FVector(0,0,700));Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    if(!TestTrue(TEXT("Actual traversal returns to Falling above the water surface"),Movement->IsFalling()))return false;
    TestEqual(TEXT("Production water exit is observed once"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.exit")),1);
    TestEqual(TEXT("Unbound landing and swimming do not claim audio playback"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    TestEqual(TEXT("Unbound movement never substitutes a warehouse sound"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);

    F.Presentation->EndPlay(EEndPlayReason::Destroyed);F.Player->SetActorLocation(FVector(0,0,300));Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("The actual producer can still enter water after presentation exit"),Movement->IsSwimming());
    TestEqual(TEXT("Ended presentation is unsubscribed from movement transitions"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.enter")),1);
    F.Presentation->BeginPlay();
    TestEqual(TEXT("BeginPlay seeds the current swimming mode without replaying entry"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.enter")),0);
    F.Player->SetActorLocation(FVector(0,0,700));Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("A fresh production exit remains observable after BeginPlay"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.exit")),1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioBoundLanding099Test,"Hearthward.Iteration.Task099.BoundActualLandingAndLifecycle",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioBoundLanding099Test::RunTest(const FString&)
{
    FAudioProducerWorld F(true);if(!TestTrue(*(TEXT("Bound landing starts with actual grounded participants: ")+F.Failure),F.Ready))return false;
    TestEqual(TEXT("Landing uses its dedicated production WAV"),FAudioFeedbackLifecycleAccess::CueFile(F.Presentation,TEXT("movement.landed")),FString(TEXT("TASK-099/movement-land.wav")));
    TestEqual(TEXT("Landing uses the effects volume channel"),FAudioFeedbackLifecycleAccess::CueChannel(F.Presentation,TEXT("movement.landed")),FName(TEXT("effects")));
    float Duration=0;if(!TestTrue(TEXT("Dedicated landing file contains a nonempty mono PCM16 wave"),ReadMovementCueDuration(TEXT("TASK-099/movement-land.wav"),Duration)))return false;
    const int32 PlayedBefore=FAudioFeedbackLifecycleAccess::Events(F.Presentation);
    const int32 LandedBefore=FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.landed"));
    auto* Movement=F.Player->GetCharacterMovement();
    TestEqual(TEXT("The real bootstrap landing has settled and expired"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    TestTrue(TEXT("Landing settlement is scheduled after character physics"),F.Presentation->PrimaryComponentTick.TickGroup==TG_PostPhysics);
    if(!TestTrue(TEXT("Actual launch falls back onto the blocking fixture floor"),LaunchAndLand(F)))return false;
    TestEqual(TEXT("The real collision publishes exactly one new landing"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.landed")),LandedBefore+1);
    TestEqual(TEXT("Landed callback waits for PostPhysics before creating audio"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    if(!TestTrue(TEXT("Actual landing has a walkable blocking floor contact"),Movement->CurrentFloor.IsWalkableFloor() && Movement->CurrentFloor.HitResult.IsValidBlockingHit()))return false;
    const FVector Impact=Movement->CurrentFloor.HitResult.ImpactPoint;
    // Moving before the deferred consumer must not move the already committed impact sound.
    F.Player->SetActorLocation(F.Player->GetActorLocation()+FVector(120,80,0));
    F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("PostPhysics creates exactly one mapped landing source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    auto* First=FAudioFeedbackLifecycleAccess::LatestSource(F.Presentation);
    if(!TestNotNull(TEXT("The real landing has an effects component"),First))return false;
    auto* Wave=Cast<USoundWaveProcedural>(First->GetSound());
    if(!TestNotNull(TEXT("The landing source owns an actual procedural PCM wave"),Wave))return false;
    TestTrue(TEXT("Landing PCM duration matches its own mapped WAV"),FMath::IsNearlyEqual(Wave->Duration,Duration,.00001f) && !Wave->bLooping && Wave->NumChannels==1);
    TestTrue(TEXT("Deferred sound stays at the real floor impact, not the moved capsule"),First->GetComponentLocation().Equals(Impact,.1)
        && !First->GetComponentLocation().Equals(F.Player->GetActorLocation(),.1));
    TestTrue(TEXT("Actual landing sound is spatial with applied attenuation"),First->bAllowSpatialization && First->bOverrideAttenuation && First->GetAttenuationSettingsToApply()!=nullptr);
    TestEqual(TEXT("Landing source obeys the current effects volume"),First->VolumeMultiplier,F.Instance->GetSubsystem<UHearthwardPlayerSettings>()->Volume(TEXT("effects")));
    for(int32 I=0;I<4;++I)Movement->TickComponent(.025f,LEVELTICK_All,nullptr);
    F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Remaining grounded cannot duplicate the landing source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    TestEqual(TEXT("Remaining grounded cannot duplicate the landing event"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.landed")),LandedBefore+1);
    if(!TestTrue(TEXT("A second actual launch independently lands"),LaunchAndLand(F)))return false;
    F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Separate rapid landings each retain their own sound"),FAudioFeedbackLifecycleAccess::Sources(F.Player),2);
    auto* Second=FAudioFeedbackLifecycleAccess::LatestSource(F.Presentation);
    TestTrue(TEXT("Separate landings allocate distinct source and wave objects"),Second && Second!=First && Second->GetSound()!=Wave);
    TestEqual(TEXT("Movement sounds do not grow the transactional played-GUID ledger"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),PlayedBefore);
    F.Presentation->TickComponent(Duration+.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Both procedural landing sources expire after their real WAV duration"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);

    if(!TestTrue(TEXT("Another real landing reaches the pre-PostPhysics pause boundary"),LaunchAndLand(F)))return false;
    auto* Pauser=F.World->SpawnActor<APlayerState>();F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestTrue(TEXT("The landing settlement boundary is actually paused"),F.World->IsPaused());
    F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Paused settlement consumes the landing without creating a source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Resume never replays a landing discarded while paused"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);

    if(!TestTrue(TEXT("Another actual collision supplies pending audio before EndPlay"),LaunchAndLand(F)))return false;
    const int32 BeforeEnd=FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.landed"));
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);
    if(!TestTrue(TEXT("Real capsule physics can still land after presentation EndPlay"),LaunchAndLand(F)))return false;
    TestEqual(TEXT("EndPlay unsubscribes the actual landing producer"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.landed")),BeforeEnd);
    TestEqual(TEXT("EndPlay leaves no movement effects"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    F.Presentation->BeginPlay();F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("BeginPlay cannot replay the pending or intervening historical landings"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    if(!TestTrue(TEXT("A fresh real landing works after restarting presentation"),LaunchAndLand(F)))return false;
    F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Only the fresh post-BeginPlay landing is audible"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    TestEqual(TEXT("Restarted movement still keeps the played-GUID ledger empty"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);
    TestEqual(TEXT("EndPlay also destroys an actively playing landing source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioBoundWater099Test,"Hearthward.Iteration.Task099.BoundActualWaterTransitionsAndLifecycle",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioBoundWater099Test::RunTest(const FString&)
{
    FAudioProducerWorld F(true);if(!TestTrue(*(TEXT("Bound water starts with real grounded participants: ")+F.Failure),F.Ready))return false;
    TestEqual(TEXT("Water entry uses its dedicated production WAV"),FAudioFeedbackLifecycleAccess::CueFile(F.Presentation,TEXT("movement.swim.enter")),FString(TEXT("TASK-099/movement-water-enter.wav")));
    TestEqual(TEXT("Water exit uses a separate dedicated production WAV"),FAudioFeedbackLifecycleAccess::CueFile(F.Presentation,TEXT("movement.swim.exit")),FString(TEXT("TASK-099/movement-water-exit.wav")));
    for(const TCHAR* Event:{TEXT("movement.swim.enter"),TEXT("movement.swim.exit")})
        TestEqual(TEXT("Each water transition uses the effects volume channel"),FAudioFeedbackLifecycleAccess::CueChannel(F.Presentation,FName(Event)),FName(TEXT("effects")));
    float EntryDuration=0,ExitDuration=0;
    if(!TestTrue(TEXT("Dedicated water entry file contains real mono PCM16"),ReadMovementCueDuration(TEXT("TASK-099/movement-water-enter.wav"),EntryDuration))
        || !TestTrue(TEXT("Dedicated water exit file contains real mono PCM16"),ReadMovementCueDuration(TEXT("TASK-099/movement-water-exit.wav"),ExitDuration)))return false;
    const int32 PlayedBefore=FAudioFeedbackLifecycleAccess::Events(F.Presentation);auto* Movement=F.Player->GetCharacterMovement();
    TestEqual(TEXT("Bootstrap physics leaves no live effects in the water fixture"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    auto* Volume=F.World->GetDefaultPhysicsVolume();Volume->bWaterVolume=true;
    auto* Bounds=NewObject<UBoxComponent>(Volume);Volume->AddInstanceComponent(Bounds);Bounds->SetupAttachment(Volume->GetRootComponent());
    Bounds->SetBoxExtent(FVector(1000,1000,100));Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bounds->RegisterComponent();Bounds->SetWorldLocation(FVector(0,0,500));
    auto* Traversal=NewObject<UHearthwardTraversalComponent>(F.Player);F.Player->AddInstanceComponent(Traversal);Traversal->RegisterComponent();
    if(!Traversal->HasBegunPlay())Traversal->BeginPlay();
    const FVector EntryPosition(0,0,300),ExitPosition(125,-70,700);F.Player->SetActorLocation(EntryPosition);float Surface=0;
    if(!TestTrue(TEXT("Real Traversal WaterSurface resolves the fixture physics volume"),F.Player->GetPhysicsVolume()==Volume && Traversal->WaterSurface(EntryPosition,Surface) && Surface>300))return false;
    Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    if(!TestTrue(TEXT("Real traversal enters Swimming below the water surface"),Movement->IsSwimming()))return false;
    TestEqual(TEXT("One actual water entry creates one source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    TestEqual(TEXT("One actual water entry is observed once"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.enter")),1);
    auto* Entry=FAudioFeedbackLifecycleAccess::LatestSource(F.Presentation);
    if(!TestNotNull(TEXT("Water entry creates an actual audio component"),Entry))return false;
    auto* EntryWave=Cast<USoundWaveProcedural>(Entry->GetSound());
    if(!TestNotNull(TEXT("Water entry source has actual PCM"),EntryWave))return false;
    TestTrue(TEXT("Entry PCM uses its own mapped duration"),FMath::IsNearlyEqual(EntryWave->Duration,EntryDuration,.00001f) && EntryWave->NumChannels==1 && !EntryWave->bLooping);
    TestTrue(TEXT("Entry sound is positioned at the character's real water transition"),Entry->GetComponentLocation().Equals(EntryPosition,.1) && Entry->bAllowSpatialization && Entry->bOverrideAttenuation);
    for(int32 I=0;I<4;++I)Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Repeated real Traversal ticks in Swimming never duplicate entry audio"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    TestEqual(TEXT("Remaining in Swimming does not increment the entry observation"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.enter")),1);
    F.Player->SetActorLocation(ExitPosition);Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    if(!TestTrue(TEXT("Real traversal leaves water for Falling above the surface"),Movement->IsFalling()))return false;
    TestEqual(TEXT("Entry and exit each retain their own sound"),FAudioFeedbackLifecycleAccess::Sources(F.Player),2);
    TestEqual(TEXT("The actual water exit is observed once"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.exit")),1);
    auto* Exit=FAudioFeedbackLifecycleAccess::LatestSource(F.Presentation);
    if(!TestNotNull(TEXT("Water exit creates an actual audio component"),Exit))return false;
    auto* ExitWave=Cast<USoundWaveProcedural>(Exit->GetSound());
    if(!TestNotNull(TEXT("Water exit source has actual PCM"),ExitWave))return false;
    TestTrue(TEXT("Exit PCM uses its own mapped duration"),FMath::IsNearlyEqual(ExitWave->Duration,ExitDuration,.00001f) && ExitWave->NumChannels==1 && !ExitWave->bLooping);
    TestTrue(TEXT("Entry and exit own independent source and PCM objects"),Exit!=Entry && ExitWave!=EntryWave);
    TestTrue(TEXT("Exit is spatial at its own transition position and leaves the entry source fixed"),Exit->GetComponentLocation().Equals(ExitPosition,.1)
        && Exit->bAllowSpatialization && Exit->bOverrideAttenuation && Entry->GetComponentLocation().Equals(EntryPosition,.1));
    for(const auto* Source:{Entry,Exit})TestEqual(TEXT("Each water transition respects effects volume"),Source->VolumeMultiplier,F.Instance->GetSubsystem<UHearthwardPlayerSettings>()->Volume(TEXT("effects")));
    for(int32 I=0;I<4;++I)Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Repeated dry Traversal ticks never duplicate exit audio"),FAudioFeedbackLifecycleAccess::Sources(F.Player),2);
    TestEqual(TEXT("Remaining out of water does not increment exit observation"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.exit")),1);
    TestEqual(TEXT("Water transitions do not retain per-event GUIDs indefinitely"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),PlayedBefore);
    F.Presentation->TickComponent(FMath::Min(EntryDuration,ExitDuration)*.5f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Both water sources survive until their own duration elapses"),FAudioFeedbackLifecycleAccess::Sources(F.Player),2);
    F.Presentation->TickComponent(FMath::Max(EntryDuration,ExitDuration)+.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Both finite water sources are destroyed after PCM playback"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);

    auto* Pauser=F.World->SpawnActor<APlayerState>();F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestTrue(TEXT("Water fixture is genuinely paused"),F.World->IsPaused());
    F.Player->SetActorLocation(EntryPosition);Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Actual paused Traversal does not commit a Swimming transition"),Movement->IsFalling());
    TestEqual(TEXT("Paused traversal produces no entry sound"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    F.Player->SetActorLocation(ExitPosition);F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Resume outside the water has no queued transition to replay"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    TestEqual(TEXT("Paused movement never fabricated a second entry receipt"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.enter")),1);
    F.Player->SetActorLocation(EntryPosition);Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("A fresh real entry after resume still sounds exactly once"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);
    TestEqual(TEXT("EndPlay destroys the current water entry source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    F.Player->SetActorLocation(ExitPosition);Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Real traversal still leaves water after presentation EndPlay"),Movement->IsFalling());
    TestEqual(TEXT("Ended presentation no longer observes actual water exits"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.exit")),1);
    F.Player->SetActorLocation(EntryPosition);Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    if(!TestTrue(TEXT("Real traversal re-enters water while presentation is stopped"),Movement->IsSwimming()))return false;
    F.Presentation->BeginPlay();F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("BeginPlay seeds current Swimming without replaying history"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    TestEqual(TEXT("BeginPlay does not invent a historical entry receipt"),FAudioFeedbackLifecycleAccess::MovementCount(F.Presentation,TEXT("movement.swim.enter")),0);
    F.Player->SetActorLocation(ExitPosition);Traversal->TickComponent(.025f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Only a fresh actual water exit sounds after restart"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    TestEqual(TEXT("Restarted water audio still does not grow played-GUID history"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);
    TestEqual(TEXT("EndPlay clears the fresh exit source as well"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioProgress099Test,"Hearthward.Iteration.Task099.ProgressCommittedSuccessUnbound",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioProgress099Test::RunTest(const FString&)
{
    FAudioProducerWorld F;if(!TestTrue(*(TEXT("Actual progress participants are ready: ")+F.Failure),F.Ready))return false;
    auto* Camps=F.World->GetSubsystem<UHearthwardCampSubsystem>();const FGuid Epoch=F.Storage->GetTimelineEpoch();
    const int32 RescuePlayed=FAudioFeedbackLifecycleAccess::Events(F.Presentation);
    if(!TestTrue(TEXT("Actual first rescue commits through the camp API"),Camps->RecordRescue(TEXT("rescued_01"))))return false;
    TestTrue(TEXT("The committed camp state owns the actual person identity"),Camps->State.Rescued.Contains(TEXT("rescued_01")));
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("A newly committed rescue enters the observation ledger"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("rescue:rescued_01")),1);
    TestEqual(TEXT("Unbound rescue observation does not add audible playback"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),RescuePlayed);
    const int32 XP=F.Gameplay->Experience;
    TestFalse(TEXT("The same person cannot receive a second rescue award"),Camps->RecordRescue(TEXT("rescued_01")));
    TestEqual(TEXT("Duplicate rejection preserves the actual experience balance"),F.Gameplay->Experience,XP);
    TestFalse(TEXT("An invalid facility identity cannot upgrade"),F.Building->UpgradeFacility(FGuid::NewGuid(),Epoch));
    TestFalse(TEXT("The actual workbench remains locked at the initial camp tier"),F.Building->UpgradeFacility(F.Station,Epoch));
    const FName FacilityKey(*(TEXT("facility_level:")+F.Station.ToString()));
    if(!TestTrue(*(TEXT("Real camp costs, food donation and timed workbench upgrade complete: ")+F.Failure),F.UpgradeWorkbenchToLevel2()))
    {AddInfo(F.Failure);return false;}
    const int32 UpgradePlayed=FAudioFeedbackLifecycleAccess::Events(F.Presentation);
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Observation reads the level published by actual timed upgrade completion"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,FacilityKey),2);
    TestEqual(TEXT("Unbound upgrade observation does not add audible playback"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),UpgradePlayed);
    TestFalse(TEXT("Next workbench level still obeys its real required camp tier"),F.Building->UpgradeFacility(F.Station,Epoch));
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("A rejected upgrade cannot change the observed committed level"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,FacilityKey),2);
    auto* Campaign=PrepareProgressQuest(F);
    TestFalse(TEXT("A stale quest request cannot publish claimed identity"),Campaign->Claim(TEXT("side_10"),FGuid::NewGuid()));
    if(!TestTrue(TEXT("Actual eligible quest Claim commits its rewards and identity"),Campaign->Claim(TEXT("side_10"),Epoch)))return false;
    TestTrue(TEXT("Actual Claim publishes its stable quest ID"),F.Gameplay->Claimed.Contains(TEXT("side_10")));
    // Claim already emits its legitimate fixed completion subtitle using the shared played-event ledger.
    const int32 QuestPlayed=FAudioFeedbackLifecycleAccess::Events(F.Presentation);
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("The actual new claimed identity is observed once"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("quest_claimed:side_10")),1);
    const int32 ClaimedXP=F.Gameplay->Experience,RewardWood=F.Storage->GetItemCount(TEXT("wood"));
    TestFalse(TEXT("Actual duplicate quest Claim cannot pay another reward"),Campaign->Claim(TEXT("side_10"),Epoch));
    TestEqual(TEXT("Duplicate Claim preserves actual experience"),F.Gameplay->Experience,ClaimedXP);
    TestEqual(TEXT("Duplicate Claim preserves actual shared items"),F.Storage->GetItemCount(TEXT("wood")),RewardWood);
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Unbound progress observations preserve existing legitimate fixed-cue events"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),QuestPlayed);
    TestEqual(TEXT("Unbound progress never substitutes warehouse audio"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    if(!TestTrue(TEXT("A fresh legal warehouse input is supplied"),F.Bag->TryAdd(TEXT("wood"),1)==EHearthwardInventoryResult::Success))return false;
    const FGuid Fresh=FGuid::NewGuid();TestEqual(TEXT("Fresh warehouse transfer really commits after progress observations"),F.Deposit(Fresh).MovedCount,1);
    TestTrue(TEXT("Unbound progress cannot swallow a fresh actual warehouse cue"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,Fresh));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioProgressLoad099Test,"Hearthward.Iteration.Task099.ProgressActualLoadAndSuppression",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioProgressLoad099Test::RunTest(const FString&)
{
    FString Pool;FGuid PoolId;if(!TestTrue(TEXT("Progress load fixture requires the isolated UUID save pool"),FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) && FGuid::Parse(Pool,PoolId)))return false;
    FAudioProducerWorld F;if(!TestTrue(*(TEXT("Actual progress load participants are ready: ")+F.Failure),F.Ready))return false;
    auto* Camps=F.World->GetSubsystem<UHearthwardCampSubsystem>();auto* Save=F.World->GetSubsystem<UHearthwardSaveSubsystem>();
    if(!TestTrue(TEXT("Saved rescue comes from actual camp settlement"),Camps->RecordRescue(TEXT("rescued_01")))
        || !TestTrue(TEXT("Saved facility level comes from actual camp and timed building APIs"),F.UpgradeWorkbenchToLevel2())){AddInfo(F.Failure);return false;}
    auto* Campaign=PrepareProgressQuest(F);const FGuid BeforeEpoch=F.Storage->GetTimelineEpoch();
    if(!TestTrue(TEXT("Saved claimed quest comes from actual successful Claim"),Campaign->Claim(TEXT("side_10"),BeforeEpoch)))return false;
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    if(!TestTrue(*(TEXT("Actual progress save coordination enables: ")+Save->GetStatus()),Save->EnablePrototype())
        || !TestTrue(*(TEXT("Actual progress campaign starts: ")+Save->GetStatus()),Save->StartNewProgress()))return false;
    TSet<FGuid> Before;for(const auto& Point:Save->GetPoints())Before.Add(Point.SaveId);
    if(!TestTrue(*(TEXT("Actual progress point persists to disk: ")+Save->GetStatus()),Save->SavePoint(true)))return false;
    const auto Points=Save->GetPoints();const auto* Saved=Points.FindByPredicate([&](const auto& Point){return !Before.Contains(Point.SaveId);});
    if(!TestTrue(TEXT("The real progress save publishes a fresh point"),Saved!=nullptr))return false;
    const FGuid SavedId=Saved->SaveId;
    auto* Pauser=F.World->SpawnActor<APlayerState>();F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    if(!TestTrue(TEXT("Diagnostic actual rescue settlement commits under the paused boundary"),Camps->RecordRescue(TEXT("rescued_02"))))return false;
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Paused observation consumes the actual new person without backlog"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("rescue:rescued_02")),1);
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Resume cannot create substituted progress audio"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    if(!TestTrue(*(TEXT("Actual LoadPoint restores the progress snapshot: ")+Save->GetStatus()),Save->LoadPoint(SavedId)))return false;
    TestTrue(TEXT("Actual progress load advances timeline identity"),F.Storage->GetTimelineEpoch()!=BeforeEpoch);
    TestTrue(TEXT("Actual load restores the saved rescued person"),Camps->State.Rescued.Contains(TEXT("rescued_01")));
    TestFalse(TEXT("Actual load removes the unsaved later rescued person"),Camps->State.Rescued.Contains(TEXT("rescued_02")));
    const FName FacilityKey(*(TEXT("facility_level:")+F.Station.ToString()));
    TestEqual(TEXT("Load seeds saved rescue history before a new frame"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("rescue:rescued_01")),1);
    TestEqual(TEXT("Load seeds actual saved facility level"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,FacilityKey),2);
    TestEqual(TEXT("Load seeds saved claimed quest identity"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("quest_claimed:side_10")),1);
    TestEqual(TEXT("Unsaved later progress identity is absent from the new epoch"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("rescue:rescued_02")),0);
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Post-load observation does not replay historical sources"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    TestEqual(TEXT("Actual Load resets historical played identities without progress replay"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);F.Presentation->BeginPlay();
    if(!TestTrue(TEXT("Actual BeginPlay reloads the production rescue cue configuration"),FAudioFeedbackLifecycleAccess::CueBound(F.Presentation,TEXT("camp.rescue"))))return false;
    FAudioFeedbackLifecycleAccess::KeepStorageCueOnly(F.Presentation);
    TestFalse(TEXT("The restarted unbound fixture explicitly removes the reloaded rescue cue"),FAudioFeedbackLifecycleAccess::CueBound(F.Presentation,TEXT("camp.rescue")));
    TestEqual(TEXT("BeginPlay does not create a historical progress source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    TestEqual(TEXT("BeginPlay seeds the current committed rescued identity"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("rescue:rescued_01")),1);
    TestEqual(TEXT("BeginPlay seeds the current committed quest identity"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("quest_claimed:side_10")),1);
    if(!TestTrue(TEXT("The same later person can freshly settle in the restored epoch"),Camps->RecordRescue(TEXT("rescued_02"))))return false;
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Only the fresh current-epoch rescue is observed"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("rescue:rescued_02")),1);
    TestEqual(TEXT("The fresh restarted unbound rescue creates no substituted source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    auto* Survival=F.Player->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!TestTrue(TEXT("Actual fatal damage establishes the progress dead-player boundary"),Survival->ReceiveDamage(1,FGuid::NewGuid(),F.Storage->GetTimelineEpoch(),true)))return false;
    if(!TestTrue(TEXT("Diagnostic actual new rescue commits under the dead-player boundary"),Camps->RecordRescue(TEXT("rescued_03"))))return false;
    F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Dead-player observation still consumes the actual newly committed person"),FAudioFeedbackLifecycleAccess::Count(F.Presentation,TEXT("rescue:rescued_03")),1);
    TestEqual(TEXT("Unbound suppressed progress never claims audible success"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioCombatConsumer099Test,"Hearthward.Iteration.Task099.CombatProducerConsumerUnbound",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioCombatConsumer099Test::RunTest(const FString&)
{
    FAudioCombatWorld F;if(!TestTrue(*(TEXT("Actual grounded combat participants are ready: ")+F.Failure),F.Ready))return false;
    TArray<FHearthwardCombatFeedbackReceipt> Receipts;
    F.Combat->OnCombatSucceeded.AddLambda([&](const auto& Receipt){Receipts.Add(Receipt);});
    F.Survival->OnDamageSucceeded.AddLambda([&](const auto& Receipt){Receipts.Add(Receipt);});
    auto* BrotherSurvival=F.Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    BrotherSurvival->OnDamageSucceeded.AddLambda([&](const auto& Receipt){Receipts.Add(Receipt);});
    const int32 PlayedBefore=FAudioFeedbackLifecycleAccess::Events(F.Presentation);
    auto* First=F.Target(TEXT("audio_consumer_first"),FVector(100,0,0));
    auto* Second=F.Target(TEXT("audio_consumer_second"),FVector(150,20,0));const FGuid Operation=FGuid::NewGuid();
    F.Combat->HitTarget(First,10,TEXT("body"),false,Operation);
    F.Combat->HitTarget(First,10,TEXT("body"),false,Operation);
    F.Combat->HitTarget(Second,10,TEXT("body"),false,Operation);
    if(!TestEqual(TEXT("One operation commits exactly two different target receipts"),Receipts.Num(),2))return false;
    TestEqual(TEXT("Real first target was damaged once"),First->Health,90.f);
    TestEqual(TEXT("Real second target was also damaged"),Second->Health,90.f);
    TestEqual(TEXT("The consumer retains both targets instead of deduplicating the whole action"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),2);
    for(const auto& Receipt:Receipts)
    {
        TestTrue(TEXT("Actual successful hit identity is consumed"),FAudioFeedbackLifecycleAccess::CombatObserved(F.Presentation,Receipt.SuccessId));
        TestTrue(TEXT("Consumer keeps the actual settled target position"),FAudioFeedbackLifecycleAccess::CombatPosition(F.Presentation,Receipt.SuccessId)==Receipt.TargetPosition);
        TestFalse(TEXT("Unbound combat cannot claim warehouse playback"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,Receipt.SuccessId));
    }
    F.Combat->HitTarget(Second,10,NAME_None,false,FGuid::NewGuid());
    TestEqual(TEXT("A rejected unmapped hit does not enter the consumer"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),2);
    if(!TestTrue(TEXT("Actual shield is owned and equipped"),F.Bag->TryAdd(TEXT("shield"),1)==EHearthwardInventoryResult::Success && F.Gameplay->CommitEquipment(TEXT("shield")))
        || !TestTrue(TEXT("Public guard starts on the actual equipped character"),F.Combat->SetGuard(true)))return false;
    F.World->GetSubsystem<UHearthwardWorldClockSubsystem>()->Tick(.16f);
    const float HealthBeforeBlock=F.Gameplay->Health;
    const FGuid Block=FGuid::NewGuid();
    if(!TestTrue(TEXT("Real guard commits a block before consumer feedback"),F.Combat->Damage(10,TEXT("body"),F.Player->GetActorLocation()+FVector(100,0,0),false,false,Block)))return false;
    TestEqual(TEXT("Actual guarded hit preserves the health after elapsed recovery"),F.Gameplay->Health,HealthBeforeBlock);
    TestFalse(TEXT("Repeated legal block operation cannot publish a second event"),F.Combat->Damage(10,TEXT("body"),F.Player->GetActorLocation()+FVector(100,0,0),false,false,Block));
    F.Combat->SetGuard(false);
    if(!TestTrue(TEXT("Own real injury commits through Survival"),F.Survival->ReceiveDamage(2,FGuid::NewGuid(),F.Storage->GetTimelineEpoch()))
        || !TestTrue(TEXT("Current brother real injury commits through Survival"),BrotherSurvival->ReceiveDamage(2,FGuid::NewGuid(),F.Storage->GetTimelineEpoch())))return false;
    TestEqual(TEXT("Two targets, one block and two actor injuries are independently consumed"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),5);
    if(!TestEqual(TEXT("All consumer identities originate in five real producer receipts"),Receipts.Num(),5))return false;
    for(const auto& Receipt:Receipts)TestTrue(TEXT("Every actual hit/block/damage receipt is consumed"),FAudioFeedbackLifecycleAccess::CombatObserved(F.Presentation,Receipt.SuccessId));
    TestEqual(TEXT("Unbound combat adds no audible event to the existing ledger"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),PlayedBefore);
    TestEqual(TEXT("Unbound combat creates no substituted warehouse source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioCombatSuppression099Test,"Hearthward.Iteration.Task099.CombatSuppressionAndRebinding",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioCombatSuppression099Test::RunTest(const FString&)
{
    FAudioCombatWorld F;if(!TestTrue(TEXT("Actual combat lifecycle participants are ready"),F.Ready))return false;
    TArray<FHearthwardCombatFeedbackReceipt> Receipts;
    F.Combat->OnCombatSucceeded.AddLambda([&](const auto& Receipt){Receipts.Add(Receipt);});
    auto* Near=F.Target(TEXT("audio_paused_target"),FVector(100,0,0));auto* Far=F.Target(TEXT("audio_far_target"),FVector(5000,0,0));
    auto* Pauser=F.World->SpawnActor<APlayerState>();F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    F.Combat->HitTarget(Near,1,TEXT("body"),false,FGuid::NewGuid());
    TestEqual(TEXT("Paused direct diagnostic hit really commits target health"),Near->Health,99.f);
    TestEqual(TEXT("Paused true success is consumed without a backlog"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),1);
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    F.Combat->HitTarget(Far,1,TEXT("body"),false,FGuid::NewGuid());
    TestEqual(TEXT("Distant true success is observed without being queued for approach"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),2);
    Far->GetOwner()->SetActorLocation(FVector(100,20,0));F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Resume and approach do not replay observed receipts"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    auto* PreviousBrotherSurvival=F.Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    TestTrue(TEXT("Current brother was subscribed through the native delegate"),PreviousBrotherSurvival->OnDamageSucceeded.IsBoundToObject(F.Presentation));
    F.Brother->Destroy();
    FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    F.Brother=F.World->SpawnActor<AHearthwardCompanionFixture>(FVector(200,-100,80),FRotator::ZeroRotator,Spawn);
    F.Brother->InitializeCompanion(F.Stock,F.Stock->GetOwner());F.Presentation->TickComponent(.01f,LEVELTICK_All,nullptr);
    auto* CurrentBrotherSurvival=F.Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    TestFalse(TEXT("Replacing brother removes the old object's listener"),PreviousBrotherSurvival->OnDamageSucceeded.IsBoundToObject(F.Presentation));
    TestTrue(TEXT("New brother receives its own real listener"),CurrentBrotherSurvival->OnDamageSucceeded.IsBoundToObject(F.Presentation));
    if(!TestTrue(TEXT("Replacement brother has enabled real survival"),CurrentBrotherSurvival->Enabled())
        || !TestTrue(TEXT("Replacement brother commits a fresh real injury"),CurrentBrotherSurvival->ReceiveDamage(1,FGuid::NewGuid(),F.Storage->GetTimelineEpoch())))return false;
    TestEqual(TEXT("Only the replacement brother's fresh receipt is added"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),3);
    if(!TestTrue(TEXT("Actual fatal damage establishes the listener's death boundary"),F.Survival->ReceiveDamage(1,FGuid::NewGuid(),F.Storage->GetTimelineEpoch(),true))
        || !TestTrue(TEXT("Brother real injury can be observed while listener is dead"),CurrentBrotherSurvival->ReceiveDamage(1,FGuid::NewGuid(),F.Storage->GetTimelineEpoch())))return false;
    TestEqual(TEXT("Death and fresh brother injury are consumed without sound backlog"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),5);
    TestEqual(TEXT("All suppressed unbound events remain silent"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);
    TestFalse(TEXT("EndPlay removes own combat subscription"),F.Combat->OnCombatSucceeded.IsBoundToObject(F.Presentation));
    TestFalse(TEXT("EndPlay removes own survival subscription"),F.Survival->OnDamageSucceeded.IsBoundToObject(F.Presentation));
    TestFalse(TEXT("EndPlay removes current brother survival subscription"),CurrentBrotherSurvival->OnDamageSucceeded.IsBoundToObject(F.Presentation));
    if(!TestTrue(TEXT("Real brother producer still commits after presentation exits"),CurrentBrotherSurvival->ReceiveDamage(1,FGuid::NewGuid(),F.Storage->GetTimelineEpoch())))return false;
    TestEqual(TEXT("Ended presentation cannot observe later successful producers"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),5);
    F.Presentation->BeginPlay();
    TestEqual(TEXT("BeginPlay has no persistent combat history to replay"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),0);
    TestEqual(TEXT("BeginPlay does not create a historical combat source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioCombatLoad099Test,"Hearthward.Iteration.Task099.CombatActualLoadNoHistory",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioCombatLoad099Test::RunTest(const FString&)
{
    FString Pool;FGuid PoolId;if(!TestTrue(TEXT("Combat load uses the runner's real isolated UUID pool"),FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) && FGuid::Parse(Pool,PoolId)))return false;
    FAudioCombatWorld F;if(!TestTrue(TEXT("Actual grounded combat/save participants are ready"),F.Ready))return false;
    auto* Save=F.World->GetSubsystem<UHearthwardSaveSubsystem>();TArray<FHearthwardCombatFeedbackReceipt> Receipts;
    F.Survival->OnDamageSucceeded.AddLambda([&](const auto& Receipt){Receipts.Add(Receipt);});
    if(!TestTrue(*(TEXT("Actual combat save coordination enables: ")+Save->GetStatus()),Save->EnablePrototype())
        || !TestTrue(*(TEXT("Actual isolated combat campaign starts: ")+Save->GetStatus()),Save->StartNewProgress()))return false;
    const FGuid FirstEpoch=F.Storage->GetTimelineEpoch();
    if(!TestTrue(TEXT("Saved injury is a real committed damage operation"),F.Survival->ReceiveDamage(2,FGuid::NewGuid(),FirstEpoch)))return false;
    TestEqual(TEXT("Saved injury was consumed when it actually happened"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),1);
    const float SavedHealth=F.Gameplay->Health;TSet<FGuid> Before;for(const auto& Point:Save->GetPoints())Before.Add(Point.SaveId);
    if(!TestTrue(*(TEXT("Actual manual point persists combat participants: ")+Save->GetStatus()),Save->SavePoint(true)))return false;
    const auto Points=Save->GetPoints();const auto* Saved=Points.FindByPredicate([&](const auto& Point){return !Before.Contains(Point.SaveId);});
    if(!TestTrue(TEXT("Real combat point has its own new save identity"),Saved!=nullptr))return false;
    const FGuid SavedId=Saved->SaveId;
    if(!TestTrue(TEXT("Later unsaved injury commits separately"),F.Survival->ReceiveDamage(1,FGuid::NewGuid(),FirstEpoch)))return false;
    TestEqual(TEXT("Later unsaved injury adds its own real receipt"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),2);
    if(!TestTrue(*(TEXT("Actual LoadPoint restores the saved combat state: ")+Save->GetStatus()),Save->LoadPoint(SavedId)))return false;
    TestEqual(TEXT("Real load restores the saved health"),F.Gameplay->Health,SavedHealth);
    TestTrue(TEXT("Real load advances the producer epoch"),F.Storage->GetTimelineEpoch()!=FirstEpoch);
    TestEqual(TEXT("Load drops presentation combat receipts instead of replaying health differences"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),0);
    TestEqual(TEXT("Load has no historical combat audio source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    TestFalse(TEXT("Old-epoch damage is rejected by its actual producer"),F.Survival->ReceiveDamage(1,FGuid::NewGuid(),FirstEpoch));
    if(!TestTrue(TEXT("Fresh actual damage after load is accepted"),F.Survival->ReceiveDamage(1,FGuid::NewGuid(),F.Storage->GetTimelineEpoch())))return false;
    TestEqual(TEXT("Only the fresh post-load success enters the consumer"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),1);
    if(!TestEqual(TEXT("Only genuine producer operations published the three receipts"),Receipts.Num(),3))return false;
    TestTrue(TEXT("Fresh post-load receipt identity is actually consumed"),FAudioFeedbackLifecycleAccess::CombatObserved(F.Presentation,Receipts.Last().SuccessId));
    TestEqual(TEXT("Unbound combat never substitutes a warehouse sound after load"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioBoundSuccess099Test,"Hearthward.Iteration.Task099.BoundSuccessAndSpatialAttenuation",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioBoundSuccess099Test::RunTest(const FString&)
{
    FAudioCombatWorld F(true);if(!TestTrue(TEXT("Bound fixture uses real grounded production participants"),F.Ready))return false;
    for(const auto& Input:HearthwardWorkshop::Materials(TEXT("craft"),TEXT("rope"),1))
        if(!TestTrue(TEXT("Bound craft owns its actual recipe input"),F.Bag->TryAdd(Input.Key,Input.Value)==EHearthwardInventoryResult::Success))return false;
    if(!TestTrue(TEXT("Real player craft commits before its bound feedback"),F.Building->Craft(F.Station,TEXT("rope"),1,F.Storage->GetTimelineEpoch())))return false;
    TestEqual(TEXT("A bound real craft creates one sound source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    auto* Craft=FAudioFeedbackLifecycleAccess::LatestSource(F.Presentation);
    if(!TestNotNull(TEXT("Actual mapped craft PCM creates an effects component"),Craft))return false;
    TestFalse(TEXT("Existing non-position confirmation remains non-spatial"),Craft->bAllowSpatialization);
    TArray<FHearthwardCombatFeedbackReceipt> Receipts;
    const FDelegateHandle Handle=F.Combat->OnCombatSucceeded.AddLambda([&](const auto& Receipt){Receipts.Add(Receipt);});
    auto* First=F.Target(TEXT("audio_bound_first"),FVector(100,0,0));auto* Second=F.Target(TEXT("audio_bound_second"),FVector(150,20,0));
    const FGuid Operation=FGuid::NewGuid();F.Combat->HitTarget(First,10,TEXT("body"),false,Operation);
    auto* Spatial=FAudioFeedbackLifecycleAccess::LatestSource(F.Presentation);
    if(!TestNotNull(TEXT("Actual hit cue creates a spatial component"),Spatial) || !TestEqual(TEXT("Actual first hit publishes its own receipt"),Receipts.Num(),1))
    {F.Combat->OnCombatSucceeded.Remove(Handle);return false;}
    TestTrue(TEXT("Audio source uses the real committed target position"),Spatial->GetComponentLocation()==First->GetOwner()->GetActorLocation());
    TestTrue(TEXT("Actual hit is recorded as audible only after creating its source"),FAudioFeedbackLifecycleAccess::Played(F.Presentation,Receipts[0].SuccessId));
    const auto* Attenuation=Spatial->GetAttenuationSettingsToApply();
    if(!TestNotNull(TEXT("Positioned source has actual applied attenuation settings"),Attenuation))
    {F.Combat->OnCombatSucceeded.Remove(Handle);return false;}
    TestTrue(TEXT("Applied settings enable spatialization and attenuation"),Spatial->bAllowSpatialization && Spatial->bOverrideAttenuation && Attenuation->bSpatialize && Attenuation->bAttenuate);
    const FVector Origin=Spatial->GetComponentLocation();const FTransform Transform(Origin);
    TestTrue(TEXT("UE attenuation evaluates unity at the source"),FMath::IsNearlyEqual(Attenuation->Evaluate(Transform,Origin),1.f));
    TestTrue(TEXT("UE attenuation evaluates half gain at fifteen metres"),FMath::IsNearlyEqual(Attenuation->Evaluate(Transform,Origin+FVector(1500,0,0)),.5f));
    TestTrue(TEXT("UE attenuation reaches zero at the existing thirty metre boundary"),FMath::IsNearlyZero(Attenuation->Evaluate(Transform,Origin+FVector(3000,0,0))));
    F.Combat->HitTarget(First,10,TEXT("body"),false,Operation);
    TestEqual(TEXT("Duplicate actual operation creates no second hit source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),2);
    TestEqual(TEXT("Duplicate actual operation does not settle damage twice"),First->Health,90.f);
    F.Combat->HitTarget(Second,10,TEXT("body"),false,Operation);
    TestEqual(TEXT("A different real target retains its own mapped sound"),FAudioFeedbackLifecycleAccess::Sources(F.Player),3);
    TestEqual(TEXT("Both real targets of one operation publish independent receipts"),Receipts.Num(),2);
    F.Combat->OnCombatSucceeded.Remove(Handle);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioBoundLoad099Test,"Hearthward.Iteration.Task099.BoundPauseAndActualLoad",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAudioBoundLoad099Test::RunTest(const FString&)
{
    FString Pool;FGuid PoolId;if(!TestTrue(TEXT("Bound actual load uses the isolated UUID pool"),FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) && FGuid::Parse(Pool,PoolId)))return false;
    FAudioCombatWorld F(true);if(!TestTrue(TEXT("Bound save participants are genuinely grounded"),F.Ready))return false;
    auto* Save=F.World->GetSubsystem<UHearthwardSaveSubsystem>();
    if(!TestTrue(*(TEXT("Bound actual save coordination enables: ")+Save->GetStatus()),Save->EnablePrototype())
        || !TestTrue(*(TEXT("Bound actual campaign starts: ")+Save->GetStatus()),Save->StartNewProgress()))return false;
    if(!TestTrue(TEXT("Bound player injury really commits before playback"),F.Survival->ReceiveDamage(2,FGuid::NewGuid(),F.Storage->GetTimelineEpoch())))return false;
    TestEqual(TEXT("Real bound damage creates one actual source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    TestEqual(TEXT("Only that committed bound event enters the played ledger"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),1);
    TSet<FGuid> Before;for(const auto& Point:Save->GetPoints())Before.Add(Point.SaveId);
    if(!TestTrue(*(TEXT("Bound actual point saves: ")+Save->GetStatus()),Save->SavePoint(true)))return false;
    const auto Points=Save->GetPoints();const auto* Saved=Points.FindByPredicate([&](const auto& Point){return !Before.Contains(Point.SaveId);});
    if(!TestTrue(TEXT("Bound save has an actual fresh point identity"),Saved!=nullptr))return false;
    const FGuid SavedId=Saved->SaveId;
    auto* Pauser=F.World->SpawnActor<APlayerState>();F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    auto* BrotherSurvival=F.Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!TestTrue(TEXT("Paused real brother injury still has a committed diagnostic receipt"),BrotherSurvival->ReceiveDamage(1,FGuid::NewGuid(),F.Storage->GetTimelineEpoch())))return false;
    TestEqual(TEXT("Paused success is observed immediately"),FAudioFeedbackLifecycleAccess::CombatCount(F.Presentation),2);
    TestEqual(TEXT("Paused success does not add a source to the existing sound"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);F.Presentation->TickComponent(.001f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Resume does not replay the paused bound success"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),1);
    if(!TestTrue(*(TEXT("Bound actual point loads: ")+Save->GetStatus()),Save->LoadPoint(SavedId)))return false;
    TestEqual(TEXT("Actual load destroys all pre-load bound components"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    TestEqual(TEXT("Actual load does not turn restored health into a new sound"),FAudioFeedbackLifecycleAccess::Events(F.Presentation),0);
    if(!TestTrue(TEXT("Fresh current-epoch bound injury is accepted after real load"),F.Survival->ReceiveDamage(1,FGuid::NewGuid(),F.Storage->GetTimelineEpoch())))return false;
    TestEqual(TEXT("Fresh post-load bound injury creates exactly one source"),FAudioFeedbackLifecycleAccess::Sources(F.Player),1);
    F.Presentation->EndPlay(EEndPlayReason::Destroyed);
    TestEqual(TEXT("EndPlay cleans the actual mapped audio component"),FAudioFeedbackLifecycleAccess::Sources(F.Player),0);
    return true;
}
#endif
