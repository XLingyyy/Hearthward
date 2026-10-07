#include "../UI/HearthwardScreenWidget.h"
#include "../UI/HearthwardCraftingTracker.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Experience/HearthwardPlayerSettings.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/SViewport.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCraftingDiscovery088Test,"Hearthward.Iteration.Task088.DiscoveryAndSessionTarget",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCraftingDiscovery088Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();auto* World=Instance->GetWorld();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);
    const TSharedRef<SViewport> SlateViewport=SNew(SViewport);TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SlateViewport);
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);Player->SetActorLocation({0,0,90});
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* G=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(G);G->RegisterComponent();G->Enabled=true;
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Combat=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
    auto* Timer=NewObject<UHearthwardTimedActionComponent>(Player);Player->AddInstanceComponent(Timer);Timer->RegisterComponent();
    auto* Building=NewObject<UHearthwardBuildingComponent>(Player);Player->AddInstanceComponent(Building);Building->RegisterComponent();
    auto* Floor=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);
    Box->SetBoxExtent({1000,1000,10});Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Floor->SetActorLocation({0,0,-10});
    auto* Camp=World->GetSubsystem<UHearthwardCampSubsystem>();Camp->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
    TestTrue(TEXT("real registered workbench"),Building->AddGift(TEXT("workbench"),{200,0,0}));
    auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();Storage->Adjust({},{{TEXT("wood"),4},{TEXT("ore"),4}});
    auto* UI=CreateWidget<UHearthwardScreenWidget>(Controller);UI->SetIsFocusable(true);UI->TakeWidget();UI->AddToViewport();UI->OpenPage(TEXT("crafting"));
    const auto HasAction=[&](const FString& Action)
    {
        TSharedPtr<FJsonObject> Layout;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeLayout()),Layout);
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {FString Bound;const auto Row=Value->AsObject();if(Row->TryGetStringField(TEXT("action"),Bound) && Bound==Action && Row->GetBoolField(TEXT("visible")))return true;}
        return false;
    };
    TestEqual(TEXT("ordinary nearby facility entry"),UI->GetPage(),FName(TEXT("crafting")));
    TestTrue(TEXT("select stable recipe"),UI->ExecuteAction(TEXT("recipe:rope")));
    UI->ExecuteAction(TEXT("craftSearch:绳索"));
    TestTrue(TEXT("Chinese search preserves matching recipe"),HasAction(TEXT("recipe:rope")));
    TestFalse(TEXT("Chinese search removes unrelated recipes"),HasAction(TEXT("recipe:metal_ingot")));
    UI->ExecuteAction(TEXT("craftSearch:不存在的配方"));
    TestTrue(TEXT("empty result has clear-filter control"),UI->DescribeLayout().Contains(TEXT("crafting.clearFilters")));
    TestTrue(TEXT("selection detail survives filtering"),UI->DescribeLayout().Contains(TEXT("crafting.name")));
    UI->ExecuteAction(TEXT("craftClearFilters"));UI->ExecuteAction(TEXT("craftAvailable"));
    TestTrue(TEXT("available rope is listed"),HasAction(TEXT("recipe:rope")));
    TestFalse(TEXT("enough ore without a smelter remains unavailable"),HasAction(TEXT("recipe:metal_ingot")));
    UI->ExecuteAction(TEXT("craftMore"));UI->ExecuteAction(TEXT("craftTrack"));
    TestEqual(TEXT("tracking reserves no bag materials"),Bag->Available(TEXT("wood")),0);
    TestEqual(TEXT("tracking reserves no warehouse materials"),Storage->Available(TEXT("wood")),4);
    TestTrue(TEXT("two-batch target reads real available materials"),UI->GetCraftingTrackerText().Contains(TEXT("4/4（缺0）")));
    UI->OpenPage(TEXT("hud"));TestTrue(TEXT("page close keeps target"),UI->GetCraftingTrackerText().Contains(TEXT("绳索")));
    Player->SetActorLocation({100000,0,90});
    TestTrue(TEXT("off-camp target excludes warehouse"),UI->GetCraftingTrackerText().Contains(TEXT("0/4（缺4）")));
    Player->SetActorLocation({0,0,90});UI->OpenPage(TEXT("crafting"));UI->ExecuteAction(TEXT("recipe:rope"));
    UI->ExecuteAction(TEXT("craftMore"));
    TestTrue(TEXT("real transaction executes once"),UI->ExecuteAction(TEXT("craft")));
    TestFalse(TEXT("second click with exhausted stock cannot settle again"),UI->ExecuteAction(TEXT("craft")));
    TestEqual(TEXT("actual warehouse debit"),Storage->GetItemCount(TEXT("wood")),0);
    TestEqual(TEXT("actual product quantity"),Bag->GetItemCount(TEXT("rope")),2);
    Storage->AdvanceTimeline();TestTrue(TEXT("timeline change clears session target"),UI->GetCraftingTrackerText().IsEmpty());
    UI->RemoveFromParent();SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
    Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCraftingMaterialAvailability088Test,"Hearthward.Iteration.Task088.ReservedAndOffCampMaterials",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCraftingMaterialAvailability088Test::RunTest(const FString&)
{
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Actor=World->SpawnActor<AActor>();auto* Bag=NewObject<UHearthwardInventoryComponent>(Actor);Actor->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();Bag->TryAdd(TEXT("wood"),3);Bag->ReserveMaterials({{TEXT("wood"),2}});
    Storage->Adjust({},{{TEXT("wood"),5}});const auto Reservation=FGuid::NewGuid();Storage->Reserve(Reservation,{{TEXT("wood"),2}});
    const auto Recipe=HearthwardData::Find(TEXT("craftingRecipes"),TEXT("rope"));
    const auto InCamp=HearthwardCraftingDiscovery::Materials(Recipe,2,Bag,Storage,true);
    TestEqual(TEXT("bag reservations excluded"),InCamp[0].InBag,1);TestEqual(TEXT("warehouse reservations excluded"),InCamp[0].InStorage,3);
    TestEqual(TEXT("sources counted once"),InCamp[0].Available,4);TestEqual(TEXT("ready deficit clamps at zero"),InCamp[0].Missing,0);
    const auto Outside=HearthwardCraftingDiscovery::Materials(Recipe,2,Bag,Storage,false);
    TestEqual(TEXT("reference warehouse remains observable"),Outside[0].InStorage,3);TestEqual(TEXT("outside source is only bag"),Outside[0].Available,1);
    TestEqual(TEXT("outside deficit uses bag only"),Outside[0].Missing,3);TestEqual(TEXT("projection consumes nothing"),Storage->GetItemCount(TEXT("wood")),5);
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}

namespace
{
bool DownedHUDMaterialCase(FAutomationTestBase& Test,bool BrotherDowned)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();auto* World=Instance->GetWorld();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SNew(SViewport));
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);Player->SetActorLocation({0,0,90});
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* G=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(G);G->RegisterComponent();G->Enabled=true;
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Combat=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
    auto* Timer=NewObject<UHearthwardTimedActionComponent>(Player);Player->AddInstanceComponent(Timer);Timer->RegisterComponent();
    auto* Building=NewObject<UHearthwardBuildingComponent>(Player);Player->AddInstanceComponent(Building);Building->RegisterComponent();
    auto* Floor=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);
    Box->SetBoxExtent({1000,1000,10});Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Floor->SetActorLocation({0,0,-10});
    auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>();Brother->SetActorLocation({150,150,80});
    auto* Camp=World->GetSubsystem<UHearthwardCampSubsystem>();Camp->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
    Test.TestTrue(TEXT("Material target uses an actual registered workbench"),Building->AddGift(TEXT("workbench"),{200,0,0}));
    Instance->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale=150;
    auto* UI=CreateWidget<UHearthwardScreenWidget>(Controller);UI->SetIsFocusable(true);UI->TakeWidget();UI->AddToViewport();
    const auto HasElement=[&](const FString& Id,const FString& Action=FString())
    {
        TSharedPtr<FJsonObject> Layout;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeLayout()),Layout);
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();FString Bound;
            if(!Row->GetBoolField(TEXT("visible")))continue;
            if(!Id.IsEmpty() && Row->GetStringField(TEXT("id"))!=Id)continue;
            if(Action.IsEmpty() || (Row->TryGetStringField(TEXT("action"),Bound) && Bound==Action))return true;
        }
        return false;
    };
    UI->OpenPage(TEXT("crafting"));
    Test.TestTrue(TEXT("Select a real recipe for the session target"),UI->ExecuteAction(TEXT("recipe:rope")));
    Test.TestTrue(TEXT("Tracking is enabled before the emergency"),UI->ExecuteAction(TEXT("craftTrack")));
    UI->OpenPage(TEXT("hud"));
    Test.TestTrue(TEXT("Alive HUD exposes the ordinary material details action at 150 percent"),HasElement(TEXT("hud.crafting.open"),TEXT("page:crafting")));
    const FGuid Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    auto* Target=BrotherDowned?Brother->FindComponentByClass<UHearthwardSurvivalComponent>():Survival;
    Test.TestTrue(TEXT("Real lethal damage supplies one rescuable downed character"),
        Target->ReceiveDamage(Target->MaxHealth(),FGuid::NewGuid(),Epoch) && Target->State.Life==EHearthwardLife::Downed);
    UI->Refresh();
    Test.TestEqual(TEXT("A single downed character retains the playable HUD"),UI->GetPage(),FName(TEXT("hud")));
    Test.TestTrue(TEXT("Actual emergency text remains present"),HasElement(BrotherDowned?TEXT("hud.companion.downed"):TEXT("hud.survival.downed")));
    Test.TestFalse(TEXT("Emergency HUD hides ordinary material details rather than overlapping rescue feedback"),HasElement(TEXT("hud.crafting.open"),TEXT("page:crafting")));
    Test.TestFalse(TEXT("Emergency HUD hides the ordinary material target row"),HasElement(TEXT("hud.crafting.target")));
    Test.TestFalse(TEXT("Emergency HUD has no alternate visible crafting entry action"),HasElement(TEXT(""),TEXT("page:crafting")));
    Test.TestFalse(TEXT("Hiding the ordinary row preserves the session material target"),UI->GetCraftingTrackerText().IsEmpty());
    UI->RemoveFromParent();SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
    Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerDownedHUD090Test,"Hearthward.Iteration.Task090.PlayerDownedSuppressesMaterials",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlayerDownedHUD090Test::RunTest(const FString&)
{return DownedHUDMaterialCase(*this,false);}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBrotherDownedHUD090Test,"Hearthward.Iteration.Task090.BrotherDownedSuppressesMaterials",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBrotherDownedHUD090Test::RunTest(const FString&)
{return DownedHUDMaterialCase(*this,true);}
#endif
