#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardGameplayComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHearthwardGameplayChanged);

UCLASS(ClassGroup=(Hearthward), meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardGameplayComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardGameplayComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* Function) override;
    UPROPERTY(BlueprintReadOnly) float Health = 100;
    UPROPERTY(BlueprintReadOnly) float Hunger = 100;
    UPROPERTY(BlueprintReadOnly) float Stamina = 100;
    UPROPERTY(BlueprintReadOnly) int32 Experience = 0;
    UPROPERTY(BlueprintReadOnly) int32 CampTier = 1;
    UPROPERTY(BlueprintReadOnly) TMap<FName, int32> Skills;
    UPROPERTY(BlueprintReadOnly) TMap<FName, FName> Equipment;
    UPROPERTY(BlueprintReadOnly) TSet<FName> Discovered;
    UPROPERTY(BlueprintReadOnly) TSet<FName> Activated;
    UPROPERTY(BlueprintReadOnly) TSet<FName> Claimed;
    UPROPERTY(BlueprintReadOnly) TMap<FName, int32> Events;
    UPROPERTY(BlueprintReadOnly) TMap<FName, float> Opponents;
    UPROPERTY(BlueprintReadOnly) TMap<FName, float> Durability;
    UPROPERTY(BlueprintReadOnly) TArray<FVector2D> Explored;
    UPROPERTY(BlueprintReadOnly) FName TrackedQuest = TEXT("ember");
    UPROPERTY(BlueprintReadOnly) FString Feedback;
    UPROPERTY(BlueprintReadOnly) bool Enabled = false;
    UPROPERTY(BlueprintReadOnly) FName CompanionOrder = TEXT("wait");
    UPROPERTY(BlueprintReadOnly) bool CompanionRoutineEnabled = true;
    UPROPERTY(BlueprintReadOnly) bool HasWaypoint = false;
    UPROPERTY(BlueprintReadOnly) FVector Waypoint = FVector::ZeroVector;
    UPROPERTY(BlueprintAssignable) FHearthwardGameplayChanged OnChanged;
    UFUNCTION(BlueprintPure) int32 Level() const;
    UFUNCTION(BlueprintPure) int32 SkillPoints() const;
    UFUNCTION(BlueprintPure) float MaxHealth() const;
    UFUNCTION(BlueprintPure) float MaxStamina() const;
    UFUNCTION(BlueprintPure) float Effect(FName Name) const;
    UFUNCTION(BlueprintPure) bool QuestAvailable(FName Id) const;
    UFUNCTION(BlueprintPure) int32 QuestProgress(FName Id) const;
    UFUNCTION(BlueprintPure) FName NearbyLocation() const;
    UFUNCTION(BlueprintCallable) bool Learn(FName Id);
    UFUNCTION(BlueprintCallable) void ResetSkills();
    UFUNCTION(BlueprintCallable) bool UseItem(FName Id);
    UFUNCTION(BlueprintCallable) bool Equip(FName Id);
    UFUNCTION(BlueprintCallable) bool Drop(FName Id, int32 Count);
    UFUNCTION(BlueprintCallable) bool Claim(FName Id);
    UFUNCTION(BlueprintCallable) bool Track(FName Id);
    UFUNCTION(BlueprintCallable) bool ActivateNearby();
    UFUNCTION(BlueprintCallable) bool Travel(FName Id);
    UFUNCTION(BlueprintCallable) void SetSprinting(bool Value);
    UFUNCTION(BlueprintCallable) bool SpendStamina(float Cost);
    UFUNCTION(BlueprintCallable) void ApplyDamage(float Damage);
    UFUNCTION(BlueprintCallable) bool Attack();
    UFUNCTION(BlueprintCallable) bool HeavyAttack();
    UFUNCTION(BlueprintCallable) bool Shoot();
    UFUNCTION(BlueprintCallable) bool ThrowItem(FName Id);
    UFUNCTION(BlueprintCallable) bool Repair(FName Id);
    UFUNCTION(BlueprintCallable) bool OrderCompanion(FName Order);
    UFUNCTION(BlueprintPure) FString PreviewCompanionDirective(AActor* Speaker, FName Directive) const;
    UFUNCTION(BlueprintCallable) bool ApplyCompanionDirective(AActor* Speaker, FName Directive);
    UFUNCTION(BlueprintPure) FName GetCompanionTacticalIntent() const { return CompanionTacticalIntent; }
    UFUNCTION(BlueprintPure) FName GetCompanionCombatTarget() const { return CompanionCombatTarget; }
    UFUNCTION(BlueprintPure) FString GetCompanionCombatReason() const { return CompanionCombatReason; }
    UFUNCTION(BlueprintPure) bool IsCompanionRoutineEnabled() const { return CompanionRoutineEnabled; }
    UFUNCTION(BlueprintPure) FName GetCompanionRoutineActivity() const { return CompanionRoutineActivity; }
    UFUNCTION(BlueprintCallable) void SetWaypoint(FVector Position);
    UFUNCTION(BlueprintPure) float AttackPower() const;
    UFUNCTION(BlueprintPure) bool InCombat() const { return CombatRemaining>0; }
    UFUNCTION(BlueprintCallable) void EnableAdventure();
    float IncomingDamage(const AActor* Target,float Seconds) const;
    bool IsRunning() const { return Sprinting && Stamina>0 && GetOwner()->GetVelocity().Size2D()>5; }
    void Record(FName Kind, FName Target, int32 Count = 1);
    void CommitOpponentHealth(FName Target,float Health,float PreviousHealth=-1);
    bool CommitEquipment(FName Id);
    void NotifyCombat() { CombatRemaining=3; }
    FString SaveSnapshot() const;
    static bool ValidateSnapshot(const FString& Json);
    void Restore(const FString& Json);
    FVector LocationPosition(FName Id) const;
    UFUNCTION() void InventoryChanged();
private:

    void DamageOpponent(FName Target,float Damage,AActor* Source);
    void TickCompanion(float Delta);
    TMap<FName,float> Stunned;
    void CreateLandmarks();
    TArray<TWeakObjectPtr<AActor>> LandmarkActors;
    TMap<FName,TWeakObjectPtr<AActor>> OpponentActors;
    float AttackDelay=0,EnemyAttackDelay=0,CombatRemaining=0,CompanionAttackDelay=0;
    FName CompanionTacticalIntent = TEXT("hold");
    FName CompanionCombatTarget;
    FString CompanionCombatReason = TEXT("EXPLICIT_HOLD");
    FName CompanionRoutineActivity;
    class UHearthwardInventoryComponent* Inventory() const;
    bool Result(bool Success, const FString& Message);
    double RecoveryDelay = 0;
    double ExploreDelay = 0;
    bool Sprinting = false;
    FVector Origin = FVector::ZeroVector;
};
