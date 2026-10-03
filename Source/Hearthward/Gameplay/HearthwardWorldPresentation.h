#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HearthwardWorldPresentation.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardWorldPresentation : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& World) override;
    virtual void Deinitialize() override;
    void ApplyTime(double Minute);
protected:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
private:
    void ConfigureSky(class ULevel* Level,UWorld* World);
    FDelegateHandle StreamingHandle;
    TWeakObjectPtr<class APostProcessVolume> Grade;
    TWeakObjectPtr<class ADirectionalLight> NightFill;
    TMap<TWeakObjectPtr<class UDirectionalLightComponent>,float> SunIntensity;
    double AppliedMinute=-1,AppliedSky=-1;
};
