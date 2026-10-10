#include "HearthwardCompanionFixture.h"
#include "../Equipment/HearthwardAxeGrip.h"
#include "../Inventory/HearthwardHarvestTools.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Experience/HearthwardTraversalComponent.h"
#include "HearthwardCompanionNavigationComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "../Animation/HearthwardBrotherAnimInstance.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../AI/HearthwardNPCPerception.h"
#include "../AI/HearthwardAgentRecovery.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Building/HearthwardWorkshopService.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Nature/HearthwardNatureActor.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

namespace
{
UHearthwardBuildingComponent* WorkshopRegistry(UWorld* World);
int32 TransferableCargo(const FHearthwardInventorySnapshot& CampSnapshot,
    const FHearthwardInventorySnapshot& BrotherSnapshot,const FHearthwardInventorySnapshot& PlayerSnapshot,
    FName Item,int32 Limit)
{
    FHearthwardInventoryState CampState(true),BrotherState,PlayerState;
    if(Limit<=0 || !CampState.Restore(CampSnapshot) || !BrotherState.Restore(BrotherSnapshot)
        || !PlayerState.Restore(PlayerSnapshot))return 0;
    int32 Low=0,High=Limit;
    while(Low<High)
    {
        const int32 Mid=Low+(High-Low+1)/2;
        auto CampAfter=CampState,BrotherAfter=BrotherState,PlayerAfter=PlayerState;
        if(CampAfter.TransferTo(BrotherAfter,Item,Mid)==EHearthwardInventoryResult::Success
            && BrotherAfter.TransferTo(PlayerAfter,Item,Mid)==EHearthwardInventoryResult::Success)Low=Mid;
        else High=Mid-1;
    }
    return Low;
}
int32 TransferableHandoff(const FHearthwardInventorySnapshot& SourceSnapshot,
    const FHearthwardInventorySnapshot& TargetSnapshot,FName Item,int32 Limit,bool SourceIsStorage=false)
{
    FHearthwardInventoryState SourceState(SourceIsStorage),TargetState;
    if(Limit<=0 || !SourceState.Restore(SourceSnapshot) || !TargetState.Restore(TargetSnapshot))return 0;
    int32 Low=0,High=Limit;
    while(Low<High)
    {
        const int32 Mid=Low+(High-Low+1)/2;
        auto SourceAfter=SourceState,TargetAfter=TargetState;
        if(SourceAfter.TransferTo(TargetAfter,Item,Mid)==EHearthwardInventoryResult::Success)Low=Mid;
        else High=Mid-1;
    }
    return Low;
}
}

AHearthwardCompanionFixture::AHearthwardCompanionFixture(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UHearthwardMovementComponent>(ACharacter::CharacterMovementComponentName))
{
    CreateDefaultSubobject<UHearthwardSurvivalComponent>(TEXT("Survival"));
    CreateDefaultSubobject<UHearthwardTraversalComponent>(TEXT("Traversal"));
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    auto* Capsule = GetCapsuleComponent();
    Capsule->InitCapsuleSize(30.0f, 80.0f);
    Capsule->SetCollisionProfileName(TEXT("Pawn"));
    // The companion must not push the follow camera into the player's head at camp.
    Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    Capsule->SetCanEverAffectNavigation(false);
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 500, 0);
    GetCharacterMovement()->MaxWalkSpeed = 180;
    GetCharacterMovement()->MaxStepHeight = 45;
    GetCharacterMovement()->SetWalkableFloorAngle(45);
    GetCharacterMovement()->MaxSwimSpeed = 300;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanSwim = true;
    bUseControllerRotationYaw = false;
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> BrotherMesh(TEXT("/Game/Characters/Brother/UE5/SK_Brother.SK_Brother"));
    GetMesh()->SetSkeletalMesh(BrotherMesh.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BrotherCloth(TEXT("/Game/Hearthward/Assets/TASK-095/Costumes/M_Brother_CoarseCloth.M_Brother_CoarseCloth"));
    GetMesh()->SetMaterial(0,BrotherCloth.Object);
    // The imported mesh is 97.864 cm tall; fit the existing 160 cm companion capsule.
    GetMesh()->SetRelativeScale3D(FVector(160.f / 97.863766f));
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -80.f));
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    GetMesh()->SetCanEverAffectNavigation(false);
    GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    GetMesh()->SetCanEverAffectNavigation(false);
    GetMesh()->SetAnimInstanceClass(UHearthwardBrotherAnimInstance::StaticClass());
    Bag = CreateDefaultSubobject<UHearthwardInventoryComponent>(TEXT("FixtureBag"));
    Action = CreateDefaultSubobject<UHearthwardTimedActionComponent>(TEXT("FixtureGatherTimer"));
    Navigation = CreateDefaultSubobject<UHearthwardCompanionNavigationComponent>(TEXT("CompanionNavigation"));
    Tags.Add(TEXT("Hearthward.Companion"));
    HeldWeapon=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldWeapon"));
    HeldWeapon->SetupAttachment(GetMesh(),TEXT("hand_r"));HeldWeapon->SetAbsolute(false,false,true);
    HeldWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);HeldWeapon->SetCanEverAffectNavigation(false);HeldWeapon->SetVisibility(false);
    const TCHAR* Families[]={TEXT("shortblade"),TEXT("longblade"),TEXT("spear"),TEXT("blunt")};
    const TCHAR* Names[]={TEXT("Shortblade"),TEXT("Longblade"),TEXT("Spear"),TEXT("Waraxe")};
    for(int32 I=0;I<UE_ARRAY_COUNT(Families);++I)
    {
        const FString Path=FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-095/Weapons/%s/SM_%s_Practical"),Names[I],Names[I]);
        ConstructorHelpers::FObjectFinder<UStaticMesh> Asset(*Path);WeaponMeshes.Add(Families[I],Asset.Object);
    }
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Axe(TEXT("/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe"));
    WeaponMeshes.Add(TEXT("axe"),Axe.Object);
    ensureMsgf(HearthwardAxeGrip::Build(BrotherMesh.Object, GetMesh()->GetRelativeScale3D(), Axe.Object, AxeGrip),
        TEXT("Stone axe requires measured grip sockets and the right-hand reference bones"));
    const auto& Ref=BrotherMesh.Object->GetRefSkeleton();
    const auto Bone=[&](FName Name)
    {
        int32 Index=Ref.FindBoneIndex(Name);FTransform Result=Ref.GetRefBonePose()[Index];
        while((Index=Ref.GetParentIndex(Index))!=INDEX_NONE)Result*=Ref.GetRefBonePose()[Index];
        return Result;
    };
    const FVector Grip=(Bone(TEXT("thumb_02_r")).GetLocation()+Bone(TEXT("middle_02_r")).GetLocation())*.5;
    const FVector Direction=(Bone(TEXT("index_01_r")).GetLocation()-Bone(TEXT("pinky_01_r")).GetLocation()).GetSafeNormal();
    WeaponGrip=FTransform(FRotationMatrix::MakeFromZX(Direction,FVector::UpVector).ToQuat(),Grip).GetRelativeTransform(Bone(TEXT("hand_r")));
    WeaponGrip.SetScale3D(FVector::OneVector);
}

void AHearthwardCompanionFixture::BeginPlay()
{
    Super::BeginPlay();Bag->OnInventoryChanged.AddDynamic(this,&AHearthwardCompanionFixture::RefreshHeldWeapon);RefreshHeldWeapon();
}

void AHearthwardCompanionFixture::RefreshHeldWeapon()
{
    const auto* Equipped=Bag->FindInstance(Bag->EquippedInstance(TEXT("weapon")));
    const auto* Survival=FindComponentByClass<UHearthwardSurvivalComponent>();
    const bool Visible=Equipped && Equipped->Durability>0 && (!Survival->Enabled() || Survival->Alive())
        && Survival->RescueElapsed()<0 && Action->GetStatus()!=EHearthwardTimedActionStatus::Running;
    HeldWeapon->SetVisibility(Visible);if(!Visible || DisplayedWeapon==Equipped->Definition)return;
    const bool Axe=Equipped->Definition==TEXT("axe");
    const auto Row=HearthwardData::Find(TEXT("items"),Equipped->Definition.ToString());
    HeldWeapon->SetStaticMesh(WeaponMeshes.FindRef(Axe?FName(TEXT("axe")):FName(*HearthwardData::Text(Row,TEXT("combatClass")))));
    HeldWeapon->EmptyOverrideMaterials();
    HeldWeapon->SetRelativeTransform(Axe?AxeGrip:WeaponGrip);
    if(!Axe)if(auto* Material=HeldWeapon->CreateDynamicMaterialInstance(0))
        Material->SetScalarParameterValue(TEXT("MetalFinish"),HearthwardData::Number(Row,TEXT("stage"),1)>1?1.f:0.f);
    DisplayedWeapon=Equipped->Definition;
}

void AHearthwardCompanionFixture::InitializeFixture(UHearthwardInventoryComponent* Resource, AActor* CampActor)
{
#if !UE_BUILD_SHIPPING
    InitializeCompanion(Resource, CampActor);
    Tags.Add(TEXT("Hearthward.Companion.PROTOTYPE_ONLY"));
#endif
}

void AHearthwardCompanionFixture::InitializeCompanion(UHearthwardInventoryComponent* Resource, AActor* CampActor)
{
    Source = Resource;
    Camp = CampActor;
    bSourceSafe = true;
    bFixtureEnabled = true;
    SetActorTickEnabled(true);
    if (auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        Nav->RegisterNavigationInvoker(this,4000,5000);
        if (auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0)) Nav->RegisterNavigationInvoker(Player,4000,5000);
        Nav->RegisterNavigationInvoker(CampActor,2000,3000);
    }
}

bool AHearthwardCompanionFixture::CanCommunicate(AActor* Speaker) const
{
    const auto* Survival=FindComponentByClass<UHearthwardSurvivalComponent>();
    if(Survival->Enabled() && !Survival->Alive()) return false;
    return bFixtureEnabled && IsValid(Speaker) && !Speaker->IsActorBeingDestroyed() && Speaker != this
        && Speaker->GetWorld() == GetWorld() && FVector::DistSquared(Speaker->GetActorLocation(), GetActorLocation()) <= FMath::Square(3000.0);
}

FHearthwardCommandTicket AHearthwardCompanionFixture::Request(AActor* Speaker, const FString& PlayerStatement)
{
    if (bSettling || !CanCommunicate(Speaker) || GetWorld()->IsPaused()) return {};
    RequestSpeaker = Speaker;
    Statement = PlayerStatement;
    return Command.Request(GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch());
}

EHearthwardProposalResult AHearthwardCompanionFixture::Submit(AActor* Speaker, FHearthwardCommandTicket Ticket,
    FName ItemId, int32 Quantity, const TArray<FName>& Steps)
{
    FHearthwardAgentGoal Goal;
    Goal.Intent=TEXT("collect");Goal.Item=ItemId;Goal.Quantity=Quantity;
    Goal.QuantityMode=TEXT("additional_acquired");Goal.SourceRef=TEXT("S1");
    return AcceptGoal(Speaker,Ticket,ItemId,Quantity,Steps,Goal);
}

EHearthwardProposalResult AHearthwardCompanionFixture::AcceptGoal(AActor* Speaker, FHearthwardCommandTicket Ticket,
    FName ItemId, int32 Quantity, const TArray<FName>& Steps, const FHearthwardAgentGoal& Goal)
{
    using R = EHearthwardProposalResult;
    if (bSettling || !bFixtureEnabled) return R::Unavailable;
    if (!CanCommunicate(Speaker)) return R::OutOfRange;
    if (RequestSpeaker.Get() != Speaker) return R::Stale;

    const auto Safety=HearthwardPerception::Evaluate(HearthwardPerception::Capture(this),Goal);
    if(!Safety.IsAllowed())
        return Safety.Verdict==EHearthwardNPCSafetyVerdict::Unsafe ? R::Unsafe : R::Unavailable;

    const auto Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    const FGuid SupersededId=Command.IsCurrent(Epoch)?Command.GetActive().Id:FGuid();
    const FName SupersededItem=Command.GetItem();
    const int32 SupersededDelivered=Command.GetDelivered();
    const FHearthwardAgentGoal SupersededGoal=Command.Goal;
    const int32 SupersededBatchBaseline=CampBatchBaseline;
    const auto Result = Goal.Intent==TEXT("nature_care") || Goal.Intent==TEXT("nature_collect") || Goal.Intent==TEXT("escort") || Goal.Intent==TEXT("hunt") || Goal.Intent==TEXT("fish") || Goal.Intent==TEXT("capture") || Goal.Intent==TEXT("camp_batch")
        ? Command.AcceptNature(Ticket,Epoch,Goal)
        : Command.Accept(Ticket,Epoch,ItemId,Quantity,Steps);
    if (Result == R::Accepted)
    {
        if(SupersededId.IsValid() && SupersededGoal.Intent==TEXT("camp_batch") && SupersededBatchBaseline>=0)
        {
            auto* R=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Regions.FindByPredicate(
                [&](const auto& X){return X.Facility==SupersededGoal.Station && X.Job==SupersededGoal.Item;});
            if(R && R->BatchStopAt==SupersededBatchBaseline+SupersededGoal.Quantity)R->Enabled=false;
        }
        if(SupersededId.IsValid())
        {
            FHearthwardNPCEvent E;E.Id=FGuid::NewGuid();E.Command=SupersededId;
            E.Campaign=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetCampaignId();
            E.Kind=TEXT("cancelled");E.Item=SupersededItem;E.Count=SupersededDelivered;
            E.Reason=TEXT("superseded_by_new_order");
            E.At=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds;
            GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->RecordEvent(E);
        }
        StopNavigation();
        Action->InterruptAction();
        BlockReason.Reset();
        Spent.Reset();AppliedOperations.Reset();Receipts.Reset();NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();CancelMeleeAttack();NextHuntAttackAt=0;CampBatchBaseline=-1;
        Command.Goal=Goal;
        if(Goal.Intent==TEXT("collect") || Goal.Intent==TEXT("give") || (Goal.Intent==TEXT("store") && Goal.SourceRef==TEXT("bag")))
        {
            // Physical cargo retained from a cancelled/superseded command may satisfy a later request,
            // but only up to the new requested quantity. The surplus remains in the bag and is never
            // silently attributed to the new command.
            const int32 Adopted=FMath::Min(Bag->GetItemCount(Goal.Item),Command.GetRequested());
            if(Adopted>0)
            {
                Command.RecordAcquisition(Adopted);
                Event(TEXT("retained_adopted"),Goal.Item,Adopted,TEXT("physical_cargo_reused"));
            }
        }
        if(Goal.Intent==TEXT("store") && Goal.SourceRef==TEXT("player_bag"))
        {
            TGuardValue<bool> Guard(bSettling,true);
            auto* PlayerBag=Speaker->FindComponentByClass<UHearthwardInventoryComponent>();
            if(!PlayerBag || PlayerBag->TransferTo(Bag,Goal.Item,Goal.Quantity)!=EHearthwardInventoryResult::Success)
            {Command.Cancel();Phase=EHearthwardCompanionPhase::Cancelled;BlockReason=TEXT("PLAYER_HANDOFF_FAILED");return R::Unavailable;}
            Command.RecordAcquisition(Goal.Quantity);
            Event(TEXT("handoff"),Goal.Item,Goal.Quantity,TEXT("player_bag"));
        }
        if(!BuildExecutionPlan(true))
        {
            Command.Cancel();
            return R::Unsupported;
        }
    }
    return Result;
}

bool AHearthwardCompanionFixture::Cancel(AActor* Speaker,bool bAllowPaused)
{
    if (bSettling || !CanCommunicate(Speaker) || (GetWorld()->IsPaused() && !bAllowPaused)) return false;
    CancelMeleeAttack();
    if(Command.IsCurrent(GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())) Event(TEXT("cancelled"),Command.GetItem(),Command.GetDelivered());
    StopCampBatch();
    Command.Cancel();
    Execution.Reset();
    StopNavigation();
    Action->InterruptAction();
    Phase = EHearthwardCompanionPhase::Cancelled;
    return true;
}

bool AHearthwardCompanionFixture::PerformingCampBatch(FName Region) const
{
    if(Command.Goal.Intent!=TEXT("camp_batch") || Phase!=EHearthwardCompanionPhase::CampBatchWorking || CampBatchBaseline<0
        || !Command.IsCurrent(GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()))return false;
    const auto* R=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Regions.FindByPredicate([&](const auto& X){return X.Id==Region;});
    return R && R->Facility==Command.Goal.Station && R->Job==Command.Goal.Item && R->Brother
        && R->BatchStopAt==CampBatchBaseline+Command.Goal.Quantity;
}

bool AHearthwardCompanionFixture::CapturingAnimal(FGuid Animal) const
{
    return Command.Goal.Intent==TEXT("capture") && Command.Goal.Station==Animal
        && Phase==EHearthwardCompanionPhase::Gathering && Execution.bStarted && Action
        && Action->GetStatus()==EHearthwardTimedActionStatus::Running;
}

void AHearthwardCompanionFixture::StopCampBatch()
{
    if(Command.Goal.Intent!=TEXT("camp_batch") || CampBatchBaseline<0)return;
    auto* Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    if(auto* R=Camps->State.Regions.FindByPredicate([&](const auto& X){return X.Facility==Command.Goal.Station && X.Job==Command.Goal.Item;}))
        if(R->BatchStopAt==CampBatchBaseline+Command.Goal.Quantity)R->Enabled=false;
}

bool AHearthwardCompanionFixture::IsProposalCurrent(AActor* Speaker, const FHearthwardCommandTicket& Ticket) const
{
    return CanCommunicate(Speaker) && RequestSpeaker.Get() == Speaker
        && Command.IsPending(Ticket, GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch());
}

bool AHearthwardCompanionFixture::IsSourceValid() const
{
    return IsValid(Source) && IsValid(Source->GetOwner()) && !Source->GetOwner()->IsActorBeingDestroyed()
        && Source->GetWorld() == GetWorld();
}

FString AHearthwardCompanionFixture::GetExecutionAction() const
{
    if(Execution.bRecoveryToCamp)return TEXT("Recovery:Camp");
    if(Execution.bAdaptiveRecovery)
    {
        if(const auto* Current=Execution.Current())
            return TEXT("Recovery:Replan->") + HearthwardPlan::ActionName(*Current);
        return TEXT("Recovery:Replan");
    }
    if(const auto* Current=Execution.Current())return HearthwardPlan::ActionName(*Current);
    return TEXT("None");
}

bool AHearthwardCompanionFixture::BuildExecutionPlan(bool PreferReturnForExistingCargo)
{
    FString Error;
    Execution.Reset();
    if(!HearthwardPlan::Build(Command.Goal,Execution.Plan,Error))
    {
        BlockReason=Error.IsEmpty()?TEXT("NO_EXECUTABLE_PLAN"):Error;
        return false;
    }
    Execution.Cursor=0;
    if(PreferReturnForExistingCargo && ((Command.Goal.Intent==TEXT("collect") && Bag->GetWeight()>0)
        || (Command.Goal.Intent==TEXT("nature_collect") && Command.Carried>0)))
    {
        const int32 Return=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Camp);
        if(Return!=INDEX_NONE)Execution.Cursor=Return;
    }
    SyncPhaseFromExecution();
    return true;
}

void AHearthwardCompanionFixture::RestoreExecutionPlan()
{
    Execution.Reset();
    auto* Storage=GetWorld()?GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>():nullptr;
    if(!Storage || !Command.IsCurrent(Storage->GetTimelineEpoch()) || Command.Goal.Intent.IsNone())return;

    FString Error;
    if(!HearthwardPlan::Build(Command.Goal,Execution.Plan,Error))return;

    using P=EHearthwardCompanionPhase;
    switch(Phase)
    {
    case P::GoingToSource:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,
            Command.Goal.Intent==TEXT("escort")?EHearthwardAgentTarget::Person:
            (Command.Goal.Intent==TEXT("nature_care") || Command.Goal.Intent==TEXT("nature_collect") || Command.Goal.Intent==TEXT("hunt") || Command.Goal.Intent==TEXT("fish") || Command.Goal.Intent==TEXT("capture"))?EHearthwardAgentTarget::Nature:EHearthwardAgentTarget::Source);break;
    case P::Gathering:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,
            (Command.Goal.Intent==TEXT("nature_care") || Command.Goal.Intent==TEXT("nature_collect") || Command.Goal.Intent==TEXT("fish") || Command.Goal.Intent==TEXT("capture"))?EHearthwardAgentActionType::CommitNature:EHearthwardAgentActionType::Gather,
            (Command.Goal.Intent==TEXT("nature_care") || Command.Goal.Intent==TEXT("nature_collect") || Command.Goal.Intent==TEXT("fish") || Command.Goal.Intent==TEXT("capture"))?EHearthwardAgentTarget::Nature:EHearthwardAgentTarget::Source);
        Execution.bStarted=true;break;
    case P::LeadingAnimal:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::LeadAnimal,EHearthwardAgentTarget::Nature);break;
    case P::CampBatchWorking:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MonitorCampBatch,EHearthwardAgentTarget::Workshop);break;
    case P::TakingMaterials:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::TakeMaterials,EHearthwardAgentTarget::Camp);break;
    case P::TakingCargo:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::Withdraw,EHearthwardAgentTarget::Camp);break;
    case P::GoingToPlayer:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,
            Command.Goal.Intent==TEXT("escort")?EHearthwardAgentActionType::Escort:EHearthwardAgentActionType::MoveTo,
            Command.Goal.Intent==TEXT("escort")?EHearthwardAgentTarget::Person:EHearthwardAgentTarget::Player);break;
    case P::HandingOff:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::Handoff,EHearthwardAgentTarget::Player);break;
    case P::GoingToWorkshop:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Workshop);break;
    case P::Returning:
        Execution.Cursor=HearthwardPlan::FindLast(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Camp);break;
    case P::ReturningBlocked:
    case P::HoldingSafely:
        Execution.bRecoveryToCamp=true;
        Execution.Cursor=(Command.Goal.Intent==TEXT("give") || Command.Goal.Intent==TEXT("fetch") || Command.Goal.Intent==TEXT("receive"))?0:(Command.Carried>0
            ? HearthwardPlan::FindLast(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Camp)
            : 0);
        break;
    case P::WaitingAtCamp:
        if(Command.Goal.Intent==TEXT("collect") || Command.Goal.Intent==TEXT("nature_collect"))
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,
                Command.Goal.Intent==TEXT("nature_collect")?EHearthwardAgentTarget::Nature:EHearthwardAgentTarget::Source);
        else if(Command.Goal.Intent==TEXT("give") || Command.Goal.Intent==TEXT("receive"))
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Player);
        else if(Command.Goal.Intent==TEXT("retrieve") || Command.Goal.Intent==TEXT("fetch") || Command.Goal.SourceRef==TEXT("camp"))
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Camp);
        else
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Workshop);
        break;
    default:
        Execution.Reset();
        return;
    }
    if(!Execution.Plan.Actions.IsValidIndex(Execution.Cursor))Execution.Reset();
}

AActor* AHearthwardCompanionFixture::ResolveActionTarget(const FHearthwardAgentAction& Current) const
{
    switch(Current.Target)
    {
    case EHearthwardAgentTarget::Source:
        return IsSourceValid()?Source->GetOwner():nullptr;
    case EHearthwardAgentTarget::Camp:
        return IsValid(Camp)&&!Camp->IsActorBeingDestroyed()?Camp.Get():nullptr;
    case EHearthwardAgentTarget::Workshop:
    {
        auto* Registry=WorkshopRegistry(GetWorld());
        return Registry?Registry->ResolveFacility(Command.Goal.Station):nullptr;
    }
    case EHearthwardAgentTarget::Nature:
        return GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->Actor(Command.Goal.Station);
    case EHearthwardAgentTarget::Person:
        return GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Actor(Command.Goal.Item);
    case EHearthwardAgentTarget::Player:
        return UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    default:
        return nullptr;
    }
}

bool AHearthwardCompanionFixture::ActionRequiresSafety(const FHearthwardAgentAction& Current) const
{
    if(Current.Type==EHearthwardAgentActionType::Deposit || (Command.Goal.Intent==TEXT("hunt") && Current.Type==EHearthwardAgentActionType::Hunt))return false;
    if(Current.Type==EHearthwardAgentActionType::MoveTo && Current.Target==EHearthwardAgentTarget::Camp)return false;
    return true;
}

void AHearthwardCompanionFixture::SyncPhaseFromExecution()
{
    if(Execution.bRecoveryToCamp)
    {
        Phase=EHearthwardCompanionPhase::ReturningBlocked;
        return;
    }
    const auto* Current=Execution.Current();
    if(!Current)return;
    using A=EHearthwardAgentActionType;
    using T=EHearthwardAgentTarget;
    if(Current->Type==A::MoveTo && (Current->Target==T::Source || Current->Target==T::Nature || Current->Target==T::Person))Phase=EHearthwardCompanionPhase::GoingToSource;
    else if(Current->Type==A::Gather || Current->Type==A::CommitNature)Phase=EHearthwardCompanionPhase::Gathering;
    else if(Current->Type==A::MoveTo && Current->Target==T::Camp)Phase=EHearthwardCompanionPhase::Returning;
    else if(Current->Type==A::Deposit)Phase=EHearthwardCompanionPhase::Returning;
    else if(Current->Type==A::TakeMaterials)Phase=EHearthwardCompanionPhase::TakingMaterials;
    else if(Current->Type==A::Withdraw)Phase=EHearthwardCompanionPhase::TakingCargo;
    else if(Current->Type==A::MoveTo && Current->Target==T::Player)Phase=EHearthwardCompanionPhase::GoingToPlayer;
    else if(Current->Type==A::Escort)Phase=EHearthwardCompanionPhase::GoingToPlayer;
    else if(Current->Type==A::Hunt)Phase=EHearthwardCompanionPhase::GoingToSource;
    else if(Current->Type==A::LeadAnimal)Phase=EHearthwardCompanionPhase::LeadingAnimal;
    else if(Current->Type==A::MonitorCampBatch)Phase=EHearthwardCompanionPhase::CampBatchWorking;
    else if(Current->Type==A::Handoff)Phase=EHearthwardCompanionPhase::HandingOff;
    else if((Current->Type==A::MoveTo && Current->Target==T::Workshop) || Current->Type==A::CommitWorkshop)
        Phase=EHearthwardCompanionPhase::GoingToWorkshop;
}

void AHearthwardCompanionFixture::AdvanceExecution()
{
    Execution.bStarted=false;
    ++Execution.Cursor;
    if(Execution.Plan.Actions.IsValidIndex(Execution.Cursor))
    {
        NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();
        SyncPhaseFromExecution();
        return;
    }

    if((Command.Goal.Intent==TEXT("collect") || Command.Goal.Intent==TEXT("nature_collect") || Command.Goal.Intent==TEXT("retrieve")
        || (Command.Goal.Intent==TEXT("give") && Command.Carried>0) || Command.Goal.Intent==TEXT("fetch")
        || Command.Goal.Intent==TEXT("receive")) && Command.GetDelivered()<Command.GetRequested())
    {
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,
            (Command.Goal.Intent==TEXT("give") || Command.Goal.Intent==TEXT("receive"))?EHearthwardAgentTarget::Player:
            (Command.Goal.Intent==TEXT("retrieve") || Command.Goal.Intent==TEXT("fetch"))?EHearthwardAgentTarget::Camp:
            Command.Goal.Intent==TEXT("nature_collect")?EHearthwardAgentTarget::Nature:EHearthwardAgentTarget::Source);
        NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();
        SyncPhaseFromExecution();
        return;
    }

    if(Command.GetDelivered()==Command.GetRequested())
    {
        Execution.Reset();
        Phase=EHearthwardCompanionPhase::Completed;
        BlockReason.Reset();
        Event(TEXT("completed"),Command.GetItem(),Command.GetDelivered());
    }
    else
    {
        Execution.Reset();
        Phase=EHearthwardCompanionPhase::WaitingAtCamp;
        BlockReason=TEXT("PLAN_INCOMPLETE");
    }
}


bool AHearthwardCompanionFixture::IsAtCamp() const
{
    return Navigation && Navigation->IsAt(Camp);
}

bool AHearthwardCompanionFixture::PlayerAtTaskCamp(const AActor* Player) const
{
    if(!IsValid(Player) || Player->IsActorBeingDestroyed() || Player->GetWorld()!=GetWorld()
        || !IsValid(Camp) || Camp->IsActorBeingDestroyed())return false;
    const auto& Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    const FName Site=Camps.CampAt(Camp->GetActorLocation());
    return !Site.IsNone() && Site==Camps.CampAt(Player->GetActorLocation());
}

bool AHearthwardCompanionFixture::At(const AActor* Target) const
{
    return Navigation && Navigation->IsAt(Target);
}

bool AHearthwardCompanionFixture::MoveTowards(const AActor* Target, float DeltaSeconds,float AcceptanceRadius)
{
    if (!IsValid(Target) || Target->IsActorBeingDestroyed()) return false;
    const double Now=GetWorld()->GetTimeSeconds();
    if(FVector::Dist(LastProgressPosition,Target->GetActorLocation())-FVector::Dist(GetActorLocation(),Target->GetActorLocation())>20)
    {LastProgressPosition=GetActorLocation();LastProgressAt=Now;}
    if(Now-LastProgressAt>HearthwardAgent::Policy(TEXT("no_progress_seconds"))) return false;
    if(NavigationFailures>HearthwardAgent::Policy(TEXT("max_navigation_retries"))) return false;
    const bool Attempt=Navigation && Navigation->IsRetryReady();
    const float Speed=HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("tuning")),TEXT("sprintSpeed"))
        *Bag->GetMoveSpeedMultiplier();
    if(NavigateTo(const_cast<AActor*>(Target),Speed,AcceptanceRadius))return true;
    if(Attempt) ++NavigationFailures;
    return NavigationFailures<=HearthwardAgent::Policy(TEXT("max_navigation_retries"));
}

void AHearthwardCompanionFixture::StopNavigation()
{
    if(Navigation)Navigation->Stop();
}

bool AHearthwardCompanionFixture::NavigateTo(AActor* Target, float Speed, float AcceptanceRadius)
{
    return Navigation && Navigation->MoveToActor(Target,Speed,AcceptanceRadius);
}

bool AHearthwardCompanionFixture::NavigateToLocation(const FVector& Location,float Speed,float AcceptanceRadius)
{
    return Navigation && Navigation->MoveToLocation(Location,Speed,AcceptanceRadius);
}

void AHearthwardCompanionFixture::ReturnBlocked(const FString& Reason)
{
    StopCampBatch();
    if (!Execution.bRecoveryToCamp)
    {
        StopNavigation();NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();
        Event(TEXT("blocked"),Command.GetItem(),Command.GetDelivered(),Reason);
    }
    Action->InterruptAction();
    Execution.bStarted=false;
    Execution.bAdaptiveRecovery=false;
    Execution.AdaptiveRetryAt=0.0;
    Execution.LastRecoveryReason=Reason;
    Execution.bRecoveryToCamp=true;
    Phase = EHearthwardCompanionPhase::ReturningBlocked;
    BlockReason = Reason;
}

void AHearthwardCompanionFixture::HandleExecutionFailure(const FString& Reason)
{
    CancelMeleeAttack();
    if(Command.Goal.Intent==TEXT("camp_batch"))
    {
        StopCampBatch();StopNavigation();Action->InterruptAction();
        Execution.bStarted=false;Execution.bAdaptiveRecovery=false;Execution.bRecoveryToCamp=false;
        Phase=EHearthwardCompanionPhase::HoldingSafely;BlockReason=Reason;
        Event(TEXT("blocked"),Command.GetItem(),Command.GetDelivered(),Reason);return;
    }
    const auto* Current=Execution.Current();
    if(!Current)
    {
        ReturnBlocked(Reason);
        return;
    }

    FHearthwardAgentRecoveryContext Context;
    Context.FailedAction=*Current;
    Context.FailureReason=Reason;
    Context.bHasCargo=Command.Carried>0;
    Context.bCampAvailable=IsValid(Camp) && !Camp->IsActorBeingDestroyed();
    Context.bSourceAvailable=IsSourceValid() && Source->GetItemCount(Command.GetItem())>0;
    auto* Registry=WorkshopRegistry(GetWorld());
    Context.bStationAvailable=(Registry && Registry->ResolveFacility(Command.Goal.Station))
        || (Command.Goal.Intent==TEXT("nature_collect")
            && IsValid(GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->Actor(Command.Goal.Station)));
    Context.AdaptiveAttempts=Execution.AdaptiveRecoveryAttempts;
    Context.MaxAdaptiveAttempts=HearthwardAgent::Policy(TEXT("max_adaptive_replans"));

    const auto Decision=HearthwardRecovery::Decide(Context);
    if(Decision.Mode==EHearthwardAgentRecoveryMode::RetryCurrent
        || Decision.Mode==EHearthwardAgentRecoveryMode::RewindToMove)
    {
        int32 ResumeCursor=Execution.Cursor;
        if(Decision.Mode==EHearthwardAgentRecoveryMode::RewindToMove)
            ResumeCursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,Decision.RewindTarget);

        if(!Execution.Plan.Actions.IsValidIndex(ResumeCursor))
        {
            ReturnBlocked(Reason);
            return;
        }

        StopNavigation();
        Action->InterruptAction();
        Execution.Cursor=ResumeCursor;
        Execution.bStarted=false;
        Execution.bRecoveryToCamp=false;
        Execution.bAdaptiveRecovery=true;
        ++Execution.AdaptiveRecoveryAttempts;
        Execution.AdaptiveRetryAt=GetWorld()->GetTimeSeconds()
            + double(HearthwardAgent::Policy(TEXT("adaptive_replan_delay_ms")))/1000.0;
        Execution.LastRecoveryReason=Reason;
        NavigationFailures=0;
        LastProgressAt=GetWorld()->GetTimeSeconds();
        LastProgressPosition=GetActorLocation();
        BlockReason=TEXT("REPLANNING：")+Reason;
        Event(TEXT("replanned"),Command.GetItem(),Command.GetDelivered(),Reason);
        SyncPhaseFromExecution();
        return;
    }

    if(Decision.Mode==EHearthwardAgentRecoveryMode::Hold)
    {
        StopNavigation();
        Action->InterruptAction();
        Execution.bStarted=false;
        Execution.bAdaptiveRecovery=false;
        Execution.bRecoveryToCamp=false;
        Execution.AdaptiveRetryAt=0.0;
        Execution.LastRecoveryReason=Reason;
        Phase=EHearthwardCompanionPhase::HoldingSafely;
        BlockReason=Reason;
        Event(TEXT("blocked"),Command.GetItem(),Command.GetDelivered(),Reason);
        return;
    }

    ReturnBlocked(Reason);
}

void AHearthwardCompanionFixture::Deposit()
{
    if (!At(Camp)) return;
    StopNavigation();
    const bool bRecovery=Execution.bRecoveryToCamp;
    auto* Storage = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    const auto Ticket = Command.GetActive();
    TGuardValue<bool> Guard(bSettling, true);
    if(Command.Goal.Intent==TEXT("retrieve") && !ReconcileMissingCargo())
    {ReturnBlocked(TEXT("CARGO_ACCOUNTING_FAILED"));return;}
    for (const auto& Item : HearthwardBasicItems())
    {
        if (!Command.IsCurrent(Storage->GetTimelineEpoch()))
        {
            Command.Cancel();Execution.Reset();
            Phase = EHearthwardCompanionPhase::Cancelled;
            return;
        }
        int32 Count = Bag->GetItemCount(Item.Id);
        const int32 Owned=Item.Id==Command.GetItem()?FMath::Min(Count,Command.Carried):0;
        if(Item.IsInstance() && Item.Id!=Command.GetItem())continue;
        if(Command.Goal.Intent!=TEXT("collect") || Item.Id==Command.GetItem())Count=Owned;
        if (Count == 0) continue;
        const FGuid Op=Command.Goal.Intent==TEXT("retrieve")?FGuid::NewGuid():
            FGuid(Ticket.Id.A,Ticket.Id.B,Ticket.Id.C^0xDE01^FCrc::StrCrc32(*Item.Id.ToString()),Ticket.Id.D^uint32(Command.Acquired));
        const bool Settled=HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,Storage->GetTimelineEpoch(),
            FString::Printf(TEXT("deposit:%s:%d:owned%d:r%lld"),*Item.Id.ToString(),Count,Owned,Ticket.Revision),[&]
            {
                FHearthwardTransferResult Result;
                if(Item.IsInstance())
                {
                    TArray<FGuid> Outputs;
                    const auto& Instances=Bag->Snapshot().Instances;
                    for(int32 I=Instances.Num()-1;I>=0 && Outputs.Num()<Count;--I)
                        if(Instances[I].Definition==Item.Id && !Bag->IsEquipped(Instances[I].Id))Outputs.Add(Instances[I].Id);
                    if(Outputs.Num()!=Count)return false;
                    Result.Result=EHearthwardInventoryResult::Success;
                    for(int32 I=0;I<Outputs.Num();++I)
                    {
                        const FGuid Part(Op.A,Op.B,Op.C,Op.D^uint32(I+1));
                        const auto Moved=Storage->TransferInstance(Bag,true,Outputs[I],Part,Ticket.Epoch);
                        if(Moved.Result!=EHearthwardInventoryResult::Success)return false;
                        Result.MovedCount+=Moved.MovedCount;
                    }
                }
                else Result=Storage->Transfer(Bag,true,Item.Id,Count,Op,Ticket.Epoch);
                if(Result.Result!=EHearthwardInventoryResult::Success)return false;
                if(Result.MovedCount>0)
                    GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->RecordCampStockReceipt(Item.Id,Storage->GetItemCount(Item.Id));
                if(Item.Id==Command.GetItem() && Result.MovedCount>0 && Owned>0)
                {
                    if(Command.Goal.Intent==TEXT("retrieve"))
                    {
                        if(!ensure(Command.RecordUnfulfilled(Owned)))return false;
                        Event(TEXT("returned"),Item.Id,Owned,TEXT("undelivered_cargo"),Op);
                    }
                    else
                    {
                        Command.Carried-=Owned;
                        Command.RecordDelivery(Ticket,Owned);
                        Event(TEXT("delivered"),Item.Id,Owned,FString(),Op);
                    }
                }
                return true;
            });
        if (Storage->GetTimelineEpoch() != Ticket.Epoch)
        {
            Command.Cancel();Execution.Reset();
            Phase = EHearthwardCompanionPhase::Cancelled;
            return;
        }
        if (!Settled)
        {
            ReturnBlocked(TEXT("入库失败，保留携带物资"));
            return;
        }
        if (Command.GetDelivered() == Command.GetRequested())
        {
            BlockReason.Reset();Execution.Reset();
            Phase = EHearthwardCompanionPhase::Completed;
            Event(TEXT("completed"),Command.GetItem(),Command.GetDelivered());
            return;
        }
    }

    NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();
    if(bRecovery)
    {
        Execution.bRecoveryToCamp=false;
        Phase=EHearthwardCompanionPhase::WaitingAtCamp;
        return;
    }

    const auto* Current=Execution.Current();
    if(Current && Current->Type==EHearthwardAgentActionType::Deposit)AdvanceExecution();
    else Phase=EHearthwardCompanionPhase::WaitingAtCamp;
}

bool AHearthwardCompanionFixture::ReconcileMissingCargo()
{
    const int32 Missing=Command.Carried-Bag->GetItemCount(Command.GetItem());
    if(Missing<=0)return true;
    if(!ensure(Command.RecordUnfulfilled(Missing)))return false;
    Event(TEXT("cargo_missing"),Command.GetItem(),Missing,TEXT("no_longer_in_brother_bag"));
    return true;
}

void AHearthwardCompanionFixture::Withdraw()
{
    if(!At(Camp)){HandleExecutionFailure(TEXT("CAMP_POSITION_CHANGED"));return;}
    auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    const bool Fetch=Command.Goal.Intent==TEXT("fetch");
    if(!Fetch && !PlayerAtTaskCamp(Player)){HandleExecutionFailure(TEXT("PLAYER_LEFT_CAMP"));return;}
    auto* PlayerBag=Player?Player->FindComponentByClass<UHearthwardInventoryComponent>():nullptr;
    auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    const auto Ticket=Command.GetActive();
    const int32 Limit=FMath::Min(Storage->Available(Command.GetItem()),Command.GetRequested()-Command.GetAcquired());
    const int32 Count=Fetch?TransferableHandoff(Storage->InventorySnapshot(),Bag->Snapshot(),Command.GetItem(),Limit,true)
        : PlayerBag?TransferableCargo(Storage->InventorySnapshot(),Bag->Snapshot(),PlayerBag->Snapshot(),Command.GetItem(),Limit):0;
    if(Count<=0){HandleExecutionFailure(Limit<=0?TEXT("CAMP_STOCK_INSUFFICIENT"):TEXT("BAG_CAPACITY_INSUFFICIENT"));return;}
    const FGuid Op=FGuid::NewGuid();
    TGuardValue<bool> Guard(bSettling,true);
    if(!HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,Storage->GetTimelineEpoch(),
        FString::Printf(TEXT("withdraw:%s:%d:r%lld"),*Command.GetItem().ToString(),Count,Ticket.Revision),[&]
        {
            const auto Result=Storage->Transfer(Bag,false,Command.GetItem(),Count,Op,Ticket.Epoch);
            if(Result.Result!=EHearthwardInventoryResult::Success || Result.MovedCount!=Count)return false;
            if(!ensure(Command.RecordAcquisition(Count)))return false;
            if(Fetch)
            {
                Command.Carried-=Count;
                if(!ensure(Command.RecordDelivery(Ticket,Count)))return false;
            }
            GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->RecordCampStockReceipt(Command.GetItem(),Storage->GetItemCount(Command.GetItem()));
            Event(Fetch?TEXT("delivered"):TEXT("withdrawn"),Command.GetItem(),Count,Fetch?TEXT("brother_bag"):TEXT("camp"),Op);
            return true;
        })) {HandleExecutionFailure(TEXT("CAMP_WITHDRAW_FAILED"));return;}
    AdvanceExecution();
}

void AHearthwardCompanionFixture::Handoff()
{
    auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    if(!PlayerAtTaskCamp(Player) || !Navigation->IsAt(Player,300.f))
    {HandleExecutionFailure(TEXT("PLAYER_HANDOFF_OUT_OF_RANGE"));return;}
    auto* PlayerBag=Player->FindComponentByClass<UHearthwardInventoryComponent>();
    const bool Receiving=Command.Goal.Intent==TEXT("receive");
    const int32 Count=Receiving && PlayerBag
        ? TransferableHandoff(PlayerBag->Snapshot(),Bag->Snapshot(),Command.GetItem(),
            FMath::Min(PlayerBag->Available(Command.GetItem()),Command.GetRequested()-Command.GetAcquired()))
        : Command.Goal.Intent==TEXT("give") && PlayerBag
        ? TransferableHandoff(Bag->Snapshot(),PlayerBag->Snapshot(),Command.GetItem(),FMath::Min(Command.Carried,Bag->Available(Command.GetItem())))
        : Command.Carried;
    if(!PlayerBag || (Receiving && PlayerBag->Available(Command.GetItem())<=0)
        || (!Receiving && (Command.Carried<=0 || Bag->Available(Command.GetItem())<=0))
        || (Command.Goal.Intent!=TEXT("give") && Bag->Available(Command.GetItem())<Command.Carried))
    {HandleExecutionFailure(TEXT("CARGO_UNAVAILABLE"));return;}
    if(Count<=0){HandleExecutionFailure(Receiving?TEXT("BAG_CAPACITY_INSUFFICIENT"):TEXT("PLAYER_BAG_CAPACITY_INSUFFICIENT"));return;}
    auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    const auto Ticket=Command.GetActive();
    const FGuid Op=FGuid::NewGuid();
    TGuardValue<bool> Guard(bSettling,true);
    if(!HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,Storage->GetTimelineEpoch(),
        FString::Printf(TEXT("handoff:%s:%d:r%lld"),*Command.GetItem().ToString(),Count,Ticket.Revision),[&]
        {
            if((Receiving?PlayerBag->TransferTo(Bag,Command.GetItem(),Count):Bag->TransferTo(PlayerBag,Command.GetItem(),Count))!=EHearthwardInventoryResult::Success)return false;
            if(Receiving && !ensure(Command.RecordAcquisition(Count)))return false;
            Command.Carried-=Count;
            if(!ensure(Command.RecordDelivery(Ticket,Count)))return false;
            Event(TEXT("delivered"),Command.GetItem(),Count,Receiving?TEXT("brother_bag"):TEXT("player_bag"),Op);
            return true;
        })) {HandleExecutionFailure(Receiving?TEXT("BAG_CAPACITY_INSUFFICIENT"):TEXT("PLAYER_BAG_CAPACITY_INSUFFICIENT"));return;}
    AdvanceExecution();
}

bool AHearthwardCompanionFixture::MeleeAttackReady() const
{
    return MeleeHitAt>0 || GetWorld()->GetTimeSeconds()>=NextHuntAttackAt;
}

void AHearthwardCompanionFixture::CancelMeleeAttack()
{
    if(MeleeHitAt>0)
        if(auto* Animation=Cast<UHearthwardBrotherAnimInstance>(GetMesh()->GetAnimInstance()))Animation->CancelAttack();
    MeleeHitAt=0;MeleeTarget.Reset();MeleeWeapon.Invalidate();
}

bool AHearthwardCompanionFixture::AdvanceMeleeAttack(AActor* Target,AActor* Player,bool Hunting)
{
    const auto Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    const auto* Weapon=Bag->FindInstance(Bag->EquippedInstance(TEXT("weapon")));
    const auto* Survival=FindComponentByClass<UHearthwardSurvivalComponent>();
    auto* Combat=IsValid(Target)?Target->FindComponentByClass<UHearthwardCombatTargetComponent>():nullptr;
    const float Range=HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("tuning")),TEXT("attackRange"))*.8f;
    if(!Player || !Combat || !Combat->CanAct() || !Weapon || Weapon->Durability<=0 || !Survival->Alive() || Survival->Busy()
        || FVector::Dist2D(GetActorLocation(),Target->GetActorLocation())>Range)
    {CancelMeleeAttack();return false;}
    if(MeleeHitAt>0 && (MeleeTarget!=Target || MeleeWeapon!=Weapon->Id || MeleeEpoch!=Epoch || bMeleeHunting!=Hunting))
    {CancelMeleeAttack();return false;}
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(CompanionMelee),false,this);Query.AddIgnoredActor(Player);
    if(GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation()+FVector(0,0,50),Target->GetActorLocation()+FVector(0,0,50),ECC_Visibility,Query)
        && Hit.GetActor()!=Target)
    {CancelMeleeAttack();return false;}
    const double Now=GetWorld()->GetTimeSeconds();
    if(MeleeHitAt<=0)
    {
        if(Now<NextHuntAttackAt)return false;
        MeleeTarget=Target;MeleeWeapon=Weapon->Id;MeleeEpoch=Epoch;bMeleeHunting=Hunting;
        MeleeHitAt=Now+.25;
        NextHuntAttackAt=Now+HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("tuning")),TEXT("companionAttackCooldown"));
        FVector Facing=Target->GetActorLocation()-GetActorLocation();Facing.Z=0;SetActorRotation(Facing.Rotation());
        if(auto* Animation=Cast<UHearthwardBrotherAnimInstance>(GetMesh()->GetAnimInstance()))Animation->PlayAttack();
        return false;
    }
    if(Now<MeleeHitAt)return false;
    const float Attack=HearthwardData::Number(HearthwardData::Find(TEXT("items"),Weapon->Definition.ToString()),TEXT("attack"))
        *(Survival->State.Severe()?.75f:1.f);
    const FGuid Instance=Weapon->Id;
    MeleeHitAt=0;MeleeTarget.Reset();MeleeWeapon.Invalidate();
    Player->FindComponentByClass<UHearthwardGameplayComponent>()->DamageOpponent(Combat->Id,Attack,this);
    Bag->WearInstance(Instance,1);
    return true;
}

void AHearthwardCompanionFixture::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    using P = EHearthwardCompanionPhase;
    if (!bFixtureEnabled || GetWorld()->IsPaused() || bSettling) return;
    if(MeleeHitAt>0 && (MeleeEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()
        || (bMeleeHunting && (!Execution.Current() || Execution.Current()->Type!=EHearthwardAgentActionType::Hunt))))CancelMeleeAttack();
    if(FindComponentByClass<UHearthwardSurvivalComponent>()->AutomaticBehavior(DeltaSeconds)) return;
    if (Phase == P::Idle || Phase == P::Cancelled || Phase == P::Completed || Phase == P::WaitingAtCamp || Phase==P::HoldingSafely) return;

    auto* Storage = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if (!Command.IsCurrent(Storage->GetTimelineEpoch()))
    {
        StopNavigation();
        Command.Cancel();
        Execution.Reset();
        Action->InterruptAction();
        Phase = P::Cancelled;
        return;
    }

    if(Execution.bRecoveryToCamp)
    {
        TickRecovery(DeltaSeconds);
        return;
    }

    if(Execution.bAdaptiveRecovery)
    {
        if(GetWorld()->GetTimeSeconds()<Execution.AdaptiveRetryAt)return;
        Execution.bAdaptiveRecovery=false;
        Execution.AdaptiveRetryAt=0.0;
        BlockReason.Reset();
        NavigationFailures=0;
        LastProgressAt=GetWorld()->GetTimeSeconds();
        LastProgressPosition=GetActorLocation();
    }

    if(!Execution.Current())
    {
        if(!BuildExecutionPlan(false)){ReturnBlocked(TEXT("NO_EXECUTABLE_PLAN"));return;}
    }
    TickExecution(DeltaSeconds);
}

void AHearthwardCompanionFixture::TickRecovery(float DeltaSeconds)
{
    if(!IsValid(Camp) || Camp->IsActorBeingDestroyed())
    {
        StopNavigation();Phase=EHearthwardCompanionPhase::HoldingSafely;BlockReason=TEXT("CAMP_UNAVAILABLE：保留物资；营地恢复后可重试");
        return;
    }
    if(At(Camp))
    {
        if(Command.Goal.Intent==TEXT("give") || Command.Goal.Intent==TEXT("fetch") || Command.Goal.Intent==TEXT("receive"))
        {
            if(Command.Goal.Intent==TEXT("give") && !ReconcileMissingCargo())BlockReason=TEXT("CARGO_ACCOUNTING_FAILED");
            Execution.bRecoveryToCamp=false;
            Phase=EHearthwardCompanionPhase::WaitingAtCamp;
            return;
        }
        Deposit();
        return;
    }
    if(!MoveTowards(Camp,DeltaSeconds))
    {
        StopNavigation();
        Phase=EHearthwardCompanionPhase::HoldingSafely;
        BlockReason=TEXT("RETURN_UNREACHABLE：返营路线受阻，保留物资；会合后可重试");
    }
}

void AHearthwardCompanionFixture::TickExecution(float DeltaSeconds)
{
    const auto* Current=Execution.Current();
    if(!Current)return;

    if(ActionRequiresSafety(*Current))
    {
        const auto Safety=HearthwardPerception::Evaluate(HearthwardPerception::Capture(this),Command.Goal);
        if(!Safety.IsAllowed()){HandleExecutionFailure(Safety.Reason);return;}
    }
    if(Command.Goal.Intent==TEXT("nature_collect") && Current->Target==EHearthwardAgentTarget::Nature
        && (LastNatureSafetyTarget!=Command.Goal.Station || GetWorld()->GetTimeSeconds()>=NextNatureSafetyAt))
    {
        LastNatureSafetyTarget=Command.Goal.Station;
        NextNatureSafetyAt=GetWorld()->GetTimeSeconds()+1;
        if(const FString Reason=ResourceTargetReason(Command.Goal,false);!Reason.IsEmpty())
        {HandleExecutionFailure(Reason);return;}
    }
    if(Command.Goal.Intent==TEXT("give") || Command.Goal.Intent==TEXT("receive"))
    {
        const auto& Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
        if(!IsValid(Camp) || Camps.CampAt(GetActorLocation())!=Camps.CampAt(Camp->GetActorLocation()))
        {HandleExecutionFailure(TEXT("BROTHER_LEFT_CAMP"));return;}
    }
    if(Command.Goal.Intent==TEXT("escort") || Command.Goal.Intent==TEXT("hunt") || Command.Goal.Intent==TEXT("fish") || Command.Goal.Intent==TEXT("capture"))
    {
        auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        if(!Player){HandleExecutionFailure(TEXT("PLAYER_UNAVAILABLE"));return;}
        if(FVector::Dist2D(GetActorLocation(),Player->GetActorLocation())>3000)
        {
            if(Command.Goal.Intent==TEXT("escort") && Current->Type==EHearthwardAgentActionType::Escort)
            {Execution.Cursor=0;Execution.bStarted=false;Phase=EHearthwardCompanionPhase::GoingToSource;}
            if((Command.Goal.Intent==TEXT("fish") || Command.Goal.Intent==TEXT("capture")) && Current->Type==EHearthwardAgentActionType::CommitNature)
            {Action->InterruptAction();Execution.bStarted=false;}
            if(!MoveTowards(Player,DeltaSeconds,300))HandleExecutionFailure(TEXT("REGROUP_UNREACHABLE"));
            return;
        }
    }

    using A=EHearthwardAgentActionType;
    using T=EHearthwardAgentTarget;
    if(Current->Type==A::MoveTo)
    {
        if(Current->Target==T::Source)
        {
            if(!IsSourceValid()){HandleExecutionFailure(TEXT("SOURCE_UNAVAILABLE"));return;}
            if(Source->GetItemCount(Command.GetItem())<=0){HandleExecutionFailure(TEXT("实际资源不足"));return;}
        }
        AActor* Target=ResolveActionTarget(*Current);
        if(!IsValid(Target))
        {
            HandleExecutionFailure(Current->Target==T::Source?TEXT("SOURCE_UNAVAILABLE"):
                Current->Target==T::Camp?TEXT("CAMP_UNAVAILABLE"):TEXT("STATION_UNAVAILABLE"));
            return;
        }
        if(Current->Target==T::Player && !PlayerAtTaskCamp(Target))
        {HandleExecutionFailure(TEXT("PLAYER_LEFT_CAMP"));return;}
        const float Acceptance=Current->Target==T::Workshop
            ? float(HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("crafting")),TEXT("reach")))
            : Current->Target==T::Nature || Current->Target==T::Player || Current->Target==T::Person ? 180.f : 40.f;
        if(At(Target) || (Current->Target==T::Workshop
                && FVector::Dist(GetActorLocation(),Target->GetActorLocation())<=Acceptance)
            || (Current->Target==T::Nature
                && FVector::Dist(GetActorLocation(),Target->GetActorLocation())<=240.f)
            || (Current->Target==T::Person
                && FVector::Dist(GetActorLocation(),Target->GetActorLocation())<=240.f)
            || (Current->Target==T::Player && Navigation->IsAt(Target,300.f)))
        {
            StopNavigation();
            AdvanceExecution();
            return;
        }
        if(!MoveTowards(Target,DeltaSeconds,Acceptance))
        {
            HandleExecutionFailure(Current->Target==T::Camp?TEXT("RETURN_UNREACHABLE"):
                Current->Target==T::Source?TEXT("去程受阻"):TEXT("PATH_BLOCKED"));
        }
        return;
    }

    if(Current->Type==A::Escort)
    {
        auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
        const auto* Person=Campaign->State.People.FindByPredicate([&](const auto& P){return P.Id==Command.Goal.Item;});
        if(!Person){HandleExecutionFailure(TEXT("PERSON_UNAVAILABLE"));return;}
        if(!Campaign->Actor(Command.Goal.Item))
        {
            if(GetWorld()->GetTimeSeconds()-LastProgressAt>HearthwardAgent::Policy(TEXT("no_progress_seconds")))
                HandleExecutionFailure(TEXT("PERSON_UNAVAILABLE"));
            return;
        }
        if(Person->Stage==TEXT("arrived"))
        {
            const auto Ticket=Command.GetActive();
            if(!Command.RecordAcquisition(1)){HandleExecutionFailure(TEXT("ESCORT_RECEIPT_INVALID"));return;}
            Command.Carried=0;
            if(!Command.RecordDelivery(Ticket,1)){HandleExecutionFailure(TEXT("ESCORT_RECEIPT_INVALID"));return;}
            Event(TEXT("delivered"),Command.Goal.Item,1,TEXT("campaign_camp_arrival"));
            AdvanceExecution();return;
        }
        if(!Execution.bStarted)
        {
            if(!Campaign->AssignEscort(Command.Goal.Item,this,Command.GetActive().Epoch))
            {HandleExecutionFailure(TEXT("ESCORT_TARGET_CHANGED"));return;}
            Execution.bStarted=true;
        }
        else if(Person->Stage!=TEXT("following") || Person->Escort!=TEXT("brother"))
        {HandleExecutionFailure(TEXT("PERSON_WAITING"));return;}
        auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        if(FVector::Dist2D(GetActorLocation(),Player->GetActorLocation())>220)
        {
            if(!MoveTowards(Player,DeltaSeconds,180))HandleExecutionFailure(TEXT("ESCORT_PATH_BLOCKED"));
        }
        else StopNavigation();
        return;
    }

    if(Current->Type==A::Hunt)
    {
        auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        auto* Animal=Nature->State.Animals.FindByPredicate([&](const auto& X){return X.Id==Command.Goal.Station;});
        auto* Target=Nature->Actor(Command.Goal.Station);
        auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        if(!Animal || Animal->Domestic || Animal->Definition!=Command.Goal.Item || !Target || Animal->Health<=0)
        {HandleExecutionFailure(TEXT("HUNT_TARGET_LOST"));return;}
        if(!Player || FVector::Dist2D(Player->GetActorLocation(),Target->GetActorLocation())>3000)
        {HandleExecutionFailure(TEXT("TARGET_LEFT_PLAYER_RANGE"));return;}
        const auto* Weapon=Bag->FindInstance(Bag->EquippedInstance(TEXT("weapon")));
        if(!Weapon || Weapon->Durability<=0){HandleExecutionFailure(TEXT("WEAPON_REQUIRED"));return;}
        const float Range=HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("tuning")),TEXT("attackRange"));
        if(FVector::Dist2D(GetActorLocation(),Target->GetActorLocation())>Range*.8f)
        {
            CancelMeleeAttack();
            if(!MoveTowards(Target,DeltaSeconds,Range*.8f))HandleExecutionFailure(TEXT("HUNT_PATH_BLOCKED"));
            return;
        }
        StopNavigation();
        if(!MeleeAttackReady())return;
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(CompanionHunt),false,this);Query.AddIgnoredActor(Player);
        if(GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation()+FVector(0,0,50),Target->GetActorLocation()+FVector(0,0,50),ECC_Visibility,Query)
            && Hit.GetActor()!=Target)
        {HandleExecutionFailure(TEXT("HUNT_LINE_BLOCKED"));return;}
        const float Before=Animal->Health;
        if(!AdvanceMeleeAttack(Target,Player,true))return;
        if(Before>0 && Animal->Health<=0)
        {
            const auto Ticket=Command.GetActive();
            if(!Command.RecordAcquisition(1)){HandleExecutionFailure(TEXT("HUNT_RECEIPT_INVALID"));return;}
            Command.Carried=0;
            if(!Command.RecordDelivery(Ticket,1)){HandleExecutionFailure(TEXT("HUNT_RECEIPT_INVALID"));return;}
            AdvanceExecution();
        }
        return;
    }

    if(Current->Type==A::LeadAnimal)
    {
        auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        const auto* Animal=Nature->State.Animals.FindByPredicate([&](const auto& X){return X.Id==Command.Goal.Station;});
        if(!Animal || Animal->Health<=0 || !Animal->Captured){HandleExecutionFailure(TEXT("CAPTURE_TARGET_LOST"));return;}
        if(Animal->Pen.IsValid() && !Animal->Following)
        {
            const auto Ticket=Command.GetActive();
            if(!Command.RecordAcquisition(1)){HandleExecutionFailure(TEXT("CAPTURE_RECEIPT_INVALID"));return;}
            Command.Carried=0;
            if(!Command.RecordDelivery(Ticket,1)){HandleExecutionFailure(TEXT("CAPTURE_RECEIPT_INVALID"));return;}
            Event(TEXT("acquired"),Command.Goal.Item,1,TEXT("animal_arrived_in_pen"));
            AdvanceExecution();return;
        }
        if(!Animal->Following || !Animal->FollowingBrother || !Animal->ReservedPen.IsValid())
        {HandleExecutionFailure(TEXT("ANIMAL_STOPPED_FOLLOWING"));return;}
        auto* Pen=Nature->Actor(Animal->ReservedPen);
        if(!Pen){HandleExecutionFailure(TEXT("PEN_UNAVAILABLE"));return;}
        if(FVector::Dist2D(GetActorLocation(),Pen->GetActorLocation())>180)
        {if(!MoveTowards(Pen,DeltaSeconds,180))HandleExecutionFailure(TEXT("LEAD_PATH_BLOCKED"));}
        else StopNavigation();
        return;
    }

    if(Current->Type==A::MonitorCampBatch)
    {
        auto* Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
        auto* R=Camps->State.Regions.FindByPredicate([&](const auto& X){return X.Facility==Command.Goal.Station && X.Job==Command.Goal.Item;});
        const auto* Facility=Camps->State.Facilities.FindByPredicate([&](const auto& F){return F.Id==Command.Goal.Station;});
        if(!R || !Facility || Facility->Paused || !R->Brother || !R->Workers.IsEmpty() || R->Player || R->ToRations)
        {StopCampBatch();HandleExecutionFailure(TEXT("CAMP_BATCH_CHANGED"));return;}
        if(CampBatchBaseline<0)
        {
            if(R->Enabled || R->Batch.Active){HandleExecutionFailure(TEXT("CAMP_BATCH_CHANGED"));return;}
            CampBatchBaseline=R->Completed;
            R->BatchStopAt=R->Completed+Command.Goal.Quantity;
            R->Enabled=true;
        }
        if(R->BatchStopAt!=CampBatchBaseline+Command.Goal.Quantity)
        {StopCampBatch();HandleExecutionFailure(TEXT("CAMP_BATCH_CHANGED"));return;}
        if(!R->Enabled && R->Completed<R->BatchStopAt)
        {HandleExecutionFailure(TEXT("CAMP_BATCH_PAUSED"));return;}
        const int32 Completed=R->Completed-CampBatchBaseline;
        const int32 NewlyCompleted=Completed-Command.GetDelivered();
        if(NewlyCompleted>0)
        {
            const auto Ticket=Command.GetActive();
            if(!Command.RecordAcquisition(NewlyCompleted)){StopCampBatch();HandleExecutionFailure(TEXT("CAMP_BATCH_RECEIPT_INVALID"));return;}
            Command.Carried=0;
            if(!Command.RecordDelivery(Ticket,NewlyCompleted)){StopCampBatch();HandleExecutionFailure(TEXT("CAMP_BATCH_RECEIPT_INVALID"));return;}
            Event(TEXT("delivered"),Command.Goal.Item,NewlyCompleted,TEXT("camp_region_completed"));
        }
        if(Command.GetDelivered()==Command.GetRequested())
        {R->Enabled=false;AdvanceExecution();return;}
        if(!R->Safe || R->Status.Contains(TEXT("等待共享仓储补料")) || R->Status.Contains(TEXT("仓储数量达到上限")))
        {StopCampBatch();HandleExecutionFailure(TEXT("CAMP_BATCH_BLOCKED"));return;}
        return;
    }

    if(Current->Type==A::Gather)
    {
        if(!IsSourceValid()){HandleExecutionFailure(TEXT("SOURCE_UNAVAILABLE"));return;}
        if(Source->GetItemCount(Command.GetItem())<=0){HandleExecutionFailure(TEXT("实际资源不足"));return;}
        if(!At(Source->GetOwner())){HandleExecutionFailure(TEXT("SOURCE_POSITION_CHANGED"));return;}

        if(!Execution.bStarted)
        {
            StopNavigation();
            if(!Action->StartAction()){HandleExecutionFailure(TEXT("无法开始采集"));return;}
            Execution.bStarted=true;
            return;
        }
        if(Action->GetStatus()==EHearthwardTimedActionStatus::Interrupted){HandleExecutionFailure(TEXT("采集被中断"));return;}
        if(Action->GetStatus()!=EHearthwardTimedActionStatus::Completed)return;

        const auto* Item=HearthwardBasicItems().FindByPredicate([this](const auto& Def){return Def.Id==Command.GetItem();});
        if(!Item){HandleExecutionFailure(TEXT("ITEM_UNAVAILABLE"));return;}
        int32 CargoWeight=0;
        for(const auto& D:HearthwardBasicItems())if(!D.IsInstance())CargoWeight+=Bag->GetItemCount(D.Id)*D.WeightHundredths;
        const int32 Free=FMath::Max(0,FMath::Min(400-CargoWeight,FMath::RoundToInt((Bag->GetCapacity()-Bag->GetWeight())*100)));
        FGuid Tool;const int32 ToolYield=HearthwardHarvestTools::Yield(Bag,Item->Id,Tool);
        if(ToolYield<=0){HandleExecutionFailure(TEXT("TOOL_REQUIRED"));return;}
        const int32 Count=FMath::Min(ToolYield,FMath::Min3(Free/Item->WeightHundredths,Source->GetItemCount(Item->Id),Command.GetRequested()-Command.GetAcquired()));
        if(Count<=0){HandleExecutionFailure(TEXT("本趟无法携带目标物品"));return;}

        auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        TGuardValue<bool> Guard(bSettling,true);
        const auto Ticket=Command.GetActive();const FGuid Op(Ticket.Id.A,Ticket.Id.B,Ticket.Id.C^0xAC01,Ticket.Id.D^uint32(Command.Acquired));
        if(!HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,Storage->GetTimelineEpoch(),
            FString::Printf(TEXT("acquire:%s:%d:%d"),*Item->Id.ToString(),Count,Command.Acquired),[&]
            {
                if(Source->TransferTo(Bag,Item->Id,Count)!=EHearthwardInventoryResult::Success)return false;
                if(Tool.IsValid())Bag->WearInstance(Tool,1);
                Command.RecordAcquisition(Count);
                Execution.AdaptiveRecoveryAttempts=0;
                Execution.LastRecoveryReason.Reset();
                Event(TEXT("acquired"),Item->Id,Count,FString(),Op);return true;
            }))
        {
            HandleExecutionFailure(TEXT("采集结算时资源或容量不足"));
            return;
        }
        AdvanceExecution();
        return;
    }

    if(Current->Type==A::CommitNature)
    {
        auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        if(!IsValid(Nature->Actor(Command.Goal.Station))){HandleExecutionFailure(TEXT("TARGET_UNAVAILABLE"));return;}
        if(FVector::Dist(GetActorLocation(),Nature->Actor(Command.Goal.Station)->GetActorLocation())>300)
        {HandleExecutionFailure(TEXT("TARGET_POSITION_CHANGED"));return;}
        if(!Execution.bStarted)
        {
            StopNavigation();
            if(!Action->StartAction()){HandleExecutionFailure(TEXT("ACTION_BUSY"));return;}
            Execution.bStarted=true;return;
        }
        if(Action->GetStatus()==EHearthwardTimedActionStatus::Interrupted){HandleExecutionFailure(TEXT("ACTION_INTERRUPTED"));return;}
        if(Action->GetStatus()!=EHearthwardTimedActionStatus::Completed)return;
        if(Command.Goal.Intent==TEXT("nature_collect"))
            if(const FString Reason=ResourceTargetReason(Command.Goal,false);!Reason.IsEmpty())
            {HandleExecutionFailure(Reason);return;}
        const auto Ticket=Command.GetActive();
        const bool Collect=Command.Goal.Intent==TEXT("nature_collect");
        const bool Fish=Command.Goal.Intent==TEXT("fish");
        const bool Capture=Command.Goal.Intent==TEXT("capture");
        const FGuid Op(Ticket.Id.A,Ticket.Id.B,Ticket.Id.C^0xCA01,Ticket.Id.D^uint32(Collect?Command.Acquired:0));
        TGuardValue<bool> Guard(bSettling,true);
        if(!HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,
            GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(),
            FString::Printf(TEXT("nature:%s:%s:%d:%d"),*Command.Goal.Item.ToString(),*Command.Goal.Station.ToString(),Command.Goal.Quantity,Command.Acquired),[&]
            {
                const int32 Before=Collect?Bag->GetItemCount(Command.Goal.Item):0;
                const bool Product=Collect && Nature->State.Pens.ContainsByPredicate([&](const auto& P){return P.Id==Command.Goal.Station;});
                FName Catch;
                if(Fish)
                {
                    if(!Nature->CatchCompanion(this,Command.Goal.Station,Ticket.Epoch,Catch))return false;
                    if(!ensure(Command.RecordAcquisition(1)))return false;
                    Command.Carried=0;
                    if(!ensure(Command.RecordDelivery(Ticket,1)))return false;
                    Event(TEXT("acquired"),Catch,1,TEXT("fishing_catch"),Op);
                }
                else if(!Nature->CommitCompanion(this,Capture?FName(Nature->State.Animals.ContainsByPredicate([&](const auto& A){return A.Id==Command.Goal.Station && A.Captured;})?TEXT("lead"):TEXT("capture")):
                    Collect?(Product?FName(TEXT("collect_product")):FName(TEXT("harvest"))):Command.Goal.Item,Command.Goal.Station,
                    Ticket.Epoch,Collect?Command.GetRequested()-Command.GetAcquired():Command.Goal.Quantity))return false;
                if(Collect)
                {
                    const int32 Gained=Bag->GetItemCount(Command.Goal.Item)-Before;
                    if(!ensure(Gained>0 && Command.RecordAcquisition(Gained)))return false;
                    Event(TEXT("acquired"),Command.Goal.Item,Gained,FString(),Op);
                }
                else if(!Fish && !Capture)
                {
                    Command.RecordAcquisition(Command.Goal.Quantity);
                    Command.Carried=0;
                    Command.RecordDelivery(Ticket,Command.Goal.Quantity);
                    Event(TEXT("nature_care"),Command.Goal.Item,Command.Goal.Quantity,FString(),Op);
                }
                return true;
            })) {HandleExecutionFailure(TEXT("NATURE_CONDITIONS_CHANGED"));return;}
        AdvanceExecution();return;
    }

    if(Current->Type==A::TakeMaterials || Current->Type==A::CommitWorkshop)
    {
        WorkshopTick();
        return;
    }

    if(Current->Type==A::Withdraw){Withdraw();return;}
    if(Current->Type==A::Handoff){Handoff();return;}

    if(Current->Type==A::Deposit)
    {
        Deposit();
        return;
    }
}

namespace
{
UHearthwardBuildingComponent* WorkshopRegistry(UWorld* World)
{auto* P=UGameplayStatics::GetPlayerPawn(World,0);return P?P->FindComponentByClass<UHearthwardBuildingComponent>():nullptr;}
}
FString AHearthwardCompanionFixture::ResourceTargetReason(const FHearthwardAgentGoal& Goal,bool RequireKnown) const
{
    if(!Goal.Station.IsValid())return TEXT("TARGET_REQUIRED");
    auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
    auto* Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    const auto* Point=Nature->State.Points.FindByPredicate([&](const auto& P){return P.Id==Goal.Station && P.Kind==TEXT("resource");});
    const auto* Pen=Nature->State.Pens.FindByPredicate([&](const auto& P){return P.Id==Goal.Station;});
    if((!Point && !Pen) || !IsValid(Nature->Actor(Goal.Station)))return TEXT("TARGET_UNAVAILABLE");
    const auto Def=HearthwardNature::Definition(Point?TEXT("resources"):TEXT("domestic"),Point?Point->Definition:Pen->Definition);
    if(!Def || FName(*HearthwardData::Text(Def,Point?TEXT("item"):TEXT("product")))!=Goal.Item)return TEXT("TARGET_INVALID");
    const FVector At=Point?Point->Position:Pen->Position;
    const FName Site=Camps->State.CampAt(At);
    if(!IsValid(Camp))return TEXT("CAMP_UNAVAILABLE");
    const FName TaskCamp=Camps->State.CampAt(Camp->GetActorLocation());
    if(TaskCamp.IsNone() || (!Site.IsNone() && Site!=TaskCamp))return TEXT("CAMP_UNAVAILABLE");
    if(RequireKnown)
    {
        const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        if(!Player || FVector::Dist2D(Player->GetActorLocation(),At)>3000)return TEXT("TARGET_NOT_KNOWN");
    }
    for(const auto& R:Camps->State.Regions)
        if(R.Camp==TaskCamp && !R.Facility.IsValid() && !R.Safe)return TEXT("AREA_UNSAFE");
    if(Point && Site.IsNone())
    {
        auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        if(!Nav || Nav->IsNavigationBuildInProgress())return TEXT("ROUTE_UNAVAILABLE");
        auto* Route=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),At,Camp->GetActorLocation());
        if(!Route || !Route->IsValid() || Route->IsPartial() || Route->PathPoints.Num()<2)return TEXT("ROUTE_UNAVAILABLE");
        for(TActorIterator<AActor> It(GetWorld());It;++It)
        {
            const auto* Threat=It->FindComponentByClass<UHearthwardCombatTargetComponent>();
            if(!Threat || !Threat->Alive())continue;
            if(const auto* Natural=Cast<AHearthwardNatureActor>(*It))
            {
                const auto* Animal=Nature->State.Animals.FindByPredicate([&](const auto& A){return A.Id==Natural->Id;});
                if(!Animal || Animal->Domestic || HearthwardData::Text(HearthwardNature::Definition(TEXT("wildlife"),Animal->Definition),TEXT("behavior"))==TEXT("flee"))continue;
            }
            const FVector Enemy=FVector(It->GetActorLocation().X,It->GetActorLocation().Y,0);
            for(int32 I=1;I<Route->PathPoints.Num();++I)
            {
                const FVector Start=FVector(Route->PathPoints[I-1].X,Route->PathPoints[I-1].Y,0);
                const FVector End=FVector(Route->PathPoints[I].X,Route->PathPoints[I].Y,0);
                if(FMath::PointDistToSegment(Enemy,Start,End)<5000)return TEXT("ROUTE_NOT_TRUSTED_SAFE");
            }
        }
    }
    if(Pen)return Pen->Products>=Goal.Quantity?FString():TEXT("SOURCE_UNAVAILABLE");
    const auto* Resource=Camps->Source(Point->Key.ToString());
    if(!Resource || Resource->Blocked || Resource->Remaining<=0)return TEXT("SOURCE_UNAVAILABLE");
    FGuid Tool;
    if(HearthwardData::Text(Def,TEXT("tool"))!=TEXT("hand")
        && HearthwardHarvestTools::Yield(Bag,Goal.Item,Tool)<=0)return TEXT("TOOL_REQUIRED");
    return {};
}
FString AHearthwardCompanionFixture::PreviewGoal(const FHearthwardAgentGoal& Goal) const
{
    const FString Error=HearthwardAgent::Validate(Goal);if(!Error.IsEmpty())return Error;
    const auto Safety=HearthwardPerception::Evaluate(HearthwardPerception::Capture(this),Goal);
    if(!Safety.IsAllowed())return Safety.Reason;
    if(Goal.Intent==TEXT("collect")) return {};
    if(Goal.Intent==TEXT("nature_collect"))return ResourceTargetReason(Goal,true);
    if(Goal.Intent==TEXT("store"))
    {
        if(Goal.SourceRef==TEXT("bag"))return Bag->GetItemCount(Goal.Item)>=Goal.Quantity?FString():TEXT("BAG_INSUFFICIENT");
        const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        auto* Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
        if(!Player || !IsValid(Camp) || FVector::Dist(GetActorLocation(),Player->GetActorLocation())>300
            || Camps->State.CampAt(Player->GetActorLocation()).IsNone()
            || Camps->State.CampAt(Player->GetActorLocation())!=Camps->State.CampAt(Camp->GetActorLocation()))return TEXT("PLAYER_HANDOFF_OUT_OF_RANGE");
        const auto* PlayerBag=Player->FindComponentByClass<UHearthwardInventoryComponent>();
        if(!PlayerBag || PlayerBag->Available(Goal.Item)<Goal.Quantity)return TEXT("PLAYER_BAG_INSUFFICIENT");
        FHearthwardInventoryState SourceAfter,TargetAfter;
        if(!SourceAfter.Restore(PlayerBag->Snapshot()) || !TargetAfter.Restore(Bag->Snapshot())
            || SourceAfter.TransferTo(TargetAfter,Goal.Item,Goal.Quantity)!=EHearthwardInventoryResult::Success)
            return TEXT("BAG_CAPACITY_INSUFFICIENT");
        return {};
    }
    if(Goal.Intent==TEXT("retrieve"))
    {
        auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        auto* PlayerBag=Player?Player->FindComponentByClass<UHearthwardInventoryComponent>():nullptr;
        auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        if(!PlayerAtTaskCamp(Player))return TEXT("PLAYER_LEFT_CAMP");
        if(Storage->Available(Goal.Item)<Goal.Quantity)return TEXT("CAMP_STOCK_INSUFFICIENT");
        if(!PlayerBag || TransferableCargo(Storage->InventorySnapshot(),Bag->Snapshot(),PlayerBag->Snapshot(),Goal.Item,1)!=1)
            return TEXT("BAG_CAPACITY_INSUFFICIENT");
        return {};
    }
    if(Goal.Intent==TEXT("give"))
    {
        auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        auto* PlayerBag=Player?Player->FindComponentByClass<UHearthwardInventoryComponent>():nullptr;
        const auto& Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
        if(!PlayerAtTaskCamp(Player) || Camps.CampAt(GetActorLocation())!=Camps.CampAt(Camp->GetActorLocation()))
            return TEXT("PLAYER_LEFT_CAMP");
        if(Bag->Available(Goal.Item)<Goal.Quantity)return TEXT("BAG_INSUFFICIENT");
        if(!PlayerBag || TransferableHandoff(Bag->Snapshot(),PlayerBag->Snapshot(),Goal.Item,1)!=1)
            return TEXT("PLAYER_BAG_CAPACITY_INSUFFICIENT");
        return {};
    }
    if(Goal.Intent==TEXT("fetch"))
    {
        auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        if(!PlayerAtTaskCamp(Player))return TEXT("PLAYER_LEFT_CAMP");
        if(Storage->Available(Goal.Item)<Goal.Quantity)return TEXT("CAMP_STOCK_INSUFFICIENT");
        if(TransferableHandoff(Storage->InventorySnapshot(),Bag->Snapshot(),Goal.Item,1,true)!=1)
            return TEXT("BAG_CAPACITY_INSUFFICIENT");
        return {};
    }
    if(Goal.Intent==TEXT("receive"))
    {
        auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        auto* PlayerBag=Player?Player->FindComponentByClass<UHearthwardInventoryComponent>():nullptr;
        const auto& Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
        if(!PlayerAtTaskCamp(Player) || Camps.CampAt(GetActorLocation())!=Camps.CampAt(Camp->GetActorLocation()))
            return TEXT("PLAYER_LEFT_CAMP");
        if(!PlayerBag || PlayerBag->Available(Goal.Item)<Goal.Quantity)return TEXT("PLAYER_BAG_INSUFFICIENT");
        if(TransferableHandoff(PlayerBag->Snapshot(),Bag->Snapshot(),Goal.Item,1)!=1)
            return TEXT("BAG_CAPACITY_INSUFFICIENT");
        return {};
    }
    if(Goal.Intent==TEXT("nature_care"))
    {
        if(!Goal.Station.IsValid())return TEXT("TARGET_REQUIRED");
        auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        if(!IsValid(Nature->Actor(Goal.Station)))return TEXT("TARGET_UNAVAILABLE");
        const auto* Crop=Nature->State.Crops.FindByPredicate([&](const auto& C){return C.Id==Goal.Station;});
        const bool Pen=Nature->State.Pens.ContainsByPredicate([&](const auto& P){return P.Id==Goal.Station;});
        if((Goal.Item==TEXT("deposit_feed"))!=Pen || (Goal.Item!=TEXT("deposit_feed") && !Crop))return TEXT("TARGET_INVALID");
        if(Crop && ((Goal.Item==TEXT("water") && Crop->Watered) || (Goal.Item==TEXT("fertilize") && Crop->Fertilized)))return TEXT("ALREADY_DONE");
        if(Crop && Goal.Item==TEXT("harvest") && !Nature->State.Ready(*Crop))return TEXT("NOT_READY");
        return Goal.Item==TEXT("deposit_feed") && Bag->GetItemCount(TEXT("feed"))<Goal.Quantity
            ? TEXT("BAG_INSUFFICIENT"):FString();
    }
    if(Goal.Intent==TEXT("escort"))
    {
        auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
        const auto* Person=Campaign->State.People.FindByPredicate([&](const auto& P){return P.Id==Goal.Item;});
        const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        if(!Person || Person->Stage==TEXT("uncontacted") || Person->Stage==TEXT("arrived") || !Campaign->Actor(Goal.Item))
            return TEXT("PERSON_NOT_CONTACTED");
        if(!Player || FVector::Dist2D(Player->GetActorLocation(),GetActorLocation())>3000)return TEXT("PLAYER_TOO_FAR");
        if(!IsValid(Camp) || GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(Camp->GetActorLocation()).IsNone())
            return TEXT("CAMP_UNAVAILABLE");
        return {};
    }
    if(Goal.Intent==TEXT("hunt"))
    {
        auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        const auto* Animal=Nature->State.Animals.FindByPredicate([&](const auto& X){return X.Id==Goal.Station;});
        const auto* Target=Nature->Actor(Goal.Station);
        const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        if(!Animal || Animal->Domestic || Animal->Definition!=Goal.Item || Animal->Health<=0 || !Target)return TEXT("HUNT_TARGET_UNAVAILABLE");
        if(!Player || FVector::Dist2D(Player->GetActorLocation(),Target->GetActorLocation())>3000)return TEXT("TARGET_NOT_KNOWN");
        const auto* Weapon=Bag->FindInstance(Bag->EquippedInstance(TEXT("weapon")));
        return Weapon && Weapon->Durability>0?FString():TEXT("WEAPON_REQUIRED");
    }
    if(Goal.Intent==TEXT("fish"))
    {
        auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        const auto* Point=Nature->State.Points.FindByPredicate([&](const auto& P){return P.Id==Goal.Station && P.Kind==TEXT("fish");});
        const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        if(!Point || Point->Remaining<=0 || !Nature->Actor(Goal.Station))return TEXT("FISH_POINT_UNAVAILABLE");
        if(!Player || FVector::Dist2D(Player->GetActorLocation(),Point->Position)>3000)return TEXT("TARGET_NOT_KNOWN");
        if(Bag->Available(TEXT("bait"))<1)return TEXT("BAIT_REQUIRED");
        const auto* Rod=Bag->FindInstance(Bag->EquippedInstance(TEXT("tool")));
        return Rod && Rod->Durability>0 && HearthwardData::Text(HearthwardData::Find(TEXT("items"),Rod->Definition.ToString()),TEXT("toolKind"))==TEXT("fishing_rod")
            ?FString():TEXT("FISHING_ROD_REQUIRED");
    }
    if(Goal.Intent==TEXT("capture"))
    {
        auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        const auto* Animal=Nature->State.Animals.FindByPredicate([&](const auto& A){return A.Id==Goal.Station;});
        const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        if(!Animal || !Animal->Domestic || Animal->Definition!=Goal.Item || Animal->Health<=0 || !Nature->Actor(Goal.Station))return TEXT("CAPTURE_TARGET_UNAVAILABLE");
        if(!Player || FVector::Dist2D(Player->GetActorLocation(),Animal->Position)>3000)return TEXT("TARGET_NOT_KNOWN");
        const auto& Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
        const FName TaskCamp=IsValid(Camp)?Camps.CampAt(Camp->GetActorLocation()):NAME_None;
        if(TaskCamp.IsNone())return TEXT("CAMP_UNAVAILABLE");
        if(Animal->Captured && Animal->ReservedPen.IsValid())
        {
            const auto* Pen=Nature->State.Pens.FindByPredicate([&](const auto& P){return P.Id==Animal->ReservedPen;});
            return Pen && Camps.CampAt(Pen->Position)==TaskCamp?FString():TEXT("PEN_UNAVAILABLE");
        }
        const bool PenAvailable=Nature->State.Pens.ContainsByPredicate([&](const auto& P){return P.Id!=Animal->Pen && P.Definition==Animal->Definition
            && Camps.CampAt(P.Position)==TaskCamp && Nature->State.Occupants(P.Id)<HearthwardNature::Capacity(P.Level);});
        if(!PenAvailable)return TEXT("PEN_FULL_OR_MISSING");
        return !Animal->Captured && (Bag->Available(TEXT("feed"))<1 || Bag->Available(TEXT("rope"))<1)?TEXT("CAPTURE_MATERIALS_REQUIRED"):FString();
    }
    if(Goal.Intent==TEXT("camp_batch"))
    {
        auto* Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
        const auto* Facility=Camps->State.Facilities.FindByPredicate([&](const auto& F){return F.Id==Goal.Station;});
        const auto* Region=Camps->State.Regions.FindByPredicate([&](const auto& R){return R.Facility==Goal.Station && R.Job==Goal.Item;});
        const auto Recipe=HearthwardCamp::Recipe(Goal.Item);
        const FName TaskCamp=IsValid(Camp)?Camps->State.CampAt(Camp->GetActorLocation()):NAME_None;
        auto* Registry=WorkshopRegistry(GetWorld());
        if(TaskCamp.IsNone() || !Facility || Facility->Camp!=TaskCamp || Facility->Paused || !Registry || !Registry->ResolveFacility(Goal.Station)
            || !Region || Region->Camp!=TaskCamp || Region->Enabled || Region->Batch.Active || !Region->Brother || Region->Player || !Region->Workers.IsEmpty()
            || Region->ToRations || !Recipe || HearthwardData::Text(Recipe,TEXT("facility"))!=Facility->Kind.ToString()
            || HearthwardData::Number(Recipe,TEXT("level"))>Facility->Level)return TEXT("CAMP_REGION_UNAVAILABLE");
        const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        const auto* Gameplay=Player?Player->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
        if(!Gameplay || !Gameplay->KnowsRecipe(Goal.Item))return TEXT("RECIPE_LOCKED");
        const auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        for(const auto& Input:HearthwardCamp::Counts(Recipe,TEXT("inputs")))
            if(Storage->Available(Input.Key)<Input.Value)return TEXT("CAMP_MATERIALS_MISSING");
        return {};
    }
    auto* Registry=WorkshopRegistry(GetWorld());
    const auto Recipe=HearthwardData::Find(Goal.Intent==TEXT("craft")?TEXT("craftingRecipes"):TEXT("repairRecipes"),Goal.Item.ToString());
    const auto* Facility=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Facilities.FindByPredicate(
        [&](const auto& F){return F.Id==Goal.Station;});
    if(!Registry || !Registry->ResolveFacility(Goal.Station) || !Recipe || !Facility || Facility->Paused
        || Facility->Kind!=FName(*HearthwardData::Text(Recipe,TEXT("facility")))
        || Facility->Level<HearthwardData::Number(Recipe,TEXT("facilityLevel")))return TEXT("STATION_UNAVAILABLE");
    TMap<FName,int32> Cost=HearthwardWorkshop::Materials(Goal.Intent,Goal.Item,Goal.Quantity);
    if(Goal.Intent==TEXT("repair"))
    {
        if(!Goal.EquipmentId.IsValid() && Bag->GetItemCount(Goal.Item)!=1)return TEXT("AMBIGUOUS_TARGET");
        const FGuid Target=Goal.EquipmentId.IsValid()?Goal.EquipmentId:Bag->FirstInstance(Goal.Item);
        const auto* Instance=Bag->FindInstance(Target);
        if(!Instance || Instance->Definition!=Goal.Item)return TEXT("TARGET_UNAVAILABLE");
        double Restored=0;
        if(!HearthwardWorkshop::RepairQuote(Bag,Target,1,Cost,Restored))return TEXT("ALREADY_REPAIRED");
    }
    if(!HearthwardAgent::AllowsCost(Goal.Limits,Cost,{}))return TEXT("POLICY_CONFLICT");
    return {};
}
EHearthwardProposalResult AHearthwardCompanionFixture::SubmitGoal(AActor* Speaker,FHearthwardCommandTicket Ticket,const FHearthwardAgentGoal& Goal)
{
    if(!PreviewGoal(Goal).IsEmpty())return EHearthwardProposalResult::Unsupported;
    FHearthwardAgentPlan CheckedPlan;FString PlanError;
    if(!HearthwardPlan::Build(Goal,CheckedPlan,PlanError))return EHearthwardProposalResult::Unsupported;
    FName Output=Goal.Item;int32 Count=Goal.Quantity;
    if(Goal.Intent==TEXT("craft"))
    {
        const auto Outputs=HearthwardWorkshop::Outputs(Goal.Item,Goal.Quantity);if(Outputs.Num()!=1)return EHearthwardProposalResult::Unsupported;
        for(const auto& I:Outputs) {Output=I.Key;Count=I.Value;}
    }
    const auto Result=AcceptGoal(Speaker,Ticket,Output,Count,{TEXT("collect"),TEXT("return"),TEXT("deposit")},Goal);
    if(Result==EHearthwardProposalResult::Accepted)
        GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->BeginCommandCoverage(GetCommandId());
    return Result;
}
bool AHearthwardCompanionFixture::ResumeBlocked(AActor* Speaker)
{
    using P=EHearthwardCompanionPhase;
    if(!CanCommunicate(Speaker) || bSettling || GetWorld()->IsPaused() || (Phase!=P::HoldingSafely && Phase!=P::WaitingAtCamp))return false;
    auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if(!Command.IsCurrent(Storage->GetTimelineEpoch()))return false;
    if(!Execution.Current())RestoreExecutionPlan();
    if(!Execution.Current() && !BuildExecutionPlan(false))return false;

    NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();BlockReason.Reset();
    if(Command.Carried>0 && Command.Goal.Intent!=TEXT("give"))
    {
        Execution.bRecoveryToCamp=true;
        Phase=P::ReturningBlocked;
        return true;
    }

    const auto Safety=HearthwardPerception::Evaluate(HearthwardPerception::Capture(this),Command.Goal);
    if(!Safety.IsAllowed()){BlockReason=Safety.Reason;return false;}
    if(Command.Goal.Intent==TEXT("collect") && IsSourceValid() && Source->GetItemCount(Command.GetItem())<=0)
    {BlockReason=TEXT("实际资源不足：指定采集点已采尽");return false;}
    if(Command.Goal.Intent==TEXT("nature_collect"))
        if(const FString Reason=ResourceTargetReason(Command.Goal,false);!Reason.IsEmpty())
        {BlockReason=Reason;return false;}
    if(Command.Goal.Intent==TEXT("camp_batch") && CampBatchBaseline>=0)
    {
        auto* R=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Regions.FindByPredicate(
            [&](const auto& X){return X.Facility==Command.Goal.Station && X.Job==Command.Goal.Item;});
        if(!R || R->BatchStopAt!=CampBatchBaseline+Command.Goal.Quantity)return false;
        R->Enabled=true;
    }
    if(Command.Goal.Intent==TEXT("give") || Command.Goal.Intent==TEXT("receive"))
    {
        auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        if(!PlayerAtTaskCamp(Player)){BlockReason=TEXT("PLAYER_LEFT_CAMP");return false;}
        const auto& Camps=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
        if(Camps.CampAt(GetActorLocation())!=Camps.CampAt(Camp->GetActorLocation()))
        {Execution.bRecoveryToCamp=true;Phase=P::ReturningBlocked;return true;}
        if(Command.Goal.Intent==TEXT("give") && Command.Carried==0)
        {
            const int32 Adopted=FMath::Min(Bag->Available(Command.GetItem()),Command.GetRequested()-Command.GetAcquired());
            if(Adopted<=0 || !Command.RecordAcquisition(Adopted)){BlockReason=TEXT("BAG_INSUFFICIENT");return false;}
            Event(TEXT("retained_adopted"),Command.GetItem(),Adopted,TEXT("physical_cargo_reused"));
        }
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Player);
    }

    if(const auto* Current=Execution.Current())
    {
        using A=EHearthwardAgentActionType;using T=EHearthwardAgentTarget;
        if(Current->Type==A::Gather)
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,A::MoveTo,T::Source);
        else if(Current->Type==A::CommitNature)
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,A::MoveTo,T::Nature);
        else if(Current->Type==A::LeadAnimal)
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,A::MoveTo,T::Nature);
        else if(Current->Type==A::TakeMaterials)
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,A::MoveTo,T::Camp);
        else if(Command.Goal.Intent==TEXT("retrieve") || Command.Goal.Intent==TEXT("fetch"))
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,A::MoveTo,T::Camp);
        else if(Current->Type==A::CommitWorkshop)
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,A::MoveTo,T::Workshop);
    }
    Execution.bRecoveryToCamp=false;Execution.bStarted=false;Action->InterruptAction();SyncPhaseFromExecution();
    return Execution.Current()!=nullptr;
}
void AHearthwardCompanionFixture::Event(FName Kind,FName Item,int32 Count,const FString& Reason,FGuid Operation)
{
    if(!Operation.IsValid())Operation=FGuid::NewGuid();
    if(AppliedOperations.Contains(Operation))return;
    AppliedOperations.Add(Operation);
    FHearthwardNPCEvent E;E.Id=Operation;E.Command=Command.GetActive().Id;E.Campaign=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetCampaignId();E.Kind=Kind;E.Item=Item;E.Count=Count;E.Reason=Reason;E.At=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds;
    GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->RecordEvent(E);
}
void AHearthwardCompanionFixture::WorkshopTick()
{
    const auto* Current=Execution.Current();
    if(!Current)return;
    const auto& G=Command.Goal;
    auto* Registry=WorkshopRegistry(GetWorld());AActor* Station=Registry?Registry->ResolveFacility(G.Station):nullptr;
    if(!IsValid(Station)){HandleExecutionFailure(TEXT("STATION_UNAVAILABLE"));return;}
    const FGuid RepairId=G.Intent==TEXT("repair")?(G.EquipmentId.IsValid()?G.EquipmentId:Bag->FirstInstance(G.Item)):FGuid();
    auto Cost=HearthwardWorkshop::Materials(G.Intent,G.Item,G.Quantity);
    if(G.Intent==TEXT("repair"))
    {
        const auto* Instance=Bag->FindInstance(RepairId);
        double Restored=0;
        if(!Instance || Instance->Definition!=G.Item || !HearthwardWorkshop::RepairQuote(Bag,RepairId,1,Cost,Restored))
        {HandleExecutionFailure(TEXT("TARGET_UNAVAILABLE"));return;}
    }
    if(!HearthwardAgent::AllowsCost(G.Limits,Cost,Spent)){HandleExecutionFailure(TEXT("POLICY_CONFLICT"));return;}

    if(Current->Type==EHearthwardAgentActionType::TakeMaterials)
    {
        if(!At(Camp))
        {
            if(!MoveTowards(Camp,0)){HandleExecutionFailure(TEXT("PATH_BLOCKED"));return;}
            return;
        }
        auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        int64 AddedWeight=0;
        for(const auto& C:Cost)
        {
            const int32 Missing=C.Value;
            const auto Ticket=Command.GetActive();const FGuid Op(Ticket.Id.A,Ticket.Id.B,Ticket.Id.C^0xCA01^FCrc::StrCrc32(*C.Key.ToString()),Ticket.Id.D);
            const FString Payload=FString::Printf(TEXT("take:%s:%d:r%lld"),*C.Key.ToString(),Missing,Ticket.Revision);
            if(const auto* Receipt=Receipts.FindByPredicate([&](const auto& R){return R.Id==Op;}))
            {
                if(Receipt->Command!=Ticket.Id || Receipt->Payload!=Payload){HandleExecutionFailure(TEXT("SETTLEMENT_FAILED"));return;}
                continue;
            }
            if(Storage->Available(C.Key)<Missing){HandleExecutionFailure(TEXT("INSUFFICIENT_MATERIAL"));return;}
            const auto* D=HearthwardBasicItems().FindByPredicate([&](const auto& I){return I.Id==C.Key;});
            if(!D){HandleExecutionFailure(TEXT("ITEM_UNAVAILABLE"));return;}
            AddedWeight+=int64(Missing)*D->WeightHundredths;
        }
        if(Bag->GetWeight()*100+AddedWeight>Bag->GetCapacity()*100){HandleExecutionFailure(TEXT("CAPACITY_EXCEEDED"));return;}

        TGuardValue<bool> Guard(bSettling,true);
        for(const auto& C:Cost)
        {
            const int32 Missing=C.Value;if(!Missing)continue;
            const auto Ticket=Command.GetActive();const FGuid Op(Ticket.Id.A,Ticket.Id.B,Ticket.Id.C^0xCA01^FCrc::StrCrc32(*C.Key.ToString()),Ticket.Id.D);
            if(!HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,Storage->GetTimelineEpoch(),
                FString::Printf(TEXT("take:%s:%d:r%lld"),*C.Key.ToString(),Missing,Ticket.Revision),[&]
                {
                    const auto R=Storage->Transfer(Bag,false,C.Key,Missing,Op,Ticket.Epoch);
                    if(R.Result!=EHearthwardInventoryResult::Success)return false;
                    Event(TEXT("materials_taken"),C.Key,R.MovedCount,TEXT("authorized_camp"),Op);return true;
                }))
            {HandleExecutionFailure(TEXT("INSUFFICIENT_MATERIAL"));return;}
        }
        Execution.AdaptiveRecoveryAttempts=0;
        Execution.LastRecoveryReason.Reset();
        StopNavigation();AdvanceExecution();return;
    }

    if(Current->Type!=EHearthwardAgentActionType::CommitWorkshop)return;
    const FString Error=HearthwardWorkshop::Check(this,Station,Bag,G.Intent,G.Item,G.Quantity,RepairId);
    if(Error==TEXT("OUT_OF_RANGE") || Error==TEXT("PATH_BLOCKED"))
    {
        if(!MoveTowards(Station,0,160))HandleExecutionFailure(TEXT("PATH_BLOCKED"));
        return;
    }
    if(!Error.IsEmpty()){HandleExecutionFailure(Error);return;}

    StopNavigation();TGuardValue<bool> Guard(bSettling,true);
    const float Before=G.Intent==TEXT("repair")?Bag->FindInstance(RepairId)->Durability:0;
    const auto Ticket=Command.GetActive();const FGuid Op(Ticket.Id.A,Ticket.Id.B,Ticket.Id.C^0xCFA1,Ticket.Id.D);
    if(!HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(),
        FString::Printf(TEXT("%s:%s:%d:r%lld"),*G.Intent.ToString(),*G.Item.ToString(),G.Quantity,Ticket.Revision),[&]
        {
            if(!HearthwardWorkshop::Commit(Bag,G.Intent,G.Item,G.Quantity,RepairId))return false;
            for(const auto& C:Cost)Spent.FindOrAdd(C.Key)+=C.Value;
            Execution.AdaptiveRecoveryAttempts=0;
            Execution.LastRecoveryReason.Reset();
            Event(G.Intent,G.Item,G.Quantity,G.Intent==TEXT("repair")?FString::Printf(TEXT("耐久 %.0f → %.0f"),Before,Bag->FindInstance(RepairId)->Durability):TEXT("实际扣料并产生物品"),Op);
            Command.RecordAcquisition(Command.GetRequested());
            if(G.Intent==TEXT("repair"))
            {
                Command.Carried=0;
                Command.RecordDelivery(Command.GetActive(),1);
            }
            AdvanceExecution();
            return true;
        }))HandleExecutionFailure(TEXT("SETTLEMENT_FAILED"));
}

void AHearthwardCompanionFixture::StopForSurvival()
{
    CancelMeleeAttack();
    if(bSettling) return;
    StopCampBatch();
    StopNavigation();Action->InterruptAction();
    const auto* Survival=FindComponentByClass<UHearthwardSurvivalComponent>();
    const auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if(Survival && Survival->Alive() && Storage && Command.IsCurrent(Storage->GetTimelineEpoch()))
    {
        if(Phase!=EHearthwardCompanionPhase::HoldingSafely)
            Event(TEXT("blocked"),Command.GetItem(),Command.GetDelivered(),TEXT("生存行动暂停，等待玩家明确继续"));
        Execution.bStarted=false;
        Execution.bRecoveryToCamp=false;
        Phase=EHearthwardCompanionPhase::HoldingSafely;
        BlockReason=TEXT("生存行动暂停了原委托；会合后由玩家明确继续");
        return;
    }
    Command.Cancel();Execution.Reset();Phase=EHearthwardCompanionPhase::Cancelled;
}

void AHearthwardCompanionFixture::FellOutOfWorld(const UDamageType& DamageType)
{
    auto* S=FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!S || !S->Enabled()) { Super::FellOutOfWorld(DamageType); return; }
    S->FatalEnvironment();
}
void AHearthwardCompanionFixture::Landed(const FHitResult& Hit)
{
    const float Speed=FMath::Max(0.f,-GetVelocity().Z);
    Super::Landed(Hit);
    if(auto* S=FindComponentByClass<UHearthwardSurvivalComponent>()) S->FallImpact(Speed);
}

float AHearthwardCompanionFixture::EquipmentDurability(FName Item) const
{const auto* I=Bag->FindInstance(Bag->FirstInstance(Item));return I?I->Durability:0;}
