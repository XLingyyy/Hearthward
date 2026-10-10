#include "../HearthwardCharacter.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardProjectile.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Animation/HearthwardHeroAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRangedMuzzle095Test,"Hearthward.Iteration.Task095.RangedMuzzle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRangedMuzzle095Test::RunTest(const FString&)
{
    // PROTOTYPE_ONLY: actual inventory/attack paths in an isolated collision world.
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);World->InitializeActorsForPlay(FURL());
    auto* Hero=World->SpawnActor<AHearthwardCharacter>(FVector(0,0,90),FRotator::ZeroRotator);
    auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Hero);
    Hero->Gameplay->Enabled=true;World->GetWorldSettings()->NotifyBeginPlay();
    Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    auto* Bag=Hero->FindComponentByClass<UHearthwardInventoryComponent>();
    auto* Combat=Hero->FindComponentByClass<UHearthwardCombatComponent>();
    auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
    auto* Anim=CastChecked<UHearthwardHeroAnimInstance>(Hero->GetMesh()->GetAnimInstance());
    auto Pose=[&]()
    {
        Anim->NativeUpdateAnimation(.2f);Hero->GetMesh()->TickAnimation(.2f,false);Hero->GetMesh()->RefreshBoneTransforms();
        TArray<USkeletalMeshComponent*> Meshes;Hero->GetComponents(Meshes);
        for(auto* Mesh:Meshes)if(Mesh!=Hero->GetMesh()){Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();}
    };
    auto Advance=[&](float Delta){Clock->Tick(Delta);Combat->TickComponent(Delta,LEVELTICK_All,nullptr);};
    Bag->TryAdd(TEXT("arrow"),20);
    for(const TCHAR* Id:{TEXT("bow_2"),TEXT("crossbow_2")})
    {
        Combat->Cancel();Combat->Aim(false);Bag->TryAdd(Id,1);Hero->Gameplay->EquipInstance(Bag->FirstInstance(Id));Advance(.5f);
        Combat->SelectRanged(true);Combat->Aim(true);
        const bool Crossbow=FName(Id)==TEXT("crossbow_2");
        for(bool Blocked:{false,true})
        {
            Combat->Cancel();Hero->Gameplay->Stamina=Hero->Gameplay->MaxStamina();
            if(Crossbow){Combat->CrossbowLoaded=false;TestTrue(TEXT("Crossbow reload begins"),Combat->Reload());Advance(1.81f);}
            else {TestTrue(TEXT("Bow draw begins"),Combat->Shoot(false));Advance(1.f);}
            Pose();
            const FVector Tip=Hero->RangedArrowTip();
            AActor* Wall=nullptr;
            if(Blocked)
            {
                Wall=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Box->SetBoxExtent(FVector(2,200,200));
                Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();
                const FVector Chest=Hero->GetActorLocation()+FVector(0,0,30);Wall->SetActorLocation(FMath::Lerp(Chest,Tip,.5f));
            }
            const int32 Before=Bag->Available(TEXT("arrow"));
            TestTrue(TEXT("Authority accepts the shot"),Combat->Shoot(!Crossbow));
            if(Crossbow)Advance(.11f);
            AHearthwardProjectile* Arrow=nullptr;
            for(TActorIterator<AHearthwardProjectile> It(World);It;++It)if(!It->IsActorBeingDestroyed())Arrow=*It;
            if(TestNotNull(TEXT("Exactly one production projectile was created"),Arrow))
            {
                TestEqual(TEXT("One shot spends exactly one arrow"),Bag->Available(TEXT("arrow")),Before-1);
                TestEqual(TEXT("Only obstructed shots land immediately"),Arrow->Landed,Blocked);
                if(!Blocked)TestTrue(TEXT("Projectile begins at the displayed arrow tip"),Arrow->GetActorLocation().Equals(Tip,.01));
                else
                {
                    TestTrue(TEXT("Blocked arrow stops before the visible tip"),FVector::Dist(Arrow->GetActorLocation(),Hero->GetActorLocation())<FVector::Dist(Tip,Hero->GetActorLocation()));
                    TestTrue(TEXT("Wall shot remains recoverable"),Arrow->Recover(Hero));
                    TestEqual(TEXT("Recovery returns that one arrow"),Bag->Available(TEXT("arrow")),Before);
                }
                Arrow->Destroy();
            }
            if(Wall)Wall->Destroy();
        }
    }
    World->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
#endif
