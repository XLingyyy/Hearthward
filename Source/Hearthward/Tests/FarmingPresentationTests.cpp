#include "../UI/HearthwardScreenWidget.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Nature/HearthwardNatureActor.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "Components/StaticMeshComponent.h"
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
namespace
{
struct FFarmingWidgetFixture
{
    UGameInstance* Instance=nullptr;
    UWorld* World=nullptr;
    UHearthwardInventoryComponent* Bag=nullptr;
    UHearthwardNatureSubsystem* Nature=nullptr;
    UHearthwardScreenWidget* Screen=nullptr;
    TSharedPtr<FSceneViewport> SceneViewport;

    FFarmingWidgetFixture()
    {
        Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();World=Instance->GetWorld();
        auto* Viewport=NewObject<UGameViewportClient>(GEngine);
        Instance->GetWorldContext()->GameViewport=Viewport;
        Viewport->Init(*Instance->GetWorldContext(),Instance,false);
        const TSharedRef<SViewport> SlateViewport=SNew(SViewport);
        SceneViewport=Viewport->CreateViewport(SlateViewport);
        auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);
        Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
        auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);
        // Standalone fixtures have not begun play; register the real local controller explicitly.
        World->AddController(Controller);
        auto* Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);
        Player->SetActorLocation(FVector(0,0,90));
        Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
        auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
        auto* Combat=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
        auto* Timer=NewObject<UHearthwardTimedActionComponent>(Player);Player->AddInstanceComponent(Timer);Timer->RegisterComponent();
        auto* Building=NewObject<UHearthwardBuildingComponent>(Player);Player->AddInstanceComponent(Building);Building->RegisterComponent();
        auto* Presentation=NewObject<UHearthwardPresentationComponent>(Player);Player->AddInstanceComponent(Presentation);Presentation->RegisterComponent();
        Nature=World->GetSubsystem<UHearthwardNatureSubsystem>();Nature->State.Seed=481;
        Screen=CreateWidget<UHearthwardScreenWidget>(Controller);
        if(Screen){Screen->SetIsFocusable(true);Screen->TakeWidget();Screen->AddToViewport();}
    }
    ~FFarmingWidgetFixture()
    {
        if(Screen)Screen->RemoveFromParent();
        SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    FString Text() const
    {
        TSharedPtr<FJsonObject> Layout;
        if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),Layout) || !Layout)return {};
        FString Result;
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();FString Label;
            if(Row->GetBoolField(TEXT("visible")) && Row->TryGetStringField(TEXT("text"),Label))Result+=Label+TEXT("\n");
        }
        return Result;
    }
    void Capture(FAutomationTestBase& Test,const TCHAR* Label) const
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("Farming062Render"))
            || FParse::Param(FCommandLine::Get(),TEXT("NullRHI")) || !FApp::CanEverRender())return;
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("Task062/native-widget-offscreen");
        IFileManager::Get().MakeDirectory(*Directory,true);
        const FString Name=FString(Label)+TEXT(".png");
        FWidgetRenderer Renderer(true);
        auto* Target=UKismetRenderingLibrary::CreateRenderTarget2D(World,1696,954,RTF_RGBA8,FLinearColor::Black);
        if(!Target){Test.AddError(TEXT("Native widget offscreen render target unavailable"));return;}
        Renderer.DrawWidget(Target,Screen->TakeWidget(),FVector2D(1696,954),0,false);
        UKismetRenderingLibrary::ExportRenderTarget(World,Target,Directory,Name);
        UKismetRenderingLibrary::ReleaseRenderTarget2D(Target);
        if(IFileManager::Get().FileSize(*(Directory/Name))<=0)Test.AddError(TEXT("Native widget offscreen PNG export failed: ")+Name);
        FFileHelper::SaveStringToFile(Screen->DescribeLayout(),*(Directory/(FString(Label)+TEXT("-layout.json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        const FString Method=TEXT("{\"method\":\"native widget offscreen FWidgetRenderer DrawWidget to RGBA8 render target; explicit C++ fixture; no PIE or input acceptance\",\"width\":1696,\"height\":954}");
        FFileHelper::SaveStringToFile(Method,*(Directory/TEXT("method.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCropPresentation062Test,
    "Hearthward.Farming062.CropProgressAndHarvestCapacity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCropPresentation062Test::RunTest(const FString&)
{
    FFarmingWidgetFixture F;
    if(!TestNotNull(TEXT("Real farming widget is available"),F.Screen))return false;
    TestTrue(TEXT("Real farming widget is attached to its viewport"),F.Screen->IsInViewport());
    FHearthwardCrop Crop;Crop.Id=FGuid::NewGuid();Crop.Definition=TEXT("greens");Crop.Position=FVector(200,0,0);Crop.Planted=100;Crop.Watered=Crop.Fertilized=true;
    F.Nature->State.Calendar=1540;F.Nature->State.Crops.Add(Crop);
    F.Screen->OpenNature(Crop.Id);
    TestEqual(TEXT("Normal crop selection opens the farming page"),F.Screen->GetPage(),FName(TEXT("nature")));
    FString Text=F.Text();
    TestTrue(TEXT("Crop progress is shown from elapsed calendar time"),Text.Contains(TEXT("50%")));
    TestTrue(TEXT("Crop remaining time is expressed in calendar minutes"),Text.Contains(TEXT("1440 W分钟")));
    TestTrue(TEXT("The real once-cared yield is retained"),Text.Contains(TEXT("预计产量 6")));
    TestTrue(TEXT("Existing care completion remains visible"),Text.Contains(TEXT("浇水 完成")) && Text.Contains(TEXT("施肥 完成")));
    F.Capture(*this,TEXT("crop-half"));
    TestEqual(TEXT("Real bag can be filled to capacity"),F.Bag->TryAdd(TEXT("wood"),100),EHearthwardInventoryResult::Success);
    F.Nature->Advance(1440);F.Screen->Refresh();Text=F.Text();
    TestTrue(TEXT("Mature crop shows complete progress"),Text.Contains(TEXT("100%")));
    TestTrue(TEXT("Mature crop exposes its actual capacity failure before spending five seconds"),Text.Contains(TEXT("容量不足")));
    TestTrue(TEXT("Returned seed is included in the displayed harvest"),Text.Contains(TEXT("一粒种子")));
    F.Capture(*this,TEXT("crop-mature-full-bag"));
    F.Bag->TryRemove(TEXT("wood"),10);F.Screen->Refresh();Text=F.Text();
    TestFalse(TEXT("Making actual bag space clears the capacity failure"),Text.Contains(TEXT("容量不足")));
    TestTrue(TEXT("Mature output remains available rather than disappearing at capacity"),F.Nature->State.Crops.ContainsByPredicate([&](const auto& C){return C.Id==Crop.Id;}));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimalPresentation062Test,
    "Hearthward.Farming062.IndividualGrowthAndProductProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAnimalPresentation062Test::RunTest(const FString&)
{
    FFarmingWidgetFixture F;
    if(!TestNotNull(TEXT("Real animal widget is available"),F.Screen))return false;
    FHearthwardPen Pen;Pen.Id=FGuid::NewGuid();Pen.Definition=TEXT("hen");Pen.Position=FVector(200,0,0);F.Nature->State.Pens.Add(Pen);
    FHearthwardAnimal Hen;Hen.Id=FGuid::NewGuid();Hen.Definition=TEXT("hen");Hen.Pen=Pen.Id;Hen.Position=Pen.Position;Hen.Health=20;Hen.Domestic=Hen.Captured=Hen.Juvenile=true;Hen.Growth=720;Hen.FedRemaining=1440;
    F.Nature->State.Animals.Add(Hen);F.Screen->OpenNature(Hen.Id);
    FString Text=F.Text();
    TestTrue(TEXT("Selected juvenile is presented as a juvenile"),Text.Contains(TEXT("幼年")));
    TestTrue(TEXT("Individual juvenile growth is shown from its paid growth"),Text.Contains(TEXT("25%")));
    TestTrue(TEXT("Individual juvenile shows the remaining approved two-day calendar growth"),Text.Contains(TEXT("2160 W分钟")));
    F.Capture(*this,TEXT("juvenile-quarter"));
    F.Nature->State.Animals[0].Juvenile=false;F.Nature->State.Animals[0].Growth=2880;F.Nature->State.Animals[0].ProductMinutes=360;
    F.Screen->Refresh();Text=F.Text();
    TestTrue(TEXT("Adult product progress uses that animal's own authority"),Text.Contains(TEXT("25%")) && Text.Contains(TEXT("1080 W分钟")));
    TestFalse(TEXT("An adult no longer advertises juvenile growth remaining"),Text.Contains(TEXT("2160 W分钟")));
    F.Nature->State.Animals[0].FedRemaining=0;F.Screen->Refresh();Text=F.Text();
    TestTrue(TEXT("Unfed individual explains paused growth or production"),Text.Contains(TEXT("缺料")) && Text.Contains(TEXT("暂停")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPenPresentation062Test,
    "Hearthward.Farming062.PenProductiveAnimalAndPausePresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPenPresentation062Test::RunTest(const FString&)
{
    FFarmingWidgetFixture F;
    if(!TestNotNull(TEXT("Real pen widget is available"),F.Screen))return false;
    FHearthwardPen Pen;Pen.Id=FGuid::NewGuid();Pen.Definition=TEXT("goat");Pen.Position=FVector(200,0,0);Pen.Products=6;F.Nature->State.Pens.Add(Pen);
    FHearthwardAnimal Unfed;Unfed.Id=FGuid::NewGuid();Unfed.Definition=TEXT("goat");Unfed.Pen=Pen.Id;Unfed.Position=Pen.Position;Unfed.Health=60;Unfed.Domestic=Unfed.Captured=true;Unfed.ProductMinutes=1200;
    FHearthwardAnimal Fed=Unfed;Fed.Id=FGuid::NewGuid();Fed.ProductMinutes=240;Fed.FedRemaining=1440;
    F.Nature->State.Animals={Unfed,Fed};F.Screen->OpenNature(Pen.Id);
    FString Text=F.Text();
    TestTrue(TEXT("Pen forecast follows a fed productive adult, rather than an unfed animal with more progress"),Text.Contains(TEXT("1200 W分钟")));
    F.Capture(*this,TEXT("goat-fed-pen"));
    F.Nature->State.Animals[1].FedRemaining=0;F.Screen->Refresh();Text=F.Text();
    TestTrue(TEXT("Pen with no paid feeding explains its paused production"),Text.Contains(TEXT("缺料")) && Text.Contains(TEXT("暂停")));
    F.Nature->State.Animals[1].FedRemaining=1440;F.Nature->State.Pens[0].Products=HearthwardNature::ProductCapacity;
    F.Screen->Refresh();Text=F.Text();
    TestTrue(TEXT("Full real product storage is displayed"),Text.Contains(TEXT("24 / 24")));
    TestTrue(TEXT("Full product storage explains the cap and no deferred reward"),Text.Contains(TEXT("已满")) && Text.Contains(TEXT("不补发")));
    F.Nature->State.Pens[0].Definition=TEXT("pig");F.Nature->State.Pens[0].Products=0;F.Nature->State.Animals.Reset();
    F.Screen->Refresh();Text=F.Text();
    TestFalse(TEXT("A pig pen does not invent a generic product storage"),Text.Contains(TEXT("普通产物")));
    F.Capture(*this,TEXT("pig-pen"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCropGeometry062Test,
    "Hearthward.Farming062.CropGeometryConsumesCalendarStage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCropGeometry062Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();auto* World=Instance->GetWorld();
    const auto Cleanup=[&](){Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);};
    auto* Nature=World->GetSubsystem<UHearthwardNatureSubsystem>();Nature->State.Seed=481;Nature->State.Calendar=100;
    FHearthwardCrop Crop;Crop.Id=FGuid::NewGuid();Crop.Definition=TEXT("greens");Crop.Planted=100;Crop.Position=FVector(200,0,0);Nature->State.Crops.Add(Crop);
    Nature->RebuildActors();auto* Actor=Nature->Actor(Crop.Id);
    if(!TestNotNull(TEXT("Crop geometry belongs to the actual nature actor"),Actor)){Cleanup();return false;}
    const auto Geometry=[&]()
    {
        TArray<FTransform> Result;TArray<UStaticMeshComponent*> Components;Actor->GetComponents(Components);
        for(const auto* Component:Components)if(Component!=Actor->GetRootComponent())Result.Add(Component->GetRelativeTransform());
        return Result;
    };
    const auto Differs=[](const TArray<FTransform>& Before,const TArray<FTransform>& After)
    {
        if(Before.Num()!=After.Num())return true;
        for(int32 I=0;I<Before.Num();++I)if(!Before[I].Equals(After[I]))return true;
        return false;
    };
    const auto Seedling=Geometry();TestTrue(TEXT("The crop has actual renderable plant parts"),!Seedling.IsEmpty());
    Nature->Advance(1440);Actor->Refresh();const auto Growing=Geometry();
    TestTrue(TEXT("The same crop's render geometry changes after calendar growth"),Differs(Seedling,Growing));
    Actor->SetActorHiddenInGame(true);Nature->Advance(1440);Actor->Refresh();Actor->SetActorHiddenInGame(false);
    const auto Mature=Geometry();
    TestTrue(TEXT("Growth while hidden produces the mature geometry on the same stable plot"),Differs(Growing,Mature));
    TestEqual(TEXT("Visual refresh keeps the stable plot identity"),Actor->Id,Crop.Id);
    TestEqual(TEXT("Visual refresh does not replant the crop"),Nature->State.Crops[0].Planted,100.);
    Cleanup();return true;
}
#endif
