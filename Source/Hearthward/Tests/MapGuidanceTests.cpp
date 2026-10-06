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
// TASK-078 replaces the legacy world canvas. Keep the exploration/disclosure and
// scaled-text regression contracts on the real current widget instead of its old rectangles.
namespace
{
struct FMapFixture078
{
    UGameInstance* Instance=nullptr;UWorld* World=nullptr;APlayerController* Controller=nullptr;
    ACharacter* Player=nullptr;UHearthwardGameplayComponent* Gameplay=nullptr;
    UHearthwardScreenWidget* Screen=nullptr;TSharedPtr<FSceneViewport> SceneViewport;
    FMapFixture078()
    {
        Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();World=Instance->GetWorld();
        Instance->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale=100;
        auto* Viewport=NewObject<UGameViewportClient>(GEngine);Instance->GetWorldContext()->GameViewport=Viewport;
        Viewport->Init(*Instance->GetWorldContext(),Instance,false);
        SceneViewport=Viewport->CreateViewport(SNew(SViewport));
        auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
        Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);World->AddController(Controller);
        Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);
        auto Add=[&]<class T>(){auto* C=NewObject<T>(Player);Player->AddInstanceComponent(C);C->RegisterComponent();return C;};
        Add.operator()<UHearthwardInventoryComponent>();Gameplay=Add.operator()<UHearthwardGameplayComponent>();Gameplay->Enabled=true;
        Add.operator()<UHearthwardSurvivalComponent>();Add.operator()<UHearthwardCombatComponent>();
        Add.operator()<UHearthwardTimedActionComponent>();Add.operator()<UHearthwardBuildingComponent>();Add.operator()<UHearthwardPresentationComponent>();
        World->GetSubsystem<UHearthwardCampaignSubsystem>()->State.Phase=TEXT("occupied");
        Screen=CreateWidget<UHearthwardScreenWidget>(Controller);Screen->SetIsFocusable(true);Screen->TakeWidget();Screen->AddToViewport();
    }
    TSharedPtr<FJsonObject> Snapshot() const
    {
        TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeHUDPreview()),J);return J;
    }
    TSharedPtr<FJsonObject> Element(const FString& Action,const FString& Id=FString()) const
    {
        const auto J=Snapshot();
        for(const auto& V:J->GetArrayField(TEXT("elements")))
        {
            auto E=V->AsObject();if((!Action.IsEmpty() && E->GetStringField(TEXT("action"))==Action)
                || (!Id.IsEmpty() && E->GetStringField(TEXT("id"))==Id))return E;
        }
        return nullptr;
    }
    ~FMapFixture078()
    {
        Screen->RemoveFromParent();SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapExplorationBoundaries064Test,
    "Hearthward.Map064.ExplorationBoundaries",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMapExplorationBoundaries064Test::RunTest(const FString&)
{
    FMapFixture078 F;auto* G=F.Gameplay;auto* Screen=F.Screen;
    F.Player->SetActorLocation(FVector(92000,53500,100));G->TickComponent(.3f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Exploration samples still come from real gameplay"),G->Explored.Num(),1);
    const double Radius=HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("tuning")),TEXT("fogRadius"));
    TestEqual(TEXT("Gameplay discovery radius remains one hundred metres"),Radius,10000.);
    Screen->OpenPage(TEXT("map"));
    const auto Initial=F.Element(TEXT(""),TEXT("map.terrain"));
    TestTrue(TEXT("Default map uses the new terrain atlas"),Initial && Initial->GetStringField(TEXT("asset"))==TEXT("mapLocalTerrain"));
    TestTrue(TEXT("Location and travel entry remains available"),Screen->ExecuteAction(TEXT("map.world")));
    const auto Expanded=F.Element(TEXT(""),TEXT("map.terrain"));
    TestTrue(TEXT("Opening locations keeps the exact same terrain projection"),Initial && Expanded && Initial->GetNumberField(TEXT("x"))==Expanded->GetNumberField(TEXT("x")));
    TestFalse(TEXT("An undiscovered ford is not exposed in the location list"),F.Element(TEXT("location:route_ford")).IsValid());
    TSharedPtr<FJsonObject> Theme;FString Source;
    FFileHelper::LoadFileToString(Source,*(FPaths::ProjectDir()/TEXT("Resources/UI/interface.json")));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),Theme);
    const auto Size=Theme->GetObjectField(TEXT("localMap"))->GetArrayField(TEXT("sizeCm"));
    TestEqual(TEXT("The rectangle is exactly three kilometres long"),Size[0]->AsNumber(),300000.);
    TestEqual(TEXT("The rectangle is exactly two kilometres wide"),Size[1]->AsNumber(),200000.);
    TestFalse(TEXT("Fog is absent from the map"),F.Element(TEXT(""),TEXT("map.fog")).IsValid());
    G->Claimed.Add(TEXT("main_04"));TestTrue(TEXT("The existing fifth objective is available"),G->QuestAvailable(TEXT("main_05")));
    G->Track(TEXT("main_05"));TestEqual(TEXT("The real tracking entry selects the fifth objective"),G->TrackedQuest,FName(TEXT("main_05")));
    const auto& Route=HearthwardData::Catalog()->GetObjectField(TEXT("campaign"))->GetArrayField(TEXT("route_trace"));
    const auto& First=Route[0]->AsArray();const FVector Origin(First[0]->AsNumber()*100,First[1]->AsNumber()*100,100);
    F.Player->SetActorLocation(Origin);G->Explored.Reset();G->TickComponent(.3f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Route discovery uses the real gameplay tick"),G->Explored.Num(),1);
    Screen->OpenPage(TEXT("hud"));Screen->OpenPage(TEXT("map"));Screen->ExecuteAction(TEXT("map.world"));
    int32 KnownRoute=0,UnknownRoute=0;
    const auto RouteSnapshot=F.Snapshot();
    for(const auto& Value:RouteSnapshot->GetArrayField(TEXT("elements")))
    {
        const auto E=Value->AsObject();
        TestFalse(TEXT("Every current map excludes the old terrain and arrow"),E->GetStringField(TEXT("asset"))==TEXT("mapTerrain") || E->GetStringField(TEXT("type"))==TEXT("arrow"));
        if(E->GetStringField(TEXT("text"))!=TEXT("·"))continue;
        const auto Terrain=F.Element(TEXT(""),TEXT("map.terrain"));
        const FVector2D World=FVector2D(-201600,-201600)+(FVector2D(E->GetNumberField(TEXT("x"))+4,E->GetNumberField(TEXT("y"))+10)-FVector2D(Terrain->GetNumberField(TEXT("x")),Terrain->GetNumberField(TEXT("y"))))*(403200/Terrain->GetNumberField(TEXT("width")));
        (FVector2D::DistSquared(World,G->Explored[0])<=Radius*Radius+1?KnownRoute:UnknownRoute)++;
    }
    TestTrue(TEXT("Already explored authored route marks stay visible"),KnownRoute>0);
    TestEqual(TEXT("Tracking does not reveal the unvisited exact route"),UnknownRoute,0);
    TestTrue(TEXT("Real exploration discovers the nearby camp"),G->Discovered.Contains(TEXT("camp")));
    TestTrue(TEXT("The discovered camp keeps its actual location action"),F.Element(TEXT("location:camp")).IsValid());
    TestFalse(TEXT("Tracking does not disclose an unexplored objective marker"),F.Element(TEXT(""),TEXT("map.questTarget")).IsValid());
    Screen->OpenPage(TEXT("journal"));Screen->ExecuteAction(TEXT("quest:main_05"));
    TestFalse(TEXT("Quest map refuses an undiscovered target"),Screen->ExecuteAction(TEXT("questMap")));
    G->Discovered.Add(F.World->GetSubsystem<UHearthwardCampaignSubsystem>()->QuestLocation(TEXT("main_05")));
    TestTrue(TEXT("A discovered quest target opens the current map"),Screen->ExecuteAction(TEXT("questMap")));
    TestTrue(TEXT("Quest positioning retains uncovered terrain and red player"),F.Element(TEXT(""),TEXT("map.terrain")).IsValid() && !F.Element(TEXT(""),TEXT("map.fog")).IsValid() && F.Element(TEXT(""),TEXT("map.player")).IsValid());
    TestTrue(TEXT("The location panel can be closed"),Screen->ExecuteAction(TEXT("map.local")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapScaledGuidance069Test,
    "Hearthward.UI069.MapScaledGuidanceKeepsCompleteText",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMapScaledGuidance069Test::RunTest(const FString&)
{
    FMapFixture078 F;F.Gameplay->Discovered.Add(TEXT("camp"));F.Gameplay->Activated.Add(TEXT("camp"));
    F.Gameplay->Claimed.Add(TEXT("main_04"));F.Gameplay->Track(TEXT("main_05"));
    F.Screen->OpenPage(TEXT("map"));F.Screen->ExecuteAction(TEXT("map.world"));
    FString Expected=TEXT("◎ ")+HearthwardData::Text(HearthwardData::Find(TEXT("quests"),TEXT("main_05")),TEXT("objective"));
    for(int32 Scale:{100,125,150})
    {
        F.Instance->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale=Scale;F.Screen->Refresh();
        const auto Travel=F.Element(TEXT("travel")),Camp=F.Element(TEXT(""),TEXT("map.location.list.camp"));
        TestTrue(TEXT("Scaled location and travel controls remain present"),Travel && Camp);
        if(Travel && Camp)
        {
            const auto FontCheck=[&](const auto& E)
            {
                const auto Typeface=MakeShared<FCompositeFont>(FName(TEXT("regular")),FPaths::ProjectDir()/TEXT("Resources/UI/Fonts/LXGWWenKai-Regular.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
                FSlateFontInfo Font(Typeface,FMath::RoundToInt(E->GetNumberField(TEXT("font"))*.75));
                const FVector2D Size=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(E->GetStringField(TEXT("text")),Font);
                TestTrue(FString::Printf(TEXT("%d%% %s: measured %.1fx%.1f within %.1fx%.1f"),Scale,*E->GetStringField(TEXT("text")),Size.X,Size.Y,E->GetNumberField(TEXT("width")),E->GetNumberField(TEXT("height"))),Size.X<=E->GetNumberField(TEXT("width")) && Size.Y<=E->GetNumberField(TEXT("height")));
            };
            FontCheck(Travel);FontCheck(Camp);
            const FVector2D At(Travel->GetNumberField(TEXT("x"))+Travel->GetNumberField(TEXT("width"))*.5,Travel->GetNumberField(TEXT("y"))+Travel->GetNumberField(TEXT("height"))*.5);
            TestEqual(TEXT("Visible travel control keeps its real hit target"),F.Screen->ActionAt(At),FString(TEXT("travel")));
        }
        bool CompleteObjective=false;
        const auto ScaledSnapshot=F.Snapshot();
        for(const auto& V:ScaledSnapshot->GetArrayField(TEXT("elements")))if(V->AsObject()->GetStringField(TEXT("text"))==Expected)CompleteObjective=true;
        TestTrue(TEXT("Scaling retains the complete authored quest objective"),CompleteObjective);
        const auto Before=F.Element(TEXT(""),TEXT("map.terrain"));
        const FPointerEvent Wheel(0,FVector2D(1000,400),FVector2D(1000,400),TSet<FKey>(),FKey(),1,FModifierKeysState());
        F.Screen->NativeOnMouseWheel(FGeometry::MakeRoot(FVector2D(1672,941),FSlateLayoutTransform()),Wheel);
        const auto After=F.Element(TEXT(""),TEXT("map.terrain"));
        TestTrue(TEXT("Canvas wheel scales the current terrain atlas"),Before && After && After->GetNumberField(TEXT("width"))>Before->GetNumberField(TEXT("width")));
    }
    F.Screen->ExecuteAction(TEXT("map.local"));
    TestTrue(TEXT("Returning keeps the current map style"),F.Element(TEXT(""),TEXT("map.terrain")).IsValid());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapRectangularDrag078Test,
    "Hearthward.Map078.RectangularDragAndRelease",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMapRectangularDrag078Test::RunTest(const FString&)
{
    FMapFixture078 F;F.Screen->OpenPage(TEXT("map"));
    const auto Geometry=FGeometry::MakeRoot(FVector2D(1672,941),FSlateLayoutTransform());
    auto Event=[](FVector2D At,FVector2D Previous,bool Held)
    {return FPointerEvent(0,At,Previous,Held?TSet<FKey>{EKeys::LeftMouseButton}:TSet<FKey>(),EKeys::LeftMouseButton,0,FModifierKeysState());};
    auto Dragging=[&](){return F.Snapshot()->GetObjectField(TEXT("mapView"))->GetBoolField(TEXT("dragging"));};
    auto Start=[&](){return F.Screen->NativeOnMouseButtonDown(Geometry,Event({600,350},{600,350},true));};
    const auto Before=F.Element(TEXT(""),TEXT("map.terrain"));
    TestTrue(TEXT("Left button starts drag on the terrain"),Start().IsEventHandled() && Dragging());
    F.Screen->NativeOnMouseMove(Geometry,Event({660,400},{600,350},true));
    const auto After=F.Element(TEXT(""),TEXT("map.terrain"));
    TestTrue(TEXT("Terrain follows the horizontal cursor distance"),FMath::Abs(After->GetNumberField(TEXT("x"))-Before->GetNumberField(TEXT("x"))-60)<.02);
    TestTrue(TEXT("Terrain follows the vertical cursor distance"),FMath::Abs(After->GetNumberField(TEXT("y"))-Before->GetNumberField(TEXT("y"))-50)<.02);
    F.Screen->NativeOnMouseButtonUp(Geometry,Event({660,400},{660,400},false));
    TestFalse(TEXT("Release ends drag"),Dragging());
    F.Screen->NativeOnMouseMove(Geometry,Event({900,500},{660,400},false));
    TestEqual(TEXT("Unpressed motion cannot move the map"),F.Element(TEXT(""),TEXT("map.terrain"))->GetNumberField(TEXT("x")),After->GetNumberField(TEXT("x")));
    Start();F.Screen->NativeOnMouseCaptureLost(FCaptureLostEvent());
    TestFalse(TEXT("Losing pointer capture ends drag"),Dragging());
    Start();F.Screen->OpenPage(TEXT("hud"));F.Screen->OpenPage(TEXT("map"));
    TestFalse(TEXT("Changing pages clears stale drag state"),Dragging());
    F.Screen->NativeOnMouseButtonDown(Geometry,Event({1460,62},{1460,62},true));
    TestTrue(TEXT("Location toggle remains clickable"),F.Element(TEXT("travel")).IsValid());
    TestFalse(TEXT("Clicking a control does not start dragging"),Dragging());
    const FPointerEvent Wheel(0,FVector2D(900,400),FVector2D(900,400),TSet<FKey>(),FKey(),-100,FModifierKeysState());
    F.Screen->NativeOnMouseWheel(Geometry,Wheel);
    const auto Map=F.Snapshot()->GetObjectField(TEXT("mapView"));const auto& View=Map->GetArrayField(TEXT("viewport"));
    const double Scale=Map->GetNumberField(TEXT("scaleDesignUnitsPerCm"));
    TestTrue(TEXT("Minimum zoom covers both viewport dimensions without margins"),300000*Scale>=View[2]->AsNumber()-View[0]->AsNumber()-.02 && 200000*Scale>=View[3]->AsNumber()-View[1]->AsNumber()-.02);
    TestTrue(TEXT("Map uses the whole screen"),FMath::Abs(View[0]->AsNumber())<.02 && FMath::Abs(View[1]->AsNumber())<.02 && FMath::Abs(View[2]->AsNumber()-1672)<.02 && FMath::Abs(View[3]->AsNumber()-941)<.02);
    F.Screen->ExecuteAction(TEXT("map.local"));
    TestTrue(TEXT("Top right location entry is retained"),F.Element(TEXT("map.world"),TEXT("map.locations.toggle")).IsValid());
    const auto PlainMap=F.Snapshot();
    for(const auto& Value:PlainMap->GetArrayField(TEXT("elements")))
    {
        const auto E=Value->AsObject();const FString Text=E->GetStringField(TEXT("text"));
        TestFalse(TEXT("The surrounding frame and descriptions are removed"),E->GetStringField(TEXT("id"))==TEXT("map.frame") || Text.Contains(TEXT("3000米")) || Text.Contains(TEXT("Esc 返回")) || Text.Contains(TEXT("按住左键拖动")));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapLocationTravel078Test,
    "Hearthward.Map078.LocationKindsAndTravelFeedback",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMapLocationTravel078Test::RunTest(const FString&)
{
    FMapFixture078 F;auto* G=F.Gameplay;auto* Story=F.World->GetSubsystem<UHearthwardCampaignSubsystem>();
    for(const FName Id:{FName(TEXT("camp")),FName(TEXT("prologue_relic")),FName(TEXT("loot_dwellings")),FName(TEXT("camp_records")),
        FName(TEXT("camp_hunter")),FName(TEXT("slice_rescue")),FName(TEXT("loc_dwellings"))})G->Discovered.Add(Id);
    G->Explored.Reset();F.Player->SetActorLocation(G->LocationPosition(TEXT("loc_dwellings")));
    F.Screen->OpenPage(TEXT("map"));F.Screen->ExecuteAction(TEXT("map.world"));
    for(const FString Id:{TEXT("prologue_relic"),TEXT("loot_dwellings"),TEXT("camp_records"),TEXT("camp_hunter"),TEXT("slice_rescue")})
        TestFalse(TEXT("Task objects and people are excluded from the location list: ")+Id,F.Element(TEXT("location:")+Id).IsValid());
    TestTrue(TEXT("A discovered control flag is marked even without old exploration samples"),F.Element(TEXT(""),TEXT("map.location.marker.loc_dwellings")).IsValid());
    F.Screen->ExecuteAction(TEXT("location:loc_dwellings"));
    const auto Guide=F.Element(TEXT(""),TEXT("map.location.detail"));
    TestTrue(TEXT("Flag detail identifies a capture point and explains five seconds"),Guide && Guide->GetStringField(TEXT("text")).Contains(TEXT("占领")) && Guide->GetStringField(TEXT("text")).Contains(TEXT("5秒")));
    const auto Travel=F.Element(TEXT("travel"));
    TestTrue(TEXT("Capture flags never offer teleportation"),Travel && !Travel->GetBoolField(TEXT("enabled")));
    Story->State.Phase=TEXT("prologue");G->SetFeedback(TEXT("正在准备目的地，请稍候"));
    TestFalse(TEXT("The prologue continues to block travel"),G->Travel(TEXT("camp")));
    TestTrue(TEXT("The current reason replaces stale loading feedback"),G->Feedback.Contains(TEXT("序章")) && G->Feedback.Contains(TEXT("护符")));
    Story->State.Phase=TEXT("occupied");F.Screen->OpenPage(TEXT("hud"));
    F.Player->SetActorLocation(G->LocationPosition(TEXT("camp")));G->Activated.Add(TEXT("camp"));G->Activated.Remove(TEXT("route_ford"));
    TestFalse(TEXT("An inactive destination remains unavailable"),G->Travel(TEXT("route_ford")));
    TestTrue(TEXT("Inactive destination explains which station and how to activate"),G->Feedback.Contains(TEXT("渡口")) && G->Feedback.Contains(TEXT("激活")) && G->Feedback.Contains(TEXT("E")));
    G->Activated.Add(TEXT("route_ford"));F.Player->AddActorWorldOffset(FVector(1000,0,0));
    TestFalse(TEXT("Travel still requires a source station"),G->Travel(TEXT("route_ford")));
    TestTrue(TEXT("Source restriction states distance and a real source station"),G->Feedback.Contains(TEXT("营地")) && G->Feedback.Contains(TEXT("米")));
    F.Player->SetActorLocation(G->LocationPosition(TEXT("camp")));
    TestFalse(TEXT("Already being at the destination is explained"),G->Travel(TEXT("camp")));
    TestTrue(TEXT("Same-station feedback names the current place"),G->Feedback.Contains(TEXT("已在")) && G->Feedback.Contains(TEXT("营地")));
    TestFalse(TEXT("A task location cannot be used as a travel destination"),G->Travel(TEXT("prologue_relic")));
    TestTrue(TEXT("The task location rejection identifies its type"),G->Feedback.Contains(TEXT("不是传送")));
    return true;
}
#endif
