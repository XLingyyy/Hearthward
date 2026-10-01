#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardAnimalMotionComponent.generated.h"
class USkeletalMeshComponent;
class UAnimSequence;
class FJsonObject;

USTRUCT()
struct FHearthwardAnimalActionStep
{
    GENERATED_BODY()
    UPROPERTY() FName Clip;
    UPROPERTY() float MinSeconds=0;
    UPROPERTY() float MaxSeconds=0;
};

UCLASS(ClassGroup=(Hearthward),meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardAnimalMotionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardAnimalMotionComponent();
    bool Configure(FName Species,float Scale=1.f);
    void SetHabitat(const FBox& Region,bool Demo=false);
    bool Ready() const { return Mesh && !Clips.IsEmpty(); }
    bool ControlsNatureMovement() const;
    void OnFatalDamage();
    void ResetAnimal();
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick) override;
    UPROPERTY(BlueprintReadOnly) FName Species;
    UPROPERTY(BlueprintReadOnly) FString DisplayName;
    UPROPERTY(BlueprintReadOnly) FString Behavior;
    UPROPERTY(BlueprintReadOnly) FName ActiveClip;
    UPROPERTY(BlueprintReadOnly) float GroundSpeed=0;
    UPROPERTY(BlueprintReadOnly) float PlayRate=1;
    UPROPERTY(BlueprintReadOnly) bool Aquatic=false;
    UPROPERTY(BlueprintReadOnly) bool Dead=false;
    UPROPERTY(BlueprintReadOnly) int32 StateRevision=0;
    UPROPERTY(BlueprintReadOnly) int32 BoundaryCorrections=0;
    UPROPERTY(BlueprintReadOnly) int32 NaturalCycles=0;
    UPROPERTY(BlueprintReadOnly) FName LastThreat;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<USkeletalMeshComponent> Mesh;
    UPROPERTY() TMap<FName,TObjectPtr<UAnimSequence>> Clips;
    UAnimSequence* Sequence() const { return Clips.FindRef(ActiveClip); }
    bool Loops() const { return Looping.Contains(ActiveClip); }
    const FBox& Habitat() const {return Region;}
    const FBox& MovementBounds() const {return SafeBounds;}
    float DetectionRadius() const {return DetectRadius;}
    float EscapeSpeed() const;
    float GaitReferenceSpeed() const {return ReferenceSpeeds.FindRef(ActiveClip);}
private:
    TSharedPtr<FJsonObject> Profile;
    TArray<TArray<FHearthwardAnimalActionStep>> Cycles;
    TArray<FHearthwardAnimalActionStep> Queue;
    TSet<FName> Looping;
    TMap<FName,float> ReferenceSpeeds;
    FRandomStream Random;
    FName Idle,Walk,Run,Start,Stop,Alert,Hit,Collapse,Corpse;
    enum class EPhase : uint8 {Idle,Walking,Natural,Alert,Starting,Fleeing,Stopping,Hit,Falling,Dead};
    EPhase Phase=EPhase::Idle;
    FVector Home=FVector::ZeroVector,Goal=FVector::ZeroVector,Previous=FVector::ZeroVector,ThreatPosition=FVector::ZeroVector;
    FBox Region=FBox(EForceInit::ForceInit),SafeBounds=FBox(EForceInit::ForceInit);
    float Remaining=0,PhaseDuration=0,CalmTime=0,DetectRadius=800,WalkSpeed=90,RunSpeed=450,RootHeight=0,PreviousHealth=0,ReplanTime=0,BodyRadius=0;
    int32 CycleIndex=0;
    bool Demo=false,WasExternal=false;
    void SetClip(FName Clip,float Seconds=0);
    void Enter(EPhase Next,FName Clip,float Seconds=0);
    void BeginWalk();
    void BeginCycle();
    void BeginFlee();
    void AdvancePhase();
    void Move(float Delta,float Speed);
    bool Ground(FVector P,FVector& Result) const;
    AActor* NearestThreat(float& Distance) const;
    void AlignMesh();
};
