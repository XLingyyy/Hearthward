#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HearthwardInteractionAlignment.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardInteractionAlignment : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardInteractionAlignment();
    bool Begin(FName Action,float Seconds,AActor* Target,FVector StandingPoint,FRotator Facing,bool Hold=false);
    bool FollowMovingTarget(AActor* Target,FVector WorldDelta);
    void Cancel();
    bool Active() const {return !Action.IsNone() || Eating;}
    class UAnimSequence* Sequence() const {return Eating?FoodClip:Clip;}
    float PoseTime() const;
    static bool CanAlign(class ACharacter* Character,FVector Point,FRotator Facing);
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UFUNCTION() void Restored();
    UPROPERTY() TObjectPtr<class UAnimSequence> Clip;
    UPROPERTY() TObjectPtr<class UAnimSequence> FoodClip;
    FName Action;
    TWeakObjectPtr<AActor> Target;
    FVector Origin;
    FGuid Epoch;
    float Health=0,Elapsed=0,Duration=0;
    bool Holding=false;
    bool Eating=false;
    float FoodTime=0;
};
