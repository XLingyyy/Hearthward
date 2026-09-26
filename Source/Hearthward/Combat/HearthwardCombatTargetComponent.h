#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardCombatTargetComponent.generated.h"

USTRUCT()
struct FHearthwardCombatTargetSave
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() FName Region;
    UPROPERTY() int32 Generation=1;
    UPROPERTY() float Health=0;
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() FRotator Rotation=FRotator::ZeroRotator;
    UPROPERTY() TMap<FName,float> ArmorDurability;
    UPROPERTY() TMap<FName,float> Detection;
    UPROPERTY() TMap<FName,FVector> LastKnown;
    UPROPERTY() TSet<FName> Seen;
    UPROPERTY() TSet<FName> BroadcastRegions;
    UPROPERTY() FName ReportingBody;
    UPROPERTY() double ReportRemaining=0;
    UPROPERTY() double HitRemaining=0;
    UPROPERTY() double InvestigationRemaining=0;
    UPROPERTY() FVector Investigation=FVector::ZeroVector;
};
UCLASS(ClassGroup=(Hearthward),meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardCombatTargetComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName Region=TEXT("prototype");
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaximumHealth=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float Health=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool Heavy=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool Protected=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TMap<FName,float> Armor;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TMap<FName,float> ArmorDurability;
    UPROPERTY(EditAnywhere) TMap<FName,FName> BoneParts;
    UPROPERTY(EditAnywhere) float Exposure=-1;
    UPROPERTY() bool PrototypeEncounter=false;
    UPROPERTY(BlueprintReadOnly) FString Awareness=TEXT("巡逻");
    UPROPERTY() FHearthwardCombatTargetSave Memory;
    TWeakObjectPtr<AActor> ExecutionOwner;
    TWeakObjectPtr<AActor> Carrier;
    TSet<FGuid> DamageIds;
    bool Alive() const { return Health>0 && !Protected; }
    UFUNCTION(BlueprintPure) bool CanAct() const { return Alive() && !ExecutionOwner.IsValid() && Memory.HitRemaining<=0; }
    FName HitPart(const FHitResult& Hit) const;
    void CreateBodyCollision();
    void SetCorpse();
    FHearthwardCombatTargetSave Snapshot() const;
    void Restore(const FHearthwardCombatTargetSave& Saved);
};
