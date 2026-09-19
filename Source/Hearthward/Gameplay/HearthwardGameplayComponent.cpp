#include "HearthwardGameplayComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../AI/HearthwardLocalAISubsystem.h"

using namespace HearthwardData;
namespace
{
double Tune(const FString& Key) { return Number(Catalog()->GetObjectField(TEXT("tuning")), Key); }
FName EventKey(FName Kind, FName Target) { return FName(*(Kind.ToString()+TEXT(":")+Target.ToString())); }
TSharedPtr<FJsonObject> Parse(const FString& Text)
{
    TSharedPtr<FJsonObject> Out;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Out); return Out;
}
}
UHearthwardGameplayComponent::UHearthwardGameplayComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}
UHearthwardInventoryComponent* UHearthwardGameplayComponent::Inventory() const
{ return GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>(); }
void UHearthwardGameplayComponent::BeginPlay()
{
    Super::BeginPlay();
    if (Inventory()) Inventory()->OnInventoryChanged.AddDynamic(this, &UHearthwardGameplayComponent::InventoryChanged);
}
void UHearthwardGameplayComponent::EnableAdventure()
{
    if (Enabled) return;
    Enabled = true; Origin = GetOwner()->GetActorLocation();
    Origin.Z = GetOwner()->GetActorLocation().Z - 100;
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        if(IsValid(It->Camp)) { Origin=It->Camp->GetActorLocation()-FVector(0,0,100); break; }
    Discovered.Add(TEXT("camp")); Activated.Add(TEXT("camp"));
    CreateLandmarks();
    OnChanged.Broadcast();
}
void UHearthwardGameplayComponent::CreateLandmarks()
{
    for(auto& A:LandmarkActors) if(A.IsValid()) A->Destroy();
    LandmarkActors.Reset();
    for(auto& A:OpponentActors) if(A.Value.IsValid()) A.Value->Destroy();
    OpponentActors.Reset();
    for(const auto& V:Rows(TEXT("locations")))
    {
        const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id"))); if(Id==TEXT("camp")) continue;
        auto* A=GetWorld()->SpawnActor<AActor>();
        auto* Mesh=NewObject<UStaticMeshComponent>(A); A->AddInstanceComponent(Mesh); A->SetRootComponent(Mesh);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->RegisterComponent();
        A->SetActorLocation(LocationPosition(Id)); A->SetActorScale3D(FVector(.35,.35,2));
        auto* Label=NewObject<UTextRenderComponent>(A); A->AddInstanceComponent(Label); Label->SetupAttachment(Mesh); Label->RegisterComponent();
        Label->SetRelativeLocation(FVector(0,0,80)); Label->SetText(FText::FromString(TEXT("E"))); Label->SetWorldSize(32);
        LandmarkActors.Add(A);
    }
    for(const auto& V:Rows(TEXT("encounters")))
    {
        const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id")));
        if(!Opponents.Contains(Id)) Opponents.Add(Id,Number(R,TEXT("health")));
        if(Opponents[Id]<=0) continue;
        auto* A=GetWorld()->SpawnActor<AActor>(); auto* Mesh=NewObject<UStaticMeshComponent>(A);
        A->AddInstanceComponent(Mesh); A->SetRootComponent(Mesh);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->RegisterComponent();
        A->SetActorScale3D(FVector(.7,.7,1.7)); A->SetActorLocation(Origin+Position(R)); OpponentActors.Add(Id,A);
    }
}
int32 UHearthwardGameplayComponent::Level() const
{
    int32 Remaining = Experience, L = 1;
    while (L < Tune(TEXT("maxLevel")))
    {
        const int32 Cost = Tune(TEXT("xpBase")) + (L-1)*Tune(TEXT("xpGrowth"));
        if (Remaining < Cost) break;
        Remaining -= Cost; ++L;
    }
    return L;
}
int32 UHearthwardGameplayComponent::SkillPoints() const
{
    int32 Spent = 0, Total = 0;
    for (const auto& V : Rows(TEXT("skills")))
    {
        const auto R = V->AsObject();
        const int32 Cost = Number(R,TEXT("cost"));
        Total += Number(R,TEXT("maxRank"))*Cost;
        Spent += Skills.FindRef(FName(*Text(R,TEXT("id"))))*Cost;
    }
    return FMath::Max(0,FMath::Min(int32(Tune(TEXT("initialSkillPoints"))) + Level()-1,
        FMath::FloorToInt(Total*Tune(TEXT("maxSkillCoverage")))) - Spent);
}
float UHearthwardGameplayComponent::Effect(FName Name) const
{
    float Value = 0;
    for (const auto& S : Skills)
    {
        const auto R=Find(TEXT("skills"),S.Key.ToString());
        if (Text(R,TEXT("effect"))==Name.ToString()) Value += S.Value*Number(R,TEXT("amount"));
    }
    return Value;
}
float UHearthwardGameplayComponent::MaxHealth() const
{ return 100 + 100.f*(Level()-1)/59 + 100.f*(CampTier-1)/7 + Effect(TEXT("health")); }
float UHearthwardGameplayComponent::MaxStamina() const
{ return 100 + 50.f*(Level()-1)/59 + 50.f*(CampTier-1)/7 + Effect(TEXT("stamina")); }
bool UHearthwardGameplayComponent::Result(bool Success,const FString& Message)
{ Feedback=Message; OnChanged.Broadcast(); return Success; }
bool UHearthwardGameplayComponent::Learn(FName Id)
{
    const auto R=Find(TEXT("skills"),Id.ToString());
    if (!R || !Enabled) return Result(false,TEXT("技能不可用"));
    if (Skills.FindRef(Id)>=Number(R,TEXT("maxRank"))) return Result(false,TEXT("已达到最高等级"));
    const FName Parent(*Text(R,TEXT("requires")));
    if (!Parent.IsNone() && Skills.FindRef(Parent)==0) return Result(false,TEXT("请先学习前置技能"));
    if (SkillPoints()<Number(R,TEXT("cost"))) return Result(false,TEXT("可用技能点不足"));
    ++Skills.FindOrAdd(Id); Record(TEXT("learn"),Id);
    return Result(true,TEXT("已学习：")+Text(R,TEXT("name")));
}
void UHearthwardGameplayComponent::ResetSkills()
{
    Skills.Reset(); Health=FMath::Min(Health,MaxHealth()); Stamina=FMath::Min(Stamina,MaxStamina());
    Result(true,TEXT("技能点已全部返还，已发生的奖励保留"));
}
bool UHearthwardGameplayComponent::Equip(FName Id)
{
    const auto R=Find(TEXT("items"),Id.ToString());
    const FName Slot(*Text(R,TEXT("slot")));
    if (!Enabled || Slot.IsNone() || Inventory()->GetItemCount(Id)<1) return Result(false,TEXT("没有可装备的物品"));
    if (Equipment.FindRef(Slot)==Id) Equipment.Remove(Slot);
    else { Equipment.Add(Slot,Id); if(!Durability.Contains(Id)) Durability.Add(Id,Number(R,TEXT("durability"),100)); Record(TEXT("equip"),Slot); }
    return Result(true,TEXT("装备已更新"));
}
bool UHearthwardGameplayComponent::UseItem(FName Id)
{
    const auto R=Find(TEXT("items"),Id.ToString());
    const float Food=Number(R,TEXT("food"));
    const float Healing=Number(R,TEXT("healing"));
    if(Healing>0)
    {
        if(!Enabled || Health<=0 || Health>=MaxHealth()) return Result(false,TEXT("当前无法使用药品"));
        if(Inventory()->TryRemove(Id,1)!=EHearthwardInventoryResult::Success) return Result(false,TEXT("药品数量不足"));
        Health=FMath::Min(MaxHealth(),Health+Healing); Record(TEXT("consume"),Id);
        return Result(true,TEXT("已使用：")+Text(R,TEXT("name")));
    }
    if(Number(R,TEXT("throwDamage"))>0) return ThrowItem(Id);
    if (Food<=0) return Equip(Id);
    if (!Enabled || Hunger>=100) return Result(false,TEXT("当前饱食已满"));
    if (Inventory()->TryRemove(Id,1)!=EHearthwardInventoryResult::Success) return Result(false,TEXT("物品数量不足"));
    Hunger=FMath::Min(100.f,Hunger+Food*(1+Effect(TEXT("food"))));
    Record(TEXT("consume"),Id); return Result(true,TEXT("已食用：")+Text(R,TEXT("name")));
}
bool UHearthwardGameplayComponent::Drop(FName Id,int32 Count)
{
    const auto R=Find(TEXT("items"),Id.ToString()); bool Key=false;
    if (R) R->TryGetBoolField(TEXT("key"),Key);
    if (Key) return Result(false,TEXT("关键物品不能丢弃"));
    return Result(Inventory()->TryRemove(Id,Count)==EHearthwardInventoryResult::Success,TEXT("丢弃操作已处理"));
}
void UHearthwardGameplayComponent::InventoryChanged()
{
    for (auto It=Equipment.CreateIterator();It;++It)
        if (Inventory()->GetItemCount(It.Value())==0) It.RemoveCurrent();
    if(Enabled)
        for(const auto& Item:Rows(TEXT("items")))
        {
            const FName Id(*Text(Item->AsObject(),TEXT("id")));
            if(Inventory()->GetItemCount(Id)>0) Events.FindOrAdd(EventKey(TEXT("collected"),Id))=1;
        }
    OnChanged.Broadcast();
}
void UHearthwardGameplayComponent::Record(FName Kind,FName Target,int32 Count)
{
    Events.FindOrAdd(EventKey(Kind,Target))+=Count;
    Events.FindOrAdd(EventKey(Kind,TEXT("any")))+=Count;
    OnChanged.Broadcast();
}
bool UHearthwardGameplayComponent::QuestAvailable(FName Id) const
{
    const auto R=Find(TEXT("quests"),Id.ToString());
    const FName Parent(*Text(R,TEXT("requires")));
    return R && (Parent.IsNone() || Claimed.Contains(Parent));
}
int32 UHearthwardGameplayComponent::QuestProgress(FName Id) const
{
    const auto R=Find(TEXT("quests"),Id.ToString()); if (!R) return 0;
    const FString Event=Text(R,TEXT("event")); const FName Target(*Text(R,TEXT("target")));
    int32 Value=Events.FindRef(EventKey(FName(*Event),Target));
    if (Event==TEXT("inventory")) Value=Inventory()->GetItemCount(Target);
    if (Event==TEXT("storage")) Value=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetItemCount(Target);
    return FMath::Min(Value,int32(Number(R,TEXT("required"))));
}
bool UHearthwardGameplayComponent::Claim(FName Id)
{
    const auto R=Find(TEXT("quests"),Id.ToString());
    if (!QuestAvailable(Id) || Claimed.Contains(Id) || QuestProgress(Id)<Number(R,TEXT("required")))
        return Result(false,TEXT("尚未完成目标或奖励已领取"));
    Claimed.Add(Id); Experience += FMath::RoundToInt(Number(R,TEXT("xp"))*(1+Effect(TEXT("xp"))));
    for (const auto& Q : Rows(TEXT("quests")))
        if (Text(Q->AsObject(),TEXT("requires"))==Id.ToString()) { TrackedQuest=FName(*Text(Q->AsObject(),TEXT("id"))); break; }
    return Result(true,TEXT("任务完成，已获得成长经验"));
}
bool UHearthwardGameplayComponent::Track(FName Id)
{
    if (!QuestAvailable(Id)) return Result(false,TEXT("任务尚未开启"));
    TrackedQuest=TrackedQuest==Id ? NAME_None : Id; return Result(true,TEXT("任务追踪已更新"));
}
FVector UHearthwardGameplayComponent::LocationPosition(FName Id) const
{ const auto R=Find(TEXT("locations"),Id.ToString()); return R ? Origin+Position(R) : Origin; }
FName UHearthwardGameplayComponent::NearbyLocation() const
{
    for (const auto& L : Rows(TEXT("locations")))
    {
        const FName Id(*Text(L->AsObject(),TEXT("id")));
        if (FVector::Dist2D(GetOwner()->GetActorLocation(),LocationPosition(Id))<=Tune(TEXT("interactRadius"))) return Id;
    }
    return NAME_None;
}
bool UHearthwardGameplayComponent::ActivateNearby()
{
    const FName Id=NearbyLocation(); const auto R=Find(TEXT("locations"),Id.ToString());
    if (Id.IsNone() || Text(R,TEXT("kind"))==TEXT("landmark")) return false;
    if (Activated.Contains(Id)) return Result(false,TEXT("路标已激活"));
    Activated.Add(Id); Record(TEXT("activate"),Id); return Result(true,TEXT("传送路标已激活"));
}
bool UHearthwardGameplayComponent::Travel(FName Id)
{
    const FName From=NearbyLocation();
    if (From.IsNone() || !Activated.Contains(From)) return Result(false,TEXT("请站在已激活的路标旁"));
    if (!Activated.Contains(Id)) return Result(false,TEXT("目标传送点尚未激活"));
    if (From==Id) return Result(false,TEXT("你已在此处"));
    auto* Character=Cast<ACharacter>(GetOwner());
    if (!Character || !Character->TeleportTo(LocationPosition(Id),Character->GetActorRotation()))
        return Result(false,TEXT("目标落点无法通行"));
    Character->GetCharacterMovement()->StopMovementImmediately();
    Record(TEXT("travel"),Id); return Result(true,TEXT("已抵达目的地"));
}
void UHearthwardGameplayComponent::SetSprinting(bool Value) { Sprinting=Value; }
bool UHearthwardGameplayComponent::SpendStamina(float Cost)
{
    Cost*=Inventory()->GetStaminaCostMultiplier()*FMath::Max(.1f,1-Effect(TEXT("cost")));
    if (Stamina<Cost) return false;
    Stamina-=Cost; RecoveryDelay=Tune(TEXT("staminaRecoveryDelay")); return true;
}
void UHearthwardGameplayComponent::ApplyDamage(float Damage)
{
    if(Damage<=0 || Health<=0) return;
    if(auto* Timer=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>()) Timer->InterruptAction();
    float Armor=0;
    for (const auto& E : Equipment)
    {
        if(Durability.FindRef(E.Value)<=0) continue;
        const float Defense=Number(Find(TEXT("items"),E.Value.ToString()),TEXT("defense"));
        if(Defense<=0) continue;
        Armor+=Defense/100;
        Durability.FindOrAdd(E.Value)=FMath::Max(0.f,Durability.FindRef(E.Value)-1/(1+Effect(TEXT("durability"))));
    }
    Health=FMath::Max(0.f,Health-Damage*(1-FMath::Clamp(Armor+Effect(TEXT("defense")),0.f,.85f)));
    OnChanged.Broadcast();
}
float UHearthwardGameplayComponent::AttackPower() const
{
    const FName Weapon=Equipment.FindRef(TEXT("weapon"));
    if(Weapon.IsNone() || Durability.FindRef(Weapon)<=0) return 0;
    return Number(Find(TEXT("items"),Weapon.ToString()),TEXT("attack"))*(1+Effect(TEXT("attack")));
}
bool UHearthwardGameplayComponent::Attack()
{ return AttackWith(false,false); }
bool UHearthwardGameplayComponent::HeavyAttack()
{ return AttackWith(true,false); }
bool UHearthwardGameplayComponent::Shoot()
{ return AttackWith(false,true); }
bool UHearthwardGameplayComponent::Repair(FName Id)
{
    if(!Enabled || Health<=0 || InCombat() || NearbyLocation()!=TEXT("camp")) return Result(false,TEXT("请在安全的营地修理装备"));
    const auto Item=Find(TEXT("items"),Id.ToString());
    if(!Item || Text(Item,TEXT("slot")).IsEmpty() || Inventory()->GetItemCount(Id)==0 || !Durability.Contains(Id) || Durability[Id]>=Number(Item,TEXT("durability"),100)) return Result(false,TEXT("这件装备无需修理"));
    const auto Materials=Catalog()->GetObjectField(TEXT("repairMaterials"));
    for(const auto& M:Materials->Values)
        if(Inventory()->GetItemCount(FName(*M.Key))<M.Value->AsNumber()) return Result(false,TEXT("修理材料不足，请从仓库取出所需材料"));
    for(const auto& M:Materials->Values) Inventory()->TryRemove(FName(*M.Key),M.Value->AsNumber());
    Durability[Id]=Number(Item,TEXT("durability"),100); Record(TEXT("repair"),Id);
    return Result(true,TEXT("装备已修复"));
}
bool UHearthwardGameplayComponent::ThrowItem(FName Id)
{
    if(!Enabled || GetWorld()->IsPaused() || Health<=0 || AttackDelay>0) return false;
    const auto Item=Find(TEXT("items"),Id.ToString());
    if(Number(Item,TEXT("throwDamage"))<=0 || Inventory()->GetItemCount(Id)<1) return Result(false,TEXT("投掷物不足"));
    FName Target; float Distance=Number(Item,TEXT("range"));
    for(const auto& Enemy:OpponentActors)
    {
        if(!Enemy.Value.IsValid() || Opponents.FindRef(Enemy.Key)<=0) continue;
        const float D=FVector::Dist(GetOwner()->GetActorLocation(),Enemy.Value->GetActorLocation());
        if(D<Distance) { Target=Enemy.Key; Distance=D; }
    }
    if(Target.IsNone()) return Result(false,TEXT("投掷范围内没有敌人"));
    FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(HearthwardThrow),false,GetOwner());
    if(GetWorld()->LineTraceSingleByChannel(Hit,GetOwner()->GetActorLocation(),OpponentActors[Target]->GetActorLocation(),ECC_Visibility,Query)) return Result(false,TEXT("目标被障碍物遮挡"));
    if(!SpendStamina(Number(Item,TEXT("stamina")))) return Result(false,TEXT("耐力不足"));
    Inventory()->TryRemove(Id,1); DamageOpponent(Target,Number(Item,TEXT("throwDamage")));
    AttackDelay=Number(Item,TEXT("cooldown")); CombatRemaining=3;
    return Result(true,TEXT("投掷命中：")+Text(Item,TEXT("name")));
}
bool UHearthwardGameplayComponent::AttackWith(bool Heavy,bool Ranged)
{
    if(!Enabled || GetWorld()->IsPaused() || Health<=0 || AttackDelay>0) return false;
    const FName Weapon=Equipment.FindRef(Ranged?TEXT("ranged"):TEXT("weapon"));
    const int32 HeavyRank=Skills.FindRef(TEXT("strong"));
    if(Heavy && HeavyRank==0) return Result(false,TEXT("请先学习强力挥击"));
    if(Ranged && Inventory()->GetItemCount(TEXT("arrow"))<1) return Result(false,TEXT("箭矢不足"));
    const auto HeavySkill=Find(TEXT("skills"),TEXT("strong"));
    float Power=Ranged?Number(Find(TEXT("items"),Weapon.ToString()),TEXT("attack"))*(1+Effect(TEXT("attack"))):AttackPower();
    if(Weapon.IsNone() || Durability.FindRef(Weapon)<=0 || Power<=0) return Result(false,TEXT("请装备可用的武器"));
    if(Heavy) Power*=HeavySkill->GetArrayField(TEXT("multipliers"))[HeavyRank-1]->AsNumber();
    FName Target; float Distance=Tune(Ranged?TEXT("rangedRange"):TEXT("attackRange"));
    for(const auto& A:OpponentActors)
    {
        if(!A.Value.IsValid() || Opponents.FindRef(A.Key)<=0) continue;
        const float D=FVector::Dist(GetOwner()->GetActorLocation(),A.Value->GetActorLocation());
        if(D<Distance) { Target=A.Key; Distance=D; }
    }
    if(Target.IsNone()) return Result(false,TEXT("攻击范围内没有敌人"));
    FHitResult Obstacle; FCollisionQueryParams Query(SCENE_QUERY_STAT(HearthwardAttack),false,GetOwner());
    if(GetWorld()->LineTraceSingleByChannel(Obstacle,GetOwner()->GetActorLocation(),OpponentActors[Target]->GetActorLocation(),ECC_Visibility,Query))
        return Result(false,TEXT("目标被障碍物遮挡"));
    if(!SpendStamina(Tune(Heavy?TEXT("heavyStamina"):Ranged?TEXT("rangedStamina"):TEXT("attackStamina")))) return Result(false,TEXT("耐力不足"));
    if(Ranged) Inventory()->TryRemove(TEXT("arrow"),1);
    AttackDelay=Tune(TEXT("attackCooldown")); CombatRemaining=3;
    DamageOpponent(Target,Power);
    if(Heavy && FMath::FRand()<HeavySkill->GetArrayField(TEXT("stunChance"))[HeavyRank-1]->AsNumber())
        Stunned.Add(Target,HeavySkill->GetArrayField(TEXT("stunSeconds"))[HeavyRank-1]->AsNumber());
    Durability[Weapon]=FMath::Max(0.f,Durability[Weapon]-1/(1+Effect(TEXT("durability"))));
    return Result(true,FString::Printf(TEXT("命中敌人 −%.0f"),Power));
}
void UHearthwardGameplayComponent::DamageOpponent(FName Target,float Damage)
{
    if(Opponents.FindRef(Target)<=0) return;
    Opponents[Target]=FMath::Max(0.f,Opponents[Target]-Damage);
    if(Opponents[Target]<=0)
    {
        OpponentActors[Target]->Destroy();
        Experience+=Number(Find(TEXT("encounters"),Target.ToString()),TEXT("xp"))*(1+Effect(TEXT("xp")));
        Record(TEXT("defeat"),Target);
    }
}
bool UHearthwardGameplayComponent::OrderCompanion(FName Order)
{
    if(!Enabled || Health<=0 || GetWorld()->IsPaused()) return false;
    if(Order!=TEXT("wait") && Order!=TEXT("follow") && Order!=TEXT("attack")) return false;
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        if(!It->CanCommunicate(GetOwner())) return Result(false,TEXT("请靠近弟弟，交流范围30米"));
        GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->CancelPending();
        if(!It->Cancel(GetOwner())) return false;
        CompanionOrder=Order;
        return Result(true,Order==TEXT("wait")?TEXT("弟弟在原地等待"):Order==TEXT("follow")?TEXT("弟弟开始跟随"):TEXT("弟弟协助攻击附近敌人"));
    }
    return Result(false,TEXT("弟弟不在附近"));
}
void UHearthwardGameplayComponent::TickCompanion(float Delta)
{
    CompanionAttackDelay=FMath::Max(0.f,CompanionAttackDelay-Delta);
    if(CompanionOrder==TEXT("wait")) return;
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        using P=EHearthwardCompanionPhase;
        const P Phase=It->GetPhase();
        if(Phase==P::GoingToSource || Phase==P::Gathering || Phase==P::Returning || Phase==P::ReturningBlocked)
        { CompanionOrder=TEXT("wait"); return; }
        AActor* Destination=GetOwner(); FName Target;
        if(CompanionOrder==TEXT("attack"))
        {
            float Nearest=Tune(TEXT("companionCommandRange"));
            for(const auto& Enemy:OpponentActors)
            {
                if(!Enemy.Value.IsValid() || Opponents.FindRef(Enemy.Key)<=0 || FVector::Dist2D(GetOwner()->GetActorLocation(),Enemy.Value->GetActorLocation())>Tune(TEXT("companionCommandRange"))) continue;
                const float Distance=FVector::Dist2D(It->GetActorLocation(),Enemy.Value->GetActorLocation());
                if(Distance<Nearest) { Nearest=Distance; Destination=Enemy.Value.Get(); Target=Enemy.Key; }
            }
        }
        const float StopDistance=Target.IsNone()?Tune(TEXT("companionFollowDistance")):Tune(TEXT("attackRange"))*.8f;
        FVector Direction=Destination->GetActorLocation()-It->GetActorLocation(); Direction.Z=0;
        if(Direction.Size()>StopDistance)
        {
            It->BlockReason=It->NavigateTo(Destination,Tune(TEXT("companionMoveSpeed")),StopDistance-10)?TEXT(""):TEXT("目标不可达，请调整位置");
        }
        else
        {
            if(Target.IsNone()) { It->StopNavigation(); It->BlockReason.Reset(); return; }
            FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(HearthwardCompanionAttack),false,*It);
            Query.AddIgnoredActor(GetOwner());
            if(!GetWorld()->LineTraceSingleByChannel(Hit,It->GetActorLocation(),Destination->GetActorLocation(),ECC_Visibility,Query))
            {
                It->StopNavigation(); It->BlockReason.Reset();
                if(CompanionAttackDelay<=0)
                { DamageOpponent(Target,Tune(TEXT("companionAttack"))); CompanionAttackDelay=Tune(TEXT("companionAttackCooldown")); CombatRemaining=3; }
            }
            else It->BlockReason=It->NavigateTo(Destination,Tune(TEXT("companionMoveSpeed")),30)?TEXT("正在绕行接近目标"):TEXT("目标不可达，请调整位置");
        }
        return;
    }
}
void UHearthwardGameplayComponent::SetWaypoint(FVector Position)
{
    Waypoint=Position; HasWaypoint=true; OnChanged.Broadcast();
}
void UHearthwardGameplayComponent::TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,TickType,Function);
    if (!Enabled || GetWorld()->IsPaused()) return;
    AttackDelay=FMath::Max(0.f,AttackDelay-Delta); CombatRemaining=FMath::Max(0.f,CombatRemaining-Delta); EnemyAttackDelay-=Delta;
    for(auto& S:Stunned) S.Value=FMath::Max(0.f,S.Value-Delta);
    if(Health<=0) { Sprinting=false; return; }
    TickCompanion(Delta);
    for(const auto& A:OpponentActors)
    {
        if(!A.Value.IsValid() || Opponents.FindRef(A.Key)<=0) continue;
        if(FVector::Dist(GetOwner()->GetActorLocation(),A.Value->GetActorLocation())<Tune(TEXT("attackRange")))
        {
            CombatRemaining=3;
            if(EnemyAttackDelay<=0 && Stunned.FindRef(A.Key)<=0) ApplyDamage(Number(Find(TEXT("encounters"),A.Key.ToString()),TEXT("attack")));
        }
    }
    if(EnemyAttackDelay<=0) EnemyAttackDelay=2;
    auto* Character=Cast<ACharacter>(GetOwner()); if (!Character) return;
    const bool Moving=Character->GetVelocity().Size2D()>5;
    const bool Running=Sprinting && Moving && Stamina>0;
    const float Regen=100/Tune(TEXT("staminaRecoverySeconds"));
    if (Running) { if (!SpendStamina(Delta*Regen*Tune(TEXT("sprintCostRatio")))) { Stamina=0; Sprinting=false; } }
    else if ((RecoveryDelay-=Delta)<=0) Stamina=FMath::Min(MaxStamina(),Stamina+Delta*Regen*(1+Effect(TEXT("staminaRecovery"))));
    const float HungerCost=Delta*100/Tune(TEXT("hungerSeconds"))*(Running?Tune(TEXT("hungerHighMultiplier")):1)*(1-Effect(TEXT("hunger")));
    Hunger=FMath::Max(0.f,Hunger-HungerCost);
    if (Hunger>0 && Health>0) Health=FMath::Min(MaxHealth(),Health+Delta*MaxHealth()*(InCombat()?.001f:.005f)*(1+Effect(TEXT("recovery"))));
    else if (Health>MaxHealth()*.1f) Health=FMath::Max(MaxHealth()*.1f,Health-Delta*MaxHealth()*.9f/300);
    Character->GetCharacterMovement()->MaxWalkSpeed=(Running ? Tune(TEXT("sprintSpeed"))*(1+Effect(TEXT("sprint"))) : Tune(TEXT("walkSpeed")))*Inventory()->GetMoveSpeedMultiplier();
    if ((ExploreDelay-=Delta)>0) return;
    ExploreDelay=.25;
    const FVector P=GetOwner()->GetActorLocation();
    const FVector2D XY(P.X,P.Y);
    if (!Explored.ContainsByPredicate([XY](const FVector2D& E){ return FVector2D::Distance(E,XY)<1000; })) Explored.Add(XY);
    for (const auto& L : Rows(TEXT("locations")))
    {
        const FName Id(*Text(L->AsObject(),TEXT("id")));
        if (!Discovered.Contains(Id) && FVector::Dist2D(P,LocationPosition(Id))<=Tune(TEXT("discoverRadius"))*(1+Effect(TEXT("discover"))))
        { Discovered.Add(Id); Record(TEXT("discover"),Id); Feedback=TEXT("发现：")+Text(L->AsObject(),TEXT("name")); }
    }
    OnChanged.Broadcast();
}
FString UHearthwardGameplayComponent::SaveSnapshot() const
{
    auto J=MakeShared<FJsonObject>();
    J->SetNumberField(TEXT("version"),1);
    if(const auto* B=GetOwner()?GetOwner()->FindComponentByClass<UHearthwardBuildingComponent>():nullptr) J->SetArrayField(TEXT("buildings"),B->Snapshot());
    J->SetNumberField(TEXT("health"),Health); J->SetNumberField(TEXT("hunger"),Hunger); J->SetNumberField(TEXT("stamina"),Stamina);
    J->SetNumberField(TEXT("experience"),Experience); J->SetNumberField(TEXT("campTier"),CampTier); J->SetBoolField(TEXT("enabled"),Enabled);
    J->SetStringField(TEXT("tracked"),TrackedQuest.ToString());
    J->SetStringField(TEXT("companionOrder"),CompanionOrder.ToString());
    J->SetBoolField(TEXT("hasWaypoint"),HasWaypoint); J->SetStringField(TEXT("waypoint"),Waypoint.ToString());
    auto Map=[&](const TCHAR* Key,const TMap<FName,int32>& Values){ auto M=MakeShared<FJsonObject>(); for(const auto& V:Values) M->SetNumberField(V.Key.ToString(),V.Value); J->SetObjectField(Key,M); };
    Map(TEXT("skills"),Skills); Map(TEXT("events"),Events);
    auto Floats=[&](const TCHAR* Key,const TMap<FName,float>& Values){ auto M=MakeShared<FJsonObject>(); for(const auto& V:Values) M->SetNumberField(V.Key.ToString(),V.Value); J->SetObjectField(Key,M); };
    Floats(TEXT("opponents"),Opponents); Floats(TEXT("durability"),Durability);
    auto E=MakeShared<FJsonObject>(); for(const auto& V:Equipment) E->SetStringField(V.Key.ToString(),V.Value.ToString()); J->SetObjectField(TEXT("equipment"),E);
    auto Set=[&](const TCHAR* Key,const TSet<FName>& Values){ TArray<TSharedPtr<FJsonValue>> A; for(FName V:Values) A.Add(MakeShared<FJsonValueString>(V.ToString())); J->SetArrayField(Key,A); };
    Set(TEXT("discovered"),Discovered); Set(TEXT("activated"),Activated); Set(TEXT("claimed"),Claimed);
    TArray<TSharedPtr<FJsonValue>> A; for(const auto& V:Explored) { auto P=MakeShared<FJsonObject>(); P->SetNumberField(TEXT("x"),V.X); P->SetNumberField(TEXT("y"),V.Y); A.Add(MakeShared<FJsonValueObject>(P)); } J->SetArrayField(TEXT("explored"),A);
    J->SetStringField(TEXT("origin"),Origin.ToString());
    FString Out; FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Out)); return Out;
}
bool UHearthwardGameplayComponent::ValidateSnapshot(const FString& Json)
{
    if (Json.IsEmpty()) return true;
    const auto J=Parse(Json); if (!J) return false;
    if(Number(J,TEXT("version"))!=1) return false;
    if(J->HasField(TEXT("buildings")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Buildings;
        if(!J->TryGetArrayField(TEXT("buildings"),Buildings) || !UHearthwardBuildingComponent::Validate(*Buildings)) return false;
    }
    if(J->HasField(TEXT("companionOrder")))
    {
        FString Order;
        if(!J->TryGetStringField(TEXT("companionOrder"),Order) || (Order!=TEXT("wait") && Order!=TEXT("follow") && Order!=TEXT("attack"))) return false;
    }
    if(J->HasField(TEXT("hasWaypoint")) || J->HasField(TEXT("waypoint")))
    {
        FVector Marker; FString Value; bool HasMarker;
        if(!J->TryGetBoolField(TEXT("hasWaypoint"),HasMarker) || !J->TryGetStringField(TEXT("waypoint"),Value) || !Marker.InitFromString(Value) || Marker.ContainsNaN()) return false;
    }
    for (const auto Key : {TEXT("health"),TEXT("hunger"),TEXT("stamina"),TEXT("experience"),TEXT("campTier")})
    { double V; if (!J->TryGetNumberField(Key,V) || !FMath::IsFinite(V) || V<0) return false; }
    for (const auto Key : {TEXT("skills"),TEXT("events"),TEXT("equipment"),TEXT("opponents"),TEXT("durability")}) if (!J->HasTypedField<EJson::Object>(Key)) return false;
    for (const auto Key : {TEXT("discovered"),TEXT("activated"),TEXT("claimed"),TEXT("explored")}) if (!J->HasTypedField<EJson::Array>(Key)) return false;
    for(const auto& S:J->GetObjectField(TEXT("skills"))->Values)
    { const auto R=Find(TEXT("skills"),FString(*S.Key)); double V; if (!R || !S.Value->TryGetNumber(V) || V<0 || V>Number(R,TEXT("maxRank")) || V!=FMath::FloorToDouble(V)) return false; }
    if(Number(J,TEXT("hunger"))>100 || Number(J,TEXT("campTier"))<1 || Number(J,TEXT("campTier"))>8 ||
        Number(J,TEXT("campTier"))!=FMath::FloorToDouble(Number(J,TEXT("campTier"))) ||
        Number(J,TEXT("experience"))>MAX_int32 || Number(J,TEXT("experience"))!=FMath::FloorToDouble(Number(J,TEXT("experience")))) return false;
    for(const auto& V:J->GetObjectField(TEXT("events"))->Values)
    { double N; if(!V.Value->TryGetNumber(N) || !FMath::IsFinite(N) || N<0 || N>MAX_int32 || N!=FMath::FloorToDouble(N)) return false; }
    for(const auto Key:{TEXT("opponents"),TEXT("durability")})
        for(const auto& V:J->GetObjectField(Key)->Values)
        {
            const bool Enemy=FString(Key)==TEXT("opponents");
            const auto R=Find(Enemy?TEXT("encounters"):TEXT("items"),FString(*V.Key)); double N;
            if(!R || !V.Value->TryGetNumber(N) || !FMath::IsFinite(N) || N<0 || N>Number(R,Enemy?TEXT("health"):TEXT("durability"),100)) return false;
        }
    for(const auto& V:J->GetObjectField(TEXT("equipment"))->Values)
    {
        FString Id; if(!V.Value->TryGetString(Id)) return false;
        const auto R=Find(TEXT("items"),Id);
        if(!R || Text(R,TEXT("slot"))!=FString(*V.Key) || !J->GetObjectField(TEXT("durability"))->HasField(Id)) return false;
    }
    for(const auto Key:{TEXT("discovered"),TEXT("activated"),TEXT("claimed")})
        for(const auto& V:J->GetArrayField(Key))
        { FString Id; if(!V->TryGetString(Id) || !Find(FString(Key)==TEXT("claimed")?TEXT("quests"):TEXT("locations"),Id)) return false; }
    for(const auto& V:J->GetArrayField(TEXT("explored")))
    {
        if(V->Type!=EJson::Object) return false;
        for(const auto Key:{TEXT("x"),TEXT("y")})
        { double N; if(!V->AsObject()->TryGetNumberField(Key,N) || !FMath::IsFinite(N)) return false; }
    }
    const FString Tracked=Text(J,TEXT("tracked"));
    if(Tracked!=TEXT("None") && !Find(TEXT("quests"),Tracked)) return false;
    FVector P; return J->HasTypedField<EJson::Boolean>(TEXT("enabled")) && J->HasTypedField<EJson::String>(TEXT("tracked")) && P.InitFromString(Text(J,TEXT("origin"))) && !P.ContainsNaN();
}
void UHearthwardGameplayComponent::Restore(const FString& Json)
{
    if(auto* B=GetOwner()->FindComponentByClass<UHearthwardBuildingComponent>())
    {
        const auto Saved=Parse(Json); const TArray<TSharedPtr<FJsonValue>>* Buildings;
        B->Restore(Saved && Saved->TryGetArrayField(TEXT("buildings"),Buildings)?*Buildings:TArray<TSharedPtr<FJsonValue>>());
    }
    for(auto& A:LandmarkActors) if(A.IsValid()) A->Destroy();
    for(auto& A:OpponentActors) if(A.Value.IsValid()) A.Value->Destroy();
    LandmarkActors.Reset(); OpponentActors.Reset(); Stunned.Reset();
    Skills.Reset(); Equipment.Reset(); Discovered.Reset(); Activated.Reset(); Claimed.Reset(); Events.Reset(); Explored.Reset(); Opponents.Reset(); Durability.Reset();
    Sprinting=false; RecoveryDelay=0; ExploreDelay=0; AttackDelay=EnemyAttackDelay=CombatRemaining=CompanionAttackDelay=0; Feedback.Reset();
    CompanionOrder=TEXT("wait"); HasWaypoint=false; Waypoint=FVector::ZeroVector;
    if (Json.IsEmpty()) { Health=Hunger=Stamina=100; Experience=0; CampTier=1; Enabled=false; TrackedQuest=TEXT("ember"); return; }
    const auto J=Parse(Json);
    if(J->HasField(TEXT("companionOrder"))) CompanionOrder=FName(*Text(J,TEXT("companionOrder")));
    if(J->HasField(TEXT("hasWaypoint"))) { HasWaypoint=J->GetBoolField(TEXT("hasWaypoint")); Waypoint.InitFromString(Text(J,TEXT("waypoint"))); }
    Health=Number(J,TEXT("health")); Hunger=Number(J,TEXT("hunger")); Stamina=Number(J,TEXT("stamina")); Experience=Number(J,TEXT("experience")); CampTier=Number(J,TEXT("campTier"));
    Enabled=J->GetBoolField(TEXT("enabled")); TrackedQuest=FName(*Text(J,TEXT("tracked"))); Origin.InitFromString(Text(J,TEXT("origin")));
    auto Map=[&](const TCHAR* Key,TMap<FName,int32>& Values){ for(const auto& V:J->GetObjectField(Key)->Values) Values.Add(FName(*V.Key),V.Value->AsNumber()); };
    Map(TEXT("skills"),Skills); Map(TEXT("events"),Events);
    for(const auto& V:J->GetObjectField(TEXT("opponents"))->Values) Opponents.Add(FName(*V.Key),V.Value->AsNumber());
    for(const auto& V:J->GetObjectField(TEXT("durability"))->Values) Durability.Add(FName(*V.Key),V.Value->AsNumber());
    for(const auto& V:J->GetObjectField(TEXT("equipment"))->Values) Equipment.Add(FName(*V.Key),FName(*V.Value->AsString()));
    auto Set=[&](const TCHAR* Key,TSet<FName>& Values){ for(const auto& V:J->GetArrayField(Key)) Values.Add(FName(*V->AsString())); };
    Set(TEXT("discovered"),Discovered); Set(TEXT("activated"),Activated); Set(TEXT("claimed"),Claimed);
    for(const auto& V:J->GetArrayField(TEXT("explored"))) Explored.Add(FVector2D(Number(V->AsObject(),TEXT("x")),Number(V->AsObject(),TEXT("y"))));
    if(Enabled) CreateLandmarks();
}
