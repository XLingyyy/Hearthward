#include "../Experience/HearthwardFootContactNotify.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
struct FFootContactNotifyAccess
{
    static bool Read(const UHearthwardFootContactNotify* Notify,USkeletalMeshComponent* Mesh,FHearthwardFootContactReceipt& Receipt)
    {return Notify->ReadGroundContact(Mesh,Receipt);}
};
namespace
{
struct FFootClip
{
    const TCHAR* Package;
    const TCHAR* CharacterClass;
    float LeftContact;
};
const FFootClip Clips[] = {
    {TEXT("/Game/Characters/Hero/AnimationV2/A_Hero_Walk"),TEXT("/Script/Hearthward.HearthwardCharacter"),.75f},
    {TEXT("/Game/Characters/Hero/AnimationV2/A_Hero_Run"),TEXT("/Script/Hearthward.HearthwardCharacter"),8.f/24.f},
    {TEXT("/Game/Characters/Brother/Animation/A_Brother_Walk"),TEXT("/Script/Hearthward.HearthwardCompanionFixture"),.75f},
    {TEXT("/Game/Characters/Brother/Animation/A_Brother_Run"),TEXT("/Script/Hearthward.HearthwardCompanionFixture"),8.f/24.f},
};
struct FFootContactWorld
{
    UGameInstance* Instance;
    UWorld* World;
    ACharacter* Player;
    UHearthwardSurvivalComponent* Survival;
    UHearthwardPresentationComponent* Presentation;
    UBoxComponent* Floor;
    FFootContactWorld()
    {
        Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();World=Instance->GetWorld();
        Player=World->SpawnActor<ACharacter>();
        Player->GetCapsuleComponent()->SetCapsuleSize(34,90);Player->SetActorLocation(FVector(0,0,90));
        auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
        Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
        auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Player);
        auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));Controller->SetPlayer(LocalPlayer);
        auto* Ground=World->SpawnActor<AActor>();Floor=NewObject<UBoxComponent>(Ground);
        Ground->AddInstanceComponent(Floor);Ground->SetRootComponent(Floor);Floor->SetBoxExtent(FVector(1000,1000,10));
        Floor->SetCollisionObjectType(ECC_WorldStatic);Floor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Floor->SetCollisionResponseToAllChannels(ECR_Block);Floor->RegisterComponent();Ground->SetActorLocation(FVector(0,0,-10));
        Presentation=NewObject<UHearthwardPresentationComponent>(Player);Player->AddInstanceComponent(Presentation);Presentation->RegisterComponent();
        World->InitializeActorsForPlay(FURL(),false);World->GetWorldSettings()->NotifyBeginPlay();World->Tick(LEVELTICK_All,.025f);
    }
    ~FFootContactWorld()
    {
        if(World->HasBegunPlay())World->EndPlay(EEndPlayReason::Quit);
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    bool Pose(const FFootClip& Clip,UAnimSequence*& Sequence)
    {
        Sequence=LoadObject<UAnimSequence>(nullptr,Clip.Package);
        auto* Class=LoadClass<ACharacter>(nullptr,Clip.CharacterClass);
        const auto* Defaults=Class?Cast<ACharacter>(Class->GetDefaultObject()):nullptr;
        auto* Mesh=Defaults?Defaults->GetMesh()->GetSkeletalMeshAsset():nullptr;
        if(!Sequence || !Mesh)return false;
        const auto* Capsule=Defaults->GetCapsuleComponent();
        Player->GetCapsuleComponent()->SetCapsuleSize(Capsule->GetUnscaledCapsuleRadius(),Capsule->GetUnscaledCapsuleHalfHeight());
        Player->SetActorLocation(FVector(0,0,Capsule->GetUnscaledCapsuleHalfHeight()));
        auto* Component=Player->GetMesh();Component->SetSkeletalMeshAsset(Mesh);
        Component->SetRelativeTransform(Defaults->GetMesh()->GetRelativeTransform());Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Component->PlayAnimation(Sequence,true);
        auto* Node=Component->GetSingleNodeInstance();if(!Node)return false;
        Node->SetPlaying(false);Node->SetPosition(Clip.LeftContact,false);
        for(int32 Step=0;Step<80;++Step)
        {
            Player->AddMovementInput(FVector::ForwardVector,1);
            Player->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
            if(Player->GetCharacterMovement()->IsMovingOnGround() && Player->GetCharacterMovement()->CurrentFloor.IsWalkableFloor()
                && Player->GetVelocity().SizeSquared2D()>KINDA_SMALL_NUMBER)break;
        }
        World->Tick(LEVELTICK_All,.025f);
        Component->TickAnimation(0,false);Component->RefreshBoneTransforms();
        return UGameplayStatics::GetPlayerPawn(World,0)==Player && Player->IsLocallyControlled() && World->HasBegunPlay()
            && Player->GetCharacterMovement()->IsMovingOnGround() && Player->GetCharacterMovement()->CurrentFloor.IsWalkableFloor();
    }
    int32 Sources() const
    {
        TArray<UAudioComponent*> Components;Player->GetComponents(Components);
        return Components.FilterByPredicate([](const auto* C){return IsValid(C) && C->ComponentTags.Contains(TEXT("Hearthward.Audio.effects"));}).Num();
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootContactGroundGuards099Test,"Hearthward.Iteration.Task099.FootContact.GroundGuards",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFootContactGroundGuards099Test::RunTest(const FString&)
{
    FFootContactWorld F;UAnimSequence* Sequence=nullptr;
    if(!TestTrue(TEXT("Real runtime foot pose is available"),F.Pose(Clips[0],Sequence)))return false;
    auto* Notify=NewObject<UHearthwardFootContactNotify>();Notify->FootBone=TEXT("foot_l");
    FHearthwardFootContactReceipt Receipt;
    TestTrue(TEXT("Grounded moving real foot has an actual walkable blocking floor"),FFootContactNotifyAccess::Read(Notify,F.Player->GetMesh(),Receipt));
    TestTrue(TEXT("Contact preserves actual source foot epoch and fresh success identity"),Receipt.Source.Get()==F.Player && Receipt.FootBone==TEXT("foot_l")
        && Receipt.Epoch==F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch() && Receipt.SuccessId.IsValid());
    TestTrue(TEXT("Sound source is the actual floor impact rather than the ankle"),FMath::IsNearlyZero(Receipt.Position.Z,.1f)
        && Receipt.Position.Z<F.Player->GetMesh()->GetSocketLocation(TEXT("foot_l")).Z);
    F.Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TestFalse(TEXT("Movement mode alone cannot invent foot contact"),FFootContactNotifyAccess::Read(Notify,F.Player->GetMesh(),Receipt));
    F.Floor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    F.Player->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    TestFalse(TEXT("Airborne nearby floor is not a step"),FFootContactNotifyAccess::Read(Notify,F.Player->GetMesh(),Receipt));
    if(!TestTrue(TEXT("Actual capsule physics returns to the walkable floor"),F.Pose(Clips[0],Sequence)))return false;
    F.Player->GetCharacterMovement()->StopMovementImmediately();
    TestFalse(TEXT("Stationary locomotion pose is not a step"),FFootContactNotifyAccess::Read(Notify,F.Player->GetMesh(),Receipt));
    if(!TestTrue(TEXT("Real input resumes grounded movement"),F.Pose(Clips[0],Sequence)))return false;
    Notify->FootBone=TEXT("missing_foot");
    TestFalse(TEXT("Missing bone cannot use an identity pose"),FFootContactNotifyAccess::Read(Notify,F.Player->GetMesh(),Receipt));
    Notify->FootBone=TEXT("foot_l");F.World->GetWorldSettings()->SetPauserPlayerState(F.World->SpawnActor<APlayerState>());
    TestTrue(TEXT("Fixture enters real paused world state"),F.World->IsPaused());
    TestFalse(TEXT("Paused contacts are refused"),FFootContactNotifyAccess::Read(Notify,F.Player->GetMesh(),Receipt));
    F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    const float HealthBefore=F.Survival->Health();
    if(!TestTrue(TEXT("Damage fixture starts with actual living health"),F.Survival->Alive() && HealthBefore>0 && FMath::IsFinite(HealthBefore)))return false;
    TestTrue(TEXT("Real accepted injury downs the foot source"),F.Survival->ReceiveDamage(HealthBefore,FGuid::NewGuid(),F.World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()));
    TestTrue(TEXT("Accepted injury actually leaves zero health and the downed state"),F.Survival->Health()==0
        && F.Survival->State.Life==EHearthwardLife::Downed && F.Survival->State.DownRemaining>0);
    TestFalse(TEXT("Downed source cannot produce a step"),FFootContactNotifyAccess::Read(Notify,F.Player->GetMesh(),Receipt));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootContactActualClipDispatch099Test,"Hearthward.Iteration.Task099.FootContact.ActualClipDispatch",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFootContactActualClipDispatch099Test::RunTest(const FString&)
{
    FFootContactWorld F;
    for(const auto& Clip:Clips)
    {
        UAnimSequence* Sequence=nullptr;if(!TestTrue(Clip.Package,F.Pose(Clip,Sequence)))return false;
        int32 Left=0,Right=0;
        for(const auto& Event:Sequence->Notifies)
            if(const auto* Contact=Cast<UHearthwardFootContactNotify>(Event.Notify))
            {
                Left+=Contact->FootBone==TEXT("foot_l");Right+=Contact->FootBone==TEXT("foot_r");
                TestFalse(TEXT("Gait follower cannot duplicate the leader contact"),Event.bTriggerOnFollower);
                TestEqual(TEXT("Contact uses the explicit gait weight threshold"),Event.TriggerWeightThreshold,.5f);
            }
        TestEqual(TEXT("Actual clip contains the two left contact candidates"),Left,2);
        TestEqual(TEXT("Actual clip contains the two right contact candidates"),Right,2);
        const int32 Before=F.Sources();
        // UE extracts the actual saved Notify and dispatches it; this test never
        // invokes Notify(), FootContactSucceeded(), or audio playback directly.
        F.Player->GetMesh()->GetSingleNodeInstance()->SetPositionWithPreviousTime(Clip.LeftContact+.002f,Clip.LeftContact-.002f,true);
        TestEqual(TEXT("Actual clip Notify creates one contact sound at its real blocking foot"),F.Sources(),Before+1);
        if(Sequence->GetName().EndsWith(TEXT("_Run")))
        {
            const auto* Seam=Sequence->Notifies.FindByPredicate([](const auto& Event)
            {
                const auto* Contact=Cast<UHearthwardFootContactNotify>(Event.Notify);
                return Contact && Contact->FootBone==TEXT("foot_r") && Event.GetTriggerTime()>0 && Event.GetTriggerTime()<.001f;
            });
            if(!TestNotNull(TEXT("Actual saved Run right seam is slightly after time zero"),Seam))return false;
            const float Time=Seam->GetTriggerTime();auto* Mesh=F.Player->GetMesh();auto* Node=Mesh->GetSingleNodeInstance();
            Node->SetPosition(Time,false);Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();
            const int32 BeforeSeam=F.Sources();
            Node->SetPositionWithPreviousTime(Time+.000025f,FMath::Max(0.f,Time-.000025f),true);
            TestEqual(TEXT("Actual saved right seam creates exactly one grounded sound"),F.Sources(),BeforeSeam+1);
        }
    }
    auto* Camp=F.World->SpawnActor<AActor>();auto* CampRoot=NewObject<USceneComponent>(Camp);
    Camp->AddInstanceComponent(CampRoot);Camp->SetRootComponent(CampRoot);CampRoot->RegisterComponent();Camp->SetActorLocation(FVector(0,200,0));
    auto* Stock=NewObject<UHearthwardInventoryComponent>(Camp);Camp->AddInstanceComponent(Stock);Stock->RegisterComponent();
    FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Brother=F.World->SpawnActor<AHearthwardCompanionFixture>(FVector(0,200,80),FRotator::ZeroRotator,Spawn);
    Brother->InitializeCompanion(Stock,Camp);Brother->SpawnDefaultController();
    auto* Sequence=LoadObject<UAnimSequence>(nullptr,Clips[2].Package);
    if(!TestNotNull(TEXT("Actual Brother Walk asset is loaded"),Sequence))return false;
    auto* Mesh=Brother->GetMesh();Mesh->PlayAnimation(Sequence,true);auto* Node=Mesh->GetSingleNodeInstance();
    if(!TestNotNull(TEXT("Actual Brother owns the live single-node animation instance"),Node))return false;
    Node->SetPlaying(false);Node->SetPosition(Clips[2].LeftContact,false);
    F.World->Tick(LEVELTICK_All,.025f);
    for(int32 Step=0;Step<80;++Step)
    {
        Brother->AddMovementInput(FVector::ForwardVector,1);Brother->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
        if(Brother->GetCharacterMovement()->IsMovingOnGround() && Brother->GetCharacterMovement()->CurrentFloor.IsWalkableFloor()
            && Brother->GetVelocity().SizeSquared2D()>KINDA_SMALL_NUMBER)break;
    }
    Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();
    if(!TestTrue(TEXT("Actual Brother has real controller, movement and walkable blocking floor"),Brother->GetController()
        && Mesh->GetOwner()==Brother && Brother->GetCharacterMovement()->IsMovingOnGround()
        && Brother->GetCharacterMovement()->CurrentFloor.IsWalkableFloor() && Brother->GetVelocity().SizeSquared2D()>KINDA_SMALL_NUMBER))return false;
    auto* AI=F.World->GetSubsystem<UHearthwardLocalAISubsystem>();
    if(!TestTrue(TEXT("Public deterministic inventory query binds the current player and actual Brother"),AI->QueryInventory(F.Player,Brother,TEXT("wood"))))return false;
    const int32 BeforeBrother=F.Sources();
    Node->SetPositionWithPreviousTime(Clips[2].LeftContact+.002f,Clips[2].LeftContact-.002f,true);
    TestEqual(TEXT("Actual Brother Notify source creates one sound through the player consumer"),F.Sources(),BeforeBrother+1);
    return true;
}
#endif
