#include "HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "EngineUtils.h"
using namespace HearthwardData;

FVector UHearthwardScreenWidget::MapOrigin() const
{
    const auto& Center=Theme->GetObjectField(TEXT("localMap"))->GetArrayField(TEXT("centerCm"));
    return FVector(Center[0]->AsNumber(),Center[1]->AsNumber(),0);
}
double UHearthwardScreenWidget::MapRadius() const
{ return Number(Theme->GetObjectField(TEXT("localMap")),TEXT("radiusCm"),25000); }
double UHearthwardScreenWidget::MapBoundaryFraction(double Angle,double* Feather) const
{
    if(Feather)*Feather=1;
    const auto Map=Theme->GetObjectField(TEXT("localMap"));
    const TArray<TSharedPtr<FJsonValue>>* Outline=nullptr;
    if(!Map->TryGetArrayField(TEXT("outlineFractions"),Outline) || Outline->Num()<3)return 1;
    const int32 Count=Outline->Num();
    const double Sample=FMath::Fmod(FMath::Fmod(Angle/(2*PI),1.)+1.,1.)*Count;
    const int32 Index=FMath::FloorToInt(Sample);
    const auto At=[&](int32 I){return (*Outline)[(I+Count)%Count]->AsNumber();};
    const TArray<TSharedPtr<FJsonValue>>* FeatherPoints=nullptr;
    if(Feather && Map->TryGetArrayField(TEXT("outlineFeatherFractions"),FeatherPoints) && FeatherPoints->Num()==Count)
        *Feather=FMath::Clamp(FMath::Lerp((*FeatherPoints)[Index]->AsNumber(),(*FeatherPoints)[(Index+1)%Count]->AsNumber(),Sample-Index),.2,1.);
    const TArray<TSharedPtr<FJsonValue>>* Angular=nullptr;
    if(Map->TryGetArrayField(TEXT("outlineAngularSegments"),Angular))
        for(const auto& Value:*Angular)if(Value->AsNumber()==Index)
        {
            const double AAngle=Index*2*PI/Count,BAngle=(Index+1)*2*PI/Count;
            const FVector2D A(FMath::Cos(AAngle)*At(Index),FMath::Sin(AAngle)*At(Index));
            const FVector2D B(FMath::Cos(BAngle)*At(Index+1),FMath::Sin(BAngle)*At(Index+1));
            const FVector2D Edge=B-A,Ray(FMath::Cos(Angle),FMath::Sin(Angle));
            // Ray/segment intersection gives an actual straight edge with a sharp corner.
            return FMath::Clamp((A.X*Edge.Y-A.Y*Edge.X)/(Ray.X*Edge.Y-Ray.Y*Edge.X),.8,1.);
        }
    // Smooth sections join the straight sections at the same radial knots.
    // A positive bounded radial contour stays connected inside the observed circle.
    return FMath::Clamp(FMath::CubicInterp(At(Index),(At(Index+1)-At(Index-1))*.5,
        At(Index+1),(At(Index+2)-At(Index))*.5,Sample-Index),.8,1.);
}
FVector2D UHearthwardScreenWidget::MapPoint(FVector World) const
{
    const FVector Delta=World-MapOrigin();
    const double Scale=Number(Theme->GetObjectField(TEXT("localMap")),TEXT("diameterPixels"),850)/(2*MapRadius());
    // Same orientation as the overhead camera: +X right, +Y down. Z never changes map scale.
    return DesignSize*.5+FVector2D(Delta.X,Delta.Y)*Scale*MapZoom+MapPan;
}
bool UHearthwardScreenWidget::MapVisible(FVector World) const
{
    const FVector Delta=World-MapOrigin();
    const double Boundary=MapRadius()*MapBoundaryFraction(FMath::Atan2(Delta.Y,Delta.X));
    return Delta.Size2D()<=Boundary+.0001;
}
double UHearthwardScreenWidget::MapFogTime() const
{
#if !UE_BUILD_SHIPPING
    if(MapFogCaptureTime>=0)return MapFogCaptureTime;
#endif
    return FPlatformTime::Seconds();
}
void UHearthwardScreenWidget::ComposeMap()
{
    const auto Config=Theme->GetObjectField(TEXT("localMap"));
    const double Diameter=Number(Config,TEXT("diameterPixels"),850)*MapZoom;
    Element(TEXT("image"),TEXT(""),DesignSize*.5+MapPan-FVector2D(Diameter*.5,Diameter*.5),{Diameter,Diameter},18,TEXT(""),TEXT("mapLocalTerrain"));
    auto& Terrain=Elements.Last();Terrain.LayoutId=TEXT("map.terrain");Terrain.Component=TEXT("map.canvas");Terrain.MapClipped=true;
    for(const auto& Value:Config->GetArrayField(TEXT("regions")))
    {
        const auto Region=Value->AsObject();const auto& XY=Region->GetArrayField(TEXT("xyCm"));
        const FVector World(XY[0]->AsNumber(),XY[1]->AsNumber(),0);
        if(!MapVisible(World))continue;
        const double Font=Number(Region,TEXT("font"),22);
        Element(TEXT("mapRegion"),Text(Region,TEXT("name")),MapPoint(World)-FVector2D(110,Font*.65),{220,Font*1.6},Font);
        auto& Label=Elements.Last();Label.LayoutId=TEXT("map.region.")+Text(Region,TEXT("id"));Label.Component=TEXT("map.canvas");Label.MapClipped=true;
        Label.Font=Font*FMath::Min(MapZoom,1.25f);Label.Tracking=0;Label.Align=TEXT("center");Label.FontRole=TEXT("display");
    }
    // Both anchors are their actor's actual ground position. Never offset, clamp or invent a companion position.
    for(const FString Id:{FString(TEXT("brother")),FString(TEXT("player"))})
    {
        Element(TEXT("mapFlame"),TEXT(""),{0,0},{15,44});
        auto& Marker=Elements.Last();Marker.LayoutId=TEXT("map.")+Id;Marker.Component=TEXT("map.canvas");Marker.MapClipped=true;
        Marker.Asset=Id==TEXT("player")?TEXT("mapFlameRed"):TEXT("mapFlameBlue");
        Marker.Color=Id==TEXT("player")?FLinearColor(1.f,.035f,.018f,1):FLinearColor(.02f,.35f,1.f,1);
    }
    UpdateMapMarkers(false);
#if !UE_BUILD_SHIPPING
    // An explicit capture-only sentinel tests opacity without exposing any real geography.
    if(MapFogProbe)
    {
        Element(TEXT("mapProbe"),TEXT(""),FVector2D::ZeroVector,DesignSize);
        auto& Probe=Elements.Last();Probe.Component=TEXT("map.canvas");Probe.MapClipped=true;
    }
#endif
    Element(TEXT("mapFog"),TEXT(""),MapPoint(MapOrigin()),{Diameter*.5,Diameter*.5});
    auto& Fog=Elements.Last();Fog.LayoutId=TEXT("map.fog");Fog.Component=TEXT("map.canvas");Fog.MapClipped=true;
    Fog.Asset=TEXT("mapDarkFog");
}
void UHearthwardScreenWidget::UpdateMapMarkers(bool ApplyComponent)
{
    bool Changed=false;
    AActor* Brother=nullptr;
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        if(IsValid(*It) && !It->IsActorBeingDestroyed()){Brother=*It;break;}
    for(auto& Marker:Elements)
    {
        if(Marker.Type!=TEXT("mapFlame"))continue;
        const AActor* Actor=Marker.LayoutId==TEXT("map.player")?GetOwningPlayerPawn():Brother;
        const bool Hidden=!IsValid(Actor) || !MapVisible(Actor->GetActorLocation());
        Changed|=Marker.Hidden!=Hidden;Marker.Hidden=Hidden;
        if(!Marker.Hidden)
        {
            const FVector2D Position=MapPoint(Actor->GetActorLocation())-FVector2D(Marker.Size.X*.5,Marker.Size.Y*.9);
            const FVector2D NewPosition=ApplyComponent?ComponentPoint(TEXT("map.canvas"),Position):Position;
            Changed|=!Marker.Position.Equals(NewPosition,.001);Marker.Position=NewPosition;
        }
    }
    if(Changed)InvalidateLayoutAndVolatility();
}
