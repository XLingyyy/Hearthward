#include "HearthwardShortcutActor.h"
#include "../Interaction/HearthwardInteractionAlignment.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "NavLinkCustomComponent.h"
#include "NavAreas/NavArea_Default.h"
#include "NavAreas/NavArea_Null.h"
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

FString UHearthwardDeviceInteraction::GetInteractionPrompt(AActor*) const {return CastChecked<AHearthwardShortcutActor>(GetOwner())->Prompt();}
FString UHearthwardDeviceInteraction::CompleteInteraction(AActor* Player){return CastChecked<AHearthwardShortcutActor>(GetOwner())->Use(Player);}
AHearthwardShortcutActor::AHearthwardShortcutActor()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Device"));Mesh->SetupAttachment(RootComponent);Mesh->SetCollisionProfileName(TEXT("NoCollision"));
    Door=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));Door->SetupAttachment(RootComponent);Door->SetCollisionProfileName(TEXT("NoCollision"));
    Barrier=CreateDefaultSubobject<UBoxComponent>(TEXT("Barrier"));Barrier->SetupAttachment(RootComponent);Barrier->SetBoxExtent(FVector(35,175,125));Barrier->SetRelativeLocation(FVector(0,0,125));Barrier->SetCollisionProfileName(TEXT("BlockAll"));
    Interaction=CreateDefaultSubobject<UHearthwardDeviceInteraction>(TEXT("Interaction"));Interaction->SetupAttachment(RootComponent);Interaction->SetRelativeLocation(FVector(-90,0,90));
    LadderLink=CreateDefaultSubobject<UNavLinkCustomComponent>(TEXT("LadderNavigation"));LadderLink->SetEnabled(false);
}
void AHearthwardShortcutActor::Configure(FName Id,EHearthwardDeviceKind Type)
{
    Identity=Id;Kind=Type;
    const TCHAR* Name=Kind==EHearthwardDeviceKind::Latch?TEXT("Latch"):Kind==EHearthwardDeviceKind::Ladder?TEXT("Ladder"):Kind==EHearthwardDeviceKind::Winch?TEXT("Winch"):TEXT("Beam");
    const FString Path=FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-118/SM_Round2_%s.SM_Round2_%s"),Name,Name);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));
    if(Kind==EHearthwardDeviceKind::Latch)
    {
        Door->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Door->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber.M_OldTimber")));
        Door->SetRelativeScale3D(FVector(.2,3.5,2.5));
    }
    if(Kind==EHearthwardDeviceKind::Beam)
    {
        Barrier->SetBoxExtent(FVector(16,160,17));Barrier->SetRelativeLocation(FVector(0,0,116));
        double Highest=-DBL_MAX;
        for(float X:{-16.f,0.f,16.f})for(float Y:{180.f,340.f,500.f})
        {
            FVector Floor;
            if(GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Ground(GetActorTransform().TransformPosition(FVector(X,Y,0)),Floor))Highest=FMath::Max(Highest,Floor.Z);
        }
        if(Highest!=-DBL_MAX)BeamRestZ=Highest-GetActorLocation().Z+2;
    }
    if(Kind==EHearthwardDeviceKind::Winch)Barrier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(Kind==EHearthwardDeviceKind::Winch || Kind==EHearthwardDeviceKind::Beam)Interaction->SetRelativeLocation(FVector(-65,-115,90));
    if(Kind==EHearthwardDeviceKind::Ladder)
    {
        Barrier->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetRelativeRotation(FRotator(20,0,0));Mesh->SetRelativeScale3D(FVector(1,1,1.2));
        LadderLink->SetLinkData(FVector(-160,0,220),FVector(100,0,0),ENavLinkDirection::BothWays);
        LadderLink->SetEnabledArea(UNavArea_Default::StaticClass());LadderLink->SetDisabledArea(UNavArea_Null::StaticClass());LadderLink->SetMoveReachedLink(this,&AHearthwardShortcutActor::LinkReached);
        Interaction->SetRelativeLocation(FVector(-160,0,300));
    }
    GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->BindInteractionTarget(Identity,Kind==EHearthwardDeviceKind::Latch || Kind==EHearthwardDeviceKind::Ladder?EHearthwardPersistentKind::Shortcut:EHearthwardPersistentKind::Cooperation,this);
    ApplyState();
}
bool AHearthwardShortcutActor::IsOpen() const
{
    const auto& State=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->InteractionState();
    if(Kind==EHearthwardDeviceKind::Latch || Kind==EHearthwardDeviceKind::Ladder)
    {const auto* S=State.Shortcuts.FindByPredicate([&](const auto& V){return V.Id==Identity;});return S && S->Open;}
    const auto* S=State.Cooperation.FindByPredicate([&](const auto& V){return V.Id==Identity;});return S && S->Result!=EHearthwardCooperationResult::Unchanged;
}
FString AHearthwardShortcutActor::Prompt() const
{
    if(Active())return Phase==EHearthwardDevicePhase::Approach?TEXT("弟弟正在就位 · E 取消"):Kind==EHearthwardDeviceKind::Winch?TEXT("弟弟正维持绞盘 · 进入后开启内闩 · E 取消"):TEXT("正在操作 · 移动或 E 取消");
    if(IsOpen())return Kind==EHearthwardDeviceKind::Ladder?TEXT("E 沿短梯登降 · 再按 E 取消"):TEXT("通路已经打开");
    switch(Kind)
    {
        case EHearthwardDeviceKind::Latch:return TEXT("E 打开内侧门闩");
        case EHearthwardDeviceKind::Ladder:return TEXT("E 放下短梯");
        case EHearthwardDeviceKind::Winch:return TEXT("E 请弟弟维持绞盘 · 可走外侧绕路");
        default:return TEXT("E 与弟弟移开横梁 · 需同时到位3秒");
    }
}
FString AHearthwardShortcutActor::Use(AActor* Player)
{
    if(Active()){Cancel();return TEXT("操作已取消，弟弟恢复原指令");}
    if(Kind==EHearthwardDeviceKind::Ladder && IsOpen())return BeginClimb(Cast<ACharacter>(Player))?TEXT("沿短梯登降中"):TEXT("短梯入口或落点被挡住，请从端点靠近");
    auto* S=IsValid(Player)?Player->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    if(!S || !S->Alive() || S->Busy() || IsOpen() || GetWorld()->IsPaused())return TEXT("当前无法操作");
    if(FVector::Dist(Player->GetActorLocation(),Interaction->GetComponentLocation())>Interaction->MaxDistance)return TEXT("请靠近操作点");
    if((Kind==EHearthwardDeviceKind::Latch || Kind==EHearthwardDeviceKind::Ladder) && GetActorTransform().InverseTransformPosition(Player->GetActorLocation()).X>0)return TEXT("需要从内侧操作，可从外侧绕行");
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Round2DeviceUse),false,Player);Query.AddIgnoredActor(this);
    if(GetWorld()->LineTraceTestByChannel(Player->GetActorLocation(),Interaction->GetComponentLocation(),ECC_Visibility,Query))return TEXT("操作点被遮挡");
    auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();if(G && G->InCombat())return TEXT("先脱离战斗再操作机关");
    User=Player;StartPosition=Player->GetActorLocation();PlayerHealth=S->Health();Elapsed=0;ApproachTime=0;ContactStarted=false;
    Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    if(Kind==EHearthwardDeviceKind::Beam || Kind==EHearthwardDeviceKind::Winch)
    {
        for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        {
            auto* BS=It->FindComponentByClass<UHearthwardSurvivalComponent>();
            if(BS && BS->Alive() && !BS->Busy() && !It->EquipmentBusy() && !It->CooperationOwner.IsValid() && FVector::Dist(It->GetActorLocation(),GetActorLocation())<=2000)
            {Brother=*It;BrotherHealth=BS->Health();break;}
        }
        if(!Brother.IsValid()){User.Reset();return TEXT("弟弟需在附近且空闲；也可沿外侧绕行");}
        Brother->CooperationOwner=this;Brother->CancelMeleeAttack();Brother->StopNavigation();Phase=EHearthwardDevicePhase::Approach;
    }
    else Phase=EHearthwardDevicePhase::Ready;
    return Prompt();
}
bool AHearthwardShortcutActor::PersistOpen()
{
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    const auto Type=Kind==EHearthwardDeviceKind::Latch || Kind==EHearthwardDeviceKind::Ladder?EHearthwardPersistentKind::Shortcut:EHearthwardPersistentKind::Cooperation;
    if(!Save->BindInteractionTarget(Identity,Type,this))return false;
    const auto Ticket=Save->BeginInteraction(Identity);if(!Ticket.Operation.IsValid())return false;
    auto Next=Save->InteractionState();
    if(Type==EHearthwardPersistentKind::Shortcut)
    {auto* S=Next.Shortcuts.FindByPredicate([&](const auto& V){return V.Id==Identity;});if(!S){FHearthwardShortcutState V;V.Id=Identity;S=&Next.Shortcuts.Add_GetRef(V);}S->Open=true;}
    else
    {auto* S=Next.Cooperation.FindByPredicate([&](const auto& V){return V.Id==Identity;});if(!S){FHearthwardCooperationState V;V.Id=Identity;S=&Next.Cooperation.Add_GetRef(V);}S->Result=Kind==EHearthwardDeviceKind::Beam?EHearthwardCooperationResult::Cleared:EHearthwardCooperationResult::Released;}
    if(!Save->CommitInteraction(Ticket,Next)){Save->CancelInteraction();return false;}
    if(User.IsValid())if(auto* Audio=User->FindComponentByClass<UHearthwardPresentationComponent>())Audio->InteractionSucceeded(TEXT("interaction.device"),Ticket.Operation,GetActorLocation());
    return true;
}
bool AHearthwardShortcutActor::CanClose() const
{
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Round2GateSafety),false,this);
    return !GetWorld()->OverlapAnyTestByObjectType(Barrier->GetComponentLocation(),Barrier->GetComponentQuat(),FCollisionObjectQueryParams(ECC_Pawn),FCollisionShape::MakeBox(Barrier->GetScaledBoxExtent()+FVector(60,60,30)),Query);
}
void AHearthwardShortcutActor::ApplyState()
{
    if(Kind==EHearthwardDeviceKind::Winch)return;
    const bool Open=IsOpen() || TemporaryOpen;
    if(Kind==EHearthwardDeviceKind::Ladder)
    {
        Barrier->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetRelativeLocation(FVector(90,0,Open?0:220));LadderLink->SetEnabled(Open);return;
    }
    Barrier->SetCollisionEnabled(Open?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);
    Mesh->SetRelativeLocation(Kind==EHearthwardDeviceKind::Beam?FVector(0,Open?340:0,Open?BeamRestZ:100):FVector(-20,0,Open?360:100));
    Door->SetRelativeLocation(FVector(0,0,Open?385:125));
}
void AHearthwardShortcutActor::Cancel()
{
    FinishClimb();
    for(AActor* A:{User.Get(),static_cast<AActor*>(Brother.Get())})if(A)if(auto* Contact=A->FindComponentByClass<UHearthwardInteractionAlignment>())Contact->Cancel();
    if(Brother.IsValid() && Brother->CooperationOwner.Get()==this){Brother->StopNavigation();Brother->CooperationOwner.Reset();}
    Brother.Reset();User.Reset();Phase=EHearthwardDevicePhase::Cancel;
    if(Kind==EHearthwardDeviceKind::Beam)ApplyState();
    if(LinkedGate.IsValid() && LinkedGate->CanClose()){LinkedGate->TemporaryOpen=false;LinkedGate->ApplyState();}
}
FVector AHearthwardShortcutActor::BeamPosition(float Progress) const
{
    if(Progress<.25f)return FMath::Lerp(FVector(0,0,100),FVector(0,0,130),Progress/.25f);
    if(Progress<.75f)return FMath::Lerp(FVector(0,0,130),FVector(0,340,130),(Progress-.25f)/.5f);
    return FMath::Lerp(FVector(0,340,130),FVector(0,340,BeamRestZ),(Progress-.75f)/.25f);
}
bool AHearthwardShortcutActor::BeamPathClear() const
{
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Round2BeamClearance),false,this);
    // The two operators' animated hands deliberately enter the beam's swept volume.
    if(User.IsValid())Query.AddIgnoredActor(User.Get());
    if(Brother.IsValid())Query.AddIgnoredActor(Brother.Get());
    const FVector Points[]={FVector(0,0,118),FVector(0,0,148),FVector(0,340,148),FVector(0,340,BeamRestZ+18)};
    for(int32 Index=1;Index<UE_ARRAY_COUNT(Points);++Index)
    {
        FHitResult Hit;
        if(GetWorld()->SweepSingleByChannel(Hit,GetActorTransform().TransformPosition(Points[Index-1]),GetActorTransform().TransformPosition(Points[Index]),GetActorQuat(),ECC_Visibility,FCollisionShape::MakeBox(FVector(16,160,16)),Query))
        {
#if !UE_BUILD_SHIPPING
            UE_LOG(LogTemp,Display,TEXT("Round2BeamBlocked %s segment=%d rest=%.2f actor=%s component=%s"),*Identity.ToString(),Index,BeamRestZ,*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()));
#endif
            return false;
        }
    }
    return true;
}
bool AHearthwardShortcutActor::BeginContact()
{
    if(Kind==EHearthwardDeviceKind::Beam && !BeamPathClear())return false;
    const float Seconds=Kind==EHearthwardDeviceKind::Beam?3.f:1.2f;
    auto Align=[&](AActor* A,float Side,bool Hold)
    {
        auto* C=Cast<ACharacter>(A);auto* Contact=A?A->FindComponentByClass<UHearthwardInteractionAlignment>():nullptr;
        if(!Contact)return true;
        const float Forward=Kind==EHearthwardDeviceKind::Ladder?-160:Kind==EHearthwardDeviceKind::Latch?-90:-65;
        FVector Point=GetActorTransform().TransformPosition(FVector(Forward,Side,C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2+(Kind==EHearthwardDeviceKind::Ladder?220:0)));
        const float Half=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        FCollisionQueryParams Query(SCENE_QUERY_STAT(DeviceContactFloor),false,C);Query.AddIgnoredActor(this);FHitResult Floor;
        if(!GetWorld()->LineTraceSingleByChannel(Floor,Point-FVector(0,0,Half-60),Point-FVector(0,0,Half+60),ECC_Visibility,Query))return false;
        if(!C->GetCharacterMovement()->IsWalkable(Floor))return false;
        const float SlopeSupport=C->GetCapsuleComponent()->GetScaledCapsuleRadius()*(1.f/Floor.ImpactNormal.Z-1.f);
        Point.Z=Floor.ImpactPoint.Z+Half+SlopeSupport+UCharacterMovementComponent::MAX_FLOOR_DIST;
        return Contact->Begin(Kind==EHearthwardDeviceKind::Beam?FName(TEXT("MoveBeam")):FName(TEXT("Operate")),Seconds+.15f,this,Point,GetActorRotation(),Hold);
    };
    if(Kind!=EHearthwardDeviceKind::Winch && !Align(User.Get(),Kind==EHearthwardDeviceKind::Beam?-115:0,false))return false;
    if(Brother.IsValid() && !Align(Brother.Get(),Kind==EHearthwardDeviceKind::Beam?115:0,Kind==EHearthwardDeviceKind::Winch))return false;
    StartPosition=User->GetActorLocation();ContactStarted=true;return true;
}
void AHearthwardShortcutActor::EndPlay(const EEndPlayReason::Type Reason){Cancel();Super::EndPlay(Reason);}
void AHearthwardShortcutActor::Tick(float Delta)
{
    Super::Tick(Delta);if(GetWorld()->IsPaused() || Identity.IsNone())return;
    if(Climber.IsValid()){TickClimb(Delta);return;}
    if(Kind==EHearthwardDeviceKind::Ladder && IsOpen())if(auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0))
    {
        const FVector A=LadderLink->GetStartPoint(),B=LadderLink->GetEndPoint();const FVector Feet=Player->GetActorLocation()-FVector(0,0,80);
        Interaction->SetWorldLocation((FVector::DistSquared(Feet,A)<FVector::DistSquared(Feet,B)?A:B)+FVector(0,0,80));
    }
    if(!Active())
    {
        if(TemporaryOpen && (!TemporaryHolder.IsValid() || !TemporaryHolder->Active()) && CanClose()){TemporaryOpen=false;ApplyState();}
        return;
    }
    auto* PS=User.IsValid()?User->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    auto* BS=Brother.IsValid()?Brother->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    auto* G=User.IsValid()?User->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    auto* Combat=User.IsValid()?User->FindComponentByClass<UHearthwardCombatComponent>():nullptr;
    const bool MayLeave=Kind==EHearthwardDeviceKind::Winch && Phase==EHearthwardDevicePhase::Hold;
    if(!PS || !PS->Alive() || PS->Busy() || PS->Health()<PlayerHealth || (G && G->InCombat()) || (Combat && Combat->Busy())
        || Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()
        || GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->IsTraveling()
        || FVector::Dist(User->GetActorLocation(),MayLeave?GetActorLocation():StartPosition)>(MayLeave?1800:60)
        || (!MayLeave && User->GetVelocity().Size2D()>5)
        || ((Kind==EHearthwardDeviceKind::Winch || Kind==EHearthwardDeviceKind::Beam) && (!BS || !BS->Alive() || BS->Health()<BrotherHealth || Brother->CooperationOwner.Get()!=this)))
    {Cancel();return;}
    const float BeamShift=Kind==EHearthwardDeviceKind::Beam && ContactStarted?Mesh->GetRelativeLocation().Y:0;
    const FVector BrotherPoint=GetActorTransform().TransformPosition(FVector(-65,(Kind==EHearthwardDeviceKind::Beam?115:0)+BeamShift,90));
    if(Phase==EHearthwardDevicePhase::Approach)
    {
        ApproachTime+=Delta;if(ApproachTime>15){Cancel();return;}
        if(FVector::Dist2D(Brother->GetActorLocation(),BrotherPoint)>55){Brother->NavigateToLocation(BrotherPoint,180,20);return;}
        Brother->StopNavigation();Brother->SetActorRotation(GetActorRotation());Phase=EHearthwardDevicePhase::Ready;Elapsed=0;
    }
    if(Brother.IsValid() && FVector::Dist2D(Brother->GetActorLocation(),BrotherPoint)>100){Cancel();return;}
    if(!ContactStarted && !BeginContact()){if(G)G->SetFeedback(TEXT("请站到操作面前，留出弟弟的站位；坡面或障碍会中止操作"));Cancel();return;}
    if(Kind==EHearthwardDeviceKind::Winch && Phase==EHearthwardDevicePhase::Hold)
    {
        if(LinkedGate.IsValid() && LinkedGate->IsOpen()){PersistOpen();Cancel();}return;
    }
    Elapsed+=Delta;
    if(Kind==EHearthwardDeviceKind::Beam)
    {
        if(!BeamPathClear()){if(G)G->SetFeedback(TEXT("横梁移动或承托区域被挡住，已放回原位"));Cancel();return;}
        const FVector Next=BeamPosition(FMath::Clamp(Elapsed/3.f,0.f,1.f));
        FVector DeltaPosition=Next-Mesh->GetRelativeLocation();DeltaPosition.Z=0;
        for(AActor* Actor:{User.Get(),static_cast<AActor*>(Brother.Get())})
            if(auto* Contact=Actor->FindComponentByClass<UHearthwardInteractionAlignment>())
                if(!Contact->FollowMovingTarget(this,GetActorRotation().RotateVector(DeltaPosition)))
                {if(G)G->SetFeedback(TEXT("搬运站位前方受阻，已将横梁放回承托处"));Cancel();return;}
        Mesh->SetRelativeLocation(Next);StartPosition=User->GetActorLocation();
    }
    if(Elapsed<(Kind==EHearthwardDeviceKind::Beam?3.f:1.2f))return;
    if(Kind==EHearthwardDeviceKind::Winch)
    {
        if(!LinkedGate.IsValid()){Cancel();return;}
        Phase=EHearthwardDevicePhase::Hold;LinkedGate->TemporaryOpen=true;LinkedGate->TemporaryHolder=this;LinkedGate->ApplyState();return;
    }
    if(PersistOpen()){Phase=EHearthwardDevicePhase::Commit;ApplyState();}
    Cancel();
}
