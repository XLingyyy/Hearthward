#include "HearthwardWorldPresentation.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/Level.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

bool UHearthwardWorldPresentation::DoesSupportWorldType(EWorldType::Type Type) const
{return Type==EWorldType::Game || Type==EWorldType::PIE;}

void UHearthwardWorldPresentation::OnWorldBeginPlay(UWorld& World)
{
    Super::OnWorldBeginPlay(World);
    if(UGameplayStatics::GetCurrentLevelName(&World,true)!=TEXT("L_HearthwardWilds"))return;
    for(TActorIterator<AActor> It(&World);It;++It)
    {
        if(auto* Sun=It->FindComponentByClass<UDirectionalLightComponent>();Sun && !It->ActorHasTag(TEXT("CampaignMoonlight")))
        {
            Sun->SetLightColor(FLinearColor(1.f,.93f,.82f));
            Sun->LightSourceAngle=1.1f;
            Sun->ContactShadowLength=.15f;
            Sun->MarkRenderStateDirty();
        }
        if(auto* Fog=It->FindComponentByClass<UExponentialHeightFogComponent>())
        {
            // The terrain is about 220 m above the template fog actor. Move its layer
            // to the playable elevation so distant mountains receive aerial perspective.
            Fog->SetWorldLocation(FVector(0,0,19500));
            Fog->SetFogDensity(.012f);
            Fog->SetFogHeightFalloff(.12f);
            Fog->SetStartDistance(2500);
            Fog->SetFogMaxOpacity(.72f);
            Fog->SetVolumetricFog(true);
            Fog->SetVolumetricFogDistance(22000);
            Fog->SetVolumetricFogScatteringDistribution(.25f);
        }
    }
    auto* Volume=World.SpawnActor<APostProcessVolume>();Grade=Volume;
    Volume->bUnbound=true;Volume->Priority=20;
    auto& P=Volume->Settings;
    P.bOverride_AutoExposureMethod=true;P.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    P.bOverride_AutoExposureBias=true;P.AutoExposureBias=.25f;
    P.bOverride_AutoExposureApplyPhysicalCameraExposure=true;P.AutoExposureApplyPhysicalCameraExposure=false;
    P.bOverride_ColorSaturation=true;P.ColorSaturation=FVector4(1,1,1,.94f);
    P.bOverride_ColorContrast=true;P.ColorContrast=FVector4(1,1,1,1.04f);
    P.bOverride_ColorGainHighlights=true;P.ColorGainHighlights=FVector4(1.025f,1.005f,.97f,1);
    P.bOverride_ColorGainShadows=true;P.ColorGainShadows=FVector4(.96f,.99f,1.04f,1);
    P.bOverride_BloomIntensity=true;P.BloomIntensity=.18f;
    P.bOverride_BloomThreshold=true;P.BloomThreshold=2;
    P.bOverride_VignetteIntensity=true;P.VignetteIntensity=.12f;
    P.bOverride_AmbientOcclusionIntensity=true;P.AmbientOcclusionIntensity=.65f;
    P.bOverride_AmbientOcclusionRadius=true;P.AmbientOcclusionRadius=100;
    P.bOverride_ScreenSpaceReflectionIntensity=true;P.ScreenSpaceReflectionIntensity=80;
    P.bOverride_ScreenSpaceReflectionQuality=true;P.ScreenSpaceReflectionQuality=75;
    P.bOverride_MotionBlurAmount=true;P.MotionBlurAmount=0;
    for(auto* Level:World.GetLevels())ConfigureSky(Level,&World);
    StreamingHandle=FWorldDelegates::LevelAddedToWorld.AddUObject(this,&UHearthwardWorldPresentation::ConfigureSky);
    SetNight(World.GetSubsystem<UHearthwardCampaignSubsystem>()->State.Phase==TEXT("prologue"));
}

void UHearthwardWorldPresentation::ConfigureSky(ULevel* Level,UWorld* World)
{
    if(World!=GetWorld() || !Level || !Grade.IsValid())return;
    for(AActor* Actor:Level->Actors)if(Actor)
        if(auto* Mesh=Actor->FindComponentByClass<UStaticMeshComponent>();Mesh && Mesh->GetMaterials().ContainsByPredicate([](UMaterialInterface* Material)
            {return Material && Material->GetMaterial()->bIsSky;}))
        {
            // The template sky sphere may arrive with a streamed cell after BeginPlay.
            // SkyAtmosphere and volumetric clouds cover every restored camera height.
            Mesh->SetVisibility(false);
        }
}

void UHearthwardWorldPresentation::Deinitialize()
{
    FWorldDelegates::LevelAddedToWorld.Remove(StreamingHandle);
    Super::Deinitialize();
}

void UHearthwardWorldPresentation::SetNight(bool Night)
{
    if(!Grade.IsValid())return;
    if(Night && !NightFill.IsValid())
    {
        auto* Fill=GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector,FRotator(-55,-130,0));
        Fill->Tags.Add(TEXT("HearthwardNightFill"));
        auto* Light=Fill->GetComponent();
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetAtmosphereSunLight(false);Light->SetCastShadows(false);
        Light->SetIntensity(.55f);Light->SetLightColor(FLinearColor(.48f,.64f,1.f));
        NightFill=Fill;
    }
    else if(!Night && NightFill.IsValid()){NightFill->Destroy();NightFill.Reset();}
    for(TActorIterator<AActor> It(GetWorld());It;++It)
    {
        if(auto* Sky=It->FindComponentByClass<USkyLightComponent>())
        {
            Sky->SetRealTimeCaptureEnabled(false);
            Sky->SetIntensity(Night?.65f:1.f);
            Sky->SetLightColor(Night?FLinearColor(.46f,.61f,1.f):FLinearColor(.91f,.96f,1.f));
            Sky->SetLowerHemisphereColor(Night?FLinearColor(.025f,.035f,.055f):FLinearColor(.025f,.028f,.022f));
            Sky->RecaptureSky();
        }
        if(auto* Fog=It->FindComponentByClass<UExponentialHeightFogComponent>())
        {
            Fog->SetFogInscatteringColor(Night?FLinearColor(.022f,.036f,.075f):FLinearColor(.45f,.57f,.68f));
            Fog->SetDirectionalInscatteringColor(Night?FLinearColor(.05f,.08f,.15f):FLinearColor(.75f,.65f,.48f));
        }
    }
}
