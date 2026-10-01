#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Blueprint/UserWidget.h"
#include "HearthwardAnimalDemo.generated.h"
class UHearthwardAnimalMotionComponent;
class UHearthwardCombatTargetComponent;
class AHearthwardCompanionFixture;
class ACameraActor;
class UBoxComponent;

UCLASS()
class HEARTHWARD_API AHearthwardAnimalDemoActor : public AActor
{
    GENERATED_BODY()
public:
    AHearthwardAnimalDemoActor();
    bool Configure(FName Species,const FBox& Bounds);
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UHearthwardAnimalMotionComponent> Motion;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UHearthwardCombatTargetComponent> Combat;
private:
    UPROPERTY() TObjectPtr<UBoxComponent> Body;
};

UCLASS()
class HEARTHWARD_API AHearthwardAnimalDemoGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AHearthwardAnimalDemoGameMode();
    virtual void StartPlay() override;
    virtual void Tick(float Delta) override;
    void NextAnimal();
    void ReturnToPlayer();
    void BrotherApproach();
    void LethalSelected();
    void ResetAnimals();
    AHearthwardAnimalDemoActor* Selected() const;
    UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<AHearthwardAnimalDemoActor>> Animals;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AHearthwardCompanionFixture> Brother;
    UPROPERTY(BlueprintReadOnly) bool BrotherWalking=false;
    UPROPERTY(BlueprintReadOnly) bool Observing=false;
    UPROPERTY(BlueprintReadOnly) int32 SelectedIndex=-1;
    FString Feedback;
    const FBox LandBounds=FBox(FVector(-2200,-1600,-100),FVector(2200,1600,1000));
    const FBox WaterBounds=FBox(FVector(2600,-1200,-215),FVector(3800,1200,-15));
private:
    UPROPERTY() TObjectPtr<ACameraActor> ObservationCamera;
    void BuildHabitat();
    void TickVerification(float Delta);
    bool Verify=false;
    double VerifyTime=0,SampleAt=0;
    float VerifyPlayerSprintSpeed=0,VerifyBoostedSprintSpeed=0;
    TMap<FName,int32> VerifyOriginalSkills;
    int32 VerifyStage=-1,VerifySpecies=0;
    FVector DeadAt=FVector::ZeroVector;
    TArray<TSharedPtr<class FJsonValue>> Checks;
    TMap<FName,TSet<FName>> SeenClips;
    TMap<FName,FTransform> FirstBone;
    TSet<FName> PoseChanged;
    TSet<FName> BoundsFailed;
    void Check(FString Name,bool Passed,FString Detail=FString());
    void WriteVerification();
};

UCLASS()
class HEARTHWARD_API AHearthwardAnimalDemoController : public APlayerController
{
    GENERATED_BODY()
    virtual void SetupInputComponent() override;
    void Next();void Back();void Brother();void Kill();void Reset();void Exit();
};

UCLASS()
class HEARTHWARD_API UHearthwardAnimalDemoWidget : public UUserWidget
{
    GENERATED_BODY()
    virtual void NativeConstruct() override;
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override;
    TSharedPtr<struct FCompositeFont> Typeface;
};

UCLASS()
class HEARTHWARD_API AHearthwardAnimalDemoHUD : public AHUD
{
    GENERATED_BODY()
    virtual void BeginPlay() override;
    UPROPERTY() TObjectPtr<UHearthwardAnimalDemoWidget> Widget;
};
