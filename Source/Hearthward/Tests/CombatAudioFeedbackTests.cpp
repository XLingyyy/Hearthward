#include "../Combat/HearthwardCombatComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Audio.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWaveProcedural.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FCombatFeedbackWorld
{
    UWorld* World;
    ACharacter* Player;
    UHearthwardInventoryComponent* Bag;
    UHearthwardGameplayComponent* Gameplay;
    UHearthwardSurvivalComponent* Survival;
    UHearthwardCombatComponent* Combat;
    UHearthwardStorageSubsystem* Storage;
    UHearthwardWorldClockSubsystem* Clock;
    FCombatFeedbackWorld()
    {
        World=UWorld::CreateWorld(EWorldType::Game,false);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Player=World->SpawnActor<ACharacter>();
        Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
        Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
        Combat=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
        Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
        Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        auto* Controller=World->SpawnActor<APlayerController>();Controller->Possess(Player);
        World->AddController(Controller);
        Bag->TryAdd(TEXT("axe"),1);Gameplay->CommitEquipment(TEXT("axe"));
    }
    ~FCombatFeedbackWorld()
    {GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}
    UHearthwardCombatTargetComponent* Target(FName Id,FVector Position)
    {
        auto* Actor=World->SpawnActor<AActor>();
        auto* Root=NewObject<USceneComponent>(Actor);Actor->AddInstanceComponent(Root);Actor->SetRootComponent(Root);Root->RegisterComponent();Actor->SetActorLocation(Position);
        auto* Target=NewObject<UHearthwardCombatTargetComponent>(Actor);Actor->AddInstanceComponent(Target);Target->RegisterComponent();
        Target->Id=Id;Target->Health=Target->MaximumHealth=100;Gameplay->Opponents.Add(Id,100);return Target;
    }
};

struct FPublicAttackWorld
{
    UGameInstance* Instance;
    UWorld* World;
    ACharacter* Player;
    UHearthwardInventoryComponent* Bag;
    UHearthwardGameplayComponent* Gameplay;
    UHearthwardCombatComponent* Combat;
    UHearthwardPresentationComponent* Presentation;
    UHearthwardStorageSubsystem* Storage;
    UHearthwardWorldClockSubsystem* Clock;
    FPublicAttackWorld()
    {
        Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();World=Instance->GetWorld();
        Player=World->SpawnActor<ACharacter>();Player->GetCapsuleComponent()->SetCapsuleSize(34,90);Player->SetActorLocation(FVector(0,0,90));
        Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
        auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
        Combat=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
        auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
        auto* Local=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(Local,FPlatformUserId::CreateFromInternalId(0));Controller->SetPlayer(Local);
        auto* Ground=World->SpawnActor<AActor>();auto* Floor=NewObject<UBoxComponent>(Ground);Ground->AddInstanceComponent(Floor);Ground->SetRootComponent(Floor);
        Floor->SetBoxExtent(FVector(1000,1000,10));Floor->SetCollisionObjectType(ECC_WorldStatic);Floor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Floor->SetCollisionResponseToAllChannels(ECR_Block);Floor->RegisterComponent();Ground->SetActorLocation(FVector(0,0,-10));
        Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
        Presentation=NewObject<UHearthwardPresentationComponent>(Player);Player->AddInstanceComponent(Presentation);Presentation->RegisterComponent();
        World->InitializeActorsForPlay(FURL(),false);World->BeginPlay();World->GetWorldSettings()->NotifyBeginPlay();World->Tick(LEVELTICK_All,.025f);
        for(int32 Step=0;Step<80 && !Player->GetCharacterMovement()->CurrentFloor.IsWalkableFloor();++Step)
            Player->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
    }
    ~FPublicAttackWorld()
    {
        if(World->HasBegunPlay())World->EndPlay(EEndPlayReason::Quit);
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    void Advance(float Seconds)
    {Clock->Tick(Seconds);Combat->TickComponent(Seconds,LEVELTICK_All,nullptr);}
    TArray<UAudioComponent*> Sources() const
    {
        TArray<UAudioComponent*> Components;Player->GetComponents(Components);
        return Components.FilterByPredicate([](const auto* C){return IsValid(C) && C->IsRegistered() && C->ComponentTags.Contains(TEXT("Hearthward.Audio.effects"));});
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatRealHitReceipts099Test,"Hearthward.Iteration.Task099.Combat.RealHitReceipts",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatRealHitReceipts099Test::RunTest(const FString&)
{
    FCombatFeedbackWorld F;TArray<FHearthwardCombatFeedbackReceipt> Receipts;
    const FDelegateHandle Handle=F.Combat->OnCombatSucceeded.AddLambda([&](const auto& Receipt){Receipts.Add(Receipt);});
    auto* First=F.Target(TEXT("audio_hit_first"),FVector(100,0,0));
    auto* Second=F.Target(TEXT("audio_hit_second"),FVector(150,20,0));
    const FGuid Operation=FGuid::NewGuid();
    F.Combat->HitTarget(First,10,TEXT("body"),false,Operation);
    TestEqual(TEXT("Real hit commits target health before its receipt"),First->Health,90.f);
    TestEqual(TEXT("Real committed target hit publishes once"),Receipts.Num(),1);
    if(Receipts.Num()==1)
    {
        TestTrue(TEXT("Hit has its own valid success identity"),Receipts[0].SuccessId.IsValid());
        TestTrue(TEXT("Hit preserves its operation and epoch"),Receipts[0].OperationId==Operation && Receipts[0].Epoch==F.Storage->GetTimelineEpoch());
        TestTrue(TEXT("Hit identifies the actual committed target position"),Receipts[0].Kind==TEXT("hit") && Receipts[0].TargetPosition==First->GetOwner()->GetActorLocation());
    }
    F.Combat->HitTarget(First,10,TEXT("body"),false,Operation);
    TestEqual(TEXT("Repeated operation does not damage twice"),First->Health,90.f);
    TestEqual(TEXT("Repeated operation does not publish another success"),Receipts.Num(),1);
    F.Combat->HitTarget(Second,10,TEXT("body"),false,Operation);
    TestEqual(TEXT("One operation may really hit another target"),Second->Health,90.f);
    TestEqual(TEXT("Both targets of one action retain their legal feedback"),Receipts.Num(),2);
    if(Receipts.Num()==2)
        TestTrue(TEXT("Different target commits have distinct success identities but the same operation"),Receipts[0].SuccessId!=Receipts[1].SuccessId && Receipts[1].OperationId==Operation);
    F.Combat->HitTarget(First,10,TEXT("body"),true,FGuid::NewGuid());
    TestEqual(TEXT("A separate legal hit has its own feedback"),Receipts.Num(),3);
    First->Protected=true;F.Combat->HitTarget(First,10,TEXT("body"),false,FGuid::NewGuid());
    F.Combat->HitTarget(Second,10,NAME_None,false,FGuid::NewGuid());
    TestEqual(TEXT("Protected and unmapped contacts are not hit successes"),Receipts.Num(),3);
    if(TestTrue(TEXT("Public attack can start before cancellation"),F.Combat->Attack()))F.Combat->Cancel();
    TestEqual(TEXT("Cancelled windup does not publish a hit"),Receipts.Num(),3);
    F.Combat->OnCombatSucceeded.Remove(Handle);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatRealGuardReceipts099Test,"Hearthward.Iteration.Task099.Combat.RealGuardReceipts",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatRealGuardReceipts099Test::RunTest(const FString&)
{
    FCombatFeedbackWorld F;TArray<FHearthwardCombatFeedbackReceipt> Blocks,Damages;
    const FDelegateHandle CombatHandle=F.Combat->OnCombatSucceeded.AddLambda([&](const auto& Receipt){Blocks.Add(Receipt);});
    const FDelegateHandle DamageHandle=F.Survival->OnDamageSucceeded.AddLambda([&](const auto& Receipt){Damages.Add(Receipt);});
    if(!TestTrue(TEXT("Real shield can be equipped"),F.Bag->TryAdd(TEXT("shield"),1)==EHearthwardInventoryResult::Success && F.Gameplay->CommitEquipment(TEXT("shield"))))return false;
    if(!TestTrue(TEXT("Existing public guard starts"),F.Combat->SetGuard(true)))return false;
    F.Clock->Tick(.16f);const FGuid Operation=FGuid::NewGuid();
    TestTrue(TEXT("Actual guard commits a blocked hit"),F.Combat->Damage(10,TEXT("body"),FVector(100,0,0),false,false,Operation));
    TestEqual(TEXT("Block leaves real health untouched"),F.Gameplay->Health,100.f);
    TestEqual(TEXT("Successful guard publishes one block receipt"),Blocks.Num(),1);
    TestEqual(TEXT("Block is not a damage success"),Damages.Num(),0);
    if(Blocks.Num()==1)
        TestTrue(TEXT("Block identity, operation, epoch and target position are real"),Blocks[0].SuccessId.IsValid() && Blocks[0].OperationId==Operation && Blocks[0].Epoch==F.Storage->GetTimelineEpoch() && Blocks[0].Kind==TEXT("block") && Blocks[0].TargetPosition==F.Player->GetActorLocation());
    TestFalse(TEXT("Duplicate blocked operation is refused"),F.Combat->Damage(10,TEXT("body"),FVector(100,0,0),false,false,Operation));
    TestEqual(TEXT("Duplicate block has no second receipt"),Blocks.Num(),1);
    TestTrue(TEXT("New independent block is still legal"),F.Combat->Damage(10,TEXT("body"),FVector(100,0,0),false,false,FGuid::NewGuid()));
    TestEqual(TEXT("New legal block is not globally throttled"),Blocks.Num(),2);
    F.Combat->SetGuard(false);F.Gameplay->Stamina=100;
    if(TestTrue(TEXT("Public dodge starts after guard release"),F.Combat->Dodge(FVector::ForwardVector)))
    {
        TestFalse(TEXT("Early dodge rejects the incoming hit"),F.Combat->Damage(10,TEXT("body"),FVector(-100,0,0),false,false,FGuid::NewGuid()));
        TestEqual(TEXT("Dodged contact is not block feedback"),Blocks.Num(),2);
        TestEqual(TEXT("Dodged contact is not damage feedback"),Damages.Num(),0);
    }
    F.Combat->OnCombatSucceeded.Remove(CombatHandle);F.Survival->OnDamageSucceeded.Remove(DamageHandle);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatRealDamageReceipts099Test,"Hearthward.Iteration.Task099.Combat.RealDamageReceipts",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatRealDamageReceipts099Test::RunTest(const FString&)
{
    FCombatFeedbackWorld F;TArray<FHearthwardCombatFeedbackReceipt> PlayerReceipts,BrotherReceipts;
    auto* Brother=F.World->SpawnActor<AHearthwardCompanionFixture>(FVector(500,0,80),FRotator::ZeroRotator);
    auto* BrotherSurvival=Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!TestTrue(TEXT("Fixture registers the possessed player for the public player lookup"),UGameplayStatics::GetPlayerPawn(F.World,0)==F.Player)
        || !TestTrue(TEXT("Brother survival sees the enabled player gameplay"),BrotherSurvival->Enabled()))return false;
    const FDelegateHandle PlayerHandle=F.Survival->OnDamageSucceeded.AddLambda([&](const auto& Receipt){PlayerReceipts.Add(Receipt);});
    const FDelegateHandle BrotherHandle=BrotherSurvival->OnDamageSucceeded.AddLambda([&](const auto& Receipt){BrotherReceipts.Add(Receipt);});
    const FGuid PlayerOperation=FGuid::NewGuid(),BrotherOperation=FGuid::NewGuid();
    TestTrue(TEXT("Public DamageActor commits real player injury"),UHearthwardCombatComponent::DamageActor(F.Player,10,TEXT("body"),FVector(-100,0,0),false,false,PlayerOperation));
    TestTrue(TEXT("Public DamageActor commits real brother injury"),UHearthwardCombatComponent::DamageActor(Brother,10,TEXT("body"),FVector(-100,0,0),false,false,BrotherOperation));
    TestEqual(TEXT("Player injury is actually settled"),F.Gameplay->Health,90.f);
    TestEqual(TEXT("Brother injury is actually settled"),BrotherSurvival->Health(),90.f);
    TestEqual(TEXT("Player injury publishes one success"),PlayerReceipts.Num(),1);
    TestEqual(TEXT("Brother injury publishes one success"),BrotherReceipts.Num(),1);
    if(PlayerReceipts.Num()==1 && BrotherReceipts.Num()==1)
    {
        TestTrue(TEXT("Each actor damage has a valid distinct success identity"),PlayerReceipts[0].SuccessId.IsValid() && BrotherReceipts[0].SuccessId.IsValid() && PlayerReceipts[0].SuccessId!=BrotherReceipts[0].SuccessId);
        TestTrue(TEXT("Player damage retains its exact operation and settled target"),PlayerReceipts[0].OperationId==PlayerOperation && PlayerReceipts[0].Kind==TEXT("damage") && PlayerReceipts[0].TargetPosition==F.Player->GetActorLocation());
        TestTrue(TEXT("Brother damage retains its exact operation, epoch and settled target"),BrotherReceipts[0].OperationId==BrotherOperation && BrotherReceipts[0].Epoch==F.Storage->GetTimelineEpoch() && BrotherReceipts[0].Kind==TEXT("damage") && BrotherReceipts[0].TargetPosition==Brother->GetActorLocation());
    }
    TestFalse(TEXT("Repeated player operation is refused"),UHearthwardCombatComponent::DamageActor(F.Player,10,TEXT("body"),FVector(-100,0,0),false,false,PlayerOperation));
    TestFalse(TEXT("Repeated brother operation is refused"),UHearthwardCombatComponent::DamageActor(Brother,10,TEXT("body"),FVector(-100,0,0),false,false,BrotherOperation));
    const FGuid OldEpoch=F.Storage->GetTimelineEpoch();F.Storage->AdvanceTimeline();
    TestFalse(TEXT("Old-epoch direct damage cannot settle"),BrotherSurvival->ReceiveDamage(10,FGuid::NewGuid(),OldEpoch));
    TestFalse(TEXT("Invalid event cannot settle"),F.Survival->ReceiveDamage(10,FGuid(),F.Storage->GetTimelineEpoch()));
    TestFalse(TEXT("Nonpositive damage cannot settle"),F.Survival->ReceiveDamage(0,FGuid::NewGuid(),F.Storage->GetTimelineEpoch()));
    TestEqual(TEXT("Rejected operations do not publish player feedback"),PlayerReceipts.Num(),1);
    TestEqual(TEXT("Rejected operations do not publish brother feedback"),BrotherReceipts.Num(),1);
    F.Combat->Restore(F.Combat->Snapshot());BrotherSurvival->ResetTransient();
    TestEqual(TEXT("Restoring current state does not replay damage history"),PlayerReceipts.Num()+BrotherReceipts.Num(),2);
    TestTrue(TEXT("Fresh current-epoch damage is legal after transient reset"),BrotherSurvival->ReceiveDamage(1,FGuid::NewGuid(),F.Storage->GetTimelineEpoch()));
    TestEqual(TEXT("Fresh legal damage publishes its own feedback"),BrotherReceipts.Num(),2);
    F.Survival->OnDamageSucceeded.Remove(PlayerHandle);BrotherSurvival->OnDamageSucceeded.Remove(BrotherHandle);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatActualEmptySwing099Test,"Hearthward.Iteration.Task099.Combat.ActualEmptySwing",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatActualEmptySwing099Test::RunTest(const FString&)
{
    FPublicAttackWorld F;
    if(!TestTrue(TEXT("Actual local player begins on a physics-confirmed walkable floor"),F.World->HasBegunPlay()
        && UGameplayStatics::GetPlayerPawn(F.World,0)==F.Player && F.Player->IsLocallyControlled()
        && F.Player->GetCharacterMovement()->IsMovingOnGround() && F.Player->GetCharacterMovement()->CurrentFloor.IsWalkableFloor()))return false;
    TArray<FHearthwardCombatFeedbackReceipt> Swings;int32 OtherReceipts=0;
    const FDelegateHandle Handle=F.Combat->OnCombatSucceeded.AddLambda([&](const auto& Receipt)
    {if(Receipt.Kind==TEXT("swing"))Swings.Add(Receipt);else ++OtherReceipts;});
    const auto InitialSources=F.Sources();
    TestFalse(TEXT("Unarmed public attack is rejected without a swing"),F.Combat->Attack());
    TestEqual(TEXT("Rejected unarmed attack creates no audio source"),F.Sources().Num(),InitialSources.Num());
    TArray<uint8> CueData;FWaveModInfo CueInfo;
    if(!TestTrue(TEXT("Actual bound knifeSlice candidate is a real mono PCM16 file"),FFileHelper::LoadFileToArray(CueData,*(FPaths::ProjectDir()/TEXT("Resources/Audio/TASK-099/candidate-knifeSlice.wav")))
        && CueInfo.ReadWaveInfo(CueData.GetData(),CueData.Num()) && *CueInfo.pFormatTag==1 && *CueInfo.pBitsPerSample==16 && *CueInfo.pChannels==1 && CueInfo.SampleDataSize>0))return false;
    if(!TestTrue(TEXT("Two real shortblade instances are added and one is equipped"),F.Bag->TryAdd(TEXT("shortblade"),2)==EHearthwardInventoryResult::Success
        && F.Gameplay->CommitEquipment(TEXT("shortblade"))))return false;
    const FGuid FirstWeapon=F.Bag->EquippedInstance(TEXT("weapon")),SecondWeapon=F.Bag->FirstInstance(TEXT("shortblade"),true);
    if(!TestTrue(TEXT("Instance-change case has two distinct real inventory identities"),FirstWeapon.IsValid() && SecondWeapon.IsValid() && FirstWeapon!=SecondWeapon))return false;
    const auto Move=HearthwardCombat::Move(TEXT("shortblade"),false);
    if(!TestTrue(TEXT("Public attack accepts the grounded equipped player"),F.Combat->Attack()))return false;
    TestTrue(TEXT("Accepted attack exposes its actual action and move duration"),F.Combat->Action==TEXT("attack") && FMath::IsNearlyEqual(F.Combat->Duration,Move.Duration()));
    TestFalse(TEXT("Busy public attack cannot start a second swing"),F.Combat->Attack());
    TestEqual(TEXT("Accepted and rejected windups have no attack audio"),F.Sources().Num(),InitialSources.Num());
    F.Advance(float(Move.Windup*.5));TestEqual(TEXT("Accepted windup has no swing receipt"),Swings.Num(),0);
    F.Advance(float(Move.Windup*.5+Move.Active*.25));
    TestTrue(TEXT("Real clock and public component tick enter the active strike window"),F.Combat->Action==TEXT("attack")
        && F.Combat->Elapsed>Move.Windup && F.Combat->Elapsed<Move.Windup+Move.Active);
    TestEqual(TEXT("An actual active empty swing publishes one receipt"),Swings.Num(),1);
    const auto ActiveSources=F.Sources();TestEqual(TEXT("Actual swing producer creates one bound effects component"),ActiveSources.Num(),InitialSources.Num()+1);
    auto* const* Created=ActiveSources.FindByPredicate([&](const auto* C){return !InitialSources.Contains(C);});
    if(TestNotNull(TEXT("Actual consumer registers the new swing audio component"),Created))
    {
        auto* Source=*Created;auto* Sound=Cast<USoundWaveProcedural>(Source->GetSound());
        if(TestNotNull(TEXT("Actual swing component owns a real procedural PCM sound"),Sound))
            TestTrue(TEXT("Actual sound duration and channels match the bound knifeSlice PCM"),Sound->NumChannels==*CueInfo.pChannels
                && FMath::IsNearlyEqual(Sound->Duration,CueInfo.SampleDataSize/float(*CueInfo.pSamplesPerSec*2),.00001f) && !Sound->bLooping);
        TestTrue(TEXT("Actual spatial swing source belongs to the player at the committed owner position"),Source->GetOwner()==F.Player
            && Source->bAllowSpatialization && !Source->bIsUISound && Source->GetComponentLocation().Equals(F.Player->GetActorLocation(),.01f));
    }
    if(Swings.Num()==1)
        TestTrue(TEXT("Swing preserves current epoch, unique success and action identity at the real source position"),Swings[0].Epoch==F.Storage->GetTimelineEpoch()
            && Swings[0].SuccessId.IsValid() && Swings[0].OperationId.IsValid() && Swings[0].TargetPosition==F.Player->GetActorLocation());
    F.Advance(float(Move.Active*.25));TestEqual(TEXT("A second tick in the same strike cannot repeat its swing"),Swings.Num(),1);
    TestEqual(TEXT("Repeated active tick cannot create another swing sound"),F.Sources().Num(),InitialSources.Num()+1);
    F.Advance(float(Move.Duration()));TestFalse(TEXT("Actual empty attack completes through the normal recovery"),F.Combat->Busy());
    TestEqual(TEXT("Recovery does not publish another swing"),Swings.Num(),1);
    TestEqual(TEXT("Completed recovery cannot replay its attack sound"),F.Sources().Num(),InitialSources.Num()+1);
    if(!TestTrue(TEXT("A new public attack is independently accepted"),F.Combat->Attack()))return false;
    F.Advance(float(Move.Duration()+.01));TestEqual(TEXT("Crossing the whole active window in one real tick still publishes once"),Swings.Num(),2);
    TestEqual(TEXT("Independent real attack creates exactly one independent sound"),F.Sources().Num(),InitialSources.Num()+2);
    if(Swings.Num()==2)
        TestTrue(TEXT("Different accepted attacks have distinct operation and success identities"),Swings[0].OperationId!=Swings[1].OperationId && Swings[0].SuccessId!=Swings[1].SuccessId);
    const int32 Committed=Swings.Num(),CommittedSources=F.Sources().Num();
    if(!TestTrue(TEXT("Cancellation case starts a real public attack"),F.Combat->Attack()))return false;
    F.Advance(float(Move.Windup*.5));F.Combat->Cancel();F.Advance(float(Move.Duration()));
    TestEqual(TEXT("Cancelled windup never publishes an active swing"),Swings.Num(),Committed);
    TestEqual(TEXT("Cancelled windup creates no swing sound"),F.Sources().Num(),CommittedSources);
    if(!TestTrue(TEXT("Pause case starts a real public attack"),F.Combat->Attack()))return false;
    F.Advance(float(Move.Windup*.5));const double BeforePause=F.Combat->Elapsed;
    F.World->GetWorldSettings()->SetPauserPlayerState(F.World->SpawnActor<APlayerState>());
    TestTrue(TEXT("Public pauser puts the world and combat clock in the actual paused state"),F.World->IsPaused() && F.Clock->Suspended());
    F.Advance(float(Move.Duration()));TestEqual(TEXT("Paused attack does not advance its strike time"),F.Combat->Elapsed,BeforePause);
    TestEqual(TEXT("Paused windup has no swing receipt"),Swings.Num(),Committed);TestFalse(TEXT("Paused public attack is rejected"),F.Combat->Attack());
    TestEqual(TEXT("Paused real attack does not create audio"),F.Sources().Num(),CommittedSources);
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);F.Combat->Cancel();F.Advance(float(Move.Duration()));
    TestEqual(TEXT("Cancelling the resumed windup cannot replay paused contact"),Swings.Num(),Committed);
    TestEqual(TEXT("Cancelled resumed windup cannot create late audio"),F.Sources().Num(),CommittedSources);
    if(!TestTrue(TEXT("Epoch case starts a real public attack"),F.Combat->Attack()))return false;
    F.Advance(float(Move.Windup*.5));F.Storage->AdvanceTimeline();F.Presentation->TickComponent(0,LEVELTICK_All,nullptr);F.Advance(float(Move.Duration()));
    TestFalse(TEXT("Timeline change cancels the old attack"),F.Combat->Busy());TestEqual(TEXT("Old-epoch attack has no swing receipt"),Swings.Num(),Committed);
    TestEqual(TEXT("Real timeline invalidation clears prior audio and cannot replay the old attack"),F.Sources().Num(),0);
    if(!TestTrue(TEXT("Instance case starts a real public attack"),F.Combat->Attack()))return false;
    F.Advance(float(Move.Windup*.5));
    TestTrue(TEXT("Real public equipment commit replaces the weapon instance"),F.Gameplay->CommitEquipmentInstance(SecondWeapon) && F.Bag->EquippedInstance(TEXT("weapon"))==SecondWeapon);
    F.Advance(float(Move.Duration()));TestFalse(TEXT("Changed real weapon instance cancels the original attack"),F.Combat->Busy());
    TestEqual(TEXT("Changed-instance windup has no swing receipt"),Swings.Num(),Committed);
    TestEqual(TEXT("Changed-instance attack creates no swing audio"),F.Sources().Num(),0);
    if(!TestTrue(TEXT("Fresh current-epoch attack uses the replacement real instance"),F.Combat->Attack()))return false;
    F.Advance(float(Move.Windup+Move.Active*.25));TestEqual(TEXT("Fresh legal attack is not suppressed by earlier rejected actions"),Swings.Num(),Committed+1);
    TestEqual(TEXT("Fresh current-epoch real attack creates one new bound sound"),F.Sources().Num(),1);
    if(Swings.Num()==Committed+1)TestTrue(TEXT("Fresh swing belongs to the new timeline"),Swings.Last().Epoch==F.Storage->GetTimelineEpoch());
    TestEqual(TEXT("Empty world never disguises its swing as a hit or block"),OtherReceipts,0);
    F.Combat->OnCombatSucceeded.Remove(Handle);return true;
}
#endif
