#include "../UI/HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "../Experience/HearthwardPlayerSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/SViewport.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapExplorationBoundaries064Test,
    "Hearthward.Map064.ExplorationBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMapExplorationBoundaries064Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();
    Instance->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale=100;
    auto* World=Instance->GetWorld();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);
    const TSharedRef<SViewport> SlateViewport=SNew(SViewport);
    TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SlateViewport);
    const auto Cleanup=[&]()
    {
        SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    };
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);
    Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);
    // This standalone world has not initialized actors for play, so register the controller explicitly.
    World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);
    Player->SetActorLocation(FVector(0,0,90));
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);
    Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);
    Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);
    Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Combat=NewObject<UHearthwardCombatComponent>(Player);
    Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
    auto* Timer=NewObject<UHearthwardTimedActionComponent>(Player);
    Player->AddInstanceComponent(Timer);Timer->RegisterComponent();
    auto* Building=NewObject<UHearthwardBuildingComponent>(Player);
    Player->AddInstanceComponent(Building);Building->RegisterComponent();
    auto* Presentation=NewObject<UHearthwardPresentationComponent>(Player);
    Player->AddInstanceComponent(Presentation);Presentation->RegisterComponent();
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Phase=TEXT("occupied");
    Gameplay->TickComponent(.3f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Exploration samples come from the real gameplay tick"),Gameplay->Explored.Num(),1);
    const double Radius=HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("tuning")),TEXT("fogRadius"));
    TestEqual(TEXT("The approved map exploration radius remains one hundred meters"),Radius,10000.);
    auto* Screen=CreateWidget<UHearthwardScreenWidget>(Controller);
    if(!TestNotNull(TEXT("Real map widget is created"),Screen))
    {
        Cleanup();return false;
    }
    Screen->SetIsFocusable(true);Screen->TakeWidget();Screen->AddToViewport();
    const auto Capture=[&](const TCHAR* Label)
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("Map064Render"))
            || FParse::Param(FCommandLine::Get(),TEXT("NullRHI")) || !FApp::CanEverRender())return;
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("Task064/native-widget-offscreen");
        IFileManager::Get().MakeDirectory(*Directory,true);
        const FString Name=FString(Label)+TEXT(".png");
        FWidgetRenderer Renderer(true);
        auto* Target=UKismetRenderingLibrary::CreateRenderTarget2D(World,1696,954,RTF_RGBA8,FLinearColor::Black);
        if(!Target){AddError(TEXT("Native map widget offscreen render target unavailable"));return;}
        Renderer.DrawWidget(Target,Screen->TakeWidget(),FVector2D(1696,954),0,false);
        UKismetRenderingLibrary::ExportRenderTarget(World,Target,Directory,Name);
        UKismetRenderingLibrary::ReleaseRenderTarget2D(Target);
        if(IFileManager::Get().FileSize(*(Directory/Name))<=0)AddError(TEXT("Native map widget offscreen PNG export failed: ")+Name);
        FFileHelper::SaveStringToFile(Screen->DescribeLayout(),*(Directory/(FString(Label)+TEXT("-layout.json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        const FString Method=TEXT("{\"method\":\"native widget offscreen FWidgetRenderer DrawWidget to RGBA8 render target; explicit C++ fixture; no PIE or input acceptance\",\"width\":1696,\"height\":954}");
        FFileHelper::SaveStringToFile(Method,*(Directory/TEXT("method.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    };
    Screen->OpenPage(TEXT("map"));
    TestTrue(TEXT("New local map remains the default presentation"),Screen->DescribeLayout().Contains(TEXT("map.terrain")));
    TestTrue(TEXT("World map gameplay is reachable through the new UI"),Screen->ExecuteAction(TEXT("map.world")));
    if(!TestEqual(TEXT("Normal map entry is available"),Screen->GetPage(),FName(TEXT("map"))))
    {
        Screen->RemoveFromParent();Cleanup();return false;
    }
    const auto Layout=[&]()
    {
        TSharedPtr<FJsonObject> Result;
        const bool Parsed=FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),Result);
        TestTrue(TEXT("The real widget layout is valid JSON"),Parsed && Result.IsValid());return Result;
    };
    const auto InitialLayout=Layout();
    if(!InitialLayout.IsValid())
    {
        Screen->RemoveFromParent();Cleanup();return false;
    }
    FVector4 Canvas(0,0,0,0);
    for(const auto& Value:InitialLayout->GetArrayField(TEXT("components")))
    {
        const auto Row=Value->AsObject();if(Row->GetStringField(TEXT("id"))!=TEXT("map.canvas"))continue;
        const auto& Rect=Row->GetArrayField(TEXT("rect"));Canvas=FVector4(Rect[0]->AsNumber(),Rect[1]->AsNumber(),Rect[2]->AsNumber(),Rect[3]->AsNumber());
    }
    if(!TestTrue(TEXT("Real map layout exposes its canvas"),Canvas.Z>0 && Canvas.W>0))
    {
        Screen->RemoveFromParent();Cleanup();return false;
    }
    int32 FogElements=0;
    for(const auto& Value:InitialLayout->GetArrayField(TEXT("components")))
        if(Value->AsObject()->GetStringField(TEXT("id")).StartsWith(TEXT("dynamic:fog:")))++FogElements;
    TestTrue(TEXT("Local exploration does not subdivide the whole map"),FogElements<4000);
    AddInfo(FString::Printf(TEXT("Fog elements for one exploration sample: %d"),FogElements));
    // The campaign map spans the existing 4032-meter terrain. Read its actual canvas transform.
    const auto MapPoint=[&](FVector2D Position)
    {return FVector2D(Canvas.X+Canvas.Z*.5+Position.X*Canvas.Z/403200,Canvas.Y+Canvas.W*.5-Position.Y*Canvas.W/403200);};
    const auto FogAt=[&](FVector2D Position)
    {
        const FVector2D Point=MapPoint(Position);
        for(const auto& Value:InitialLayout->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();
            if(!Row->GetStringField(TEXT("id")).StartsWith(TEXT("dynamic:fog:")) || !Row->GetBoolField(TEXT("visible")))continue;
            const auto& Rect=Row->GetArrayField(TEXT("rect"));
            if(Point.X>=Rect[0]->AsNumber() && Point.X<=Rect[0]->AsNumber()+Rect[2]->AsNumber()
                && Point.Y>=Rect[1]->AsNumber() && Point.Y<=Rect[1]->AsNumber()+Rect[3]->AsNumber())return true;
        }
        return false;
    };
    TestFalse(TEXT("The player's actual explored position is revealed"),FogAt(Gameplay->Explored[0]));
    const FVector2D Outside=Gameplay->Explored[0]+FVector2D(15000,5000);
    TestTrue(TEXT("The boundary probe is outside the approved radius"),FVector2D::Distance(Outside,Gameplay->Explored[0])>Radius);
    TestTrue(TEXT("A point beyond one hundred meters remains behind real map fog"),FogAt(Outside));
    Capture(TEXT("map-initial-exploration"));

    Gameplay->Claimed.Add(TEXT("main_04"));
    TestTrue(TEXT("The existing fifth-main-quest entry is genuinely available"),Gameplay->QuestAvailable(TEXT("main_05")));
    Gameplay->Track(TEXT("main_05"));
    TestEqual(TEXT("The real quest tracking entry selects the fifth objective"),Gameplay->TrackedQuest,FName(TEXT("main_05")));
    const auto& Route=HearthwardData::Catalog()->GetObjectField(TEXT("campaign"))->GetArrayField(TEXT("route_trace"));
    const auto& First=Route[0]->AsArray();
    Player->SetActorLocation(FVector(First[0]->AsNumber()*100,First[1]->AsNumber()*100,90));
    Gameplay->Explored.Reset();Gameplay->TickComponent(.3f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("The route sample comes from real gameplay at its authored start"),Gameplay->Explored.Num(),1);
    Screen->Refresh();
    const auto RouteLayout=Layout();
    if(!RouteLayout.IsValid())
    {
        Screen->RemoveFromParent();Cleanup();return false;
    }
    int32 RevealedRoute=0,UnexploredRoute=0;
    for(const auto& Value:RouteLayout->GetArrayField(TEXT("components")))
    {
        const auto Row=Value->AsObject();FString Text;
        if(Row->GetStringField(TEXT("parent"))!=TEXT("map.canvas") || !Row->GetBoolField(TEXT("visible"))
            || !Row->TryGetStringField(TEXT("text"),Text) || Text!=TEXT("·"))continue;
        const auto& Rect=Row->GetArrayField(TEXT("rect"));
        const FVector2D Point(Rect[0]->AsNumber()+4,Rect[1]->AsNumber()+10);
        if(Point.X<Canvas.X || Point.X>Canvas.X+Canvas.Z || Point.Y<Canvas.Y || Point.Y>Canvas.Y+Canvas.W)continue;
        const FVector2D AuthoredWorld((Point.X-Canvas.X-Canvas.Z*.5)*403200/Canvas.Z,
            -(Point.Y-Canvas.Y-Canvas.W*.5)*403200/Canvas.W);
        if(Gameplay->Explored.ContainsByPredicate([&](FVector2D Seen){return FVector2D::Distance(AuthoredWorld,Seen)<=Radius+.1;}))++RevealedRoute;
        else ++UnexploredRoute;
    }
    TestTrue(TEXT("Already explored authored route marks remain visible"),RevealedRoute>0);
    TestEqual(TEXT("Making a quest available does not reveal the unvisited exact route through fog"),UnexploredRoute,0);
    const auto& Components=RouteLayout->GetArrayField(TEXT("components"));
    const auto HasAction=[&](const FString& Action)
    {
        return Components.ContainsByPredicate([&](const auto& Value)
        { FString Actual;return Value->AsObject()->TryGetStringField(TEXT("action"),Actual) && Actual==Action; });
    };
    TestTrue(TEXT("The real exploration tick discovers the nearby initial camp"),Gameplay->Discovered.Contains(TEXT("camp")));
    TestTrue(TEXT("A discovered camp inside the actual exploration radius keeps its marker"),HasAction(TEXT("location:camp")));
    TestFalse(TEXT("The unvisited ford does not expose a location marker"),HasAction(TEXT("location:route_ford")));
    TestFalse(TEXT("Tracking the quest does not expose its unvisited exact objective position"),Components.ContainsByPredicate([](const auto& Value)
    { FString Asset;return Value->AsObject()->TryGetStringField(TEXT("asset"),Asset) && Asset==TEXT("mapQuestIcon"); }));
    // These are the actual three-line objective and wrapped sidebar that overlapped in the render.
    const FString ExpectedObjective=TEXT("◎ ")+HearthwardData::Text(HearthwardData::Find(TEXT("quests"),TEXT("main_05")),TEXT("objective"));
    TArray<FString> ObjectiveLines;ExpectedObjective.ParseIntoArrayLines(ObjectiveLines,false);
    TestEqual(TEXT("The authored fifth objective keeps all three explicit lines"),ObjectiveLines.Num(),3);
    TSharedPtr<FJsonObject> Description,Objective,Legend,Footer;
    for(const auto& Value:Components)
    {
        const auto Row=Value->AsObject();const FString Id=Row->GetStringField(TEXT("id"));FString Text;
        if(Id==TEXT("map.element.008"))Description=Row;
        if(Id==TEXT("map.element.012"))Legend=Row;
        if(Id==TEXT("map.footer"))Footer=Row;
        if(Row->TryGetStringField(TEXT("text"),Text) && Text.StartsWith(TEXT("◎ ")))Objective=Row;
    }
    if(!TestTrue(TEXT("The real map exposes its description, objective, first legend and footer"),
        Description.IsValid() && Objective.IsValid() && Legend.IsValid() && Footer.IsValid()))
    {
        Screen->RemoveFromParent();Cleanup();return false;
    }
    TestEqual(TEXT("Map guidance retains the complete authored objective"),Objective->GetStringField(TEXT("text")),ExpectedObjective);
    FString ThemeText;TSharedPtr<FJsonObject> Theme;
    if(!TestTrue(TEXT("The real map typography is available"),
        FFileHelper::LoadFileToString(ThemeText,*(FPaths::ProjectDir()/TEXT("Resources/UI/interface.json")))
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ThemeText),Theme)))
    {
        Screen->RemoveFromParent();Cleanup();return false;
    }
    FString ExpectedDescription;
    for(const auto& Value:Theme->GetObjectField(TEXT("worldMapPage"))->GetArrayField(TEXT("elements")))
    {
        const auto Row=Value->AsObject();
        if(HearthwardData::Text(Row,TEXT("layoutId"))==TEXT("map.element.008"))ExpectedDescription=Row->GetStringField(TEXT("text"));
    }
    TestEqual(TEXT("The sidebar retains its complete authored description"),Description->GetStringField(TEXT("text")),ExpectedDescription);
    const auto Typeface=MakeShared<FCompositeFont>(NAME_None,FPaths::ProjectDir()/TEXT("Resources/UI")/
        Theme->GetObjectField(TEXT("typography"))->GetStringField(TEXT("body")),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
    // The production minimum at the explicit 100% fixture scale is 24 design pixels.
    FSlateFontInfo Font(Typeface,18);
    const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    TSharedPtr<FJsonObject> Exploration,ProgressBar;
    for(const auto& Value:Components)
    {
        const auto Row=Value->AsObject();FString Bind;
        if(Row->TryGetStringField(TEXT("bind"),Bind) && Bind==TEXT("exploration"))Exploration=Row;
        if(Row->GetStringField(TEXT("parent"))==TEXT("map.sidebar")
            && Row->GetStringField(TEXT("id")).StartsWith(TEXT("dynamic:bar:")))ProgressBar=Row;
    }
    if(!TestTrue(TEXT("The real default-size map exposes exploration text and its progress bar"),Exploration.IsValid() && ProgressBar.IsValid()))
    {
        Screen->RemoveFromParent();Cleanup();return false;
    }
    const float ExplorationSize=Exploration->GetNumberField(TEXT("font"));
    TestTrue(TEXT("The resolved exploration label keeps the actual 24-pixel body minimum"),ExplorationSize>=24.f);
    FSlateFontInfo ExplorationFont(Typeface,FMath::RoundToInt(ExplorationSize*.75f));
    ExplorationFont.LetterSpacing=Exploration->GetNumberField(TEXT("tracking"));
    const auto ExplorationBounds=Measure->Measure(Exploration->GetStringField(TEXT("text")),ExplorationFont);
    const auto& ExplorationRect=Exploration->GetArrayField(TEXT("rect"));
    const double ExplorationBottom=ExplorationRect[1]->AsNumber()+ExplorationBounds.Y;
    TestTrue(TEXT("The complete resolved exploration label fits its real width"),ExplorationBounds.X<=ExplorationRect[2]->AsNumber()+.1);
    TestTrue(TEXT("The exploration label's measured font height fits its real rectangle"),ExplorationBounds.Y<=ExplorationRect[3]->AsNumber()+.1);
    TestTrue(TEXT("The exploration label's measured font bounds stay above its actual progress bar"),
        ExplorationBottom<=ProgressBar->GetArrayField(TEXT("rect"))[1]->AsNumber()+.1);
    AddInfo(FString::Printf(TEXT("Map069 actual exploration font=%.1f tracking=%.1f measured=%.1fx%.1f bottom=%.1f barY=%.1f"),
        ExplorationSize,Exploration->GetNumberField(TEXT("tracking")),ExplorationBounds.X,ExplorationBounds.Y,ExplorationBottom,
        ProgressBar->GetArrayField(TEXT("rect"))[1]->AsNumber()));
    for(const auto& Row:{Description,Objective})
    {
        const auto& Rect=Row->GetArrayField(TEXT("rect"));
        TArray<FString> SourceLines;Row->GetStringField(TEXT("text")).ParseIntoArrayLines(SourceLines,false);int32 Lines=0;
        for(FString Line:SourceLines)
        {
            while(Line.Len()>1 && Measure->Measure(Line,Font).X>Rect[2]->AsNumber())
            {
                const int32 Count=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,Rect[2]->AsNumber())+1,1,Line.Len());
                Line=Line.Mid(Count);++Lines;
            }
            ++Lines;
        }
        const double RequiredHeight=Lines*24.*1.6;
        const double VisibleBottom=Rect[1]->AsNumber()+RequiredHeight;
        TestTrue(Row==Description?TEXT("The sidebar rectangle contains its actual wrapped text height"):
            TEXT("The objective rectangle contains its actual wrapped text height"),Rect[3]->AsNumber()+.1>=RequiredHeight);
        if(Row==Description)
            TestTrue(TEXT("The first legend leaves a real gap after the complete wrapped description"),
                Legend->GetArrayField(TEXT("rect"))[1]->AsNumber()>=VisibleBottom+8);
        else
        {
            TestTrue(TEXT("The complete objective reserves space above the real canvas bottom"),VisibleBottom<=Canvas.Y+Canvas.W-8+.1);
            TestTrue(TEXT("The complete objective stays clear of the footer"),
                VisibleBottom<=Footer->GetArrayField(TEXT("rect"))[1]->AsNumber()-8+.1);
        }
    }
    Capture(TEXT("map-trace-explored-boundary"));

    TestTrue(TEXT("Return to the new local map is available"),Screen->ExecuteAction(TEXT("map.local")));
    TestTrue(TEXT("Returning preserves the new terrain presentation"),Screen->DescribeLayout().Contains(TEXT("map.terrain")));
    Screen->RemoveFromParent();Cleanup();return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapScaledGuidance069Test,
    "Hearthward.UI069.MapScaledGuidanceKeepsCompleteText",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMapScaledGuidance069Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();
    auto* Settings=Instance->GetSubsystem<UHearthwardPlayerSettings>();Settings->Comfort.TextScale=100;
    auto* World=Instance->GetWorld();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);
    const TSharedRef<SViewport> SlateViewport=SNew(SViewport);
    TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SlateViewport);
    const auto Cleanup=[&]()
    {
        SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    };
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);
    Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);
    World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);
    Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);
    Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);
    Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Combat=NewObject<UHearthwardCombatComponent>(Player);
    Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
    auto* Timer=NewObject<UHearthwardTimedActionComponent>(Player);
    Player->AddInstanceComponent(Timer);Timer->RegisterComponent();
    auto* Building=NewObject<UHearthwardBuildingComponent>(Player);
    Player->AddInstanceComponent(Building);Building->RegisterComponent();
    auto* Presentation=NewObject<UHearthwardPresentationComponent>(Player);
    Player->AddInstanceComponent(Presentation);Presentation->RegisterComponent();
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();
    Campaign->State.Initialize(true);
    bool KeptRemaining=false;
    for(auto& Enemy:Campaign->State.Enemies)if(Enemy.Group==TEXT("base"))
    {
        if(!KeptRemaining)KeptRemaining=true;
        else Enemy.Combat.Health=0;
    }
    if(!TestTrue(TEXT("Authored campaign enemies trigger the real remaining-garrison hint"),Campaign->State.ShowRemaining()))
    {Cleanup();return false;}
    Gameplay->Claimed.Add(TEXT("main_04"));
    if(!TestTrue(TEXT("The authored fifth quest is available in the real campaign fixture"),Gameplay->QuestAvailable(TEXT("main_05"))))
    {Cleanup();return false;}
    Gameplay->Track(TEXT("main_05"));
    const auto& First=HearthwardData::Catalog()->GetObjectField(TEXT("campaign"))->GetArrayField(TEXT("route_trace"))[0]->AsArray();
    Player->SetActorLocation(FVector(First[0]->AsNumber()*100,First[1]->AsNumber()*100,90));
    Gameplay->TickComponent(.3f,LEVELTICK_All,nullptr);
    auto* Screen=CreateWidget<UHearthwardScreenWidget>(Controller);
    if(!TestNotNull(TEXT("Real scaled map widget is created"),Screen)) {Cleanup();return false;}
    Screen->SetIsFocusable(true);Screen->TakeWidget();Screen->AddToViewport();Screen->OpenPage(TEXT("map"));
    TestTrue(TEXT("New local map remains the default presentation"),Screen->DescribeLayout().Contains(TEXT("map.terrain")));
    TestTrue(TEXT("World map gameplay is reachable through the new UI"),Screen->ExecuteAction(TEXT("map.world")));
    FString ThemeText;TSharedPtr<FJsonObject> Theme;
    if(!TestEqual(TEXT("The scaled fixture uses the normal map entry"),Screen->GetPage(),FName(TEXT("map")))
        || !TestTrue(TEXT("The real map typography is readable"),
            FFileHelper::LoadFileToString(ThemeText,*(FPaths::ProjectDir()/TEXT("Resources/UI/interface.json")))
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ThemeText),Theme)))
    {Screen->RemoveFromParent();Cleanup();return false;}
    const auto Typography=Theme->GetObjectField(TEXT("typography"));
    const auto Typeface=MakeShared<FCompositeFont>(NAME_None,FPaths::ProjectDir()/TEXT("Resources/UI")/
        Typography->GetStringField(TEXT("body")),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
    const auto DisplayTypeface=MakeShared<FCompositeFont>(NAME_None,FPaths::ProjectDir()/TEXT("Resources/UI")/
        Typography->GetStringField(TEXT("display")),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
    const auto& ThemeRows=Theme->GetObjectField(TEXT("worldMapPage"))->GetArrayField(TEXT("elements"));
    const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    const auto Rect=[](const TSharedPtr<FJsonObject>& Row)
    {
        const auto& R=Row->GetArrayField(TEXT("rect"));
        return FVector4(R[0]->AsNumber(),R[1]->AsNumber(),R[2]->AsNumber(),R[3]->AsNumber());
    };
    const auto Definition=[&](const FString& Id)->TSharedPtr<FJsonObject>
    {
        for(const auto& Value:ThemeRows)
            if(HearthwardData::Text(Value->AsObject(),TEXT("layoutId"))==Id)return Value->AsObject();
        return nullptr;
    };
    const auto PaintedLines=[&](const TSharedPtr<FJsonObject>& Row,float& Advance)
    {
        const auto Authored=Definition(Row->GetStringField(TEXT("id")));
        const float FontSize=FMath::Max(Authored?float(HearthwardData::Number(Authored,TEXT("font"),18)):24.f,24.f)*Settings->Comfort.TextScale/100.f;
        const FString Role=Authored?HearthwardData::Text(Authored,TEXT("fontRole")):FString();
        const bool Display=Role==TEXT("display") || (Role.IsEmpty() && FontSize>=30);
        FSlateFontInfo Font(Display?DisplayTypeface:Typeface,FMath::RoundToInt(FontSize*.75f));
        Font.LetterSpacing=Authored?HearthwardData::Number(Authored,TEXT("tracking"),HearthwardData::Number(Typography,TEXT("tracking"))):HearthwardData::Number(Typography,TEXT("tracking"));
        Advance=FontSize*1.6f;
        const auto R=Rect(Row);TArray<FString> SourceLines,Lines;Row->GetStringField(TEXT("text")).ParseIntoArrayLines(SourceLines,false);
        for(FString Line:SourceLines)
        {
            while(Line.Len()>1 && Measure->Measure(Line,Font).X>R.Z)
            {
                const int32 Count=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,R.Z)+1,1,Line.Len());
                Lines.Add(Line.Left(Count));Line=Line.Mid(Count);
            }
            Lines.Add(Line);
        }
        TArray<FVector4> Result;
        for(int32 I=0;I<Lines.Num();++I)
        {
            const auto Size=Measure->Measure(Lines[I],Font);
            Result.Add(FVector4(R.X,R.Y+I*Advance,Size.X,Size.Y));
        }
        return Result;
    };
    const auto Overlap=[](const TArray<FVector4>& A,const TArray<FVector4>& B)
    {
        for(const auto& X:A)for(const auto& Y:B)
            if(X.X<Y.X+Y.Z && X.X+X.Z>Y.X && X.Y<Y.Y+Y.W && X.Y+X.W>Y.Y)return true;
        return false;
    };
    const auto Capture=[&](int32 Scale,const TCHAR* Suffix)
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("Map064Render"))
            || FParse::Param(FCommandLine::Get(),TEXT("NullRHI")) || !FApp::CanEverRender())return;
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("Task069/native-map-scaled");
        IFileManager::Get().MakeDirectory(*Directory,true);
        const FString Label=FString::Printf(TEXT("map-long-objective-remaining-%d"),Scale)+Suffix,Name=Label+TEXT(".png");
        FWidgetRenderer Renderer(true);
        auto* Target=UKismetRenderingLibrary::CreateRenderTarget2D(World,1696,954,RTF_RGBA8,FLinearColor::Black);
        if(!Target){AddError(TEXT("Native scaled map render target unavailable"));return;}
        Renderer.DrawWidget(Target,Screen->TakeWidget(),FVector2D(1696,954),0,false);
        UKismetRenderingLibrary::ExportRenderTarget(World,Target,Directory,Name);
        UKismetRenderingLibrary::ReleaseRenderTarget2D(Target);
        if(IFileManager::Get().FileSize(*(Directory/Name))<=0)AddError(TEXT("Native scaled map PNG export failed: ")+Name);
        FFileHelper::SaveStringToFile(Screen->DescribeLayout(),*(Directory/(Label+TEXT("-layout.json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"method\":\"native widget offscreen; explicit campaign fixture; no PIE or input acceptance\",\"width\":1696,\"height\":954,\"textScale\":%d}"),Scale),
            *(Directory/(Label+TEXT("-method.json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    };
    const FString ExpectedObjective=TEXT("◎ ")+HearthwardData::Text(HearthwardData::Find(TEXT("quests"),TEXT("main_05")),TEXT("objective"));
    for(const int32 Scale:{125,150})
    {
        Settings->Comfort.TextScale=Scale;Screen->Refresh();
        TSharedPtr<FJsonObject> Layout;
        if(!TestTrue(TEXT("The refreshed real map layout is valid"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),Layout) && Layout.IsValid()))
        {Screen->RemoveFromParent();Cleanup();return false;}
        TSharedPtr<FJsonObject> Description,Objective,Remaining,Canvas,Footer;TArray<TSharedPtr<FJsonObject>> Legends;
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();const FString Id=Row->GetStringField(TEXT("id"));FString Text;
            if(Id==TEXT("map.element.008"))Description=Row;
            else if(Id==TEXT("map.canvas"))Canvas=Row;
            else if(Id==TEXT("map.footer"))Footer=Row;
            if(Row->TryGetStringField(TEXT("text"),Text))
            {
                if(Text.StartsWith(TEXT("◎ ")))Objective=Row;
                else if(Text==TEXT("○ 剩余驻军的大致区域"))Remaining=Row;
            }
        }
        for(const int32 Id:{12,14,16,18,20,22})
            for(const auto& Value:Layout->GetArrayField(TEXT("components")))
                if(Value->AsObject()->GetStringField(TEXT("id"))==FString::Printf(TEXT("map.element.%03d"),Id))Legends.Add(Value->AsObject());
        if(!TestTrue(TEXT("The scaled map retains the real description, six legends, objective, remaining hint and boundaries"),
            Description.IsValid() && Legends.Num()==6 && Objective.IsValid() && Remaining.IsValid() && Canvas.IsValid() && Footer.IsValid()))
        {Screen->RemoveFromParent();Cleanup();return false;}
        TestEqual(TEXT("Scaling retains the complete authored sidebar"),Description->GetStringField(TEXT("text")),Definition(TEXT("map.element.008"))->GetStringField(TEXT("text")));
        TestEqual(TEXT("Scaling retains the complete authored three-line objective"),Objective->GetStringField(TEXT("text")),ExpectedObjective);
        float DescriptionAdvance,ObjectiveAdvance,RemainingAdvance;
        const auto DescriptionLines=PaintedLines(Description,DescriptionAdvance),ObjectiveLines=PaintedLines(Objective,ObjectiveAdvance),RemainingLines=PaintedLines(Remaining,RemainingAdvance);
        const double DescriptionHeight=DescriptionLines.Num()*DescriptionAdvance,ObjectiveHeight=ObjectiveLines.Num()*ObjectiveAdvance;
        const auto D=Rect(Description),O=Rect(Objective),C=Rect(Canvas),F=Rect(Footer);
        TestTrue(FString::Printf(TEXT("%d%% sidebar reserves its complete wrapped text height"),Scale),D.W+.1>=DescriptionHeight);
        TestTrue(FString::Printf(TEXT("%d%% first legend stays below the complete description"),Scale),Rect(Legends[0]).Y>=D.Y+DescriptionHeight+8);
        TArray<FVector4> Previous;
        for(const auto& Legend:Legends)
        {
            float Advance;const auto Lines=PaintedLines(Legend,Advance);const auto L=Rect(Legend);
            const FString Id=Legend->GetStringField(TEXT("id"));
            TestEqual(TEXT("Scaling retains each complete authored legend"),Legend->GetStringField(TEXT("text")),Definition(Id)->GetStringField(TEXT("text")));
            for(const auto& Line:Lines)
                TestTrue(FString::Printf(TEXT("%d%% %s contains its measured font bounds"),Scale,*Id),Line.Y+Line.W<=L.Y+L.W+.1);
            TestFalse(FString::Printf(TEXT("%d%% consecutive legends keep distinct measured text lines"),Scale),Overlap(Previous,Lines));Previous=Lines;
        }
        TestTrue(FString::Printf(TEXT("%d%% objective reserves the complete wrapped text"),Scale),O.W+.1>=ObjectiveHeight);
        TestTrue(FString::Printf(TEXT("%d%% objective stays above the canvas bottom and footer"),Scale),
            O.Y+ObjectiveHeight<=C.Y+C.W-8+.1 && O.Y+ObjectiveHeight<=F.Y-8+.1);
        for(const auto& Line:RemainingLines)
            TestTrue(FString::Printf(TEXT("%d%% remaining-garrison hint stays inside the clipping canvas"),Scale),
                Line.X>=C.X && Line.X+Line.Z<=C.X+C.Z+.1 && Line.Y>=C.Y && Line.Y+Line.W<=C.Y+C.W+.1);
        TestFalse(FString::Printf(TEXT("%d%% remaining-garrison hint and tracked objective keep distinct measured text lines"),Scale),Overlap(ObjectiveLines,RemainingLines));
        AddInfo(FString::Printf(TEXT("Map069 scale=%d descriptionLines=%d reservedHeight=%.1f rectHeight=%.1f firstLegendY=%.1f objectiveLines=%d remainingLines=%d remainingLastMeasuredBottom=%.1f"),
            Scale,DescriptionLines.Num(),DescriptionHeight,D.W,Rect(Legends[0]).Y,ObjectiveLines.Num(),RemainingLines.Num(),RemainingLines.Last().Y+RemainingLines.Last().W));
        Capture(Scale,TEXT(""));
        Screen->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::End,FModifierKeysState(),0,false,0,0));
        TSharedPtr<FJsonObject> Tail;
        if(!TestTrue(TEXT("End exposes a real scrollable map layout"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),Tail) && Tail.IsValid()))
        {Screen->RemoveFromParent();Cleanup();return false;}
        TSharedPtr<FJsonObject> Travel,Sidebar;
        for(const auto& Value:Tail->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();FString Action;
            if(Row->GetStringField(TEXT("id"))==TEXT("map.sidebar"))Sidebar=Row;
            if(Row->TryGetStringField(TEXT("action"),Action) && Action==TEXT("travel"))Travel=Row;
        }
        if(!TestTrue(TEXT("Scrolled sidebar retains its actual camp action"),Travel.IsValid() && Sidebar.IsValid()))
        {Screen->RemoveFromParent();Cleanup();return false;}
        const auto T=Rect(Travel),S=Rect(Sidebar);
        TestTrue(FString::Printf(TEXT("%d%% End makes the complete camp action visible inside its sidebar"),Scale),
            Travel->GetBoolField(TEXT("visible")) && T.Y>=S.Y+16-.1 && T.Y+T.W<=S.Y+S.W-8+.1);
        TestEqual(TEXT("The visible scrolled camp button keeps its actual hit target"),Screen->ActionAt(FVector2D(T.X+T.Z*.5,T.Y+T.W*.5)),FString(TEXT("travel")));
        Capture(Scale,TEXT("-sidebar-end"));
        const auto WheelLayout=[&](FVector4& Terrain,FVector4& Description)
        {
            TSharedPtr<FJsonObject> State;
            if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),State) || !State.IsValid())return false;
            bool FoundTerrain=false,FoundDescription=false;
            for(const auto& Value:State->GetArrayField(TEXT("components")))
            {
                const auto Row=Value->AsObject();FString Asset;
                if(Row->TryGetStringField(TEXT("asset"),Asset) && Asset==TEXT("mapTerrain")) {Terrain=Rect(Row);FoundTerrain=true;}
                if(Row->GetStringField(TEXT("id"))==TEXT("map.element.008")) {Description=Rect(Row);FoundDescription=true;}
            }
            return FoundTerrain && FoundDescription;
        };
        const FGeometry Geometry=FGeometry::MakeRoot(FVector2D(1696,954),FSlateLayoutTransform(1.25f,FVector2D(40,30)));
        const FVector2D Design(1672,941);
        const float PaintScale=FMath::Min(Geometry.GetLocalSize().X/Design.X,Geometry.GetLocalSize().Y/Design.Y);
        const FVector2D Offset=(Geometry.GetLocalSize()-Design*PaintScale)*.5;
        const FVector2D SidebarPointer=Geometry.LocalToAbsolute(Offset+FVector2D(S.X+S.Z*.5,S.Y+S.W*.5)*PaintScale);
        const FVector2D CanvasPointer=Geometry.LocalToAbsolute(Offset+FVector2D(C.X+C.Z*.5,C.Y+C.W*.5)*PaintScale);
        FVector4 TerrainBefore,DescriptionBefore,TerrainSide,DescriptionSide,TerrainCanvas,DescriptionCanvas;
        if(!TestTrue(TEXT("Wheel baseline resolves actual terrain and description"),WheelLayout(TerrainBefore,DescriptionBefore)))
        {Screen->RemoveFromParent();Cleanup();return false;}
        Screen->NativeOnMouseWheel(Geometry,FPointerEvent(0,SidebarPointer,SidebarPointer,TSet<FKey>(),EKeys::Invalid,1.f,FModifierKeysState()));
        if(!TestTrue(TEXT("Sidebar wheel exposes actual refreshed layout"),WheelLayout(TerrainSide,DescriptionSide)))
        {Screen->RemoveFromParent();Cleanup();return false;}
        TestTrue(FString::Printf(TEXT("%d%% sidebar wheel moves description toward the top of its content"),Scale),DescriptionSide.Y>DescriptionBefore.Y+.1);
        TestTrue(TEXT("Sidebar wheel keeps the real map terrain rectangle unchanged"),TerrainSide==TerrainBefore);
        Screen->NativeOnMouseWheel(Geometry,FPointerEvent(0,CanvasPointer,CanvasPointer,TSet<FKey>(),EKeys::Invalid,1.f,FModifierKeysState()));
        if(!TestTrue(TEXT("Canvas wheel exposes actual refreshed layout"),WheelLayout(TerrainCanvas,DescriptionCanvas)))
        {Screen->RemoveFromParent();Cleanup();return false;}
        TestTrue(FString::Printf(TEXT("%d%% canvas wheel enlarges the real map terrain range"),Scale),TerrainCanvas.Z>TerrainSide.Z+.1 && TerrainCanvas.W>TerrainSide.W+.1);
        TestTrue(TEXT("Canvas wheel keeps the scrolled sidebar rectangle unchanged"),DescriptionCanvas==DescriptionSide);
        Screen->NativeOnMouseWheel(Geometry,FPointerEvent(0,CanvasPointer,CanvasPointer,TSet<FKey>(),EKeys::Invalid,-1.f,FModifierKeysState()));
        Screen->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::Home,FModifierKeysState(),0,false,0,0));
        TSharedPtr<FJsonObject> Home;
        if(!TestTrue(TEXT("Home restores a real map layout"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),Home) && Home.IsValid()))
        {Screen->RemoveFromParent();Cleanup();return false;}
        for(const auto& Value:Home->GetArrayField(TEXT("components")))
            if(Value->AsObject()->GetStringField(TEXT("id"))==TEXT("map.element.008"))
                TestEqual(TEXT("Home restores the top description position without changing the map page"),Rect(Value->AsObject()).Y,D.Y);
        TestEqual(TEXT("Reading the sidebar retains the actual map page"),Screen->GetPage(),FName(TEXT("map")));
    }
    TestTrue(TEXT("Return to the new local map is available"),Screen->ExecuteAction(TEXT("map.local")));
    TestTrue(TEXT("Returning preserves the new terrain presentation"),Screen->DescribeLayout().Contains(TEXT("map.terrain")));
    Screen->RemoveFromParent();Cleanup();return true;
}
#endif
