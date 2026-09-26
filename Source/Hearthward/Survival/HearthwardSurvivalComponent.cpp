#include "HearthwardSurvivalComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "GameFramework/PainCausingVolume.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Curves/CurveFloat.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"

using namespace HearthwardData;
UHearthwardSurvivalComponent::UHearthwardSurvivalComponent()
{
    PrimaryComponentTick.bCanEverTick=false;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}
void UHearthwardSurvivalComponent::BeginPlay()
{
    Super::BeginPlay();
    GetOwner()->OnTakeAnyDamage.AddDynamic(this,&UHearthwardSurvivalComponent::NativeDamage);
    if(auto* G=Gameplay()) AddTickPrerequisiteComponent(G);
}
UHearthwardGameplayComponent* UHearthwardSurvivalComponent::Gameplay() const { return GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>(); }
UHearthwardInventoryComponent* UHearthwardSurvivalComponent::Bag() const { return GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>(); }
FGuid UHearthwardSurvivalComponent::Epoch() const { return GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch(); }
float& UHearthwardSurvivalComponent::Health() { if(auto* G=Gameplay()) return G->Health; return BrotherHealth; }
float& UHearthwardSurvivalComponent::Hunger() { if(auto* G=Gameplay()) return G->Hunger; return BrotherHunger; }
float& UHearthwardSurvivalComponent::Stamina() { if(auto* G=Gameplay()) return G->Stamina; return BrotherStamina; }
float UHearthwardSurvivalComponent::MaxHealth() const { if(auto* G=Gameplay()) return G->MaxHealth(); return 100+GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Bonus(TEXT("cumulative_hp_bonus")); }
float UHearthwardSurvivalComponent::MaxStamina() const { if(auto* G=Gameplay()) return G->MaxStamina(); return 100+GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Bonus(TEXT("cumulative_stamina_bonus")); }
bool UHearthwardSurvivalComponent::Enabled() const
{
    const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    const auto* G=Gameplay(); if(!G && Player) G=Player->FindComponentByClass<UHearthwardGameplayComponent>();
    return G && G->Enabled;
}
bool UHearthwardSurvivalComponent::InCombat() const
{
    const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    const auto* G=Gameplay(); if(!G && Player) G=Player->FindComponentByClass<UHearthwardGameplayComponent>();
    return G && G->InCombat();
}
bool UHearthwardSurvivalComponent::SafeToSave() const
{
    const auto* C=Cast<ACharacter>(GetOwner());
    return Alive() && !Settling && CancelledMedicine.IsNone() && State.DrowningRemaining<0 && !InCombat()
        && (!C || (!C->GetCharacterMovement()->IsFalling() && !C->GetCharacterMovement()->IsSwimming()));
}
bool UHearthwardSurvivalComponent::Permitted(FName Item) const
{
    const auto R=Find(TEXT("items"),Item.ToString()); bool Rare=false,Key=false;
    if(!R) return false;
    R->TryGetBoolField(TEXT("rare"),Rare); R->TryGetBoolField(TEXT("key"),Key);
    const FString Base=Text(R,TEXT("medicineBase"));
    if(!Base.IsEmpty())
    {
        const auto Original=Find(TEXT("items"),Base); bool BaseRare=false,BaseKey=false;
        if(Original) { Original->TryGetBoolField(TEXT("rare"),BaseRare); Original->TryGetBoolField(TEXT("key"),BaseKey); }
        Rare|=BaseRare; Key|=BaseKey;
    }
    return !(Rare || Key) || State.AutoPermissions.Contains(Base.IsEmpty()?Item:FName(*Base));
}
void UHearthwardSurvivalComponent::SetAutoPermission(FName Item,bool Allowed)
{
    const auto Row=Find(TEXT("items"),Item.ToString()); if(!Row) return;
    const FString Base=Text(Row,TEXT("medicineBase"));
    const FName Key=Base.IsEmpty()?Item:FName(*Base);
    if(Allowed) State.AutoPermissions.Add(Key); else State.AutoPermissions.Remove(Key);
}
bool UHearthwardSurvivalComponent::BeginMedicine(FName Item,bool Automatic)
{
    if(const auto* C=GetOwner()->FindComponentByClass<UHearthwardCombatComponent>();C && (C->Busy() || C->Guarding() || C->MovementMultiplier()<1)) return false;
    const auto R=Find(TEXT("items"),Item.ToString());
    const double Fraction=Number(R,TEXT("healing"))/100.;
    const double Duration=Number(R,TEXT("medicineDuration"));
    if(!Enabled() || !Alive() || Settling || Busy() || !CancelledMedicine.IsNone() || Health()>=MaxHealth()
        || Fraction<=0 || (Automatic && !Permitted(Item)) || GetOwner()->GetVelocity().Size()>5) return false;
    if(Duration>0 && State.HotRemaining>0 && MaxHealth()*Fraction/Duration<State.HotRate) return false;
    if(!Bag() || !Bag()->Reserve(Item)) return false;
    State.Medicine=Item; State.MedicineRemaining=3; State.AutomaticMedicine=Automatic;
    ActionOrigin=GetOwner()->GetActorLocation(); ActionEpoch=Epoch();
    if(auto* Timer=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>()) Timer->InterruptAction();
    Resting=false; Status=TEXT("正在用药，移动会取消"); return true;
}
bool UHearthwardSurvivalComponent::Eat(FName Item,bool Automatic)
{
    const auto R=Find(TEXT("items"),Item.ToString()); const double Food=Number(R,TEXT("food"));
    if(!Enabled() || !Alive() || Settling || Food<=0 || Hunger()>=100 || (Automatic && !Permitted(Item))) return false;
    TGuardValue<bool> Guard(Settling,true);
    if(Bag()->TryRemove(Item,1,false)!=EHearthwardInventoryResult::Success) return false;
    Hunger()=FMath::Min(100.f,Hunger()+float(Food)*(1+(Gameplay()?Gameplay()->Effect(TEXT("food")):0)));
    State.Food(Hunger()); Bag()->OnInventoryChanged.Broadcast(); return true;
}
void UHearthwardSurvivalComponent::CancelAction(bool Damaged)
{
    if(Settling || (!Damaged && !Busy() && !Resting && !Treatment)) return;
    TGuardValue<bool> Guard(Settling,true);
    Rescue.Reset(); RescueRemaining=0; Resting=false; Treatment=false;
    FName Item=State.Medicine;
    if(Item.IsNone() && Damaged && CancelFrame==GFrameCounter) Item=CancelledMedicine;
    State.Medicine=NAME_None; State.MedicineRemaining=0;
    if(!Item.IsNone() && Bag())
    {
        if(Damaged)
        {
            const FString Half=Text(Find(TEXT("items"),Item.ToString()),TEXT("halfDose"));
            InventoryNotificationPending=Bag()->CommitReservation(Half.IsEmpty()?NAME_None:FName(*Half),false);
            CancelledMedicine=NAME_None;
        }
        else { CancelFrame=GFrameCounter; CancelledMedicine=Item; }
    }
    Status=Damaged?(Item.IsNone()?TEXT("受到伤害"):TEXT("受伤中断，损失半份药量")):TEXT("动作已取消");
}
bool UHearthwardSurvivalComponent::ReceiveDamage(float Amount,FGuid Event,FGuid Timeline,bool Fatal)
{
    if(!Enabled() || Settling || Timeline!=Epoch() || !Event.IsValid() || DamageEvents.Contains(Event)
        || Amount<=0 || !FMath::IsFinite(Amount) || State.Life==EHearthwardLife::Dead) return false;
    DamageEvents.Add(Event);
    CancelAction(true);
    TGuardValue<bool> Guard(Settling,true);
    State.Damage(Health(),Amount);
    if(Fatal) { Health()=0; State.Kill(); }
    if(!Alive())
    {
        if(auto* C=Cast<ACharacter>(GetOwner())) C->GetCharacterMovement()->StopMovementImmediately();
        if(auto* C=Cast<AHearthwardCompanionFixture>(GetOwner())) C->StopForSurvival();
    }
    if(InventoryNotificationPending) { InventoryNotificationPending=false; Bag()->OnInventoryChanged.Broadcast(); }
    return true;
}
void UHearthwardSurvivalComponent::NativeDamage(AActor*,float Amount,const UDamageType*,AController*,AActor*)
{
    if(auto* G=Gameplay()) G->ApplyDamage(Amount);
    else ReceiveDamage(Amount,FGuid::NewGuid(),Epoch());
}
void UHearthwardSurvivalComponent::GiveUp()
{
    if(State.Life!=EHearthwardLife::Downed) return;
    CancelAction(); Health()=0; State.Kill();
}
bool UHearthwardSurvivalComponent::CanRescue(const UHearthwardSurvivalComponent* Target) const
{
    if(!Target || Target==this || !Alive() || Target->State.Life!=EHearthwardLife::Downed || Target->State.DownRemaining<=0
        || FVector::Dist(GetOwner()->GetActorLocation(),Target->GetOwner()->GetActorLocation())>200) return false;
    const auto* C=Cast<ACharacter>(GetOwner());
    if(C && !C->GetCharacterMovement()->IsMovingOnGround()) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SurvivalRescue),false,GetOwner()); Query.AddIgnoredActor(Target->GetOwner());
    return !GetWorld()->LineTraceTestByChannel(GetOwner()->GetActorLocation(),Target->GetOwner()->GetActorLocation(),ECC_Visibility,Query);
}
bool UHearthwardSurvivalComponent::BeginRescue(UHearthwardSurvivalComponent* Target)
{
    if(!Enabled() || Busy() || Settling || !CanRescue(Target)) return false;
    if(const auto* C=GetOwner()->FindComponentByClass<UHearthwardCombatComponent>();C && (C->Busy() || C->Guarding() || C->MovementMultiplier()<1)) return false;
    if(auto* C=Cast<AHearthwardCompanionFixture>(GetOwner())) C->StopForSurvival();
    if(auto* T=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>()) T->InterruptAction();
    Rescue=Target; RescueRemaining=5; ActionEpoch=Epoch(); ActionOrigin=GetOwner()->GetActorLocation();
    Status=TEXT("正在扶起，需持续5秒"); return true;
}
void UHearthwardSurvivalComponent::FinishActions(double Delta)
{
    if(!Busy()) return;
    if(!Alive() || ActionEpoch!=Epoch() || FVector::Dist(ActionOrigin,GetOwner()->GetActorLocation())>5 || GetOwner()->GetVelocity().Size()>5)
    { CancelAction(); return; }
    if(auto* Target=Rescue.Get())
    {
        if(!CanRescue(Target)) { CancelAction(); return; }
        RescueRemaining=FMath::Max(0.,RescueRemaining-Delta);
        if(RescueRemaining==0)
        {
            Target->State.Life=EHearthwardLife::Alive; Target->State.DownRemaining=0; Target->Health()=Target->MaxHealth()*.1f;
            Rescue.Reset(); Status=TEXT("已扶起");
        }
    }
    if(State.Medicine.IsNone()) return;
    State.MedicineRemaining=FMath::Max(0.,State.MedicineRemaining-Delta);
    if(State.MedicineRemaining>0) return;
    const auto R=Find(TEXT("items"),State.Medicine.ToString()); const double Duration=Number(R,TEXT("medicineDuration"));
    const double Heal=MaxHealth()*Number(R,TEXT("healing"))/100.;
    if(Health()>=MaxHealth() || (State.AutomaticMedicine && !Permitted(State.Medicine))
        || (Duration>0 && State.HotRemaining>0 && Heal/Duration<State.HotRate)) { CancelAction(); return; }
    TGuardValue<bool> Guard(Settling,true);
    if(!Bag()->CommitReservation(NAME_None,false)) { State.Medicine=NAME_None; State.MedicineRemaining=0; return; }
    if(Duration>0) { State.HotRemaining=Duration; State.HotRate=Heal/Duration; }
    else Health()=FMath::Min(MaxHealth(),Health()+float(Heal));
    const FName Used=State.Medicine;
    State.Medicine=NAME_None; Status=TEXT("用药完成");
    if(auto* G=Gameplay()) G->Record(TEXT("consume"),Used);
    Bag()->OnInventoryChanged.Broadcast();
}
void UHearthwardSurvivalComponent::ResetTransient()
{
    InventoryNotificationPending=false;
    DamageEvents.Reset(); Rescue.Reset(); RescueRemaining=0; CancelledMedicine=NAME_None;
    Bag()->ReleaseReservation();
    if(!State.Medicine.IsNone()) Bag()->Reserve(State.Medicine);
    ActionEpoch=Epoch(); ActionOrigin=GetOwner()->GetActorLocation(); Resting=Treatment=false;
}
FString UHearthwardSurvivalComponent::Describe() const
{
    if(State.Life==EHearthwardLife::Dead) return TEXT("已死亡，请载入保存节点");
    if(State.Life==EHearthwardLife::Downed) return FString::Printf(TEXT("倒地：剩余 %.0f 秒"),State.DownRemaining);
    if(Rescue.IsValid()) return FString::Printf(TEXT("扶起：剩余 %.1f 秒"),RescueRemaining);
    if(!State.Medicine.IsNone()) return FString::Printf(TEXT("用药：剩余 %.1f 秒"),State.MedicineRemaining);
    if(State.DrowningRemaining>=0) return FString::Printf(TEXT("溺水：剩余 %.1f 秒"),State.DrowningRemaining);
    if(State.Severe()) return FString::Printf(TEXT("严重饥饿：距死亡 %.0f 游戏分钟"),FMath::Max(0.,State.SevereDue-GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes));
    return Status;
}
void UHearthwardSurvivalComponent::AdvanceContinuous(double Delta,double Calendar,double StartW)
{
    if(!Enabled() || GetWorld()->IsPaused()) return;
    if(!CancelledMedicine.IsNone() && CancelFrame!=GFrameCounter) { Bag()->ReleaseReservation(); CancelledMedicine=NAME_None; }
    auto* C=Cast<ACharacter>(GetOwner()); const bool Swimming=C && C->GetCharacterMovement()->IsSwimming();
    if(Swimming && SwimmingCostPerSecond>0 && Alive()) { Stamina()=FMath::Max(0.f,Stamina()-float(Delta*SwimmingCostPerSecond)); State.RecoveryDelay=.5; }
    if(Swimming && Stamina()<=0 && State.DrowningRemaining<0) State.DrowningRemaining=10;
    if(!Swimming && C && !C->GetPhysicsVolume()->bWaterVolume && C->GetCharacterMovement()->IsMovingOnGround()) State.DrowningRemaining=-1;
    if(PreviousSwimming && !Swimming) State.RecoveryDelay=.5;
    PreviousSwimming=Swimming;
    if((Resting || Treatment) && (GetOwner()->GetVelocity().Size()>5 || Swimming)) CancelAction();
    const bool Combat=InCombat();
    const bool Running=Gameplay() && Gameplay()->IsRunning();
    const double Recovery=Combat?.001:(Treatment?.03:(Resting?.02:.005));
    State.Advance(Health(),Hunger(),MaxHealth(),Delta,Calendar,StartW,(Delta>0 && (Running || Swimming || Combat))?2:1,Recovery);
    State.SafeSeconds=Combat?0:State.SafeSeconds+Delta;
    if(!Gameplay() && Alive() && !Swimming)
    {
        const double Recover=FMath::Max(0.,double(Delta)-State.RecoveryDelay);
        State.RecoveryDelay=FMath::Max(0.,State.RecoveryDelay-Delta);
        Stamina()=FMath::Min(MaxStamina(),Stamina()+float(Recover*MaxStamina()/12));
    }
    if(!Alive() && Busy()) CancelAction();

}
bool UHearthwardSurvivalComponent::AutomaticBehavior(float Delta)
{
    auto* C=Cast<AHearthwardCompanionFixture>(GetOwner());
    if(!C || !Enabled()) return false;
    if(!Alive()) { C->StopNavigation(); return true; }
    auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    auto* Target=Player?Player->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    if(Target && Target->State.Life==EHearthwardLife::Downed)
    {
        const auto* Pain=Cast<APainCausingVolume>(C->GetPhysicsVolume());
        const auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();
        if(State.DrowningRemaining>=0 || (Pain && Pain->bPainCausing && Pain->DamagePerSec>0))
        {
            CancelAction(); C->StopForSurvival();
            const bool Moving=C->Camp && C->NavigateTo(C->Camp,350,150);
            C->BlockReason=Moving?TEXT("先撤离当前环境伤害区域"):TEXT("身处危险区域，撤离路径不可达"); return true;
        }
        if(!CanRescue(Target))
        {
            const auto* Path=UNavigationSystemV1::FindPathToActorSynchronously(GetWorld(),C->GetActorLocation(),Player,100,C);
            const double Speed=350*(State.Severe()?.8:1.);
            if(!Path || !Path->IsValid() || Path->IsPartial() || Path->GetPathLength()/Speed+5>=Target->State.DownRemaining)
            { C->StopForSurvival(); C->BlockReason=TEXT("救援路径不可达或剩余时间不足"); Status=C->BlockReason; return true; }
        }
        if(G && G->IncomingDamage(C,5)>=Health())
        {
            if(Busy()) return true;
            C->StopForSurvival();
            if(Target->State.DownRemaining>8 && State.SafeSeconds>=3)
                for(const auto& V:Rows(TEXT("items")))
                {
                    const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id")));
                    if(Number(R,TEXT("healing"))>0 && Number(R,TEXT("medicineDuration"))==0 && BeginMedicine(Id,true)) return true;
                }
            if(C->Camp) C->NavigateTo(C->Camp,350,150);
            C->BlockReason=TEXT("当前火力下无法完成扶起，正在撤向营地"); Status=C->BlockReason; return true;
        }
        if(Busy()) return true;
        if(C->GetPhase()!=EHearthwardCompanionPhase::Cancelled) C->StopForSurvival();
        if(CanRescue(Target)) BeginRescue(Target);
        else if(Target->State.DownRemaining<=5 || !C->NavigateTo(Player,350,150))
            C->BlockReason=TEXT("无法及时抵达或救援路径被阻挡");
        else C->BlockReason=TEXT("正在靠近倒地的哥哥");
        Status=C->BlockReason;
        return true;
    }
    if(Busy()) return true;
    if(Hunger()<25)
    {
        FName Choice; double Best=DBL_MAX,Largest=0;
        for(const auto& V:Rows(TEXT("items")))
        {
            const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id"))); const double Food=Number(R,TEXT("food"));
            if(Food<=0 || Bag()->Available(Id)<1 || !Permitted(Id)) continue;
            if(Hunger()+Food>=25 && Food<Best) { Best=Food; Choice=Id; }
            else if(Best==DBL_MAX && Food>Largest) { Largest=Food; Choice=Id; }
        }
        if(!Choice.IsNone()) Eat(Choice,true);
    }
    if(Health()<MaxHealth()*.35f && State.SafeSeconds>=3 && Health()+State.HotRemaining*State.HotRate<MaxHealth()*.35)
    {
        FName Choice; bool Sustained=true;
        for(const auto& V:Rows(TEXT("items")))
        {
            const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id")));
            if(Number(R,TEXT("healing"))<=0 || Bag()->Available(Id)<1 || !Permitted(Id)) continue;
            if(Choice.IsNone() || (Sustained && Number(R,TEXT("medicineDuration"))==0)) { Choice=Id; Sustained=Number(R,TEXT("medicineDuration"))>0; }
        }
        if(!Choice.IsNone()) { C->StopForSurvival(); BeginMedicine(Choice,true); return true; }
    }
    return false;
}

bool UHearthwardSurvivalComponent::HasFailed(UWorld* World)
{
    int32 Down=0;
    for(TActorIterator<AActor> It(World);It;++It)
        if(const auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>();S && S->Enabled())
        {
            if(S->State.Life==EHearthwardLife::Dead) return true;
            if(S->State.Life==EHearthwardLife::Downed) ++Down;
        }
    return Down>=2;
}

void UHearthwardSurvivalComponent::FatalEnvironment()
{ ReceiveDamage(1,FGuid::NewGuid(),Epoch(),true); }
void UHearthwardSurvivalComponent::FallImpact(float Speed)
{
    if(!Enabled() || !FallDamageCurve) return;
    ReceiveDamage(FallDamageCurve->GetFloatValue(Speed),FGuid::NewGuid(),Epoch());
}
