#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardProjectile.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Gameplay/HearthwardGameData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Animation/HearthwardBrotherAnimInstance.h"
#include "../Animation/HearthwardHeroAnimInstance.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../HearthwardCharacter.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPresentation095Test,
    "Hearthward.Iteration.Task095.WeaponInstancePresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponPresentation095Test::RunTest(const FString&)
{
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);World->InitializeActorsForPlay(FURL());
    auto* Hero=World->SpawnActor<AHearthwardCharacter>();
    auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Hero);
    Hero->Gameplay->Enabled=true;World->GetWorldSettings()->NotifyBeginPlay();Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    auto* Bag=Hero->FindComponentByClass<UHearthwardInventoryComponent>();
    auto* Combat=Hero->FindComponentByClass<UHearthwardCombatComponent>();
    auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
    TArray<UStaticMeshComponent*> Components;Hero->GetComponents(Components);
    auto** Found=Components.FindByPredicate([](const auto* C){return C->GetFName()==TEXT("HeldWeapon");});
    if(!TestTrue(TEXT("Production hero has a weapon visual"),Found!=nullptr))
    {World->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return false;}
    auto* Held=*Found;
    TestFalse(TEXT("Empty inventory has no displayed weapon"),Held->GetVisibleFlag());
    for(const TCHAR* Id:{TEXT("shortblade"),TEXT("longblade"),TEXT("spear"),TEXT("shortblade_2"),TEXT("longblade_2"),TEXT("spear_2"),TEXT("blunt_2"),TEXT("shortblade_3"),TEXT("longblade_3"),TEXT("spear_3"),TEXT("blunt_3"),TEXT("hearth_blade"),TEXT("bow"),TEXT("bow_2"),TEXT("crossbow_2"),TEXT("bow_3"),TEXT("crossbow_3")})
    {
        if(!TestTrue(FString::Printf(TEXT("%s can enter the actual inventory"),Id),Bag->TryAdd(Id,1)==EHearthwardInventoryResult::Success))continue;
        FGuid Instance=Bag->FirstInstance(Id);
        TestTrue(TEXT("Production timed equip starts"),Hero->Gameplay->EquipInstance(Instance));
        Clock->Tick(.5f);Combat->TickComponent(.5f,LEVELTICK_All,nullptr);
        const auto Row=HearthwardData::Find(TEXT("items"),Id);
        const FString Kind=HearthwardData::Text(Row,TEXT("combatClass"));
        const FString Name=Kind==TEXT("blunt")?TEXT("Waraxe"):Kind.Left(1).ToUpper()+Kind.Mid(1);
        TestTrue(FString::Printf(TEXT("%s is visible after equip"),Id),Held->GetVisibleFlag());
        const FString Expected=Kind==TEXT("bow")?TEXT("SM_Longbow_Practical"):TEXT("SM_")+Name+TEXT("_Practical");
        TestEqual(TEXT("Actual family mesh follows equipped GUID"),Held->GetStaticMesh()->GetName(),Expected);
        TestEqual(TEXT("Bow uses left hand, other weapons right"),Held->GetAttachSocketName(),FName(Kind==TEXT("bow")?TEXT("hand_l"):TEXT("hand_r")));
        TestTrue(TEXT("World scale cannot inherit the legacy skeleton's 100x root"),Held->GetComponentScale().Equals(FVector::OneVector,.001));
        TestTrue(TEXT("Weapon is cosmetic; authority retains combat sweeps"),Held->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
        if(FName(Id)==TEXT("spear_2"))
        {
            auto* Anim=CastChecked<UHearthwardHeroAnimInstance>(Hero->GetMesh()->GetAnimInstance());
            for(bool Heavy:{false,true})
            {
                TestTrue(TEXT("Actual spear attack starts"),Combat->Attack(Heavy));
                TestTrue(TEXT("Spear selects its thrust, without enabling axe physical sampling"),Anim->IsSpear && !Anim->IsStoneAxe);
                const auto Move=HearthwardCombat::Move(TEXT("spear"),Heavy);
                TestTrue(TEXT("Light/heavy timing stays authoritative"),FMath::IsNearlyEqual(Combat->Duration,Move.Duration(),.001));
                const auto Tip=[&](double Time)
                {
                    Combat->Elapsed=Time;Anim->NativeUpdateAnimation(0);
                    Hero->GetMesh()->TickAnimation(.15f,false);Hero->GetMesh()->RefreshBoneTransforms();Held->UpdateComponentToWorld();
                    return Held->GetComponentLocation()+Held->GetUpVector()*119;
                };
                const auto Prepare=Tip(Move.Windup),Contact=Tip(Move.Windup+Move.Active);
                TestTrue(TEXT("Spear advances forward during its effective interval"),FVector::DotProduct(Contact-Prepare,Hero->GetActorForwardVector())>15);
                TestTrue(TEXT("Spear points along the actual attack heading"),FVector::DotProduct(Held->GetUpVector(),Hero->GetActorForwardVector())>.95);
                const FVector RootScale=Hero->GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton().GetRefBonePose()[0].GetScale3D();
                TestTrue(TEXT("Thrust preserves legacy root scale"),Hero->GetMesh()->GetSocketTransform(TEXT("root"),RTS_Component).GetScale3D().Equals(RootScale,.02));
                Combat->Cancel();
            }
        }
        if(FName(Id)==TEXT("bow_2") || FName(Id)==TEXT("crossbow_2"))
        {
            auto* Anim=CastChecked<UHearthwardHeroAnimInstance>(Hero->GetMesh()->GetAnimInstance());
            const auto Pose=[&]() { Anim->NativeUpdateAnimation(.2f);Hero->GetMesh()->TickAnimation(.2f,false);Hero->GetMesh()->RefreshBoneTransforms(); };
            Bag->TryAdd(TEXT("arrow"),5);Combat->SelectRanged(true);Combat->Aim(true);
            if(Kind==TEXT("bow"))
            {
                TArray<USkeletalMeshComponent*> Skeletal;Hero->GetComponents(Skeletal);
                auto** Bow=Skeletal.FindByPredicate([](const auto* C){return C->GetFName()==TEXT("HeldBow");});
                TestTrue(TEXT("Animated bow exists"),Bow!=nullptr);
                TestTrue(TEXT("Production draw starts"),Combat->Shoot(false));
                Pose();(*Bow)->TickAnimation(0,false);(*Bow)->RefreshBoneTransforms();
                const auto Rest=(*Bow)->GetSocketTransform(TEXT("BowNock"),RTS_Component).GetLocation();
                Clock->Tick(1.f);Combat->TickComponent(1.f,LEVELTICK_All,nullptr);Pose();
                (*Bow)->TickAnimation(0,false);(*Bow)->RefreshBoneTransforms();
                const auto Drawn=(*Bow)->GetSocketTransform(TEXT("BowNock"),RTS_Component).GetLocation();
                TestEqual(TEXT("Draw follows authority elapsed"),Anim->BowDrawTime,1.f);
                TestTrue(TEXT("Bow string draws 35 centimetres"),FMath::IsNearlyEqual(FVector::Dist(Rest,Drawn),35.,.2));
                TestTrue(TEXT("Animated bow replaces static idle bow"),(*Bow)->GetVisibleFlag() && !Held->GetVisibleFlag());
                TestTrue(TEXT("Animated bow retains centimetre world scale"),(*Bow)->GetComponentScale().Equals(FVector::OneVector,.001));
                Controller->SetControlRotation(FRotator(20,45,0));Pose();(*Bow)->UpdateComponentToWorld();
                TestTrue(TEXT("Visible bow follows actual yaw/pitch aim"),FVector::DotProduct((*Bow)->GetForwardVector(),Controller->GetControlRotation().Vector())>.99);
                Hero->GetCharacterMovement()->Velocity=FVector(200,0,0);Pose();
                const auto FootA=Hero->GetMesh()->GetSocketLocation(TEXT("foot_l"));Pose();
                TestTrue(TEXT("Upper-body aim retains moving leg animation"),FVector::Dist(FootA,Hero->GetMesh()->GetSocketLocation(TEXT("foot_l")))>2);
                Hero->GetCharacterMovement()->Velocity=FVector::ZeroVector;
                Controller->SetControlRotation(FRotator::ZeroRotator);
                const int32 Arrows=Bag->Available(TEXT("arrow"));
                TestTrue(TEXT("Production release starts recoil"),Combat->Shoot(true));Pose();
                TestEqual(TEXT("Release selects its follow-through"),Anim->RangedClip,13);
                TestEqual(TEXT("Only authority consumes one arrow"),Bag->Available(TEXT("arrow")),Arrows-1);
                Combat->Cancel();Combat->Aim(false);Pose();
                TestEqual(TEXT("Cancellation clears draw deformation"),Anim->BowDrawTime,0.f);
                TestFalse(TEXT("Cancelled bow returns to static carry"),(*Bow)->GetVisibleFlag());
            }
            else
            {
                TestTrue(TEXT("Production crossbow reload starts"),Combat->Reload());Pose();
                TestEqual(TEXT("Reload selects authored animation"),Anim->RangedClip,15);
                TestTrue(TEXT("Reload retains 1.8 second authority duration"),FMath::IsNearlyEqual(Combat->Duration,1.8,.001));
                Clock->Tick(1.81f);Combat->TickComponent(1.81f,LEVELTICK_All,nullptr);
                TestTrue(TEXT("Reload completion still loads crossbow"),Combat->CrossbowLoaded);
                TestTrue(TEXT("Production crossbow fire starts"),Combat->Shoot(false));Pose();
                TestEqual(TEXT("Crossbow selects aim/fire pose"),Anim->RangedClip,14);
                Combat->Cancel();Combat->Aim(false);Pose();
            }
        }
        TestTrue(TEXT("Wear uses actual instance"),Bag->WearInstance(Instance,10000));
        TestFalse(TEXT("Broken current instance disappears"),Held->GetVisibleFlag());
        TestTrue(TEXT("Remove tested instance"),Bag->RemoveInstance(Instance));
        TestFalse(TEXT("Removed weapon stays hidden"),Held->GetVisibleFlag());
    }
    auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>();
    Components.Reset();Brother->GetComponents(Components);
    auto** BrotherFound=Components.FindByPredicate([](const auto* C){return C->GetFName()==TEXT("HeldWeapon");});
    if(TestTrue(TEXT("Production brother has a weapon visual"),BrotherFound!=nullptr))
    {
        auto* BrotherHeld=*BrotherFound;
        for(const TCHAR* Id:{TEXT("axe"),TEXT("shortblade"),TEXT("longblade"),TEXT("spear"),TEXT("shortblade_2"),TEXT("longblade_2"),TEXT("spear_2"),TEXT("blunt_2"),TEXT("shortblade_3"),TEXT("longblade_3"),TEXT("spear_3"),TEXT("blunt_3"),TEXT("hearth_blade")})
        {
            TestTrue(TEXT("Brother receives an actual inventory instance"),Brother->Bag->TryAdd(Id,1)==EHearthwardInventoryResult::Success);
            const FGuid Instance=Brother->Bag->FirstInstance(Id);TestTrue(TEXT("Brother equips that GUID"),Brother->Bag->EquipInstance(Instance));
            TestTrue(TEXT("Brother's equipped weapon is visible"),BrotherHeld->GetVisibleFlag());
            TestEqual(TEXT("Brother weapon follows right hand"),BrotherHeld->GetAttachSocketName(),FName(TEXT("hand_r")));
            TestTrue(TEXT("Brother weapon ignores reference root scaling"),BrotherHeld->GetComponentScale().Equals(FVector(FName(Id)==TEXT("axe")?.7:1),.001));
            TestTrue(TEXT("Brother weapon adds no collision"),BrotherHeld->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
            Brother->Bag->WearInstance(Instance,10000);TestFalse(TEXT("Broken brother weapon is hidden"),BrotherHeld->GetVisibleFlag());
            Brother->Bag->RemoveInstance(Instance);TestFalse(TEXT("Removed brother weapon is hidden"),BrotherHeld->GetVisibleFlag());
        }
    }
    World->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
#if WITH_EDITOR
static FAutoConsoleCommandWithWorld FRescuePreview095Command(
    TEXT("Hearthward.Test095.RescuePreview"),TEXT("Prepare a downed brother for isolated rescue animation review."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        auto* Player=Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(World,0));if(!Player)return;
        auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();Campaign->State.Phase=NAME_None;
        Player->SetActorLocation(FVector(0,0,90));Player->SetActorRotation(FRotator::ZeroRotator);
        auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();G->Enabled=true;G->Health=G->MaxHealth();
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>(FVector(180,0,80),FRotator(0,180,0),Params);
        Brother->SetActorTickEnabled(false);Brother->Tags.Add(TEXT("Task095RescuePatient"));
        Brother->FindComponentByClass<UHearthwardSurvivalComponent>()->ReceiveDamage(10000,FGuid::NewGuid(),World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch());
    }));
static FAutoConsoleCommandWithWorld FCastPreview095Command(
    TEXT("Hearthward.Test095.CastPreview"),TEXT("Arrange production character appearances in an isolated unsaved QA world."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        auto* Player=Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(World,0));if(!Player)return;
        auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();Campaign->State.Phase=NAME_None;
        const auto Place=[](ACharacter* Actor,FName Role,FVector Location)
        {
            Actor->SetActorLocation(Location);Actor->SetActorRotation(FRotator::ZeroRotator);
            Actor->SetActorTickEnabled(false);Actor->Tags.Add(Role);
        };
        Place(Player,TEXT("Task095Cast.Hero"),FVector(0,-500,90));
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>(FVector(0,-300,80),FRotator::ZeroRotator,Params);
        Place(Brother,TEXT("Task095Cast.Brother"),FVector(0,-300,80));
        auto* Civilian=World->SpawnActor<AHearthwardCampaignActor>(FVector(0,-100,80),FRotator::ZeroRotator,Params);
        Civilian->Initialize(TEXT("task095_civilian"),false);Place(Civilian,TEXT("Task095Cast.Civilian"),FVector(0,-100,80));
        int32 Index=0;
        for(FName Kind:{FName(TEXT("guard")),FName(TEXT("archer")),FName(TEXT("heavy"))})
        {
            auto* Record=Campaign->State.Enemies.FindByPredicate([&](const auto& Entry){return Entry.Kind==Kind;});if(!Record)continue;
            FVector Location(0,100+200*Index++,80);Record->Home=Location;Record->Combat.Position=Location;Record->Located=true;
            auto* Enemy=World->SpawnActor<AHearthwardCampaignActor>(Location,FRotator::ZeroRotator,Params);
            Enemy->Initialize(Record->Id,true);Enemy->Target->SetComponentTickEnabled(false);
            Place(Enemy,FName(*(TEXT("Task095Cast.")+Kind.ToString())),Location);
        }
    }));
// Isolated PIE visual fixture; never registered in Shipping or saved into a map.
static FAutoConsoleCommandWithWorld FArcherPreview095Command(
    TEXT("Hearthward.Test095.ArcherPreview"),TEXT("Spawn a production archer for the isolated TASK-095 visual review."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        auto* Player=UGameplayStatics::GetPlayerPawn(World,0);if(!Player)return;
        auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();Campaign->State.Phase=NAME_None;
        auto* Record=Campaign->State.Enemies.FindByPredicate([](const auto& Entry){return Entry.Kind==TEXT("archer");});if(!Record)return;
        Record->Home=Player->GetActorLocation()-FVector(1000,0,0);Record->Combat.Position=Record->Home;Record->Located=true;
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Archer=World->SpawnActor<AHearthwardCampaignActor>(Record->Home,FRotator::ZeroRotator,Params);
        Archer->Initialize(Record->Id,true);Archer->Tags.Add(TEXT("Task095ArcherPreview"));Archer->Target->Exposure=1;
        Archer->Target->Memory.Seen.Add(TEXT("player"));
    }));
#endif
struct FArcherShotTestAccess
{
    static float Pending(const AHearthwardCampaignActor* Actor){return Actor->BowRemaining;}
    static void Ready(AHearthwardCampaignActor* Actor){Actor->AttackIn=0;Actor->DecisionIn=0;}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurvivalMotion095Test,
    "Hearthward.Iteration.Task095.SurvivalMotionUnitsAndEndpoints",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSurvivalMotion095Test::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    for(const TCHAR* Role:{TEXT("Hero"),TEXT("Brother")})
    {
        auto* Owner=World->SpawnActor<AActor>();
        auto* Mesh=NewObject<USkeletalMeshComponent>(Owner);Owner->AddInstanceComponent(Mesh);Owner->SetRootComponent(Mesh);
        const FString Path=FString::Printf(TEXT("/Game/Characters/%s/UE5/SK_%s"),Role,Role);
        auto* Asset=LoadObject<USkeletalMesh>(nullptr,*Path);Mesh->SetSkeletalMesh(Asset);Mesh->RegisterComponent();
        Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        FVector DownHead;
        for(const TCHAR* Kind:{TEXT("Down"),TEXT("GetUp"),TEXT("Rescue")})
        {
            auto* Clip=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-095/Survival/A_%s_%s"),Role,Kind));
            if(!TestNotNull(TEXT("Survival clip loads"),Clip))continue;
            TestTrue(TEXT("Animation retains the character's original skeleton"),Clip->GetSkeleton()==Asset->GetSkeleton());
            TestTrue(TEXT("Duration matches the authoritative action"),FMath::IsNearlyEqual(Clip->GetPlayLength(),FString(Kind)==TEXT("Down")?1.2f:5.f,.001f));
            Mesh->SetAnimation(Clip);
            const auto Sample=[&](float Time)
            {
                Mesh->SetPosition(Time,false);Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();
                const FVector ReferenceScale=Asset->GetRefSkeleton().GetRefBonePose()[0].GetScale3D();
                TestTrue(TEXT("Legacy root scale is preserved for skinning"),Mesh->GetSocketTransform(TEXT("root"),RTS_Component).GetScale3D().Equals(ReferenceScale,.02));
                TestTrue(TEXT("Head skinning retains inherited scale"),Mesh->GetSocketTransform(TEXT("head"),RTS_Component).GetScale3D().Equals(ReferenceScale,.02));
                return Mesh->GetSocketLocation(TEXT("head"));
            };
            Sample(Clip->GetPlayLength()*.5f);
            if(FString(Kind)==TEXT("Down"))
            {
                DownHead=Sample(1.2f);TestTrue(TEXT("Downed head is close to ground in source centimetres"),DownHead.Z>=0 && DownHead.Z<35);
            }
            else if(FString(Kind)==TEXT("GetUp"))
            {
                TestTrue(TEXT("Down and rising endpoints meet"),FVector::Dist(DownHead,Sample(0))<1);
                const auto End=Sample(5);TestTrue(TEXT("Rising ends at adult source height"),End.Z>70 && End.Z<105);
            }
        }
    }
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcherShotLifecycle095Test,
    "Hearthward.Iteration.Task095.ArcherWindupReleaseAndInterrupt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcherShotLifecycle095Test::RunTest(const FString&)
{
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Player=World->SpawnActor<ACharacter>(FVector(1000,0,80),FRotator::ZeroRotator,Params);
    auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();
    auto* Record=Campaign->State.Enemies.FindByPredicate([](const auto& Entry){return Entry.Kind==TEXT("archer");});
    if(!TestNotNull(TEXT("Production campaign includes an archer"),Record))
    {GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return false;}
    Record->Home=FVector(0,0,80);Record->Combat.Position=Record->Home;Record->Located=true;
    auto* Archer=World->SpawnActor<AHearthwardCampaignActor>(Record->Home,FRotator::ZeroRotator,Params);
    Archer->Initialize(Record->Id,true);Archer->Target->Memory.Seen.Add(TEXT("player"));
    auto* Mesh=Archer->GetMesh();
    TestEqual(TEXT("Real campaign selects the specialized archer mesh"),Mesh->GetSkeletalMeshAsset()->GetName(),FString(TEXT("SK_Archer_Combat")));
    TestTrue(TEXT("Equipment does not change body height"),FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().Z,160./99.74417,.001));
    auto* Anim=Cast<UHearthwardBrotherAnimInstance>(Mesh->GetAnimInstance());
    TestTrue(TEXT("Campaign binds the actual shoot clip"),Anim && Anim->Clips[4] && Anim->Clips[4]->GetName()==TEXT("A_Archer_Shoot"));
    const auto Count=[&]()
    {int32 N=0;for(TActorIterator<AHearthwardProjectile> It(World);It;++It)if(!It->IsActorBeingDestroyed())++N;return N;};
    Archer->Tick(.01f);
    TestTrue(TEXT("Attack starts a visible 0.6-second windup"),FMath::IsNearlyEqual(FArcherShotTestAccess::Pending(Archer),.6f));
    TestEqual(TEXT("Windup does not spawn an immediate arrow"),Count(),0);
    Archer->Tick(.3f);TestEqual(TEXT("Half windup still has no arrow"),Count(),0);
    Archer->Tick(.3f);TestEqual(TEXT("Release spawns exactly one arrow"),Count(),1);
    float Power=0;
    for(TActorIterator<AHearthwardProjectile> It(World);It;++It)
    {
        TestTrue(TEXT("Arrow keeps calibrated speed"),FMath::IsNearlyEqual(It->Velocity.Size(),3000.,.01));
        TestEqual(TEXT("Arrow keeps range"),It->RemainingRange,3000.);
        TestTrue(TEXT("Arrow is emitted by the actual campaign actor"),It->EnemyShooter.Get()==Archer);
        Power=It->Power;
    }
    TestTrue(TEXT("Existing calibrated damage remains positive"),Power>0);
    Archer->Tick(.1f);TestEqual(TEXT("Subsequent ticks cannot duplicate the release"),Count(),1);
    FArcherShotTestAccess::Ready(Archer);Archer->Tick(.01f);
    Archer->Target->Memory.HitRemaining=1;Archer->Tick(.1f);
    TestEqual(TEXT("Hit interruption cancels the pending arrow"),FArcherShotTestAccess::Pending(Archer),0.f);
    Archer->Target->Memory.HitRemaining=0;Archer->Tick(.7f);
    TestEqual(TEXT("Cancelled shot never appears late"),Count(),1);
    FArcherShotTestAccess::Ready(Archer);Archer->Tick(.01f);
    World->GetSubsystem<UHearthwardStorageSubsystem>()->AdvanceTimeline();Archer->Tick(.6f);
    TestEqual(TEXT("A load/timeline change cancels the pending arrow"),Count(),1);
    auto* PreviewOwner=World->SpawnActor<AActor>();
    auto* Preview=NewObject<USkeletalMeshComponent>(PreviewOwner);PreviewOwner->AddInstanceComponent(Preview);PreviewOwner->SetRootComponent(Preview);
    Preview->SetSkeletalMesh(Mesh->GetSkeletalMeshAsset());Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);Preview->RegisterComponent();
    Preview->SetAnimationMode(EAnimationMode::AnimationSingleNode);Preview->SetAnimation(Anim->Clips[4]);
    const auto Sample=[&](float Time)
    {Preview->SetPosition(Time,false);Preview->TickAnimation(0,false);Preview->RefreshBoneTransforms();};
    Sample(.59f);
    const FVector Grip=Preview->GetSocketLocation(TEXT("BowGrip")),Nock=Preview->GetSocketLocation(TEXT("BowNock"));
    TestTrue(TEXT("Imported drawing pose lifts bow to chest height in centimetres"),Grip.Z>75 && Grip.Z<85 && Grip.X>25 && Grip.X<35);
    TestTrue(TEXT("Imported string draws back toward the character"),Grip.X-Nock.X>27 && Grip.X-Nock.X<30);
    TestTrue(TEXT("Nocked arrow is visible immediately before release"),Preview->GetSocketTransform(TEXT("NockedArrow"),RTS_Component).GetScale3D().GetMin()>.5);
    Sample(.64f);
    TestTrue(TEXT("Held arrow disappears when the real projectile releases"),Preview->GetSocketTransform(TEXT("NockedArrow"),RTS_Component).GetScale3D().GetMax()<.01);
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcherReferencePose095Test,
    "Hearthward.Iteration.Task095.ArcherReferencePose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcherReferencePose095Test::RunTest(const FString&)
{
    auto* Guard=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Hearthward/Campaign/Guard/SK_Guard_Runtime.SK_Guard_Runtime"));
    auto* Archer=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-095/Archer/SK_Archer_Practical.SK_Archer_Practical"));
    if(!TestNotNull(TEXT("Guard loads"),Guard) || !TestNotNull(TEXT("Archer loads"),Archer))return false;
    TestTrue(TEXT("Archer reuses Guard skeleton"),Archer->GetSkeleton()==Guard->GetSkeleton());
    const auto& Expected=Guard->GetRefSkeleton();
    const auto& Actual=Archer->GetRefSkeleton();
    if(!TestEqual(TEXT("Reference bone count"),Actual.GetNum(),Expected.GetNum()))return false;
    for(int32 Index=0;Index<Expected.GetNum();++Index)
    {
        const FString Name=Expected.GetBoneName(Index).ToString();
        TestEqual(Name+TEXT(" name"),Actual.GetBoneName(Index),Expected.GetBoneName(Index));
        TestEqual(Name+TEXT(" parent"),Actual.GetParentIndex(Index),Expected.GetParentIndex(Index));
        TestTrue(Name+TEXT(" reference transform"),Actual.GetRefBonePose()[Index].Equals(Expected.GetRefBonePose()[Index],.01));
    }
    TestTrue(TEXT("Archer feet remain at ground height"),FMath::Abs(Archer->GetBounds().Origin.Z-Archer->GetBounds().BoxExtent.Z)<.01);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcheryProjectilePresentation095Test,
    "Hearthward.Iteration.Task095.ArrowPresentationKeepsTrajectory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcheryProjectilePresentation095Test::RunTest(const FString&)
{
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Owner=World->SpawnActor<AActor>();
    auto* Root=NewObject<USceneComponent>(Owner);
    Owner->AddInstanceComponent(Root);Owner->SetRootComponent(Root);Root->RegisterComponent();
    auto* Shooter=NewObject<UHearthwardCombatComponent>(Owner);
    Owner->AddInstanceComponent(Shooter);Shooter->RegisterComponent();
    const auto Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    const FVector InitialVelocity(900,200,100);
    const auto Spawn=[&](FName Item,FVector Position)
    {
        auto* Projectile=World->SpawnActor<AHearthwardProjectile>(Position,FRotator::ZeroRotator);
        Projectile->Shooter=Shooter;Projectile->Epoch=Epoch;Projectile->Item=Item;
        Projectile->Velocity=InitialVelocity;Projectile->Gravity=980;
        Projectile->Lifetime=2;Projectile->RemainingRange=10000;Projectile->Power=17;
        return Projectile;
    };
    const FVector Offset(0,200,0);
    auto* Arrow=Spawn(TEXT("arrow"),FVector(0,0,500));
    auto* Stone=Spawn(TEXT("stone"),FVector(0,0,500)+Offset);
    Arrow->Tick(.1f);Stone->Tick(.1f);
    auto* ArrowMesh=CastChecked<UStaticMeshComponent>(Arrow->GetRootComponent());
    auto* StoneMesh=CastChecked<UStaticMeshComponent>(Stone->GetRootComponent());
    const FString ArrowPath=TEXT("/Game/Hearthward/Assets/TASK-095/Archery/Arrow/SM_Arrow_Practical.SM_Arrow_Practical");
    TestNotNull(TEXT("Authored arrow mesh loads"),ArrowMesh->GetStaticMesh().Get());
    if(ArrowMesh->GetStaticMesh())
    {
        TestEqual(TEXT("Player arrow uses the authored asset"),ArrowMesh->GetStaticMesh()->GetPathName(),ArrowPath);
        const auto Bounds=ArrowMesh->GetStaticMesh()->GetBoundingBox();
        TestTrue(TEXT("Arrow tip stays at the existing trace origin"),FMath::Abs(Bounds.Max.X)<.01);
        TestTrue(TEXT("Arrow length is in centimetres"),Bounds.GetSize().X>79 && Bounds.GetSize().X<81);
    }
    TestEqual(TEXT("Stone retains its existing visual"),StoneMesh->GetStaticMesh()->GetPathName(),FString(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    TestTrue(TEXT("Stone retains its existing scale"),StoneMesh->GetRelativeScale3D().Equals(FVector(.06),.0001));
    TestTrue(TEXT("Visual replacement preserves the integrated trajectory"),(Stone->GetActorLocation()-Offset).Equals(Arrow->GetActorLocation(),.001));
    TestTrue(TEXT("Gravity updates are unchanged"),Stone->Velocity.Equals(Arrow->Velocity,.001));
    TestEqual(TEXT("Remaining range is unchanged"),Stone->RemainingRange,Arrow->RemainingRange);
    TestEqual(TEXT("Lifetime is unchanged"),Stone->Lifetime,Arrow->Lifetime);
    TestEqual(TEXT("Damage input is unchanged"),Arrow->Power,17.f);
    TestTrue(TEXT("Arrow points along its actual velocity"),FVector::DotProduct(Arrow->GetActorForwardVector(),Arrow->Velocity.GetSafeNormal())>.999);
    TestTrue(TEXT("Mesh does not introduce a second collision path"),ArrowMesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision);

    auto* EnemyArrow=Spawn(NAME_None,FVector(0,400,500));
    EnemyArrow->Shooter.Reset();EnemyArrow->EnemyShooter=Owner;EnemyArrow->Tick(.01f);
    TestEqual(TEXT("Enemy arrows also use the authored asset"),
        CastChecked<UStaticMeshComponent>(EnemyArrow->GetRootComponent())->GetStaticMesh()->GetPathName(),ArrowPath);
    auto* Restored=Spawn(TEXT("arrow"),FVector(0,600,500));
    const FRotator SavedRotation(15,35,5);
    Restored->SetActorRotation(SavedRotation);Restored->Landed=true;Restored->Velocity=FVector::ZeroVector;
    Restored->Tick(.1f);
    TestEqual(TEXT("Restored landed arrow receives the correct visual"),
        CastChecked<UStaticMeshComponent>(Restored->GetRootComponent())->GetStaticMesh()->GetPathName(),ArrowPath);
    TestTrue(TEXT("Restored landed arrow keeps its saved rotation"),Restored->GetActorRotation().Equals(SavedRotation,.001));
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return true;
}
#endif
