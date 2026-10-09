#include "HearthwardHometownFortress.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AHearthwardHometownFortress::AHearthwardHometownFortress()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("FortressRoot")));
    Tags.Add(TEXT("CampaignHometownFortress"));
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickInterval=1.f;
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> StoneAsset(TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_RoughStone"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TimberAsset(TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> FrameAsset(TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/SM_BedroomDoorframe"));
    Stone=StoneAsset.Object;Timber=TimberAsset.Object;DoorframeMesh=FrameAsset.Object;
    // Cooked stateless emitters require Niagara's initialized template module lists.
    RaidFlame=FSoftObjectPath(TEXT("/Game/Hearthward/Assets/TASK-096/Fire/NS_HearthFire.NS_HearthFire"));
    RaidSmoke=FSoftObjectPath(TEXT("/Game/Hearthward/Assets/TASK-096/Fire/NS_HearthSmoke.NS_HearthSmoke"));
}

float AHearthwardHometownFortress::Terrain(float X, float Y) const
{
    const FVector Origin=GetActorLocation()+FVector(X,Y,0);
    TArray<FHitResult> Hits;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HometownFoundation),false,this);
    GetWorld()->LineTraceMultiByObjectType(Hits,FVector(Origin.X,Origin.Y,80000),
        FVector(Origin.X,Origin.Y,-30000),FCollisionObjectQueryParams(ECC_WorldStatic),Query);
    for(const auto& Hit:Hits)
        if(Hit.GetActor() && (Hit.GetActor()->GetClass()->GetName().Contains(TEXT("Landscape"))
            || Hit.GetActor()->ActorHasTag(TEXT("Hearthward.NatureGround"))))
            return Hit.ImpactPoint.Z-GetActorLocation().Z;
    return 0;
}

UStaticMeshComponent* AHearthwardHometownFortress::Part(FString Label,FVector Center,FVector Size,
                                                       bool Blocking,UMaterialInterface* Material)
{
    auto* Mesh=NewObject<UStaticMeshComponent>(this,FName(*FString::Printf(TEXT("%s_%d"),*Label,PartIndex++)));
    AddInstanceComponent(Mesh);
    Mesh->SetupAttachment(GetRootComponent());
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetRelativeLocation(Center);
    Mesh->SetRelativeScale3D(Size/100);
    Mesh->SetMaterial(0,Material?Material:Stone.Get());
    Mesh->SetCollisionProfileName(Blocking?TEXT("BlockAll"):TEXT("NoCollision"));
    Mesh->SetCanEverAffectNavigation(Blocking);
    Mesh->ComponentTags.Add(FName(*Label));
    Mesh->RegisterComponent();
    return Mesh;
}

void AHearthwardHometownFortress::Furniture(const TCHAR* Label,const TCHAR* Path,FVector Position,float Scale,float Yaw)
{
    auto* Mesh=NewObject<UStaticMeshComponent>(this,FName(Label));
    AddInstanceComponent(Mesh);Mesh->SetupAttachment(GetRootComponent());
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));
    Mesh->SetRelativeLocation(Position);Mesh->SetRelativeScale3D(FVector(Scale));
    Mesh->SetRelativeRotation(FRotator(0,Yaw,0));
    Mesh->SetCollisionProfileName(TEXT("NoCollision"));Mesh->SetCanEverAffectNavigation(false);
    Mesh->RegisterComponent();
}

void AHearthwardHometownFortress::Lamp(FVector Position)
{
    const FString Name=FString::Printf(TEXT("Lantern_%d"),PartIndex++);
    Furniture(*Name,TEXT("/Game/Hearthward/Assets/TASK-028/furniture/metal_lantern/SM_metal_lantern.SM_metal_lantern"),Position,.5f);
    auto* Light=NewObject<UPointLightComponent>(this);
    AddInstanceComponent(Light);Light->SetupAttachment(GetRootComponent());
    Light->SetRelativeLocation(Position);Light->SetIntensity(11000);
    Light->SetLightColor(FLinearColor(1,.64f,.32f));Light->SetAttenuationRadius(1500);
    Light->SetCastShadows(false);Light->RegisterComponent();
}

void AHearthwardHometownFortress::Wall(FVector2D A,FVector2D B,float Height,float Width)
{
    // Short foundations follow the existing landscape without flattening the map.
    const float Length=FVector2D::Distance(A,B);
    const int32 Count=FMath::CeilToInt(Length/400);
    for(int32 I=0;I<Count;++I)
    {
        const FVector2D P=FMath::Lerp(A,B,(I+.5f)/Count);
        const float Floor=Terrain(P.X,P.Y);
        const bool AlongX=FMath::Abs(B.X-A.X)>FMath::Abs(B.Y-A.Y);
        Part(TEXT("CurtainWall"),FVector(P,Floor+Height/2-80),AlongX?FVector(Length/Count+2,Width,Height+160):FVector(Width,Length/Count+2,Height+160));
        Part(TEXT("WallCoping"),FVector(P,Floor+Height),AlongX?FVector(Length/Count+6,Width+40,40):FVector(Width+40,Length/Count+6,40));
        Part(TEXT("Merlon"),FVector(P,Floor+Height+85),FVector(145,145,140));
    }
}

void AHearthwardHometownFortress::Tower(FVector2D Center,float Width,float Height)
{
    const float Floor=Terrain(Center.X,Center.Y);
    float Base=Floor-100;
    for(float X:{-Width/2,Width/2})for(float Y:{-Width/2,Width/2})
        Base=FMath::Min(Base,Terrain(Center.X+X,Center.Y+Y)-100);
    Part(TEXT("TowerFoundation"),FVector(Center,(Base+Floor+200)/2),FVector(Width+160,Width+160,Floor+200-Base));
    Part(TEXT("Tower"),FVector(Center,(Base+Floor+Height)/2),FVector(Width,Width,Floor+Height-Base));
    for(float Z:{Height*.35f,Height*.7f,Height})
        Part(TEXT("TowerCornice"),FVector(Center,Floor+Z),FVector(Width+90,Width+90,65));
    for(int32 I=0;I<4;++I)
    {
        const float Offset=(I/3.f-.5f)*Width;
        for(float Sign:{-1.f,1.f})
        {
            Part(TEXT("Battlement"),FVector(Center.X+Offset,Center.Y+Sign*Width/2,Floor+Height+120),FVector(150,150,240));
            if(I>0 && I<3)Part(TEXT("Battlement"),FVector(Center.X+Sign*Width/2,Center.Y+Offset,Floor+Height+120),FVector(150,150,240));
        }
    }
}

FVector AHearthwardHometownFortress::BedroomLanding() const {return GetActorLocation()+FVector(0,100,BedroomFloor+100);}
FVector AHearthwardHometownFortress::RelicPosition() const {return GetActorLocation()+FVector(-280,170,BedroomFloor+80);}

void AHearthwardHometownFortress::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateRaidFire();
    if(FacadeMaterial)
    {
        const double Day=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().Daylight;
        // Marble exports baked color. Tint follows the same clock as the native stonework.
        FacadeMaterial->SetVectorParameterValue(TEXT("Tint"),FMath::Lerp(FLinearColor(.10f,.14f,.22f),FLinearColor(.85f,.85f,.85f),Day));
    }
}

void AHearthwardHometownFortress::ClearRaidFire()
{
    for(USceneComponent* Component:RaidEffects)if(IsValid(Component))Component->DestroyComponent();
    RaidEffects.Reset();
}

void AHearthwardHometownFortress::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearRaidFire();
    Super::EndPlay(Reason);
}

void AHearthwardHometownFortress::UpdateRaidFire()
{
    const auto* Controller=GetWorld()->GetFirstPlayerController();
    const APawn* Player=Controller?Controller->GetPawn():nullptr;
    const bool Visible=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->State.Phase==TEXT("prologue")
        && Player && FVector::DistSquared2D(Player->GetActorLocation(),GetActorLocation())<FMath::Square(20000.f);
    if(!Visible){ClearRaidFire();return;}
    if(!RaidEffects.IsEmpty())return;
    UNiagaraSystem* Flame=RaidFlame.LoadSynchronous();
    UNiagaraSystem* Smoke=RaidSmoke.LoadSynchronous();
    // Three small pockets stay outside the bedroom, stairs, postern and main court path.
    for(const FVector2D Point:{FVector2D(-1400,2100),FVector2D(2800,2700),FVector2D(2850,4700)})
    {
        const FVector Base(Point,Terrain(Point.X,Point.Y));
        for(int32 I=0;I<3;++I)
        {
            auto* Wood=Part(TEXT("RaidBurningWood"),Base+FVector(0,(I-1)*24,12+I*5),FVector(110,16,16),false,Timber);
            Wood->SetRelativeRotation(FRotator(0,(I-1)*28,0));RaidEffects.Add(Wood);
        }
        for(int32 I=0;I<2;++I)
        {
            auto* Effect=NewObject<UNiagaraComponent>(this);
            AddInstanceComponent(Effect);Effect->SetupAttachment(GetRootComponent());
            Effect->SetAutoActivate(false);Effect->SetAutoDestroy(false);
            Effect->SetRelativeLocation(Base+FVector(0,0,I?100:70));
            Effect->SetAsset(I?Smoke:Flame);
            Effect->SetCanEverAffectNavigation(false);Effect->SetCastShadow(false);
            Effect->ComponentTags.Add(TEXT("HearthwardRaidVFX"));
            Effect->RegisterComponent();Effect->Activate(true);RaidEffects.Add(Effect);
        }
        auto* Light=NewObject<UPointLightComponent>(this);
        AddInstanceComponent(Light);Light->SetupAttachment(GetRootComponent());
        Light->SetRelativeLocation(Base+FVector(0,0,90));Light->SetIntensity(3000);
        Light->SetLightColor(FLinearColor(1,.35f,.06f));Light->SetAttenuationRadius(550);
        Light->SetCastShadows(false);Light->ComponentTags.Add(TEXT("HearthwardRaidLight"));
        Light->RegisterComponent();RaidEffects.Add(Light);
    }
}

void AHearthwardHometownFortress::BeginPlay()
{
    Super::BeginPlay();
    for(float X:{-650.f,0.f,1850.f})for(float Y:{-550.f,500.f,1050.f})
        BedroomFloor=FMath::Max(BedroomFloor,Terrain(X,Y)+120);
    const float F=BedroomFloor;
    float FoundationBase=0;
    for(float X:{-660.f,660.f})for(float Y:{-560.f,560.f})FoundationBase=FMath::Min(FoundationBase,Terrain(X,Y)-80);
    Part(TEXT("BedroomFoundation"),FVector(0,0,(FoundationBase+F-30)/2),FVector(1320,1120,F-30-FoundationBase));
    Part(TEXT("BedroomFloor"),FVector(0,0,F-15),FVector(1200,1000,30),true,Timber);
    Part(TEXT("BedroomBack"),FVector(0,-550,F+270),FVector(1320,100,540));
    Part(TEXT("BedroomEast"),FVector(650,0,F+270),FVector(100,1000,540));
    // A real window and a 2.8 m doorway keep the camera and companion clear.
    for(float Y:{-390.f,390.f})Part(TEXT("WindowPier"),FVector(-650,Y,F+270),FVector(100,220,540));
    Part(TEXT("WindowSill"),FVector(-650,0,F+65),FVector(100,560,130));
    Part(TEXT("WindowLintel"),FVector(-650,0,F+495),FVector(100,560,90));
    for(float X:{-395.f,395.f})Part(TEXT("BedroomDoorPier"),FVector(X,550,F+270),FVector(510,100,540));
    Part(TEXT("BedroomDoorLintel"),FVector(0,550,F+460),FVector(280,100,160));
    auto* Doorframe=NewObject<UStaticMeshComponent>(this,TEXT("BedroomDoorframe"));
    AddInstanceComponent(Doorframe);Doorframe->SetupAttachment(GetRootComponent());
    Doorframe->SetStaticMesh(DoorframeMesh);Doorframe->SetRelativeLocation(FVector(0,550,F));
    Doorframe->SetCollisionProfileName(TEXT("NoCollision"));Doorframe->SetCanEverAffectNavigation(false);
    Doorframe->RegisterComponent();
    Furniture(TEXT("BedroomJoinery"),TEXT("/Game/Hearthward/Assets/TASK-096/Interior/SM_BedroomJoinery.SM_BedroomJoinery"),FVector(0,0,F),1.f);
    Part(TEXT("BedroomCeiling"),FVector(0,0,F+555),FVector(1450,1250,70),true,Timber);
    for(float X:{-480.f,0.f,480.f})Part(TEXT("CeilingBeam"),FVector(X,0,F+510),FVector(32,1100,45),false,Timber);
    for(float X:{-340.f,340.f})
    {
        Furniture(X<0?TEXT("PlayerBed"):TEXT("BrotherBed"),TEXT("/Game/Hearthward/Assets/TASK-098/CampSet/SM_RopeBed.SM_RopeBed"),FVector(X,-240,F),1.f,90.f);
        auto* BedCollision=Part(TEXT("BedCollision"),FVector(X,-240,F+40),FVector(150,230,80));
        BedCollision->SetVisibility(false);BedCollision->SetCastShadow(false);
    }
    Lamp(FVector(480,320,F+270));
    Lamp(FVector(0,-470,F+230));
    Part(TEXT("GalleryFloor"),FVector(550,800,F-30),FVector(2500,600,60),true,Timber);
    Part(TEXT("GalleryRoof"),FVector(550,850,F+555),FVector(2700,700,60),true,Timber);
    for(float X:{-600.f,200.f,900.f,1800.f})
    {
        Part(TEXT("GalleryColumn"),FVector(X,1050,F+260),FVector(60,60,520));
        const float Base=Terrain(X,1050)-80;
        Part(TEXT("GallerySupport"),FVector(X,1050,(Base+F)/2),FVector(100,100,F-Base));
    }
    Part(TEXT("GalleryRail"),FVector(250,1080,F+65),FVector(1900,50,130));
    Part(TEXT("GalleryRail"),FVector(1780,1080,F+65),FVector(140,50,130));
    // Twenty-centimetre risers are below the existing character step height.
    const float Bottom=Terrain(1500,2700)+10;
    const float Rise=FMath::Max(0.f,F-Bottom);
    const int32 Steps=FMath::Max(1,FMath::CeilToInt(Rise/20));
    const float Run=FMath::Max(1600.f,Steps*35.f);
    for(int32 I=0;I<Steps;++I)
    {
        const float Top=F-Rise*(I+1)/Steps;
        const float Y=1100+(I+.5f)*Run/Steps;
        const float Base=FMath::Min(Terrain(1500,Y)-40,Bottom-40);
        Part(TEXT("EscapeStair"),FVector(1500,Y,(Top+Base)/2),FVector(400,Run/Steps+1,Top-Base));
    }
    // The courts retain their natural ground and existing campaign spawn positions.
    Wall(FVector2D(-2200,-1600),FVector2D(500,-1600),900);
    Wall(FVector2D(3500,-1600),FVector2D(8000,-1600),900);
    Wall(FVector2D(-2200,-1600),FVector2D(-2200,2000),800);
    Wall(FVector2D(-2200,3600),FVector2D(-2200,6500),800);
    Wall(FVector2D(-2200,6500),FVector2D(-600,6500),850);
    Wall(FVector2D(600,6500),FVector2D(8000,6500),850);
    Wall(FVector2D(8000,-1600),FVector2D(8000,3000),1100);
    Wall(FVector2D(8000,4600),FVector2D(8000,6500),1100);
    for(FVector2D P:{FVector2D(-2200,-1600),FVector2D(-2200,6500),FVector2D(8000,-1600),FVector2D(8000,6500)})Tower(P,800,1700);
    Tower(FVector2D(6500,1000),1900,2800);
    Tower(FVector2D(5350,0),650,3200);
    Tower(FVector2D(7650,2000),650,3000);
    // Only the inspected inner face is reused. Solid masonry closes its back and edges.
    const float FacadeFloor=Terrain(5300,500);
    Part(TEXT("FacadeBacking"),FVector(5600,750,FacadeFloor+1000),FVector(400,2000,2100));
    Part(TEXT("FacadePlinth"),FVector(4930,750,FacadeFloor-30),FVector(1400,2100,140));
    Part(TEXT("FacadeCoping"),FVector(5260,750,FacadeFloor+1810),FVector(750,2100,90));
    auto* Facade=NewObject<UStaticMeshComponent>(this,TEXT("CourtyardFacade"));
    AddInstanceComponent(Facade);Facade->SetupAttachment(GetRootComponent());
    Facade->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-077/SM_StoneholdFacade/StaticMeshes/SM_StoneholdFacade.SM_StoneholdFacade")));
    Facade->SetRelativeLocation(FVector(3400,500,FacadeFloor));
    Facade->SetRelativeRotation(FRotator(0,90,0));
    Facade->SetCollisionProfileName(TEXT("NoCollision"));Facade->SetCanEverAffectNavigation(false);Facade->SetCastShadow(false);
    FacadeMaterial=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-077/Materials/M_StoneholdFacade_PBR.M_StoneholdFacade_PBR")),this);
    Facade->SetMaterial(0,FacadeMaterial);Facade->RegisterComponent();
    Tick(0);
    // Raised covered gallery along the west court, with open ground-level bays.
    for(float Y=2800;Y<=5600;Y+=700)
    {
        const float Z=Terrain(-1600,Y);
        Part(TEXT("CloisterPier"),FVector(-1600,Y,Z+230),FVector(120,120,460));
        Part(TEXT("CloisterRoof"),FVector(-1850,Y,Z+480),FVector(800,710,60),true,Timber);
    }
    const float Gate=FMath::Max(Terrain(-600,6500),Terrain(600,6500));
    Part(TEXT("PosternLintel"),FVector(0,6500,Gate+700),FVector(1400,180,180));
    Lamp(FVector(-520,6400,Gate+350));Lamp(FVector(520,6400,Gate+350));
    Lamp(FVector(1800,1050,F+300));

    auto* Slate=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")),this);
    Slate->SetVectorParameterValue(TEXT("Color"),FLinearColor(.075f,.095f,.12f));
    // Houses form two staggered rows east of the escape route and campaign markers.
    for(int32 Row=0;Row<2;++Row)for(int32 Column=0;Column<3;++Column)
    {
        const float X=3300+Column*1650+Row*180;
        const float Y=3350+Row*1850;
        const float W=1000,D=1150,H=650+((Row+Column)%2)*180;
        float Base=Terrain(X,Y)-80;
        const float Floor=Terrain(X,Y)+80;
        for(float DX:{-W/2,W/2})for(float DY:{-D/2,D/2})
            Base=FMath::Min(Base,Terrain(X+DX,Y+DY)-80);
        Part(TEXT("VillageHouse"),FVector(X,Y,(Base+Floor+H)/2),FVector(W,D,Floor+H-Base));
        Part(TEXT("HouseBelt"),FVector(X,Y,Floor+H*.5f),FVector(W+45,D+45,40),false,Timber);
        const float Pitch=35;
        const float RoofRise=W*.5f*FMath::Tan(FMath::DegreesToRadians(Pitch));
        // Stepped gables sit beneath the pitched roof and close the silhouette.
        for(int32 Tier=0;Tier<8;++Tier)
        {
            const float Fraction=(Tier+.5f)/8;
            Part(TEXT("HouseGable"),FVector(X,Y,Floor+H+Fraction*RoofRise),
                FVector(W*(1-Tier/8.f),D,RoofRise/8+2),false);
        }
        for(float Sign:{-1.f,1.f})
        {
            auto* Roof=Part(TEXT("SlateRoof"),FVector(X+Sign*W/4,Y,Floor+H+RoofRise/2+25),
                FVector((W/2+100)/FMath::Cos(FMath::DegreesToRadians(Pitch)),D+180,55),false,Slate);
            Roof->SetRelativeRotation(FRotator(-Sign*Pitch,0,0));
            for(float Z:{H*.35f,H*.75f})
            {
                Part(TEXT("HouseWindow"),FVector(X+Sign*W/2+Sign*3,Y-230,Floor+Z),FVector(12,160,150),false,Timber);
                Part(TEXT("WindowHeader"),FVector(X+Sign*(W/2+12),Y-230,Floor+Z+100),FVector(35,210,40),false);
            }
        }
        Part(TEXT("Chimney"),FVector(X+W*.25f,Y+D*.22f,Floor+H+RoofRise),FVector(130,160,420),false);
        Part(TEXT("ChimneyCap"),FVector(X+W*.25f,Y+D*.22f,Floor+H+RoofRise+210),FVector(170,200,40),false);
        Part(TEXT("HouseDoor"),FVector(X,Y-D/2-5,Floor+140),FVector(180,15,280),false,Timber);
        Lamp(FVector(X-160,Y-D/2-35,Floor+250));
    }
    // Buttresses and narrow openings break up the keep's tall stone faces.
    const float KeepFloor=Terrain(6500,1000);
    for(float X:{5750.f,6500.f,7250.f})
    {
        const float Base=Terrain(X,-50)-80;
        Part(TEXT("KeepButtress"),FVector(X,-50,(Base+KeepFloor+1600)/2),FVector(180,340,KeepFloor+1600-Base));
        for(float Z:{1800.f,2350.f})
            Part(TEXT("KeepArrowSlit"),FVector(X,42,KeepFloor+Z),FVector(70,18,210),false,Timber);
    }
}
