#include "HearthwardCampaignActor.h"
#include "HearthwardCampaignSubsystem.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardProjectile.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Animation/HearthwardBrotherAnimInstance.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
using namespace HearthwardData;
AHearthwardCampaignActor::AHearthwardCampaignActor()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.1f;
    GetCapsuleComponent()->InitCapsuleSize(30,80);
    GetCharacterMovement()->MaxWalkSpeed=240;GetCharacterMovement()->bOrientRotationToMovement=true;
    bUseControllerRotationYaw=false;AIControllerClass=AAIController::StaticClass();AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Target=CreateDefaultSubobject<UHearthwardCombatTargetComponent>(TEXT("CampaignTarget"));
    Target->CampaignTarget=true;
}
void AHearthwardCampaignActor::Initialize(FName Id,bool Hostile)
{
    Identity=Id;Enemy=Hostile;Target->Id=Id;
    auto* C=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
    auto* E=C->Enemy(Id);
    Target->Protected=!Hostile;
    // Civilian instances share the existing rig and locomotion assets.
    GetMesh()->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Brother/UE5/SK_Brother.SK_Brother")));
    GetMesh()->SetRelativeScale3D(FVector(160.f/97.863766f));GetMesh()->SetRelativeLocation(FVector(0,0,-80));GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
    GetMesh()->SetAnimInstanceClass(UHearthwardBrotherAnimInstance::StaticClass());
    if(E)
    {
        const FString Kind=E->Kind==TEXT("heavy")?TEXT("Heavy"):TEXT("Guard");
        const FString Path=TEXT("/Game/Hearthward/Campaign/")+Kind+TEXT("/");
        auto* EnemyMesh=LoadObject<USkeletalMesh>(nullptr,*(Path+TEXT("SK_")+Kind+TEXT("_Runtime")));
        checkf(EnemyMesh,TEXT("Campaign enemy mesh missing: %s"),*Path);
        GetMesh()->SetSkeletalMesh(EnemyMesh);GetMesh()->SetRelativeScale3D(FVector(160./(EnemyMesh->GetBounds().BoxExtent.Z*2)));GetMesh()->SetRelativeRotation(FRotator::ZeroRotator);
        GetMesh()->SetAnimInstanceClass(UHearthwardBrotherAnimInstance::StaticClass());
        if(auto* Anim=Cast<UHearthwardBrotherAnimInstance>(GetMesh()->GetAnimInstance()))
        {
            int32 I=0;for(const TCHAR* Name:{TEXT("Idle"),TEXT("Walk"),TEXT("Run"),TEXT("Idle"),TEXT("Attack"),TEXT("Idle")})
                Anim->Clips[I++]=LoadObject<UAnimSequence>(nullptr,*(Path+TEXT("A_")+Kind+TEXT("_")+Name));
        }
        Target->Region=E->Zone;Target->MaximumHealth=HearthwardCampaign::Health(E->Kind,E->Stage);
        Target->Heavy=E->Kind==TEXT("heavy");Target->RewardKind=E->Kind;Target->Restore(E->Combat);
        Target->CreateBodyCollision();if(!Target->Alive())Target->SetCorpse();
    }
    if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))Nav->RegisterNavigationInvoker(this,3500,4500);
    if(!GetController())SpawnDefaultController();
}
bool AHearthwardCampaignActor::WalkTo(FVector Goal,float Acceptance)
{
    auto* AI=Cast<AAIController>(GetController());if(!AI)return false;
    return AI->MoveToLocation(Goal,Acceptance,true,true,true,false,nullptr,false)!=EPathFollowingRequestResult::Failed;
}
void AHearthwardCampaignActor::Tick(float Delta)
{
    Super::Tick(Delta);if(!Enemy || !Target->CanAct())return;
    AttackIn-=Delta;if((DecisionIn-=Delta)>0)return;DecisionIn=.4f;
    auto* C=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();auto* E=C->Enemy(Identity);if(!E)return;
    if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FNavLocation Here,Home;
        if(Nav->ProjectPointToNavigation(GetActorLocation(),Here,FVector(100,100,220)))OffNavigation=0;
        else if((OffNavigation+=.4f)>=15)
        {
            FVector Floor;
            if(C->Ground(E->Home,Floor) && Nav->ProjectPointToNavigation(Floor,Home,FVector(200,200,300)) && TeleportTo(Home.Location+FVector(0,0,80),GetActorRotation()))
            {GetCharacterMovement()->StopMovementImmediately();OffNavigation=0;}
            else C->Feedback=TEXT("敌军岗哨的通路尚不可用，状态已保留。请记录该区域以便修复。");
        }
    }
    AActor* Threat=nullptr;const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    if(Player && Target->Memory.Seen.Contains(TEXT("player")))Threat=const_cast<APawn*>(Player);
    if(!Threat && Target->Memory.Seen.Contains(TEXT("brother")))
        for(TActorIterator<ACharacter> It(GetWorld());It;++It)if(*It!=Player && It->FindComponentByClass<UHearthwardSurvivalComponent>()){Threat=*It;break;}
    if(Threat && FVector::Dist2D(Threat->GetActorLocation(),E->Home)<30000)
    {
        const float Range=E->Kind==TEXT("archer")?1800:180;
        const float Distance=FVector::Distance(GetActorLocation(),Threat->GetActorLocation());
        if(Distance>Range)WalkTo(Threat->GetActorLocation(),Range*.8f);
        else if(AttackIn<=0)
        {
            FCollisionQueryParams Query(SCENE_QUERY_STAT(CampaignAttack),false,this);Query.AddIgnoredActor(Threat);
            if(!GetWorld()->LineTraceTestByChannel(GetActorLocation()+FVector(0,0,40),Threat->GetActorLocation(),ECC_Visibility,Query))
            {
                const auto Calibration=Catalog()->GetObjectField(TEXT("progression"))->GetArrayField(TEXT("enemyCalibration"))[E->Stage-1]->AsObject();
                const double Multiplier=E->Kind==TEXT("heavy")?Number(Calibration,TEXT("heavy_attack_multiplier")):E->Kind==TEXT("archer")?Number(Calibration,TEXT("archer_attack_multiplier")):1;
                const float Power=Number(Calibration,TEXT("guard_attack"))*Multiplier;
                if(E->Kind==TEXT("archer"))
                {
                    const FVector Start=GetActorLocation()+FVector(0,0,40);
                    auto* Arrow=GetWorld()->SpawnActor<AHearthwardProjectile>(Start,FRotator::ZeroRotator);
                    Arrow->EnemyShooter=this;Arrow->Power=Power;Arrow->Event=FGuid::NewGuid();Arrow->Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
                    Arrow->Velocity=(Threat->GetActorLocation()-Start).GetSafeNormal()*3000;Arrow->RemainingRange=3000;Arrow->Lifetime=3;
                }
                else if(auto* Combat=Threat->FindComponentByClass<UHearthwardCombatComponent>())Combat->Damage(Power,TEXT("body"),GetActorLocation(),Target->Heavy,false,FGuid::NewGuid());
                else if(auto* S=Threat->FindComponentByClass<UHearthwardSurvivalComponent>())S->ReceiveDamage(Power,FGuid::NewGuid(),GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch());
                if(auto* A=Cast<UHearthwardBrotherAnimInstance>(GetMesh()->GetAnimInstance()))A->PlayAttack();
                AttackIn=2.5f;
            }
        }
        return;
    }
    FVector Goal=E->Home;
    TArray<FVector> Points;
    if(E->Group==TEXT("prologue"))for(const auto& Offset:{FVector(-500,-500,0),FVector(500,-500,0),FVector(500,500,0),FVector(-500,500,0)})Points.Add(E->Home+Offset);
    if(E->Group==TEXT("field"))for(const auto& V:Catalog()->GetObjectField(TEXT("campaign"))->GetObjectField(TEXT("field_encounter"))->GetArrayField(TEXT("patrol_relative_m")))
    {const auto& A=V->AsArray();Points.Add(FVector(-38000+A[0]->AsNumber()*100,-84500+A[1]->AsNumber()*100,E->Home.Z));}
    if(E->Group==TEXT("base"))
    {
        const auto Row=HearthwardCampaign::Find(TEXT("enemies"),Identity);const FString Patrol=Text(Row,TEXT("patrol"));
        if(!Patrol.IsEmpty())for(const auto& V:HearthwardCampaign::Find(TEXT("zones"),E->Zone)->GetArrayField(TEXT("patrols")))
            if(Text(V->AsObject(),TEXT("id"))==Patrol)for(const auto& P:V->AsObject()->GetArrayField(TEXT("points")))
            {const auto& A=P->AsArray();Points.Add(FVector(A[0]->AsNumber()*100,A[1]->AsNumber()*100,E->Home.Z));}
    }
    if(Points.Num()==4)
    {
        Goal=Points[E->PatrolPoint];
        if(FVector::Dist2D(GetActorLocation(),Goal)<150)
        {
            if(E->PatrolPoint==0 && Pause<3){Pause+=.4f;return;}
            Pause=0;E->PatrolPoint=(E->PatrolPoint+1)%4;Goal=Points[E->PatrolPoint];
        }
    }
    FVector Floor;if(C->Ground(Goal,Floor))WalkTo(Floor+FVector(0,0,80));
}
