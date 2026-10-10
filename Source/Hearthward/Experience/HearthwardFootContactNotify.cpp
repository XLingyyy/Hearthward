#include "HearthwardFootContactNotify.h"
#include "HearthwardPresentationComponent.h"
#include "HearthwardFootstepSurface.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

bool UHearthwardFootContactNotify::ReadGroundContact(USkeletalMeshComponent* MeshComp,FHearthwardFootContactReceipt& Receipt) const
{
    auto* Character=MeshComp?Cast<ACharacter>(MeshComp->GetOwner()):nullptr;
    auto* World=Character?Character->GetWorld():nullptr;
    if(!World || !World->IsGameWorld() || World->IsPaused() || World->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring())return false;
    const auto* Survival=Character->FindComponentByClass<UHearthwardSurvivalComponent>();
    const auto* Movement=Character->GetCharacterMovement();
    if(!Survival || !Survival->Enabled() || !Survival->Alive() || !Movement->IsMovingOnGround() || !Movement->CurrentFloor.IsWalkableFloor()
        || Character->GetVelocity().SizeSquared2D()<=KINDA_SMALL_NUMBER || !MeshComp->DoesSocketExist(FootBone))return false;
    const FVector Foot=MeshComp->GetSocketLocation(FootBone);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HearthwardFootContact),false,Character);
    Query.bReturnPhysicalMaterial=true;Query.bReturnFaceIndex=true;
    FHitResult Hit;
    // Actual sampled ankle contacts are 10.52--14.97 cm above the capsule bottom.
    // Search the nearby floor; timing still comes exclusively from the asset Notify.
    if(!World->LineTraceSingleByChannel(Hit,Foot+FVector(0,0,2),Foot-FVector(0,0,20),ECC_Visibility,Query)
        || !Hit.bBlockingHit || Hit.bStartPenetrating || !Movement->IsWalkable(Hit) || Cast<ACharacter>(Hit.GetActor()))return false;
    Receipt.SuccessId=FGuid::NewGuid();Receipt.Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    Receipt.Source=Character;Receipt.FootBone=FootBone;Receipt.Position=Hit.ImpactPoint;Receipt.Material=Hit.PhysMaterial;
    Receipt.Surface=HearthwardFootstepSurface::Resolve(Hit);
    return true;
}

void UHearthwardFootContactNotify::Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase*,const FAnimNotifyEventReference&)
{
    FHearthwardFootContactReceipt Receipt;
    if(!ReadGroundContact(MeshComp,Receipt))return;
    Receipt.Frame=GFrameCounter;
    auto* Player=UGameplayStatics::GetPlayerPawn(MeshComp->GetWorld(),0);
    if(auto* Presentation=Player?Player->FindComponentByClass<UHearthwardPresentationComponent>():nullptr)
        Presentation->FootContactSucceeded(Receipt);
}
