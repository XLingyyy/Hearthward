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
#include "../Building/HearthwardBuildingComponent.h"
#include "../Building/HearthwardWorkshopService.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Kismet/GameplayStatics.h"

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
    using R = EHearthwardProposalResult;
    if (bSettling || !bFixtureEnabled || GetWorld()->IsPaused()) return R::Unavailable;
    if (!CanCommunicate(Speaker)) return R::OutOfRange;
    if (RequestSpeaker.Get() != Speaker) return R::Stale;
    if (!IsSourceValid() || !IsValid(Camp) || Camp->IsActorBeingDestroyed()) return R::Unavailable;
    if (!bSourceSafe) return R::Unsafe;
    const auto Result = Command.Accept(Ticket, GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(), ItemId, Quantity, Steps);
    if (Result == R::Accepted)
    {
        StopNavigation();
        Action->InterruptAction();
        BlockReason.Reset();
        Spent.Reset();AppliedOperations.Reset();Receipts.Reset();NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();
        Phase = Bag->GetWeight() > 0 ? EHearthwardCompanionPhase::Returning : EHearthwardCompanionPhase::GoingToSource;
    }
    return Result;
}

bool AHearthwardCompanionFixture::Cancel(AActor* Speaker)
{
    if (bSettling || !CanCommunicate(Speaker) || GetWorld()->IsPaused()) return false;
    if(Command.IsCurrent(GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())) Event(TEXT("cancelled"),Command.GetItem(),Command.GetDelivered());
    Command.Cancel();
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

bool AHearthwardCompanionFixture::At(const AActor* Target) const
{
    return IsValid(Target) && !Target->IsActorBeingDestroyed() && Target->GetWorld() == GetWorld()
        && FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(50.0)
        && FMath::Abs(GetActorLocation().Z-Target->GetActorLocation().Z) < 100;
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
    if (Phase != EHearthwardCompanionPhase::ReturningBlocked) {StopNavigation();NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();Event(TEXT("blocked"),Command.GetItem(),Command.GetDelivered(),Reason);}
    Action->InterruptAction();
    Phase = EHearthwardCompanionPhase::ReturningBlocked;
    BlockReason = Reason;
}

void AHearthwardCompanionFixture::Deposit()
{
    if (!At(Camp)) return;
    StopNavigation();
    const bool bWasBlocked = Phase == EHearthwardCompanionPhase::ReturningBlocked;
    auto* Storage = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    const auto Ticket = Command.GetActive();
    TGuardValue<bool> Guard(bSettling, true);
    for (const auto& Item : HearthwardBasicItems())
    {
        if (!Command.IsCurrent(Storage->GetTimelineEpoch()))
        {
            Command.Cancel();
            Phase = EHearthwardCompanionPhase::Cancelled;
            return;
        }
        int32 Count = Bag->GetItemCount(Item.Id);
        const int32 Owned=Item.Id==Command.GetItem()?FMath::Min(Count,Command.Carried):0;
        if(OwnedDurability.Contains(Item.Id))continue;
        if(Command.Goal.Intent!=TEXT("collect"))Count=Owned;
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
            Command.Cancel();
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
            BlockReason.Reset();
            Phase = EHearthwardCompanionPhase::Completed;
            Event(TEXT("completed"),Command.GetItem(),Command.GetDelivered());
            return;
        }
    }
    Phase = bWasBlocked ? EHearthwardCompanionPhase::WaitingAtCamp : EHearthwardCompanionPhase::GoingToSource;
    NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();
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
        Action->InterruptAction();
        Phase = P::Cancelled;
        return;
    }
    if(Phase==P::GoingToWorkshop || Phase==P::TakingMaterials) {WorkshopTick();return;}
    if (Phase == P::GoingToSource || Phase == P::Gathering)
    {
        if (!IsSourceValid() || !bSourceSafe) ReturnBlocked(TEXT("资源点失效或不再安全"));
        else if (Source->GetItemCount(Command.GetItem()) == 0) ReturnBlocked(TEXT("实际资源不足"));
    }
    if (Phase == P::GoingToSource)
    {
        if (!At(Source->GetOwner()))
        {
            if (!MoveTowards(Source->GetOwner(), DeltaSeconds)) ReturnBlocked(TEXT("去程受阻"));
            return;
        }
        StopNavigation();
        if (Action->StartAction()) Phase = P::Gathering;
        else ReturnBlocked(TEXT("无法开始采集"));
    }
    else if (Phase == P::Gathering)
    {
        if (!At(Source->GetOwner()) || Action->GetStatus() == EHearthwardTimedActionStatus::Interrupted)
        {
            ReturnBlocked(TEXT("采集被中断"));
            return;
        }
        if (Action->GetStatus() != EHearthwardTimedActionStatus::Completed) return;
        const auto* Item = HearthwardBasicItems().FindByPredicate([this](const auto& Def) { return Def.Id == Command.GetItem(); });
        int32 CargoWeight=0;
        for(const auto& D:HearthwardBasicItems())if(!OwnedDurability.Contains(D.Id))CargoWeight+=Bag->GetItemCount(D.Id)*D.WeightHundredths;
        const int32 Free=FMath::Max(0,FMath::Min(400-CargoWeight,FMath::RoundToInt((Bag->GetCapacity()-Bag->GetWeight())*100)));
        const int32 Count = FMath::Min3(Free / Item->WeightHundredths,
            Source->GetItemCount(Item->Id), Command.GetRequested() - Command.GetAcquired());
        if (Count <= 0) { ReturnBlocked(TEXT("本趟无法携带目标物品")); return; }
        Phase = P::Returning;
        TGuardValue<bool> Guard(bSettling, true);
        const auto T=Command.GetActive();const FGuid Op(T.Id.A,T.Id.B,T.Id.C^0xAC01,T.Id.D^uint32(Command.Acquired));
        if (!HearthwardAgent::Settle(Receipts,Op,T.Id,T.Epoch,Storage->GetTimelineEpoch(),
            FString::Printf(TEXT("acquire:%s:%d:%d"),*Item->Id.ToString(),Count,Command.Acquired),[&]
            {if(Source->TransferTo(Bag,Item->Id,Count)!=EHearthwardInventoryResult::Success)return false;
             Command.RecordAcquisition(Count);Event(TEXT("acquired"),Item->Id,Count,FString(),Op);return true;}))
            ReturnBlocked(TEXT("采集结算时资源或容量不足"));
        else {NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();}
    }
    else if (Phase == P::Returning || Phase == P::ReturningBlocked)
    {
        if (At(Camp)) Deposit();
        else if (!MoveTowards(Camp, DeltaSeconds)) {StopNavigation();Phase=P::HoldingSafely;BlockReason=TEXT("RETURN_UNREACHABLE：返营路线受阻，保留物资；会合后可重试");Event(TEXT("blocked"),Command.GetItem(),Command.GetDelivered(),BlockReason);}
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
    if(!bSourceSafe || !IsValid(Camp) || !IsSourceValid())return TEXT("UNAVAILABLE");
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
    const auto R=Submit(Speaker,Ticket,Output,Count,{TEXT("collect"),TEXT("return"),TEXT("deposit")});
    if(R!=EHearthwardProposalResult::Accepted)return R;
    Command.Goal=Goal;
    if(Goal.Intent!=TEXT("collect"))Phase=Goal.SourceRef==TEXT("camp")?EHearthwardCompanionPhase::TakingMaterials:EHearthwardCompanionPhase::GoingToWorkshop;
    return R;
}
bool AHearthwardCompanionFixture::ResumeBlocked(AActor* Speaker)
{
    if(!CanCommunicate(Speaker) || bSettling || GetWorld()->IsPaused() || (Phase!=EHearthwardCompanionPhase::HoldingSafely && Phase!=EHearthwardCompanionPhase::WaitingAtCamp))return false;
    NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();BlockReason.Reset();Phase=EHearthwardCompanionPhase::ReturningBlocked;return true;
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
    using P=EHearthwardCompanionPhase;const auto& G=Command.Goal;
    if(!bSourceSafe) {ReturnBlocked(TEXT("UNSAFE"));return;}
    auto* Registry=WorkshopRegistry(GetWorld());AActor* Station=Registry?Registry->ResolveWorkbench(G.Station):nullptr;
    if(!IsValid(Station)) {ReturnBlocked(TEXT("STATION_UNAVAILABLE"));return;}
    const auto Cost=HearthwardWorkshop::Materials(G.Intent,G.Item,G.Quantity);
    if(!HearthwardAgent::AllowsCost(G.Limits,Cost,Spent)) {ReturnBlocked(TEXT("POLICY_CONFLICT"));return;}
    if(Phase==P::TakingMaterials)
    {
        if(!At(Camp)){if(!MoveTowards(Camp,0))ReturnBlocked(TEXT("PATH_BLOCKED"));return;}
        auto* Storage=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
        // Preflight every missing ingredient before touching any container. No reservation.
        int64 AddedWeight=0;
        for(const auto& C:Cost)
        {
            const int32 Missing=FMath::Max(0,C.Value-Bag->GetItemCount(C.Key));
            if(Storage->GetItemCount(C.Key)<Missing){ReturnBlocked(TEXT("INSUFFICIENT_MATERIAL"));return;}
            const auto* D=HearthwardBasicItems().FindByPredicate([&](const auto& I){return I.Id==C.Key;});AddedWeight+=int64(Missing)*D->WeightHundredths;
        }
        if(Bag->GetWeight()*100+AddedWeight>10000){ReturnBlocked(TEXT("CAPACITY_EXCEEDED"));return;}
        TGuardValue<bool> Guard(bSettling,true);
        for(const auto& C:Cost)
        {
            const int32 Missing=FMath::Max(0,C.Value-Bag->GetItemCount(C.Key));if(!Missing)continue;
            const auto T=Command.GetActive();const FGuid Op(T.Id.A,T.Id.B,T.Id.C^0xCA01^FCrc::StrCrc32(*C.Key.ToString()),T.Id.D);
            if(!HearthwardAgent::Settle(Receipts,Op,T.Id,T.Epoch,Storage->GetTimelineEpoch(),
                FString::Printf(TEXT("take:%s:%d:r%lld"),*C.Key.ToString(),Missing,T.Revision),[&]
                {
                    const auto R=Storage->Transfer(Bag,false,C.Key,Missing,Op,T.Epoch);
                    if(R.Result!=EHearthwardInventoryResult::Success)return false;
                    Event(TEXT("materials_taken"),C.Key,R.MovedCount,TEXT("authorized_camp"),Op);return true;
                })){ReturnBlocked(TEXT("INSUFFICIENT_MATERIAL"));return;}
        }
        StopNavigation();NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();Phase=P::GoingToWorkshop;return;
    }
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
            {Command.Carried=0;Command.RecordDelivery(Command.GetActive(),1);Phase=P::Completed;Event(TEXT("completed"),G.Item,1);}
            else {Phase=P::Returning;NavigationFailures=0;LastProgressAt=GetWorld()->GetTimeSeconds();LastProgressPosition=GetActorLocation();}
            return true;
        }))ReturnBlocked(TEXT("SETTLEMENT_FAILED"));
}
