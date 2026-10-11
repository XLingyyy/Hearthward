#include "../UI/HearthwardScreenWidget.h"
#include "../Camp/HearthwardCampSubsystem.h"
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
#include "Components/BoxComponent.h"
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
struct FUIPerformanceFixture109
{
    UGameInstance* Instance=nullptr;UWorld* World=nullptr;APlayerController* Controller=nullptr;
    ACharacter* Player=nullptr;UHearthwardGameplayComponent* Gameplay=nullptr;
    UHearthwardScreenWidget* Screen=nullptr;TSharedPtr<FSceneViewport> SceneViewport;
    FUIPerformanceFixture109()
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
    ~FUIPerformanceFixture109()
    {
        Screen->RemoveFromParent();SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUI109ProfileTest,
    "Hearthward.Engineering.Task109.UIProfile",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUI109ProfileTest::RunTest(const FString&)
{
    const double Start=FPlatformTime::Seconds();
    FUIPerformanceFixture109 F;
    AddInfo(FString::Printf(TEXT("UI109 fixture_and_theme_ms=%.3f"),(FPlatformTime::Seconds()-Start)*1000));
    for(const FName Page:{FName(TEXT("title")),FName(TEXT("inventory")),FName(TEXT("journal")),FName(TEXT("map")),FName(TEXT("hud"))})
    {
        F.Screen->OpenPage(Page);
        TArray<double> Samples;
        for(int32 I=0;I<50;++I)
        {
            const double Tick=FPlatformTime::Seconds();F.Screen->Refresh();
            Samples.Add((FPlatformTime::Seconds()-Tick)*1000);
        }
        Samples.Sort();
        TestTrue(TEXT("Composed page remains inspectable"),F.Snapshot().IsValid());
        AddInfo(FString::Printf(TEXT("UI109 page=%s median_ms=%.3f p95_ms=%.3f"),*Page.ToString(),Samples[25],Samples[47]));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJournal109ObjectiveTest,
    "Hearthward.Engineering.Task109.JournalObjective",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJournal109ObjectiveTest::RunTest(const FString&)
{
    FUIPerformanceFixture109 F;
    auto* Campaign=F.World->GetSubsystem<UHearthwardCampaignSubsystem>();
    Campaign->State.Initialize();Campaign->State.Phase=TEXT("occupied");
    auto Label=[&](const TCHAR* Id)
    {
        const auto Snapshot=F.Snapshot();
        for(const auto& Value:Snapshot->GetArrayField(TEXT("elements")))
        {
            const auto E=Value->AsObject();
            if(E->GetStringField(TEXT("id"))==Id)return E->GetStringField(TEXT("text"));
        }
        return FString();
    };
    IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Task020")),true);
    for(int32 Scale:{100,150})
    {
        F.Instance->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale=Scale;
        F.Screen->OpenPage(TEXT("journal"));F.Screen->ExecuteAction(TEXT("category:side"));F.Screen->ExecuteAction(TEXT("quest:side_01"));
        const auto Side=HearthwardData::Find(TEXT("quests"),TEXT("side_01"));
        TestEqual(TEXT("rescue objective appears once"),Label(TEXT("journal.detail.objective")),HearthwardData::Text(Side,TEXT("objective")));
        TestTrue(TEXT("capture rescue objective at actual text scale"),F.Screen->CaptureUI(FString::Printf(TEXT("TASK109-journal-side-%d"),Scale),1920,1080));
        F.Screen->ExecuteAction(TEXT("category:main"));F.Screen->ExecuteAction(TEXT("quest:main_01"));
        TestEqual(TEXT("current stage has one actionable step"),Label(TEXT("journal.detail.objective")),FString(TEXT("靠近家中的遗物包，按交互键取回遗物。")));
        TestEqual(TEXT("full opening objectives remain in details"),Label(TEXT("journal.detail.description")),HearthwardData::Text(HearthwardData::Find(TEXT("quests"),TEXT("main_01")),TEXT("objective")));
        TestTrue(TEXT("capture opening objective at actual text scale"),F.Screen->CaptureUI(FString::Printf(TEXT("TASK109-journal-main-%d"),Scale),1920,1080));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemArt110Test,
    "Hearthward.UI.Task110.ItemArt",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemArt110Test::RunTest(const FString&)
{
    FUIPerformanceFixture109 F;
    F.Player->SetActorLocation(FVector(0,0,90));
    auto* Bag=F.Player->FindComponentByClass<UHearthwardInventoryComponent>();
    FHearthwardInventorySnapshot Empty;Empty.BackpackRank=3;
    TestTrue(TEXT("isolated QA inventory"),Bag->RestoreInventory(Empty));
    for(const auto& Value:HearthwardData::Rows(TEXT("items")))
        TestEqual(TEXT("QA item fits inventory"),Bag->TryAdd(FName(*HearthwardData::Text(Value->AsObject(),TEXT("id"))),1),EHearthwardInventoryResult::Success);
    const auto Asset=[&](const FString& Id)
    {
        const auto Snapshot=F.Snapshot();
        for(const auto& Value:Snapshot->GetArrayField(TEXT("elements")))
        {
            const auto Row=Value->AsObject();
            if(Row->GetStringField(TEXT("id"))==Id)return Row->GetStringField(TEXT("asset"));
        }
        return FString();
    };
    const auto ScrollTo=[&](const FString& Id)
    {
        const auto Snapshot=F.Snapshot();
        for(const auto& Value:Snapshot->GetArrayField(TEXT("elements")))
        {
            const auto Row=Value->AsObject();if(Row->GetStringField(TEXT("id"))!=Id)continue;
            const float Delta=(200-Row->GetNumberField(TEXT("y")))/90;
            F.Screen->NativeOnMouseWheel(F.Screen->GetCachedGeometry(),FPointerEvent(0,FVector2D(500,500),FVector2D(500,500),TSet<FKey>(),FKey(),Delta,FModifierKeysState()));
            return;
        }
        AddError(TEXT("Scroll target missing: ")+Id);
    };
    F.Screen->OpenPage(TEXT("inventory"));
    for(const auto& Value:HearthwardData::Rows(TEXT("items")))
    {
        const FString Id=HearthwardData::Text(Value->AsObject(),TEXT("id"));
        F.Screen->ExecuteAction(TEXT("item:")+Id);
        TestEqual(TEXT("inventory detail resolves this item's own art key"),Asset(TEXT("inventory.detail.art")),TEXT("item.")+Id);
    }
    auto& Camp=F.World->GetSubsystem<UHearthwardCampSubsystem>()->State;
    Camp.AddCamp(TEXT("camp"),FVector::ZeroVector);Camp.Tier=4;
    auto* Floor=F.World->SpawnActor<AActor>();
    auto* FloorBox=NewObject<UBoxComponent>(Floor);Floor->AddInstanceComponent(FloorBox);Floor->SetRootComponent(FloorBox);
    FloorBox->SetBoxExtent(FVector(1000,1000,10));FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FloorBox->SetCollisionResponseToAllChannels(ECR_Block);FloorBox->RegisterComponent();Floor->SetActorLocation(FVector(0,0,-10));
    TestTrue(TEXT("nearby real workbench"),F.Player->FindComponentByClass<UHearthwardBuildingComponent>()->AddGift(TEXT("workbench"),FVector(200,0,0)));
    FHearthwardCampFacility Facility;Facility.Id=FGuid::NewGuid();Facility.Kind=TEXT("forge");Facility.Level=3;Facility.Camp=TEXT("camp");Camp.Facilities.Add(Facility);
    Bag->WearInstance(Bag->FirstInstance(TEXT("axe")),20);
    for(int32 Scale:{100,150})
    {
        F.Instance->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale=Scale;
        for(const TCHAR* Id:{TEXT("herb"),TEXT("axe"),TEXT("fish_carp"),TEXT("seed_grain")})
        {
            F.Screen->OpenPage(TEXT("inventory"));F.Screen->ExecuteAction(FString(TEXT("item:"))+Id);
            TestTrue(TEXT("render inventory detail"),F.Screen->CaptureUI(FString::Printf(TEXT("TASK110-inventory-%s-%d"),Id,Scale),1920,1080));
        }
        F.Screen->OpenPage(TEXT("crafting"));
        TestEqual(TEXT("crafting entry succeeds"),F.Screen->GetPage(),FName(TEXT("crafting")));
        TestTrue(TEXT("select three-material recipe"),F.Screen->ExecuteAction(TEXT("recipe:firepot")));
        for(const TCHAR* Id:{TEXT("wood"),TEXT("stone"),TEXT("rope")})
            TestEqual(TEXT("recipe ingredient carries matching item image"),Asset(FString(TEXT("crafting.ingredients."))+Id+TEXT(".icon")),FString(TEXT("item."))+Id);
        TestEqual(TEXT("recipe output carries matching image"),Asset(TEXT("crafting.outputs.firepot.icon")),FString(TEXT("item.firepot")));
        if(Scale>100)ScrollTo(TEXT("crafting.ingredients.wood.icon"));
        TestTrue(TEXT("render crafting materials and output"),F.Screen->CaptureUI(FString::Printf(TEXT("TASK110-crafting-%d"),Scale),1920,1080));
        F.Screen->OpenPage(TEXT("equipment"));F.Screen->ExecuteAction(TEXT("gear.select:")+Bag->FirstInstance(TEXT("axe")).ToString());
        TestEqual(TEXT("equipment uses same axe art"),Asset(TEXT("equipment.detail.art")),FString(TEXT("item.axe")));
        TestTrue(TEXT("render repair material table"),F.Screen->CaptureUI(FString::Printf(TEXT("TASK110-equipment-%d"),Scale),1920,1080));
        F.Screen->OpenPage(TEXT("building"));
        TestTrue(TEXT("render construction material rows"),F.Screen->CaptureUI(FString::Printf(TEXT("TASK110-building-%d"),Scale),1920,1080));
        F.Screen->OpenPage(TEXT("camp"));F.Screen->ExecuteAction(TEXT("camp.tab:facilities"));F.Screen->ExecuteAction(TEXT("camp.facility:")+Facility.Id.ToString());
        TestEqual(TEXT("camp processing retains output icon at each text scale"),Asset(TEXT("camp.recipe.outputs.shortblade_2.icon")),FString(TEXT("item.shortblade_2")));
        if(Scale>100)ScrollTo(TEXT("camp.recipe.outputs.shortblade_2.icon"));
        TestTrue(TEXT("render camp processing"),F.Screen->CaptureUI(FString::Printf(TEXT("TASK110-camp-%d"),Scale),1920,1080));
    }
    return true;
}
#endif
