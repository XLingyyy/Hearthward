#include "../Environment/HearthwardShortcutActor.h"
#include "../Environment/HearthwardLampInteraction.h"
#include "../Environment/HearthwardNoiseInteraction.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Interaction/HearthwardInteractionAlignment.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2ShortcutTest,"Hearthward.Round2.Devices.SideCancellationCommitAndReload",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2ShortcutTest::RunTest(const FString&)
{
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Player=World->SpawnActor<ACharacter>(FVector(100,0,90),FRotator::ZeroRotator,Spawn);
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Game=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Game);Game->RegisterComponent();
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Gate=World->SpawnActor<AHearthwardShortcutActor>(FVector::ZeroVector,FRotator::ZeroRotator,Spawn);Gate->Identity=TEXT("workshops_inner_latch");
    Gate->Use(Player);TestFalse(TEXT("Outer side cannot start"),Gate->Active());
    Player->SetActorLocation(FVector(-100,0,90));Gate->Use(Player);TestTrue(TEXT("Inner side starts"),Gate->Active());
    Gate->Tick(.6f);TestFalse(TEXT("No early persistence"),Gate->IsOpen());
    Player->SetActorLocation(FVector(-210,0,90));Gate->Tick(.1f);TestFalse(TEXT("Movement cancels"),Gate->Active());TestFalse(TEXT("Cancellation keeps collision"),Gate->IsOpen());
    Player->SetActorLocation(FVector(-100,0,90));Gate->Use(Player);
    World->GetSubsystem<UHearthwardStorageSubsystem>()->AdvanceTimeline();Gate->Tick(2);
    TestFalse(TEXT("Old epoch cannot open"),Gate->IsOpen());
    Gate->Use(Player);Gate->Tick(1.3f);TestTrue(TEXT("Valid completion opens once"),Gate->IsOpen());
    TestEqual(TEXT("Collision follows committed state"),Gate->Barrier->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
    auto* Save=World->GetSubsystem<UHearthwardSaveSubsystem>();const auto Revision=Save->InteractionState().Revision;
    Gate->Use(Player);Gate->Tick(2);TestEqual(TEXT("Repeat cannot recommit"),Save->InteractionState().Revision,Revision);
    auto* Reloaded=World->SpawnActor<AHearthwardShortcutActor>(FVector(2000,0,0),FRotator::ZeroRotator,Spawn);Reloaded->Identity=Gate->Identity;
    TestTrue(TEXT("Recreated actor derives state from stable identity"),Reloaded->IsOpen());
    const auto& Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>()->State;
    TestTrue(TEXT("Opening does not claim flags or victory"),Campaign.Flags.IsEmpty() && !Campaign.Victory);
    auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>(FVector(-65,115,90),FRotator::ZeroRotator,Spawn);
    Brother->FindComponentByClass<UHearthwardInteractionAlignment>()->DestroyComponent();
    auto* Beam=World->SpawnActor<AHearthwardShortcutActor>(FVector::ZeroVector,FRotator::ZeroRotator,Spawn);
    Beam->Configure(TEXT("workshops_beam"),EHearthwardDeviceKind::Beam);
    Player->SetActorLocation(FVector(-65,-115,90));Beam->Use(Player);Beam->Tick(1.5f);
    TestTrue(TEXT("Beam moves during the shared three second action"),Beam->Mesh->GetRelativeLocation().Y>100);
    TestFalse(TEXT("Moving beam does not unlock route early"),Beam->IsOpen());
    Beam->Cancel();TestTrue(TEXT("Cancellation returns beam to support"),Beam->Mesh->GetRelativeLocation().Equals(FVector(0,0,100)));
    auto* Block=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Block);Block->AddInstanceComponent(Box);Block->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(30,30,60));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Block->SetActorLocation(FVector(0,340,60));
    Beam->Use(Player);Beam->Tick(.1f);TestFalse(TEXT("Occupied receiving support cancels"),Beam->Active());TestFalse(TEXT("Blocked beam cannot persist"),Beam->IsOpen());
    Block->Destroy();Beam->Use(Player);Beam->Tick(3.1f);TestTrue(TEXT("Clear beam commits after three seconds"),Beam->IsOpen());
    TestTrue(TEXT("Committed beam stays on receiving support"),Beam->Mesh->GetRelativeLocation().Equals(FVector(0,340,0)));
    auto* First=World->SpawnActor<AHearthwardShortcutActor>(FVector(3000,0,0),FRotator::ZeroRotator,Spawn);
    auto* Second=World->SpawnActor<AHearthwardShortcutActor>(FVector(4000,0,0),FRotator::ZeroRotator,Spawn);
    First->Configure(TEXT("workshops_winch"),EHearthwardDeviceKind::Winch);Second->Configure(TEXT("cellar_winch"),EHearthwardDeviceKind::Winch);
    Brother->SetActorLocation(FVector(3200,0,90));Player->SetActorLocation(First->Interaction->GetComponentLocation()+FVector(0,-100,0));
    const auto BeforeSwitch=Save->InteractionState().Revision;First->Use(Player);
    TestTrue(TEXT("First device owns the one companion session"),First->Active() && Brother->CooperationOwner.Get()==First);
    Player->SetActorLocation(Second->Interaction->GetComponentLocation()+FVector(0,-100,0));Second->Use(Player);
    TestFalse(TEXT("Second device cannot seize an active companion"),Second->Active());
    TestTrue(TEXT("Rejected second input preserves first owner"),Brother->CooperationOwner.Get()==First);
    First->Cancel();Second->Use(Player);TestTrue(TEXT("Cancellation allows the second device to acquire ownership"),Second->Active() && Brother->CooperationOwner.Get()==Second);
    First->Cancel();First->Tick(10);
    TestTrue(TEXT("Late work from old device cannot cancel the new owner"),Second->Active() && Brother->CooperationOwner.Get()==Second);
    World->GetSubsystem<UHearthwardStorageSubsystem>()->AdvanceTimeline();Second->Tick(.1f);
    TestFalse(TEXT("Timeline change releases active cooperation"),Second->Active() || Brother->CooperationOwner.IsValid());
    TestEqual(TEXT("Cancelled or stale sessions cannot commit device results"),Save->InteractionState().Revision,BeforeSwitch);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2LampTest,"Hearthward.Round2.Devices.LampPersistenceNoiseCooldownAndWhitelist",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2LampTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Player=World->SpawnActor<ACharacter>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Game=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Game);Game->RegisterComponent();
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Prop=World->SpawnActor<AActor>();
    auto* Lamp=NewObject<UHearthwardLampInteraction>(Prop);Prop->AddInstanceComponent(Lamp);Prop->SetRootComponent(Lamp);Lamp->RegisterComponent();Lamp->Identity=TEXT("cellar_lamp_entry");
    TestTrue(TEXT("Old save default is lit"),Lamp->IsLit());Lamp->CompleteInteraction(Player);TestFalse(TEXT("Whitelisted lamp toggles"),Lamp->IsLit());
    auto* Save=World->GetSubsystem<UHearthwardSaveSubsystem>();TestEqual(TEXT("Save owns exact lamp identity"),Save->InteractionState().Lamps[0].Id,Lamp->Identity);
    Lamp->Identity=TEXT("unregistered_lamp");Lamp->CompleteInteraction(Player);TestEqual(TEXT("Arbitrary lamp cannot enter snapshot"),Save->InteractionState().Lamps.Num(),1);
    auto* Noise=NewObject<UHearthwardNoiseInteraction>(Prop);Prop->AddInstanceComponent(Noise);Noise->SetupAttachment(Lamp);Noise->RegisterComponent();
    Noise->CompleteInteraction(Player);TestTrue(TEXT("A real use starts cooldown even without listeners"),Noise->RemainingCooldown()>9.9f);
    const auto Revision=Save->InteractionState().Revision;Noise->CompleteInteraction(Player);TestEqual(TEXT("Noise never creates persistent rewards"),Save->InteractionState().Revision,Revision);
    World->GetSubsystem<UHearthwardStorageSubsystem>()->AdvanceTimeline();TestEqual(TEXT("Readback epoch discards old cooldown"),Noise->RemainingCooldown(),0.f);
    return true;
}
#endif
