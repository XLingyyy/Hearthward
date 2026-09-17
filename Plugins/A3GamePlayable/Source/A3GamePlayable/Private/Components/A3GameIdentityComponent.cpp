#include "Components/A3GameIdentityComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UA3GameIdentityComponent::UA3GameIdentityComponent()
{
    SetIsReplicatedByDefault(true);
}

void UA3GameIdentityComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UA3GameIdentityComponent, ParticipantId);
    DOREPLIFETIME(UA3GameIdentityComponent, EntityId);
}

void UA3GameIdentityComponent::SetRuntimeIdentity(
    const FString& InParticipantId,
    const FString& InEntityId)
{
    const AActor* Owner = GetOwner();
    if (Owner && !Owner->HasAuthority())
    {
        return;
    }

    ParticipantId = InParticipantId;
    EntityId = InEntityId;
}
