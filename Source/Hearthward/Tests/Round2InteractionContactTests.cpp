#include "../Interaction/HearthwardInteractionAlignment.h"
#include "../Environment/HearthwardShortcutActor.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Experience/HearthwardSituationalAudio.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2ContactTest,"Hearthward.Round2.Contact.ClearanceCancellationAndLadder",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2ContactTest::RunTest(const FString&)
{
    auto* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
    ON_SCOPE_EXIT{GEngine->DestroyWorldContext(W);W->DestroyWorld(false);};
    auto Box=[&](FVector Center,FVector Extent)
    {auto* A=W->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(A);A->AddInstanceComponent(B);A->SetRootComponent(B);B->SetBoxExtent(Extent);B->SetCollisionProfileName(TEXT("BlockAll"));B->RegisterComponent();A->SetActorLocation(Center);return A;};
    Box(FVector(0,0,-10),FVector(3000,3000,10));
    auto* C=W->SpawnActor<ACharacter>(FVector(-500,0,90),FRotator::ZeroRotator);C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    auto* S=NewObject<UHearthwardSurvivalComponent>(C);C->AddInstanceComponent(S);S->RegisterComponent();
    auto* Contact=NewObject<UHearthwardInteractionAlignment>(C);C->AddInstanceComponent(Contact);Contact->RegisterComponent();
    auto* Target=W->SpawnActor<AActor>();
    TestFalse(TEXT("No long distance snapping"),Contact->CanAlign(C,FVector(-300,0,90),FRotator::ZeroRotator));
    TestFalse(TEXT("No unsupported raised alignment"),Contact->CanAlign(C,FVector(-480,0,160),FRotator::ZeroRotator));
    TestFalse(TEXT("No forced turn from the back"),Contact->CanAlign(C,FVector(-480,0,90),FRotator(0,180,0)));
    auto* Block=Box(FVector(-475,0,95),FVector(8,100,95));
    TestFalse(TEXT("Sweep rejects an occupied contact path"),Contact->CanAlign(C,FVector(-450,0,90),FRotator::ZeroRotator));Block->Destroy();
    TestTrue(TEXT("Clear supported contact begins"),Contact->Begin(TEXT("Operate"),1.2f,Target,FVector(-480,0,90),FRotator::ZeroRotator));
    S->Health()-=1;Contact->TickComponent(.1f,LEVELTICK_All,nullptr);TestFalse(TEXT("Damage releases contact"),Contact->Active());
    TestTrue(TEXT("Later contact can restart"),Contact->Begin(TEXT("Operate"),1.2f,Target,C->GetActorLocation(),FRotator::ZeroRotator));
    W->GetSubsystem<UHearthwardStorageSubsystem>()->AdvanceTimeline();Contact->TickComponent(.1f,LEVELTICK_All,nullptr);TestFalse(TEXT("Timeline discards old pose"),Contact->Active());
    auto* Gate=W->SpawnActor<AHearthwardShortcutActor>(FVector(-1000,0,0),FRotator::ZeroRotator);
    Gate->Configure(TEXT("workshops_inner_latch"),EHearthwardDeviceKind::Latch);
    C->SetActorLocation(FVector(-1090,0,90));Gate->Use(C);Gate->Tick(1.3f);
    TestTrue(TEXT("Latch contact station clears the closed door capsule"),Gate->IsOpen());
    Contact->Cancel();Gate->Destroy();
    Contact->DestroyComponent();
    Box(FVector(-170,0,110),FVector(110,140,110));Box(FVector(0,0,120),FVector(30,140,120));
    auto* Ladder=W->SpawnActor<AHearthwardShortcutActor>();Ladder->Configure(TEXT("cellar_drop_ladder"),EHearthwardDeviceKind::Ladder);
    auto* Bag=NewObject<UHearthwardInventoryComponent>(C);C->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* G=NewObject<UHearthwardGameplayComponent>(C);C->AddInstanceComponent(G);G->RegisterComponent();
    C->SetActorLocation(FVector(-160,0,310));Ladder->Use(C);Ladder->Tick(1.3f);TestTrue(TEXT("Inner platform releases ladder"),Ladder->IsOpen());
    Ladder->Use(C);TestTrue(TEXT("Opened ladder accepts bounded descent"),Ladder->Active());
    for(int32 I=0;I<31;++I)Ladder->Tick(.1f);
    TestFalse(TEXT("Bounded descent ends"),Ladder->Active());TestTrue(TEXT("Actual capsule reaches outer floor"),C->GetActorLocation().Equals(FVector(100,0,90),3));
    TestEqual(TEXT("Character control restored"),C->GetCharacterMovement()->MovementMode,EMovementMode::MOVE_Falling);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2MixTest,"Hearthward.Round2.Audio.ContextPriority",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2MixTest::RunTest(const FString&)
{
    using namespace HearthwardSituationalAudio;
    TestEqual(TEXT("Combat takes priority in a building"),Context(true,true,true),EContext::Combat);
    TestEqual(TEXT("Solid roof distinguishes interior"),Context(false,true,true),EContext::Indoor);
    TestEqual(TEXT("Return to camp"),Context(false,false,true),EContext::Camp);
    TestEqual(TEXT("Ordinary outdoor exploration"),Context(false,false,false),EContext::Outdoor);
    TestTrue(TEXT("Dialogue ducks existing environment beds"),EnvironmentGain(EContext::Outdoor,true,false)<EnvironmentGain(EContext::Outdoor,false,false));
    TestTrue(TEXT("Successful interaction has room above the bed"),EnvironmentGain(EContext::Camp,false,true)<EnvironmentGain(EContext::Camp,false,false));return true;
}
#endif
