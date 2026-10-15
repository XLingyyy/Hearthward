#include "HearthwardCampaignActor.h"
#include "HearthwardPersonalStory.h"
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
#include "Components/WidgetComponent.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInterface.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "Widgets/Text/STextBlock.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
using namespace HearthwardData;
AHearthwardCampaignActor::AHearthwardCampaignActor()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.1f;
    GetCapsuleComponent()->InitCapsuleSize(30,80);
    GetCharacterMovement()->MaxWalkSpeed=240;GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->bUseRVOAvoidance=true;GetCharacterMovement()->AvoidanceConsiderationRadius=300;
    bUseControllerRotationYaw=false;AIControllerClass=AAIController::StaticClass();AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Target=CreateDefaultSubobject<UHearthwardCombatTargetComponent>(TEXT("CampaignTarget"));
    Target->CampaignTarget=true;
    LaborLabel=CreateDefaultSubobject<UWidgetComponent>(TEXT("LaborLabel"));
    LaborLabel->SetupAttachment(GetRootComponent());LaborLabel->SetRelativeLocation(FVector(0,0,120));
    LaborLabel->SetWidgetSpace(EWidgetSpace::Screen);LaborLabel->SetDrawAtDesiredSize(true);
    LaborLabel->SetVisibility(false);LaborLabel->PrimaryComponentTick.bStartWithTickEnabled=false;
}
void AHearthwardCampaignActor::Initialize(FName Id,bool Hostile)
{
    Identity=Id;Enemy=Hostile;Target->Id=Id;
    if(!Hostile && HearthwardPersonalStory::Row(Id) && !FindComponentByClass<UHearthwardPersonalInteraction>())
    {
        auto* Story=NewObject<UHearthwardPersonalInteraction>(this);AddInstanceComponent(Story);Story->SetupAttachment(GetRootComponent());Story->Identity=Id;Story->RegisterComponent();
    }
    auto* C=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
    auto* E=C->Enemy(Id);
    Target->Protected=!Hostile;
    // Civilian instances share the existing rig and locomotion assets.
    GetMesh()->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Brother/UE5/SK_Brother.SK_Brother")));
    GetMesh()->SetRelativeScale3D(FVector(160.f/97.863766f));GetMesh()->SetRelativeLocation(FVector(0,0,-80));GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
    GetMesh()->SetAnimInstanceClass(UHearthwardBrotherAnimInstance::StaticClass());
    if(E)
    {
        const bool Archer=E->Kind==TEXT("archer");
        const FString Kind=Archer?TEXT("Archer"):E->Kind==TEXT("heavy")?TEXT("Heavy"):TEXT("Guard");
        const FString Path=Archer?TEXT("/Game/Hearthward/Assets/TASK-095/Archer/"):TEXT("/Game/Hearthward/Campaign/")+Kind+TEXT("/");
        auto* EnemyMesh=LoadObject<USkeletalMesh>(nullptr,*(Path+TEXT("SK_")+Kind+(Archer?TEXT("_Combat"):TEXT("_Runtime"))));
        checkf(EnemyMesh,TEXT("Campaign enemy mesh missing: %s"),*Path);
        GetMesh()->SetSkeletalMesh(EnemyMesh);GetMesh()->SetRelativeScale3D(FVector(160./(Archer?99.74417:EnemyMesh->GetBounds().BoxExtent.Z*2)));GetMesh()->SetRelativeRotation(FRotator::ZeroRotator);
        if(Archer)GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        GetMesh()->SetAnimInstanceClass(UHearthwardBrotherAnimInstance::StaticClass());
        if(auto* Anim=Cast<UHearthwardBrotherAnimInstance>(GetMesh()->GetAnimInstance()))
        {
            int32 I=0;for(const TCHAR* Name:{TEXT("Idle"),TEXT("Walk"),TEXT("Run"),TEXT("Idle"),TEXT("Attack"),TEXT("Idle")})
                Anim->Clips[I++]=LoadObject<UAnimSequence>(nullptr,*(Path+TEXT("A_")+Kind+TEXT("_")+(Archer && FString(Name)==TEXT("Attack")?TEXT("Shoot"):Name)));
        }
        Target->Region=E->Zone;Target->MaximumHealth=HearthwardCampaign::Health(E->Kind,E->Stage);
        Target->Heavy=E->Kind==TEXT("heavy");Target->RewardKind=E->Kind;Target->Restore(E->Combat);
        Target->CreateBodyCollision();if(!Target->Alive())Target->SetCorpse();
    }
    else GetMesh()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-095/Costumes/M_Civilian_CoarseCloth.M_Civilian_CoarseCloth")));
    if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))Nav->RegisterNavigationInvoker(this,3500,4500);
    if(!GetController())SpawnDefaultController();
}
void AHearthwardCampaignActor::PresentLabor(FName Region,const FString& Status,bool Working,FName LifePose)
{
    LaborRegion=Region;LaborStatus=Status;
    if(!LaborText)
    {
        LaborTypeface=MakeShared<FCompositeFont>(NAME_None,FPaths::ProjectDir()/TEXT("Resources/UI/Fonts/NotoSerifCJKsc-Regular.otf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
        LaborText=SNew(STextBlock).Font(FSlateFontInfo(LaborTypeface,16)).ColorAndOpacity(FLinearColor(FColor(240,220,170)));
        LaborLabel->SetSlateWidget(LaborText);
    }
    LaborText->SetText(FText::FromString(Status));
    const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    const bool ShowLabel=Player && FVector::Dist2D(Player->GetActorLocation(),GetActorLocation())<=280;
    LaborLabel->SetVisibility(ShowLabel);LaborLabel->SetComponentTickEnabled(ShowLabel);
    if(Working)LifePose=NAME_None;
    if(!LaborPresented || LaborWorking!=Working || CampLifePose!=LifePose)
    {
        const TCHAR* Clip=Working?TEXT("/Game/Characters/Brother/Animation/A_Brother_Dig.A_Brother_Dig")
            :TEXT("/Game/Characters/Brother/Animation/A_Brother_Idle.A_Brother_Idle");
        if(LifePose==TEXT("seated"))Clip=TEXT("/Game/Hearthward/Assets/TASK-109/CampLife/A_Brother_SeatedIdle.A_Brother_SeatedIdle");
        if(LifePose==TEXT("seated_talk"))Clip=TEXT("/Game/Hearthward/Assets/TASK-109/CampLife/A_Brother_SeatedTalk.A_Brother_SeatedTalk");
        if(LifePose==TEXT("memento"))Clip=TEXT("/Game/Hearthward/Assets/TASK-126/A_Brother_Operate");
        if(LifePose==TEXT("walking"))Clip=TEXT("/Game/Characters/Brother/Animation/A_Brother_Walk.A_Brother_Walk");
        const bool WasSeated=CampLifePose==TEXT("seated") || CampLifePose==TEXT("seated_talk");
        const bool Seated=LifePose==TEXT("seated") || LifePose==TEXT("seated_talk");
        LifeTransitionNext=LoadObject<UAnimSequence>(nullptr,Clip);
        if(WasSeated!=Seated)
        {
            auto* Transition=LoadObject<UAnimSequence>(nullptr,Seated?TEXT("/Game/Hearthward/Assets/TASK-126/A_Brother_SitDown"):TEXT("/Game/Hearthward/Assets/TASK-126/A_Brother_StandUp"));
            GetMesh()->PlayAnimation(Transition,false);LifeTransitionRemaining=Transition->GetPlayLength();
        }
        else {GetMesh()->PlayAnimation(LifeTransitionNext,true);LifeTransitionRemaining=0;}
    }
    LaborPresented=true;LaborWorking=Working;CampLifePose=LifePose;
}
bool AHearthwardCampaignActor::WalkTo(FVector Goal,float Acceptance)
{
    auto* AI=Cast<AAIController>(GetController());if(!AI)return false;
    return AI->MoveToLocation(Goal,Acceptance,true,true,true,false,nullptr,false)!=EPathFollowingRequestResult::Failed;
}
void AHearthwardCampaignActor::Tick(float Delta)
{
    Super::Tick(Delta);
    if(LifeTransitionRemaining>0 && (LifeTransitionRemaining-=Delta)<=0 && LifeTransitionNext)GetMesh()->PlayAnimation(LifeTransitionNext,true);
    if(!Enemy || !Target->CanAct() || GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->IsTraveling())
    {CancelBowShot();CancelMelee();NoiseEpoch.Invalidate();return;}
    AttackIn-=Delta;
    UpdateBowShot(Delta);
    const bool WasMelee=MeleeTarget.IsValid();
    UpdateMelee(Delta);
    if(WasMelee)return;
    if((DecisionIn-=Delta)>0)return;DecisionIn=.4f;
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
    if(Threat)if(const auto* S=Threat->FindComponentByClass<UHearthwardSurvivalComponent>();S && !S->Alive())Threat=nullptr;
    if(Threat && FVector::Dist2D(Threat->GetActorLocation(),E->Home)<30000)
    {
        NoiseEpoch.Invalidate();
        const float Range=E->Kind==TEXT("archer")?1800:180;
        const float Distance=FVector::Distance(GetActorLocation(),Threat->GetActorLocation());
        if(Distance>Range)WalkTo(Threat->GetActorLocation(),Range*.8f);
        else if(AttackIn<=0 && (E->Kind==TEXT("archer") || HasAttackSpace(Threat)))
        {
            FCollisionQueryParams Query(SCENE_QUERY_STAT(CampaignAttack),false,this);Query.AddIgnoredActor(Threat);
            if(!GetWorld()->LineTraceTestByChannel(GetActorLocation()+FVector(0,0,40),Threat->GetActorLocation(),ECC_Visibility,Query))
            {
                const auto Calibration=Catalog()->GetObjectField(TEXT("progression"))->GetArrayField(TEXT("enemyCalibration"))[E->Stage-1]->AsObject();
                const double Multiplier=E->Kind==TEXT("heavy")?Number(Calibration,TEXT("heavy_attack_multiplier")):E->Kind==TEXT("archer")?Number(Calibration,TEXT("archer_attack_multiplier")):1;
                const float Power=Number(Calibration,TEXT("guard_attack"))*Multiplier;
                if(E->Kind==TEXT("archer"))
                {
                    if(auto* AI=Cast<AAIController>(GetController()))AI->StopMovement();
                    GetCharacterMovement()->StopMovementImmediately();
                    SetActorRotation(FRotator(0,(Threat->GetActorLocation()-GetActorLocation()).Rotation().Yaw,0));
                    BowTarget=Threat;BowRemaining=.6f;BowPower=Power;
                    BowEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
                }
                else
                {
                    if(auto* AI=Cast<AAIController>(GetController()))AI->StopMovement();
                    GetCharacterMovement()->StopMovementImmediately();
                    MeleeTarget=Threat;MeleeTiming=HearthwardEnemyRole::Attack(Identity,E->Kind);
                    MeleeElapsed=0;MeleePower=Power;MeleeCommitted=false;MeleeEvent=FGuid::NewGuid();
                    MeleeEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
                    MeleeFacing=(Threat->GetActorLocation()-GetActorLocation()).GetSafeNormal2D();
                    SetActorRotation(MeleeFacing.Rotation());
                }
                if(auto* A=Cast<UHearthwardBrotherAnimInstance>(GetMesh()->GetAnimInstance()))
                {
                    if(E->Kind==TEXT("archer"))A->PlayAttack();
                    else A->PlayTimedAttack(MeleeTiming.Windup,MeleeTiming.Active,MeleeTiming.Recovery);
                }
                AttackIn=2.5f;
            }
            else if(E->Kind==TEXT("archer"))
            {
                const FVector Side=FVector::CrossProduct((Threat->GetActorLocation()-GetActorLocation()).GetSafeNormal2D(),FVector::UpVector);
                WalkTo(GetActorLocation()+Side*250,50);
            }
        }
        else if(E->Kind!=TEXT("archer") && !HasAttackSpace(Threat))
        {
            const FVector Away=(GetActorLocation()-Threat->GetActorLocation()).GetSafeNormal2D();
            WalkTo(Threat->GetActorLocation()+Away*260+FVector::CrossProduct(Away,FVector::UpVector)*120,50);
        }
        return;
    }
    if(UpdateNoiseInvestigation(.4f))return;
    const auto& Memory=Target->Memory;
    if(Memory.InvestigationRemaining>0 && FVector::Dist2D(Memory.Investigation,E->Home)<30000)
    {
        if(!WalkTo(Memory.Investigation))if(auto* AI=Cast<AAIController>(GetController()))AI->StopMovement();
        return;
    }
    // LastKnown survives suspicion expiry in saves; only positive detection keeps the search active.
    for(const FName Who:{FName(TEXT("player")),FName(TEXT("brother"))})
        if(const auto* Known=Memory.LastKnown.Find(Who);Known && Memory.Detection.FindRef(Who)>0 && FVector::Dist2D(*Known,E->Home)<30000)
        {
            if(!WalkTo(*Known))if(auto* AI=Cast<AAIController>(GetController()))AI->StopMovement();
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
    if(Identity==TEXT("home_workshops_16"))if(const auto* Landing=C->State.Positions.Find(TEXT("round2_archer_landing")))
    {WalkTo(*Landing+FVector(0,0,80));return;}
    FVector Floor;if(C->Ground(Goal,Floor))WalkTo(Floor+FVector(0,0,80));
}

void AHearthwardCampaignActor::CancelBowShot()
{
    if(BowRemaining<=0)return;
    BowRemaining=0;BowTarget.Reset();
    if(auto* Anim=Cast<UHearthwardBrotherAnimInstance>(GetMesh()->GetAnimInstance()))Anim->CancelAttack();
}

void AHearthwardCampaignActor::UpdateBowShot(float Delta)
{
    if(BowRemaining<=0)return;
    if(!BowTarget.IsValid() || BowEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())
    {CancelBowShot();return;}
    if(const auto* S=BowTarget->FindComponentByClass<UHearthwardSurvivalComponent>();S && !S->Alive()){CancelBowShot();return;}
    BowRemaining=FMath::Max(0.f,BowRemaining-Delta);
    if(BowRemaining>UE_KINDA_SMALL_NUMBER)return;
    BowRemaining=0;
    const FVector Start=GetMesh()->GetSocketLocation(TEXT("NockedArrow"));
    const FVector Destination=BowTarget->GetActorLocation();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CampaignBowRelease),false,this);Query.AddIgnoredActor(BowTarget.Get());
    const bool Blocked=GetWorld()->LineTraceTestByChannel(GetActorLocation()+FVector(0,0,40),Start,ECC_Visibility,Query)
        || GetWorld()->LineTraceTestByChannel(Start,Destination,ECC_Visibility,Query);
    BowTarget.Reset();
    if(Blocked)return;
    auto* Arrow=GetWorld()->SpawnActor<AHearthwardProjectile>(Start,(Destination-Start).Rotation());
    Arrow->EnemyShooter=this;Arrow->Power=BowPower;Arrow->Event=FGuid::NewGuid();Arrow->Epoch=BowEpoch;
    Arrow->Velocity=(Destination-Start).GetSafeNormal()*3000;Arrow->RemainingRange=3000;Arrow->Lifetime=3;
}

bool AHearthwardCampaignActor::HasAttackSpace(AActor* Threat) const
{
    int32 Active=0;
    for(TActorIterator<AHearthwardCampaignActor> It(GetWorld());It;++It)
        if(*It!=this && It->Target->CanAct() && It->MeleeTarget.Get()==Threat && ++Active>=2)return false;
    return true;
}
bool AHearthwardCampaignActor::CanInvestigateNoise() const
{
    if(!Enemy || !Target->CanAct() || !Target->Memory.Seen.IsEmpty() || MeleeTarget.IsValid() || BowRemaining>0 || NoiseEpoch.IsValid())return false;
    for(const auto& Detection:Target->Memory.Detection)if(Detection.Value>=1)return false;
    return true;
}
bool AHearthwardCampaignActor::BeginNoiseInvestigation(FVector Source,FGuid Epoch)
{
    if(!CanInvestigateNoise() || Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())return false;
    const auto* E=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Enemy(Identity);
    if(!E || FVector::Dist2D(Source,E->Home)>=30000 || !WalkTo(Source,100))return false;
    NoisePoint=Source;NoiseEpoch=Epoch;NoiseTravelRemaining=30;NoiseInspectRemaining=6;
    return true;
}
bool AHearthwardCampaignActor::UpdateNoiseInvestigation(float Delta)
{
    if(!NoiseEpoch.IsValid())return false;
    if(NoiseEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())
    {NoiseEpoch.Invalidate();return false;}
    if(FVector::Dist2D(GetActorLocation(),NoisePoint)<=150)
    {
        if(auto* AI=Cast<AAIController>(GetController()))AI->StopMovement();
        Target->Awareness=TEXT("查看声源");NoiseInspectRemaining-=Delta;
        if(NoiseInspectRemaining>0)return true;
    }
    else
    {
        Target->Awareness=TEXT("调查声源");NoiseTravelRemaining-=Delta;
        if(NoiseTravelRemaining>0 && WalkTo(NoisePoint,100))return true;
    }
    NoiseEpoch.Invalidate();return false;
}
void AHearthwardCampaignActor::CancelMelee()
{
    if(!MeleeTarget.IsValid())return;
    MeleeTarget.Reset();MeleeCommitted=false;
    if(auto* A=Cast<UHearthwardBrotherAnimInstance>(GetMesh()->GetAnimInstance()))A->CancelAttack();
}
void AHearthwardCampaignActor::UpdateMelee(float Delta)
{
    if(!MeleeTarget.IsValid())return;
    auto* Victim=MeleeTarget.Get();
    const auto* Survival=Victim->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(MeleeEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch() || (Survival && !Survival->Alive()))
    {CancelMelee();return;}
    const float Previous=MeleeElapsed;MeleeElapsed+=Delta;
    // Crossing the contact boundary once also handles a long frame without applying two hits.
    if(!MeleeCommitted && Previous<MeleeTiming.Windup+MeleeTiming.Active*.5f && MeleeElapsed>=MeleeTiming.Windup+MeleeTiming.Active*.5f)
    {
        MeleeCommitted=true;
        const FVector Offset=Victim->GetActorLocation()-GetActorLocation();
        FCollisionQueryParams Query(SCENE_QUERY_STAT(CampaignMeleeContact),false,this);Query.AddIgnoredActor(Victim);
        if(Offset.Size()<=210 && FVector::DotProduct(Offset.GetSafeNormal2D(),MeleeFacing)>.5f
            && !GetWorld()->LineTraceTestByChannel(GetActorLocation()+FVector(0,0,40),Victim->GetActorLocation(),ECC_Visibility,Query))
        {
            if(auto* Combat=Victim->FindComponentByClass<UHearthwardCombatComponent>())Combat->Damage(MeleePower,TEXT("body"),GetActorLocation(),Target->Heavy,false,MeleeEvent);
            else if(auto* S=Victim->FindComponentByClass<UHearthwardSurvivalComponent>())S->ReceiveDamage(MeleePower,MeleeEvent,MeleeEpoch);
        }
    }
    if(MeleeElapsed>=MeleeTiming.Duration())CancelMelee();
}
