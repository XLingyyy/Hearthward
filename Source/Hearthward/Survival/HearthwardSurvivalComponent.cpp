#include "HearthwardSurvivalComponent.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "../Gameplay/HearthwardProgression.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "GameFramework/PainCausingVolume.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "../Experience/HearthwardTraversalComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Interaction/HearthwardFurnitureInteractionComponent.h"
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
float UHearthwardSurvivalComponent::MaxHealth() const { if(auto* G=Gameplay()) return G->MaxHealth(); const auto* P=UGameplayStatics::GetPlayerPawn(GetWorld(),0); const auto* G=P?P->FindComponentByClass<UHearthwardGameplayComponent>():nullptr; return 100+HearthwardProgression::Attribute(G?G->Level():1,TEXT("hp_bonus"))+GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Bonus(TEXT("cumulative_hp_bonus")); }
float UHearthwardSurvivalComponent::MaxStamina() const { if(auto* G=Gameplay()) return G->MaxStamina(); const auto* P=UGameplayStatics::GetPlayerPawn(GetWorld(),0); const auto* G=P?P->FindComponentByClass<UHearthwardGameplayComponent>():nullptr; return 100+HearthwardProgression::Attribute(G?G->Level():1,TEXT("stamina_bonus"))+GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Bonus(TEXT("cumulative_stamina_bonus")); }
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
        && (!C || (!C->GetCharacterMovement()->IsFalling() && !C->GetCharacterMovement()->IsSwimming()))
        && (!GetOwner()->FindComponentByClass<UHearthwardTraversalComponent>() || !GetOwner()->FindComponentByClass<UHearthwardTraversalComponent>()->IsVaulting());
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
FName UHearthwardSurvivalComponent::ActiveConsumable() const
{
    if(!State.Medicine.IsNone()) return State.Medicine;
    if(!State.FoodItem.IsNone()) return State.FoodItem;
    if(State.HotRemaining>0) return State.HotItem.IsNone()?FName(TEXT("medicine")):State.HotItem;
    // Keep a same-frame cancelled dose reserved until damage or the next active tick settles it.
    return CancelledMedicine;
}
bool UHearthwardSurvivalComponent::BeginMedicine(FName Item,bool Automatic)
{
    auto Reject=[&](const TCHAR* Reason){Status=Reason;return false;};
    if(!Enabled() || HasFailed(GetWorld())) return Reject(TEXT("兄弟已无法继续，请载入保存节点"));
    if(!Alive()) return Reject(TEXT("倒地或死亡时不能用药"));
    if(const auto* C=GetOwner()->FindComponentByClass<UHearthwardCombatComponent>();C && (C->Busy() || C->Guarding() || C->MovementMultiplier()<1)) return Reject(TEXT("请先结束战斗动作再用药"));
    const auto R=Find(TEXT("items"),Item.ToString());
    const double Fraction=Number(R,TEXT("healing"))/100.;
    const double Duration=Number(R,TEXT("medicineDuration"));
    if(Settling || Busy() || !CancelledMedicine.IsNone()) return Reject(TEXT("当前动作尚未结束"));
    if(!Automatic && !ActiveConsumable().IsNone()) return Reject(TEXT("当前道具仍在使用中"));
    if(Health()>=MaxHealth()) return Reject(TEXT("生命已满，无需用药"));
    if(Fraction<=0) return Reject(TEXT("此物品不能作为药品使用"));
    if(Automatic && !Permitted(Item)) return Reject(TEXT("此药品尚未获准自动使用"));
    if(GetOwner()->GetVelocity().Size()>5) return Reject(TEXT("请停下后站定用药3秒"));
    if(Duration>0 && State.HotRemaining>0 && MaxHealth()*Fraction/Duration<State.HotRate) return Reject(TEXT("当前疗程药效更强，不能覆盖"));
    if(!Bag() || !Bag()->Reserve(Item)) return Reject(TEXT("背包中没有可用药品"));
    RecoveryFacility.Reset();RecoveryEpoch.Invalidate();Treatment=false;
    State.Medicine=Item; State.MedicineRemaining=3; State.AutomaticMedicine=Automatic;
    ActionOrigin=GetOwner()->GetActorLocation(); ActionEpoch=Epoch();
    if(auto* Timer=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>()) Timer->InterruptAction();
    Resting=false; SetStatus(TEXT("正在用药，移动会取消")); return true;
}
bool UHearthwardSurvivalComponent::Eat(FName Item,bool Automatic)
{
    const auto R=Find(TEXT("items"),Item.ToString()); const double Food=Number(R,TEXT("food"));
    if(!Enabled() || HasFailed(GetWorld())) {SetStatus(TEXT("兄弟已无法继续，请载入保存节点"));return false;}
    if(!Enabled() || !Alive() || Settling || Busy() || Food<=0 || Hunger()>=100 || (Automatic && !Permitted(Item))) return false;
    if(!Automatic)
    {
        if(!ActiveConsumable().IsNone()) return false;
        if(const auto* C=GetOwner()->FindComponentByClass<UHearthwardCombatComponent>();C && (C->Busy() || C->Guarding() || C->MovementMultiplier()<1)) return false;
        if(!Bag() || !Bag()->Reserve(Item)) return false;
        State.FoodItem=Item; State.FoodRemaining=3;
        ActionEpoch=Epoch(); ActionOrigin=GetOwner()->GetActorLocation();
        if(auto* Timer=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>()) Timer->InterruptAction();
        Resting=false; SetStatus(TEXT("正在进食")); return true;
    }
    TGuardValue<bool> Guard(Settling,true);
    if(Bag()->TryRemove(Item,1,false)!=EHearthwardInventoryResult::Success) return false;
    Hunger()=FMath::Min(100.f,Hunger()+float(Food)*(1+(Gameplay()?Gameplay()->Effect(TEXT("food")):0)));
    State.Food(Hunger());Status=TEXT("已进食");Bag()->OnInventoryChanged.Broadcast();return true;
}
bool UHearthwardSurvivalComponent::BeginRest(UHearthwardFurnitureInteractionComponent* Facility)
{
    if(!IsValid(Facility) || Facility->GetWorld()!=GetWorld() || !Enabled() || !Alive() || HasFailed(GetWorld())
        || InCombat() || (Facility->Kind!=TEXT("bed") && Facility->Kind!=TEXT("medical_area"))
        || FVector::Dist(GetOwner()->GetActorLocation(),Facility->GetComponentLocation())>Facility->MaxDistance) return false;
    CancelAction();RecoveryFacility=Facility;RecoveryEpoch=Epoch();Resting=true;Treatment=Facility->Kind==TEXT("medical_area");
    Status=Treatment?TEXT("正在治疗，每秒恢复3%最大生命；离开或交战停止"):TEXT("正在休息，每秒恢复2%最大生命；移动离开");
    return true;
}
bool UHearthwardSurvivalComponent::RestValid() const
{
    const auto* Facility=RecoveryFacility.Get();
    return Facility && IsValid(Facility->GetOwner()) && !Facility->GetOwner()->IsActorBeingDestroyed()
        && Facility->GetWorld()==GetWorld() && RecoveryEpoch==Epoch() && Alive() && !InCombat()
        && FVector::Dist(GetOwner()->GetActorLocation(),Facility->GetComponentLocation())<=Facility->MaxDistance;
}
void UHearthwardSurvivalComponent::CancelAction(bool Damaged)
{
    if(Settling || (!Damaged && !Busy() && !Resting && !Treatment)) return;
    TGuardValue<bool> Guard(Settling,true);
    Rescue.Reset(); RescueRemaining=0; Resting=false; Treatment=false;RecoveryFacility.Reset();RecoveryEpoch.Invalidate();
    if(!State.FoodItem.IsNone())
    {
        State.FoodItem=NAME_None; State.FoodRemaining=0;
        if(Bag()) Bag()->ReleaseReservation();
    }
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
    SetStatus(Damaged?(Item.IsNone()?TEXT("受到伤害"):TEXT("受伤中断，损失半份药量")):TEXT("动作已取消"));
}
bool UHearthwardSurvivalComponent::ReceiveDamage(float Amount,FGuid Event,FGuid Timeline,bool Fatal)
{
    if(!Enabled() || Settling || Timeline!=Epoch() || !Event.IsValid() || DamageEvents.Contains(Event)
        || Amount<=0 || !FMath::IsFinite(Amount) || State.Life==EHearthwardLife::Dead) return false;
    DamageEvents.Add(Event);
    if(auto* T=GetOwner()->FindComponentByClass<UHearthwardTraversalComponent>()) T->CancelVault();
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
    return RescueBlockReason(Target).IsEmpty();
}
FString UHearthwardSurvivalComponent::RescueBlockReason(const UHearthwardSurvivalComponent* Target) const
{
    if(!Target || Target==this || Target->GetWorld()!=GetWorld()) return TEXT("救援目标已失效");
    if(HasFailed(GetWorld())) return TEXT("兄弟已无法继续，请载入保存节点");
    if(!Alive()) return TEXT("当前无法施救");
    if(Target->State.Life!=EHearthwardLife::Downed || Target->State.DownRemaining<=0) return TEXT("目标已不再等待救援");
    if(FVector::Dist(GetOwner()->GetActorLocation(),Target->GetOwner()->GetActorLocation())>200) return TEXT("需靠近到2米内才能扶起");
    for(const auto* Actor:TArray<const AActor*>{GetOwner(),Target->GetOwner()})
        if(const auto* C=Cast<ACharacter>(Actor))
        {
            const auto* Traversal=Actor->FindComponentByClass<UHearthwardTraversalComponent>();
            if(Traversal?!Traversal->BreathingOnGround():(!C->GetCharacterMovement()->IsMovingOnGround() || C->GetPhysicsVolume()->bWaterVolume))
                return TEXT("双方需到可站立且能呼吸的位置才能扶起");
        }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SurvivalRescue),false,GetOwner()); Query.AddIgnoredActor(Target->GetOwner());
    return GetWorld()->LineTraceTestByChannel(GetOwner()->GetActorLocation(),Target->GetOwner()->GetActorLocation(),ECC_Visibility,Query)?TEXT("救援路径被遮挡，请移到同一侧"):FString();
}
bool UHearthwardSurvivalComponent::BeginRescue(UHearthwardSurvivalComponent* Target)
{
    if(!Enabled() || HasFailed(GetWorld())) {Status=TEXT("兄弟已无法继续，请载入保存节点");return false;}
    if(!Target || Target->State.Life!=EHearthwardLife::Downed) return false;
    const FString Reason=RescueBlockReason(Target);if(!Reason.IsEmpty()){Status=Reason;return false;}
    if(Busy() || Settling) {Status=TEXT("当前动作尚未结束");return false;}
    if(const auto* C=GetOwner()->FindComponentByClass<UHearthwardCombatComponent>();C && (C->Busy() || C->Guarding() || C->MovementMultiplier()<1)) {Status=TEXT("请先结束战斗动作再扶起");return false;}
    if(auto* C=Cast<AHearthwardCompanionFixture>(GetOwner())) C->StopForSurvival();
    if(auto* T=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>()) T->InterruptAction();
    RecoveryFacility.Reset();RecoveryEpoch.Invalidate();Resting=Treatment=false;
    Rescue=Target; RescueRemaining=5; ActionEpoch=Epoch(); ActionOrigin=GetOwner()->GetActorLocation();
    if(Cast<AHearthwardCompanionFixture>(GetOwner()))
        if(auto* P=Target->GetOwner()->FindComponentByClass<UHearthwardPresentationComponent>()) P->PlayFixedCue(TEXT("fixed.rescue.started"),FGuid::NewGuid(),true);
    SetStatus(TEXT("正在扶起，需持续5秒")); return true;
}
void UHearthwardSurvivalComponent::FinishActions(double Delta)
{
    if(!Busy()) return;
    if(HasFailed(GetWorld())) {CancelAction();Status=TEXT("兄弟已无法继续，请载入保存节点");return;}
    if(!Alive() || ActionEpoch!=Epoch() || (State.FoodItem.IsNone() && (FVector::Dist(ActionOrigin,GetOwner()->GetActorLocation())>5 || GetOwner()->GetVelocity().Size()>5)))
    { CancelAction(); return; }
    if(auto* Target=Rescue.Get())
    {
        const FString Reason=RescueBlockReason(Target);
        if(!Reason.IsEmpty()) { CancelAction();Status=Reason;return; }
        RescueRemaining=FMath::Max(0.,RescueRemaining-Delta);
        if(RescueRemaining==0)
        {
            Target->State.Life=EHearthwardLife::Alive; Target->State.DownRemaining=0; Target->Health()=Target->MaxHealth()*.1f;
            Rescue.Reset(); SetStatus(TEXT("已扶起"));
        }
    }
    if(!State.FoodItem.IsNone())
    {
        State.FoodRemaining=FMath::Max(0.,State.FoodRemaining-Delta);
        if(State.FoodRemaining>0) return;
        if(Hunger()>=100) { CancelAction();return; }
        const FName Used=State.FoodItem;
        const float Food=Number(Find(TEXT("items"),Used.ToString()),TEXT("food"));
        TGuardValue<bool> Guard(Settling,true);
        const bool Committed=Bag()->CommitReservation(NAME_None,false);
        State.FoodItem=NAME_None;State.FoodRemaining=0;
        if(!Committed) {SetStatus(TEXT("进食已取消"));return;}
        Hunger()=FMath::Min(100.f,Hunger()+Food*(1+(Gameplay()?Gameplay()->Effect(TEXT("food")):0)));
        State.Food(Hunger());SetStatus(TEXT("进食完成"));
        if(auto* G=Gameplay()) G->Record(TEXT("consume"),Used);
        Bag()->OnInventoryChanged.Broadcast();return;
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
    if(Duration>0) { State.HotRemaining=Duration; State.HotRate=Heal/Duration; State.HotItem=State.Medicine; }
    else Health()=FMath::Min(MaxHealth(),Health()+float(Heal));
    const FName Used=State.Medicine;
    State.Medicine=NAME_None; SetStatus(TEXT("用药完成"));
    if(auto* G=Gameplay()) G->Record(TEXT("consume"),Used);
    Bag()->OnInventoryChanged.Broadcast();
}
void UHearthwardSurvivalComponent::ResetTransient()
{
    InventoryNotificationPending=false;SetStatus(FString());
    DamageEvents.Reset(); Rescue.Reset(); RescueRemaining=0; CancelledMedicine=NAME_None;
    Bag()->ReleaseReservation();
    if(!State.Medicine.IsNone()) Bag()->Reserve(State.Medicine);
    else if(!State.FoodItem.IsNone()) Bag()->Reserve(State.FoodItem);
    ActionEpoch=Epoch(); ActionOrigin=GetOwner()->GetActorLocation(); Resting=Treatment=false;RecoveryFacility.Reset();RecoveryEpoch.Invalidate();Status.Reset();
}
FString UHearthwardSurvivalComponent::Describe() const
{
    if(State.Life==EHearthwardLife::Dead) return TEXT("已死亡，请载入保存节点");
    if(State.Life==EHearthwardLife::Downed) return FString::Printf(TEXT("倒地：剩余 %.0f 秒"),State.DownRemaining);
    if(Rescue.IsValid()) return FString::Printf(TEXT("扶起：剩余 %.1f 秒"),RescueRemaining);
    if(!State.Medicine.IsNone()) return FString::Printf(TEXT("用药：剩余 %.1f 秒"),State.MedicineRemaining);
    if(!State.FoodItem.IsNone()) return FString::Printf(TEXT("进食：剩余 %.1f 秒"),State.FoodRemaining);
    if(State.DrowningRemaining>=0) return FString::Printf(TEXT("溺水：剩余 %.1f 秒"),State.DrowningRemaining);
    if(State.Severe()) return FString::Printf(TEXT("严重饥饿：距死亡 %.0f 游戏分钟"),FMath::Max(0.,State.SevereDue-GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes));
    return Status;
}
void UHearthwardSurvivalComponent::AdvanceContinuous(double Delta,double Calendar,double StartW)
{
    if(!Enabled() || GetWorld()->IsPaused()) return;
    if(!CancelledMedicine.IsNone() && CancelFrame!=GFrameCounter) { Bag()->ReleaseReservation(); CancelledMedicine=NAME_None; }
    auto* C=Cast<ACharacter>(GetOwner()); const bool Swimming=C && C->GetCharacterMovement()->IsSwimming();
    double ExhaustionAt=0;
    if(Swimming && Alive())
    {
        const bool Paddling=!C->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero() || C->GetVelocity().Size2D()>=5;
        if(Paddling)
        {
            const float Correction=Bag()->GetStaminaCostMultiplier()*FMath::Max(.1f,1-(Gameplay()?Gameplay()->Effect(TEXT("cost")):0));
            const double Rate=MaxStamina()/25*Correction;
            if(Stamina()>0) ExhaustionAt=FMath::Min(Delta,Stamina()/Rate);
            Stamina()=FMath::Max(0.f,Stamina()-float(Delta*Rate));
        }
        State.RecoveryDelay=.5;
    }
    if(Swimming && Stamina()<=0 && State.DrowningRemaining<0) State.DrowningRemaining=10+ExhaustionAt;
    const auto* Traversal=GetOwner()->FindComponentByClass<UHearthwardTraversalComponent>();
    if(!Swimming && C && (Traversal?Traversal->BreathingOnGround():(!C->GetPhysicsVolume()->bWaterVolume && C->GetCharacterMovement()->IsMovingOnGround()))) State.DrowningRemaining=-1;
    if(PreviousSwimming && !Swimming) State.RecoveryDelay=.5;
    PreviousSwimming=Swimming;
    if((Resting || Treatment) && (!RestValid() || GetOwner()->GetVelocity().Size()>5 || Swimming))
    {CancelAction();Status=TEXT("已离开休息设施或进入战斗，恢复改为当前状态速率");}
    const bool Combat=InCombat();
    State.Advance(Health(),Hunger(),MaxHealth(),Delta,Calendar,StartW,HungerMultiplier(Delta),RecoveryFraction());
    State.SafeSeconds=Combat?0:State.SafeSeconds+Delta;
    if(!Gameplay() && Alive() && !Swimming)
    {
        const double Recover=FMath::Max(0.,double(Delta)-State.RecoveryDelay);
        State.RecoveryDelay=FMath::Max(0.,State.RecoveryDelay-Delta);
        Stamina()=FMath::Min(MaxStamina(),Stamina()+float(Recover*MaxStamina()/12));
    }
    if(!Alive() && Busy()) CancelAction();

}
double UHearthwardSurvivalComponent::HungerMultiplier(double Active) const
{
    const auto* C=Cast<ACharacter>(GetOwner());
    const bool Exertion=InCombat() || (Gameplay() && Gameplay()->IsRunning()) || (C && C->GetCharacterMovement()->IsSwimming());
    return ((Active>0 && Exertion)?2:1)*(1-(Gameplay()?Gameplay()->Effect(TEXT("hunger")):0));
}
double UHearthwardSurvivalComponent::RecoveryFraction() const
{
    return (InCombat()?.001:(RestValid()?(Treatment?.03:.02):.005))*(1+(Gameplay()?Gameplay()->Effect(TEXT("recovery")):0));
}
double UHearthwardSurvivalComponent::PreviewAdvance(double Active,double Calendar,double StartW)
{
    auto Preview=State;float HP=Health(),Food=Hunger();
    return Preview.Advance(HP,Food,MaxHealth(),Active,Calendar,StartW,HungerMultiplier(Active),RecoveryFraction());
}
bool UHearthwardSurvivalComponent::AutomaticBehavior(float Delta)
{
    auto* C=Cast<AHearthwardCompanionFixture>(GetOwner());
    if(!C || !Enabled()) return false;
    if(HasFailed(GetWorld())) {CancelAction();C->StopForSurvival();Status=TEXT("兄弟已无法继续，请载入保存节点");return true;}
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
            { C->StopForSurvival(); C->BlockReason=TEXT("救援路径不可达或剩余时间不足"); SetStatus(C->BlockReason); return true; }
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
            C->BlockReason=TEXT("当前火力下无法完成扶起，正在撤向营地"); SetStatus(C->BlockReason); return true;
        }
        if(Busy()) return true;
        if(C->GetPhase()!=EHearthwardCompanionPhase::Cancelled) C->StopForSurvival();
        if(CanRescue(Target)) BeginRescue(Target);
        else if(Target->State.DownRemaining<=5 || !C->NavigateTo(Player,350,150))
            C->BlockReason=TEXT("无法及时抵达或救援路径被阻挡");
        else C->BlockReason=TEXT("正在靠近倒地的哥哥");
        SetStatus(C->BlockReason);
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
        if(!Choice.IsNone())
        {
            if(auto* P=Player->FindComponentByClass<UHearthwardPresentationComponent>()) P->PlayFixedCue(TEXT("fixed.hunger.food"),FGuid::NewGuid(),true);
            Eat(Choice,true);
        }
        else Status=TEXT("饱食不足，背包中没有获准使用的食物");
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
        Status=TEXT("生命偏低，背包中没有获准使用的药品");
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
    if(!Enabled()) return;
    const auto* C=CastChecked<ACharacter>(GetOwner());
    ReceiveDamage(UHearthwardTraversalComponent::FallDamage(Speed,C->GetCharacterMovement()->GetGravityZ(),MaxHealth()),FGuid::NewGuid(),Epoch());
}
