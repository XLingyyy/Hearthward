#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "HearthwardPresentationComponent.generated.h"
struct FHearthwardCombatFeedbackReceipt;
struct FHearthwardFootContactReceipt;
UCLASS(ClassGroup=(Hearthward),meta=(BlueprintSpawnableComponent))
class HEARTHWARD_API UHearthwardPresentationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHearthwardPresentationComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick) override;
    UFUNCTION(BlueprintCallable) bool PlayFixedCue(FName Id,FGuid Event,bool SharedKnowledge);
    UFUNCTION(BlueprintCallable) void StopFixedCue();
    UFUNCTION(BlueprintPure) FString GetSubtitle() const;
    UFUNCTION(BlueprintPure) FName GetCue() const {return CurrentCue;}
    UFUNCTION(BlueprintPure) FString GetVoiceStatus() const {return VoiceStatus;}
    bool FootContactSucceeded(const FHearthwardFootContactReceipt& Receipt);
private:
    friend struct FAudioFeedbackLifecycleAccess;
    friend struct FSurfaceFootstepAccess;
    UFUNCTION() void Restored();
    UFUNCTION() void StorageTransferred(FGuid Operation,bool ToCamp,FName Item,int32 Count);
    UFUNCTION() void GameplayChanged();
    UFUNCTION() void CharacterLanded(const FHitResult& Hit);
    UFUNCTION() void CharacterMovementChanged(class ACharacter* Character,EMovementMode PreviousMode,uint8 PreviousCustomMode);
    void SeedSuccessEvents();
    void ObserveProgressSuccess(bool Seed);
    void ObserveNPCSuccess(class AHearthwardCompanionFixture* Brother);
    void BindCombatSources(class AHearthwardCompanionFixture* Brother);
    void UnbindCombatSources();
    void CombatSucceeded(const FHearthwardCombatFeedbackReceipt& Receipt);
    bool PlaySoundEvent(FName Event,FGuid Operation,const FVector* Position=nullptr,bool Remember=true);
    void StopEffects();
    void InitializeEnvironment();
    void InitializeAmbientProfiles(const TSharedPtr<class FJsonObject>& Root);
    void UpdateAmbient();
    void StopAmbient();
    void RemoveFireSource(int32 Index);
    class UAudioComponent* CreateAmbientSource(FName Event,const FVector& Position,float Radius);
    void ApplyAmbientMix();
    bool PrepareEnvironmentWave();
    void CacheEnvironmentActor(class AActor* Actor);
    void EnvironmentActorSpawned(class AActor* Actor);
    void EnvironmentLevelAdded(class ULevel* Level,class UWorld* World);
    void EnvironmentLevelRemoved(class ULevel* Level,class UWorld* World);
    void UpdateEnvironment();
    void StopEnvironment();
    void ReleaseEnvironment();
    UPROPERTY() TObjectPtr<class UAudioComponent> Voice;
    UPROPERTY() TObjectPtr<class USoundWaveProcedural> Wave;
    UPROPERTY() TArray<TObjectPtr<class UAudioComponent>> Effects;
    UPROPERTY() TObjectPtr<class UAudioComponent> EnvironmentSource;
    UPROPERTY() TObjectPtr<class UHearthwardEnvironmentLoopWave> EnvironmentWave;
    // Each simultaneous source owns a separate procedural cursor via its AudioComponent.
    UPROPERTY() TArray<TObjectPtr<class UAudioComponent>> FireSources;
    UPROPERTY() TObjectPtr<class UAudioComponent> WindSource;
    TArray<TWeakObjectPtr<class UNiagaraComponent>> FireEmitters;
    TArray<TWeakObjectPtr<class AActor>> AmbientActors;
    TWeakObjectPtr<class AActor> WindAnchor;
    struct FAmbientProfile
    {
        FName Event;
        FString System;
        TArray<FName> Tags;
        float Radius=0,InnerRadius=0,Gain=0,RoofTrace=0;
        int32 MaxSources=0;
        bool Enabled=false;
    };
    FAmbientProfile FireProfile,WindProfile;
    TMap<FName,TArray<uint8>> AmbientPCM;
    TMap<FName,int32> AmbientRates;
    TSet<FName> AmbientAttempted;
    float WindWeight=0;
    TArray<FVector> EnvironmentVertices;
    TArray<FIntVector> EnvironmentTriangles;
    FString EnvironmentMesh;
    FName EnvironmentTag,EnvironmentEvent;
    bool EnvironmentWaveAttempted=false;
    TArray<TWeakObjectPtr<class UStaticMeshComponent>> EnvironmentCandidates;
    TArray<TWeakObjectPtr<class AActor>> PendingEnvironmentActors;
    TWeakObjectPtr<class UStaticMeshComponent> EnvironmentSurface;
    FDelegateHandle EnvironmentSpawnHandle,EnvironmentRegisterHandle;
    TArray<float> EffectRemaining;
    struct FCue {FString Speaker,Text,Group,ProductionStatus;};
    struct FSoundCue {FString File;FName Channel;TArray<FString> Variants;int32 LastVariant=INDEX_NONE;};
    TMap<FName,FCue> Cues;
    TMap<FName,FSoundCue> SoundCues;
    TSet<FGuid> ObservedTransfers;
    TSet<FGuid> ObservedNPCEvents;
    TMap<FGuid,FVector> ObservedCombatEvents;
    TMap<FGuid,FVector> ObservedFootContacts;
    uint64 ObservedFootFrame=0;
    TWeakObjectPtr<class UHearthwardCombatComponent> BoundCombat;
    TWeakObjectPtr<class UHearthwardSurvivalComponent> BoundSurvival,BoundBrotherSurvival;
    TMap<FName,int32> ObservedPlayerEvents;
    TMap<FName,int32> ObservedMovementEvents;
    EMovementMode ObservedMovementMode=MOVE_None;
    bool AwaitingLanding=false;
    FGuid PendingLanding;
    FVector PendingLandingPosition=FVector::ZeroVector;
    TSet<FGuid> PlayedEvents;
    FName CurrentCue;
    FGuid Epoch;
    float Remaining=0,ShakeRemaining=0,PreviousHealth=100;
    FString VoiceStatus;
    TWeakObjectPtr<class AHearthwardCompanionFixture> ObservedBrother;
    FGuid PreviousCommand;
    uint8 PreviousPhase=0;
    int32 PreviousDelivered=0;
    FName PreviousInitiative;
};
