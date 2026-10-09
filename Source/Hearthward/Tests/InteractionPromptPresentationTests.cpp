#include "../UI/HearthwardScreenWidget.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Interaction/HearthwardInteractionComponent.h"
#include "../Interaction/HearthwardFurnitureInteractionComponent.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "Components/BoxComponent.h"
#include "../Experience/HearthwardPlayerSettings.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/SViewport.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionPrompt090Test,"Hearthward.Iteration.Task090.NearbyInteractionPrompt",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInteractionPrompt090Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();auto* World=Instance->GetWorld();
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);
    auto* G=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(G);G->RegisterComponent();G->Enabled=true;
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Presentation=NewObject<UHearthwardPresentationComponent>(Player);Player->AddInstanceComponent(Presentation);Presentation->RegisterComponent();
    auto* Interaction=NewObject<UHearthwardInteractionComponent>(Player);Player->AddInstanceComponent(Interaction);Interaction->RegisterComponent();
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();
    const FVector Relic(700000,700000,100);
    Campaign->State.Positions.Reset();Campaign->State.Positions.Add(TEXT("prologue_relic"),Relic);
    Player->SetActorLocation(Relic+FVector(100,0,0));
    TestTrue(TEXT("existing interaction resolver detects nearby relic"),Campaign->Prompt().Contains(TEXT("家中的遗物包")));
    const auto Before=Campaign->State.Snapshot();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);
    const TSharedRef<SViewport> SlateViewport=SNew(SViewport);TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SlateViewport);
    auto* UI=CreateWidget<UHearthwardScreenWidget>(Controller);UI->SetIsFocusable(true);UI->TakeWidget();UI->AddToViewport();UI->OpenPage(TEXT("hud"));
    const auto Prompt=[&]() -> FString
    {
        TSharedPtr<FJsonObject> Layout;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeLayout()),Layout);
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();
            if(Row->GetStringField(TEXT("id"))==TEXT("hud.interaction.prompt") && Row->GetBoolField(TEXT("visible")))return Row->GetStringField(TEXT("text"));
        }
        return {};
    };
    TestTrue(TEXT("actual HUD displays nearby relic interaction"),Prompt().Contains(TEXT("E 家中的遗物包")));
    auto* Settings=Instance->GetSubsystem<UHearthwardPlayerSettings>();
    Settings->Bindings.FindChecked(TEXT("interact"))[0].Key=EKeys::F;
    Settings->Comfort.TextScale=150;UI->Refresh();
    TestTrue(TEXT("prompt follows rebound key at enlarged text scale"),Prompt().Contains(TEXT("F 家中的遗物包")));
    if(FParse::Param(FCommandLine::Get(),TEXT("HearthwardPromptCapture")))TestTrue(TEXT("render actual enlarged relic HUD"),UI->CaptureUI(TEXT("task090-relic-150"),1280,720));
    for(int32 I=0;I<25;++I)UI->Refresh();
    TestTrue(TEXT("nearby prompt survives repeated HUD refresh"),Prompt().Contains(TEXT("家中的遗物包")));
    TestEqual(TEXT("view cannot award relic or mutate campaign"),Campaign->State.Snapshot(),Before);
    Player->SetActorLocation(Relic+FVector(400,0,0));Campaign->Feedback=TEXT("previous interaction result");UI->Refresh();
    TestTrue(TEXT("out-of-range target and stale completion are not proximity prompts"),Prompt().IsEmpty());
    Campaign->State.Phase=NAME_None;
    auto* Furniture=World->SpawnActor<AActor>();
    auto* Target=NewObject<UHearthwardFurnitureInteractionComponent>(Furniture);Furniture->AddInstanceComponent(Target);Furniture->SetRootComponent(Target);
    Target->Kind=TEXT("bed");Target->MaxDistance=150;Target->RegisterComponent();Furniture->SetActorLocation(Relic);
    Player->SetActorLocation(Relic+FVector(100,0,0));
    TestTrue(TEXT("generic interaction resolver detects actual furniture"),Interaction->GetNearestTarget()==Target);
    UI->Refresh();TestTrue(TEXT("generic target prompt and rebound key reach HUD"),Prompt().Contains(TEXT("F 就座休息")));
    if(FParse::Param(FCommandLine::Get(),TEXT("HearthwardPromptCapture")))TestTrue(TEXT("render actual multiline enlarged HUD"),UI->CaptureUI(TEXT("task090-bed-150"),1280,720));
    Player->SetActorLocation(Relic+FVector(200,0,0));UI->Refresh();
    TestTrue(TEXT("generic prompt uses target's actual distance"),Prompt().IsEmpty());
    Player->SetActorLocation(Relic+FVector(100,0,0));UI->OpenPage(TEXT("inventory"));
    TestTrue(TEXT("proximity prompt is absent from inventory page"),Prompt().IsEmpty());
    Furniture->Destroy();
    auto* StorageActor=World->SpawnActor<AActor>();
    auto* StorageTarget=NewObject<UHearthwardResourceInteractionComponent>(StorageActor);StorageActor->AddInstanceComponent(StorageTarget);StorageActor->SetRootComponent(StorageTarget);
    StorageTarget->RegisterComponent();StorageTarget->InitializePrototype(true);StorageActor->SetActorLocation(Relic+FVector(1000,0,0));
    Player->SetActorLocation(Relic+FVector(1100,0,0));
    TestTrue(TEXT("actual resource resolver selects storage"),Interaction->GetNearestTarget()==StorageTarget);
    Settings->Bindings.FindChecked(TEXT("storage.open"))[0].Key=EKeys::E;UI->OpenPage(TEXT("hud"));
    TestTrue(FString::Printf(TEXT("storage and interact remaps remain distinct: %s"),*Prompt()),Prompt().Contains(TEXT("E 管理仓储 · F 五秒存入木材")));
    StorageActor->Destroy();
    auto* Timer=NewObject<UHearthwardTimedActionComponent>(Player);Player->AddInstanceComponent(Timer);Timer->RegisterComponent();
    auto* Building=NewObject<UHearthwardBuildingComponent>(Player);Player->AddInstanceComponent(Building);Building->RegisterComponent();
    auto* Floor=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);
    Box->SetBoxExtent({1000,1000,10});Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Floor->SetActorLocation({0,0,-10});
    World->GetSubsystem<UHearthwardCampSubsystem>()->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
    Player->SetActorLocation({0,0,90});TestTrue(TEXT("real fixture workbench is built"),Building->AddGift(TEXT("workbench"),{200,0,0}));
    Campaign->State.Phase=TEXT("prologue");Campaign->State.Positions[TEXT("prologue_relic")]=Player->GetActorLocation();
    TestTrue(TEXT("actual workbench resolver selects facility"),Building->NearbyWorkbench().IsValid());
    UI->Refresh();TestTrue(TEXT("workbench prompt has the same priority as interaction input"),Prompt().Contains(TEXT("F 使用工作台")));
    Building->Selected=TEXT("workbench");UI->Refresh();TestTrue(TEXT("placement suppresses ordinary interaction prompt"),Prompt().IsEmpty());Building->Selected=NAME_None;
    UI->RemoveFromParent();SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
    Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
#endif
