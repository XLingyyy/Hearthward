// PROTOTYPE_ONLY: explicit Development fixture; only starts in a disposable GUID save pool.
#if !UE_BUILD_SHIPPING
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/CharacterMovementComponent.h"
namespace
{
struct FLocalMapVerification
{
    FString Output;
    TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>(),States=MakeShared<FJsonObject>();
    bool Passed=true,Observe=false,PlayerHidden=false,BrotherHidden=false,WasPaused=false;
    int32 Step=0,GroundRow=0;
    double Started=FPlatformTime::Seconds(),WaitUntil=0;
    TWeakObjectPtr<ACameraActor> Camera;
    TArray<TSharedPtr<FJsonValue>> Heights;
    FVector OriginalPlayer,OriginalBrother,Center=FVector(92000,53500,0);
    void Check(FString Name,bool Value)
    { Checks->SetBoolField(Name,Value);Passed&=Value;UE_LOG(LogTemp,Log,TEXT("Local map %s: %s"),*Name,Value?TEXT("PASS"):TEXT("FAIL")); }
    bool Finish()
    {
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("passed"),Passed);J->SetObjectField(TEXT("checks"),Checks);J->SetObjectField(TEXT("states"),States);
        J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        J->SetStringField(TEXT("scope"),TEXT("Actual standalone new-game spawn, overhead cameras, collision height samples and actor footprints. M through the game viewport opens the reference view and resets it on reopening; live actor coordinates, uncovered rectangular bounds, left-button dragging, zoom/pan, paused/unpaused pages and image exports. Development fixture in a GUID pool; no player save modifications or Windows physical-input claims."));
        FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));
        FFileHelper::SaveStringToFile(Text,*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);return false;
    }
};
bool VerifyLocalMap(float)
{
    static FLocalMapVerification Run;
    if(Run.Output.IsEmpty())
    {
        FString Pool;FGuid Id;
        if(!FParse::Value(FCommandLine::Get(),TEXT("HearthwardMapVerify="),Run.Output)
            || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id) || !Id.IsValid())return false;
        Run.Observe=FParse::Param(FCommandLine::Get(),TEXT("HearthwardMapObserve"));
        FString ConfigText;TSharedPtr<FJsonObject> Config;
        const bool Loaded=FFileHelper::LoadFileToString(ConfigText,*(FPaths::ProjectDir()/TEXT("Resources/UI/interface.json")))
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ConfigText),Config);
        const auto Map=Loaded?Config->GetObjectField(TEXT("localMap")):nullptr;
        const TArray<TSharedPtr<FJsonValue>>* Size=nullptr;
        const bool Rectangle=Map && Map->TryGetArrayField(TEXT("sizeCm"),Size) && Size->Num()==2 && (*Size)[0]->AsNumber()==300000 && (*Size)[1]->AsNumber()==200000;
        Run.Check(TEXT("rectangle_configuration_is_3000_by_2000_metres"),Rectangle);
        Run.Check(TEXT("fog_and_circle_configuration_removed"),Map && !Map->HasField(TEXT("outlineFractions")) && !Map->HasField(TEXT("fogStyle")));
        if(!Rectangle)return Run.Finish();
        Run.States->SetArrayField(TEXT("rectangle_size_cm"),*Size);
    }
    const double Now=FPlatformTime::Seconds();
    if(Now-Run.Started>260){Run.Check(TEXT("completion_within_timeout"),false);return Run.Finish();}
    if(Now<Run.WaitUntil || !GEngine)return true;
    UWorld* World=nullptr;
    for(const auto& C:GEngine->GetWorldContexts())if(C.WorldType==EWorldType::Game && C.World()){World=C.World();break;}
    auto* PC=World?World->GetFirstPlayerController():nullptr;auto* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;auto* UI=HUD?HUD->Screen.Get():nullptr;
    if(!UI || !Pawn || !World->HasBegunPlay() || World->GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>()->IsLoading())return true;
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();
    if(!Campaign->Active() || !Campaign->State.Facts.Contains(TEXT("prologue_intro")))return true;
    AHearthwardCompanionFixture* Brother=nullptr;for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It){Brother=*It;break;}
    const FString Folder=FPaths::GetPath(Run.Output);
    // The map pauses the game, so explicitly update the observer camera's cache.
    if(Run.Camera.IsValid() && PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(.016f);
    auto Snapshot=[&]() {TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),J);return J;};
    auto Find=[&](const FString& Id)->TSharedPtr<FJsonObject>
    { const auto J=Snapshot();for(const auto& V:J->GetArrayField(TEXT("elements")))if(V->AsObject()->GetStringField(TEXT("id"))==Id)return V->AsObject();return nullptr; };
    auto Project=[&](FVector World)
    {
        const auto J=Snapshot()->GetObjectField(TEXT("mapView"));
        const auto& C=J->GetArrayField(TEXT("regionCenterCm"));const auto& P=J->GetArrayField(TEXT("pan"));const auto& V=J->GetArrayField(TEXT("viewport"));
        return FVector2D((V[0]->AsNumber()+V[2]->AsNumber())*.5+P[0]->AsNumber(),(V[1]->AsNumber()+V[3]->AsNumber())*.5+P[1]->AsNumber())
            +FVector2D(World.X-C[0]->AsNumber(),World.Y-C[1]->AsNumber())*J->GetNumberField(TEXT("scaleDesignUnitsPerCm"));
    };
    auto CheckAnchor=[&](FString Name,FString Id,FVector At)
    {
        const auto E=Find(Id);Run.Check(Name+TEXT("_marker_present"),E.IsValid());if(!E)return;
        const FVector2D Anchor(E->GetNumberField(TEXT("x"))+E->GetNumberField(TEXT("width"))*.5,E->GetNumberField(TEXT("y"))+E->GetNumberField(TEXT("height"))*.9);
        Run.Check(Name+TEXT("_actual_actor_xy"),FVector2D::Distance(Anchor,Project(At))<.02);
        Run.Check(Name+TEXT("_visibility"),E->GetBoolField(TEXT("visible")));
    };
    auto ScreenPoint=[&](FVector2D Point)
    {
        const auto& G=UI->GetCachedGeometry();const double Scale=FMath::Min(G.GetLocalSize().X/1672.,G.GetLocalSize().Y/941.);
        return G.LocalToAbsolute((G.GetLocalSize()-FVector2D(1672,941)*Scale)*.5+Point*Scale);
    };
    auto Pointer=[&](FVector2D At,FVector2D Before,bool Down,FKey Button=EKeys::LeftMouseButton)
    {return FPointerEvent(0,ScreenPoint(At),ScreenPoint(Before),Down?TSet<FKey>{EKeys::LeftMouseButton}:TSet<FKey>(),Button,0,FModifierKeysState());};
    auto Wheel=[&](float Amount)
    {const FVector2D At=ScreenPoint({900,400});UI->NativeOnMouseWheel(UI->GetCachedGeometry(),FPointerEvent(0,At,At,TSet<FKey>(),FKey(),Amount,FModifierKeysState()));};
    auto Trace=[&](const TCHAR* Name){auto J=Snapshot();J->SetStringField(TEXT("page"),UI->GetPage().ToString());Run.States->SetObjectField(Name,J);};
    auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Folder/(FString(Name)+TEXT(".png")),true,false);};
    auto ExportSize=[&](const TCHAR* Name,int32 Width,int32 Height)
    {
        IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Task020")),true);
        const FString Id=FPaths::GetBaseFilename(Folder)+TEXT("-")+Name;
        const bool Done=UI->CaptureUI(Id,Width,Height);
        const FString Source=FPaths::ProjectSavedDir()/TEXT("Task020")/(Id+TEXT(".png"));
        Run.Check(FString(TEXT("render_"))+Name,Done && IFileManager::Get().Copy(*(Folder/(FString(Name)+TEXT(".png"))),*Source)==COPY_OK);
        const FString Metadata=FPaths::ProjectSavedDir()/TEXT("Task020")/(Id+TEXT(".json"));
        Run.Check(FString(TEXT("metadata_"))+Name,IFileManager::Get().Copy(*(Folder/(FString(Name)+TEXT(".json"))),*Metadata)==COPY_OK);
    };
    auto ExportVisibilityPair=[&](const TCHAR* Name,int32 Width,int32 Height)
    {
        const FString Id=FPaths::GetBaseFilename(Folder)+TEXT("-")+Name;
        bool Done=UI->CaptureMapVisibilityPair(Id,Width,Height);
        for(const FString Suffix:{FString(TEXT("-base")),FString(TEXT("-probe"))})
        {
            const FString Source=FPaths::ProjectSavedDir()/TEXT("Task020")/(Id+Suffix+TEXT(".png"));
            Done&=IFileManager::Get().Copy(*(Folder/(FString(Name)+Suffix+TEXT(".png"))),*Source)==COPY_OK;
            const FString Metadata=FPaths::ProjectSavedDir()/TEXT("Task020")/(Id+Suffix+TEXT(".json"));
            Done&=IFileManager::Get().Copy(*(Folder/(FString(Name)+Suffix+TEXT(".json"))),*Metadata)==COPY_OK;
        }
        Run.Check(FString(TEXT("render_"))+Name+TEXT("_opacity_pair"),Done);
    };
    auto Key=[&](FKey K){const FKeyEvent E(K,FModifierKeysState(),0,false,0,0);UI->NativeOnKeyDown(UI->GetCachedGeometry(),E);};
    auto GameMapKey=[&]()
    {
        auto* Viewport=World->GetGameInstance()->GetGameViewportClient();
        Viewport->InputKey(FInputKeyEventArgs(Viewport->Viewport,FInputDeviceId::CreateFromInternalId(0),EKeys::M,IE_Pressed,1.f,false,FPlatformTime::Cycles64()));
        PC->PlayerTick(.02f);
        Viewport->InputKey(FInputKeyEventArgs(Viewport->Viewport,FInputDeviceId::CreateFromInternalId(0),EKeys::M,IE_Released,0.f,false,FPlatformTime::Cycles64()));
    };
    auto DefaultView=[&]()
    {
        const auto Terrain=Find(TEXT("map.terrain"));const auto Map=Snapshot()->GetObjectField(TEXT("mapView"));
        return Terrain && !Find(TEXT("map.fog")) && Terrain->GetStringField(TEXT("asset"))==TEXT("mapLocalTerrain")
            && FMath::Abs(Map->GetNumberField(TEXT("zoom"))-1.15)<.001;
    };
    switch(Run.Step)
    {
    case 0:
    {
        Run.OriginalPlayer=Pawn->GetActorLocation();Run.OriginalBrother=Brother?Brother->GetActorLocation():FVector::ZeroVector;
        Run.Check(TEXT("new_game_spawn_in_authored_stonehold_bedroom"),FVector::DistXY(Run.OriginalPlayer,Run.Center)<200);
        Run.Check(TEXT("new_game_has_actual_brother"),Brother!=nullptr);
        UI->OpenPage(TEXT("hud"));GameMapKey();
        Run.Check(TEXT("m_key_opens_map_from_world"),UI->GetPage()==TEXT("map"));
        Run.Check(TEXT("first_m_view_matches_reference_zoom_and_pan"),DefaultView());
        UI->SetVisibility(ESlateVisibility::Hidden);
        // Let the game update its dynamic render buffers for the new distant camera.
        Run.WasPaused=World->IsPaused();if(Run.WasPaused)PC->SetPause(false);
        // Hide only the characters during terrain inspection; actor transforms remain untouched.
        Run.PlayerHidden=Pawn->IsHidden();Pawn->SetActorHiddenInGame(true);
        if(Brother){Run.BrotherHidden=Brother->IsHidden();Brother->SetActorHiddenInGame(true);}
        auto* Camera=World->SpawnActor<ACameraActor>(FVector(Run.Center.X,Run.Center.Y,Run.OriginalPlayer.Z+75000),FRotator(-90,-90,0));Run.Camera=Camera;
        Camera->GetCameraComponent()->FieldOfView=60;Camera->GetCameraComponent()->bConstrainAspectRatio=false;
        PC->SetViewTarget(Camera);Run.Step=1;Run.WaitUntil=Now+1;break;
    }
    case 1:
        Run.Check(TEXT("observer_camera_is_750_m_above_spawn"),FMath::Abs(PC->PlayerCameraManager->GetCameraLocation().Z-Run.OriginalPlayer.Z-75000)<1);
        Shot(TEXT("overhead-perspective"));Run.Step=2;Run.WaitUntil=Now+.6;break;
    case 2:
        Run.Camera->GetCameraComponent()->ProjectionMode=ECameraProjectionMode::Orthographic;
        Run.Camera->GetCameraComponent()->OrthoWidth=80000;Run.Step=3;Run.WaitUntil=Now+.6;break;
    case 3:Shot(TEXT("overhead-orthographic"));Run.Step=4;Run.WaitUntil=Now+.6;break;
    case 4:
    {
        PC->SetViewTarget(Pawn);Run.Camera->Destroy();UI->SetVisibility(ESlateVisibility::Visible);
        Pawn->SetActorHiddenInGame(Run.PlayerHidden);if(Brother)Brother->SetActorHiddenInGame(Run.BrotherHidden);
        if(Run.WasPaused)PC->SetPause(true);
        Run.Step=5;break;
    }
    case 5:
    {
        FCollisionQueryParams Q(SCENE_QUERY_STAT(LocalMapObservation),false);
        for(TActorIterator<AActor> It(World);It;++It)
            if(!It->GetClass()->GetName().Contains(TEXT("Landscape")) && !It->ActorHasTag(TEXT("Hearthward.NatureGround")))Q.AddIgnoredActor(*It);
        const int32 Stop=FMath::Min(Run.GroundRow+10,251);
        for(int32 Y=Run.GroundRow;Y<Stop;++Y)for(int32 X=0;X<251;++X)
        {
            const FVector At=Run.Center+FVector(-25000+X*200,-25000+Y*200,0);FHitResult Hit;
            const bool Found=World->LineTraceSingleByObjectType(Hit,At+FVector(0,0,100000),At-FVector(0,0,30000),FCollisionObjectQueryParams(ECC_WorldStatic),Q);
            Run.Heights.Add(Found?TSharedPtr<FJsonValue>(MakeShared<FJsonValueNumber>(Hit.ImpactPoint.Z/100)):TSharedPtr<FJsonValue>(MakeShared<FJsonValueNull>()));
        }
        Run.GroundRow=Stop;if(Stop<251)break;
        int32 Missing=0;for(const auto& Value:Run.Heights)Missing+=Value->Type==EJson::Null;
        Run.Check(TEXT("all_63001_height_samples_have_actual_ground"),Missing==0);
        auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("samples"),251);J->SetNumberField(TEXT("spacingM"),2);
        J->SetArrayField(TEXT("originM"),{MakeShared<FJsonValueNumber>(670),MakeShared<FJsonValueNumber>(285)});J->SetArrayField(TEXT("heightsM"),Run.Heights);
        J->SetArrayField(TEXT("spawnCm"),{MakeShared<FJsonValueNumber>(Run.OriginalPlayer.X),MakeShared<FJsonValueNumber>(Run.OriginalPlayer.Y),MakeShared<FJsonValueNumber>(Run.OriginalPlayer.Z)});
        // Distributed real collision probes across the entire expanded rectangle.
        const auto Map=Snapshot()->GetObjectField(TEXT("mapView"));const auto& Center=Map->GetArrayField(TEXT("regionCenterCm"));
        TArray<TSharedPtr<FJsonValue>> Probes;int32 ProbeMissing=0;
        for(int32 Y=0;Y<21;++Y)for(int32 X=0;X<31;++X)
        {
            const FVector At(Center[0]->AsNumber()-150000+(X+.5)*300000/31,Center[1]->AsNumber()-100000+(Y+.5)*200000/21,0);FHitResult Hit;
            const bool Found=World->LineTraceSingleByObjectType(Hit,At+FVector(0,0,200000),At-FVector(0,0,30000),FCollisionObjectQueryParams(ECC_WorldStatic),Q);
            auto Probe=MakeShared<FJsonObject>();Probe->SetNumberField(TEXT("xM"),At.X/100);Probe->SetNumberField(TEXT("yM"),At.Y/100);
            Probe->SetBoolField(TEXT("found"),Found);if(Found)Probe->SetNumberField(TEXT("heightM"),Hit.ImpactPoint.Z/100);else ++ProbeMissing;
            Probes.Add(MakeShared<FJsonValueObject>(Probe));
        }
        J->SetArrayField(TEXT("rectangleHeightProbes"),Probes);J->SetObjectField(TEXT("mapView"),Map);
        Run.Check(TEXT("all_651_distributed_rectangle_probes_have_real_ground"),ProbeMissing==0);

        TArray<TSharedPtr<FJsonValue>> Trees,Rocks,Water,Houses;
        auto XYValue=[](FVector V){auto P=MakeShared<FJsonObject>();P->SetNumberField(TEXT("x"),V.X/100);P->SetNumberField(TEXT("y"),V.Y/100);return P;};
        for(TActorIterator<AActor> It(World);It;++It)
        {
            auto* A=*It;FVector Bounds,Extent;A->GetActorBounds(false,Bounds,Extent);
            if(FVector::DistXY(Bounds,Run.Center)>403200+Extent.Size2D())continue;
            TArray<UInstancedStaticMeshComponent*> Components;A->GetComponents<UInstancedStaticMeshComponent>(Components);
            for(auto* C:Components)for(int32 I=0;I<C->GetInstanceCount();++I)
            {
                FTransform Transform;if(!C->GetInstanceTransform(I,Transform,true) || FVector::DistXY(Transform.GetLocation(),Run.Center)>403200)continue;
                if(A->ActorHasTag(TEXT("tree")))Trees.Add(MakeShared<FJsonValueObject>(XYValue(Transform.GetLocation())));
                if(A->ActorHasTag(TEXT("rock")))Rocks.Add(MakeShared<FJsonValueObject>(XYValue(Transform.GetLocation())));
            }
            if(A->ActorHasTag(TEXT("water")))
            {
                auto P=XYValue(A->GetActorLocation());P->SetNumberField(TEXT("z"),A->GetActorLocation().Z/100);
                const FVector Scale=A->GetActorScale3D();P->SetArrayField(TEXT("scale"),{MakeShared<FJsonValueNumber>(Scale.X),MakeShared<FJsonValueNumber>(Scale.Y),MakeShared<FJsonValueNumber>(Scale.Z)});
                if(auto* Mesh=A->FindComponentByClass<UStaticMeshComponent>();Mesh && Mesh->GetStaticMesh())P->SetStringField(TEXT("mesh"),Mesh->GetStaticMesh()->GetName());
                P->SetArrayField(TEXT("boundsM"),{MakeShared<FJsonValueNumber>((Bounds.X-Extent.X)/100),MakeShared<FJsonValueNumber>((Bounds.Y-Extent.Y)/100),MakeShared<FJsonValueNumber>((Bounds.X+Extent.X)/100),MakeShared<FJsonValueNumber>((Bounds.Y+Extent.Y)/100)});
                Water.Add(MakeShared<FJsonValueObject>(P));
            }
            if(A->GetClass()->GetName().Contains(TEXT("CampHouse")))
            {
                auto P=XYValue(Bounds);P->SetNumberField(TEXT("halfX"),Extent.X/100);P->SetNumberField(TEXT("halfY"),Extent.Y/100);Houses.Add(MakeShared<FJsonValueObject>(P));
            }
        }
        // Preserve actual constructed stonehold footprints instead of drawing the old camp-house proxy.
        for(TActorIterator<AActor> It(World);It;++It)if(It->ActorHasTag(TEXT("CampaignHometownFortress")))
        {
            TArray<UStaticMeshComponent*> Parts;It->GetComponents<UStaticMeshComponent>(Parts);
            for(const auto* Part:Parts)
            {
                const FString Name=Part->GetName();
                if(!Name.StartsWith(TEXT("BedroomCeiling")) && !Name.StartsWith(TEXT("GalleryRoof"))
                    && !Name.StartsWith(TEXT("VillageHouse")) && !Name.StartsWith(TEXT("Tower"))
                    && !Name.StartsWith(TEXT("Curtain")) && !Name.StartsWith(TEXT("FacadePlinth")))continue;
                const auto B=Part->Bounds;auto P=XYValue(B.Origin);
                P->SetNumberField(TEXT("halfX"),B.BoxExtent.X/100);P->SetNumberField(TEXT("halfY"),B.BoxExtent.Y/100);
                P->SetStringField(TEXT("part"),Name);Houses.Add(MakeShared<FJsonValueObject>(P));
            }
        }
        J->SetArrayField(TEXT("trees"),Trees);J->SetArrayField(TEXT("rocks"),Rocks);J->SetArrayField(TEXT("water"),Water);J->SetArrayField(TEXT("houses"),Houses);
        FString Json;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Json));
        Run.Check(TEXT("export_actual_scene"),FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("scene.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
        if(Run.Observe)return Run.Finish();
        Run.Step=6;Run.WaitUntil=Now+.3;break;
    }
    case 6:
    {
        UI->Refresh();bool Clean=true;int32 Flames=0,Regions=0;
        const auto J=Snapshot();for(const auto& V:J->GetArrayField(TEXT("elements")))
        {
            const auto E=V->AsObject();const auto Type=E->GetStringField(TEXT("type"));Flames+=Type==TEXT("mapFlame");Regions+=Type==TEXT("mapRegion");
            Clean&=E->GetStringField(TEXT("asset"))!=TEXT("mapTerrain") && Type!=TEXT("arrow");
        }
        Run.Check(TEXT("all_map_art_uses_latest_terrain_and_two_flames"),Clean && Flames==2 && Regions>0);
        const auto PlayerMarker=Find(TEXT("map.player")),BrotherMarker=Find(TEXT("map.brother"));
        Run.Check(TEXT("player_flame_red_brother_flame_blue"),PlayerMarker && BrotherMarker && PlayerMarker->GetNumberField(TEXT("red"))>.9 && PlayerMarker->GetNumberField(TEXT("blue"))<.1
            && BrotherMarker->GetNumberField(TEXT("blue"))>.9 && BrotherMarker->GetNumberField(TEXT("red"))<.1);
        const auto Terrain=Find(TEXT("map.terrain"));Run.Check(TEXT("uses_derived_scene_terrain_asset"),Terrain && Terrain->GetStringField(TEXT("asset"))==TEXT("mapLocalTerrain"));
        CheckAnchor(TEXT("initial_player"),TEXT("map.player"),Pawn->GetActorLocation());
        if(Brother)CheckAnchor(TEXT("initial_brother"),TEXT("map.brother"),Brother->GetActorLocation());
        const auto MapState=Snapshot()->GetObjectField(TEXT("mapView"));const auto& View=MapState->GetArrayField(TEXT("viewport"));
        const double Scale=MapState->GetNumberField(TEXT("scaleDesignUnitsPerCm"));
        Run.Check(TEXT("initial_view_shows_part_of_the_rectangle"),300000*Scale>View[2]->AsNumber()-View[0]->AsNumber()+1 || 200000*Scale>View[3]->AsNumber()-View[1]->AsNumber()+1);
        Run.Check(TEXT("no_fog_element_on_any_map_entry"),!Find(TEXT("map.fog")));
        const FRotator PlayerRotation=Pawn->GetActorRotation(),BrotherRotation=Brother?Brother->GetActorRotation():FRotator::ZeroRotator;
        for(const float Yaw:{0.f,90.f,180.f,270.f,45.f})
        {
            Pawn->SetActorRotation(FRotator(0,Yaw,0));if(Brother)Brother->SetActorRotation(FRotator(0,Yaw-90,0));UI->Refresh();
            Run.Check(FString::Printf(TEXT("player_heading_%.0f"),Yaw),FMath::Abs(FMath::FindDeltaAngleDegrees(Find(TEXT("map.player"))->GetNumberField(TEXT("value")),Yaw-90))<.02);
            if(Brother)Run.Check(FString::Printf(TEXT("brother_heading_%.0f"),Yaw-90),FMath::Abs(FMath::FindDeltaAngleDegrees(Find(TEXT("map.brother"))->GetNumberField(TEXT("value")),Yaw-180))<.02);
            ExportSize(*FString::Printf(TEXT("heading-%.0f"),Yaw),1600,1000);
        }
        Pawn->SetActorRotation(PlayerRotation);if(Brother)Brother->SetActorRotation(BrotherRotation);UI->Refresh();
        UI->ExecuteAction(TEXT("map.world"));
        Run.Check(TEXT("location_panel_uses_same_latest_terrain"),Find(TEXT("map.terrain")) && Find(TEXT("map.player")) && !Find(TEXT("map.fog")));
        ExportSize(TEXT("map-locations"),1600,1000);UI->ExecuteAction(TEXT("map.local"));
        Trace(TEXT("initial"));Shot(TEXT("map-in-game"));ExportSize(TEXT("map-1600x1000"),1600,1000);ExportSize(TEXT("map-1920x1080"),1920,1080);
        ExportSize(TEXT("map-1280x720"),1280,720);ExportSize(TEXT("map-ultrawide"),2560,1080);
        ExportSize(TEXT("map-reference-size"),2395,1453);
        ExportVisibilityPair(TEXT("reveal-1600x1000"),1600,1000);ExportVisibilityPair(TEXT("reveal-1920x1080"),1920,1080);
        ExportVisibilityPair(TEXT("reveal-1280x720"),1280,720);ExportVisibilityPair(TEXT("reveal-ultrawide"),2560,1080);

        Pawn->SetActorLocation(Run.OriginalPlayer+FVector(6000,3500,1000));if(Brother)Brother->SetActorLocation(Run.OriginalBrother+FVector(-2000,-2500,1000));
        Run.Step=7;Run.WaitUntil=Now+.2;break;
    }
    case 7:
    {
        CheckAnchor(TEXT("moved_player"),TEXT("map.player"),Pawn->GetActorLocation());
        if(Brother)CheckAnchor(TEXT("moved_brother"),TEXT("map.brother"),Brother->GetActorLocation());
        Trace(TEXT("moved"));ExportSize(TEXT("map-separated-flames"),1600,1000);
        const FVector2D Start(600,350),End=Start+FVector2D(100,70);
        const auto Before=Find(TEXT("map.terrain"));
        const auto Down=UI->NativeOnMouseButtonDown(UI->GetCachedGeometry(),Pointer(Start,Start,true));
        Run.Check(TEXT("left_button_captures_map_drag"),Down.IsEventHandled() && Snapshot()->GetObjectField(TEXT("mapView"))->GetBoolField(TEXT("dragging")));
        UI->NativeOnMouseMove(UI->GetCachedGeometry(),Pointer(End,Start,true));const auto After=Find(TEXT("map.terrain"));
        Run.Check(TEXT("left_drag_moves_terrain_by_cursor_delta"),FMath::Abs(After->GetNumberField(TEXT("x"))-Before->GetNumberField(TEXT("x"))-100)<.02 && FMath::Abs(After->GetNumberField(TEXT("y"))-Before->GetNumberField(TEXT("y"))-70)<.02);
        CheckAnchor(TEXT("dragged_player"),TEXT("map.player"),Pawn->GetActorLocation());Trace(TEXT("dragged"));ExportSize(TEXT("map-dragged"),1600,1000);
        UI->NativeOnMouseMove(UI->GetCachedGeometry(),Pointer(Start+FVector2D(100000,100000),End,true));
        const auto Clamp=Snapshot()->GetObjectField(TEXT("mapView"));const auto& VP=Clamp->GetArrayField(TEXT("viewport"));const auto& Pan=Clamp->GetArrayField(TEXT("pan"));
        const double S=Clamp->GetNumberField(TEXT("scaleDesignUnitsPerCm"));
        Run.Check(TEXT("drag_clamps_both_rectangle_edges"),FMath::Abs(Pan[0]->AsNumber()-(300000*S-(VP[2]->AsNumber()-VP[0]->AsNumber()))*.5)<.02 && FMath::Abs(Pan[1]->AsNumber()-(200000*S-(VP[3]->AsNumber()-VP[1]->AsNumber()))*.5)<.02);
        UI->NativeOnMouseButtonUp(UI->GetCachedGeometry(),Pointer(End,End,false));
        Run.Check(TEXT("left_release_stops_map_drag"),!Snapshot()->GetObjectField(TEXT("mapView"))->GetBoolField(TEXT("dragging")));
        const auto Released=Find(TEXT("map.terrain"));UI->NativeOnMouseMove(UI->GetCachedGeometry(),Pointer(Start,End,false));
        Run.Check(TEXT("moving_after_release_does_not_pan"),Find(TEXT("map.terrain"))->GetNumberField(TEXT("x"))==Released->GetNumberField(TEXT("x")));
        Wheel(6);
        Key(EKeys::Right);Key(EKeys::Up);Run.Step=8;Run.WaitUntil=Now+.2;break;
    }
    case 8:
    {
        CheckAnchor(TEXT("zoom_and_pan_player"),TEXT("map.player"),Pawn->GetActorLocation());
        Trace(TEXT("zoom_pan"));ExportSize(TEXT("map-zoom-pan"),1600,1000);
        ExportVisibilityPair(TEXT("reveal-zoom-pan"),1600,1000);
        Wheel(-100);ExportSize(TEXT("map-minimum-zoom"),1600,1000);Trace(TEXT("minimum_zoom"));
        ExportVisibilityPair(TEXT("reveal-minimum-zoom"),1600,1000);
        const auto C=Snapshot()->GetObjectField(TEXT("mapView"))->GetArrayField(TEXT("regionCenterCm"));
        Pawn->SetActorLocation(FVector(C[0]->AsNumber()+150000,C[1]->AsNumber()+100000,Run.OriginalPlayer.Z));Run.Step=9;Run.WaitUntil=Now+.2;break;
    }
    case 9:
    {
        Run.Check(TEXT("exact_rectangle_corner_visible_without_circle_cut"),Find(TEXT("map.player"))->GetBoolField(TEXT("visible")));
        Pawn->AddActorWorldOffset(FVector(1,0,0));Run.Step=10;Run.WaitUntil=Now+.2;break;
    }
    case 10:
    {
        Run.Check(TEXT("one_cm_outside_rectangle_length_hidden"),!Find(TEXT("map.player"))->GetBoolField(TEXT("visible")));
        Pawn->AddActorWorldOffset(FVector(-1,1,0));Run.Step=11;Run.WaitUntil=Now+.2;break;
    }
    case 11:
        Run.Check(TEXT("one_cm_outside_rectangle_width_hidden"),!Find(TEXT("map.player"))->GetBoolField(TEXT("visible")));
        Trace(TEXT("outside"));Pawn->SetActorLocation(Run.OriginalPlayer);if(Brother)Brother->SetActorLocation(Run.OriginalBrother);
        Key(EKeys::M);Run.Check(TEXT("m_key_closes_map"),UI->GetPage()==TEXT("hud"));
        GameMapKey();Run.Check(TEXT("m_key_reopens_map_from_world"),UI->GetPage()==TEXT("map"));
        Run.Check(TEXT("reopened_m_resets_zoom_and_pan"),DefaultView());Trace(TEXT("reopened"));
        Key(EKeys::Escape);Run.Check(TEXT("escape_returns_to_playable_world"),UI->GetPage()==TEXT("hud") && !World->IsPaused() && !PC->bShowMouseCursor);
        Run.Step=12;Run.WaitUntil=Now+.1;break;
    case 12:
        UI->OpenPage(TEXT("map"));Run.Check(TEXT("reopen_map_preserves_fixed_region"),UI->GetPage()==TEXT("map"));
        GConfig->SetBool(TEXT("Hearthward.Survival"),TEXT("MenuPause"),false,GGameUserSettingsIni);
        UI->OpenPage(TEXT("hud"));UI->OpenPage(TEXT("map"));
        Pawn->SetActorLocation(Run.OriginalPlayer+FVector(-4500,5000,5000));
        Run.Step=13;Run.WaitUntil=Now+.2;break;
    case 13:
        Run.Check(TEXT("map_with_menu_pause_disabled_world_keeps_running"),!World->IsPaused());
        CheckAnchor(TEXT("unpaused_live_player"),TEXT("map.player"),Pawn->GetActorLocation());
        Trace(TEXT("unpaused"));
        if(Brother)Brother->Destroy();Run.Step=14;Run.WaitUntil=Now+.2;break;
    case 14:
        Run.Check(TEXT("missing_brother_has_no_fake_position"),!Find(TEXT("map.brother"))->GetBoolField(TEXT("visible")));
        return Run.Finish();
    }
    return true;
}
const FTSTicker::FDelegateHandle LocalMapTicker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&VerifyLocalMap),.02f);
}
#endif
