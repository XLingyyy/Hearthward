#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/HearthwardInteractionTargetComponent.h"
#include "HearthwardShortcutActor.generated.h"

UENUM()
enum class EHearthwardDeviceKind : uint8 { Latch, Ladder, Winch, Beam };
UENUM()
enum class EHearthwardDevicePhase : uint8 { Idle, Approach, Ready, Hold, Commit, Cancel };

UCLASS()
class HEARTHWARD_API UHearthwardDeviceInteraction : public UHearthwardInteractionTargetComponent
{
    GENERATED_BODY()
public:
    UHearthwardDeviceInteraction(){MaxDistance=200;}
    virtual FString GetInteractionPrompt(AActor* Interactor) const override;
    virtual FString CompleteInteraction(AActor* Interactor) override;
};

UCLASS()
class HEARTHWARD_API AHearthwardShortcutActor : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardShortcutActor();
    void Configure(FName Id,EHearthwardDeviceKind Type);
    FName Identity;
    EHearthwardDeviceKind Kind=EHearthwardDeviceKind::Latch;
    EHearthwardDevicePhase Phase=EHearthwardDevicePhase::Idle;
    TWeakObjectPtr<AHearthwardShortcutActor> LinkedGate;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Door;
    UPROPERTY() TObjectPtr<class UBoxComponent> Barrier;
    UPROPERTY() TObjectPtr<UHearthwardDeviceInteraction> Interaction;
    FString Prompt() const;
    FString Use(AActor* Player);
    bool IsOpen() const;
    bool ClimbingBy(const AActor* Player) const;
    bool Active() const {return Climber.IsValid() || Phase==EHearthwardDevicePhase::Approach || Phase==EHearthwardDevicePhase::Ready || Phase==EHearthwardDevicePhase::Hold;}
    void Cancel();
    virtual void Tick(float Delta) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    bool PersistOpen();
    bool CanClose() const;
    void ApplyState();
    bool BeginContact();
    bool BeamPathClear() const;
    FVector BeamPosition(float Progress) const;
    bool BeginClimb(class ACharacter* Character,class UPathFollowingComponent* Following=nullptr);
    void TickClimb(float Delta);
    void FinishClimb();
    void LinkReached(class UNavLinkCustomComponent* Link,UObject* Following,const FVector& Destination);
    UPROPERTY() TObjectPtr<class UNavLinkCustomComponent> LadderLink;
    TWeakObjectPtr<class ACharacter> Climber;
    TWeakObjectPtr<class UPathFollowingComponent> ClimbFollowing;
    FVector ClimbStart,ClimbUp,ClimbOver,ClimbEnd;
    float ClimbElapsed=0,ClimbHealth=0;
    TWeakObjectPtr<AActor> User;
    TWeakObjectPtr<class AHearthwardCompanionFixture> Brother;
    FGuid Epoch;
    FVector StartPosition;
    float PlayerHealth=0,BrotherHealth=0,Elapsed=0,ApproachTime=0;
    float BeamRestZ=0;
    bool TemporaryOpen=false;
    bool ContactStarted=false;
    TWeakObjectPtr<AHearthwardShortcutActor> TemporaryHolder;
};
