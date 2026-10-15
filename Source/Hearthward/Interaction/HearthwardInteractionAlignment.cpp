#include "HearthwardInteractionAlignment.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../HearthwardCharacter.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

UHearthwardInteractionAlignment::UHearthwardInteractionAlignment(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PrePhysics;}
void UHearthwardInteractionAlignment::BeginPlay()
{Super::BeginPlay();GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.AddDynamic(this,&UHearthwardInteractionAlignment::Restored);}
void UHearthwardInteractionAlignment::EndPlay(const EEndPlayReason::Type Reason)
{Cancel();GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.RemoveDynamic(this,&UHearthwardInteractionAlignment::Restored);Super::EndPlay(Reason);}
void UHearthwardInteractionAlignment::Restored(){Cancel();}
bool UHearthwardInteractionAlignment::CanAlign(ACharacter* C,FVector Point,FRotator Facing)
{
    if(!C || !C->GetCharacterMovement()->IsMovingOnGround() || FVector::Dist2D(C->GetActorLocation(),Point)>60
        || FMath::Abs(C->GetActorLocation().Z-Point.Z)>18 || FMath::Abs(FMath::FindDeltaAngleDegrees(C->GetActorRotation().Yaw,Facing.Yaw))>100)return false;
    const auto* Capsule=C->GetCapsuleComponent();FCollisionQueryParams Query(SCENE_QUERY_STAT(ContactAlign),false,C);FHitResult Hit;
    const auto Shape=FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()-2);
    if(C->GetWorld()->SweepSingleByChannel(Hit,C->GetActorLocation(),Point,FQuat::Identity,ECC_Pawn,Shape,Query))return false;
    const FVector Feet=Point-FVector(0,0,Capsule->GetScaledCapsuleHalfHeight());
    return C->GetWorld()->LineTraceSingleByChannel(Hit,Feet+FVector(0,0,12),Feet-FVector(0,0,20),ECC_Visibility,Query)
        && C->GetCharacterMovement()->IsWalkable(Hit) && Hit.ImpactNormal.Z>.85;
}
bool UHearthwardInteractionAlignment::Begin(FName Name,float Seconds,AActor* InTarget,FVector Point,FRotator Facing,bool Hold)
{
    auto* C=Cast<ACharacter>(GetOwner());auto* S=GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(Active() || Seconds<=0 || !IsValid(InTarget) || !S || !S->Alive() || !CanAlign(C,Point,Facing))return false;
    const FString Role=Cast<AHearthwardCharacter>(C)?TEXT("Hero"):TEXT("Brother");
    const FString Path=Name==TEXT("Climb")?FString::Printf(TEXT("/Game/Characters/%s/%s/A_%s_Climb"),*Role,Role==TEXT("Hero")?TEXT("AnimationV2"):TEXT("Animation"),*Role)
        :FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-126/A_%s_%s"),*Role,*Name.ToString());
    Clip=LoadObject<UAnimSequence>(nullptr,*Path);if(!Clip)return false;
    FHitResult Hit;C->GetCharacterMovement()->SafeMoveUpdatedComponent(Point-C->GetActorLocation(),Facing.Quaternion(),true,Hit);
    if(Hit.IsValidBlockingHit()){Clip=nullptr;return false;}
    C->GetCharacterMovement()->StopMovementImmediately();Action=Name;Target=InTarget;Origin=C->GetActorLocation();Health=S->Health();
    Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();Elapsed=0;Duration=Seconds;Holding=Hold;
    if(auto* Hero=Cast<AHearthwardCharacter>(C))Hero->RefreshHeldTool();
    if(auto* Brother=Cast<AHearthwardCompanionFixture>(C))Brother->RefreshHeldWeapon();return true;
}
float UHearthwardInteractionAlignment::PoseTime() const
{return Eating?FoodTime:Clip?FMath::Min(Holding?.65f:1.f,Elapsed/Duration)*Clip->GetPlayLength():0;}
bool UHearthwardInteractionAlignment::FollowMovingTarget(AActor* InTarget,FVector WorldDelta)
{
    auto* C=Cast<ACharacter>(GetOwner());
    if(Action!=TEXT("MoveBeam") || Target.Get()!=InTarget || !C->GetCharacterMovement()->IsMovingOnGround())return false;
    if(WorldDelta.IsNearlyZero())return true;
    const auto* Capsule=C->GetCapsuleComponent();const float Half=Capsule->GetScaledCapsuleHalfHeight();
    FVector Point=C->GetActorLocation()+WorldDelta;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(MovingContactGround),false,C);Query.AddIgnoredActor(InTarget);FHitResult Floor;
    if(!GetWorld()->LineTraceSingleByChannel(Floor,Point-FVector(0,0,Half-60),Point-FVector(0,0,Half+120),ECC_Visibility,Query)
        || !C->GetCharacterMovement()->IsWalkable(Floor) || Floor.ImpactNormal.Z<=.85)return false;
    Point.Z=Floor.ImpactPoint.Z+Half+Capsule->GetScaledCapsuleRadius()*(1.f/Floor.ImpactNormal.Z-1.f)+UCharacterMovementComponent::MAX_FLOOR_DIST;
    FHitResult Hit;C->GetCharacterMovement()->SafeMoveUpdatedComponent(Point-C->GetActorLocation(),C->GetActorQuat(),true,Hit);
    if(Hit.IsValidBlockingHit())return false;
    C->GetCharacterMovement()->StopMovementImmediately();Origin=C->GetActorLocation();return true;
}
void UHearthwardInteractionAlignment::Cancel()
{
    const bool WasActive=Active();Action=NAME_None;Target.Reset();Clip=nullptr;Elapsed=Duration=0;Holding=false;Eating=false;
    if(WasActive){if(auto* Hero=Cast<AHearthwardCharacter>(GetOwner()))Hero->RefreshHeldTool();if(auto* Brother=Cast<AHearthwardCompanionFixture>(GetOwner()))Brother->RefreshHeldWeapon();}
}
void UHearthwardInteractionAlignment::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Delta,Type,Tick);if(GetWorld()->IsPaused())return;
    auto* S=GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>();
    const bool WasEating=Eating;Eating=S && S->Alive() && !S->State.FoodItem.IsNone();
    if(Eating && !FoodClip)
    {
        const FString Role=Cast<AHearthwardCharacter>(GetOwner())?TEXT("Hero"):TEXT("Brother");
        FoodClip=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-126/A_%s_Eat"),*Role));
    }
    Eating&=FoodClip!=nullptr;if(Eating)FoodTime=float(FMath::Clamp(3-S->State.FoodRemaining,0.,3.));
    if(WasEating!=Eating){if(auto* Hero=Cast<AHearthwardCharacter>(GetOwner()))Hero->RefreshHeldTool();if(auto* Brother=Cast<AHearthwardCompanionFixture>(GetOwner()))Brother->RefreshHeldWeapon();}
    if(Action.IsNone())return;
#if !UE_BUILD_SHIPPING
    CSV_CUSTOM_STAT_GLOBAL(HearthwardContactTargets,1,ECsvCustomStatOp::Accumulate);
#endif
    const auto* Combat=GetOwner()->FindComponentByClass<UHearthwardCombatComponent>();
    if(!Target.IsValid() || !S->Alive() || S->Health()<Health || (Combat && Combat->Busy())
        || Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()
        || (Action!=TEXT("Climb") && (FVector::Dist2D(Origin,GetOwner()->GetActorLocation())>12 || GetOwner()->GetVelocity().Size2D()>5))){Cancel();return;}
    Elapsed+=Delta;if(!Holding && Elapsed>=Duration)Cancel();
}
