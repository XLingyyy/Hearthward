#include "HearthwardCompanionNavigationComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"

#include "../Gameplay/HearthwardGameData.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"

UHearthwardCompanionNavigationComponent::UHearthwardCompanionNavigationComponent()
{
    PrimaryComponentTick.bCanEverTick=false;
}

ACharacter* UHearthwardCompanionNavigationComponent::CharacterOwner() const
{
    return Cast<ACharacter>(GetOwner());
}

bool UHearthwardCompanionNavigationComponent::IsAt(const AActor* Other,float AcceptanceRadius) const
{
    const auto* Character=CharacterOwner();
    return Character && IsValid(Other) && !Other->IsActorBeingDestroyed() && Other->GetWorld()==GetWorld()
        // MoveTo uses horizontal acceptance; stepping onto low props must not leave an arrived path unsettled.
        && FVector::DistSquared2D(Character->GetActorLocation(),Other->GetActorLocation())<=FMath::Square(double(AcceptanceRadius))
        && FMath::Abs(Character->GetActorLocation().Z-Other->GetActorLocation().Z)<=AcceptanceRadius;
}

void UHearthwardCompanionNavigationComponent::Stop()
{
    auto* Character=CharacterOwner();
    if(Character)
    {
        if(auto* AI=Cast<AAIController>(Character->GetController()))AI->StopMovement();
        if(auto* Movement=Character->GetCharacterMovement())Movement->StopMovementImmediately();
    }
    Target.Reset();
    bToLocation=false;
    Location=FVector::ZeroVector;
    Acceptance=0;
    RetryAt=0;
    Status.Reset();
}

bool UHearthwardCompanionNavigationComponent::IsRetryReady() const
{
    return GetWorld() && GetWorld()->GetTimeSeconds()>=RetryAt;
}

bool UHearthwardCompanionNavigationComponent::MoveToActor(AActor* Other,float Speed,float AcceptanceRadius)
{
    auto* Character=CharacterOwner();
    auto* AI=Character?Cast<AAIController>(Character->GetController()):nullptr;
    if(!Character || !AI || !IsValid(Other) || Other->IsActorBeingDestroyed())
    {
        Stop();
        Status=TEXT("通行目标已失效");
        return false;
    }

    const auto* Survival=Character->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(Survival && Survival->Enabled() && !Survival->Alive()) { Stop(); return false; }
    Character->GetCharacterMovement()->MaxWalkSpeed=Speed*(Survival && Survival->State.Severe()?.8f:1.f);
    if(bToLocation || Target!=Other || !FMath::IsNearlyEqual(Acceptance,AcceptanceRadius))
    {
        Stop();
        Target=Other;
        Acceptance=AcceptanceRadius;
    }

    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if(AI->GetMoveStatus()!=EPathFollowingStatus::Idle)
    {
        const auto Path=AI->GetPathFollowingComponent()->GetPath();
        Status=Path.IsValid() && Path->IsPartial()?TEXT("沿附近路线前进，等待后续导航"):FString();
        return true;
    }
    FNavLocation Local;
    if(Nav && Nav->IsNavigationBuildInProgress() && !Nav->ProjectPointToNavigation(Character->GetActorLocation(),Local,FVector(80,80,200)))
    {Status=TEXT("正在准备脚下导航，请稍候");return true;}
    if(!IsRetryReady())return false;

    EPathFollowingRequestResult::Type Result;
    if(Other->ActorHasTag(TEXT("Hearthward.Building.Completed")))
    {
        FVector Direction=(Character->GetActorLocation()-Other->GetActorLocation()).GetSafeNormal2D();
        if(Direction.IsNearlyZero())Direction=Other->GetActorForwardVector();
        const double Reach=HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("crafting")),TEXT("reach"));
        const FVector Approach=Other->GetActorLocation()+Direction*(Reach-60);
        FNavLocation Projected;
        const bool ProjectedOK=Nav && Nav->ProjectPointToNavigation(Approach,Projected,FVector(50,50,200));
        Result=ProjectedOK?AI->MoveToLocation(Projected.Location,30,false,true,false,false,nullptr,false):EPathFollowingRequestResult::Failed;
        UE_LOG(LogTemp,Display,TEXT("NPC workshop approach: actor=%s station=%s desired=%s projected=%s valid=%d move=%d"),
            *Character->GetActorLocation().ToString(),*Other->GetActorLocation().ToString(),*Approach.ToString(),
            *Projected.Location.ToString(),ProjectedOK,int32(Result));
    }
    else
    {
        Result=AI->MoveToActor(Other,AcceptanceRadius,false,true,false,nullptr,true);
    }

    if(Result==EPathFollowingRequestResult::Failed)
    {
        RetryAt=GetWorld()->GetTimeSeconds()+0.5;
        Status=TEXT("附近没有可通行路线，正在重试");
        return false;
    }
    const auto Path=AI->GetPathFollowingComponent()->GetPath();
    Status=Path.IsValid() && Path->IsPartial()?TEXT("沿附近路线前进，等待后续导航"):FString();
    return true;
}

bool UHearthwardCompanionNavigationComponent::MoveToLocation(const FVector& Destination,float Speed,float AcceptanceRadius)
{
    auto* Character=CharacterOwner();
    auto* AI=Character?Cast<AAIController>(Character->GetController()):nullptr;
    if(!Character || !AI || Destination.ContainsNaN())
    {
        Stop();
        Status=TEXT("通行落点已失效");
        return false;
    }

    const auto* Survival=Character->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(Survival && Survival->Enabled() && !Survival->Alive()) { Stop(); return false; }
    Character->GetCharacterMovement()->MaxWalkSpeed=Speed*(Survival && Survival->State.Severe()?.8f:1.f);
    if(!bToLocation || !Location.Equals(Destination,5.0f) || !FMath::IsNearlyEqual(Acceptance,AcceptanceRadius))
    {
        Stop();
        bToLocation=true;
        Location=Destination;
        Acceptance=AcceptanceRadius;
    }

    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if(AI->GetMoveStatus()!=EPathFollowingStatus::Idle)
    {
        const auto Path=AI->GetPathFollowingComponent()->GetPath();
        Status=Path.IsValid() && Path->IsPartial()?TEXT("沿附近路线前进，等待后续导航"):FString();
        return true;
    }
    FNavLocation Local;
    if(Nav && Nav->IsNavigationBuildInProgress() && !Nav->ProjectPointToNavigation(Character->GetActorLocation(),Local,FVector(80,80,200)))
    {Status=TEXT("正在准备脚下导航，请稍候");return true;}
    if(!IsRetryReady())return false;

    FNavLocation Projected;
    if(!Nav || !Nav->ProjectPointToNavigation(Destination,Projected,FVector(80,80,200)))
    {
        RetryAt=GetWorld()->GetTimeSeconds()+0.5;
        Status=TEXT("落点导航尚不可用，请稍候重试");
        return false;
    }

    const auto Result=AI->MoveToLocation(Projected.Location,AcceptanceRadius,false,true,false,false,nullptr,true);
    if(Result==EPathFollowingRequestResult::Failed)
    {
        RetryAt=GetWorld()->GetTimeSeconds()+0.5;
        Status=TEXT("附近没有可通行路线，正在重试");
        return false;
    }
    const auto Path=AI->GetPathFollowingComponent()->GetPath();
    Status=Path.IsValid() && Path->IsPartial()?TEXT("沿附近路线前进，等待后续导航"):FString();
    return true;
}
