#include "../Combat/HearthwardCombatComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatRulesTest,"Hearthward.Combat.GuardDetectionAndParts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatRulesTest::RunTest(const FString&)
{
    HearthwardCombat::FGuard Guard; float Stamina=1;
    TestTrue(TEXT("Positive stamina raises shield"),Guard.Raise(0,Stamina,100));
    TestFalse(TEXT("Raise windup cannot block"),Guard.Hit(.149,20,Stamina));
    TestTrue(TEXT("First hit fully blocks with insufficient stamina"),Guard.Hit(.15,20,Stamina));
    TestEqual(TEXT("Break drains to zero"),Stamina,0.f);
    TestFalse(TEXT("Independent second hit passes broken shield"),Guard.Hit(.15,20,Stamina));
    TestFalse(TEXT("Holding button prevents re-raise"),Guard.Raise(2,100,100));
    Guard.Release(); TestFalse(TEXT("Below recovery threshold"),Guard.Raise(2,19,100));
    TestTrue(TEXT("Recovered and released re-raises"),Guard.Raise(2,20,100));
    TestFalse(TEXT("Rear is outside guard"),HearthwardCombat::InFront(FVector::ForwardVector,-FVector::ForwardVector));
    TestEqual(TEXT("One observer needs five seconds at ten meters"),HearthwardCombat::Detection(0,5,10,true),1.);
    TestEqual(TEXT("No sight decays over ten seconds"),HearthwardCombat::Detection(1,10,10,false),0.);
    TestEqual(TEXT("Only struck part reduces damage"),HearthwardCombat::ArmorDamage(100,.2f),80.f);
    TestTrue(TEXT("Reduction caps at eighty-five percent"),FMath::IsNearlyEqual(HearthwardCombat::ArmorDamage(100,1),15.f,.0001f));
    TestFalse(TEXT("Sense excludes next floor"),HearthwardCombat::SenseVisible(FVector::ZeroVector,FVector(0,0,401)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatActionsTest,"Hearthward.Combat.ExecutionActionsAndSnapshot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatActionsTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Player=World->SpawnActor<ACharacter>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player); Player->AddInstanceComponent(Bag); Bag->RegisterComponent();
    auto* G=NewObject<UHearthwardGameplayComponent>(Player); Player->AddInstanceComponent(G); G->RegisterComponent(); G->Enabled=true;
    auto* S=NewObject<UHearthwardSurvivalComponent>(Player); Player->AddInstanceComponent(S); S->RegisterComponent();
    auto* C=NewObject<UHearthwardCombatComponent>(Player); Player->AddInstanceComponent(C); C->RegisterComponent();
    Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    Bag->TryAdd(TEXT("axe"),1); G->CommitEquipment(TEXT("axe"));
    auto* Enemy=World->SpawnActor<AActor>();
    auto* Root=NewObject<USceneComponent>(Enemy); Enemy->AddInstanceComponent(Root); Enemy->SetRootComponent(Root); Root->RegisterComponent();
    Enemy->SetActorLocation(FVector(100,0,0));
    auto* T=NewObject<UHearthwardCombatTargetComponent>(Enemy); Enemy->AddInstanceComponent(T); T->RegisterComponent();
    T->Id=TEXT("combat_test"); T->Health=T->MaximumHealth=100; G->Opponents.Add(T->Id,T->Health);
    auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
    auto Advance=[&](float Delta) { Clock->Tick(Delta); C->TickComponent(Delta,LEVELTICK_All,nullptr); };
    G->Stamina=0;
    TestTrue(TEXT("Execution accepts zero stamina"),C->Execute(Enemy));
    TestFalse(TEXT("Claimed target cannot retaliate"),T->CanAct());
    TestFalse(TEXT("Repeated request cannot start another execution"),C->Execute(Enemy));
    TestFalse(TEXT("Incomplete execution cannot be saved"),C->CanSave());
    Advance(2.99f); TestEqual(TEXT("Before three seconds target is alive"),T->Health,100.f);
    Advance(.011f); TestEqual(TEXT("Three seconds completes execution"),T->Health,0.f);
    TestEqual(TEXT("Execution costs no stamina"),G->Stamina,0.f);
    const int32 Defeats=G->Events.FindRef(TEXT("defeat:combat_test"));
    TestEqual(TEXT("One defeat"),Defeats,1);
    C->HitTarget(T,100,TEXT("body"),false,FGuid::NewGuid());
    TestEqual(TEXT("Corpse cannot award defeat again"),G->Events.FindRef(TEXT("defeat:combat_test")),Defeats);
    C->State.SenseCooldown=12; C->State.Alarms.Add(TEXT("prototype"),150);
    T->Memory.BroadcastRegions.Add(TEXT("prototype"));
    const FString Snapshot=C->Snapshot();
    TestTrue(TEXT("Snapshot validates"),C->ValidateSnapshot(Snapshot));
    C->State={}; T->Memory.BroadcastRegions.Reset(); C->Restore(Snapshot);
    TestEqual(TEXT("Cooldown restores"),C->State.SenseCooldown,12.);
    TestTrue(TEXT("Corpse propagation receipt restores"),T->Memory.BroadcastRegions.Contains(TEXT("prototype")));
    TestTrue(TEXT("Legacy snapshot accepted"),C->ValidateSnapshot(TEXT("")));
    TestFalse(TEXT("Bad snapshot rejected"),C->ValidateSnapshot(TEXT("{")));
    T->Health=100; Enemy->SetActorRotation(FRotator::ZeroRotator); G->Opponents[T->Id]=100;
    TestTrue(TEXT("Next live target execution starts"),C->Execute(Enemy));
    C->Cancel(); TestTrue(TEXT("Cancel releases target without damage"),T->CanAct() && T->Health==100);
    T->Heavy=true; T->Health=90;
    TestTrue(TEXT("Heavy current-health equality allows execution"),C->Execute(Enemy)); C->Cancel();
    T->Health=90.1f; TestFalse(TEXT("Heavy current-health above threshold refuses"),C->Execute(Enemy)); T->Heavy=false;
    G->Stamina=100;
    TestTrue(TEXT("Heavy attack needs no strong skill unlock"),C->Attack(true));
    TestFalse(TEXT("Windup cannot dodge"),C->Dodge(FVector::ForwardVector));
    Advance(.851f); TestTrue(TEXT("Recovery can dodge"),C->Dodge(FVector::ForwardVector));
    TestFalse(TEXT("Dodge has early combat immunity"),C->Damage(10,TEXT("body"),FVector(-100,0,0)));
    Advance(.31f); TestTrue(TEXT("Dodge immunity ends at point three"),C->Damage(10,TEXT("body"),FVector(-100,0,0)));
    C->Cancel(); G->Stamina=100; C->State.SenseCooldown=0;
    TestTrue(TEXT("Sense starts"),C->Sense()); const float AfterSense=G->Stamina;
    TestFalse(TEXT("Sense cooldown refuses another input"),C->Sense()); TestEqual(TEXT("Refusal charges nothing"),G->Stamina,AfterSense);
    T->Health=100; T->Memory={}; T->Armor.Add(TEXT("legs"),.5); T->ArmorDurability.Add(TEXT("legs"),1);
    const FGuid Hit=FGuid::NewGuid(); C->HitTarget(T,10,TEXT("legs"),false,Hit); C->HitTarget(T,10,TEXT("legs"),false,Hit);
    TestEqual(TEXT("One event charges struck armor once"),T->Health,95.f);
    C->HitTarget(T,10,TEXT("legs"),false,FGuid::NewGuid()); TestEqual(TEXT("Next hit sees broken armor"),T->Health,85.f);
    T->Protected=true; C->HitTarget(T,1000,TEXT("head"),true,FGuid::NewGuid()); TestEqual(TEXT("Noncombatant is protected"),T->Health,85.f);
    // Observe two independent bars, then isolate one observer for the corpse propagation boundaries.
    T->Protected=false; T->Health=100; T->Memory={}; Enemy->SetActorLocation(FVector(1000,0,0)); Enemy->SetActorRotation(FRotator(0,180,0));
    Player->SetActorLocation(FVector::ZeroVector);
    auto* Second=World->SpawnActor<AActor>(); auto* SecondRoot=NewObject<USceneComponent>(Second);
    Second->AddInstanceComponent(SecondRoot); Second->SetRootComponent(SecondRoot); SecondRoot->RegisterComponent();
    Second->SetActorLocation(FVector(1000,0,0)); Second->SetActorRotation(FRotator(0,180,0));
    auto* O=NewObject<UHearthwardCombatTargetComponent>(Second); Second->AddInstanceComponent(O); O->RegisterComponent(); O->Id=TEXT("observer_two"); O->Health=100;
    Advance(2.5f); TestTrue(TEXT("Two observers do not sum their discovery"),FMath::IsNearlyEqual(C->Discovery,.5f));
    Advance(2.5f); TestEqual(TEXT("Both reach discovery at five seconds"),C->Discovery,1.f);
    Second->Destroy(); Player->SetActorLocation(FVector(-5000,0,0));
    auto* Corpse=World->SpawnActor<AActor>(); auto* CorpseRoot=NewObject<USceneComponent>(Corpse);
    Corpse->AddInstanceComponent(CorpseRoot); Corpse->SetRootComponent(CorpseRoot); CorpseRoot->RegisterComponent(); Corpse->SetActorLocation(FVector(500,0,0));
    auto* Dead=NewObject<UHearthwardCombatTargetComponent>(Corpse); Corpse->AddInstanceComponent(Dead); Dead->RegisterComponent(); Dead->Id=TEXT("body_test"); Dead->Health=0;
    C->State.Alarms.Reset(); Advance(.01f); Advance(2.99f);
    TestTrue(TEXT("Report not committed before three seconds"),C->State.Alarms.IsEmpty());
    C->ObserveDamage(T); TestEqual(TEXT("Positive hit interrupts report"),T->Memory.ReportRemaining,0.);
    Advance(.01f); Advance(3.f);
    TestTrue(TEXT("Completed report marks corpse"),Dead->Memory.BroadcastRegions.Contains(T->Region));
    const double Alarm=C->State.Alarms.FindRef(T->Region); Advance(4.f);
    TestEqual(TEXT("Same corpse cannot refresh alarm"),C->State.Alarms.FindRef(T->Region),Alarm);
    Player->SetActorLocation(FVector::ZeroVector); Advance(121.f);
    TestTrue(TEXT("Persistent sight prevents alarm expiry"),C->State.Alarms.Contains(T->Region));
    Player->SetActorLocation(FVector(-5000,0,0)); Advance(.1f);
    TestFalse(TEXT("Expired alarm clears when sight ends"),C->State.Alarms.Contains(T->Region));
    T->Memory.Detection.Reset(); T->Memory.LastKnown.Reset();
    C->ObserveDamage(T,Corpse);
    TestTrue(TEXT("Ally hit does not reveal player position"),T->Memory.Detection.Contains(TEXT("brother")) && !T->Memory.Detection.Contains(TEXT("player")));
    T->CreateBodyCollision();
    for(const int32 FPS:{1,30,120})
    {
        C->Cancel(); Player->SetActorLocation(FVector::ZeroVector); Player->SetActorRotation(FRotator::ZeroRotator);
        Enemy->SetActorLocation(FVector(100,0,0)); Enemy->SetActorRotation(FRotator::ZeroRotator);
        T->Health=100; T->Memory={}; T->DamageIds.Reset(); G->Stamina=100; G->Opponents[T->Id]=100;
        TestTrue(TEXT("Collision attack starts"),C->Attack(false));
        for(int32 Frame=0;Frame<FPS+1;++Frame) Advance(1.f/FPS);
        TestEqual(FString::Printf(TEXT("Real sweep damages once at %d fps"),FPS),T->Health,70.f);
        TestEqual(FString::Printf(TEXT("One stamina debit at %d fps"),FPS),G->Stamina,100.f-14*Bag->GetStaminaCostMultiplier());
    }
    Enemy->SetActorLocation(FVector(5000,0,0));
    auto* Wall=World->SpawnActor<AActor>(); auto* WallBox=NewObject<UBoxComponent>(Wall);
    Wall->AddInstanceComponent(WallBox); Wall->SetRootComponent(WallBox); WallBox->SetBoxExtent(FVector(10,100,100));
    WallBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly); WallBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    WallBox->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block); WallBox->RegisterComponent(); Wall->SetActorLocation(FVector(100,500,0));
    auto* Arrow=World->SpawnActor<AHearthwardProjectile>(FVector(0,500,10),FRotator::ZeroRotator);
    Arrow->Shooter=C; Arrow->Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    Arrow->Item=TEXT("arrow"); Arrow->Velocity=FVector(1000,0,0); Arrow->Gravity=100; Arrow->RemainingRange=1000;
    Arrow->Tick(.2f); TestTrue(TEXT("Wall stops real projectile"),Arrow->Landed && Arrow->GetActorLocation().X<100);
    const FString WithArrow=C->Snapshot(); C->Restore(WithArrow);
    AHearthwardProjectile* RestoredArrow=nullptr;
    for(TActorIterator<AHearthwardProjectile> It(World);It;++It) if(It->Item==TEXT("arrow")) RestoredArrow=*It;
    TestNotNull(TEXT("Recoverable arrow survives snapshot"),RestoredArrow);
    if(RestoredArrow)
    {
        Player->SetActorLocation(FVector(0,500,0)); const int32 Before=Bag->GetItemCount(TEXT("arrow"));
        TestTrue(TEXT("Missed arrow can be recovered"),RestoredArrow->Recover(Player));
        TestEqual(TEXT("Recovery adds exactly one arrow"),Bag->GetItemCount(TEXT("arrow")),Before+1);
        TestFalse(TEXT("Repeated recovery refuses duplicate"),RestoredArrow->Recover(Player));
    }
    GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return true;
}
#endif
