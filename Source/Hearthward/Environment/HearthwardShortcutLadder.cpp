#include "HearthwardShortcutActor.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Interaction/HearthwardInteractionAlignment.h"
#include "../Animation/HearthwardBrotherAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "NavLinkCustomComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"

bool AHearthwardShortcutActor::ClimbingBy(const AActor* Player) const
{return Climber.Get()==Player;}

void AHearthwardShortcutActor::LinkReached(UNavLinkCustomComponent*,UObject* Following,const FVector&)
{
    auto* Path=Cast<UPathFollowingComponent>(Following);auto* Controller=Path?Cast<AController>(Path->GetOwner()):nullptr;
    if(!BeginClimb(Controller?Cast<ACharacter>(Controller->GetPawn()):nullptr,Path) && Path)
        Path->AbortMove(*this,FPathFollowingResultFlags::Blocked);
}
bool AHearthwardShortcutActor::BeginClimb(ACharacter* C,UPathFollowingComponent* Following)
{
    if(!C || Climber.IsValid() || !IsOpen() || GetWorld()->IsPaused())return false;
    if(auto* Target=C->FindComponentByClass<UHearthwardCombatTargetComponent>();Target && !Target->Protected)return false;
    auto* S=C->FindComponentByClass<UHearthwardSurvivalComponent>();const auto* Combat=C->FindComponentByClass<UHearthwardCombatComponent>();
    if((S && (!S->Alive() || S->Busy())) || (Combat && Combat->Busy()))return false;
    const float Half=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();const FVector Feet=C->GetActorLocation()-FVector(0,0,Half);
    const FVector A=LadderLink->GetStartPoint(),B=LadderLink->GetEndPoint();const bool AtA=FVector::DistSquared(Feet,A)<FVector::DistSquared(Feet,B);
    if(FVector::Dist(Feet,AtA?A:B)>145)return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Round2LadderPath),false,C);FHitResult Hit;
    const FVector Goal=AtA?B:A;
    if(!GetWorld()->LineTraceSingleByChannel(Hit,Goal+FVector(0,0,25),Goal-FVector(0,0,30),ECC_Visibility,Query)
        || !C->GetCharacterMovement()->IsWalkable(Hit))return false;
    ClimbStart=C->GetActorLocation();ClimbEnd=Hit.ImpactPoint+FVector(0,0,Half+2);
    const float Top=float(GetActorLocation().Z+260+Half);
    ClimbUp=FVector(ClimbStart.X,ClimbStart.Y,Top);ClimbOver=FVector(ClimbEnd.X,ClimbEnd.Y,Top);
    const auto Shape=FCollisionShape::MakeCapsule(C->GetCapsuleComponent()->GetScaledCapsuleRadius(),Half-2);
    if(GetWorld()->SweepSingleByChannel(Hit,ClimbStart,ClimbUp,FQuat::Identity,ECC_Pawn,Shape,Query)
        || GetWorld()->SweepSingleByChannel(Hit,ClimbUp,ClimbOver,FQuat::Identity,ECC_Pawn,Shape,Query)
        || GetWorld()->SweepSingleByChannel(Hit,ClimbOver,ClimbEnd,FQuat::Identity,ECC_Pawn,Shape,Query))return false;
    if(auto* Contact=C->FindComponentByClass<UHearthwardInteractionAlignment>())
    {if(!Contact->Begin(TEXT("Climb"),3.15f,this,C->GetActorLocation(),C->GetActorRotation()))return false;}
    else C->GetMesh()->PlayAnimation(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Brother/Animation/A_Brother_Climb")),true);
    Climber=C;ClimbFollowing=Following;ClimbElapsed=0;ClimbHealth=S?S->Health():0;
    Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    C->GetCharacterMovement()->StopMovementImmediately();C->GetCharacterMovement()->DisableMovement();return true;
}
void AHearthwardShortcutActor::TickClimb(float Delta)
{
    auto* C=Climber.Get();auto* S=C->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch() || (S && (!S->Alive() || S->Health()<ClimbHealth))){FinishClimb();return;}
    ClimbElapsed=FMath::Min(3.f,ClimbElapsed+Delta);const float T=ClimbElapsed/3;
    const FVector Next=T<.4f?FMath::Lerp(ClimbStart,ClimbUp,T/.4f):T<.65f?FMath::Lerp(ClimbUp,ClimbOver,(T-.4f)/.25f):FMath::Lerp(ClimbOver,ClimbEnd,(T-.65f)/.35f);
    FHitResult Hit;C->GetCharacterMovement()->SafeMoveUpdatedComponent(Next-C->GetActorLocation(),C->GetActorQuat(),true,Hit);
    if(Hit.IsValidBlockingHit() || T>=1)FinishClimb();
}
void AHearthwardShortcutActor::FinishClimb()
{
    if(auto* C=Climber.Get())
    {
        C->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        if(auto* Contact=C->FindComponentByClass<UHearthwardInteractionAlignment>())Contact->Cancel();
        else C->GetMesh()->SetAnimInstanceClass(UHearthwardBrotherAnimInstance::StaticClass());
    }
    if(auto* Path=ClimbFollowing.Get())Path->FinishUsingCustomLink(LadderLink);
    Climber.Reset();ClimbFollowing.Reset();
}
