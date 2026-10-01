#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Input/HearthwardInputBindings.h"
#include "HearthwardPlayerSettings.generated.h"
struct FHearthwardComfortSettings
{
    int32 Music=100,Voice=100,Effects=100,Environment=100;
    int32 TextScale=100,SubtitleSize=32,SubtitleBackground=70,Shake=50,HungerVisual=100,FOV=90;
    bool Subtitles=true,SubtitleSpeaker=true,MotionBlur=false,SprintToggle=false,GuardToggle=false,AimToggle=false;
};
UCLASS()
class HEARTHWARD_API UHearthwardPlayerSettings : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    FHearthwardComfortSettings Comfort;
    FHearthwardBindings Bindings;
    void Persist();
    float Volume(FName Channel) const;
    void ApplyTo(class ACharacter* Character) const;
private:
    TSharedPtr<class FHearthwardHungerView,ESPMode::ThreadSafe> HungerView;
};
