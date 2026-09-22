#include "HearthwardCompanionFixture.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../AI/HearthwardNPCPerception.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Building/HearthwardWorkshopService.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Kismet/GameplayStatics.h"

namespace
{
UHearthwardBuildingComponent* WorkshopRegistry(UWorld* World);
}

AHearthwardCompanionFixture::AHearthwardCompanionFixture()
{
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
    auto* MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    MarkerMesh->SetupAttachment(Capsule);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    MarkerMesh->SetStaticMesh(Shape.Object);
    MarkerMesh->SetRelativeScale3D(FVector(0.6, 0.6, 1.6));
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bag = CreateDefaultSubobject<UHearthwardInventoryComponent>(TEXT("FixtureBag"));
    Action = CreateDefaultSubobject<UHearthwardTimedActionComponent>(TEXT("FixtureGatherTimer"));
    Tags.Add(TEXT("Hearthward.Companion.PROTOTYPE_ONLY"));
}

void AHearthwardCompanionFixture::InitializeFixture(UHearthwardInventoryComponent* Resource, AActor* CampActor)
{
#if !UE_BUILD_SHIPPING
    Source = Resource;
    Camp = CampActor;
    bSourceSafe = true;
    bFixtureEnabled = true;
    SetActorTickEnabled(true);
#endif
}

bool AHearthwardCompanionFixture::CanCommunicate(AActor* Speaker) const
{
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

    const auto Result = Command.Accept(Ticket, GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(), ItemId, Quantity, Steps);
    if (Result == R::Accepted)
    {
        StopNavigation();
        Action->InterruptAction();
        BlockReason.Reset();
        Spent.Reset();AppliedOperations.Reset();Receipts.Reset();NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();
        Command.Goal=Goal;
        if(Goal.Intent==TEXT("collect"))
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
        if(!BuildExecutionPlan(true))
        {
            Command.Cancel();
            return R::Unsupported;
        }
    }
    return Result;
}

bool AHearthwardCompanionFixture::Cancel(AActor* Speaker)
{
    if (bSettling || !CanCommunicate(Speaker) || GetWorld()->IsPaused()) return false;
    if(Command.IsCurrent(GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())) Event(TEXT("cancelled"),Command.GetItem(),Command.GetDelivered());
    Command.Cancel();
    Execution.Reset();
    StopNavigation();
    Action->InterruptAction();
    Phase = EHearthwardCompanionPhase::Cancelled;
    return true;
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
    if(PreferReturnForExistingCargo && Command.Goal.Intent==TEXT("collect") && Bag->GetWeight()>0)
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
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Source);break;
    case P::Gathering:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::Gather,EHearthwardAgentTarget::Source);
        Execution.bStarted=true;break;
    case P::TakingMaterials:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::TakeMaterials,EHearthwardAgentTarget::Camp);break;
    case P::GoingToWorkshop:
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Workshop);break;
    case P::Returning:
        Execution.Cursor=HearthwardPlan::FindLast(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Camp);break;
    case P::ReturningBlocked:
    case P::HoldingSafely:
        Execution.bRecoveryToCamp=true;
        Execution.Cursor=Command.Carried>0
            ? HearthwardPlan::FindLast(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Camp)
            : 0;
        break;
    case P::WaitingAtCamp:
        if(Command.Goal.Intent==TEXT("collect"))
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Source);
        else if(Command.Goal.SourceRef==TEXT("camp"))
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
        return Registry?Registry->ResolveWorkbench(Command.Goal.Station):nullptr;
    }
    default:
        return nullptr;
    }
}

bool AHearthwardCompanionFixture::ActionRequiresSafety(const FHearthwardAgentAction& Current) const
{
    if(Current.Type==EHearthwardAgentActionType::Deposit)return false;
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
    if(Current->Type==A::MoveTo && Current->Target==T::Source)Phase=EHearthwardCompanionPhase::GoingToSource;
    else if(Current->Type==A::Gather)Phase=EHearthwardCompanionPhase::Gathering;
    else if(Current->Type==A::MoveTo && Current->Target==T::Camp)Phase=EHearthwardCompanionPhase::Returning;
    else if(Current->Type==A::Deposit)Phase=EHearthwardCompanionPhase::Returning;
    else if(Current->Type==A::TakeMaterials)Phase=EHearthwardCompanionPhase::TakingMaterials;
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

    if(Command.Goal.Intent==TEXT("collect") && Command.GetDelivered()<Command.GetRequested())
    {
        Execution.Cursor=HearthwardPlan::Find(Execution.Plan,EHearthwardAgentActionType::MoveTo,EHearthwardAgentTarget::Source);
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


bool AHearthwardCompanionFixture::At(const AActor* Target) const
{
    return IsValid(Target) && !Target->IsActorBeingDestroyed() && Target->GetWorld() == GetWorld()
        && FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(50.0);
}

bool AHearthwardCompanionFixture::MoveTowards(const AActor* Target, float DeltaSeconds,float AcceptanceRadius)
{
    if (!IsValid(Target) || Target->IsActorBeingDestroyed()) return false;
    const double Now=GetWorld()->GetTimeSeconds();
    if(FVector::Dist(LastProgressPosition,Target->GetActorLocation())-FVector::Dist(GetActorLocation(),Target->GetActorLocation())>20)
    {LastProgressPosition=GetActorLocation();LastProgressAt=Now;}
    if(Now-LastProgressAt>HearthwardAgent::Policy(TEXT("no_progress_seconds"))) return false;
    if(NavigationFailures>HearthwardAgent::Policy(TEXT("max_navigation_retries"))) return false;
    const bool Attempt=Now>=NavigationRetryAt;
    if(NavigateTo(const_cast<AActor*>(Target),180,AcceptanceRadius))return true;
    if(Attempt) ++NavigationFailures;
    return NavigationFailures<=HearthwardAgent::Policy(TEXT("max_navigation_retries"));
}

void AHearthwardCompanionFixture::StopNavigation()
{
    if (auto* AI = Cast<AAIController>(GetController())) AI->StopMovement();
    GetCharacterMovement()->StopMovementImmediately();
    NavigationTarget.Reset(); NavigationRetryAt = 0;
}

bool AHearthwardCompanionFixture::NavigateTo(AActor* Target, float Speed, float AcceptanceRadius)
{
    auto* AI = Cast<AAIController>(GetController());
    if (!AI || !IsValid(Target) || Target->IsActorBeingDestroyed()) { StopNavigation(); return false; }
    GetCharacterMovement()->MaxWalkSpeed = Speed;
    if (NavigationTarget != Target || !FMath::IsNearlyEqual(NavigationAcceptance, AcceptanceRadius))
    {
        StopNavigation(); NavigationTarget = Target; NavigationAcceptance = AcceptanceRadius;
    }
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    // Runtime tiles may still be rebuilding after a world obstacle changes.
    if (Nav && Nav->IsNavigationBuildInProgress()) return true;
    if (AI->GetMoveStatus() != EPathFollowingStatus::Idle) return true;
    if (GetWorld()->GetTimeSeconds() < NavigationRetryAt) return false;
    EPathFollowingRequestResult::Type Result;
    if(Target->ActorHasTag(TEXT("Hearthward.Building.Completed")))
    {
        // A built workbench cuts a hole in navigation. Its solid centre cannot be the path endpoint.
        FVector Direction=(GetActorLocation()-Target->GetActorLocation()).GetSafeNormal2D();
        if(Direction.IsNearlyZero())Direction=Target->GetActorForwardVector();
        const double Reach=HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("crafting")),TEXT("reach"));
        const FVector Approach=Target->GetActorLocation()+Direction*(Reach-60);
        FNavLocation Projected;
        const bool ProjectedOK=Nav && Nav->ProjectPointToNavigation(Approach,Projected,FVector(50,50,200));
        Result=ProjectedOK ? AI->MoveToLocation(Projected.Location,30,false,true,false,false,nullptr,false) : EPathFollowingRequestResult::Failed;
        UE_LOG(LogTemp,Display,TEXT("NPC workshop approach: actor=%s station=%s desired=%s projected=%s valid=%d move=%d"),
            *GetActorLocation().ToString(),*Target->GetActorLocation().ToString(),*Approach.ToString(),*Projected.Location.ToString(),ProjectedOK,int32(Result));
    }
    else Result=AI->MoveToActor(Target, AcceptanceRadius, false, true, false, nullptr, false);
    if (Result == EPathFollowingRequestResult::Failed)
    {
        NavigationRetryAt = GetWorld()->GetTimeSeconds() + .5;
        return false;
    }
    return true;
}

void AHearthwardCompanionFixture::ReturnBlocked(const FString& Reason)
{
    if (!Execution.bRecoveryToCamp)
    {
        StopNavigation();NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();
        Event(TEXT("blocked"),Command.GetItem(),Command.GetDelivered(),Reason);
    }
    Action->InterruptAction();
    Execution.bStarted=false;
    Execution.bRecoveryToCamp=true;
    Phase = EHearthwardCompanionPhase::ReturningBlocked;
    BlockReason = Reason;
}

void AHearthwardCompanionFixture::Deposit()
{
    if (!At(Camp)) return;
    StopNavigation();
    const bool bRecovery=Execution.bRecoveryToCamp;
    auto* Storage = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    const auto Ticket = Command.GetActive();
    TGuardValue<bool> Guard(bSettling, true);
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
        if(OwnedDurability.Contains(Item.Id))continue;
        if(Command.Goal.Intent!=TEXT("collect") || Item.Id==Command.GetItem())Count=Owned;
        if (Count == 0) continue;
        const FGuid Op(Ticket.Id.A,Ticket.Id.B,Ticket.Id.C^0xDE01^FCrc::StrCrc32(*Item.Id.ToString()),Ticket.Id.D^uint32(Command.Acquired));
        const bool Settled=HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,Storage->GetTimelineEpoch(),
            FString::Printf(TEXT("deposit:%s:%d:owned%d:r%lld"),*Item.Id.ToString(),Count,Owned,Ticket.Revision),[&]
            {
                const auto Result=Storage->Transfer(Bag,true,Item.Id,Count,Op,Ticket.Epoch);
                if(Result.Result!=EHearthwardInventoryResult::Success)return false;
                if(Item.Id==Command.GetItem() && Result.MovedCount>0 && Owned>0)
                {Command.Carried-=Owned;Command.RecordDelivery(Ticket,Owned);Event(TEXT("delivered"),Item.Id,Owned,FString(),Op);}
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

void AHearthwardCompanionFixture::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    using P = EHearthwardCompanionPhase;
    if (!bFixtureEnabled || GetWorld()->IsPaused() || bSettling) return;
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
        if(!Safety.IsAllowed()){ReturnBlocked(Safety.Reason);return;}
    }

    using A=EHearthwardAgentActionType;
    using T=EHearthwardAgentTarget;
    if(Current->Type==A::MoveTo)
    {
        if(Current->Target==T::Source)
        {
            if(!IsSourceValid()){ReturnBlocked(TEXT("SOURCE_UNAVAILABLE"));return;}
            if(Source->GetItemCount(Command.GetItem())<=0){ReturnBlocked(TEXT("实际资源不足"));return;}
        }
        AActor* Target=ResolveActionTarget(*Current);
        if(!IsValid(Target))
        {
            ReturnBlocked(Current->Target==T::Source?TEXT("SOURCE_UNAVAILABLE"):
                Current->Target==T::Camp?TEXT("CAMP_UNAVAILABLE"):TEXT("STATION_UNAVAILABLE"));
            return;
        }
        const float Acceptance=Current->Target==T::Workshop
            ? float(HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("crafting")),TEXT("reach"))) : 40.f;
        if(At(Target) || (Current->Target==T::Workshop && FVector::Dist2D(GetActorLocation(),Target->GetActorLocation())<=Acceptance))
        {
            StopNavigation();
            AdvanceExecution();
            return;
        }
        if(!MoveTowards(Target,DeltaSeconds,Acceptance))
        {
            ReturnBlocked(Current->Target==T::Camp?TEXT("RETURN_UNREACHABLE"):
                Current->Target==T::Source?TEXT("去程受阻"):TEXT("PATH_BLOCKED"));
        }
        return;
    }

    if(Current->Type==A::Gather)
    {
        if(!IsSourceValid()){ReturnBlocked(TEXT("SOURCE_UNAVAILABLE"));return;}
        if(Source->GetItemCount(Command.GetItem())<=0){ReturnBlocked(TEXT("实际资源不足"));return;}
        if(!At(Source->GetOwner())){ReturnBlocked(TEXT("SOURCE_POSITION_CHANGED"));return;}

        if(!Execution.bStarted)
        {
            StopNavigation();
            if(!Action->StartAction()){ReturnBlocked(TEXT("无法开始采集"));return;}
            Execution.bStarted=true;
            return;
        }
        if(Action->GetStatus()==EHearthwardTimedActionStatus::Interrupted){ReturnBlocked(TEXT("采集被中断"));return;}
        if(Action->GetStatus()!=EHearthwardTimedActionStatus::Completed)return;

        const auto* Item=HearthwardBasicItems().FindByPredicate([this](const auto& Def){return Def.Id==Command.GetItem();});
        if(!Item){ReturnBlocked(TEXT("ITEM_UNAVAILABLE"));return;}
        int32 CargoWeight=0;
        for(const auto& D:HearthwardBasicItems())if(!OwnedDurability.Contains(D.Id))CargoWeight+=Bag->GetItemCount(D.Id)*D.WeightHundredths;
        const int32 Free=FMath::Max(0,FMath::Min(400-CargoWeight,FMath::RoundToInt((Bag->GetCapacity()-Bag->GetWeight())*100)));
        const int32 Count=FMath::Min3(Free/Item->WeightHundredths,Source->GetItemCount(Item->Id),Command.GetRequested()-Command.GetAcquired());
        if(Count<=0){ReturnBlocked(TEXT("本趟无法携带目标物品"));return;}

        auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        TGuardValue<bool> Guard(bSettling,true);
        const auto Ticket=Command.GetActive();const FGuid Op(Ticket.Id.A,Ticket.Id.B,Ticket.Id.C^0xAC01,Ticket.Id.D^uint32(Command.Acquired));
        if(!HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,Storage->GetTimelineEpoch(),
            FString::Printf(TEXT("acquire:%s:%d:%d"),*Item->Id.ToString(),Count,Command.Acquired),[&]
            {
                if(Source->TransferTo(Bag,Item->Id,Count)!=EHearthwardInventoryResult::Success)return false;
                Command.RecordAcquisition(Count);Event(TEXT("acquired"),Item->Id,Count,FString(),Op);return true;
            }))
        {
            ReturnBlocked(TEXT("采集结算时资源或容量不足"));
            return;
        }
        AdvanceExecution();
        return;
    }

    if(Current->Type==A::TakeMaterials || Current->Type==A::CommitWorkshop)
    {
        WorkshopTick();
        return;
    }

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
FString AHearthwardCompanionFixture::PreviewGoal(const FHearthwardAgentGoal& Goal) const
{
    const FString Error=HearthwardAgent::Validate(Goal);if(!Error.IsEmpty())return Error;
    const auto Safety=HearthwardPerception::Evaluate(HearthwardPerception::Capture(this),Goal);
    if(!Safety.IsAllowed())return Safety.Reason;
    if(Goal.Intent==TEXT("collect")) return {};
    auto* Registry=WorkshopRegistry(GetWorld());if(!Registry || !Registry->ResolveWorkbench(Goal.Station))return TEXT("STATION_UNAVAILABLE");
    if(Goal.Intent==TEXT("repair"))
    {
        if(Bag->GetItemCount(Goal.Item)!=1 || !OwnedDurability.Contains(Goal.Item))return TEXT("AMBIGUOUS_TARGET");
        if(OwnedDurability[Goal.Item]>=HearthwardData::Number(HearthwardData::Find(TEXT("items"),Goal.Item.ToString()),TEXT("durability")))return TEXT("ALREADY_REPAIRED");
    }
    if(!HearthwardAgent::AllowsCost(Goal.Limits,HearthwardWorkshop::Materials(Goal.Intent,Goal.Item,Goal.Quantity),{}))return TEXT("POLICY_CONFLICT");
    return {};
}
EHearthwardProposalResult AHearthwardCompanionFixture::SubmitGoal(AActor* Speaker,FHearthwardCommandTicket Ticket,const FHearthwardAgentGoal& Goal)
{
    if(!PreviewGoal(Goal).IsEmpty())return EHearthwardProposalResult::Unsupported;
    FName Output=Goal.Item;int32 Count=Goal.Quantity;
    if(Goal.Intent==TEXT("craft"))
    {
        const auto Outputs=HearthwardWorkshop::Outputs(Goal.Item,Goal.Quantity);if(Outputs.Num()!=1)return EHearthwardProposalResult::Unsupported;
        for(const auto& I:Outputs) {Output=I.Key;Count=I.Value;}
    }
    return AcceptGoal(Speaker,Ticket,Output,Count,{TEXT("collect"),TEXT("return"),TEXT("deposit")},Goal);
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
    if(Command.Carried>0)
    {
        Execution.bRecoveryToCamp=true;
        Phase=P::ReturningBlocked;
        return true;
    }

    const auto Safety=HearthwardPerception::Evaluate(HearthwardPerception::Capture(this),Command.Goal);
    if(!Safety.IsAllowed()){BlockReason=Safety.Reason;return false;}

    if(const auto* Current=Execution.Current())
    {
        using A=EHearthwardAgentActionType;using T=EHearthwardAgentTarget;
        if(Current->Type==A::Gather)
            Execution.Cursor=HearthwardPlan::Find(Execution.Plan,A::MoveTo,T::Source);
        else if(Current->Type==A::TakeMaterials)
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
    auto* Registry=WorkshopRegistry(GetWorld());AActor* Station=Registry?Registry->ResolveWorkbench(G.Station):nullptr;
    if(!IsValid(Station)){ReturnBlocked(TEXT("STATION_UNAVAILABLE"));return;}
    const auto Cost=HearthwardWorkshop::Materials(G.Intent,G.Item,G.Quantity);
    if(!HearthwardAgent::AllowsCost(G.Limits,Cost,Spent)){ReturnBlocked(TEXT("POLICY_CONFLICT"));return;}

    if(Current->Type==EHearthwardAgentActionType::TakeMaterials)
    {
        if(!At(Camp))
        {
            if(!MoveTowards(Camp,0)){ReturnBlocked(TEXT("PATH_BLOCKED"));return;}
            return;
        }
        auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        int64 AddedWeight=0;
        for(const auto& C:Cost)
        {
            const int32 Missing=FMath::Max(0,C.Value-Bag->GetItemCount(C.Key));
            if(Storage->GetItemCount(C.Key)<Missing){ReturnBlocked(TEXT("INSUFFICIENT_MATERIAL"));return;}
            const auto* D=HearthwardBasicItems().FindByPredicate([&](const auto& I){return I.Id==C.Key;});
            if(!D){ReturnBlocked(TEXT("ITEM_UNAVAILABLE"));return;}
            AddedWeight+=int64(Missing)*D->WeightHundredths;
        }
        if(Bag->GetWeight()*100+AddedWeight>10000){ReturnBlocked(TEXT("CAPACITY_EXCEEDED"));return;}

        TGuardValue<bool> Guard(bSettling,true);
        for(const auto& C:Cost)
        {
            const int32 Missing=FMath::Max(0,C.Value-Bag->GetItemCount(C.Key));if(!Missing)continue;
            const auto Ticket=Command.GetActive();const FGuid Op(Ticket.Id.A,Ticket.Id.B,Ticket.Id.C^0xCA01^FCrc::StrCrc32(*C.Key.ToString()),Ticket.Id.D);
            if(!HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,Storage->GetTimelineEpoch(),
                FString::Printf(TEXT("take:%s:%d:r%lld"),*C.Key.ToString(),Missing,Ticket.Revision),[&]
                {
                    const auto R=Storage->Transfer(Bag,false,C.Key,Missing,Op,Ticket.Epoch);
                    if(R.Result!=EHearthwardInventoryResult::Success)return false;
                    Event(TEXT("materials_taken"),C.Key,R.MovedCount,TEXT("authorized_camp"),Op);return true;
                }))
            {ReturnBlocked(TEXT("INSUFFICIENT_MATERIAL"));return;}
        }
        StopNavigation();AdvanceExecution();return;
    }

    if(Current->Type!=EHearthwardAgentActionType::CommitWorkshop)return;
    const FString Error=HearthwardWorkshop::Check(this,Station,Bag,&OwnedDurability,G.Intent,G.Item,G.Quantity);
    if(Error==TEXT("OUT_OF_RANGE") || Error==TEXT("PATH_BLOCKED"))
    {
        if(!MoveTowards(Station,0,160))ReturnBlocked(TEXT("PATH_BLOCKED"));
        return;
    }
    if(!Error.IsEmpty()){ReturnBlocked(Error);return;}

    StopNavigation();TGuardValue<bool> Guard(bSettling,true);
    const float Before=OwnedDurability.FindRef(G.Item);
    const auto Ticket=Command.GetActive();const FGuid Op(Ticket.Id.A,Ticket.Id.B,Ticket.Id.C^0xCFA1,Ticket.Id.D);
    if(!HearthwardAgent::Settle(Receipts,Op,Ticket.Id,Ticket.Epoch,GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(),
        FString::Printf(TEXT("%s:%s:%d:r%lld"),*G.Intent.ToString(),*G.Item.ToString(),G.Quantity,Ticket.Revision),[&]
        {
            if(!HearthwardWorkshop::Commit(Bag,&OwnedDurability,G.Intent,G.Item,G.Quantity))return false;
            for(const auto& C:Cost)Spent.FindOrAdd(C.Key)+=C.Value;
            Event(G.Intent,G.Item,G.Quantity,G.Intent==TEXT("repair")?FString::Printf(TEXT("耐久 %.0f → %.0f"),Before,OwnedDurability.FindRef(G.Item)):TEXT("实际扣料并产生物品"),Op);
            Command.RecordAcquisition(Command.GetRequested());
            if(G.Intent==TEXT("repair"))
            {
                Command.Carried=0;
                Command.RecordDelivery(Command.GetActive(),1);
            }
            AdvanceExecution();
            return true;
        }))ReturnBlocked(TEXT("SETTLEMENT_FAILED"));
}
