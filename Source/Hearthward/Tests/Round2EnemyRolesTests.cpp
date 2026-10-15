#include "../Campaign/HearthwardCampaignActor.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
struct FRound2EnemyTestAccess
{
    static bool Pending(const AHearthwardCampaignActor* A){return A->MeleeTarget.IsValid();}
    static bool Committed(const AHearthwardCampaignActor* A){return A->MeleeCommitted;}
    static void Ready(AHearthwardCampaignActor* A){A->AttackIn=0;A->DecisionIn=0;}
    static void SeedArrival(AHearthwardCampaignActor* A,FGuid Epoch)
    {A->NoisePoint=A->GetActorLocation();A->NoiseEpoch=Epoch;A->NoiseTravelRemaining=30;A->NoiseInspectRemaining=6;}
    static bool Investigating(const AHearthwardCampaignActor* A){return A->NoiseEpoch.IsValid();}
    static bool Inspect(AHearthwardCampaignActor* A,float Delta){return A->UpdateNoiseInvestigation(Delta);}
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2EnemyTest,"Hearthward.Round2.Enemies.EliteTimingIdentityAndCancellation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2EnemyTest::RunTest(const FString&)
{
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT{GEngine->DestroyWorldContext(World);World->DestroyWorld(false);};
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();
    TestEqual(TEXT("Original base garrison remains eighty"),Campaign->State.Total(),80);
    auto* Record=Campaign->Enemy(TEXT("home_workshops_18"));
    if(!TestNotNull(TEXT("Elite is an existing soldier"),Record))return false;
    const float Health=Record->Combat.Health;
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Player=World->SpawnActor<ACharacter>(FVector(150,0,80),FRotator::ZeroRotator,Params);
    auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
    Record->Home=FVector(0,0,80);Record->Combat.Position=Record->Home;Record->Located=true;
    auto* Elite=World->SpawnActor<AHearthwardCampaignActor>(Record->Home,FRotator::ZeroRotator,Params);
    Elite->Initialize(Record->Id,true);Elite->Target->Memory.Seen.Add(TEXT("player"));
    TestEqual(TEXT("Elite keeps original health"),Elite->Target->Health,Health);
    TestEqual(TEXT("Elite keeps original reward kind"),Elite->Target->RewardKind,FName(TEXT("heavy")));
    const auto Timing=HearthwardEnemyRole::Attack(Elite->Identity,TEXT("heavy"));
    TestEqual(TEXT("Approved windup"),Timing.Windup,1.1f);TestEqual(TEXT("Approved active"),Timing.Active,.2f);TestEqual(TEXT("Approved recovery"),Timing.Recovery,1.3f);
    Elite->Tick(.01f);TestTrue(TEXT("Real actor starts telegraphed attack"),FRound2EnemyTestAccess::Pending(Elite));
    Elite->Tick(1.f);TestFalse(TEXT("Windup cannot commit a hit"),FRound2EnemyTestAccess::Committed(Elite));
    Elite->Tick(.21f);TestTrue(TEXT("Contact resolves in active segment"),FRound2EnemyTestAccess::Committed(Elite));
    Elite->Tick(.5f);TestTrue(TEXT("Recovery keeps a counter window"),FRound2EnemyTestAccess::Pending(Elite));
    Elite->Tick(1.f);TestFalse(TEXT("Attack eventually releases its slot"),FRound2EnemyTestAccess::Pending(Elite));
    FRound2EnemyTestAccess::Ready(Elite);Elite->Tick(.01f);Elite->Target->Memory.HitRemaining=1;Elite->Tick(.1f);
    TestFalse(TEXT("Hit interruption cancels pending contact"),FRound2EnemyTestAccess::Pending(Elite));
    Elite->Target->Memory.HitRemaining=0;FRound2EnemyTestAccess::Ready(Elite);Elite->Tick(.01f);
    World->GetSubsystem<UHearthwardStorageSubsystem>()->AdvanceTimeline();Elite->Tick(.2f);
    TestFalse(TEXT("Old epoch cannot finish elite attack"),FRound2EnemyTestAccess::Pending(Elite));
    Record->Combat.Health=0;Record->Combat.bStunned=true;
    FHearthwardCampaignState Restored;
    TestTrue(TEXT("Actual campaign snapshot restores permanent elimination"),FHearthwardCampaignState::Parse(Campaign->State.Snapshot(),Restored));
    const auto* Dead=Restored.Enemies.FindByPredicate([](const auto& E){return E.Id==TEXT("home_workshops_18");});
    TestTrue(TEXT("Same elite stays cleared without extra population"),Dead && Dead->Combat.Health==0 && Dead->Combat.bStunned && Restored.Total()==80);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2NoiseBoundaryTest,"Hearthward.Round2.Enemies.NoiseArrivalEpochAndConfirmedThreat",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2NoiseBoundaryTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT{GEngine->DestroyWorldContext(World);World->DestroyWorld(false);};
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();
    auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();
    auto* Record=Campaign->Enemy(TEXT("home_workshops_18"));
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Record->Home=FVector(0,0,80);Record->Combat.Position=Record->Home;Record->Located=true;
    auto* Enemy=World->SpawnActor<AHearthwardCampaignActor>(Record->Home,FRotator::ZeroRotator,Params);Enemy->Initialize(Record->Id,true);
    auto* Player=World->SpawnActor<ACharacter>(FVector(1000,0,80),FRotator::ZeroRotator,Params);
    auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
    TestTrue(TEXT("Unaware living soldier may investigate"),Enemy->CanInvestigateNoise());
    // Isolated arrival fixture: navigation is deliberately outside this test's scope.
    FRound2EnemyTestAccess::SeedArrival(Enemy,Storage->GetTimelineEpoch());
    TestFalse(TEXT("Active investigation cannot accept another distraction"),Enemy->CanInvestigateNoise());
    TestTrue(TEXT("Inspection still active before six seconds"),FRound2EnemyTestAccess::Inspect(Enemy,5.9f));
    TestTrue(TEXT("Sound alone grants no player knowledge"),Enemy->Target->Memory.Seen.IsEmpty() && Enemy->Target->Memory.LastKnown.IsEmpty());
    TestFalse(TEXT("Inspection expires after six seconds"),FRound2EnemyTestAccess::Inspect(Enemy,.11f));
    FRound2EnemyTestAccess::SeedArrival(Enemy,Storage->GetTimelineEpoch());Storage->AdvanceTimeline();
    TestFalse(TEXT("Old timeline cannot resume investigation"),FRound2EnemyTestAccess::Inspect(Enemy,.01f));
    FRound2EnemyTestAccess::SeedArrival(Enemy,Storage->GetTimelineEpoch());Enemy->Target->Memory.Seen.Add(TEXT("player"));
    FRound2EnemyTestAccess::Ready(Enemy);Enemy->Tick(.01f);
    TestFalse(TEXT("Confirmed threat cancels distraction"),FRound2EnemyTestAccess::Investigating(Enemy));
    TestTrue(TEXT("Confirmed threat remains known"),Enemy->Target->Memory.Seen.Contains(TEXT("player")));
    TestFalse(TEXT("Confirmed combat cannot be repeatedly distracted"),Enemy->CanInvestigateNoise());
    TestFalse(TEXT("Old source callback is rejected"),Enemy->BeginNoiseInvestigation(Player->GetActorLocation(),FGuid::NewGuid()));
    return true;
}
#endif
