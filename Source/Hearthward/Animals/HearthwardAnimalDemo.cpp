#include "HearthwardAnimalDemo.h"
#include "HearthwardAnimalMotionComponent.h"
#include "HearthwardAnimalBounds.h"
#include "../HearthwardCharacter.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InputComponent.h"
#include "Animation/SkeletalMeshActor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "InputCoreTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HighResScreenshot.h"
#include "UnrealClient.h"
#include "Fonts/CompositeFont.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
const TCHAR* Species[]={TEXT("stag_a"),TEXT("hare"),TEXT("goat"),TEXT("pheasant"),TEXT("pig"),TEXT("wolf"),TEXT("black_bear"),TEXT("ram"),TEXT("hen"),TEXT("red_fox"),TEXT("carp"),TEXT("crucian_carp"),TEXT("catfish"),TEXT("eel")};
AHearthwardAnimalDemoGameMode* Demo(const UObject* Context){return Cast<AHearthwardAnimalDemoGameMode>(UGameplayStatics::GetGameMode(Context));}
void MovePlayer(UWorld* World,FVector Position)
{
    if(auto* P=UGameplayStatics::GetPlayerPawn(World,0))
    {if(auto* C=Cast<ACharacter>(P))C->GetCharacterMovement()->StopMovementImmediately();P->SetActorLocation(Position,false,nullptr,ETeleportType::TeleportPhysics);}
}
}
AHearthwardAnimalDemoActor::AHearthwardAnimalDemoActor()
{
    auto* Root=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));SetRootComponent(Root);
    Body=CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));Body->SetupAttachment(Root);Body->ComponentTags.Add(TEXT("body"));
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Body->SetCollisionResponseToAllChannels(ECR_Ignore);Body->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Body->SetCanEverAffectNavigation(false);
    Combat=CreateDefaultSubobject<UHearthwardCombatTargetComponent>(TEXT("Combat"));Combat->Protected=false;Combat->NaturalTarget=true;Combat->Region=NAME_None;Combat->RewardKind=TEXT("hunt");
    Motion=CreateDefaultSubobject<UHearthwardAnimalMotionComponent>(TEXT("AnimalMotion"));
}
bool AHearthwardAnimalDemoActor::Configure(FName InSpecies,const FBox& Bounds)
{
    Combat->Id=FName(*(TEXT("animal_demo_")+InSpecies.ToString()));Combat->MaximumHealth=Combat->Health=100;
    if(!Motion->Configure(InSpecies))return false;
    Motion->SetHabitat(Bounds,true);
    const auto B=Motion->Mesh->GetSkeletalMeshAsset()->GetBounds();Body->SetBoxExtent(B.BoxExtent);Body->SetRelativeLocation(B.Origin);
    return true;
}
AHearthwardAnimalDemoGameMode::AHearthwardAnimalDemoGameMode()
{
    PrimaryActorTick.bCanEverTick=true;DefaultPawnClass=AHearthwardCharacter::StaticClass();
    PlayerControllerClass=AHearthwardAnimalDemoController::StaticClass();HUDClass=AHearthwardAnimalDemoHUD::StaticClass();
}
void AHearthwardAnimalDemoGameMode::BuildHabitat()
{
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* GroundMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Tests/AnimalMotion/Materials/M_Grass_PBR.M_Grass_PBR"));
    auto* WoodMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Tests/AnimalMotion/Materials/M_Wood_PBR.M_Wood_PBR"));
    auto* StoneMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_RockScan.M_RockScan"));
    TArray<AActor*> OldLights;
    for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It)OldLights.Add(*It);
    for(TActorIterator<ASkyLight> It(GetWorld());It;++It)OldLights.Add(*It);
    for(auto* Light:OldLights)Light->Destroy();
    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,1800),FRotator(-48,-32,0));
    auto* Sunlight=Cast<UDirectionalLightComponent>(Sun->GetLightComponent());Sunlight->SetMobility(EComponentMobility::Movable);
    Sunlight->SetIntensity(3.5f);Sunlight->SetDynamicShadowDistanceMovableLight(16000);Sunlight->SetDynamicShadowCascades(4);
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>();auto* Skylight=Sky->GetLightComponent();Skylight->SetMobility(EComponentMobility::Movable);
    Skylight->SourceType=SLS_SpecifiedCubemap;Skylight->SetCubemap(LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap")));
    Skylight->SetIntensity(.8f);Skylight->bLowerHemisphereIsBlack=false;Skylight->RecaptureSky();
    auto* SkyDome=GetWorld()->SpawnActor<AStaticMeshActor>();auto* Dome=SkyDome->GetStaticMeshComponent();Dome->SetMobility(EComponentMobility::Movable);
    Dome->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/MapTemplates/Sky/SM_SkySphere.SM_SkySphere")));Dome->SetWorldScale3D(FVector(1000));
    Dome->SetCollisionEnabled(ECollisionEnabled::NoCollision);Dome->CastShadow=false;
    if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/MapTemplates/Sky/M_Procedural_Sky_Daytime.M_Procedural_Sky_Daytime")))Dome->SetMaterial(0,M);
    auto Add=[&](const TCHAR* Name,FVector P,FVector Size,UMaterialInterface* Material,bool Ground=false)
    {
        auto* A=GetWorld()->SpawnActor<AStaticMeshActor>(P,FRotator::ZeroRotator);A->Tags.Add(FName(Name));
        auto* C=A->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Cube);C->SetWorldScale3D(Size/100);
        C->SetCollisionObjectType(ECC_WorldStatic);C->SetCollisionProfileName(TEXT("BlockAll"));if(Material)C->SetMaterial(0,Material);
        if(FCString::Strcmp(Name,TEXT("AnimalDemoFence"))==0)C->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
        if(Ground)A->Tags.Add(TEXT("Hearthward.NatureGround"));return A;
    };
    bool HasGround=false;
    for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
        if(UStaticMesh* Asset=It->GetStaticMeshComponent()->GetStaticMesh();Asset && Asset->GetPathName().StartsWith(TEXT("/Game/Hearthward/Tests/AnimalMotion/Geometry/")))
        {It->Tags.Add(TEXT("Hearthward.NatureGround"));if(GroundMaterial)It->GetStaticMeshComponent()->SetMaterial(0,GroundMaterial);HasGround=true;}
    if(!HasGround)Add(TEXT("AnimalDemoGround"),FVector(0,0,-20),FVector(5100,3900,40),GroundMaterial,true);
    Add(TEXT("AnimalDemoObserverDeck"),FVector(-4200,-3200,-20),FVector(1000,1000,40),WoodMaterial,true);
    // Extend the outer walkway without putting a land plane over the open water.
    Add(TEXT("AnimalDemoPondApproach"),FVector(3200,-1530,-20),FVector(1900,600,40),WoodMaterial,true);
    Add(TEXT("AnimalDemoPondFloor"),FVector(3200,0,-235),FVector(1200,2400,40),StoneMaterial);
    for(int32 Side:{-1,1})
    {
        Add(TEXT("AnimalDemoPondWall"),FVector(3200+Side*620,0,-110),FVector(40,2440,230),StoneMaterial);
        Add(TEXT("AnimalDemoPondWall"),FVector(3200,Side*1220,-110),FVector(1280,40,230),StoneMaterial);
    }
    auto* Surface=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(3200,0,-8),FRotator::ZeroRotator);
    auto* Water=Surface->GetStaticMeshComponent();Water->SetMobility(EComponentMobility::Movable);
    Water->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane")));Water->SetWorldScale3D(FVector(12,24,1));Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_RiverWater.M_RiverWater")))Water->SetMaterial(0,M);
    // A low fence makes the exact animal activity rectangle visible; the player can use the front gap.
    for(int32 X=-2200;X<=2200;X+=400)for(int32 Side:{-1,1})
    {
        if(Side==-1 && FMath::Abs(X)<300)continue;
        Add(TEXT("AnimalDemoFence"),FVector(X,Side*1600,35),FVector(12,12,70),WoodMaterial);
        Add(TEXT("AnimalDemoFence"),FVector(X+180,Side*1600,46),FVector(360,8,8),WoodMaterial);
    }
    for(int32 Y=-1600;Y<=1600;Y+=400)for(int32 Side:{-1,1})
    {
        Add(TEXT("AnimalDemoFence"),FVector(Side*2200,Y,35),FVector(12,12,70),WoodMaterial);
        if(Y<1600)Add(TEXT("AnimalDemoFence"),FVector(Side*2200,Y+200,46),FVector(8,400,8),WoodMaterial);
    }
}
void AHearthwardAnimalDemoGameMode::StartPlay()
{
    Super::StartPlay();BuildHabitat();
    // Keep one native animal per species, even if editor previews are added later.
    TArray<AActor*> Preview;
    for(TActorIterator<ASkeletalMeshActor> It(GetWorld());It;++It)
        if(auto* Asset=It->GetSkeletalMeshComponent()->GetSkeletalMeshAsset();Asset && Asset->GetPathName().StartsWith(TEXT("/Game/Hearthward/Animals/MotionR3/")))Preview.Add(*It);
    for(auto* A:Preview)A->Destroy();
    for(int32 I=0;I<14;++I)
    {
        const FVector P=I<10?FVector(-1700+(I%5)*850,I<5?-650:650,2):FVector(3000+(I%2)*350,-700+((I-10)/2)*1400,-130);
        auto* A=GetWorld()->SpawnActor<AHearthwardAnimalDemoActor>(P,FRotator(0,I*29,0));
        if(!A->Configure(FName(Species[I]),I<10?LandBounds:WaterBounds))
        {UE_LOG(LogTemp,Fatal,TEXT("Animal demo asset binding failed: %s"),Species[I]);return;}
        Animals.Add(A);
    }
    MovePlayer(GetWorld(),FVector(0,-1850,100));
    auto* Player=UGameplayStatics::GetPlayerPawn(this,0);
    if(auto* G=Player?Player->FindComponentByClass<UHearthwardGameplayComponent>():nullptr)G->Enabled=true;
    Brother=GetWorld()->SpawnActor<AHearthwardCompanionFixture>(FVector(-300,-1800,100),FRotator::ZeroRotator);
    ObservationCamera=GetWorld()->SpawnActor<ACameraActor>();ObservationCamera->GetCameraComponent()->SetFieldOfView(50);
    Verify=FParse::Param(FCommandLine::Get(),TEXT("HearthwardAnimalDemoVerify"));
    if(Verify)
    {MovePlayer(GetWorld(),FVector(-4200,-3200,100));Brother->SetActorLocation(FVector(-4500,-3200,100),false,nullptr,ETeleportType::TeleportPhysics);}
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->SetControlRotation(FRotator(-8,90,0));
    Feedback=TEXT("进入围栏可观察接近反应。按 F5 查看动物近景。");
    UE_LOG(LogTemp,Display,TEXT("ANIMAL_DEMO_READY species=%d land=44x32m pond=12x24m"),Animals.Num());
}
AHearthwardAnimalDemoActor* AHearthwardAnimalDemoGameMode::Selected() const{return Animals.IsValidIndex(SelectedIndex)?Animals[SelectedIndex].Get():nullptr;}
void AHearthwardAnimalDemoGameMode::NextAnimal()
{
    SelectedIndex=(SelectedIndex+1)%Animals.Num();Observing=true;
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->SetViewTargetWithBlend(ObservationCamera,.35f);
    Feedback=TEXT("近景观察；F2 让弟弟靠近，F3 查看致命伤倒地。");
    UE_LOG(LogTemp,Display,TEXT("ANIMAL_DEMO_INPUT F5 selected=%s"),*Selected()->Motion->Species.ToString());
}
void AHearthwardAnimalDemoGameMode::ReturnToPlayer()
{
    Observing=false;
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->SetViewTargetWithBlend(PC->GetPawn(),.25);
    Feedback=TEXT("使用 WASD / 鼠标移动观察；动物仅在围栏或水池范围内活动。");
    UE_LOG(LogTemp,Display,TEXT("ANIMAL_DEMO_INPUT F1 player_view"));
}
void AHearthwardAnimalDemoGameMode::BrotherApproach()
{
    if(!Selected())NextAnimal();BrotherWalking=!BrotherWalking;
    Feedback=BrotherWalking?TEXT("弟弟正在步行靠近所选动物。"):TEXT("弟弟在原地等待。");
    UE_LOG(LogTemp,Display,TEXT("ANIMAL_DEMO_INPUT F2 brother_walking=%d"),BrotherWalking);
}
void AHearthwardAnimalDemoGameMode::LethalSelected()
{
    if(!Selected())NextAnimal();auto* A=Selected();if(!A || A->Motion->Dead)return;
    auto* Player=UGameplayStatics::GetPlayerPawn(this,0);
    if(auto* C=Player?Player->FindComponentByClass<UHearthwardCombatComponent>():nullptr)
        C->HitTarget(A->Combat,10000,TEXT("body"),false,FGuid::NewGuid(),Player);
    Feedback=TEXT("已施加致命伤；F4 重置后可继续观察。");
    UE_LOG(LogTemp,Display,TEXT("ANIMAL_DEMO_INPUT F3 health=%.0f selected=%s"),A->Combat->Health,*A->Motion->Species.ToString());
}
void AHearthwardAnimalDemoGameMode::ResetAnimals()
{
    BrotherWalking=false;for(auto A:Animals)A->Motion->ResetAnimal();
    if(Brother){Brother->GetCharacterMovement()->StopMovementImmediately();Brother->SetActorLocation(FVector(-300,-1800,100),false,nullptr,ETeleportType::TeleportPhysics);}
    MovePlayer(GetWorld(),FVector(0,-1850,100));Feedback=TEXT("14只动物已重置，重新开始自然活动。");
    UE_LOG(LogTemp,Display,TEXT("ANIMAL_DEMO_INPUT F4 reset=%d"),Animals.Num());
}
void AHearthwardAnimalDemoGameMode::Tick(float Delta)
{
    Super::Tick(Delta);
    if(Observing && Selected() && ObservationCamera)
    {
        const auto* M=Selected()->Motion->Mesh.Get();const FVector Center=M->Bounds.Origin;
        const float D=FMath::Max(260.f,M->GetSkeletalMeshAsset()->GetBounds().BoxExtent.Size()*3.8f);
        FVector At=Center+FVector(-D*.72f,-D*.7f,Selected()->Motion->Aquatic?35:D*.3f);
        if(Selected()->Motion->Aquatic)
        {
            const FVector Middle=WaterBounds.GetCenter();At=Center+FVector((Center.X<Middle.X?1:-1)*D*.72f,(Center.Y<Middle.Y?1:-1)*D*.7f,35);
            At=HearthwardAnimalBounds::Clamp(FBox(WaterBounds.Min+FVector(30,30,20),WaterBounds.Max-FVector(30,30,10)),At);
        }
        ObservationCamera->SetActorLocationAndRotation(At,(Center-At).Rotation());
    }
    if(BrotherWalking && Brother && Selected() && !Selected()->Motion->Dead)
    {
        FVector Goal=Selected()->GetActorLocation();if(Selected()->Motion->Aquatic)Goal=FVector(Goal.X,-1320,100);
        const FVector Direction=(Goal-Brother->GetActorLocation()).GetSafeNormal2D();
        Brother->GetCharacterMovement()->MaxWalkSpeed=300;
        if(FVector::Dist2D(Goal,Brother->GetActorLocation())>160)Brother->AddMovementInput(Direction,1,true);
        else {BrotherWalking=false;Brother->GetCharacterMovement()->StopMovementImmediately();}
    }
    if(Verify)TickVerification(Delta);
}
void AHearthwardAnimalDemoController::SetupInputComponent()
{
    Super::SetupInputComponent();
    // Development's legacy F1-F5 view modes execute independently of action consumption.
    // Reserve these keys only in this exhibit's controller; do not change shared input config.
    if(PlayerInput)PlayerInput->DebugExecBindings.RemoveAll([](const FKeyBind& Binding)
    {return Binding.Key==EKeys::F1 || Binding.Key==EKeys::F2 || Binding.Key==EKeys::F3 || Binding.Key==EKeys::F4 || Binding.Key==EKeys::F5;});
    InputComponent->BindKey(EKeys::F5,IE_Pressed,this,&AHearthwardAnimalDemoController::Next);
    InputComponent->BindKey(EKeys::F1,IE_Pressed,this,&AHearthwardAnimalDemoController::Back);
    InputComponent->BindKey(EKeys::F2,IE_Pressed,this,&AHearthwardAnimalDemoController::Brother);
    InputComponent->BindKey(EKeys::F3,IE_Pressed,this,&AHearthwardAnimalDemoController::Kill);
    InputComponent->BindKey(EKeys::F4,IE_Pressed,this,&AHearthwardAnimalDemoController::Reset);
    InputComponent->BindKey(EKeys::Escape,IE_Pressed,this,&AHearthwardAnimalDemoController::Exit);
}
void AHearthwardAnimalDemoController::Next(){if(auto* D=Demo(this))D->NextAnimal();}
void AHearthwardAnimalDemoController::Back(){if(auto* D=Demo(this))D->ReturnToPlayer();}
void AHearthwardAnimalDemoController::Brother(){if(auto* D=Demo(this))D->BrotherApproach();}
void AHearthwardAnimalDemoController::Kill(){if(auto* D=Demo(this))D->LethalSelected();}
void AHearthwardAnimalDemoController::Reset(){if(auto* D=Demo(this))D->ResetAnimals();}
void AHearthwardAnimalDemoController::Exit(){ConsoleCommand(TEXT("quit"));}
void UHearthwardAnimalDemoWidget::NativeConstruct()
{
    Super::NativeConstruct();Typeface=MakeShared<FCompositeFont>(NAME_None,FPaths::ProjectDir()/TEXT("Resources/UI/Fonts/LXGWWenKai-Regular.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
}
int32 UHearthwardAnimalDemoWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geo,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const
{
    const int32 Base=Super::NativePaint(Args,Geo,Cull,Out,Layer,Style,Enabled);const auto* D=Demo(this);if(!D)return Base;
    auto Text=[&](const FString& Value,float X,float Y,int32 Size,FLinearColor Color=FLinearColor::White)
    {FSlateDrawElement::MakeText(Out,Base+2,Geo.ToPaintGeometry(FVector2D(1100,60),FSlateLayoutTransform(FVector2D(X,Y))),Value,FSlateFontInfo(Typeface,Size),ESlateDrawEffect::None,Color);};
    auto Panel=[&](float X,float Y,float W,float H)
    {FSlateDrawElement::MakeBox(Out,Base+1,Geo.ToPaintGeometry(FVector2D(W,H),FSlateLayoutTransform(FVector2D(X,Y))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(.018f,.035f,.027f,.82f));};
    Panel(20,20,590,118);Text(TEXT("动物活动演示 · 14种各一只"),36,30,25,FLinearColor(.93f,.8f,.48f));
    Text(TEXT("F5 下一种近景  ·  F1 返回角色  ·  WASD / 鼠标移动"),36,72,17);
    Text(TEXT("F2 弟弟靠近/等待  ·  F3 致命伤  ·  F4 重置  ·  Esc 退出"),36,105,17);
    const float X=Geo.GetLocalSize().X-300;Panel(X,20,280,494);
    Text(TEXT("动物 / 当前行为"),X+16,30,19,FLinearColor(.93f,.8f,.48f));
    for(int32 I=0;I<D->Animals.Num();++I)
    {
        const auto* M=D->Animals[I]->Motion.Get();const FLinearColor C=I==D->SelectedIndex?FLinearColor(.95f,.8f,.35f):M->Dead?FLinearColor(.6f,.62f,.6f):FLinearColor(.87f,.93f,.88f);
        Text(M->DisplayName+TEXT(" · ")+M->Behavior,X+16,64+I*29,15,C);
    }
    Text(TEXT("围栏内44×32米 · 水池12×24米"),X+16,474,15);
    if(const auto* A=D->Selected())
    {
        const auto* M=A->Motion.Get();Panel(20,Geo.GetLocalSize().Y-110,660,86);
        Text(M->DisplayName+TEXT(" · ")+M->Behavior+FString::Printf(TEXT(" · %.2f 米/秒"),M->GroundSpeed/100),36,Geo.GetLocalSize().Y-100,22,FLinearColor(.95f,.8f,.35f));
        Text(D->Feedback,36,Geo.GetLocalSize().Y-62,17);
    }
    return Base+3;
}
void AHearthwardAnimalDemoHUD::BeginPlay()
{
    Super::BeginPlay();Widget=CreateWidget<UHearthwardAnimalDemoWidget>(GetOwningPlayerController());Widget->AddToViewport();Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
}
