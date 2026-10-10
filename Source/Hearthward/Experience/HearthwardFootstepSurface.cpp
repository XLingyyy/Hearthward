#include "HearthwardFootstepSurface.h"
#include "../Building/HearthwardTask028CampHouse.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "UObject/UnrealType.h"

namespace
{
constexpr int32 TerrainSide=2017;
constexpr TCHAR LandscapeMaterial[]=TEXT("/Game/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_Landscape.M_Landscape");
const TArray<uint8>& TerrainSurfaces()
{
    // Read once. No synchronous disk access on subsequent landscape contacts.
    static const TArray<uint8> Cells=[]()
    {
        TArray<uint8> Bytes,Result;
        const FString Path=FPaths::ProjectDir()/TEXT("Resources/Audio/TASK-104/terrain-surfaces.rle");
        if(!FFileHelper::LoadFileToArray(Bytes,*Path) || Bytes.Num()<8
            || FMemory::Memcmp(Bytes.GetData(),"HWS1",4)!=0)return Result;
        const uint32 Side=uint32(Bytes[4])|(uint32(Bytes[5])<<8)|(uint32(Bytes[6])<<16)|(uint32(Bytes[7])<<24);
        if(Side!=TerrainSide || (Bytes.Num()-8)%3!=0)return Result;
        Result.Reserve(TerrainSide*TerrainSide);
        for(int32 Offset=8;Offset<Bytes.Num();Offset+=3)
        {
            const uint8 Surface=Bytes[Offset];const int32 Count=Bytes[Offset+1]|(int32(Bytes[Offset+2])<<8);
            if(Surface<1 || Surface>3 || Count==0 || Count>TerrainSide*TerrainSide-Result.Num())return TArray<uint8>();
            const int32 Start=Result.AddUninitialized(Count);FMemory::Memset(Result.GetData()+Start,Surface,Count);
        }
        if(Result.Num()!=TerrainSide*TerrainSide)return TArray<uint8>();
        return Result;
    }();
    return Cells;
}
FString BasePath(UMaterialInterface* Material)
{
    const UMaterial* Base=Material?Material->GetBaseMaterial():nullptr;
    return Base?Base->GetPathName():FString();
}
}

FName HearthwardFootstepSurface::Canonical(FName Surface)
{
    for(const FName Known:{FName(TEXT("grass")),FName(TEXT("dirt")),FName(TEXT("stone")),FName(TEXT("wood"))})
        if(Surface==Known)return Known;
    return NAME_None;
}
FName HearthwardFootstepSurface::EventFor(FName Surface)
{
    Surface=Canonical(Surface);
    return Surface.IsNone()?FName(TEXT("movement.footstep")):FName(*(FString(TEXT("movement.footstep."))+Surface.ToString()));
}
int32 HearthwardFootstepSurface::SelectVariant(int32 Count,int32 Previous,int32 Draw)
{
    if(Count<=0)return INDEX_NONE;
    if(Count==1)return 0;
    const bool Exclude=Previous>=0 && Previous<Count;
    int32 Selected=FMath::Clamp(Draw,0,Count-(Exclude?2:1));
    if(Exclude && Selected>=Previous)++Selected;
    return Selected;
}
FName HearthwardFootstepSurface::FromMaterialPath(const FString& Path)
{
    // Exact assets actually used by the fortress, rocks and original natural-world kit.
    if(Path==TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber.M_OldTimber")
        || Path==TEXT("/Game/Hearthward/Assets/TASK-028/house/backing/M_FloorBacking.M_FloorBacking"))return TEXT("wood");
    if(Path==TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_RoughStone.M_RoughStone")
        || Path==TEXT("/Game/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_RockScan.M_RockScan"))return TEXT("stone");
    return NAME_None;
}
FName HearthwardFootstepSurface::TerrainAt(const FVector& Point)
{
    if(Point.ContainsNaN() || Point.X < -201600 || Point.X > 201600 || Point.Y < -201600 || Point.Y > 201600)return NAME_None;
    const auto& Cells=TerrainSurfaces();if(Cells.IsEmpty())return NAME_None;
    // Matches M_Landscape UV=(WorldXY+201600)/403200; texel centers, no Y inversion.
    const int32 X=FMath::Clamp(FMath::FloorToInt((Point.X+201600)/403200*TerrainSide),0,TerrainSide-1);
    const int32 Y=FMath::Clamp(FMath::FloorToInt((Point.Y+201600)/403200*TerrainSide),0,TerrainSide-1);
    const uint8 Surface=Cells[Y*TerrainSide+X];
    return Surface==1?FName(TEXT("grass")):Surface==2?FName(TEXT("dirt")):FName(TEXT("stone"));
}
FName HearthwardFootstepSurface::Resolve(const FHitResult& Hit)
{
    if(!Hit.bBlockingHit || Hit.bStartPenetrating)return NAME_None;
    // Explicit non-default engine physical surfaces win. Unknown configured surfaces
    // use the neutral cue rather than silently inheriting an unrelated visual material.
    if(const auto* Physical=Hit.PhysMaterial.Get())
    {
        const EPhysicalSurface Type=Physical->SurfaceType;
        if(Type!=SurfaceType_Default)
        {
            for(const auto& Config:UPhysicsSettings::Get()->PhysicalSurfaces)
                if(Config.Type==Type)return Canonical(Config.Name);
            return NAME_None;
        }
    }
    const auto* Component=Hit.GetComponent();const auto* Actor=Hit.GetActor();
    if(!Component || !Actor)return NAME_None;
    // These collision-only boxes cover the actual camp timber floor and entrance steps.
    if(Cast<AHearthwardTask028CampHouse>(Actor))
    {
        const FName Part=Component->GetFName();
        if(Part==TEXT("WalkableFloor") || Part==TEXT("StepLow") || Part==TEXT("StepMiddle") || Part==TEXT("StepHigh"))return TEXT("wood");
    }
    int32 Section=INDEX_NONE;
    UMaterialInterface* Material=Hit.FaceIndex>=0?Component->GetMaterialFromCollisionFaceIndex(Hit.FaceIndex,Section):nullptr;
    if(!Material && Component->GetNumMaterials()==1)Material=Component->GetMaterial(0);
    const FString Path=BasePath(Material);
    const FName Mapped=FromMaterialPath(Path);if(!Mapped.IsNone())return Mapped;
    // Avoid a Landscape module dependency: inspect only the engine's exact base class
    // and its real assigned LandscapeMaterial property. Static meshes never sample this grid.
    bool IsLandscape=false;
    for(const UClass* Class=Actor->GetClass();Class;Class=Class->GetSuperClass())
        if(Class->GetPathName()==TEXT("/Script/Landscape.LandscapeProxy")){IsLandscape=true;break;}
    if(IsLandscape)
    {
        FString TerrainPath=Path;
        if(const auto* Property=FindFProperty<FObjectPropertyBase>(Actor->GetClass(),TEXT("LandscapeMaterial")))
            if(auto* Assigned=Cast<UMaterialInterface>(Property->GetObjectPropertyValue_InContainer(Actor)))TerrainPath=BasePath(Assigned);
        if(TerrainPath==LandscapeMaterial)return TerrainAt(Hit.ImpactPoint);
    }
    return NAME_None;
}
