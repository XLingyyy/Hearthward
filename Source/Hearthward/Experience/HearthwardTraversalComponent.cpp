#include "HearthwardTraversalComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PhysicsVolume.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"

UHearthwardTraversalComponent::UHearthwardTraversalComponent()
{ PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PrePhysics; }
void UHearthwardTraversalComponent::BeginPlay()
{
    Super::BeginPlay();
    auto* Character=CastChecked<ACharacter>(GetOwner());
    Character->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
    GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.AddDynamic(this,&UHearthwardTraversalComponent::Restored);
    FString Text;TSharedPtr<FJsonObject> Root;
    if(FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("Resources/Data/experience.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root))
        for(const auto& Value:Root->GetArrayField(TEXT("water")))
        {
            auto Row=Value->AsObject();const auto& XY=Row->GetArrayField(TEXT("center_m"));const auto& R=Row->GetArrayField(TEXT("radius_m"));
            WaterAreas.Add({FVector2D(XY[0]->AsNumber()*100,XY[1]->AsNumber()*100),FVector2D(R[0]->AsNumber()*100,R[1]->AsNumber()*100),float(Row->GetNumberField(TEXT("surface_m"))*100)});
        }
}
void UHearthwardTraversalComponent::EndPlay(const EEndPlayReason::Type Reason)
{ GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.RemoveDynamic(this,&UHearthwardTraversalComponent::Restored);Super::EndPlay(Reason); }
void UHearthwardTraversalComponent::Restored() { CancelVault();InWater=false;Status.Reset(); }
float UHearthwardTraversalComponent::FallDamage(float DownSpeed,float Gravity,float MaximumHealth)
{
    const float Height=FMath::Square(FMath::Max(0.f,DownSpeed)*.01f)/(2*FMath::Abs(Gravity)*.01f);
    if(Height<=3.f+KINDA_SMALL_NUMBER) return 0;
    if(Height>=12.f-KINDA_SMALL_NUMBER) return MaximumHealth;
    return MaximumHealth*((Height-3.f)/9.f);
}
bool UHearthwardTraversalComponent::WaterSurface(FVector Position,float& Height) const
{
    if(UGameplayStatics::GetCurrentLevelName(GetWorld(),true)==TEXT("L_HearthwardWilds"))
        for(const auto& Area:WaterAreas)
        {
            const FVector2D Offset=(FVector2D(Position.X,Position.Y)-Area.Center)/Area.Radius;
            if(Offset.SizeSquared()<=1) { Height=Area.Height;return true; }
        }
    const auto* Character=CastChecked<ACharacter>(GetOwner());
    const auto* Volume=Character->GetPhysicsVolume();
    if(Volume && Volume->bWaterVolume)
    { FVector Origin,Extent;Volume->GetActorBounds(false,Origin,Extent);Height=Origin.Z+Extent.Z;return true; }
    return false;
}
bool UHearthwardTraversalComponent::BreathingOnGround() const
{
    const auto* C=CastChecked<ACharacter>(GetOwner());float Surface=0;
    return C->GetCharacterMovement()->IsMovingOnGround() && (!WaterSurface(C->GetActorLocation(),Surface)
        || C->GetActorLocation().Z+C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()>Surface+10);
}
bool UHearthwardTraversalComponent::FindVault(FVector& Landing) const
{
    const auto* C=CastChecked<ACharacter>(GetOwner());const auto* Movement=C->GetCharacterMovement();
    if(!Movement->IsMovingOnGround() || Vaulting) return false;
    const float Half=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),Radius=C->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const FVector Position=C->GetActorLocation(),Feet=Position-FVector(0,0,Half);
    const FVector Forward=FRotator(0,C->GetControlRotation().Yaw,0).Vector();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HearthwardVault),false,C);FHitResult Wall,Top,FarTop;
    if(!GetWorld()->LineTraceSingleByChannel(Wall,Feet+FVector(0,0,46),Feet+FVector(0,0,46)+Forward*80,ECC_Visibility,Query)) return false;
    const FVector Near=Wall.ImpactPoint+Forward*5;
    if(!GetWorld()->LineTraceSingleByChannel(Top,FVector(Near.X,Near.Y,Feet.Z+121),FVector(Near.X,Near.Y,Feet.Z+45),ECC_Visibility,Query)) return false;
    const float Height=Top.ImpactPoint.Z-Feet.Z;
    if(Height<=45 || Height>120 || !Movement->IsWalkable(Top)) return false;
    const FVector Far=Top.ImpactPoint+Forward*60;
    if(!GetWorld()->LineTraceSingleByChannel(FarTop,Far+FVector(0,0,6),Far-FVector(0,0,6),ECC_Visibility,Query)
        || !Movement->IsWalkable(FarTop)) return false;
    Landing=FarTop.ImpactPoint+FVector(0,0,Half+2);
    const auto Capsule=FCollisionShape::MakeCapsule(Radius,Half);
    if(GetWorld()->OverlapBlockingTestByChannel(Landing,FQuat::Identity,ECC_Pawn,Capsule,Query)) return false;
    const FVector Up(Position.X,Position.Y,Landing.Z+4),Over(Landing.X,Landing.Y,Landing.Z+4);
    FHitResult Hit;
    return !GetWorld()->SweepSingleByChannel(Hit,Position,Up,FQuat::Identity,ECC_Pawn,Capsule,Query)
        && !GetWorld()->SweepSingleByChannel(Hit,Up,Over,FQuat::Identity,ECC_Pawn,Capsule,Query)
        && !GetWorld()->SweepSingleByChannel(Hit,Over,Landing,FQuat::Identity,ECC_Pawn,Capsule,Query);
}
bool UHearthwardTraversalComponent::BeginVault()
{
    auto* C=CastChecked<ACharacter>(GetOwner());auto* G=C->FindComponentByClass<UHearthwardGameplayComponent>();auto* S=C->FindComponentByClass<UHearthwardSurvivalComponent>();
    const auto* Combat=C->FindComponentByClass<UHearthwardCombatComponent>();
    if(!S->Enabled() || !S->Alive() || S->Busy() || GetWorld()->IsPaused()
        || UHearthwardSurvivalComponent::HasFailed(GetWorld()) || (Combat && Combat->Busy()) || !FindVault(End)) return false;
    if(G)
    {
        if(!G->SpendStamina(8)) {Status=TEXT("攀越需要8耐力");return false;}
    }
    else
    {
        const float Cost=8*C->FindComponentByClass<UHearthwardInventoryComponent>()->GetStaminaCostMultiplier();
        if(S->Stamina()<Cost) {Status=TEXT("攀越需要8耐力");return false;}
        S->Stamina()-=Cost;S->State.RecoveryDelay=.5;
    }
    Start=C->GetActorLocation();HighStart=FVector(Start.X,Start.Y,End.Z+4);HighEnd=FVector(End.X,End.Y,End.Z+4);
    StartingHealth=S->Health();VaultEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    Elapsed=0;Vaulting=true;Status=TEXT("攀越中");if(G)G->SetSprinting(false);C->GetCharacterMovement()->StopMovementImmediately();C->GetCharacterMovement()->SetMovementMode(MOVE_Flying);return true;
}
void UHearthwardTraversalComponent::CancelVault()
{
    if(!Vaulting) return;
    Vaulting=false;Status=TEXT("攀越已中止");CastChecked<ACharacter>(GetOwner())->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}
void UHearthwardTraversalComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Delta,Type,Tick);if(GetWorld()->IsPaused()) return;
    auto* C=CastChecked<ACharacter>(GetOwner());auto* M=C->GetCharacterMovement();auto* G=C->FindComponentByClass<UHearthwardGameplayComponent>();
    auto* S=C->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(Vaulting)
    {
        if(VaultEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()
            || !S->Enabled() || !S->Alive() || S->Health()<StartingHealth || S->Health()<=0) {CancelVault();return;}
        Elapsed=FMath::Min(1.f,Elapsed+Delta);
        const FVector Next=Elapsed<.3f?FMath::Lerp(Start,HighStart,Elapsed/.3f):Elapsed<.8f?FMath::Lerp(HighStart,HighEnd,(Elapsed-.3f)/.5f):FMath::Lerp(HighEnd,End,(Elapsed-.8f)/.2f);
        FHitResult Hit;M->SafeMoveUpdatedComponent(Next-C->GetActorLocation(),C->GetActorQuat(),true,Hit);
        if(Hit.IsValidBlockingHit()) {CancelVault();return;}
        if(Elapsed>=1) {Vaulting=false;M->SetMovementMode(MOVE_Falling);Status.Reset();}
        return;
    }
    float Surface=0;const bool Area=WaterSurface(C->GetActorLocation(),Surface);
    InWater=Area && C->GetActorLocation().Z-C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()<Surface-5;
    if(!S->Enabled() || !S->Alive() || S->Health()<=0) return;
    FFindFloorResult Floor;M->FindFloor(C->GetActorLocation(),Floor,false);
    const bool Standing=Floor.IsWalkableFloor() && Floor.FloorDist<=2.5f;
    if(InWater && !Standing) {if(G)G->SetSprinting(false);M->MaxSwimSpeed=300;M->SetMovementMode(MOVE_Swimming);}
    else if(M->IsSwimming()) M->SetMovementMode(Standing?MOVE_Walking:MOVE_Falling);
}
void UHearthwardMovementComponent::PhysicsVolumeChanged(APhysicsVolume* NewVolume)
{
    const auto* Traversal=CharacterOwner?CharacterOwner->FindComponentByClass<UHearthwardTraversalComponent>():nullptr;
    if(Traversal && Traversal->IsInWater()) return;
    Super::PhysicsVolumeChanged(NewVolume);
}
void UHearthwardMovementComponent::PhysSwimming(float DeltaTime,int32 Iterations)
{
    auto* Traversal=CharacterOwner->FindComponentByClass<UHearthwardTraversalComponent>();float Surface=0;
    if(!Traversal || !Traversal->WaterSurface(CharacterOwner->GetActorLocation(),Surface)) {Super::PhysSwimming(DeltaTime,Iterations);return;}
    auto* Survival=CharacterOwner->FindComponentByClass<UHearthwardSurvivalComponent>();
    const FVector Horizontal=FVector(Acceleration.X,Acceleration.Y,0).GetClampedToMaxSize(1)*MaxSwimSpeed;
    const float Vertical=Survival->Stamina()>0?FMath::Clamp((Surface-20-CharacterOwner->GetActorLocation().Z)*2,-60.f,60.f):-50.f;
    Velocity=FVector(Horizontal.X,Horizontal.Y,Vertical);FHitResult Hit;
    SafeMoveUpdatedComponent(Velocity*DeltaTime,UpdatedComponent->GetComponentQuat(),true,Hit);
    if(Hit.IsValidBlockingHit()) SlideAlongSurface(Velocity*DeltaTime,1-Hit.Time,Hit.Normal,Hit,true);
}
