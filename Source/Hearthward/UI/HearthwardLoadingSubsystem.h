#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "HearthwardLoadingSubsystem.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardLoadingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void BeginLoading();
    void FinishSession(bool Success);
    void InputModeChanged();
    UFUNCTION(BlueprintPure) bool IsLoading() const { return Loading; }
private:
    void BeforeMap(const FString& Map);
    void AfterMap(UWorld* World);
    bool Tick(float Delta);
    void Hide();
    TSharedPtr<class SHearthwardLoadingScreen> Screen;
    TWeakObjectPtr<class UGameViewportClient> Viewport;
    bool PreviousIgnoreInput=false;
    TWeakObjectPtr<class UCharacterMovementComponent> HeldMovement;
    uint8 PreviousMovementMode=0,PreviousCustomMode=0;
    FTSTicker::FDelegateHandle TickHandle;
    FDelegateHandle BeforeHandle,AfterHandle;
    bool Loading=false,AwaitingSession=false;
    double ReadySince=0,WorldReadySince=0,StartedAt=0;
};
