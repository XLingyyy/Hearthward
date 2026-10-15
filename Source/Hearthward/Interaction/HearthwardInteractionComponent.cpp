#include "HearthwardInteractionComponent.h"
#include "HearthwardInteractionAlignment.h"
#include "../Environment/HearthwardNoiseInteraction.h"
#include "../Environment/HearthwardLampInteraction.h"
#include "../Environment/HearthwardShortcutActor.h"
#include "../Campaign/HearthwardPersonalStory.h"
#include "../Nature/HearthwardAnimalTraces.h"
#include "../Nature/HearthwardNatureActor.h"
#include "../Inventory/HearthwardDroppedEquipment.h"
#include "HearthwardHarvestSubsystem.h"
#include "HearthwardInteractionTargetComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

UHearthwardInteractionComponent::UHearthwardInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UHearthwardInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    Action = GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>();
    if (Action)
    {
        Action->OnTimerCompleted.AddDynamic(this, &UHearthwardInteractionComponent::TimerCompleted);
        Action->OnInterrupted.AddDynamic(this, &UHearthwardInteractionComponent::TimerInterrupted);
        // Invalidate a lost/out-of-range target before the timer can complete on the same frame.
        Action->AddTickPrerequisiteComponent(this);
    }
}

void UHearthwardInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bActive) Cancel(EHearthwardInteractionStatus::Interrupted);
    if (Action)
    {
        Action->OnTimerCompleted.RemoveDynamic(this, &UHearthwardInteractionComponent::TimerCompleted);
        Action->OnInterrupted.RemoveDynamic(this, &UHearthwardInteractionComponent::TimerInterrupted);
        Action->RemoveTickPrerequisiteComponent(this);
    }
    Super::EndPlay(EndPlayReason);
}

EHearthwardInteractionStatus UHearthwardInteractionComponent::ValidateTarget(UHearthwardInteractionTargetComponent* Target) const
{
    if (!IsValid(Target) || !IsValid(Target->GetOwner()) || Target->GetOwner()->IsActorBeingDestroyed()
        || Target->GetOwner()->IsHidden() || Target->GetWorld() != GetWorld() || Target->GetOwner() == GetOwner()) return EHearthwardInteractionStatus::InvalidTarget;
    if (!FMath::IsFinite(Target->MaxDistance) || Target->MaxDistance <= 0.0f) return EHearthwardInteractionStatus::Unconfigured;
    return FVector::DistSquared(GetOwner()->GetActorLocation(), Target->GetComponentLocation()) <= FMath::Square(double(Target->MaxDistance))
        ? EHearthwardInteractionStatus::Ready : EHearthwardInteractionStatus::OutOfRange;
}

bool UHearthwardInteractionComponent::BeginInteraction(UHearthwardInteractionTargetComponent* Target)
{
    if (bActive) return false;
    if (GetWorld()->IsPaused()) { SetStatus(EHearthwardInteractionStatus::Paused); return false; }
    const auto Check = ValidateTarget(Target);
    if (Check != EHearthwardInteractionStatus::Ready) { SetStatus(Check); return false; }
    if (!GetOwner()->GetVelocity().IsNearlyZero()) { SetStatus(EHearthwardInteractionStatus::Interrupted); return false; }
    if (!Action || !Action->StartAction()) { SetStatus(EHearthwardInteractionStatus::Busy); return false; }
    PendingTarget = Target;
    bActive = true;
    if(auto* Harvest=Cast<UHearthwardHarvestTargetComponent>(Target);Harvest && Harvest->Item==TEXT("herb"))
        if(auto* Contact=GetOwner()->FindComponentByClass<UHearthwardInteractionAlignment>())
        {
            const FVector Direction=(Target->GetComponentLocation()-GetOwner()->GetActorLocation()).GetSafeNormal2D();
            FVector Point=Target->GetComponentLocation()-Direction*65;Point.Z=GetOwner()->GetActorLocation().Z;
            Contact->Begin(TEXT("Gather"),FHearthwardTimedActionState::DurationSeconds,Target->GetOwner(),Point,Direction.Rotation());
        }
    SetStatus(EHearthwardInteractionStatus::Running);
    SetComponentTickEnabled(true);
    return true;
}

bool UHearthwardInteractionComponent::InteractNearest()
{
    auto* Target=GetNearestTarget();
    if(Cast<UHearthwardEquipmentPickup>(Target) || Cast<UHearthwardNatureInteraction>(Target) || Cast<UHearthwardNoiseInteraction>(Target) || Cast<UHearthwardDeviceInteraction>(Target) || Cast<UHearthwardPersonalInteraction>(Target) || Cast<UHearthwardTrailInteraction>(Target)){CompletionFeedback=Target->CompleteInteraction(GetOwner());SetStatus(EHearthwardInteractionStatus::Ready);return true;}
    return BeginInteraction(Target);
}

UHearthwardInteractionTargetComponent* UHearthwardInteractionComponent::GetNearestTarget() const
{
    GetWorld()->GetSubsystem<UHearthwardHarvestSubsystem>()->RefreshNearby(GetOwner());
    UHearthwardInteractionTargetComponent* Nearest = nullptr;
    double Distance = TNumericLimits<double>::Max();
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        TInlineComponentArray<UHearthwardInteractionTargetComponent*> Targets;
        It->GetComponents(Targets);
        for (auto* Target : Targets)
        {
            if (ValidateTarget(Target) != EHearthwardInteractionStatus::Ready) continue;
            const double CandidateDistance = FVector::DistSquared(GetOwner()->GetActorLocation(), Target->GetComponentLocation());
            if (CandidateDistance < Distance ||
                (CandidateDistance == Distance && Nearest && Target->GetPathName() < Nearest->GetPathName()))
            { Nearest = Target; Distance = CandidateDistance; }
        }
    }
    return Nearest;
}
UHearthwardInteractionTargetComponent* UHearthwardInteractionComponent::GetPriorityTarget() const
{
    auto* Target=GetNearestTarget();
    return Cast<UHearthwardNoiseInteraction>(Target) || Cast<UHearthwardLampInteraction>(Target) || Cast<UHearthwardDeviceInteraction>(Target)
        || Cast<UHearthwardPersonalInteraction>(Target) || Cast<UHearthwardTrailInteraction>(Target)?Target:nullptr;
}

void UHearthwardInteractionComponent::Cancel(EHearthwardInteractionStatus Reason)
{
    bActive = false;
    PendingTarget.Reset();
    SetComponentTickEnabled(false);
    SetStatus(Reason);
    if (Action) Action->InterruptAction();
}

void UHearthwardInteractionComponent::TimerInterrupted()
{
    if (!bActive) return;
    if(auto* Contact=GetOwner()->FindComponentByClass<UHearthwardInteractionAlignment>())Contact->Cancel();
    bActive = false;
    PendingTarget.Reset();
    SetComponentTickEnabled(false);
    SetStatus(EHearthwardInteractionStatus::Interrupted);
}

void UHearthwardInteractionComponent::TimerCompleted()
{
    if (!bActive) return;
    auto* Target = PendingTarget.Get();
    const auto Check = ValidateTarget(Target);
    bActive = false;
    PendingTarget.Reset();
    SetComponentTickEnabled(false);
    SetStatus(Check);
    if (Check == EHearthwardInteractionStatus::Ready) CompletionFeedback = Target->CompleteInteraction(GetOwner());
}

void UHearthwardInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bActive) return;
    const auto Check = ValidateTarget(PendingTarget.Get());
    if (Check != EHearthwardInteractionStatus::Ready) Cancel(Check);
    else if (!GetOwner()->GetVelocity().IsNearlyZero()) Cancel(EHearthwardInteractionStatus::Interrupted);
}
