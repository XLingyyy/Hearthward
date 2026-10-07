#include "../UI/HearthwardScreenWidget.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
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
namespace
{
struct FStorageUXWorld
{
    UGameInstance* Instance;
    UWorld* World;
    ACharacter* Player;
    UHearthwardInventoryComponent* Bag;
    UHearthwardStorageSubsystem* Storage;
    UHearthwardGameplayComponent* Gameplay;
    UHearthwardScreenWidget* Screen;
    TSharedPtr<FSceneViewport> SceneViewport;
    FStorageUXWorld()
    {
        Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();World=Instance->GetWorld();
        auto* Viewport=NewObject<UGameViewportClient>(GEngine);Instance->GetWorldContext()->GameViewport=Viewport;
        Viewport->Init(*Instance->GetWorldContext(),Instance,false);SceneViewport=Viewport->CreateViewport(SNew(SViewport));
        auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
        auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);World->AddController(Controller);
        Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);Player->SetActorLocation({0,0,90});
        Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
        Gameplay=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
        auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
        auto* Combat=NewObject<UHearthwardCombatComponent>(Player);Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
        auto* Building=NewObject<UHearthwardBuildingComponent>(Player);Player->AddInstanceComponent(Building);Building->RegisterComponent();
        auto* Warehouse=World->SpawnActor<AActor>();
        auto* Access=NewObject<UHearthwardResourceInteractionComponent>(Warehouse);Warehouse->AddInstanceComponent(Access);Warehouse->SetRootComponent(Access);
        Access->RegisterComponent();Access->InitializeResource(true);Warehouse->SetActorLocation(FVector::ZeroVector);
        Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();
        Screen=CreateWidget<UHearthwardScreenWidget>(Controller);Screen->SetIsFocusable(true);Screen->TakeWidget();Screen->AddToViewport();
    }
    ~FStorageUXWorld()
    {
        Screen->RemoveFromParent();SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    FString Field(const FString& Id,const FString& Name) const
    {
        TSharedPtr<FJsonObject> Layout;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),Layout);
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();FString Result;
            if(Row->GetStringField(TEXT("id"))==Id && Row->GetBoolField(TEXT("visible")) && Row->TryGetStringField(Name,Result))return Result;
        }
        return FString();
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorageUXInstances089Test,"Hearthward.Iteration.Task089.InstanceSelection",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStorageUXInstances089Test::RunTest(const FString&)
{
    FStorageUXWorld F;F.Bag->TryAdd(TEXT("axe"),2);const auto Gear=F.Bag->Snapshot().Instances;
    const FGuid First=Gear[0].Id,Second=Gear[1].Id;F.Bag->WearInstance(First,30);F.Bag->WearInstance(Second,10);F.Gameplay->InventoryChanged();
    F.Screen->OpenPage(TEXT("storage"));
    TestTrue(TEXT("Second same-kind instance is selected through the actual warehouse action"),F.Screen->ExecuteAction(TEXT("depositInstance:")+Second.ToString()));
    TestTrue(TEXT("Warehouse detail follows its exact wear"),F.Field(TEXT("storage.detail.properties"),TEXT("text")).Contains(TEXT("耐久 70.00 / 80")));
    const FString OldCard=F.Field(TEXT("storage.transfer"),TEXT("action"));
    TestTrue(TEXT("Bound warehouse action transfers the selected instance"),F.Screen->ExecuteAction(OldCard));
    TestTrue(TEXT("The other instance remains in the bag"),F.Bag->FindInstance(First)!=nullptr);
    const auto* InStorage=F.Storage->InventorySnapshot().Instances.FindByPredicate([&](const auto& I){return I.Id==Second;});
    if(TestNotNull(TEXT("Only the selected GUID reaches the warehouse"),InStorage))TestEqual(TEXT("Actual transfer preserves wear"),InStorage->Durability,70.);
    TestFalse(TEXT("Repeated bound card cannot move the other same-kind item"),F.Screen->ExecuteAction(OldCard));
    TestEqual(TEXT("Only one copy was deposited"),F.Storage->GetItemCount(TEXT("axe")),1);
    TestTrue(TEXT("Explicitly select the warehouse instance"),F.Screen->ExecuteAction(TEXT("withdrawInstance:")+Second.ToString()));
    TestTrue(TEXT("Withdraw selected instance"),F.Screen->ExecuteAction(TEXT("transfer")));
    if(const auto* Returned=F.Bag->FindInstance(Second);TestNotNull(TEXT("Returned instance still owns the same GUID"),Returned))
        TestEqual(TEXT("Return retains the same GUID and wear"),Returned->Durability,70.);
    F.Screen->OpenPage(TEXT("equipment"));F.Screen->ExecuteAction(TEXT("gear.select:")+Second.ToString());
    F.Screen->OpenPage(TEXT("inventory"));F.Screen->ExecuteAction(TEXT("item:axe"));
    TestTrue(TEXT("Inventory details retain the deliberate instance instead of the first kind"),F.Field(TEXT("inventory.detail.description"),TEXT("text")).Contains(Second.ToString(EGuidFormats::Digits).Left(8)));
    TestTrue(TEXT("Inventory wear matches the same selected spare"),F.Screen->DescribeLayout().Contains(TEXT("70.00 / 80")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorageUXReplay089Test,"Hearthward.Iteration.Task089.PreviewAndReplay",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStorageUXReplay089Test::RunTest(const FString&)
{
    FStorageUXWorld F;F.Bag->TryAdd(TEXT("wood"),4);F.Screen->OpenPage(TEXT("storage"));F.Screen->ExecuteAction(TEXT("deposit:wood"));
    F.Screen->ExecuteAction(TEXT("quantity:-999"));F.Screen->ExecuteAction(TEXT("quantity:1"));
    const FString Card=F.Field(TEXT("storage.transfer"),TEXT("action"));
    TestTrue(TEXT("Explicit two-item preview succeeds"),F.Screen->ExecuteAction(Card));
    TestEqual(TEXT("Actual MovedCount appears in feedback"),F.Screen->GetMessage().Contains(TEXT("×2")),true);
    TestFalse(TEXT("Fast repeated click cannot create a fresh transaction"),F.Screen->ExecuteAction(Card));
    TestEqual(TEXT("A preview settles only once"),F.Storage->GetItemCount(TEXT("wood")),2);
    TestTrue(TEXT("An explicit new operation remains available"),F.Screen->ExecuteAction(TEXT("storage.newTransfer")));
    TestTrue(TEXT("New operation transfers the remaining two"),F.Screen->ExecuteAction(TEXT("transfer")));
    TestEqual(TEXT("Separate legal operation settles"),F.Storage->GetItemCount(TEXT("wood")),4);
    F.Bag->TryAdd(TEXT("wood"),3);F.Screen->ExecuteAction(TEXT("deposit:wood"));F.Screen->ExecuteAction(TEXT("quantity:2"));
    F.Bag->TryRemove(TEXT("wood"),2);
    TestFalse(TEXT("Source changed after preview is rechecked"),F.Screen->ExecuteAction(TEXT("transfer")));
    TestEqual(TEXT("Failed preview does not lose the remaining source"),F.Bag->GetItemCount(TEXT("wood")),1);
    TestEqual(TEXT("Failed preview does not add warehouse stock"),F.Storage->GetItemCount(TEXT("wood")),4);
    F.Screen->ExecuteAction(TEXT("quantity:-1"));TestTrue(TEXT("Adjusted bounded quantity makes a fresh valid preview"),F.Screen->ExecuteAction(TEXT("transfer")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorageUXAccess089Test,"Hearthward.Iteration.Task089.AccessAndTimeline",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStorageUXAccess089Test::RunTest(const FString&)
{
    FStorageUXWorld F;F.Bag->TryAdd(TEXT("wood"),3);F.Screen->OpenPage(TEXT("storage"));F.Screen->ExecuteAction(TEXT("deposit:wood"));
    const FString Card=F.Field(TEXT("storage.transfer"),TEXT("action"));F.Player->SetActorLocation({1000,0,90});
    TestFalse(TEXT("Moving out of the original facility reach rejects transfer"),F.Screen->ExecuteAction(Card));
    F.Player->SetActorLocation({0,0,90});F.Storage->AdvanceTimeline();
    TestFalse(TEXT("Old card rejects a replaced timeline"),F.Screen->ExecuteAction(Card));
    TestEqual(TEXT("Old operation preserves source stock"),F.Bag->GetItemCount(TEXT("wood")),3);
    F.Screen->OpenPage(TEXT("storage"));F.Screen->ExecuteAction(TEXT("deposit:wood"));F.Gameplay->NotifyCombat();
    TestFalse(TEXT("Original gameplay combat restriction is rechecked"),F.Screen->ExecuteAction(TEXT("transfer")));
    TestEqual(TEXT("Rejected operations do not create stock"),F.Storage->GetItemCount(TEXT("wood")),0);
    return true;
}
#endif
