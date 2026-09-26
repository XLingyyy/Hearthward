#include "HearthwardCombatComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Animation/HearthwardHeroAnimInstance.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "AIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "EngineUtils.h"
#include "HearthwardProjectile.h"
#include "HearthwardCombatRegion.h"

using namespace HearthwardData;
UHearthwardCombatComponent::UHearthwardCombatComponent() { PrimaryComponentTick.bCanEverTick=true; }
void UHearthwardCombatComponent::BeginPlay()
{
    Super::BeginPlay(); if(G()) AddTickPrerequisiteComponent(G());
}
UHearthwardGameplayComponent* UHearthwardCombatComponent::G() const { return GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>(); }
UHearthwardInventoryComponent* UHearthwardCombatComponent::Bag() const { return GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>(); }
double UHearthwardCombatComponent::Now() const { return GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds; }
FGuid UHearthwardCombatComponent::Epoch() const { return GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(); }
bool UHearthwardCombatComponent::Available() const
{
    const auto* S=GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>();
    return G() && G()->Enabled && S && S->Alive() && !GetWorld()->IsPaused() && !Committing;
}
TArray<UHearthwardCombatTargetComponent*> UHearthwardCombatComponent::Targets() const
{
    TArray<UHearthwardCombatTargetComponent*> Out;
    for(TActorIterator<AActor> It(GetWorld());It;++It)
        if(auto* T=It->FindComponentByClass<UHearthwardCombatTargetComponent>();T && !T->Id.IsNone()) Out.Add(T);
    Out.Sort([](const auto& A,const auto& B){ return A.Id.LexicalLess(B.Id); }); return Out;
}
bool UHearthwardCombatComponent::Visible(const AActor* From,const AActor* To) const
{
    FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(CombatSight),false,From); Q.AddIgnoredActor(To);
    return !GetWorld()->LineTraceSingleByChannel(Hit,From->GetActorLocation()+FVector(0,0,30),To->GetActorLocation()+FVector(0,0,30),ECC_Visibility,Q);
}
FName UHearthwardCombatComponent::WeaponKind() const
{
    const auto R=Find(TEXT("items"),G()->Equipment.FindRef(TEXT("weapon")).ToString());
    return FName(*Text(R,TEXT("combatClass")));
}
bool UHearthwardCombatComponent::Start(FName Name,double Seconds)
{
    if(!Available() || Busy() || Body.IsValid()) return false;
    auto* C=Cast<ACharacter>(GetOwner());
    if(!C || !C->GetCharacterMovement()->IsMovingOnGround()) return false;
    GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>()->CancelAction();
    if(auto* T=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>()) T->InterruptAction();
    Action=Name; Duration=Seconds; Elapsed=0; StartedAt=Now(); ActionEpoch=Epoch(); ActionId=FGuid::NewGuid();
    StartPosition=GetOwner()->GetActorLocation(); ActionWeapon=G()->Equipment.FindRef(TEXT("weapon"));
    HitIds.Reset(); Guard.Release(); G()->SetSprinting(false); return true;
}
bool UHearthwardCombatComponent::Attack(bool Heavy)
{
    if(Busy())
    {
        if(Action==TEXT("attack") && Duration-Elapsed<=.15) { BufferedAttack=Heavy?TEXT("heavy"):TEXT("light"); BufferedUntil=Now()+.15; }
        return false;
    }
    const FName Kind=WeaponKind(); if(Kind.IsNone() || G()->AttackPower()<=0) { Feedback=TEXT("需要可用近战武器"); return false; }
    Move=HearthwardCombat::Move(Kind,Heavy);
    if(!Available() || G()->Stamina<Move.Cost*Bag()->GetStaminaCostMultiplier()*FMath::Max(.1f,1-G()->Effect(TEXT("cost")))) return false;
    if(!Start(TEXT("attack"),Move.Duration())) return false;
    G()->SpendStamina(Move.Cost); HeavyAttack=Heavy;
    if(auto* C=Cast<ACharacter>(GetOwner())) if(auto* A=Cast<UHearthwardHeroAnimInstance>(C->GetMesh()->GetAnimInstance())) A->PlayCombat(float(Duration),false);
    return true;
}
bool UHearthwardCombatComponent::Eligible(UHearthwardCombatTargetComponent* T) const
{
    if(!T || !T->Alive() || (T->ExecutionOwner.IsValid() && T->ExecutionOwner!=GetOwner()) || T->Memory.Seen.Contains(TEXT("player"))) return false;
    const FVector Offset=GetOwner()->GetActorLocation()-T->GetOwner()->GetActorLocation();
    if(Offset.Size()>150 || !HearthwardCombat::InFront(-T->GetOwner()->GetActorForwardVector(),Offset) || !Visible(GetOwner(),T->GetOwner())) return false;
    if(auto* C=Cast<ACharacter>(T->GetOwner());C && !T->ExecutionOwner.IsValid() && !C->GetCharacterMovement()->IsMovingOnGround()) return false;
    if(G()->AttackPower()<=0 || WeaponKind().IsNone()) return false;
    const float Armor=T->ArmorDurability.FindRef(TEXT("body"))>0?T->Armor.FindRef(TEXT("body")):0;
    return !T->Heavy || HearthwardCombat::ArmorDamage(G()->AttackPower()*(WeaponKind()==TEXT("shortblade")?5:3),Armor)>=T->Health;
}
bool UHearthwardCombatComponent::Execute(AActor* Target)
{
    if(!Available() || Busy()) return false;
    UHearthwardCombatTargetComponent* Best=Target?Target->FindComponentByClass<UHearthwardCombatTargetComponent>():nullptr;
    if(!Target)
    {
        double D=DBL_MAX;
        for(auto* T:Targets()) if(Eligible(T))
        { const double N=FVector::DistSquared(GetOwner()->GetActorLocation(),T->GetOwner()->GetActorLocation()); if(N<D) { Best=T; D=N; } }
    }
    if(!Eligible(Best)) { Feedback=TEXT("处决需要贴近未发现你的敌人背后，重型目标须满足伤害门槛"); return false; }
    if(!Start(TEXT("execution"),3)) return false;
    Captive=Best; Best->ExecutionOwner=GetOwner(); CaptivePosition=Best->GetOwner()->GetActorLocation(); CaptiveRotation=Best->GetOwner()->GetActorRotation();
    auto* Player=CastChecked<ACharacter>(GetOwner()); Player->GetCharacterMovement()->StopMovementImmediately();
    Player->SetActorRotation(FRotator(0,(CaptivePosition-StartPosition).Rotation().Yaw,0));
    if(auto* C=Cast<ACharacter>(Best->GetOwner()))
    {
        if(auto* AI=Cast<AAIController>(C->GetController())) AI->StopMovement();
        CaptiveMovementMode=uint8(C->GetCharacterMovement()->MovementMode);
        C->GetCharacterMovement()->StopMovementImmediately(); C->GetCharacterMovement()->DisableMovement();
    }
    if(auto* A=Cast<UHearthwardHeroAnimInstance>(Player->GetMesh()->GetAnimInstance())) A->PlayCombat(3,true);
    Feedback=TEXT("处决中"); return true;
}
void UHearthwardCombatComponent::Cancel()
{
    if(Committing) return;
    if(auto* T=Captive.Get())
    {
        T->ExecutionOwner.Reset();
        if(auto* C=Cast<ACharacter>(T->GetOwner());C && T->Alive()) C->GetCharacterMovement()->SetMovementMode(EMovementMode(CaptiveMovementMode));
    }
    if(Action==TEXT("pickup")) DropBody();
    Captive.Reset(); Action=NAME_None; Elapsed=Duration=0; PendingItem=NAME_None; BufferedAttack=NAME_None;
    if(auto* C=Cast<ACharacter>(GetOwner())) if(auto* A=Cast<UHearthwardHeroAnimInstance>(C->GetMesh()->GetAnimInstance())) A->StopCombat();
}
bool UHearthwardCombatComponent::Dodge(FVector Direction)
{
    if(!Available() || Body.IsValid() || (Busy() && !(Action==TEXT("attack") && Elapsed>=Move.Windup+Move.Active))) return false;
    const float Cost=15*Bag()->GetStaminaCostMultiplier()*FMath::Max(.1f,1-G()->Effect(TEXT("cost")));
    if(G()->Stamina<Cost) return false;
    Cancel(); if(!Start(TEXT("dodge"),.55)) return false;
    G()->SpendStamina(15); DodgeDirection=Direction.GetSafeNormal2D();
    if(DodgeDirection.IsNearlyZero()) DodgeDirection=GetOwner()->GetActorForwardVector(); return true;
}
bool UHearthwardCombatComponent::SetGuard(bool Value)
{
    if(!Value) { Guard.Release(); return true; }
    const FName Kind=WeaponKind(),Shield=G()->Equipment.FindRef(TEXT("offhand"));
    if(!Available() || Busy() || Body.IsValid() || (Kind!=TEXT("shortblade") && Kind!=TEXT("blunt"))
        || Shield!=TEXT("shield") || G()->Durability.FindRef(Shield)<=0) return false;
    G()->SetSprinting(false); return Guard.Raise(Now(),G()->Stamina,G()->MaxStamina());
}
bool UHearthwardCombatComponent::Damage(float Raw,FName Part,FVector Source,bool Heavy,bool Projectile,FGuid Event)
{
    if(!G() || Raw<=0 || !FMath::IsFinite(Raw)) return false;
    if(Event.IsValid() && DamageIds.Contains(Event)) return false;
    if(!Event.IsValid()) Event=FGuid::NewGuid(); DamageIds.Add(Event);
    G()->NotifyCombat();
    if(Action==TEXT("dodge") && Now()-StartedAt<.3) return false;
    if((WeaponKind()==TEXT("shortblade") || WeaponKind()==TEXT("blunt")) && G()->Durability.FindRef(G()->Equipment.FindRef(TEXT("offhand")))>0
        && HearthwardCombat::InFront(GetOwner()->GetActorForwardVector(),Source-GetOwner()->GetActorLocation()))
    {
        const float Cost=(Heavy?40:20)*Bag()->GetStaminaCostMultiplier()*FMath::Max(.1f,1-G()->Effect(TEXT("cost")));
        if(Guard.Hit(Now(),Cost,G()->Stamina))
        { GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>()->State.RecoveryDelay=.5; return true; }
    }
    const FName Item=G()->Equipment.FindRef(Part==TEXT("body")?FName(TEXT("chest")):Part); const auto R=Find(TEXT("items"),Item.ToString());
    const float Armor=G()->Durability.FindRef(Item)>0?Number(R,TEXT("defense"))/100:0;
    const float Actual=HearthwardCombat::ArmorDamage(Raw*(Projectile && Part==TEXT("head")?3:1),Armor,G()->Effect(TEXT("defense")));
    Cancel(); DropBody(); Guard.Release();
    if(Armor>0) G()->Durability[Item]=FMath::Max(0.f,G()->Durability[Item]-1/(1+G()->Effect(TEXT("durability"))));
    auto* S=GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>();
    S->ReceiveDamage(Actual,Event,Epoch());
    if(S->Alive()) { Action=TEXT("hit"); Duration=.25; StartedAt=Now(); Elapsed=0; ActionEpoch=Epoch(); }
    G()->OnChanged.Broadcast(); return true;
}
void UHearthwardCombatComponent::ObserveDamage(UHearthwardCombatTargetComponent* T,AActor* Source)
{
    T->Memory.ReportRemaining=0; T->Memory.ReportingBody=NAME_None;
    if(!Source) Source=GetOwner(); const FName Who=Source==GetOwner()?TEXT("player"):TEXT("brother");
    T->Memory.Detection.Add(Who,1); T->Memory.LastKnown.Add(Who,Source->GetActorLocation());
}
void UHearthwardCombatComponent::HitTarget(UHearthwardCombatTargetComponent* T,float Raw,FName Part,bool Projectile,FGuid Event,AActor* Source)
{
    if(!T || !T->Alive() || Raw<=0 || T->DamageIds.Contains(Event) || Part.IsNone()) return;
    T->DamageIds.Add(Event); ObserveDamage(T,Source);
    if(auto* Owner=T->ExecutionOwner.Get()) if(auto* C=Owner->FindComponentByClass<UHearthwardCombatComponent>()) C->Cancel();
    const float Armor=T->ArmorDurability.FindRef(Part)>0?T->Armor.FindRef(Part):0;
    const float Actual=HearthwardCombat::ArmorDamage(Raw*(Projectile && Part==TEXT("head")?3:1),Armor);
    if(Armor>0) T->ArmorDurability[Part]=FMath::Max(0.f,T->ArmorDurability[Part]-1);
    const float PreviousHealth=T->Health;
    T->Health=FMath::Max(0.f,T->Health-Actual); T->Memory.HitRemaining=.25;
    G()->CommitOpponentHealth(T->Id,T->Health,PreviousHealth);
    if(T->Health<=0) T->SetCorpse();
}
void UHearthwardCombatComponent::Sweep(double From,double To)
{
    const double A=FMath::Max(From,Move.Windup),B=FMath::Min(To,Move.Windup+Move.Active);
    if(B<=A) return;
    // Subdivide the crossed window, including low-frame-rate steps; each target is charged once.
    const int32 Steps=FMath::Max(1,FMath::CeilToInt((B-A)/.015));
    for(int32 I=0;I<=Steps;++I)
    {
        const double P=(FMath::Lerp(A,B,double(I)/Steps)-Move.Windup)/Move.Active;
        const float Angle=WeaponKind()==TEXT("spear")?0:FMath::Lerp(-55.,55.,P);
        const FVector FromPoint=GetOwner()->GetActorLocation()+FVector(0,0,10);
        const FVector ToPoint=FromPoint+GetOwner()->GetActorForwardVector().RotateAngleAxis(Angle,FVector::UpVector)*Move.Reach;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(CombatWeapon),false,GetOwner());
        for(auto* T:Targets()) if(HitIds.Contains(T->Id)) Q.AddIgnoredActor(T->GetOwner());
        FHitResult Hit;
        if(GetWorld()->SweepSingleByChannel(Hit,FromPoint,ToPoint,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(5),Q))
        {
            auto* T=Hit.GetActor()?Hit.GetActor()->FindComponentByClass<UHearthwardCombatTargetComponent>():nullptr;
            if(T && !HitIds.Contains(T->Id))
            {
                HitIds.Add(T->Id); const FName Part=T->HitPart(Hit);
                if(Part.IsNone()) { Feedback=TEXT("目标缺少命中部位配置"); continue; }
                float SkillScale=1; bool Stun=false;
                const auto Skill=Find(TEXT("skills"),TEXT("strong")); const int32 Rank=G()->Skills.FindRef(TEXT("strong"));
                if(HeavyAttack && Skill && Rank>0)
                {
                    const auto& Multipliers=Skill->GetArrayField(TEXT("multipliers")); const auto& Chance=Skill->GetArrayField(TEXT("stunChance"));
                    if(Multipliers.IsValidIndex(Rank-1)) SkillScale=Multipliers[Rank-1]->AsNumber();
                    if(Chance.IsValidIndex(Rank-1)) Stun=FMath::FRand()<Chance[Rank-1]->AsNumber();
                }
                HitTarget(T,G()->AttackPower()*Move.Multiplier*SkillScale,Part,false,ActionId);
                if(Stun && T->Alive()) { const float Before=T->Health; T->Health=0; G()->CommitOpponentHealth(T->Id,0,Before); T->SetCorpse(); }
                if(WeaponKind()!=TEXT("longblade")) return;
            }
        }
    }
}
void UHearthwardCombatComponent::Finish()
{
    const FName Completed=Action;
    if(Completed==TEXT("execution"))
    {
        auto* T=Captive.Get();
        if(!Eligible(T)) { Cancel(); return; }
        TGuardValue<bool> Commit(Committing,true);
        const float Before=T->Health; T->Health=0; G()->CommitOpponentHealth(T->Id,0,Before); T->SetCorpse();
    }
    else if(Completed==TEXT("pickup")) Action=NAME_None;
    else if(Completed==TEXT("reload")) CrossbowLoaded=true;
    else if(Completed==TEXT("switch")) G()->CommitEquipment(PendingItem);
    Cancel();
}
bool UHearthwardCombatComponent::MovementLocked() const
{ return Busy() && Action!=TEXT("draw"); }
float UHearthwardCombatComponent::MovementMultiplier() const { return Body.IsValid()?(CarryingOnBack?.7f:.5f):1; }
bool UHearthwardCombatComponent::CanSave() const { return !Busy() && !Body.IsValid() && !Committing; }
FString UHearthwardCombatComponent::Describe() const
{
    if(Executing()) return FString::Printf(TEXT("处决 %.1f / 3.0 秒"),Elapsed);
    if(Guard.Held) return TEXT("格挡中");
    if(Body.IsValid()) return TEXT("搬运尸体 · E 放下");
    if(Busy())
    {
        const FString Label=Action==TEXT("attack")?TEXT("攻击"):Action==TEXT("dodge")?TEXT("闪避"):Action==TEXT("draw")?TEXT("蓄力"):
            Action==TEXT("switch")?TEXT("换装"):Action==TEXT("reload")?TEXT("装填"):Action==TEXT("pickup")?TEXT("搬起"):
            Action==TEXT("throw")?TEXT("投掷"):Action==TEXT("hit")?TEXT("受击"):TEXT("收招");
        return FString::Printf(TEXT("%s %.1f 秒"),*Label,Elapsed);
    }
    return Feedback;
}
void UHearthwardCombatComponent::TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Tick,Function);
    if(!G() || !G()->Enabled || GetWorld()->IsPaused()) return;
    if(!GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>()->Alive()) { Cancel(); DropBody(); Guard.Release(); State.SenseRemaining=0; return; }
    State.SenseRemaining=FMath::Max(0.,State.SenseRemaining-Delta); State.SenseCooldown=FMath::Max(0.,State.SenseCooldown-Delta);
    if(ActionEpoch.IsValid() && ActionEpoch!=Epoch()) { Cancel(); DropBody(); DamageIds.Reset(); }
    Perception(Delta); UpdateCarry(Delta);
    if(Locked.IsValid())
    {
        const auto* T=Locked->FindComponentByClass<UHearthwardCombatTargetComponent>();
        LostLock=Visible(GetOwner(),Locked.Get())?0:LostLock+Delta;
        if(!T || !T->Alive() || FVector::Dist(GetOwner()->GetActorLocation(),Locked->GetActorLocation())>2500 || LostLock>=1) Locked.Reset();
        else if(auto* C=Cast<APawn>(GetOwner());C && C->GetController()) C->GetController()->SetControlRotation((Locked->GetActorLocation()-GetOwner()->GetActorLocation()).Rotation());
    }
    if(!Busy()) return;
    const double Previous=Elapsed; Elapsed=FMath::Min(Duration,Now()-StartedAt);
    if(Action==TEXT("execution"))
    {
        if(!Eligible(Captive.Get()) || FVector::DistSquared(StartPosition,GetOwner()->GetActorLocation())>25 || Discovery>=1) { Cancel(); return; }
        Captive->GetOwner()->SetActorLocationAndRotation(CaptivePosition,CaptiveRotation,false,nullptr,ETeleportType::TeleportPhysics);
    }
    if((Action==TEXT("throw") && Previous<.25 && Elapsed>=.25) || (Action==TEXT("crossbow") && Previous<.1 && Elapsed>=.1))
    {
        const bool Thrown=Action==TEXT("throw"); const FName Item=Thrown?PendingItem:G()->Equipment.FindRef(TEXT("ranged"));
        const auto R=Find(TEXT("items"),Item.ToString()); const float Cost=Thrown?(Number(R,TEXT("bait"))>0?5:10):8;
        const FName Ammo=Thrown?Item:FName(TEXT("arrow"));
        if(Bag()->Available(Ammo)<=0 || !G()->SpendStamina(Cost)) { Cancel(); return; }
        Bag()->TryRemove(Ammo,1); LaunchProjectile(Item,1,Thrown); if(!Thrown) CrossbowLoaded=false;
    }
    if(Action==TEXT("attack"))
    {
        if(G()->Equipment.FindRef(TEXT("weapon"))!=ActionWeapon) { Cancel(); return; }
        if(WeaponKind()==TEXT("longblade") || HitIds.IsEmpty()) Sweep(Previous,Elapsed);
    }
    if(Action==TEXT("dodge"))
    {
        FHitResult Hit; GetOwner()->AddActorWorldOffset(DodgeDirection*(250*(Elapsed-Previous)/.55),true,&Hit);
    }
    if(Elapsed>=Duration)
    {
        const FName Next=Now()<=BufferedUntil?BufferedAttack:NAME_None;
        Finish(); if(!Next.IsNone()) Attack(Next==TEXT("heavy"));
    }
}

bool UHearthwardCombatComponent::Sense()
{
    if(!Available() || State.SenseCooldown>0 || !G()->SpendStamina(20)) return false;
    State.SenseRemaining=5; State.SenseCooldown=20; return true;
}
TArray<AActor*> UHearthwardCombatComponent::SensedTargets() const
{
    TArray<AActor*> Out; if(State.SenseRemaining<=0) return Out;
    for(auto* T:Targets()) if(T->Alive() && HearthwardCombat::SenseVisible(GetOwner()->GetActorLocation(),T->GetOwner()->GetActorLocation())) Out.Add(T->GetOwner());
    return Out;
}
bool UHearthwardCombatComponent::ToggleLock()
{
    if(Locked.IsValid()) { Locked.Reset(); return true; }
    if(!Available()) return false;
    double Closest=FMath::Square(2000.);
    for(auto* T:Targets()) if(T->Alive() && Visible(GetOwner(),T->GetOwner()))
    { const double D=FVector::DistSquared(GetOwner()->GetActorLocation(),T->GetOwner()->GetActorLocation()); if(D<Closest) { Locked=T->GetOwner(); Closest=D; } }
    LostLock=0; return Locked.IsValid();
}
void UHearthwardCombatComponent::Perception(double Delta)
{
    const auto All=Targets();
    TArray<AHearthwardCombatRegion*> Regions;
    for(TActorIterator<AHearthwardCombatRegion> It(GetWorld());It;++It) Regions.Add(*It);
    Regions.Sort([](const auto& A,const auto& B){return A.RegionId.LexicalLess(B.RegionId);});
    for(auto* T:All) for(auto* R:Regions) if(R->Contains(T->GetOwner()->GetActorLocation())) { T->Region=R->RegionId; break; }
    TMap<FName,AActor*> Brothers; Brothers.Add(TEXT("player"),GetOwner());
    for(TActorIterator<AActor> It(GetWorld());It;++It)
        if(*It!=GetOwner() && It->FindComponentByClass<UHearthwardSurvivalComponent>()) { Brothers.Add(TEXT("brother"),*It); break; }
    Discovery=0; TSet<FName> EngagedRegions;
    const double Hour=FMath::Fmod(GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes/60,24.);
    for(auto* Observer:All)
    {
        Observer->Memory.HitRemaining=FMath::Max(0.,Observer->Memory.HitRemaining-Delta);
        if(!Observer->CanAct()) continue;
        Observer->Memory.Seen.Reset(); bool Engaged=false;
        for(const auto& Brother:Brothers)
        {
            const auto* S=Brother.Value->FindComponentByClass<UHearthwardSurvivalComponent>(); if(!S || !S->Alive()) continue;
            const FVector Offset=Brother.Value->GetActorLocation()-Observer->GetOwner()->GetActorLocation();
            double Light=Observer->Exposure>=0?FMath::Clamp(Observer->Exposure,0.f,1.f):(Hour>=6 && Hour<18?1:0);
            for(auto* R:Regions) if(R->Lighting>=0 && R->Contains(Brother.Value->GetActorLocation())) { Light=FMath::Clamp(R->Lighting,0.f,1.f); break; }
            const bool Seeing=Offset.Size()<=(12+13*Light)*100 && HearthwardCombat::InFront(Observer->GetOwner()->GetActorForwardVector(),Offset) && Visible(Observer->GetOwner(),Brother.Value);
            auto& P=Observer->Memory.Detection.FindOrAdd(Brother.Key);
            P=HearthwardCombat::Detection(P,Delta,Offset.Size()/100,Seeing);
            if(Seeing) Observer->Memory.LastKnown.Add(Brother.Key,Brother.Value->GetActorLocation());
            if(Seeing && P>=1) { Observer->Memory.Seen.Add(Brother.Key); Engaged=true; }
            if(Brother.Key==TEXT("player")) Discovery=FMath::Max(Discovery,P);
        }
        if(Engaged) { Observer->Awareness=TEXT("战斗"); EngagedRegions.Add(Observer->Region); G()->NotifyCombat(); }
        else
        {
            bool Suspicious=false; for(const auto& P:Observer->Memory.Detection) Suspicious|=P.Value>0;
            Observer->Awareness=Suspicious?TEXT("调查／搜索"):TEXT("巡逻");
        }
        Observer->Memory.InvestigationRemaining=FMath::Max(0.,Observer->Memory.InvestigationRemaining-Delta);
        if(!Engaged && Observer->Memory.InvestigationRemaining>0) Observer->Awareness=TEXT("调查诱饵");
        if(auto* Pawn=Cast<APawn>(Observer->GetOwner())) if(auto* AI=Cast<AAIController>(Pawn->GetController()))
        {
            FVector Goal=Observer->Memory.Investigation;
            if(Engaged) { for(const auto& B:Brothers) if(Observer->Memory.Seen.Contains(B.Key)) { Goal=B.Value->GetActorLocation(); break; } }
            else if(Observer->Memory.InvestigationRemaining<=0 && Observer->Memory.LastKnown.Contains(TEXT("player"))) Goal=Observer->Memory.LastKnown[TEXT("player")];
            bool Allowed=false; for(auto* R:Regions) if(R->RegionId==Observer->Region && R->Contains(Goal)) Allowed=true;
            if(Allowed && (Engaged || Observer->Memory.InvestigationRemaining>0 || !Observer->Memory.LastKnown.IsEmpty())) AI->MoveToLocation(Goal,100);
            else AI->StopMovement();
        }
        if(Engaged) { Observer->Memory.ReportingBody=NAME_None; Observer->Memory.ReportRemaining=0; continue; }
        UHearthwardCombatTargetComponent* Corpse=nullptr;
        for(auto* BodyTarget:All)
            if(BodyTarget->Health<=0 && !BodyTarget->Memory.BroadcastRegions.Contains(Observer->Region)
                && FVector::Dist(Observer->GetOwner()->GetActorLocation(),BodyTarget->GetOwner()->GetActorLocation())<=2500
                && HearthwardCombat::InFront(Observer->GetOwner()->GetActorForwardVector(),BodyTarget->GetOwner()->GetActorLocation()-Observer->GetOwner()->GetActorLocation())
                && Visible(Observer->GetOwner(),BodyTarget->GetOwner())) { Corpse=BodyTarget; break; }
        if(!Corpse) { Observer->Memory.ReportingBody=NAME_None; Observer->Memory.ReportRemaining=0; continue; }
        if(Observer->Memory.ReportingBody!=Corpse->Id) { Observer->Memory.ReportingBody=Corpse->Id; Observer->Memory.ReportRemaining=3; }
        else Observer->Memory.ReportRemaining=FMath::Max(0.,Observer->Memory.ReportRemaining-Delta);
        Observer->Awareness=TEXT("正在报警");
        if(Observer->Memory.ReportRemaining==0)
        {
            Corpse->Memory.BroadcastRegions.Add(Observer->Region); State.Alarms.Add(Observer->Region,Now()+120);
            Observer->Memory.ReportingBody=NAME_None;
        }
    }
    TArray<FName> Clear;
    for(const auto& A:State.Alarms) if(Now()>=A.Value && !EngagedRegions.Contains(A.Key)) Clear.Add(A.Key);
    for(FName Id:Clear) State.Alarms.Remove(Id);
    bool Inside=false;
    for(const auto& B:Brothers)
    {
        for(auto* R:Regions) if(R->Contains(B.Value->GetActorLocation())) Inside=true;
        for(auto* T:All) if(T->PrototypeEncounter && FVector::DistSquared(B.Value->GetActorLocation(),T->GetOwner()->GetActorLocation())<=FMath::Square(2500.)) Inside=true;
    }
    if(Inside || !EngagedRegions.IsEmpty()) State.OutsideSeconds=0;
    else if(State.OutsideSeconds<30)
    { State.OutsideSeconds=FMath::Min(30.,State.OutsideSeconds+Delta); if(State.OutsideSeconds==30) ++State.Infiltration; }
}
bool UHearthwardCombatComponent::Carry(bool Back)
{
    if(Body.IsValid()) { DropBody(); return true; }
    if(!Available() || Busy()) return false;
    double Best=FMath::Square(150.); UHearthwardCombatTargetComponent* Found=nullptr;
    for(auto* T:Targets()) if(T->Health<=0 && !T->Carrier.IsValid() && Visible(GetOwner(),T->GetOwner()))
    { const double D=FVector::DistSquared(GetOwner()->GetActorLocation(),T->GetOwner()->GetActorLocation()); if(D<Best) { Found=T; Best=D; } }
    if(!Found || !Start(TEXT("pickup"),.5)) return false;
    Body=Found; Found->Carrier=GetOwner(); CarryingOnBack=Back; return true;
}
void UHearthwardCombatComponent::DropBody()
{
    if(Body.IsValid())
    {
        auto* Actor=Body->GetOwner(); Body->Carrier.Reset();
        FCollisionQueryParams Q(SCENE_QUERY_STAT(DropBody),false,Actor); Q.AddIgnoredActor(GetOwner()); FHitResult Hit;
        const FVector P=Actor->GetActorLocation();
        if(GetWorld()->SweepSingleByChannel(Hit,P,P-FVector(0,0,300),FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(25),Q)) Actor->SetActorLocation(Hit.Location);
    }
    Body.Reset();
}
void UHearthwardCombatComponent::UpdateCarry(double Delta)
{
    if(!Body.IsValid() || Action==TEXT("pickup")) return;
    if(CarryingOnBack && !G()->SpendStamina(float(2*Delta))) { DropBody(); return; }
    const FVector Dest=GetOwner()->GetActorLocation()-GetOwner()->GetActorForwardVector()*(CarryingOnBack?40:90)+FVector(0,0,CarryingOnBack?30:-50);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(CarryBody),false,GetOwner()); Q.AddIgnoredActor(Body->GetOwner());
    FHitResult Hit;
    if(GetWorld()->SweepSingleByChannel(Hit,Body->GetOwner()->GetActorLocation(),Dest,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(25),Q)) { DropBody(); return; }
    Body->GetOwner()->SetActorLocation(Dest);
}
bool UHearthwardCombatComponent::SwitchEquipment(FName Item)
{
    if(!Find(TEXT("items"),Item.ToString()) || Bag()->Available(Item)<=0 || !Start(TEXT("switch"),.4)) return false;
    PendingItem=Item; return true;
}
void UHearthwardCombatComponent::Aim(bool Value)
{
    Aiming=Value; if(!Value && Action==TEXT("draw")) Cancel();
    if(Value) Guard.Release();
}
bool UHearthwardCombatComponent::Reload()
{
    const auto R=Find(TEXT("items"),G()->Equipment.FindRef(TEXT("ranged")).ToString());
    return Text(R,TEXT("combatClass"))==TEXT("crossbow") && !CrossbowLoaded && Bag()->Available(TEXT("arrow"))>0 && Start(TEXT("reload"),1.8);
}
bool UHearthwardCombatComponent::Shoot(bool Release)
{
    const FName Weapon=G()->Equipment.FindRef(TEXT("ranged")); const auto R=Find(TEXT("items"),Weapon.ToString());
    if(!Available() || !Aiming || G()->Durability.FindRef(Weapon)<=0 || Bag()->Available(TEXT("arrow"))<=0) return false;
    // Ballistics are a content prerequisite, not an invented weapon tuning default.
    if(Number(R,TEXT("projectileSpeed"))<=0 || Number(R,TEXT("projectileGravity"))<=0) { Feedback=TEXT("该远程武器尚未配置弹道参数"); return false; }
    const bool Crossbow=Text(R,TEXT("combatClass"))==TEXT("crossbow");
    if(Crossbow) return !Release && CrossbowLoaded && Start(TEXT("crossbow"),.5);
    if(!Release) return Start(TEXT("draw"),3600);
    if(Action!=TEXT("draw") || Now()-StartedAt<.2) { Cancel(); return false; }
    const float Charge=FMath::Lerp(.5f,1.f,FMath::Clamp(float(Now()-StartedAt),0.f,1.f));
    if(!G()->SpendStamina(8)) { Cancel(); return false; }
    if(Bag()->TryRemove(TEXT("arrow"),1)!=EHearthwardInventoryResult::Success) { Cancel(); return false; }
    LaunchProjectile(Weapon,Charge,false); Cancel(); return Start(TEXT("recoil"),.35);
}
bool UHearthwardCombatComponent::Throw(FName Item)
{
    const auto R=Find(TEXT("items"),Item.ToString());
    if(!R || Bag()->Available(Item)<=0 || (Number(R,TEXT("throwDamage"))<=0 && Number(R,TEXT("bait"))<=0)) return false;
    if(Number(R,TEXT("projectileSpeed"))<=0 || Number(R,TEXT("projectileGravity"))<=0) { Feedback=TEXT("投掷物尚未配置弹道参数"); return false; }
    if(!Start(TEXT("throw"),.8)) return false; PendingItem=Item; return true;
}
void UHearthwardCombatComponent::LaunchProjectile(FName Item,float Scale,bool Thrown)
{
    const auto R=Find(TEXT("items"),Item.ToString()); const auto* C=Cast<APawn>(GetOwner());
    const FVector Dir=C?C->GetControlRotation().Vector():GetOwner()->GetActorForwardVector();
    const FVector Origin=GetOwner()->GetActorLocation()+FVector(0,0,30)+Dir*45;
    auto* P=GetWorld()->SpawnActor<AHearthwardProjectile>(Origin,Dir.Rotation());
    P->Shooter=this; P->Epoch=Epoch(); P->Event=FGuid::NewGuid(); P->Velocity=Dir*Number(R,TEXT("projectileSpeed"));
    P->Gravity=Number(R,TEXT("projectileGravity")); P->Item=Thrown?NAME_None:FName(TEXT("arrow"));
    P->Power=(Thrown?Number(R,TEXT("throwDamage")):Number(R,TEXT("attack")))*Scale*(1+G()->Effect(TEXT("attack")))*(GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>()->State.Severe()?.75f:1.f);
    P->Bait=Number(R,TEXT("bait"))>0; P->RemainingRange=Thrown?1500:Number(R,TEXT("range"),MAX_flt);
}
FString UHearthwardCombatComponent::Snapshot() const
{
    FHearthwardCombatSave Out=State; Out.Targets.Reset(); Out.CrossbowLoaded=CrossbowLoaded;
    for(auto* T:Targets()) Out.Targets.Add(T->Snapshot());
    Out.Arrows.Reset();
    for(TActorIterator<AHearthwardProjectile> It(GetWorld());It;++It)
        if(It->Landed && !It->HitTarget && It->Item==TEXT("arrow")) { FHearthwardArrowSave A; A.Position=It->GetActorLocation(); A.Rotation=It->GetActorRotation(); Out.Arrows.Add(A); }
    FString Json; FJsonObjectConverter::UStructToJsonObjectString(Out,Json); return Json;
}
bool UHearthwardCombatComponent::ValidateSnapshot(const FString& Json)
{
    if(Json.IsEmpty()) return true;
    FHearthwardCombatSave S;
    TSharedPtr<FJsonObject> Root;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root || !Root->HasTypedField<EJson::Number>(TEXT("version"))) return false;
    if(!FJsonObjectConverter::JsonObjectToUStruct(Root.ToSharedRef(),&S) || S.Version!=1 || !FMath::IsFinite(S.SenseRemaining) || S.SenseRemaining<0 || S.SenseRemaining>5
        || !FMath::IsFinite(S.SenseCooldown) || S.SenseCooldown<0 || S.SenseCooldown>20 || !FMath::IsFinite(S.OutsideSeconds) || S.OutsideSeconds<0 || S.OutsideSeconds>30) return false;
    TSet<FName> Ids;
    for(const auto& T:S.Targets)
    {
        if(T.Id.IsNone() || Ids.Contains(T.Id) || T.Generation<1 || !FMath::IsFinite(T.Health) || T.Health<0 || T.Position.ContainsNaN() || T.Rotation.ContainsNaN()
            || !FMath::IsFinite(T.ReportRemaining) || T.ReportRemaining<0 || T.ReportRemaining>3) return false;
        Ids.Add(T.Id); for(const auto& P:T.Detection) if(!FMath::IsFinite(P.Value) || P.Value<0 || P.Value>1) return false;
    }
    for(const auto& A:S.Arrows) if(A.Position.ContainsNaN() || A.Rotation.ContainsNaN()) return false;
    for(const auto& A:S.Alarms) if(!FMath::IsFinite(A.Value) || A.Value<0) return false;
    return true;
}
void UHearthwardCombatComponent::Restore(const FString& Json)
{
    Cancel(); DropBody(); Locked.Reset(); Guard={}; DamageIds.Reset(); Aiming=false;
    State={}; if(!Json.IsEmpty()) FJsonObjectConverter::JsonObjectStringToUStruct(Json,&State);
    CrossbowLoaded=State.CrossbowLoaded;
    for(TActorIterator<AHearthwardProjectile> It(GetWorld());It;++It) It->Destroy();
    for(const auto& A:State.Arrows)
    {
        auto* P=GetWorld()->SpawnActor<AHearthwardProjectile>(A.Position,A.Rotation); P->Landed=true; P->Item=TEXT("arrow"); P->Epoch=Epoch(); P->Shooter=this;
    }
    for(auto* T:Targets())
    {
        T->DamageIds.Reset();
        if(const auto* S=State.Targets.FindByPredicate([T](const auto& V){return V.Id==T->Id;})) T->Restore(*S);
    }
    ActionEpoch=Epoch();
}
