#include "../UI/HearthwardPreparationView.h"
#include "../UI/HearthwardScreenWidget.h"
#include "../UI/HearthwardQuestGuidance.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/SViewport.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparation090Test,"Hearthward.Iteration.Task090.AuthoritativeStagesAndReadOnly",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPreparation090Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();auto* World=Instance->GetWorld();
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);
    auto* G=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(G);G->RegisterComponent();G->Enabled=true;
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Initialize();
    auto* Camp=World->GetSubsystem<UHearthwardCampSubsystem>();Camp->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
    auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();
    G->TrackedQuest=TEXT("main_01");
    auto View=HearthwardPreparation::Read(G,Campaign,G->TrackedQuest);
    TestTrue(TEXT("new game offers relic action"),View.Visible && View.NextStep.Contains(TEXT("遗物包")));
    TestEqual(TEXT("journal and HUD use actual opening location"),View.Location,FName(TEXT("prologue_relic")));
    TestFalse(TEXT("later rescue remains hidden before prerequisites"),HearthwardPreparation::Read(G,Campaign,TEXT("main_03")).Visible);
    Campaign->State.Facts.Add(TEXT("relic"));
    View=HearthwardPreparation::Read(G,Campaign,G->TrackedQuest);
    TestEqual(TEXT("relic receipt changes authoritative destination"),View.Location,FName(TEXT("prologue_exit")));
    TestEqual(TEXT("brother confirmation offers existing dialogue"),View.Action,FString(TEXT("page:dialogue")));
    Campaign->State.Facts.Add(TEXT("prologue_order"));Campaign->State.Facts.Add(TEXT("prologue_complete"));
    View=HearthwardPreparation::Read(G,Campaign,G->TrackedQuest);
    TestTrue(TEXT("ready quest points to manual claim"),View.ReadyToClaim && View.NextStep.Contains(TEXT("领取")) && View.Location.IsNone());
    TestEqual(TEXT("ready action opens specific existing journal quest"),View.Action,FString(TEXT("preparation.journal:main_01")));
    G->Claimed.Add(TEXT("main_01"));G->Claimed.Add(TEXT("main_02"));Campaign->State.Phase=TEXT("occupied");
    TestFalse(TEXT("claimed opening hides its guidance"),HearthwardPreparation::Read(G,Campaign,TEXT("main_01")).Visible);
    View=HearthwardPreparation::Read(G,Campaign,TEXT("main_03"));
    TestTrue(TEXT("actual uncontacted rescue recommends preparation"),View.Visible && View.NextStep.Contains(TEXT("食物")) && View.NextStep.Contains(TEXT("药品")));
    auto* Person=Campaign->State.People.FindByPredicate([](const auto& P){return P.Id==TEXT("rescued_01");});
    if(TestNotNull(TEXT("original first-rescue identity exists"),Person))
    {
        Person->Stage=TEXT("following");View=HearthwardPreparation::Read(G,Campaign,TEXT("main_03"));
        TestEqual(TEXT("escort stage targets actual camp"),View.Location,FName(TEXT("camp")));
        Person->Stage=TEXT("waiting");View=HearthwardPreparation::Read(G,Campaign,TEXT("main_03"));
        TestTrue(TEXT("waiting escort explains threat and route"),View.NextStep.Contains(TEXT("威胁")) && View.NextStep.Contains(TEXT("路线")));
    }
    G->Claimed.Add(TEXT("main_03"));View=HearthwardPreparation::Read(G,Campaign,TEXT("main_04"));
    TestTrue(TEXT("upgrade guidance retains actual prerequisites"),View.NextStep.Contains(TEXT("工作台 I")) && View.NextStep.Contains(TEXT("获救者")));
    const FString CampaignBefore=Campaign->State.Snapshot(),CampBefore=Camp->State.Snapshot();
    const auto Epoch=Storage->GetTimelineEpoch();const int32 XP=G->Experience;const auto Claims=G->Claimed;const auto Rewards=G->RewardFacts;
    for(int32 I=0;I<20;++I)HearthwardPreparation::Read(G,Campaign,TEXT("main_04"));
    TestEqual(TEXT("repeated views cannot change campaign"),Campaign->State.Snapshot(),CampaignBefore);
    TestEqual(TEXT("repeated views cannot change camp"),Camp->State.Snapshot(),CampBefore);
    TestTrue(TEXT("views do not claim or grant experience"),G->Experience==XP && G->Claimed.Num()==Claims.Num() && G->RewardFacts.Num()==Rewards.Num());
    TestEqual(TEXT("views do not change timeline"),Storage->GetTimelineEpoch(),Epoch);
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);
    const TSharedRef<SViewport> SlateViewport=SNew(SViewport);TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SlateViewport);
    G->Claimed.Reset();Campaign->State.Initialize();
    auto* UI=CreateWidget<UHearthwardScreenWidget>(Controller);UI->SetIsFocusable(true);UI->TakeWidget();UI->AddToViewport();
    UI->OpenPage(TEXT("journal"));UI->ExecuteAction(TEXT("quest:main_01"));
    TestTrue(TEXT("real journal shows authoritative opening relic"),UI->DescribeLayout().Contains(TEXT("相关地点 · 家中的遗物包")));
    TestFalse(TEXT("real journal does not show old static flag destination"),UI->DescribeLayout().Contains(TEXT("相关地点 · 住区旗帜")));
    Campaign->State.Facts.Add(TEXT("relic"));Campaign->State.Facts.Add(TEXT("prologue_order"));Campaign->State.Facts.Add(TEXT("prologue_complete"));
    UI->OpenPage(TEXT("hud"));
    TestTrue(TEXT("real HUD offers manual journal claim"),UI->DescribeLayout().Contains(TEXT("打开日志领奖")));
    TestTrue(TEXT("normal guidance action opens the existing journal"),UI->ExecuteAction(TEXT("preparation.journal:main_01")));
    TestEqual(TEXT("guidance opens correct page"),UI->GetPage(),FName(TEXT("journal")));
    TestFalse(TEXT("opening claim page cannot auto-claim"),G->Claimed.Contains(TEXT("main_01")));
    Campaign->State.Initialize();Campaign->State.Phase=TEXT("occupied");
    G->Claimed.Add(TEXT("main_01"));G->Claimed.Add(TEXT("main_02"));G->TrackedQuest=TEXT("main_03");
    Person=Campaign->State.People.FindByPredicate([](const auto& P){return P.Id==TEXT("rescued_01");});
    Person->Stage=TEXT("waiting");Person->Located=true;Person->Position=FVector(-81000,-80000,100);
    G->Discovered.Add(TEXT("slice_rescue"));G->Explored.Add(FVector2D(Campaign->Position(TEXT("slice_rescue"))));
    G->Explored.Add(FVector2D(Person->Position));
    UI->OpenPage(TEXT("journal"));UI->ExecuteAction(TEXT("quest:main_03"));
    TestTrue(TEXT("waiting journal identifies the current person instead of the old rescue site"),UI->DescribeLayout().Contains(TEXT("相关目标 · 等待中的获救者")));
    TestTrue(TEXT("journal locate opens the actual known waiting target"),UI->ExecuteAction(TEXT("questMap")));
    const auto MapTarget=[&]() -> TSharedPtr<FJsonObject>
    {
        TSharedPtr<FJsonObject> Layout;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeLayout()),Layout);
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {const auto Row=Value->AsObject();if(Row->GetStringField(TEXT("id"))==TEXT("map.questTarget") && Row->GetBoolField(TEXT("visible")))return Row;}
        return nullptr;
    };
    const auto BeforeMove=MapTarget();
    if(TestTrue(TEXT("known waiting target has an actual visible map component"),BeforeMove.IsValid()))
    {
        const double BeforeX=BeforeMove->GetArrayField(TEXT("rect"))[0]->AsNumber();
        Person->Position.X+=1000;UI->Refresh();const auto AfterMove=MapTarget();
        if(TestTrue(TEXT("moving the known person retains its actual map component"),AfterMove.IsValid()))
            TestTrue(TEXT("map consumes the same current person position as the HUD"),AfterMove->GetArrayField(TEXT("rect"))[0]->AsNumber()>BeforeX);
    }
    G->TrackedQuest=TEXT("side_05");
    const auto SelectedGoal=HearthwardQuestGuidance::Resolve(Controller,TEXT("main_03"));
    TestTrue(TEXT("selected journal query reads the current person without changing tracking"),SelectedGoal.Visible && SelectedGoal.World.Equals(Person->Position,1));
    TestEqual(TEXT("journal query preserves the tracked quest"),G->TrackedQuest,FName(TEXT("side_05")));
    UI->RemoveFromParent();SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
    Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
#endif
