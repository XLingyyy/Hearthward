#include "../UI/HearthwardScreenWidget.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Building/HearthwardWorkshopService.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Interaction/HearthwardHarvestSubsystem.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Nature/HearthwardNatureActor.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCraftingWarehousePresentation057Test,
    "Hearthward.Crafting057.WarehouseMaterialsPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCraftingWarehousePresentation057Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);
    Instance->InitializeStandalone();
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
    auto* Controller=World->SpawnActor<APlayerController>();
    Controller->SetPlayer(LocalPlayer);
    // This standalone world has not initialized actors for play, so register the controller explicitly.
    World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();
    Controller->Possess(Player);
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

    auto* Floor=World->SpawnActor<AActor>();
    auto* FloorBox=NewObject<UBoxComponent>(Floor);
    Floor->AddInstanceComponent(FloorBox);Floor->SetRootComponent(FloorBox);
    FloorBox->SetBoxExtent(FVector(1000,1000,10));
    FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FloorBox->SetCollisionResponseToAllChannels(ECR_Block);FloorBox->RegisterComponent();
    Floor->SetActorLocation(FVector(0,0,-10));
    World->GetSubsystem<UHearthwardCampSubsystem>()->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
    TestTrue(TEXT("Real nearby workbench is built"),Building->AddGift(TEXT("workbench"),FVector(200,0,0)));

    auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();
    TestTrue(TEXT("All first-batch materials are held in camp storage"),Storage->Adjust({},{{TEXT("wood"),4}}));
    auto* Screen=CreateWidget<UHearthwardScreenWidget>(Controller);
    if(!TestNotNull(TEXT("Real crafting widget is created"),Screen))
    {
        Cleanup();return false;
    }
    Screen->SetIsFocusable(true);
    if(!TestEqual(TEXT("Widget resolves its local owning controller"),Screen->GetOwningPlayer(),Controller))
    {
        Cleanup();return false;
    }
    Screen->TakeWidget();Screen->AddToViewport();
    if(!TestTrue(TEXT("Crafting widget is attached to its standalone viewport"),Screen->IsInViewport()))
    {
        Screen->RemoveFromParent();Cleanup();return false;
    }
    const auto LayoutText=[&](const TCHAR* Id)
    {
        TSharedPtr<FJsonObject> Layout;
        if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),Layout))return FString();
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();
            if(Row->GetStringField(TEXT("id"))==Id)
            {
                FString Text;Row->TryGetStringField(TEXT("text"),Text);return Text;
            }
        }
        return FString();
    };

    Screen->OpenPage(TEXT("crafting"));
    if(!TestEqual(TEXT("Normal nearby-workbench entry opens crafting"),Screen->GetPage(),FName(TEXT("crafting"))))
    {
        Screen->RemoveFromParent();Cleanup();return false;
    }
    TestTrue(TEXT("Select the known rope recipe through the UI"),Screen->ExecuteAction(TEXT("recipe:rope")));
    TestTrue(TEXT("Select two batches through the UI"),Screen->ExecuteAction(TEXT("craftMore")));
    TestTrue(TEXT("Warehouse supply is visible instead of a zero bag-only count"),
        LayoutText(TEXT("crafting.ingredients")).Contains(TEXT("木材   背包可用 0 + 仓储可用 4 / 消耗 4")));
    TestTrue(TEXT("Ready status states that camp storage can supply materials"),
        LayoutText(TEXT("crafting.status")).Contains(TEXT("仓储")));
    TestFalse(TEXT("Ready status does not claim that materials come only from the bag"),
        LayoutText(TEXT("crafting.status")).Contains(TEXT("仅使用背包")));
    const double WarehouseBefore=Bag->GetWeight();
    const FString WarehousePrediction=LayoutText(TEXT("crafting.weight"));
    TestTrue(TEXT("Crafting uses the real Workshop transaction through the UI"),Screen->ExecuteAction(TEXT("craft")));
    TestEqual(TEXT("Two rope products enter the bag"),Bag->GetItemCount(TEXT("rope")),2);
    TestEqual(TEXT("The warehouse pays for both batches once"),Storage->GetItemCount(TEXT("wood")),0);
    TestEqual(TEXT("Warehouse-only prediction matches the actual bag after Workshop"),WarehousePrediction,
        FString::Printf(TEXT("负重 %.2f → %.2f / %.0f"),WarehouseBefore,Bag->GetWeight(),Bag->GetCapacity()));

    // Reservations belong to another operation and must be excluded from both displayed sources.
    TestEqual(TEXT("Mixed-supply bag materials are available"),Bag->TryAdd(TEXT("wood"),3),EHearthwardInventoryResult::Success);
    TestTrue(TEXT("Reserve two bag materials for another operation"),Bag->ReserveMaterials({{TEXT("wood"),2}}));
    TestTrue(TEXT("Mixed-supply warehouse materials are available"),Storage->Adjust({},{{TEXT("wood"),5}}));
    const FGuid Reservation=FGuid::NewGuid();
    TestTrue(TEXT("Reserve two warehouse materials for another operation"),Storage->Reserve(Reservation,{{TEXT("wood"),2}}));
    TestTrue(TEXT("Refresh the real recipe after reservation changes"),Screen->ExecuteAction(TEXT("recipe:rope")));
    TestTrue(TEXT("Keep the mixed-supply recipe at two batches"),Screen->ExecuteAction(TEXT("craftMore")));
    TestTrue(TEXT("Displayed source availability excludes bag and warehouse reservations"),
        LayoutText(TEXT("crafting.ingredients")).Contains(TEXT("木材   背包可用 1 + 仓储可用 3 / 消耗 4")));
    const double MixedBefore=Bag->GetWeight();
    const FString MixedPrediction=LayoutText(TEXT("crafting.weight"));
    TestTrue(TEXT("Mixed available materials settle through the same real UI transaction"),Screen->ExecuteAction(TEXT("craft")));
    TestEqual(TEXT("Workshop consumes only the one available bag material"),Bag->GetItemCount(TEXT("wood")),2);
    TestEqual(TEXT("Workshop consumes only the three available warehouse materials"),Storage->GetItemCount(TEXT("wood")),2);
    TestEqual(TEXT("Other-operation bag materials remain reserved"),Bag->Available(TEXT("wood")),0);
    TestEqual(TEXT("Other-operation warehouse materials remain reserved"),Storage->Available(TEXT("wood")),0);
    TestEqual(TEXT("Mixed-supply prediction matches the actual bag after Workshop"),MixedPrediction,
        FString::Printf(TEXT("负重 %.2f → %.2f / %.0f"),MixedBefore,Bag->GetWeight(),Bag->GetCapacity()));

    TestTrue(TEXT("Choose one batch while all remaining materials are reserved"),Screen->ExecuteAction(TEXT("recipe:rope")));
    TestTrue(TEXT("Unavailable reserved stock is displayed as zero usable materials"),
        LayoutText(TEXT("crafting.ingredients")).Contains(TEXT("木材   背包可用 0 + 仓储可用 0 / 消耗 2")));
    TestTrue(TEXT("Insufficient available materials have an explicit failure reason"),
        LayoutText(TEXT("crafting.status")).Contains(TEXT("材料不足")));
    TestFalse(TEXT("A blocked recipe does not display a successful post-craft prediction"),
        LayoutText(TEXT("crafting.weight")).Contains(TEXT("→")));
    TestFalse(TEXT("The real Workshop refuses another operation's reserved materials"),Screen->ExecuteAction(TEXT("craft")));
    TestEqual(TEXT("Rejected crafting creates no extra products"),Bag->GetItemCount(TEXT("rope")),4);
    TestEqual(TEXT("Rejected crafting retains the bag reservation stock"),Bag->GetItemCount(TEXT("wood")),2);
    TestEqual(TEXT("Rejected crafting retains the warehouse reservation stock"),Storage->GetItemCount(TEXT("wood")),2);

    Bag->ReleaseMaterials();Storage->Release(Reservation);

    // Start inside both production acceptance radii; every material transfer and craft is still executed by Tick.
    const auto& Facilities=World->GetSubsystem<UHearthwardCampSubsystem>()->State.Facilities;
    const auto* Workbench=Facilities.FindByPredicate([](const auto& F){return F.Kind==TEXT("workbench");});
    auto* Station=Workbench?Building->ResolveFacility(Workbench->Id):nullptr;
    if(!TestNotNull(TEXT("Source regression has its real registered workbench"),Station))
    {Screen->RemoveFromParent();Cleanup();return false;}
    const FVector BrotherPosition(Station->GetActorLocation().X,Station->GetActorLocation().Y-100,80);
    auto* CampActor=World->SpawnActor<AActor>();
    auto* CampRoot=NewObject<USceneComponent>(CampActor);
    CampActor->AddInstanceComponent(CampRoot);CampActor->SetRootComponent(CampRoot);CampRoot->RegisterComponent();
    CampActor->SetActorLocation(BrotherPosition);
    auto* CampStock=NewObject<UHearthwardInventoryComponent>(CampActor);
    CampActor->AddInstanceComponent(CampStock);CampStock->RegisterComponent();
    auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>();
    Brother->SetActorLocation(BrotherPosition);Brother->InitializeCompanion(CampStock,CampActor);
    auto* AI=World->GetSubsystem<UHearthwardLocalAISubsystem>();
    if(!TestTrue(TEXT("Source regression starts at camp without a navigation request"),Brother->IsAtCamp())
        || !TestTrue(TEXT("Source regression has a communicable living brother"),Brother->CanCommunicate(Player)))
    {Screen->RemoveFromParent();Cleanup();return false;}

    FHearthwardAgentGoal MissingTarget;MissingTarget.Intent=TEXT("nature_care");MissingTarget.Item=TEXT("harvest");
    MissingTarget.Quantity=1;MissingTarget.QuantityMode=TEXT("action_count");MissingTarget.SourceRef=TEXT("known_target");
    if(!TestTrue(TEXT("A missing-target care draft has its valid typed contract"),HearthwardAgent::Validate(MissingTarget).IsEmpty())
        || !TestFalse(TEXT("The public staging fixture is unpaused"),World->IsPaused())
        || !TestEqual(TEXT("The actual companion preview identifies the missing target"),Brother->PreviewGoal(MissingTarget),FString(TEXT("TARGET_REQUIRED"))))
    {Screen->RemoveFromParent();Cleanup();return false;}
    const FString MissingTargetBag=Brother->Bag->DescribeInventory();
    TestFalse(TEXT("A care draft without a bound target cannot form an executable card"),AI->SetStructuredGoal(Player,Brother,MissingTarget));
    if(!TestEqual(TEXT("Public staging reached the actual missing-target branch"),AI->GetReasonCode(),FString(TEXT("TARGET_REQUIRED"))))
    {Screen->RemoveFromParent();Cleanup();return false;}
    TestFalse(TEXT("Missing-target staging retains no candidate"),AI->HasCandidate());
    TestEqual(TEXT("Missing target asks for information rather than reporting unavailable facilities"),AI->GetLastAppliedIntent(),FString(TEXT("clarify")));
    TestEqual(TEXT("Missing-target clarification records exactly one public conversation turn"),AI->GetClarificationTurns(),1);
    TestEqual(TEXT("The manual draft retains its canonical original in working memory"),AI->GetMemorySnapshot().WorkingGoal.Original,HearthwardAgent::GoalText(MissingTarget));
    TestEqual(TEXT("Missing-target clarification does not change the brother inventory"),Brother->Bag->DescribeInventory(),MissingTargetBag);

    auto* Nature=World->GetSubsystem<UHearthwardNatureSubsystem>();
    FHearthwardCrop IdentifiedCrop;IdentifiedCrop.Id=FGuid::NewGuid();IdentifiedCrop.Definition=TEXT("greens");
    IdentifiedCrop.Position=Player->GetActorLocation()-FVector(0,0,90);IdentifiedCrop.Planted=Nature->State.Calendar;
    Nature->State.Crops.Add(IdentifiedCrop);
    FHearthwardPen IdentifiedPen;IdentifiedPen.Id=FGuid::NewGuid();IdentifiedPen.Definition=TEXT("goat");
    IdentifiedPen.Position=IdentifiedCrop.Position+FVector(100,0,0);Nature->State.Pens.Add(IdentifiedPen);Nature->RebuildActors();
    FHearthwardAgentGoal Care;Care.Intent=TEXT("nature_care");Care.Item=TEXT("water");Care.Quantity=1;
    Care.QuantityMode=TEXT("action_count");Care.SourceRef=TEXT("known_target");Care.Station=IdentifiedCrop.Id;
    if(!TestNotNull(TEXT("Identified crop owns a real Nature actor"),Nature->Actor(IdentifiedCrop.Id))
        || !TestNotNull(TEXT("Identified pen owns a real Nature actor"),Nature->Actor(IdentifiedPen.Id))
        || !TestTrue(TEXT("The correctly bound water target satisfies the actual preview"),Brother->PreviewGoal(Care).IsEmpty()))
    {Screen->RemoveFromParent();Cleanup();return false;}
    Care.Station.Invalidate();
    const FString IdentifiedNature=Nature->Describe();const FString BeforeWaterBag=Brother->Bag->DescribeInventory();
    const bool WaterStaged=AI->SetStructuredGoal(Player,Brother,Care);
    TestTrue(TEXT("A unique nearby unwatered crop stages through the public entry without a supplied station"),WaterStaged);
    if(WaterStaged)TestEqual(TEXT("The water candidate binds the actual identified crop GUID"),AI->GetCandidate().Station,IdentifiedCrop.Id);
    TestEqual(TEXT("Unconfirmed water staging changes no actual Nature state"),Nature->Describe(),IdentifiedNature);
    TestEqual(TEXT("Unconfirmed water staging changes no brother inventory"),Brother->Bag->DescribeInventory(),BeforeWaterBag);
    AI->CancelPending();
    if(!TestEqual(TEXT("Feed control uses an actual public inventory grant"),Brother->Bag->TryAdd(TEXT("feed"),1),EHearthwardInventoryResult::Success))
    {Screen->RemoveFromParent();Cleanup();return false;}
    Care.Item=TEXT("deposit_feed");Care.Station=IdentifiedPen.Id;
    if(!TestTrue(TEXT("The correctly bound feed target satisfies the actual preview"),Brother->PreviewGoal(Care).IsEmpty()))
    {Screen->RemoveFromParent();Cleanup();return false;}
    Care.Station.Invalidate();const FString BeforeFeedBag=Brother->Bag->DescribeInventory();
    const bool FeedStaged=AI->SetStructuredGoal(Player,Brother,Care);
    TestTrue(TEXT("A unique nearby pen still stages the feed branch without a supplied station"),FeedStaged);
    if(FeedStaged)TestEqual(TEXT("The feed candidate binds the actual identified pen GUID"),AI->GetCandidate().Station,IdentifiedPen.Id);
    TestEqual(TEXT("Unconfirmed feed staging changes no actual Nature state"),Nature->Describe(),IdentifiedNature);
    TestEqual(TEXT("Unconfirmed feed staging consumes no actual feed"),Brother->Bag->DescribeInventory(),BeforeFeedBag);
    AI->CancelPending();Brother->Bag->TryRemove(TEXT("feed"),1);
    Nature->State.Crops.RemoveAll([&](const auto& C){return C.Id==IdentifiedCrop.Id;});
    Nature->State.Pens.RemoveAll([&](const auto& P){return P.Id==IdentifiedPen.Id;});Nature->RebuildActors();

    const auto BeforeRepairInventory=Brother->Bag->Snapshot();
    const FString BeforeRepairCamp=CampStock->DescribeInventory();
    FHearthwardAgentGoal Repair;
    if(!TestTrue(TEXT("The unchanged CPU A03 model output parses"),HearthwardAgent::Parse(
        TEXT(R"({"intent":"repair","item":"axe","quantity":1,"mode":"one_owned","source":"bag","limits":[],"unresolved":[],"npc_line":"哥，这张卡维修你背包里的石制战斧，请确认后再执行。"})"),Repair))
        || !TestEqual(TEXT("The repair fixture starts without an axe"),Brother->Bag->GetItemCount(TEXT("axe")),0)
        || !TestEqual(TEXT("Two actual owned axe instances are granted"),Brother->Bag->TryAdd(TEXT("axe"),2),EHearthwardInventoryResult::Success))
    {Screen->RemoveFromParent();Cleanup();return false;}
    Repair.Original=TEXT("修好你自己的斧头");
    Repair.Station=Workbench->Id;
    if(!TestEqual(TEXT("Two owned instances reach the actual repair ambiguity"),Brother->PreviewGoal(Repair),FString(TEXT("AMBIGUOUS_TARGET"))))
    {Screen->RemoveFromParent();Cleanup();return false;}
    Repair.Station.Invalidate();
    const FString TwoAxes=Brother->Bag->DescribeInventory();
    TestFalse(TEXT("An unspecified repair with two owned instances forms no executable card"),AI->SetStructuredGoal(Player,Brother,Repair));
    TestEqual(TEXT("The public structured entry uses its actual canonical input"),AI->GetLastInput(),HearthwardAgent::GoalText(Repair));
    TestEqual(TEXT("The public repair draft retains that canonical input, without claiming free-text replay"),AI->GetMemorySnapshot().WorkingGoal.Original,HearthwardAgent::GoalText(Repair));
    TestEqual(TEXT("The actual repair ambiguity reason is retained"),AI->GetReasonCode(),FString(TEXT("AMBIGUOUS_TARGET")));
    TestEqual(TEXT("Multiple owned repair targets ask for clarification"),AI->GetLastAppliedIntent(),FString(TEXT("clarify")));
    TestEqual(TEXT("Repair ambiguity records exactly one clarification turn"),AI->GetClarificationTurns(),1);
    TestEqual(TEXT("Unconfirmed ambiguous repair changes no equipment"),Brother->Bag->DescribeInventory(),TwoAxes);
    TestEqual(TEXT("Unconfirmed ambiguous repair spends no camp stock"),CampStock->DescribeInventory(),BeforeRepairCamp);
    Repair.Quantity=2;
    TestFalse(TEXT("An invalid repair quantity cannot form a card"),AI->SetStructuredGoal(Player,Brother,Repair));
    TestEqual(TEXT("Contract ambiguity still refuses even with two owned axes"),AI->GetLastAppliedIntent(),FString(TEXT("refuse")));
    TestEqual(TEXT("Contract refusal records no target clarification"),AI->GetClarificationTurns(),0);
    Repair.Quantity=1;
    const FGuid RepairTarget=Brother->Bag->FirstInstance(TEXT("axe"));
    if(!TestTrue(TEXT("The explicit repair target is an actual owned damaged instance"),Brother->Bag->WearInstance(RepairTarget,10)))
    {Screen->RemoveFromParent();Cleanup();return false;}
    Repair.EquipmentId=RepairTarget;
    const bool ExplicitRepair=AI->SetStructuredGoal(Player,Brother,Repair);
    TestTrue(TEXT("An explicit actual instance remains accepted among multiple owned axes"),ExplicitRepair);
    if(ExplicitRepair)TestEqual(TEXT("Explicit repair retains the chosen instance GUID"),AI->GetCandidate().EquipmentId,RepairTarget);
    AI->CancelPending();
    const auto RepairAxes=Brother->Bag->Snapshot().Instances;
    for(const auto& RepairInstance:RepairAxes)
        if(RepairInstance.Definition==TEXT("axe") && RepairInstance.Id!=RepairTarget)Brother->Bag->RemoveInstance(RepairInstance.Id);
    Repair.EquipmentId.Invalidate();
    const bool UniqueRepair=AI->SetStructuredGoal(Player,Brother,Repair);
    TestTrue(TEXT("A unique owned repair instance still binds automatically"),UniqueRepair);
    if(UniqueRepair)TestEqual(TEXT("Automatic repair binding uses the remaining actual GUID"),AI->GetCandidate().EquipmentId,RepairTarget);
    AI->CancelPending();Brother->Bag->RemoveInstance(RepairTarget);
    TestFalse(TEXT("Zero owned repair instances form no card"),AI->SetStructuredGoal(Player,Brother,Repair));
    TestEqual(TEXT("Missing ownership still refuses"),AI->GetLastAppliedIntent(),FString(TEXT("refuse")));
    TestEqual(TEXT("Missing ownership does not ask to select an owned target"),AI->GetClarificationTurns(),0);
    AI->CancelPending();
    TestTrue(TEXT("The repair controls restore the original public inventory snapshot"),Brother->Bag->RestoreInventory(BeforeRepairInventory));

    for(const FString& SourceRef:{FString(TEXT("camp")),FString(TEXT("bag"))})
    {
        const bool CampSource=SourceRef==TEXT("camp");
        const FString Label=CampSource?TEXT("Camp source"):TEXT("Bag source");
        TMap<FName,int32> OldCampStock;
        for(const FName Item:{FName(TEXT("wood")),FName(TEXT("rope"))})
            if(Storage->GetItemCount(Item)>0)OldCampStock.Add(Item,Storage->GetItemCount(Item));
        if(!TestTrue(*(Label+TEXT(" fixture prepares exactly twenty camp wood")),Storage->Adjust(OldCampStock,{{TEXT("wood"),20}})))
        {Screen->RemoveFromParent();Cleanup();return false;}
        const int32 OldWood=Brother->Bag->GetItemCount(TEXT("wood"));
        if(OldWood>0 && !TestEqual(*(Label+TEXT(" fixture resets brother wood through inventory API")),
            Brother->Bag->TryRemove(TEXT("wood"),OldWood),EHearthwardInventoryResult::Success))
        {Screen->RemoveFromParent();Cleanup();return false;}
        if(!TestEqual(*(Label+TEXT(" fixture prepares exactly twelve brother wood")),
            Brother->Bag->TryAdd(TEXT("wood"),12),EHearthwardInventoryResult::Success))
        {Screen->RemoveFromParent();Cleanup();return false;}
        TestEqual(*(Label+TEXT(" starts with camp wood twenty")),Storage->GetItemCount(TEXT("wood")),20);
        TestEqual(*(Label+TEXT(" starts with brother wood twelve")),Brother->Bag->GetItemCount(TEXT("wood")),12);
        TestEqual(*(Label+TEXT(" starts with no camp rope")),Storage->GetItemCount(TEXT("rope")),0);
        if(!TestTrue(*(Label+TEXT(" fixture satisfies the real Workshop check")),
            HearthwardWorkshop::Check(Brother,Station,Brother->Bag,TEXT("craft"),TEXT("rope"),1).IsEmpty()))
        {Screen->RemoveFromParent();Cleanup();return false;}

        FHearthwardAgentGoal Goal;Goal.Intent=TEXT("craft");Goal.Item=TEXT("rope");Goal.Quantity=1;
        Goal.QuantityMode=TEXT("batches");Goal.SourceRef=SourceRef;Goal.Station=Workbench->Id;
        if(!TestTrue(*(Label+TEXT(" stages its actual structured goal")),AI->SetStructuredGoal(Player,Brother,Goal)))
        {Screen->RemoveFromParent();Cleanup();return false;}
        TestEqual(*(Label+TEXT(" is retained in the actual candidate")),AI->GetCandidate().SourceRef,SourceRef);
        TestEqual(*(Label+TEXT(" staging does not spend camp materials")),Storage->GetItemCount(TEXT("wood")),20);
        TestEqual(*(Label+TEXT(" staging does not spend brother materials")),Brother->Bag->GetItemCount(TEXT("wood")),12);
        if(!TestTrue(*(Label+TEXT(" confirms the actual candidate")),AI->ConfirmCandidate(AI->GetCandidateId())))
        {Screen->RemoveFromParent();Cleanup();return false;}
        TestEqual(*(Label+TEXT(" is retained in the accepted execution goal")),Brother->GetGoal().SourceRef,SourceRef);
        for(int32 Step=0;Step<16 && Brother->GetPhase()!=EHearthwardCompanionPhase::Completed;++Step)Brother->Tick(.01f);
        TestTrue(*(Label+TEXT(" completes through the production executor")),Brother->GetPhase()==EHearthwardCompanionPhase::Completed);
        TestEqual(*(Label+TEXT(" delivers one real rope")),Brother->GetDelivered(),1);
        TestEqual(*(Label+TEXT(" pays from the authorized camp stock")),Storage->GetItemCount(TEXT("wood")),CampSource?18:20);
        TestEqual(*(Label+TEXT(" retains the correct brother stock")),Brother->Bag->GetItemCount(TEXT("wood")),CampSource?12:10);
        TestEqual(*(Label+TEXT(" deposits one real product into camp")),Storage->GetItemCount(TEXT("rope")),1);
        TestEqual(*(Label+TEXT(" leaves no crafted product in the brother bag")),Brother->Bag->GetItemCount(TEXT("rope")),0);
    }

    const auto Cost=HearthwardWorkshop::Materials(TEXT("craft"),TEXT("firepot"),1);
    TArray<FName> CostItems;Cost.GetKeys(CostItems);
    if(!TestTrue(TEXT("Recovery fixture uses a real known multi-material workbench recipe"),
        CostItems.Num()>1 && Gameplay->KnowsRecipe(TEXT("firepot"))))
    {Screen->RemoveFromParent();Cleanup();return false;}
    TMap<FName,int32> PreviousStock;
    for(const FName Item:CostItems)
    {
        if(Storage->GetItemCount(Item)>0)PreviousStock.Add(Item,Storage->GetItemCount(Item));
        const int32 Count=Brother->Bag->GetItemCount(Item);
        if(Count>0 && !TestEqual(TEXT("Recovery fixture clears held inputs through inventory API"),
            Brother->Bag->TryRemove(Item,Count),EHearthwardInventoryResult::Success))
        {Screen->RemoveFromParent();Cleanup();return false;}
    }
    if(Storage->GetItemCount(TEXT("firepot"))>0)PreviousStock.Add(TEXT("firepot"),Storage->GetItemCount(TEXT("firepot")));
    if(!TestTrue(TEXT("Recovery fixture gives camp exactly one real recipe"),Storage->Adjust(PreviousStock,Cost)))
    {Screen->RemoveFromParent();Cleanup();return false;}
    // Iterate the actual material order: the first is free; another operation reserves all later inputs.
    TMap<FName,int32> ReservedInputs;
    for(int32 Index=1;Index<CostItems.Num();++Index)ReservedInputs.Add(CostItems[Index],Cost.FindChecked(CostItems[Index]));
    const FGuid OtherOperation=FGuid::NewGuid();
    if(!TestTrue(TEXT("Another real operation reserves later recipe inputs"),Storage->Reserve(OtherOperation,ReservedInputs)))
    {Screen->RemoveFromParent();Cleanup();return false;}
    FHearthwardAgentGoal RecoveryGoal;RecoveryGoal.Intent=TEXT("craft");RecoveryGoal.Item=TEXT("firepot");RecoveryGoal.Quantity=1;
    RecoveryGoal.QuantityMode=TEXT("batches");RecoveryGoal.SourceRef=TEXT("camp");RecoveryGoal.Station=Workbench->Id;
    if(!TestTrue(TEXT("Real camp recovery candidate stages"),AI->SetStructuredGoal(Player,Brother,RecoveryGoal))
        || !TestTrue(TEXT("Real camp recovery candidate confirms"),AI->ConfirmCandidate(AI->GetCandidateId())))
    {Storage->Release(OtherOperation);Screen->RemoveFromParent();Cleanup();return false;}
    for(int32 Step=0;Step<16;++Step)Brother->Tick(.01f);
    TestTrue(TEXT("Reserved inputs block normal execution safely"),Brother->GetPhase()==EHearthwardCompanionPhase::WaitingAtCamp
        || Brother->GetPhase()==EHearthwardCompanionPhase::HoldingSafely);
    TestEqual(TEXT("Blocked production has not created its product"),Storage->GetItemCount(TEXT("firepot")),0);
    for(const auto& Reserved:ReservedInputs)
        TestEqual(TEXT("Another operation's input remains in camp"),Storage->GetItemCount(Reserved.Key),Reserved.Value);
    AddInfo(FString::Printf(TEXT("Material recovery before release: first=%s camp=%d held=%d phase=%s"),
        *CostItems[0].ToString(),Storage->GetItemCount(CostItems[0]),Brother->Bag->GetItemCount(CostItems[0]),
        *UEnum::GetValueAsString(Brother->GetPhase())));
    Storage->Release(OtherOperation);
    if(!TestTrue(TEXT("Public resume accepts the same command after reservation release"),Brother->ResumeBlocked(Player)))
    {Screen->RemoveFromParent();Cleanup();return false;}
    for(int32 Step=0;Step<16 && Brother->GetPhase()!=EHearthwardCompanionPhase::Completed;++Step)Brother->Tick(.01f);
    TestTrue(TEXT("Released recipe resumes to actual completion"),Brother->GetPhase()==EHearthwardCompanionPhase::Completed);
    TestEqual(TEXT("Recovered production delivers exactly one product"),Storage->GetItemCount(TEXT("firepot")),1);
    for(const auto& Input:Cost)
    {
        TestEqual(TEXT("Recovered production consumes exactly the authorized camp input"),Storage->GetItemCount(Input.Key),0);
        TestEqual(TEXT("Recovered production leaves no duplicate held recipe input"),Brother->Bag->GetItemCount(Input.Key),0);
    }
    Screen->RemoveFromParent();Cleanup();return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHarvestToolPresentation057Test,
    "Hearthward.Crafting057.HarvestToolPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHarvestToolPresentation057Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();
    auto* World=Instance->GetWorld();
    auto* Player=World->SpawnActor<AActor>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);
    Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);
    Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
    auto* Resource=World->SpawnActor<AActor>();
    auto* Target=NewObject<UHearthwardHarvestTargetComponent>(Resource);
    Resource->AddInstanceComponent(Target);Resource->SetRootComponent(Target);
    Target->ResourceKey=TEXT("harvest-presentation");Target->Item=TEXT("wood");Target->Capacity=12;Target->MaxDistance=240;
    Target->RegisterComponent();
    auto* Harvest=World->GetSubsystem<UHearthwardHarvestSubsystem>();

    TestTrue(TEXT("Missing wood tool hides unusable interaction"),Target->GetInteractionPrompt(Player).IsEmpty());
    TestFalse(TEXT("A missing tool does not advertise a zero-yield harvest"),Target->GetInteractionPrompt(Player).Contains(TEXT("×0")));
    Bag->TryAdd(TEXT("pickaxe"),1);
    TestTrue(TEXT("A wrong tool hides unusable interaction"),Target->GetInteractionPrompt(Player).IsEmpty());
    Bag->TryAdd(TEXT("axe"),1);
    const FGuid StoneAxe=Bag->FirstInstance(TEXT("axe"));Bag->WearInstance(StoneAxe,10);
    FString Prompt=Target->GetInteractionPrompt(Player);
    TestTrue(TEXT("An unequipped valid axe is shown as the actual harvesting tool"),Prompt.Contains(TEXT("石制战斧")));
    TestTrue(TEXT("Tool wear is displayed from its real instance"),Prompt.Contains(TEXT("耐久 70.00 / 80")));
    TestTrue(TEXT("Grade-one tool yield is displayed"),Prompt.Contains(TEXT("×2")));
    Bag->TryAdd(TEXT("blunt_2"),1);
    const FGuid MetalAxe=Bag->FirstInstance(TEXT("blunt_2"));Bag->WearInstance(MetalAxe,20);
    Prompt=Target->GetInteractionPrompt(Player);
    TestTrue(TEXT("The highest valid bag tool appears in the prompt"),Prompt.Contains(TEXT("金属战斧")));
    TestTrue(TEXT("The selected tool's own durability appears in the prompt"),Prompt.Contains(TEXT("耐久 60.00 / 80")));
    TestTrue(TEXT("Grade-two tool yield is displayed"),Prompt.Contains(TEXT("×3")));
    TestTrue(TEXT("Actual harvest uses the same selected tool and yield"),Target->CompleteInteraction(Player).Contains(TEXT("×3")));
    TestEqual(TEXT("Actual output agrees with the prompt"),Bag->GetItemCount(TEXT("wood")),3);
    TestEqual(TEXT("The advertised metal axe pays the actual durability cost"),Bag->FindInstance(MetalAxe)->Durability,59.);
    TestEqual(TEXT("The lower-grade axe remains untouched"),Bag->FindInstance(StoneAxe)->Durability,70.);
    TestTrue(TEXT("The next prompt shows the actual depleted source"),Target->GetInteractionPrompt(Player).Contains(TEXT("剩余 9")));

    Bag->WearInstance(StoneAxe,70);Bag->WearInstance(MetalAxe,59);
    TestTrue(TEXT("Broken matching tools hide unusable interaction"),Target->GetInteractionPrompt(Player).IsEmpty());
    TestFalse(TEXT("Broken tools do not advertise a zero-yield harvest"),Target->GetInteractionPrompt(Player).Contains(TEXT("×0")));
    Target->Item=TEXT("herb");Target->ResourceKey=TEXT("herb-presentation");Target->Capacity=4;
    Prompt=Target->GetInteractionPrompt(Player);
    TestTrue(TEXT("Herbs retain their approved tool-free harvest"),Prompt.Contains(TEXT("徒手")) && Prompt.Contains(TEXT("×2")));
    Target->Item=TEXT("wood");Target->ResourceKey=TEXT("harvest-presentation");Target->Capacity=12;
    Harvest->Restore({{Target->ResourceKey,12}});
    TestTrue(TEXT("An exhausted source hides interaction"),Target->GetInteractionPrompt(Player).IsEmpty());

    Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}
#endif
