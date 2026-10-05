#include "../Companion/HearthwardCompanionFixture.h"
#include "../Companion/HearthwardCompanionNavigationComponent.h"
#include "../Experience/HearthwardTraversalComponent.h"
#include "../Gameplay/HearthwardCompanionBehavior.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBrotherTraversal053Test,"Hearthward.Traversal053.BrotherVaultCostsAndCancellation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBrotherTraversal053Test::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Player=World->SpawnActor<ACharacter>(FVector(0,800,90),FRotator::ZeroRotator);
    auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
    auto* G=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(G);G->RegisterComponent();G->Enabled=true;
    auto Box=[&](FVector Center,FVector Extent)
    {
        auto* Actor=World->SpawnActor<AActor>();auto* Shape=NewObject<UBoxComponent>(Actor);
        Actor->AddInstanceComponent(Shape);Actor->SetRootComponent(Shape);Shape->SetBoxExtent(Extent);
        Shape->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Shape->SetCollisionResponseToAllChannels(ECR_Block);
        Shape->RegisterComponent();Actor->SetActorLocation(Center);return Actor;
    };
    Box(FVector(0,0,-10),FVector(2000,2000,10));
    Box(FVector(90,0,40),FVector(40,150,40));
    auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>(FVector(0,0,82),FRotator::ZeroRotator);
    auto* Traversal=Brother->FindComponentByClass<UHearthwardTraversalComponent>();
    auto* Survival=Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(TestNotNull(TEXT("Brother has the shared traversal executor"),Traversal))
    {
        auto* Movement=Brother->GetCharacterMovement();Movement->SetMovementMode(MOVE_Walking);
        TestNotNull(TEXT("Brother uses shared swimming physics"),Cast<UHearthwardMovementComponent>(Movement));
        FVector Landing;TestTrue(TEXT("Actual 80cm obstacle has a clear landing"),Traversal->FindVault(Landing));
        Survival->Stamina()=7;
        TestFalse(TEXT("Insufficient brother stamina rejects vault"),Traversal->BeginVault());
        Survival->Stamina()=100;
        TestTrue(TEXT("Brother can vault without a player gameplay component"),Traversal->BeginVault());
        TestEqual(TEXT("Vault charges brother once"),Survival->Stamina(),92.f);
        Traversal->TickComponent(.15f,LEVELTICK_All,nullptr);
        const auto Position=Brother->GetActorLocation();
        Survival->ReceiveDamage(1,FGuid::NewGuid(),World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch());
        TestFalse(TEXT("Damage interrupts the same traversal"),Traversal->IsVaulting());
        Traversal->TickComponent(.5f,LEVELTICK_All,nullptr);
        TestTrue(TEXT("Cancellation leaves the actual position"),Brother->GetActorLocation().Equals(Position));
        TestEqual(TEXT("Cancellation does not refund stamina"),Survival->Stamina(),92.f);
        Brother->SetActorLocation(FVector(0,0,82));Movement->SetMovementMode(MOVE_Walking);
        TestTrue(TEXT("A later vault can start"),Traversal->BeginVault());
        World->GetSubsystem<UHearthwardStorageSubsystem>()->AdvanceTimeline();
        Traversal->TickComponent(.1f,LEVELTICK_All,nullptr);
        TestFalse(TEXT("An old timeline cannot continue the vault"),Traversal->IsVaulting());
    }
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBrotherFollowHeight053Test,"Hearthward.Traversal053.FollowKeepsVerticalSeparation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBrotherFollowHeight053Test::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Player=World->SpawnActor<ACharacter>(FVector(0,0,700),FRotator::ZeroRotator);
    auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>(FVector(0,0,80),FRotator::ZeroRotator);
    FHearthwardCompanionBehaviorContext Context;Context.Player=Player;Context.Companion=Brother;Context.RequestedOrder=TEXT("follow");
    HearthwardCompanionBehavior::Tick(Context);
    TestFalse(TEXT("Follow must seek a route or explain failure when floors differ"),Brother->BlockReason.IsEmpty());
    Player->SetActorLocation(FVector(100,0,80));HearthwardCompanionBehavior::Tick(Context);
    TestTrue(TEXT("Same floor within follow radius can settle"),Brother->BlockReason.IsEmpty());
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
#endif
