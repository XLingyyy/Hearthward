#include "HearthwardTimedActionComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UHearthwardTimedActionComponent::UHearthwardTimedActionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UHearthwardTimedActionComponent::BeginPlay()
{
    Super::BeginPlay();
    Clock = GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>();
    GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UHearthwardTimedActionComponent::OnOwnerDamaged);
}

void UHearthwardTimedActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetOwner()->OnTakeAnyDamage.RemoveDynamic(this, &UHearthwardTimedActionComponent::OnOwnerDamaged);
    Super::EndPlay(EndPlayReason);
}

bool UHearthwardTimedActionComponent::StartAction()
{
    if(auto* S=GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>();S && S->Enabled() && (!S->Alive() || S->Busy())) return false;
    if (!Clock || GetWorld()->IsPaused() || !State.Start(Clock->GetSnapshot().ActivePlaySeconds)) return false;
    SetComponentTickEnabled(true);
    return true;
}

void UHearthwardTimedActionComponent::InterruptAction()
{
    if (Clock && State.Interrupt(Clock->GetSnapshot().ActivePlaySeconds))
    {
        SetComponentTickEnabled(false);
        OnInterrupted.Broadcast();
    }
}

void UHearthwardTimedActionComponent::OnOwnerDamaged(AActor* DamagedActor, float Damage,
    const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    if (Damage > 0.0f) InterruptAction();
}

void UHearthwardTimedActionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if(auto* S=GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>();S && S->Enabled())
    {
        if(!S->Alive() || S->Busy()) { InterruptAction(); return; }
        if(Cast<AHearthwardCompanionFixture>(GetOwner()) && S->State.Severe()) State.StartedAt+=DeltaTime*.3;
    }
    if (State.Update(Clock->GetSnapshot().ActivePlaySeconds))
    {
        SetComponentTickEnabled(false);
        OnTimerCompleted.Broadcast();
    }
}
