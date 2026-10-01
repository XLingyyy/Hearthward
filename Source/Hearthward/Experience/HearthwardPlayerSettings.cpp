#include "HearthwardPlayerSettings.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Misc/ConfigCacheIni.h"
#include "SceneViewExtension.h"
#include "SceneView.h"
#include "SceneInterface.h"
class FHearthwardHungerView : public FSceneViewExtensionBase
{
public:
    FHearthwardHungerView(const FAutoRegister& Register,UHearthwardPlayerSettings* Settings)
        : FSceneViewExtensionBase(Register),Owner(Settings) {}
    float Strength=0;
    virtual void SetupViewFamily(FSceneViewFamily&) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}
    virtual void SetupView(FSceneViewFamily& Family,FSceneView& View) override
    {
        if(!Owner.IsValid() || !Family.Scene || Family.Scene->GetWorld()!=Owner->GetWorld() || !View.bIsGameView || View.bIsSceneCapture || Strength<=0) return;
        const float Far=FMath::Lerp(1000000.f,8000.f,Strength),Near=View.NearClippingDistance;
        FMatrix Projection=View.ViewMatrices.GetViewToClip();
        Projection.M[2][2]=Near/(Near-Far);Projection.M[3][2]=-Far*Near/(Near-Far);
        View.UpdateProjectionMatrix(Projection);
    }
private:
    TWeakObjectPtr<UHearthwardPlayerSettings> Owner;
};
void UHearthwardPlayerSettings::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);Bindings=HearthwardInput::Load();
    HungerView=FSceneViewExtensions::NewExtension<FHearthwardHungerView>(this);
    auto Int=[&](const TCHAR* Key,int32& Value,int32 Min,int32 Max)
    { GConfig->GetInt(TEXT("Hearthward.Comfort"),Key,Value,GGameUserSettingsIni);Value=FMath::Clamp(Value,Min,Max); };
    auto Bool=[&](const TCHAR* Key,bool& Value){GConfig->GetBool(TEXT("Hearthward.Comfort"),Key,Value,GGameUserSettingsIni);};
    Int(TEXT("Music"),Comfort.Music,0,100);Int(TEXT("Voice"),Comfort.Voice,0,100);
    Int(TEXT("Effects"),Comfort.Effects,0,100);Int(TEXT("Environment"),Comfort.Environment,0,100);
    Int(TEXT("TextScale"),Comfort.TextScale,100,150);Int(TEXT("SubtitleSize"),Comfort.SubtitleSize,32,48);
    Int(TEXT("SubtitleBackground"),Comfort.SubtitleBackground,0,100);Int(TEXT("Shake"),Comfort.Shake,0,100);
    Int(TEXT("HungerVisual"),Comfort.HungerVisual,0,100);Int(TEXT("FOV"),Comfort.FOV,70,110);
    Bool(TEXT("Subtitles"),Comfort.Subtitles);Bool(TEXT("SubtitleSpeaker"),Comfort.SubtitleSpeaker);Bool(TEXT("MotionBlur"),Comfort.MotionBlur);
    Bool(TEXT("SprintToggle"),Comfort.SprintToggle);Bool(TEXT("GuardToggle"),Comfort.GuardToggle);Bool(TEXT("AimToggle"),Comfort.AimToggle);
}
void UHearthwardPlayerSettings::Deinitialize() {HungerView.Reset();Super::Deinitialize();}
void UHearthwardPlayerSettings::Persist()
{
    HearthwardInput::Save(Bindings);
    auto Int=[&](const TCHAR* Key,int32 Value){GConfig->SetInt(TEXT("Hearthward.Comfort"),Key,Value,GGameUserSettingsIni);};
    auto Bool=[&](const TCHAR* Key,bool Value){GConfig->SetBool(TEXT("Hearthward.Comfort"),Key,Value,GGameUserSettingsIni);};
    Int(TEXT("Music"),Comfort.Music);Int(TEXT("Voice"),Comfort.Voice);Int(TEXT("Effects"),Comfort.Effects);Int(TEXT("Environment"),Comfort.Environment);
    Int(TEXT("TextScale"),Comfort.TextScale);Int(TEXT("SubtitleSize"),Comfort.SubtitleSize);Int(TEXT("SubtitleBackground"),Comfort.SubtitleBackground);
    Int(TEXT("Shake"),Comfort.Shake);Int(TEXT("HungerVisual"),Comfort.HungerVisual);Int(TEXT("FOV"),Comfort.FOV);
    Bool(TEXT("Subtitles"),Comfort.Subtitles);Bool(TEXT("SubtitleSpeaker"),Comfort.SubtitleSpeaker);Bool(TEXT("MotionBlur"),Comfort.MotionBlur);
    Bool(TEXT("SprintToggle"),Comfort.SprintToggle);Bool(TEXT("GuardToggle"),Comfort.GuardToggle);Bool(TEXT("AimToggle"),Comfort.AimToggle);
    GConfig->Flush(false,GGameUserSettingsIni);
}
float UHearthwardPlayerSettings::Volume(FName Channel) const
{
    int32 Master=100;GConfig->GetInt(TEXT("Hearthward.Audio"),TEXT("MasterVolume"),Master,GGameUserSettingsIni);
    const int32 Value=Channel==TEXT("music")?Comfort.Music:Channel==TEXT("voice")?Comfort.Voice:Channel==TEXT("environment")?Comfort.Environment:Comfort.Effects;
    return FMath::Clamp(Master,0,100)*Value/10000.f;
}
void UHearthwardPlayerSettings::ApplyTo(ACharacter* Character) const
{
    if(!Character) return;
    auto* Camera=Character->FindComponentByClass<UCameraComponent>();if(!Camera) return;
    const auto* Survival=Character->FindComponentByClass<UHearthwardSurvivalComponent>();
    const float Hunger=Survival && Survival->State.Severe()?Comfort.HungerVisual/100.f:0;
    HungerView->Strength=Hunger;
    Camera->SetFieldOfView(FMath::Max(70.f,Comfort.FOV-10*Hunger));
    Camera->PostProcessSettings.bOverride_MotionBlurAmount=true;Camera->PostProcessSettings.MotionBlurAmount=Comfort.MotionBlur?.5f:0;
    Camera->PostProcessSettings.bOverride_VignetteIntensity=true;Camera->PostProcessSettings.VignetteIntensity=0;
    Camera->PostProcessBlendWeight=1;
}
