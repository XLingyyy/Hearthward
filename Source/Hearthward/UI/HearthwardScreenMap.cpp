#include "HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "EngineUtils.h"
using namespace HearthwardData;

FVector UHearthwardScreenWidget::MapOrigin() const
{ return FVector(MapCenter.X,MapCenter.Y,0); }
FVector2D UHearthwardScreenWidget::MapWorldSize() const
{
    const auto& Size=Theme->GetObjectField(TEXT("localMap"))->GetArrayField(TEXT("sizeCm"));
    return {Size[0]->AsNumber(),Size[1]->AsNumber()};
}
FSlateRect UHearthwardScreenWidget::MapViewRect() const
{
    const FVector2D Min=(DesignSize-MapViewDesignSize)*.5;
    return FSlateRect(float(Min.X),float(Min.Y),float(Min.X+MapViewDesignSize.X),float(Min.Y+MapViewDesignSize.Y));
}
double UHearthwardScreenWidget::MapScale() const
{
    const FVector2D View=MapViewRect().GetSize(),World=MapWorldSize();
    return FMath::Max(View.X/World.X,View.Y/World.Y);
}
float UHearthwardScreenWidget::MapMinimumZoom() const
{
    // MapScale covers the viewport without stretching. Keep that coverage at
    // minimum zoom; dragging reveals the remaining part of the rectangle.
    return 1.f;
}
void UHearthwardScreenWidget::ClampMapPan()
{
    MapZoom=FMath::Clamp(MapZoom,MapMinimumZoom(),3.f);
    const FVector2D Extent=MapWorldSize()*MapScale()*MapZoom,View=MapViewRect().GetSize();
    const FVector2D Limit(FMath::Max(0.,(Extent.X-View.X)*.5),FMath::Max(0.,(Extent.Y-View.Y)*.5));
    MapPan.X=FMath::Clamp(MapPan.X,-Limit.X,Limit.X);MapPan.Y=FMath::Clamp(MapPan.Y,-Limit.Y,Limit.Y);
}
void UHearthwardScreenWidget::CenterMapRegion(FVector World)
{
    const auto Config=Theme->GetObjectField(TEXT("localMap"));
    const auto& Origin=Config->GetArrayField(TEXT("terrainOriginCm"));
    const FVector2D Min(Origin[0]->AsNumber(),Origin[1]->AsNumber()),Half=MapWorldSize()*.5;
    const double Span=Number(Config,TEXT("terrainSpanCm"),403200);
    // The rectangle stays inside the actual authored terrain, including at distant camps.
    MapCenter={FMath::Clamp(World.X,Min.X+Half.X,Min.X+Span-Half.X),FMath::Clamp(World.Y,Min.Y+Half.Y,Min.Y+Span-Half.Y)};
}
void UHearthwardScreenWidget::ResetMapView()
{
    const auto Map=Theme->GetObjectField(TEXT("localMap"));
    const auto& Center=Map->GetArrayField(TEXT("centerCm"));
    FVector Focus(Center[0]->AsNumber(),Center[1]->AsNumber(),0);
    if(const auto* Pawn=GetOwningPlayerPawn())Focus=Pawn->GetActorLocation();
    CenterMapRegion(Focus);
    MapZoom=Number(Map,TEXT("initialZoom"),1.15);MapPan=-FVector2D(Focus.X-MapCenter.X,Focus.Y-MapCenter.Y)*MapScale()*MapZoom;
    ClampMapPan();
}
void UHearthwardScreenWidget::FocusMapLocation(FName Location)
{
    const FVector World=Gameplay()->LocationPosition(Location);
    if(!MapVisible(World))CenterMapRegion(World);
    MapZoom=Number(Theme->GetObjectField(TEXT("localMap")),TEXT("initialZoom"),1.15);
    MapPan=FVector2D(WorldMap?210:0,0)-FVector2D(World.X-MapCenter.X,World.Y-MapCenter.Y)*MapScale()*MapZoom;
    ClampMapPan();
}
FVector2D UHearthwardScreenWidget::MapPoint(FVector World) const
{
    const FVector Delta=World-MapOrigin();
    return MapViewRect().GetCenter()+FVector2D(Delta.X,Delta.Y)*MapScale()*MapZoom+MapPan;
}
FVector UHearthwardScreenWidget::MapWorldAt(FVector2D Point) const
{
    const FVector2D Delta=(Point-MapViewRect().GetCenter()-MapPan)/(MapScale()*MapZoom);
    return MapOrigin()+FVector(Delta.X,Delta.Y,0);
}
bool UHearthwardScreenWidget::MapVisible(FVector World) const
{
    const FVector Delta=World-MapOrigin();const FVector2D Half=MapWorldSize()*.5;
    return FMath::Abs(Delta.X)<=Half.X+.0001 && FMath::Abs(Delta.Y)<=Half.Y+.0001;
}
bool UHearthwardScreenWidget::MapCursorInside(FVector2D Point) const
{
    const auto View=MapViewRect();
    return Point.X>=View.Left && Point.X<=View.Right && Point.Y>=View.Top && Point.Y<=View.Bottom
        && (!WorldMap || Point.X<36 || Point.X>404 || Point.Y<100 || Point.Y>810) && MapVisible(MapWorldAt(Point));
}
void UHearthwardScreenWidget::ComposeMap()
{
    ClampMapPan();
    const auto Config=Theme->GetObjectField(TEXT("localMap"));
    const auto& AtlasOrigin=Config->GetArrayField(TEXT("terrainOriginCm"));
    const double AtlasSide=Number(Config,TEXT("terrainSpanCm"),403200)*MapScale()*MapZoom;
    const FVector2D AtlasPoint=MapPoint(FVector(AtlasOrigin[0]->AsNumber(),AtlasOrigin[1]->AsNumber(),0));
    Element(TEXT("image"),TEXT(""),AtlasPoint,{AtlasSide,AtlasSide},18,TEXT(""),TEXT("mapLocalTerrain"));
    auto& Terrain=Elements.Last();Terrain.LayoutId=TEXT("map.terrain");Terrain.Component=TEXT("map.canvas");Terrain.MapClipped=true;
#if !UE_BUILD_SHIPPING
    if(MapTerrainProbe)
    {
        Element(TEXT("mapProbe"),TEXT(""),AtlasPoint,{AtlasSide,AtlasSide});
        Elements.Last().Component=TEXT("map.canvas");Elements.Last().MapClipped=true;
    }
#endif
    for(const auto& Value:Config->GetArrayField(TEXT("regions")))
    {
        const auto Region=Value->AsObject();const auto& XY=Region->GetArrayField(TEXT("xyCm"));
        const FVector World(XY[0]->AsNumber(),XY[1]->AsNumber(),0);
        if(!MapVisible(World))continue;
        const double Font=Number(Region,TEXT("font"),22);
        const auto& Offset=Region->GetArrayField(TEXT("labelOffset"));
        const FVector2D LabelOffset(Offset[0]->AsNumber(),Offset[1]->AsNumber());
        if(!LabelOffset.IsNearlyZero())
        {
            Element(TEXT("mapSite"),TEXT(""),MapPoint(World),LabelOffset);
            Elements.Last().Component=TEXT("map.canvas");Elements.Last().MapClipped=true;
        }
        Element(TEXT("mapRegion"),Text(Region,TEXT("name")),MapPoint(World)+LabelOffset-FVector2D(110,Font*.65),{220,Font*1.6},Font);
        auto& Label=Elements.Last();Label.LayoutId=TEXT("map.region.")+Text(Region,TEXT("id"));Label.Component=TEXT("map.canvas");Label.MapClipped=true;
        Label.Font=Font*FMath::Min(MapZoom,1.25f);Label.Tracking=0;Label.Align=TEXT("center");Label.FontRole=TEXT("display");
    }
    for(const FString Id:{FString(TEXT("brother")),FString(TEXT("player"))})
    {
        Element(TEXT("mapFlame"),TEXT(""),{0,0},{15,44});
        auto& Marker=Elements.Last();Marker.LayoutId=TEXT("map.")+Id;Marker.Component=TEXT("map.canvas");Marker.MapClipped=true;
        Marker.Asset=Id==TEXT("player")?TEXT("mapFlameRed"):TEXT("mapFlameBlue");
        Marker.Color=Id==TEXT("player")?FLinearColor(1.f,.035f,.018f,1):FLinearColor(.02f,.35f,1.f,1);
    }
    UpdateMapMarkers(false);
    ComposeWorldMap();
    const auto View=MapViewRect();
    Element(TEXT("menuAction"),WorldMap?TEXT("收起地点 ‹"):TEXT("地点与传送 ›"),{View.Right-316,View.Top+24},{292,54},22,WorldMap?TEXT("map.local"):TEXT("map.world"));
    Elements.Last().LayoutId=TEXT("map.locations.toggle");Elements.Last().Align=TEXT("right");
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
            const float Heading=FMath::UnwindDegrees(Actor->GetActorRotation().Yaw-90.f);
            Changed|=!FMath::IsNearlyEqual(Marker.Value,Heading,.001f);Marker.Value=Heading;
        }
    }
    if(Changed)InvalidateLayoutAndVolatility();
}
