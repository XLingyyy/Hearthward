#include "HearthwardCompanionFixture.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AHearthwardCompanionFixture::AHearthwardCompanionFixture()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    auto* Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
    Capsule->InitCapsuleSize(30.0f, 80.0f);
    Capsule->SetCollisionProfileName(TEXT("Pawn"));
    SetRootComponent(Capsule);
    auto* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    Mesh->SetupAttachment(Capsule);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Mesh->SetStaticMesh(Shape.Object);
    Mesh->SetRelativeScale3D(FVector(0.6, 0.6, 1.6));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
        Action->InterruptAction();
        BlockReason.Reset();
        Phase = Bag->GetWeight() > 0 ? EHearthwardCompanionPhase::Returning : EHearthwardCompanionPhase::GoingToSource;
    }
    return Result;
}

bool AHearthwardCompanionFixture::Cancel(AActor* Speaker)
{
    if (bSettling || !CanCommunicate(Speaker) || GetWorld()->IsPaused()) return false;
    Command.Cancel();
    Action->InterruptAction();
    Phase = EHearthwardCompanionPhase::Cancelled;
    return true;
}

bool AHearthwardCompanionFixture::IsSourceValid() const
{
    return IsValid(Source) && IsValid(Source->GetOwner()) && !Source->GetOwner()->IsActorBeingDestroyed()
        && Source->GetWorld() == GetWorld();
}

bool AHearthwardCompanionFixture::At(const AActor* Target) const
{
    return IsValid(Target) && !Target->IsActorBeingDestroyed() && Target->GetWorld() == GetWorld()
        && FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(50.0);
}

bool AHearthwardCompanionFixture::MoveTowards(const AActor* Target, float DeltaSeconds)
{
    if (!IsValid(Target) || Target->IsActorBeingDestroyed()) return false;
    const FVector Delta = Target->GetActorLocation() - GetActorLocation();
    FHitResult Hit;
    // PROTOTYPE_ONLY flat test lane. Sweep the capsule; never teleport through a blocked route.
    SetActorLocation(GetActorLocation() + Delta.GetClampedToMaxSize(180.0 * DeltaSeconds), true, &Hit);
    return !Hit.bBlockingHit;
}

void AHearthwardCompanionFixture::ReturnBlocked(const FString& Reason)
{
    Action->InterruptAction();
    Phase = EHearthwardCompanionPhase::ReturningBlocked;
    BlockReason = Reason;
}

void AHearthwardCompanionFixture::Deposit()
{
    if (!At(Camp)) return;
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
        if (Item.Id == Command.GetItem()) Count = FMath::Min(Count, Command.GetRequested() - Command.GetDelivered());
        if (Count == 0) continue;
        const auto Result = Storage->Transfer(Bag, true, Item.Id, Count, FGuid::NewGuid(), Ticket.Epoch);
        if (Storage->GetTimelineEpoch() != Ticket.Epoch)
        {
            Command.Cancel();
            Phase = EHearthwardCompanionPhase::Cancelled;
            return;
        }
        if (Result.Result != EHearthwardInventoryResult::Success)
        {
            ReturnBlocked(TEXT("入库失败，保留携带物资"));
            return;
        }
        if (Item.Id == Command.GetItem()) Command.RecordDelivery(Ticket, Result.MovedCount);
        if (Command.GetDelivered() == Command.GetRequested())
        {
            Phase = EHearthwardCompanionPhase::Completed;
            return;
        }
    }
    Phase = bWasBlocked ? EHearthwardCompanionPhase::WaitingAtCamp : EHearthwardCompanionPhase::GoingToSource;
}

void AHearthwardCompanionFixture::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    using P = EHearthwardCompanionPhase;
    if (!bFixtureEnabled || GetWorld()->IsPaused() || bSettling) return;
    if (Phase == P::Idle || Phase == P::Cancelled || Phase == P::Completed || Phase == P::WaitingAtCamp) return;
    auto* Storage = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if (!Command.IsCurrent(Storage->GetTimelineEpoch()))
    {
        Command.Cancel();
        Action->InterruptAction();
        Phase = P::Cancelled;
        return;
    }
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
        const int32 Free = FMath::Max(0, 400 - FMath::RoundToInt(Bag->GetWeight() * 100));
        const int32 Count = FMath::Min3(Free / Item->WeightHundredths,
            Source->GetItemCount(Item->Id), Command.GetRequested() - Command.GetDelivered());
        if (Count <= 0) { ReturnBlocked(TEXT("本趟无法携带目标物品")); return; }
        Phase = P::Returning;
        TGuardValue<bool> Guard(bSettling, true);
        if (Source->TransferTo(Bag, Item->Id, Count) != EHearthwardInventoryResult::Success)
            ReturnBlocked(TEXT("采集结算时资源或容量不足"));
    }
    else if (Phase == P::Returning || Phase == P::ReturningBlocked)
    {
        if (At(Camp)) Deposit();
        else if (!MoveTowards(Camp, DeltaSeconds)) ReturnBlocked(TEXT("返营路线受阻，保留物资等待通路"));
    }
}
