#include "HearthwardProjectile.h"
#include "HearthwardCombatComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
AHearthwardProjectile::AHearthwardProjectile()
{
    PrimaryActorTick.bCanEverTick=true;
    auto* Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Projectile")); SetRootComponent(Mesh);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Mesh->SetRelativeScale3D(FVector(.06)); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AHearthwardProjectile::Tick(float Delta)
{
    Super::Tick(Delta);
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()) { Destroy(); return; }
    if(GetActorLocation().Z<GetWorld()->GetWorldSettings()->KillZ) { Destroy(); return; }
    if(Landed || GetWorld()->IsPaused() || !Shooter.IsValid()) return;
    const int32 Steps=FMath::Max(1,FMath::CeilToInt(Delta/.016)); const double Dt=Delta/Steps;
    for(int32 I=0;I<Steps && !Landed;++I)
    {
        const FVector Move=Velocity*Dt+FVector(0,0,-.5*Gravity*Dt*Dt); Velocity.Z-=Gravity*Dt;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(CombatProjectile),false,this); Q.AddIgnoredActor(Shooter->GetOwner()); FHitResult Hit;
        if(GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation(),GetActorLocation()+Move,ECC_Visibility,Q))
        {
            SetActorLocation(Hit.ImpactPoint); Landed=true;
            auto* T=Hit.GetActor()?Hit.GetActor()->FindComponentByClass<UHearthwardCombatTargetComponent>():nullptr;
            HitTarget=Hit.GetActor() && (T || Hit.GetActor()->IsA<APawn>());
            if(T && !Bait) Shooter->HitTarget(T,Power,T->HitPart(Hit),Item==TEXT("arrow"),Event);
        }
        else { SetActorLocation(GetActorLocation()+Move); RemainingRange-=Move.Size(); if(RemainingRange<=0) { Velocity.X=Velocity.Y=0; Power=0; } }
        if(Landed && Bait)
        {
            UHearthwardCombatTargetComponent* Nearest=nullptr; double Best=FMath::Square(1500.);
            for(auto* T:Shooter->Targets()) if(T->CanAct() && T->Memory.Seen.IsEmpty())
            {
                const double D=FVector::DistSquared(GetActorLocation(),T->GetOwner()->GetActorLocation());
                const auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),T->GetOwner()->GetActorLocation(),GetActorLocation(),T->GetOwner());
                if(D<Best && Path && Path->IsValid() && !Path->IsPartial()) { Best=D; Nearest=T; }
            }
            if(Nearest) { Nearest->Memory.Investigation=GetActorLocation(); Nearest->Memory.InvestigationRemaining=8; }
        }
    }
}
bool AHearthwardProjectile::Recover(AActor* Player)
{
    if(!Landed || HitTarget || Item.IsNone() || FVector::Dist(GetActorLocation(),Player->GetActorLocation())>150) return false;
    auto* Bag=Player->FindComponentByClass<UHearthwardInventoryComponent>();
    if(!Bag || Bag->TryAdd(Item,1)!=EHearthwardInventoryResult::Success) return false;
    Item=NAME_None; Destroy(); return true;
}
