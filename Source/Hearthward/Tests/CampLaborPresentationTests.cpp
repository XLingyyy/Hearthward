#include "../UI/HearthwardScreenWidget.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Experience/HearthwardPresentationComponent.h"
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
#include "Slate/WidgetRenderer.h"
#include "Widgets/SViewport.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampWorkerPresentation059Test,
    "Hearthward.Camp059.WorkerAssignmentPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCampWorkerPresentation059Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();
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

    auto* Floor=World->SpawnActor<AActor>();Floor->Tags.Add(TEXT("Hearthward.NatureGround"));
    auto* FloorBox=NewObject<UBoxComponent>(Floor);
    Floor->AddInstanceComponent(FloorBox);Floor->SetRootComponent(FloorBox);
    FloorBox->SetBoxExtent(FVector(6000,6000,10));
    FloorBox->SetCollisionObjectType(ECC_WorldStatic);
    FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FloorBox->SetCollisionResponseToAllChannels(ECR_Block);FloorBox->RegisterComponent();
    Floor->SetActorLocation(FVector(0,0,-10));
    auto* Camp=World->GetSubsystem<UHearthwardCampSubsystem>();
    Camp->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
    const FVector WoodSource(1000,-1000,0),FoodSource(-1000,-1000,0);
    TestTrue(TEXT("Register an actual finite wood source"),Camp->RegisterSource(TEXT("labor-wood"),TEXT("wood"),12,12,WoodSource,2880));
    TestTrue(TEXT("Register an actual finite forage source"),Camp->RegisterSource(TEXT("labor-food"),TEXT("wild_food"),12,12,FoodSource,2880));
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();
    Campaign->State.Phase=TEXT("occupied");
    Campaign->State.Positions.Add(TEXT("camp"),FVector(0,0,80));
    Campaign->State.Positions.Add(TEXT("hometown"),FVector(4000,0,80));
    auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();
    const FGuid Epoch=Storage->GetTimelineEpoch();
    TestTrue(TEXT("Assign ordinary worker zero through the real camp command"),Camp->AssignWorker(TEXT("camp_wood"),0,Epoch));
    TestTrue(TEXT("Enable the real finite wood queue"),Camp->SetProduction(TEXT("camp_wood"),true,false,Epoch));
    Campaign->Tick(.5f);
    auto* Initial=Campaign->Actor(TEXT("civilian_initial_01"));
    if(!TestNotNull(TEXT("Worker zero uses the existing first ordinary-person identity"),Initial))
    {
        Cleanup();return false;
    }
    TestTrue(TEXT("An assigned ordinary worker appears beside its actual source"),FVector::Dist2D(Initial->GetActorLocation(),WoodSource)<=240);
    TestTrue(TEXT("Move the same person to the real forage queue"),Camp->AssignWorker(TEXT("camp_forage"),0,Epoch));
    TestTrue(TEXT("Enable the real finite forage queue"),Camp->SetProduction(TEXT("camp_forage"),true,false,Epoch));
    Campaign->Tick(.5f);
    TestEqual(TEXT("Changing jobs preserves the ordinary-person actor identity"),Campaign->Actor(TEXT("civilian_initial_01")),Initial);
    TestTrue(TEXT("Reassignment replaces the old workplace with the new actual source"),FVector::Dist2D(Initial->GetActorLocation(),FoodSource)<=240);
    TestFalse(TEXT("Reassignment removes the old authoritative worker slot"),Camp->State.Regions[1].Workers.Contains(0));

    FHearthwardCampaignPerson Person;Person.Id=TEXT("rescued_01");Person.Location=TEXT("slice_rescue");
    Person.Stage=TEXT("arrived");Person.Position=FVector(-2000,500,80);Person.Located=true;
    Campaign->State.People.Add(Person);
    TestTrue(TEXT("A returned person's actual identity is recorded once"),Camp->RecordRescue(Person.Id));
    TestFalse(TEXT("The same returned person cannot increase population twice"),Camp->RecordRescue(Person.Id));
    TestTrue(TEXT("Worker index twenty is assigned through the real camp command"),Camp->AssignWorker(TEXT("camp_wood"),20,Epoch));
    Campaign->Tick(.5f);
    auto* Rescued=Campaign->Actor(Camp->State.Rescued[0]);
    TestNotNull(TEXT("The rescued worker uses its original returned-person identity"),Rescued);
    if(Rescued)TestTrue(TEXT("A rescued worker also appears beside its assigned actual source"),FVector::Dist2D(Rescued->GetActorLocation(),WoodSource)<=240);
    TestNull(TEXT("A rescued worker does not acquire a duplicate initial-person identity"),Campaign->Actor(TEXT("civilian_initial_21")));
    TestEqual(TEXT("Protected hometown residents do not increase labor population"),Camp->State.Population(),21);
    for(int32 I=1;I<=4;++I)
        TestNotNull(TEXT("Protected hometown residents retain their separate identities"),Campaign->Actor(FName(*FString::Printf(TEXT("protected_%02d"),I))));

    TestTrue(TEXT("Release the rescued person's temporary wood assignment"),Camp->AssignWorker(TEXT("camp_wood"),20,Epoch));
    for(int32 I=1;I<=4;++I)TestTrue(TEXT("Assign four actual ordinary worker bodies"),Camp->AssignWorker(TEXT("camp_wood"),I,Epoch));
    TestTrue(TEXT("The player occupies the fifth body slot"),Camp->AssignWorker(TEXT("camp_wood"),30,Epoch));
    TestFalse(TEXT("A sixth body cannot replace an existing assignment"),Camp->AssignWorker(TEXT("camp_wood"),20,Epoch));
    Player->SetActorLocation(WoodSource+FVector(0,0,90));
    Camp->Advance(0,false);
    TestEqual(TEXT("The real stationary player provides the approved efficiency"),Camp->State.Regions[1].PlayerEfficiency,3.);
    auto* Screen=CreateWidget<UHearthwardScreenWidget>(Controller);
    if(!TestNotNull(TEXT("Real camp widget is created"),Screen))
    {
        Cleanup();return false;
    }
    Screen->SetIsFocusable(true);Screen->TakeWidget();Screen->AddToViewport();
    const auto Capture=[&](const TCHAR* Label)
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("Camp059Render"))
            || FParse::Param(FCommandLine::Get(),TEXT("NullRHI")) || !FApp::CanEverRender())return;
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("Task059/native-widget-offscreen");
        IFileManager::Get().MakeDirectory(*Directory,true);
        const FString Name=FString(Label)+TEXT(".png");
        FWidgetRenderer Renderer(true);
        auto* Target=UKismetRenderingLibrary::CreateRenderTarget2D(World,1696,954,RTF_RGBA8,FLinearColor::Black);
        if(!Target){AddError(TEXT("Native camp widget offscreen render target unavailable"));return;}
        Renderer.DrawWidget(Target,Screen->TakeWidget(),FVector2D(1696,954),0,false);
        UKismetRenderingLibrary::ExportRenderTarget(World,Target,Directory,Name);
        UKismetRenderingLibrary::ReleaseRenderTarget2D(Target);
        if(IFileManager::Get().FileSize(*(Directory/Name))<=0)AddError(TEXT("Native camp widget offscreen PNG export failed: ")+Name);
        FFileHelper::SaveStringToFile(Screen->DescribeLayout(),*(Directory/(FString(Label)+TEXT("-layout.json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        const FString Method=TEXT("{\"method\":\"native widget offscreen FWidgetRenderer DrawWidget to RGBA8 render target; explicit C++ fixture; no PIE or input acceptance\",\"width\":1696,\"height\":954}");
        FFileHelper::SaveStringToFile(Method,*(Directory/TEXT("method.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    };
    Screen->OpenPage(TEXT("camp"));
    TestEqual(TEXT("The normal safe camp entry opens the management screen"),Screen->GetPage(),FName(TEXT("camp")));
    TestTrue(TEXT("Open the real labor tab"),Screen->ExecuteAction(TEXT("camp.tab:workers")));
    TestTrue(TEXT("Select the actual wood region"),Screen->ExecuteAction(TEXT("camp.regionNext")));
    TestTrue(TEXT("The UI separates five occupied bodies from seven effective workers"),Screen->DescribeLayout().Contains(TEXT("身体岗位 5 / 5 · 有效工效 7.0")));
    Capture(TEXT("workers-normal-7-0"));
    Survival->State.SevereDue=1000;Camp->Advance(0,false);
    TestTrue(TEXT("Refresh the labor view after actual severe-hunger state changes"),Screen->ExecuteAction(TEXT("camp.tab:workers")));
    TestEqual(TEXT("The real severe-hunger efficiency is reduced"),Camp->State.Regions[1].PlayerEfficiency,2.1);
    TestTrue(TEXT("The UI displays reduced efficiency without releasing a body slot"),Screen->DescribeLayout().Contains(TEXT("身体岗位 5 / 5 · 有效工效 6.1")));
    Capture(TEXT("workers-severe-hunger-6-1"));
    Survival->State.SevereDue=-1;Player->SetActorLocation(WoodSource+FVector(500,0,90));Camp->Advance(0,false);
    TestTrue(TEXT("Refresh the labor view after the player leaves its workplace"),Screen->ExecuteAction(TEXT("camp.tab:workers")));
    TestEqual(TEXT("The real off-site player provides no labor"),Camp->State.Regions[1].PlayerEfficiency,0.);
    TestTrue(TEXT("The UI keeps the assigned body slot while showing its absent contribution"),Screen->DescribeLayout().Contains(TEXT("身体岗位 5 / 5 · 有效工效 4.0")));
    Capture(TEXT("workers-offsite-4-0"));

    auto* Threat=World->SpawnActor<AActor>();
    auto* ThreatTarget=NewObject<UHearthwardCombatTargetComponent>(Threat);
    Threat->AddInstanceComponent(ThreatTarget);ThreatTarget->Health=100;ThreatTarget->RegisterComponent();
    Camp->Advance(0,false);Campaign->Tick(.5f);
    const auto* Unsafe=Campaign->Actor(TEXT("civilian_initial_01"));
    TestTrue(TEXT("Unsafe ordinary workers are hidden or unloaded instead of presented as working"),!Unsafe || Unsafe->IsHidden());
    TestEqual(TEXT("Presentation refreshes do not advance the authoritative production clock"),Camp->State.Calendar,0.);
    TestEqual(TEXT("Presentation refreshes consume no finite source stock"),Camp->Source(TEXT("labor-wood"))->Remaining,12);

    Screen->RemoveFromParent();Cleanup();return true;
}
#endif
