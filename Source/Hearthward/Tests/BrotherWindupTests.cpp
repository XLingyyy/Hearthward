#include "../HearthwardCharacter.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Animation/HearthwardBrotherAnimInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS
#if WITH_EDITOR
static FAutoConsoleCommandWithWorldAndArgs FBrotherMelee095Preview(
    TEXT("Hearthward.Test095.MeleePreview"),TEXT("Isolated brother authority windup preview: setup spear/longblade, tick, or interrupt."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* World)
    {
        auto* Hero=UGameplayStatics::GetPlayerPawn(World,0);
        AHearthwardCompanionFixture* Brother=nullptr;AActor* Target=nullptr;
        for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It)if(It->ActorHasTag(TEXT("Task095Cast.Brother")))Brother=*It;
        for(TActorIterator<AActor> It(World);It;++It)if(It->ActorHasTag(TEXT("Task095MeleeTarget")))Target=*It;
        if(!Hero || !Brother || Args.IsEmpty())return;
        if(Args[0]==TEXT("setup"))
        {
            Brother->CancelMeleeAttack();
            const FName Item(Args.Num()>1?*Args[1]:TEXT("spear"));Brother->Bag->TryAdd(Item,1);Brother->Bag->EquipInstance(Brother->Bag->FirstInstance(Item));
            if(!Target)
            {
                Target=World->SpawnActor<AActor>();Target->Tags.Add(TEXT("Task095MeleeTarget"));
                auto* Root=NewObject<USceneComponent>(Target);Target->SetRootComponent(Root);Root->RegisterComponent();
                auto* Combat=NewObject<UHearthwardCombatTargetComponent>(Target);Target->AddInstanceComponent(Combat);Combat->RegisterComponent();
                Combat->Id=TEXT("windup_visual");Combat->Health=Combat->MaximumHealth=10000;
            }
            Target->SetActorLocation(Brother->GetActorLocation()+FVector(100,0,0));
        }
        else if(Args[0]==TEXT("tick") && Target)
        {
            Target->FindComponentByClass<UHearthwardCombatTargetComponent>()->Memory.HitRemaining=0;
            Brother->AdvanceMeleeAttack(Target,Hero);
        }
        else if(Args[0]==TEXT("interrupt"))
            Brother->FindComponentByClass<UHearthwardSurvivalComponent>()->ReceiveDamage(1,FGuid::NewGuid(),World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch());
    }));
#endif
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBrotherWindup095Test,"Hearthward.Iteration.Task095.BrotherWindup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBrotherWindup095Test::RunTest(const FString&)
{
    // PROTOTYPE_ONLY: isolated authority fixture, no production save or campaign facts.
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);World->InitializeActorsForPlay(FURL());
    auto* Hero=World->SpawnActor<AHearthwardCharacter>(FVector(-400,0,90),FRotator::ZeroRotator);
    auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Hero);
    Hero->Gameplay->Enabled=true;World->GetWorldSettings()->NotifyBeginPlay();
    auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>(FVector(0,0,90),FRotator::ZeroRotator);
    auto* Target=World->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Target);Target->SetRootComponent(Root);Root->RegisterComponent();Target->SetActorLocation(FVector(100,0,90));
    auto* Combat=NewObject<UHearthwardCombatTargetComponent>(Target);Target->AddInstanceComponent(Combat);Combat->RegisterComponent();
    Combat->Id=TEXT("windup_test");Combat->Health=Combat->MaximumHealth=10000;
    Brother->Bag->TryAdd(TEXT("spear"),1);const auto Weapon=Brother->Bag->FirstInstance(TEXT("spear"));Brother->Bag->EquipInstance(Weapon);
    auto* Survival=Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    const auto Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    const float Damage=HearthwardData::Number(HearthwardData::Find(TEXT("items"),TEXT("spear")),TEXT("attack"));
    auto* Anim=CastChecked<UHearthwardBrotherAnimInstance>(Brother->GetMesh()->GetAnimInstance());
    auto Advance=[&](float Seconds)
    {
        // WorldSettings caps a single frame; advance actual world time in normal-size steps.
        while(Seconds>0){const float Step=FMath::Min(Seconds,.02f);World->Tick(LEVELTICK_TimeOnly,Step);Seconds-=Step;}
    };
    for(bool Hunting:{false,true})
    {
        Advance(1.3f);Combat->Memory.HitRemaining=0;
        const float Before=Combat->Health;const double Wear=Brother->Bag->FindInstance(Weapon)->Durability;
        TestFalse(TEXT("Starting a melee only starts the windup"),Brother->AdvanceMeleeAttack(Target,Hero,Hunting));
        TestEqual(TEXT("Spear uses the authored two-hand thrust"),Anim->Clips[4]->GetName(),FString(TEXT("A_Brother_SpearThrust")));
        Advance(.24f);
        TestFalse(TEXT("No hit before 0.25 seconds"),Brother->AdvanceMeleeAttack(Target,Hero,Hunting));
        TestEqual(TEXT("Windup spends no health or durability"),Combat->Health,Before);
        TestEqual(TEXT("Windup leaves weapon intact"),Brother->Bag->FindInstance(Weapon)->Durability,Wear);
        Advance(.02f);
        TestTrue(TEXT("One hit after the approved windup"),Brother->AdvanceMeleeAttack(Target,Hero,Hunting));
        TestEqual(TEXT("Original weapon damage retained"),Combat->Health,Before-Damage);
        TestEqual(TEXT("Exactly one durability charged"),Brother->Bag->FindInstance(Weapon)->Durability,Wear-1);
        Combat->Memory.HitRemaining=0;
        TestFalse(TEXT("Repeated call cannot repeat damage"),Brother->AdvanceMeleeAttack(Target,Hero,Hunting));
        Advance(.93f);TestFalse(TEXT("Cycle remains blocked before 1.2 seconds"),Brother->MeleeAttackReady());
        Advance(.02f);TestTrue(TEXT("Next windup is ready at 1.2 seconds from start"),Brother->MeleeAttackReady());
        Brother->AdvanceMeleeAttack(Target,Hero,Hunting);
        Survival->ReceiveDamage(1,FGuid::NewGuid(),Epoch);
        Advance(.3f);
        TestFalse(TEXT("Received damage cancels the uncommitted hit"),Brother->AdvanceMeleeAttack(Target,Hero,Hunting));
        TestEqual(TEXT("Interrupted hit causes no target damage"),Combat->Health,Before-Damage);
        Advance(1.3f);Brother->AdvanceMeleeAttack(Target,Hero,Hunting);
        Target->SetActorLocation(FVector(1500,0,90));Advance(.3f);
        TestFalse(TEXT("Target leaving reach cancels the hit"),Brother->AdvanceMeleeAttack(Target,Hero,Hunting));
        Target->SetActorLocation(FVector(100,0,90));
    }
    Advance(1.3f);Combat->Memory.HitRemaining=0;
    const float BeforeRestore=Combat->Health;
    Brother->AdvanceMeleeAttack(Target,Hero);
    World->GetSubsystem<UHearthwardStorageSubsystem>()->AdvanceTimeline();Advance(.3f);
    TestFalse(TEXT("Restored timeline cancels a pending attack"),Brother->AdvanceMeleeAttack(Target,Hero));
    TestEqual(TEXT("Old timeline cannot deal a late hit"),Combat->Health,BeforeRestore);
    Advance(1.3f);Brother->AdvanceMeleeAttack(Target,Hero);
    Brother->Bag->TryAdd(TEXT("longblade"),1);Brother->Bag->EquipInstance(Brother->Bag->FirstInstance(TEXT("longblade")));Advance(.3f);
    TestFalse(TEXT("Switching weapon GUID cancels the pending hit"),Brother->AdvanceMeleeAttack(Target,Hero));
    TestEqual(TEXT("Weapon switch cannot inherit the old swing"),Combat->Health,BeforeRestore);
    World->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
#endif
