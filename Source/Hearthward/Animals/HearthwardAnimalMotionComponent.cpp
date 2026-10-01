#include "HearthwardAnimalMotionComponent.h"
#include "HearthwardAnimalAnimInstance.h"
#include "HearthwardAnimalBounds.h"
#include "../Nature/HearthwardNatureActor.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

namespace
{
TSharedPtr<FJsonObject> AnimalCatalog()
{
    static TSharedPtr<FJsonObject> Data;
    if(!Data)
    {
        FString Text;
        if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("Resources/Data/animal_motion.json")))
            || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Data))
            UE_LOG(LogTemp,Error,TEXT("Animal motion catalog could not be loaded"));
    }
    return Data;
}
}
UHearthwardAnimalMotionComponent::UHearthwardAnimalMotionComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
}
bool UHearthwardAnimalMotionComponent::Configure(FName InSpecies,float Scale)
{
    Species=InSpecies==TEXT("deer")?FName(TEXT("stag_a")):InSpecies==TEXT("boar")?FName(TEXT("pig")):InSpecies;
    const auto Data=AnimalCatalog();if(!Data)return false;
    for(const auto& V:Data->GetArrayField(TEXT("species")))
        if(V->AsObject()->GetStringField(TEXT("slug"))==Species.ToString()){Profile=V->AsObject();break;}
    if(!Profile){UE_LOG(LogTemp,Error,TEXT("Animal profile missing: %s"),*Species.ToString());return false;}
    auto* Asset=LoadObject<USkeletalMesh>(nullptr,*Profile->GetStringField(TEXT("mesh")));
    if(!Asset)return false;
    for(const auto& Entry:Profile->GetObjectField(TEXT("clips"))->Values)
    {
        const auto C=Entry.Value->AsObject();const FName Name(*Entry.Key);
        auto* Clip=LoadObject<UAnimSequence>(nullptr,*C->GetStringField(TEXT("asset")));
        if(!Clip || Clip->GetSkeleton()!=Asset->GetSkeleton())
        {UE_LOG(LogTemp,Error,TEXT("Animal paired skeleton mismatch: %s/%s"),*Species.ToString(),*Entry.Key);return false;}
        Clips.Add(Name,Clip);ReferenceSpeeds.Add(Name,C->GetNumberField(TEXT("reference_speed_cm_s")));
        if(C->GetBoolField(TEXT("loop")))Looping.Add(Name);
    }
    DisplayName=Profile->GetStringField(TEXT("name"));Aquatic=Profile->GetBoolField(TEXT("aquatic"));
    Idle=FName(*Profile->GetStringField(TEXT("idle")));Walk=FName(*Profile->GetStringField(TEXT("walk")));
    Run=FName(*Profile->GetStringField(TEXT("run")));Start=FName(*Profile->GetStringField(TEXT("start")));
    Stop=FName(*Profile->GetStringField(TEXT("stop")));Alert=FName(*Profile->GetStringField(TEXT("alert")));
    Hit=FName(*Profile->GetStringField(TEXT("hit")));Collapse=FName(*Profile->GetStringField(TEXT("collapse")));
    Corpse=FName(*Profile->GetStringField(TEXT("corpse")));
    DetectRadius=Profile->GetNumberField(TEXT("detect_radius_cm"));WalkSpeed=Profile->GetNumberField(TEXT("walk_speed_cm_s"));RunSpeed=Profile->GetNumberField(TEXT("run_speed_cm_s"));
    // Fish exports carry zero reference speed. Keep an explicit presentation
    // reference separate from the newly balanced actor movement speed.
    ReferenceSpeeds.Add(Walk,Profile->GetNumberField(TEXT("walk_reference_speed_cm_s")));
    ReferenceSpeeds.Add(Run,Profile->GetNumberField(TEXT("run_reference_speed_cm_s")));
    Cycles.Reset();
    for(const auto& Plan:Profile->GetArrayField(TEXT("natural_cycles")))
    {
        TArray<FHearthwardAnimalActionStep> Steps;
        for(const auto& V:Plan->AsArray())
        {
            const auto S=V->AsObject();FHearthwardAnimalActionStep Step;
            Step.Clip=FName(*S->GetStringField(TEXT("clip")));Step.MinSeconds=S->GetNumberField(TEXT("min_seconds"));Step.MaxSeconds=S->GetNumberField(TEXT("max_seconds"));Steps.Add(Step);
        }
        Cycles.Add(Steps);
    }
    if(!Mesh)
    {
        Mesh=NewObject<USkeletalMeshComponent>(GetOwner(),TEXT("AnimalAnimatedMesh"));GetOwner()->AddInstanceComponent(Mesh);
        Mesh->SetupAttachment(GetOwner()->GetRootComponent());Mesh->SetAbsolute(false,false,true);Mesh->RegisterComponent();
    }
    Mesh->SetSkeletalMesh(Asset);Mesh->SetWorldScale3D(FVector(Scale));Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionObjectType(ECC_WorldDynamic);Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);Mesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Mesh->SetCanEverAffectNavigation(false);Mesh->ComponentTags.AddUnique(TEXT("body"));Mesh->CastShadow=true;
    Mesh->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Mesh->bEnableUpdateRateOptimizations=false;
    Home=Previous=GetOwner()->GetActorLocation();Goal=Home;
    Random.Initialize(int32(GetTypeHash(Species)^GetTypeHash(GetOwner()->GetName())));
    CycleIndex=Random.RandRange(0,FMath::Max(0,Cycles.Num()-1));
    FVector Floor;if(!Aquatic && Ground(Home,Floor))RootHeight=Home.Z-Floor.Z;
    BodyRadius=Asset->GetBounds().BoxExtent.Size2D()*Scale*1.15f+45;
    SetHabitat(FBox(Home-FVector(1800,1800,400),Home+FVector(1800,1800,400)));
    if(auto* Nature=Cast<AHearthwardNatureActor>(GetOwner()))
    {
        auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        const auto* A=N->State.Animals.FindByPredicate([&](const auto& X){return X.Id==Nature->Id;});
        const auto* Slot=A?N->State.Slots.FindByPredicate([&](const auto& X){return X.Id==A->Slot;}):nullptr;
        if(Slot){Home=Slot->Position;Home.Z=Previous.Z;SetHabitat(FBox(Home-FVector(1800,1800,400),Home+FVector(1800,1800,400)));}
    }
    if(auto* C=GetOwner()->FindComponentByClass<UHearthwardCombatTargetComponent>())
    {
        PreviousHealth=C->Health;
        for(int32 I=0;I<Mesh->GetNumBones();++I)
        {
            const FName Bone=Mesh->GetBoneName(I);const FString Name=Bone.ToString().ToLower();
            C->BoneParts.Add(Bone,Name.Contains(TEXT("head"))?FName(TEXT("head")):Name.Contains(TEXT("leg"))?FName(TEXT("legs")):FName(TEXT("body")));
        }
    }
    Enter(EPhase::Idle,Idle,Random.FRandRange(1.5f,4.f));Mesh->SetAnimInstanceClass(UHearthwardAnimalAnimInstance::StaticClass());
    AlignMesh();AddTickPrerequisiteActor(GetOwner());return true;
}
void UHearthwardAnimalMotionComponent::SetHabitat(const FBox& InRegion,bool IsDemo)
{
    Region=InRegion;Demo=IsDemo;SafeBounds=Region;
    SafeBounds.Min.X+=BodyRadius;SafeBounds.Min.Y+=BodyRadius;SafeBounds.Max.X-=BodyRadius;SafeBounds.Max.Y-=BodyRadius;
    if(Aquatic && Mesh)
    {
        const auto B=Mesh->GetSkeletalMeshAsset()->GetBounds().GetBox();const float S=Mesh->GetComponentScale().Z;
        SafeBounds.Min.Z-=B.Min.Z*S-25;SafeBounds.Max.Z-=B.Max.Z*S+25;
    }
    for(int32 Axis=0;Axis<3;++Axis)if(SafeBounds.Min[Axis]>SafeBounds.Max[Axis])SafeBounds.Min[Axis]=SafeBounds.Max[Axis]=Region.GetCenter()[Axis];
    Goal=HearthwardAnimalBounds::Clamp(SafeBounds,Goal);
}
bool UHearthwardAnimalMotionComponent::ControlsNatureMovement() const
{
    if(!Ready())return false;
    const auto* Nature=Cast<AHearthwardNatureActor>(GetOwner());if(!Nature)return true;
    const auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
    const auto* A=N->State.Animals.FindByPredicate([&](const auto& X){return X.Id==Nature->Id;});
    return A && !A->Captured && !N->WorkingOn(Nature->Id);
}
void UHearthwardAnimalMotionComponent::SetClip(FName Clip,float Seconds)
{
    if(!Clips.Contains(Clip))Clip=Idle;
    ActiveClip=Clip;PlayRate=1;++StateRevision;
    Remaining=Seconds>0?Seconds:Clips[Clip]->GetPlayLength();PhaseDuration=Remaining;
}
void UHearthwardAnimalMotionComponent::Enter(EPhase Next,FName Clip,float Seconds)
{
    Phase=Next;SetClip(Clip,Seconds);
    const TCHAR* Names[]={TEXT("静候"),TEXT("缓游 / 缓步"),TEXT("自然行为"),TEXT("察觉靠近"),TEXT("起步逃离"),TEXT("快速逃离"),TEXT("减速收步"),TEXT("受击"),TEXT("倒地 / 沉降"),TEXT("死亡保持")};
    Behavior=Names[int32(Next)];
}
void UHearthwardAnimalMotionComponent::BeginWalk()
{
    Queue.Reset();Goal=HearthwardAnimalBounds::Clamp(SafeBounds,Home+FVector(Random.FRandRange(-650.f,650.f),Random.FRandRange(-650.f,650.f),0));
    if(Aquatic)Goal.Z=Random.FRandRange(SafeBounds.Min.Z,SafeBounds.Max.Z);
    Enter(EPhase::Walking,Walk,Random.FRandRange(6.f,11.f));
}
void UHearthwardAnimalMotionComponent::BeginCycle()
{
    if(Cycles.IsEmpty()){Enter(EPhase::Idle,Idle,3);return;}
    Queue=Cycles[CycleIndex];CycleIndex=(CycleIndex+1)%Cycles.Num();++NaturalCycles;
    const auto Step=Queue[0];Queue.RemoveAt(0);Enter(EPhase::Natural,Step.Clip,Step.MaxSeconds>0?Random.FRandRange(Step.MinSeconds,Step.MaxSeconds):0);
}
void UHearthwardAnimalMotionComponent::BeginFlee()
{
    Queue.Reset();CalmTime=0;ReplanTime=0;
    Goal=HearthwardAnimalBounds::FleeGoal(SafeBounds,GetOwner()->GetActorLocation(),ThreatPosition,GetOwner()->GetActorForwardVector());
    if(Clips.Contains(Start))Enter(EPhase::Starting,Start);else Enter(EPhase::Fleeing,Run,3600);
}
float UHearthwardAnimalMotionComponent::EscapeSpeed() const
{
    const auto* Player=UGameplayStatics::GetPlayerPawn(this,0);
    const auto* Gameplay=Player?Player->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    // Keep the small speed advantage after sprint skills, without changing speed
    // according to pursuit distance or whether Shift is currently held.
    return RunSpeed*(1.f+(Gameplay?FMath::Max(0.f,Gameplay->Effect(TEXT("sprint"))):0.f));
}
void UHearthwardAnimalMotionComponent::AdvancePhase()
{
    switch(Phase)
    {
    case EPhase::Idle:BeginWalk();break;
    case EPhase::Walking:Enter(EPhase::Stopping,Stop);break;
    case EPhase::Stopping:BeginCycle();break;
    case EPhase::Natural:
        if(!Queue.IsEmpty())
        {const auto S=Queue[0];Queue.RemoveAt(0);Enter(EPhase::Natural,S.Clip,S.MaxSeconds>0?Random.FRandRange(S.MinSeconds,S.MaxSeconds):0);}
        else BeginWalk();break;
    case EPhase::Alert:BeginFlee();break;
    case EPhase::Starting:Enter(EPhase::Fleeing,Run,3600);break;
    case EPhase::Hit:BeginFlee();break;
    case EPhase::Falling:Enter(EPhase::Dead,Corpse,3600);break;
    default:break;
    }
}
bool UHearthwardAnimalMotionComponent::Ground(FVector P,FVector& Result) const
{
    TArray<FHitResult> Hits;FCollisionQueryParams Q(SCENE_QUERY_STAT(AnimalGround),false,GetOwner());
    GetWorld()->LineTraceMultiByObjectType(Hits,P+FVector(0,0,1500),P-FVector(0,0,3000),FCollisionObjectQueryParams(ECC_WorldStatic),Q);
    for(const auto& H:Hits)if(H.GetActor() && (H.GetActor()->ActorHasTag(TEXT("Hearthward.NatureGround")) || H.GetActor()->GetClass()->GetName().Contains(TEXT("Landscape"))) && H.ImpactNormal.Z>.65)
    {Result=H.ImpactPoint;return true;}
    return false;
}
void UHearthwardAnimalMotionComponent::AlignMesh()
{
    if(!Mesh)return;
    FVector P=GetOwner()->GetActorLocation(),Floor;
    if(!Aquatic && Ground(P,Floor))P.Z=Floor.Z+2;
    Mesh->SetWorldLocation(P);
    if(const auto* Nature=Cast<AHearthwardNatureActor>(GetOwner()))
    {
        const auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        const auto* A=N->State.Animals.FindByPredicate([&](const auto& X){return X.Id==Nature->Id;});
        Mesh->SetWorldScale3D(FVector(A && A->Juvenile?.55f:1.f));
    }
}
AActor* UHearthwardAnimalMotionComponent::NearestThreat(float& Distance) const
{
    AActor* Best=UGameplayStatics::GetPlayerPawn(this,0);Distance=FLT_MAX;
    if(Best)
    {
        if(const auto* S=Best->FindComponentByClass<UHearthwardSurvivalComponent>();S && !S->Alive())Best=nullptr;
        else Distance=FVector::Dist2D(Best->GetActorLocation(),GetOwner()->GetActorLocation());
    }
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        if(const auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>();S && !S->Alive())continue;
        const float D=FVector::Dist2D(It->GetActorLocation(),GetOwner()->GetActorLocation());
        if(D<Distance){Distance=D;Best=*It;}
    }
    return Best;
}
void UHearthwardAnimalMotionComponent::Move(float Delta,float Speed)
{
    const FVector P=GetOwner()->GetActorLocation();const FVector To=Goal-P;
    if(To.Size2D()<15 || Speed<1)return;
    FVector Desired=To.GetSafeNormal2D();
    for(TActorIterator<AActor> It(GetWorld());It;++It)
    {
        if(*It==GetOwner())continue;
        const auto* Other=It->FindComponentByClass<UHearthwardAnimalMotionComponent>();if(!Other || !Other->Ready() || Other->Aquatic!=Aquatic)continue;
        const FVector D=P-It->GetActorLocation();const float Range=BodyRadius+Other->BodyRadius+25;
        if(D.Size2D()<Range && D.Size2D()>1)Desired+=D.GetSafeNormal2D()*(1-D.Size2D()/Range)*2;
    }
    Desired=Desired.GetSafeNormal2D();
    const FRotator Yaw=FMath::RInterpConstantTo(FRotator(0,GetOwner()->GetActorRotation().Yaw,0),Desired.Rotation(),Delta,Aquatic?210.f:135.f);
    GetOwner()->SetActorRotation(Yaw);
    FVector Next=P+GetOwner()->GetActorForwardVector()*FMath::Min(Speed*Delta,float(To.Size2D()));
    if(Aquatic)Next.Z=FMath::FInterpConstantTo(P.Z,Goal.Z,Delta,20.f);
    const FVector Limited=HearthwardAnimalBounds::Clamp(SafeBounds,Next);
    if(!Limited.Equals(Next,.001)){++BoundaryCorrections;ReplanTime=0;}
    Next=Limited;
    if(!Aquatic)
    {FVector Floor;if(!Ground(Next,Floor))return;Next.Z=Floor.Z+RootHeight;}
    GetOwner()->SetActorLocation(Next,true);
}
void UHearthwardAnimalMotionComponent::OnFatalDamage()
{
    if(!Ready() || Dead)return;
    Dead=true;Queue.Reset();GroundSpeed=0;CalmTime=0;
    GetOwner()->SetActorRotation(FRotator(0,GetOwner()->GetActorRotation().Yaw,0));
    Enter(EPhase::Falling,Collapse);
}
void UHearthwardAnimalMotionComponent::ResetAnimal()
{
    Dead=false;Queue.Reset();CalmTime=0;LastThreat=NAME_None;GroundSpeed=0;
    GetOwner()->SetActorLocation(Home,false,nullptr,ETeleportType::TeleportPhysics);Previous=Home;
    if(auto* C=GetOwner()->FindComponentByClass<UHearthwardCombatTargetComponent>())
    {C->Health=C->MaximumHealth;C->DamageIds.Reset();C->Memory.HitRemaining=0;PreviousHealth=C->Health;}
    Enter(EPhase::Idle,Idle,Random.FRandRange(1.5f,4));AlignMesh();
}
void UHearthwardAnimalMotionComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Delta,Type,Tick);if(!Ready() || GetWorld()->IsPaused() || Delta<=0)return;
    auto* Combat=GetOwner()->FindComponentByClass<UHearthwardCombatTargetComponent>();
    if(Combat && Combat->Health<=0)OnFatalDamage();
    if(Combat && Combat->Health<PreviousHealth && Combat->Health>0)
    {
        ThreatPosition=UGameplayStatics::GetPlayerPawn(this,0)?UGameplayStatics::GetPlayerPawn(this,0)->GetActorLocation():Home;
        Queue.Reset();CalmTime=0;Enter(EPhase::Hit,Hit);
    }
    if(Combat)PreviousHealth=Combat->Health;
    const bool External=!ControlsNatureMovement();
    if(!Dead && External)
    {
        GroundSpeed=FVector::Dist2D(Previous,GetOwner()->GetActorLocation())/Delta;
        FName Wanted=GroundSpeed>WalkSpeed*1.8?Run:GroundSpeed>3?Walk:Clips.Contains(TEXT("CapturedIdle"))?FName(TEXT("CapturedIdle")):Idle;
        if(ActiveClip!=Wanted)SetClip(Wanted,3600);
        Behavior=GroundSpeed>3?TEXT("牵引跟随"):TEXT("照料 / 等待");WasExternal=true;
    }
    else
    {
        if(WasExternal && !Dead){WasExternal=false;Enter(EPhase::Idle,Idle,2);}
        Remaining-=Delta;
        float Distance;AActor* Threat=NearestThreat(Distance);
        if(!Dead && Threat && Distance<DetectRadius)
        {
            ThreatPosition=Threat->GetActorLocation();LastThreat=Cast<AHearthwardCompanionFixture>(Threat)?FName(TEXT("brother")):FName(TEXT("player"));CalmTime=0;
            if(Phase!=EPhase::Alert && Phase!=EPhase::Starting && Phase!=EPhase::Fleeing && Phase!=EPhase::Hit)
            {
                Queue.Reset();
                const FString Current=ActiveClip.ToString();
                const bool Lying=Current==TEXT("Rest") || Current==TEXT("CurlRest") || Current==TEXT("LieDown") || Current==TEXT("CurlDown");
                Enter(EPhase::Alert,Lying && Clips.Contains(TEXT("GetUp"))?FName(TEXT("GetUp")):Alert,Lying?0:.25f);
            }
        }
        if(!Dead && Phase==EPhase::Fleeing)
        {
            if(!Threat || Distance>DetectRadius*1.35f)CalmTime+=Delta;else CalmTime=0;
            ReplanTime-=Delta;
            if(ReplanTime<=0)
            {Goal=HearthwardAnimalBounds::FleeGoal(SafeBounds,GetOwner()->GetActorLocation(),ThreatPosition,GetOwner()->GetActorForwardVector());ReplanTime=.3f;}
            if(CalmTime>3)Enter(EPhase::Stopping,Stop);
        }
        if(Remaining<=0 && Phase!=EPhase::Dead)AdvancePhase();
        if(Dead)
        {
            if(Aquatic)
            {FVector P=GetOwner()->GetActorLocation();P.Z=FMath::FInterpConstantTo(P.Z,SafeBounds.Min.Z,Delta,30);GetOwner()->SetActorLocation(HearthwardAnimalBounds::Clamp(SafeBounds,P));}
            GroundSpeed=0;
        }
        else
        {
            const float FleeSpeed=EscapeSpeed();
            float Speed=Phase==EPhase::Walking?WalkSpeed:Phase==EPhase::Fleeing?FleeSpeed:Phase==EPhase::Starting?FleeSpeed*(1-FMath::Clamp(Remaining/FMath::Max(.01f,PhaseDuration),0.f,1.f)):0;
            if(Phase==EPhase::Stopping)Speed=FMath::Min(GroundSpeed,FleeSpeed)*FMath::Clamp(Remaining/FMath::Max(.01f,PhaseDuration),0.f,1.f);
            Move(Delta,Speed);GroundSpeed=FVector::Dist2D(Previous,GetOwner()->GetActorLocation())/Delta;
            if(Phase==EPhase::Walking || Phase==EPhase::Fleeing)
            {
                const FName Gait=Phase==EPhase::Walking?Walk:Run;
                const FName Wanted=GroundSpeed>2?Gait:Phase==EPhase::Fleeing?Alert:Idle;
                if(ActiveClip!=Wanted)SetClip(Wanted,Remaining);
                if(Phase==EPhase::Walking && FVector::Dist2D(GetOwner()->GetActorLocation(),Goal)<20)Remaining=0;
            }
            if(auto* Nature=Cast<AHearthwardNatureActor>(GetOwner()))
                if(auto* A=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->State.Animals.FindByPredicate([&](const auto& X){return X.Id==Nature->Id;}))A->Position=GetOwner()->GetActorLocation();
        }
    }
    if((ActiveClip==Walk || ActiveClip==Run) && GroundSpeed>0)
    {
        const float Reference=FMath::Max(1.f,GaitReferenceSpeed()>0?GaitReferenceSpeed():ActiveClip==Walk?WalkSpeed:RunSpeed);
        // Some paired gaits need more than 1.8x at the balanced escape speed.
        // Bound playback by the configured movement speed, preserving stride length.
        PlayRate=FMath::Clamp(GroundSpeed/Reference,.2f,FMath::Max(1.8f,EscapeSpeed()/Reference));
    }
    else PlayRate=1;
    AlignMesh();Previous=GetOwner()->GetActorLocation();
}
