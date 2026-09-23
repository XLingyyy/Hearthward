#include "HearthwardTask028CampHouse.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
constexpr TCHAR Floor[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/wood_floor_panel/SM_wood_floor_panel.SM_wood_floor_panel");
constexpr TCHAR Stairs[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/wood_stairs/SM_wood_stairs.SM_wood_stairs");
constexpr TCHAR Solid[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/wood_wall_solid/SM_wood_wall_solid.SM_wood_wall_solid");
constexpr TCHAR Doorway[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/wood_wall_doorway/SM_wood_wall_doorway.SM_wood_wall_doorway");
constexpr TCHAR Window[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/wood_wall_window/SM_wood_wall_window.SM_wood_wall_window");
constexpr TCHAR Door[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/wood_door/SM_wood_door.SM_wood_door");
constexpr TCHAR Roof[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/thatched_roof_slope/SM_thatched_roof_slope.SM_thatched_roof_slope");
constexpr TCHAR Ridge[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/thatched_roof_ridge/SM_thatched_roof_ridge.SM_thatched_roof_ridge");
constexpr TCHAR FloorMaterial[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/backing/M_FloorBacking.M_FloorBacking");
constexpr TCHAR WallMaterial[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/wood_wall_solid/tripo_mat_5ab09d9b.tripo_mat_5ab09d9b");
constexpr TCHAR RoofMaterial[] = TEXT("/Game/Hearthward/Assets/TASK-028/house/backing/M_RoofBacking.M_RoofBacking");
constexpr TCHAR Bed[] = TEXT("/Game/Hearthward/Assets/TASK-028/furniture/rope_wood_bed/SM_rope_wood_bed.SM_rope_wood_bed");
constexpr TCHAR Chest[] = TEXT("/Game/Hearthward/Assets/TASK-028/furniture/wood_chest/SM_wood_chest.SM_wood_chest");
constexpr TCHAR Chair[] = TEXT("/Game/Hearthward/Assets/TASK-028/furniture/wood_chair/SM_wood_chair.SM_wood_chair");
constexpr TCHAR Lantern[] = TEXT("/Game/Hearthward/Assets/TASK-028/furniture/metal_lantern/SM_metal_lantern.SM_metal_lantern");
constexpr TCHAR Cube[] = TEXT("/Engine/BasicShapes/Cube.Cube");
}

AHearthwardTask028CampHouse::AHearthwardTask028CampHouse()
{
    HouseRoot=CreateDefaultSubobject<USceneComponent>(TEXT("HouseRoot"));
    SetRootComponent(HouseRoot);
    Tags.Add(TEXT("Hearthward.TASK-028.CampHouse"));
}

UStaticMeshComponent* AHearthwardTask028CampHouse::AddVisual(const TCHAR* Name,const TCHAR* Path,FVector Position,
                                                             FVector Scale,FRotator Rotation)
{
    auto* Mesh=NewObject<UStaticMeshComponent>(this,FName(Name));
    AddInstanceComponent(Mesh);
    Mesh->SetupAttachment(HouseRoot);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));
    if(!Mesh->GetStaticMesh()) UE_LOG(LogTemp,Error,TEXT("TASK-028 missing mesh: %s"),Path);
    Mesh->SetRelativeLocation(Position);
    Mesh->SetRelativeRotation(Rotation);
    Mesh->SetRelativeScale3D(Scale);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
    Mesh->RegisterComponent();
    return Mesh;
}

void AHearthwardTask028CampHouse::AddBlockingBox(const TCHAR* Name,FVector Position,FVector Extent)
{
    auto* Box=NewObject<UBoxComponent>(this,FName(Name));
    AddInstanceComponent(Box);
    Box->SetupAttachment(HouseRoot);
    Box->SetBoxExtent(Extent);
    Box->SetRelativeLocation(Position);
    Box->SetCollisionProfileName(TEXT("BlockAll"));
    Box->RegisterComponent();
}

void AHearthwardTask028CampHouse::BeginPlay()
{
    Super::BeginPlay();
    // 2 x 2 floor modules form a 5.44 m square; the door faces the existing camp.
    int32 Index=0;
    // Keep the backing surface below the plank tops to avoid depth fighting.
    auto* FloorLiner=AddVisual(TEXT("FloorLiner"),Cube,FVector(0,0,43),FVector(5.6,5.6,.12));
    if(auto* Material=LoadObject<UMaterialInterface>(nullptr,FloorMaterial)) FloorLiner->SetMaterial(0,Material);
    else UE_LOG(LogTemp,Error,TEXT("TASK-028 missing floor material: %s"),FloorMaterial);
    for(const float X:{-136.f,136.f}) for(const float Y:{-136.f,136.f})
    {
        const FString Name=FString::Printf(TEXT("Floor_%d"),Index++);
        AddVisual(*Name,Floor,FVector(X,Y,35),FVector(2.4,2.4,2.1));
    }
    AddBlockingBox(TEXT("WalkableFloor"),FVector(0,0,45),FVector(272,272,8));

    const FVector WallScale(.5,3,3); // The single-view wall sources need a shallow depth.
    AddVisual(TEXT("FrontDoorway"),Doorway,FVector(270,-136,167),WallScale);
    AddVisual(TEXT("FrontWindow"),Window,FVector(270,136,167),WallScale);
    AddVisual(TEXT("BackLeft"),Solid,FVector(-270,-136,167),WallScale);
    AddVisual(TEXT("BackRight"),Solid,FVector(-270,136,167),WallScale);
    AddVisual(TEXT("SideLeftFront"),Window,FVector(136,-270,167),WallScale,FRotator(0,90,0));
    AddVisual(TEXT("SideLeftBack"),Solid,FVector(-136,-270,167),WallScale,FRotator(0,90,0));
    AddVisual(TEXT("SideRightFront"),Solid,FVector(136,270,167),WallScale,FRotator(0,90,0));
    AddVisual(TEXT("SideRightBack"),Solid,FVector(-136,270,167),WallScale,FRotator(0,90,0));
    AddBlockingBox(TEXT("BackWall"),FVector(-270,0,167),FVector(15,272,115));
    AddBlockingBox(TEXT("LeftWall"),FVector(0,-270,167),FVector(255,15,115));
    AddBlockingBox(TEXT("RightWall"),FVector(0,270,167),FVector(255,15,115));
    AddBlockingBox(TEXT("FrontWindowWall"),FVector(270,136,167),FVector(15,136,115));
    AddBlockingBox(TEXT("DoorLeftJamb"),FVector(270,-231,167),FVector(15,41,115));
    AddBlockingBox(TEXT("DoorRightJamb"),FVector(270,-41,167),FVector(15,41,115));
    AddBlockingBox(TEXT("DoorHeader"),FVector(270,-136,267),FVector(15,54,15));
    // Source walls taper above their bounds. Cover the visible wall/floor light gap
    // without putting collision across the walkable doorway.
    if(auto* Material=LoadObject<UMaterialInterface>(nullptr,WallMaterial))
    {
        for(auto* Liner:{
            AddVisual(TEXT("BackBaseboard"),Cube,FVector(-268,0,62),FVector(.18,5.45,.25)),
            AddVisual(TEXT("LeftBaseboard"),Cube,FVector(0,-268,62),FVector(5.45,.18,.25)),
            AddVisual(TEXT("RightBaseboard"),Cube,FVector(0,268,62),FVector(5.45,.18,.25))})
            Liner->SetMaterial(0,Material);
    }
    else UE_LOG(LogTemp,Error,TEXT("TASK-028 missing wall material: %s"),WallMaterial);
    // The static door is held fully open alongside the frame; no lock or new interaction.
    AddVisual(TEXT("OpenDoor"),Door,FVector(245,-25,161),FVector(.5,2.2,2.2),FRotator(0,90,0));

    AddVisual(TEXT("EntryStairs"),Stairs,FVector(360,-136,26),FVector(1.5,1.5,1.2));
    AddBlockingBox(TEXT("StepLow"),FVector(420,-136,8.5),FVector(30,75,8.5));
    AddBlockingBox(TEXT("StepMiddle"),FVector(360,-136,17),FVector(30,75,17));
    AddBlockingBox(TEXT("StepHigh"),FVector(300,-136,26),FVector(30,75,26));

    Index=0;
    for(const float X:{-140.f,140.f}) for(const float Y:{-120.f,120.f})
    {
        const FString Name=FString::Printf(TEXT("Roof_%d"),Index++);
        AddVisual(*Name,Roof,FVector(X,Y,310),FVector(3.2,3.2,2.3),FRotator(0,X<0?180:0,0));
    }
    AddVisual(TEXT("RoofRidge"),Ridge,FVector(0,0,360),FVector(1.5,6.3,2.3));
    // The single-view source mesh has holes around its underside and edges. A thin
    // non-colliding roof liner carrying the imported roof material closes those gaps.
    auto* RoofMat=LoadObject<UMaterialInterface>(nullptr,RoofMaterial);
    if(!RoofMat) UE_LOG(LogTemp,Error,TEXT("TASK-028 missing roof material: %s"),RoofMaterial);
    for(const float X:{-140.f,140.f})
    {
        const TCHAR* Name=X<0?TEXT("RearRoofLiner"):TEXT("FrontRoofLiner");
        auto* Liner=AddVisual(Name,Cube,FVector(X,0,310),FVector(3.4,6.2,.08),
                              FRotator(X<0?15:-15,0,0));
        if(RoofMat) Liner->SetMaterial(0,RoofMat);
    }
    AddVisual(TEXT("Bed"),Bed,FVector(-130,-170,61),FVector(1.4));
    AddVisual(TEXT("Chest"),Chest,FVector(-155,140,80),FVector(.85));
    AddVisual(TEXT("Chair"),Chair,FVector(120,150,90),FVector(.7));
    AddVisual(TEXT("DoorLantern"),Lantern,FVector(225,-45,210),FVector(.35));
    auto* Light=NewObject<UPointLightComponent>(this,TEXT("DoorLanternLight"));
    AddInstanceComponent(Light);
    Light->SetupAttachment(HouseRoot);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetRelativeLocation(FVector(225,-45,210));
    Light->SetIntensity(6000);
    Light->SetLightColor(FLinearColor(1.f,.72f,.44f));
    Light->SetAttenuationRadius(900);
    Light->SetCastShadows(false);
    Light->RegisterComponent();
}
