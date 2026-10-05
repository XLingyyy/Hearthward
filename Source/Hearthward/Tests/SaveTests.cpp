#include "../HearthwardCharacter.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Nature/HearthwardNatureActor.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"
#include "../Save/HearthwardSaveGame.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Crc.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../UI/HearthwardLoadingSubsystem.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformTime.h"
#include "../Campaign/HearthwardCampaignState.h"
#include "../Save/HearthwardSaveCompatibility.h"
#include "../Update/HearthwardUpdateSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/GameInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FHearthwardSavePoint Point(bool Manual = false)
{
    FHearthwardSavePoint P;
    P.SaveId = FGuid::NewGuid(); P.CampaignId = FGuid::NewGuid(); P.Created = FDateTime::UtcNow(); P.Manual = Manual;
    P.World.Map = TEXT("PROTOTYPE_ONLY");
    P.World.SurvivalVersion=1;
    P.World.NPCStateVersion=HearthwardSave::NPCStateVersion;
    P.World.NPCMemory.Campaign=P.CampaignId;
    return P;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSavePoolTest, "Hearthward.Save.PoolProtectionAndSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSavePoolTest::RunTest(const FString& Parameters)
{
    TArray<FHearthwardSavePoint> Points;
    TestEqual(TEXT("Empty pool can create initial node"), HearthwardSave::SelectSlot(Points), 0);
    for (int32 I = 0; I < 50; ++I) Points.Add(Point(true));
    TestEqual(TEXT("Manual nodes protected"), HearthwardSave::SelectSlot(Points), INDEX_NONE);
    for (auto& P : Points) { P.Manual = false; P.Locked = true; }
    TestEqual(TEXT("All locked protected"), HearthwardSave::SelectSlot(Points), INDEX_NONE);
    Points[7].Locked = false; Points[7].Created = FDateTime(2000, 1, 1);
    Points[12].Locked = false;
    TestEqual(TEXT("Oldest unlocked auto selected globally across progress IDs"), HearthwardSave::SelectSlot(Points), 7);
    TestEqual(TEXT("New progress may use replaceable auto capacity"), HearthwardSave::SelectSlot(Points), 7);
    Points.RemoveAt(0);
    TestEqual(TEXT("Explicit deletion permits initial point"), HearthwardSave::SelectSlot(Points), 49);
    FHearthwardSaveSafety S;
    TestTrue(TEXT("Safe allowed"), S.CanSave());
    S.SevereHunger = true; TestTrue(TEXT("Hunger allowed with warning"), S.CanSave());
    bool FHearthwardSaveSafety::* Hazards[] = {&FHearthwardSaveSafety::Combat, &FHearthwardSaveSafety::EitherDowned,
        &FHearthwardSaveSafety::Pursued, &FHearthwardSaveSafety::Drowning, &FHearthwardSaveSafety::Falling, &FHearthwardSaveSafety::CompanionDanger};
    for (auto Hazard : Hazards) { S.*Hazard = true; TestFalse(TEXT("Each confirmed hazard prevents save"), S.CanSave()); S.*Hazard = false; }
    auto* World = NewObject<UWorld>();
    auto* Knowledge = NewObject<UHearthwardSaveSubsystem>(World);
    for (int32 I = 0; I < 8; ++I) Knowledge->RememberExchange(TEXT("untrusted player"), FString::ChrN(1000, TEXT('X')));
    const auto Context = Knowledge->RecentKnowledge();
    TestTrue(TEXT("Full exchanges retained for save"), Knowledge->GetKnowledge().Len() > 8000);
    TestTrue(TEXT("Prompt history bounded for 4096-token runtime"), FString::Join(Context, TEXT("\n")).Len() <= 516 && Context.Num() <= 8);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveActualLoadPointEpochTest,"Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveActualLoadPointEpochTest::RunTest(const FString&)
{
    FString PoolArgument;FGuid PoolId;
    if(!TestTrue(TEXT("Actual LoadPoint uses the runner's isolated save pool"),
        FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),PoolArgument) && FGuid::Parse(PoolArgument,PoolId)))return false;
    FString RestartPhase;
    if(FParse::Value(FCommandLine::Get(),TEXT("Hearthward071RestartPhase="),RestartPhase)
        && !TestTrue(TEXT("Explicit restart phase is write or read"),RestartPhase==TEXT("write") || RestartPhase==TEXT("read")))return false;
    const bool RestartEcology=FParse::Param(FCommandLine::Get(),TEXT("Hearthward071RestartEcology"));
    if(RestartEcology && !TestFalse(TEXT("Ecology requires an explicit restart phase"),RestartPhase.IsEmpty()))return false;
    const bool RestartTwoCamps=FParse::Param(FCommandLine::Get(),TEXT("Hearthward071RestartTwoCamps"));
    if(RestartTwoCamps && !TestFalse(TEXT("Two camps require an explicit restart phase"),RestartPhase.IsEmpty()))return false;
    const FVector TwoCampAnchor=RestartTwoCamps?HearthwardCampaign::XY(HearthwardCampaign::Find(TEXT("locations"),TEXT("camp")),TEXT("xy")):FVector::ZeroVector;
    const FString ManifestPath=FPaths::ProjectSavedDir()/TEXT("Task071")/PoolId.ToString(EGuidFormats::Digits)/TEXT("manifest.json");
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    const FName WorldName=RestartPhase.IsEmpty()?FName():FName(TEXT("Task071RestartFixture"));
    auto* FixturePackage=RestartPhase.IsEmpty()?nullptr:CreatePackage(TEXT("/Temp/Task071RestartFixture"));
    UGameInstance* Instance=nullptr;UWorld* World=nullptr;
    if(!RestartPhase.IsEmpty())
    {
        Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone(WorldName,FixturePackage);World=Instance->GetWorld();
    }
    else
    {
        World=UWorld::CreateWorld(EWorldType::Game,false,WorldName,FixturePackage,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    }
    if(!RestartPhase.IsEmpty())TestEqual(TEXT("Explicit restart fixture has a stable actual map package"),UGameplayStatics::GetCurrentLevelName(World,true),FString(TEXT("Task071RestartFixture")));
    ON_SCOPE_EXIT { if(World->HasBegunPlay())World->EndPlay(EEndPlayReason::Quit);if(Instance)Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false); };
    FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Hero=World->SpawnActor<AHearthwardCharacter>(FVector(0,0,100),FRotator::ZeroRotator,Spawn);
    if(RestartTwoCamps)Hero->SetActorLocation(TwoCampAnchor+FVector(0,0,100));
    auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Hero);
    if(Instance)
    {
        auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));Controller->SetPlayer(LocalPlayer);
    }
    auto* Camp=World->SpawnActor<AActor>();auto* CampRoot=NewObject<USceneComponent>(Camp);
    Camp->AddInstanceComponent(CampRoot);Camp->SetRootComponent(CampRoot);CampRoot->RegisterComponent();
    if(RestartTwoCamps)Camp->SetActorLocation(TwoCampAnchor+FVector(0,0,100));
    auto* Stock=NewObject<UHearthwardInventoryComponent>(Camp);Camp->AddInstanceComponent(Stock);Stock->RegisterComponent();
    auto* Brother=World->SpawnActor<AHearthwardCompanionFixture>(FVector(0,200,100),FRotator::ZeroRotator,Spawn);
    if(RestartTwoCamps)Brother->SetActorLocation(TwoCampAnchor+FVector(0,200,100));
    Brother->InitializeFixture(Stock,Camp);
    auto* PlayerBag=Hero->FindComponentByClass<UHearthwardInventoryComponent>();
    auto* Store=World->GetSubsystem<UHearthwardStorageSubsystem>();auto* Save=World->GetSubsystem<UHearthwardSaveSubsystem>();
    auto* AI=World->GetSubsystem<UHearthwardLocalAISubsystem>();auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
    if(!TestTrue(TEXT("Real controlled Hero and single initialized Brother have normal stock"),
        UGameplayStatics::GetPlayerPawn(World,0)==Hero && PlayerBag->TryAdd(TEXT("wood"),3)==EHearthwardInventoryResult::Success
        && Stock->TryAdd(TEXT("wood"),8)==EHearthwardInventoryResult::Success))return false;
    auto* Gameplay=Hero->FindComponentByClass<UHearthwardGameplayComponent>();
    auto* Building=Hero->FindComponentByClass<UHearthwardBuildingComponent>();
    auto* Timer=Hero->FindComponentByClass<UHearthwardTimedActionComponent>();
    auto* Economy=World->GetSubsystem<UHearthwardCampSubsystem>();auto* Nature=World->GetSubsystem<UHearthwardNatureSubsystem>();
    if(!RestartPhase.IsEmpty())
    {
        auto* Floor=World->SpawnActor<AActor>();Floor->Tags.Add(TEXT("Hearthward.NatureGround"));
        auto* Box=NewObject<UBoxComponent>(Floor);Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(80000,80000,10));Box->SetCollisionObjectType(ECC_WorldStatic);
        if(RestartTwoCamps)Box->SetBoxExtent(FVector(200000,160000,10));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent();Floor->SetActorLocation(FVector(0,0,-10));
        World->InitializeActorsForPlay(FURL(),false);World->GetWorldSettings()->NotifyBeginPlay();
        if(!TestTrue(TEXT("Mature fixture initializes actors and real building/timer lifecycle"),World->AreActorsInitialized()
            && World->HasBegunPlay() && Building->HasBegunPlay() && Timer->HasBegunPlay()))return false;
        World->Tick(LEVELTICK_All,.025f);
        // Synchronous World ticks share GFrameCounter; advance real movement after the first scheduled tick.
        for(int32 I=1;I<80 && (!Hero->GetCharacterMovement()->IsMovingOnGround() || !Brother->GetCharacterMovement()->IsMovingOnGround());++I)
        {
            Hero->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
            Brother->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
        }
        if(!TestTrue(FString::Printf(TEXT("Both actual capsules settle: Hero mode=%d z=%.3f active=%d tick=%d controller=%d; Brother mode=%d z=%.3f active=%d tick=%d controller=%d"),int32(Hero->GetCharacterMovement()->MovementMode),Hero->GetActorLocation().Z,Hero->GetCharacterMovement()->IsActive(),Hero->GetCharacterMovement()->IsComponentTickEnabled(),Hero->GetController()!=nullptr,int32(Brother->GetCharacterMovement()->MovementMode),Brother->GetActorLocation().Z,Brother->GetCharacterMovement()->IsActive(),Brother->GetCharacterMovement()->IsComponentTickEnabled(),Brother->GetController()!=nullptr),
            Hero->GetCharacterMovement()->IsMovingOnGround() && Brother->GetCharacterMovement()->IsMovingOnGround()))return false;
        Gameplay->EnableAdventure();Nature->EnsureWorld();FHitResult Ground;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(Task071Terrain),false,Hero);
        const bool Supported=World->LineTraceSingleByObjectType(Ground,FVector(0,0,500),FVector(0,0,-500),FCollisionObjectQueryParams(ECC_WorldStatic),Query);
        if(!TestTrue(TEXT("Public adventure initialization creates a real camp and seeded ecology on tagged ground"),Gameplay->Enabled
            && Economy->State.Camps.Num()==1 && !Economy->State.CampAt(Hero->GetActorLocation()).IsNone() && Nature->State.Seed!=0
            && Nature->State.Camps.Contains(TEXT("camp")) && Supported && Ground.GetActor()==Floor && FMath::Abs(Ground.ImpactPoint.Z)<1))return false;
        // These living arena opponents are scene fixtures; keep them outside the production camp.
        const FVector Site=Economy->State.Camps[0].Position;int32 Relocated=0;
        for(TActorIterator<AActor> It(World);It;++It)
            if(auto* Target=It->FindComponentByClass<UHearthwardCombatTargetComponent>();Target && Target->PrototypeEncounter)
            {
                const float Health=Target->Health;const FVector Desired=Site+FVector(30000,30000+Relocated*1000,0);
                FHitResult Support;
                if(!TestTrue(TEXT("Each prototype opponent's explicit scene destination has real terrain support"),
                    World->LineTraceSingleByObjectType(Support,Desired+FVector(0,0,500),Desired-FVector(0,0,500),FCollisionObjectQueryParams(ECC_WorldStatic),Query)
                    && Support.GetActor()==Floor && Support.ImpactNormal.Z>.99))return false;
                FVector BoundsOrigin,BoundsExtent;It->GetActorBounds(true,BoundsOrigin,BoundsExtent);
                const double FloorOffset=It->GetActorLocation().Z-(BoundsOrigin.Z-BoundsExtent.Z)+2;
                if(!TestTrue(TEXT("Public scene placement keeps the exact prototype target alive outside camp"),
                    Target->Alive() && It->SetActorLocation(Support.ImpactPoint+FVector(0,0,FloorOffset),false,nullptr,ETeleportType::TeleportPhysics)
                    && Target->Health==Health && FVector::Dist2D(It->GetActorLocation(),Site)>Economy->State.Radius()))return false;
                ++Relocated;
            }
        if(!TestEqual(TEXT("Scene preparation moves only the three real prototype encounters"),Relocated,3))return false;
        Economy->Advance(0,false);
        if(!TestTrue(TEXT("Actual camp safety accepts the prepared world without changing enemy health"),
            !Economy->State.Regions.ContainsByPredicate([](const auto& R){return !R.Safe;})))return false;
        if(RestartTwoCamps)
        {
            FVector Support;
            if(!TestTrue(TEXT("Formal first-camp anchor is created by normal adventure initialization on real ground"),
                World->GetSubsystem<UHearthwardCampaignSubsystem>()->Ground(TwoCampAnchor,Support) && Support.Equals(TwoCampAnchor,.01)
                && Economy->State.Camps[0].Id==TEXT("camp") && Economy->State.Camps[0].Position.Equals(TwoCampAnchor+FVector(0,0,100),.01)))return false;
        }
    }
    const bool Enabled=Save->EnablePrototype();
    if(!TestTrue(FString(TEXT("Real prototype save participants initialize: "))+Save->GetStatus(),Enabled))return false;
    FGuid EcologyFire,EcologyAnimal,EcologyFirstCorpse;FName EcologySlot;FString EcologySource;
    const auto MoveEcologyPlayer=[&](FVector At)
    {
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(Task071EcologyPlacement),false,Hero);
        if(!TestTrue(TEXT("Explicit ecology placement uses actual tagged terrain"),World->LineTraceSingleByObjectType(Hit,
            FVector(At.X,At.Y,500),FVector(At.X,At.Y,-500),FCollisionObjectQueryParams(ECC_WorldStatic),Query)
            && Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("Hearthward.NatureGround"))))return false;
        if(!TestTrue(TEXT("Public scene placement accepts the actual capsule height"),Hero->SetActorLocation(
            Hit.ImpactPoint+FVector(0,0,Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3),false,nullptr,ETeleportType::TeleportPhysics)))return false;
        Hero->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
        return TestTrue(TEXT("Ecology capsule stands naturally on queried terrain"),Hero->GetCharacterMovement()->IsMovingOnGround());
    };
    const auto WaitEcology=[&](double Minutes)
    {
        for(auto* Survival:{Hero->FindComponentByClass<UHearthwardSurvivalComponent>(),Brother->FindComponentByClass<UHearthwardSurvivalComponent>()})
        {
            if(!TestTrue(TEXT("Actual participants are alive and safe before paid waiting"),Survival && Survival->SafeToSave()))return false;
            while(Survival->Hunger()<80)
                if(!TestTrue(FString(TEXT("Actual food consumption prepares eight-hour waiting: "))+Survival->Status,Survival->Eat(TEXT("wild_food"))))return false;
        }
        const auto Before=Clock->GetSnapshot();FHearthwardTimeAdvanceRequest Request;
        Request.Kind=EHearthwardTimeAdvanceKind::Campfire;Request.Facility=EcologyFire;Request.Minutes=Minutes;
        Request.Campaign=Save->GetCampaignId();Request.Epoch=Store->GetTimelineEpoch();Request.OperationId=FGuid::NewGuid();Request.StartW=Before.ElapsedCalendarMinutes;
        const auto Receipt=Clock->RequestTimeAdvance(Request);
        if(!TestTrue(FString(TEXT("Real paid campfire request completes: "))+Receipt.Reason,Receipt.Accepted && Receipt.Completed
            && Receipt.Status==EHearthwardTimeAdvanceStatus::Completed && Receipt.CommittedMinutes==Minutes))return false;
        TestEqual(TEXT("Campfire waiting adds no active seconds"),Clock->GetSnapshot().ActivePlaySeconds,Before.ActivePlaySeconds);
        TestEqual(TEXT("Campfire waiting commits exactly the requested calendar minutes"),Clock->GetSnapshot().ElapsedCalendarMinutes,Before.ElapsedCalendarMinutes+Minutes);
        Nature->Tick(.5f);
        return true;
    };
    const auto CheckTwoCamps=[&](const FHearthwardWorldSave& Expected)
    {
        auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();FHearthwardCampaignState SavedCampaign;
        FHearthwardCampState SavedCamp;FHearthwardNatureState SavedNature;TSharedPtr<FJsonObject> SavedGameplay;
        if(!TestTrue(TEXT("Actual saved two-camp domains and gameplay parse through their normal contracts"),
            FHearthwardCampaignState::Parse(Expected.Campaign,SavedCampaign) && FHearthwardCampState::Parse(Expected.CampEconomy,SavedCamp)
            && FHearthwardNatureState::Parse(Expected.Nature,SavedNature)
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Expected.Gameplay),SavedGameplay)))return false;
        TestTrue(TEXT("Repeated load preserves all registered campaign identities, combat generations, flags and terminal reinforcements"),
            Campaign->State.Snapshot()==Expected.Campaign);
        TestTrue(TEXT("Repeated load preserves every camp, source, facility, unique assignment and invested batch"),Economy->State.Snapshot()==Expected.CampEconomy);
        TestTrue(TEXT("Repeated load preserves every ecological identity and both camp markers"),Nature->Describe()==Expected.Nature);
        if(!TestTrue(TEXT("Restored permanent victory remains a legitimate continuous two-camp state"),Campaign->State.Victory
            && Campaign->State.Phase==TEXT("reclaimed") && !Campaign->State.Legacy && !Campaign->State.LegacyHometown
            && Campaign->State.ReadyForVictory() && Campaign->State.Validate() && Economy->State.Hometown && Economy->State.Camps.Num()==2
            && Nature->State.Camps.Contains(TEXT("camp")) && Nature->State.Camps.Contains(TEXT("hometown"))
            && Campaign->State.Facts.Contains(TEXT("prologue_complete")) && Campaign->State.Facts.Contains(TEXT("home_saved"))
            && !Campaign->State.Facts.Contains(TEXT("home_continued")) && Gameplay->Activated.Contains(TEXT("hometown"))))return false;
        int32 Gifts=0,Beds=0,Access=0,Fires=0;
        for(const auto& Saved:SavedCamp.Facilities)
        {
            auto* Actor=Building->ResolveFacility(Saved.Id);
            if(!TestTrue(FString::Printf(TEXT("Actual load reconstructs facility %s"),*Saved.Id.ToString()),Actor!=nullptr))return false;
            if(Saved.Camp==TEXT("hometown"))
            {
                ++Gifts;Beds+=int32(Saved.Kind==TEXT("bed"));Access+=int32(Saved.Kind==TEXT("warehouse_access"));Fires+=int32(Saved.Kind==TEXT("campfire"));
                TestTrue(TEXT("All four hometown facilities retain original unpaid gift accounting"),Saved.Level==1 && Saved.Paid.IsEmpty() && !Saved.Paused);
            }
        }
        TestTrue(TEXT("Both different bed identities, warehouse and campfire survive actual reconstruction"),Gifts==4 && Beds==2 && Access==1 && Fires==1);
        const auto ActualBuildings=Building->Snapshot();const auto& SavedBuildings=SavedGameplay->GetArrayField(TEXT("buildings"));
        TestEqual(TEXT("Repeated load cannot duplicate any paid or gifted building"),ActualBuildings.Num(),SavedBuildings.Num());
        for(const auto& V:SavedBuildings)
        {
            const auto R=V->AsObject();FGuid Id;FVector Position;
            if(!TestTrue(TEXT("Actual saved building identity and position are readable"),FGuid::Parse(R->GetStringField(TEXT("id")),Id)
                && Position.InitFromString(R->GetStringField(TEXT("position")))))return false;
            const auto* Row=ActualBuildings.FindByPredicate([&](const auto& B){return B->AsObject()->GetStringField(TEXT("id"))==R->GetStringField(TEXT("id"));});
            auto* Actor=Building->ResolveFacility(Id);auto* Box=Actor?Cast<UBoxComponent>(Actor->GetRootComponent()):nullptr;
            TestTrue(TEXT("Every actual reconstructed building retains its saved recipe, position, yaw and root geometry"),Row && Box
                && (*Row)->AsObject()->GetStringField(TEXT("recipe"))==R->GetStringField(TEXT("recipe"))
                && (*Row)->AsObject()->GetStringField(TEXT("position"))==R->GetStringField(TEXT("position"))
                && (*Row)->AsObject()->GetNumberField(TEXT("yaw"))==R->GetNumberField(TEXT("yaw"))
                && Actor->GetActorLocation().Equals(Position+FVector(0,0,Box->GetScaledBoxExtent().Z),.01)
                && Actor->GetActorRotation().Equals(FRotator(0,R->GetNumberField(TEXT("yaw")),0),.01));
        }
        int32 Person1=0,Person2=0;
        for(const auto& R:Economy->State.Regions)
        {
            if(R.Workers.Contains(1)){++Person1;TestTrue(TEXT("Ordinary person one remains only in first-camp forage"),R.Id==TEXT("camp_forage") && R.Camp==TEXT("camp"));}
            if(R.Workers.Contains(2)){++Person2;TestTrue(TEXT("Migrated ordinary person two remains only in hometown forage"),R.Id==TEXT("hometown_forage") && R.Camp==TEXT("hometown"));}
        }
        TestTrue(TEXT("Actual restore does not duplicate ordinary workers across camps"),Person1==1 && Person2==1);
        for(const auto& Site:SavedCamp.Camps)
            TestTrue(TEXT("Each saved camp retains real identified sources"),Economy->State.Sources.ContainsByPredicate([&](const auto& S){return S.Camp==Site.Id;}));
        const auto* Reward=Expected.StorageItems.Instances.FindByPredicate([](const auto& I){return I.Definition==TEXT("hearth_blade");});
        const auto Shared=Store->InventorySnapshot();
        TestTrue(TEXT("Unique hometown reward survives without second grant"),Reward && Reward->UniqueClaim==TEXT("hometown_hearth_blade")
            && Shared.Instances.ContainsByPredicate([&](const auto& I){return I.Id==Reward->Id && I.Definition==Reward->Definition && I.UniqueClaim==Reward->UniqueClaim;})
            && Store->GetItemCount(TEXT("hearth_blade"))==1 && PlayerBag->GetItemCount(TEXT("hearth_blade"))==0 && Brother->Bag->GetItemCount(TEXT("hearth_blade"))==0);
        TestEqual(TEXT("Repeated victory restore cannot grant experience again"),double(Gameplay->Experience),SavedGameplay->GetNumberField(TEXT("experience")));
        const auto& RewardFacts=SavedGameplay->GetArrayField(TEXT("rewardFacts"));
        TestEqual(TEXT("Victory and every actual defeat fact are not replayed"),Gameplay->RewardFacts.Num(),RewardFacts.Num());
        for(const auto& Fact:RewardFacts)TestTrue(TEXT("Original reward fact retains its identity"),Gameplay->RewardFacts.Contains(FName(*Fact->AsString())));
        TestTrue(TEXT("All participants and camp/source transforms restore the actual saved boundary"),Hero->GetActorTransform().Equals(Expected.Player,.001)
            && Brother->GetActorTransform().Equals(Expected.Companion,.001) && Camp->GetActorTransform().Equals(Expected.Camp,.001)
            && Stock->GetOwner()->GetActorTransform().Equals(Expected.Source,.001) && Controller->GetControlRotation().Equals(Expected.View,.001));
        const auto CheckSurvival=[&](const FHearthwardSurvivalState& A,const FHearthwardSurvivalState& B)
        {
            TestTrue(TEXT("Saved life, calendar Due and active treatment timers restore without advancement"),A.Life==B.Life && A.SevereDue==B.SevereDue
                && A.DownRemaining==B.DownRemaining && A.DrowningRemaining==B.DrowningRemaining && A.HotRemaining==B.HotRemaining && A.HotRate==B.HotRate
                && A.RecoveryDelay==B.RecoveryDelay && A.SafeSeconds==B.SafeSeconds && A.Medicine==B.Medicine && A.MedicineRemaining==B.MedicineRemaining
                && A.AutomaticMedicine==B.AutomaticMedicine && A.AutoPermissions.Num()==B.AutoPermissions.Num());
            for(FName Permission:B.AutoPermissions)TestTrue(TEXT("Saved treatment permission retains its identity"),A.AutoPermissions.Contains(Permission));
        };
        CheckSurvival(Hero->FindComponentByClass<UHearthwardSurvivalComponent>()->State,Expected.PlayerSurvival);
        CheckSurvival(Brother->FindComponentByClass<UHearthwardSurvivalComponent>()->State,Expected.BrotherSurvival);
        TestTrue(TEXT("Actual health, hunger and stamina have no victory-load gain"),double(Gameplay->Health)==SavedGameplay->GetNumberField(TEXT("health"))
            && double(Gameplay->Hunger)==SavedGameplay->GetNumberField(TEXT("hunger")) && double(Gameplay->Stamina)==SavedGameplay->GetNumberField(TEXT("stamina"))
            && Brother->FindComponentByClass<UHearthwardSurvivalComponent>()->Health()==Expected.BrotherHealth
            && Brother->FindComponentByClass<UHearthwardSurvivalComponent>()->Hunger()==Expected.BrotherHunger
            && Brother->FindComponentByClass<UHearthwardSurvivalComponent>()->Stamina()==Expected.BrotherStamina);
        return true;
    };
    if(RestartPhase==TEXT("read"))
    {
        FString ManifestText;TSharedPtr<FJsonObject> Manifest;
        if(!TestTrue(TEXT("Read phase finds the write process's QA manifest"),FFileHelper::LoadFileToString(ManifestText,*ManifestPath)
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ManifestText),Manifest) && Manifest.IsValid()))return false;
        FGuid SavedId,CampaignId,CommandId,AxeId,MemoryId;
        if(!TestTrue(TEXT("Manifest identifies the same isolated pool and real saved GUIDs"),Manifest->GetStringField(TEXT("pool"))==PoolId.ToString(EGuidFormats::Digits)
            && FGuid::Parse(Manifest->GetStringField(TEXT("save_id")),SavedId) && FGuid::Parse(Manifest->GetStringField(TEXT("campaign_id")),CampaignId)
            && FGuid::Parse(Manifest->GetStringField(TEXT("command_id")),CommandId) && FGuid::Parse(Manifest->GetStringField(TEXT("axe_id")),AxeId)
            && FGuid::Parse(Manifest->GetStringField(TEXT("memory_id")),MemoryId)))return false;
        if(!TestTrue(TEXT("Read runs in a different process from write"),uint32(Manifest->GetNumberField(TEXT("writer_pid")))!=FPlatformProcess::GetCurrentProcessId()))return false;
        const auto Points=Save->GetPoints();
        TestEqual(TEXT("Fresh process sees the write process's exact pool node count"),Points.Num(),Manifest->GetIntegerField(TEXT("point_count")));
        const auto* Found=Points.FindByPredicate([&](const auto& P){return P.SaveId==SavedId;});
        if(!TestNotNull(TEXT("Manifest selects the actual manual node read from disk"),Found))return false;
        const auto Point=*Found;const auto& Expected=Point.World;
        if(RestartTwoCamps)
        {
            FHearthwardCampaignState SavedCampaign;FHearthwardCampState SavedCamp;FGuid RewardId;TSharedPtr<FJsonObject> SavedGameplay;
            if(!TestTrue(TEXT("Two-camp read uses the write process's actual continuous campaign and unique reward identities"),
                Manifest->GetBoolField(TEXT("two_camps")) && Expected.Campaign==Manifest->GetStringField(TEXT("campaign_state"))
                && FHearthwardCampaignState::Parse(Expected.Campaign,SavedCampaign) && SavedCampaign.Victory && SavedCampaign.ReadyForVictory()
                && FHearthwardCampState::Parse(Expected.CampEconomy,SavedCamp) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Expected.Gameplay),SavedGameplay)
                && SavedGameplay->GetNumberField(TEXT("experience"))==Manifest->GetNumberField(TEXT("campaign_experience"))
                && FGuid::Parse(Manifest->GetStringField(TEXT("hometown_reward_id")),RewardId)
                && Expected.StorageItems.Instances.ContainsByPredicate([&](const auto& I){return I.Id==RewardId && I.Definition==TEXT("hearth_blade")
                    && I.UniqueClaim==TEXT("hometown_hearth_blade");})))return false;
            const auto& GiftIds=Manifest->GetArrayField(TEXT("hometown_gift_ids"));TSet<FGuid> Distinct;
            for(const auto& Value:GiftIds)
            {
                FGuid Id;
                if(!TestTrue(TEXT("Manifest gift identity refers to an actual saved unpaid hometown facility"),FGuid::Parse(Value->AsString(),Id)
                    && SavedCamp.Facilities.ContainsByPredicate([&](const auto& F){return F.Id==Id && F.Camp==TEXT("hometown") && F.Level==1 && F.Paid.IsEmpty();})))return false;
                Distinct.Add(Id);
            }
            if(!TestTrue(TEXT("Read manifest preserves all four different original gift GUIDs"),GiftIds.Num()==4 && Distinct.Num()==4))return false;
        }
        TestTrue(TEXT("Persisted campaign and command match write-process GUIDs"),Point.CampaignId==CampaignId && Expected.CommandId==CommandId && Expected.CommandActive);
        TestEqual(TEXT("Persisted A matches write-process boundary"),Expected.ActiveSeconds,Manifest->GetNumberField(TEXT("active_seconds")));
        TestEqual(TEXT("Persisted W matches write-process boundary"),Expected.CalendarMinutes,Manifest->GetNumberField(TEXT("calendar_minutes")));
        TestEqual(TEXT("Persisted origin day matches write process"),Expected.InitialDay,int64(Manifest->GetNumberField(TEXT("initial_day"))));
        TestEqual(TEXT("Persisted origin minute matches write process"),Expected.InitialMinute,Manifest->GetNumberField(TEXT("initial_minute")));
        TestEqual(TEXT("Persisted NPC revision matches write process"),Expected.NPCMemory.Revision,int64(Manifest->GetNumberField(TEXT("npc_revision"))));
        TestEqual(TEXT("Persisted NPC events match write-process count"),Expected.NPCMemory.Events.Num(),Manifest->GetIntegerField(TEXT("npc_events")));
        TestEqual(TEXT("Persisted NPC coverage matches write-process count"),Expected.NPCMemory.CommandCoverage.Num(),Manifest->GetIntegerField(TEXT("npc_coverage")));
        const auto& EventIds=Manifest->GetArrayField(TEXT("event_ids"));
        TestEqual(TEXT("Persisted event GUID count matches write process"),Expected.NPCMemory.Events.Num(),EventIds.Num());
        for(int32 I=0;I<FMath::Min(Expected.NPCMemory.Events.Num(),EventIds.Num());++I)
            TestEqual(TEXT("Persisted event retains write-process identity"),Expected.NPCMemory.Events[I].Id.ToString(),EventIds[I]->AsString());
        TestTrue(TEXT("Persisted NPC conversation clock matches write process"),Expected.NPCMemory.ConversationClockStarted==Manifest->GetBoolField(TEXT("conversation_started"))
            && Expected.NPCMemory.ConversationClockAwaitingFirstMeeting==Manifest->GetBoolField(TEXT("awaiting_meeting"))
            && Expected.NPCMemory.LastConversationCalendar==Manifest->GetNumberField(TEXT("last_contact")));
        TestEqual(TEXT("Persisted knowledge revision matches write process"),Expected.KnowledgeRevision,int64(Manifest->GetNumberField(TEXT("knowledge_revision"))));
        TestEqual(TEXT("Persisted exact exchange matches write process"),FString::Join(Expected.Knowledge,TEXT("\n")),Manifest->GetStringField(TEXT("knowledge")));
        TestEqual(TEXT("Persisted camp snapshot matches the actual write-process state"),Expected.CampEconomy,Manifest->GetStringField(TEXT("camp_economy")));
        TestEqual(TEXT("Persisted nature snapshot matches the actual write-process state"),Expected.Nature,Manifest->GetStringField(TEXT("nature")));
        FGuid WorkbenchId,TreasureId;const FName RegionId(*Manifest->GetStringField(TEXT("production_region")));
        FHearthwardCampState ExpectedCamp;FHearthwardNatureState ExpectedNature;
        if(!TestTrue(TEXT("Write manifest identifies real saved workbench, queue and treasure GUIDs"),
            FGuid::Parse(Manifest->GetStringField(TEXT("workbench_id")),WorkbenchId)
            && FGuid::Parse(Manifest->GetStringField(TEXT("treasure_id")),TreasureId)
            && FHearthwardCampState::Parse(Expected.CampEconomy,ExpectedCamp) && FHearthwardNatureState::Parse(Expected.Nature,ExpectedNature)))return false;
        const auto* ExpectedRegion=ExpectedCamp.Regions.FindByPredicate([&](const auto& R){return R.Id==RegionId && R.Facility==WorkbenchId;});
        const auto* ExpectedFacility=ExpectedCamp.Facilities.FindByPredicate([&](const auto& F){return F.Id==WorkbenchId;});
        const auto* ExpectedTreasure=ExpectedNature.Points.FindByPredicate([&](const auto& P){return P.Id==TreasureId;});
        if(!TestTrue(TEXT("Disk node contains a genuinely invested partial batch and unclaimed instance reward"),ExpectedRegion && ExpectedFacility && ExpectedTreasure
            && ExpectedRegion->Batch.Active && ExpectedRegion->Batch.Work>0 && ExpectedRegion->Batch.Work<ExpectedRegion->Batch.Required
            && ExpectedRegion->Batch.Inputs.FindRef(TEXT("wood"))==2 && ExpectedRegion->Batch.Outputs.FindRef(TEXT("rope"))==1
            && ExpectedRegion->Workers==TArray<int32>{0} && ExpectedTreasure->Kind==TEXT("treasure")
            && !ExpectedTreasure->Pending.Instances.IsEmpty() && !ExpectedNature.Opened.Contains(ExpectedTreasure->Definition)))return false;
        const FHearthwardWildSlot* ExpectedSlot=nullptr;const FHearthwardAnimal* ExpectedAnimal=nullptr;
        const FHearthwardAnimal* ExpectedFirstCorpse=nullptr;const FHearthwardCampSource* ExpectedSource=nullptr;
        if(RestartEcology)
        {
            EcologySlot=FName(*Manifest->GetStringField(TEXT("ecology_slot")));EcologySource=Manifest->GetStringField(TEXT("ecology_source"));
            if(!TestTrue(TEXT("Ecology read requires actual paid-facility and animal identities from write"),Manifest->GetBoolField(TEXT("ecology"))
                && FGuid::Parse(Manifest->GetStringField(TEXT("ecology_fire")),EcologyFire)
                && FGuid::Parse(Manifest->GetStringField(TEXT("ecology_animal")),EcologyAnimal)
                && FGuid::Parse(Manifest->GetStringField(TEXT("ecology_first_corpse")),EcologyFirstCorpse)))return false;
            ExpectedSlot=ExpectedNature.Slots.FindByPredicate([&](const auto& S){return S.Id==EcologySlot;});
            ExpectedAnimal=ExpectedNature.Animals.FindByPredicate([&](const auto& A){return A.Id==EcologyAnimal;});
            ExpectedFirstCorpse=ExpectedNature.Animals.FindByPredicate([&](const auto& A){return A.Id==EcologyFirstCorpse;});
            ExpectedSource=ExpectedCamp.Sources.FindByPredicate([&](const auto& S){return S.Id==EcologySource;});
            if(!TestTrue(TEXT("Disk node contains genuinely exhausted source and second-generation death with future Due"),
                ExpectedSlot && ExpectedAnimal && ExpectedFirstCorpse && ExpectedSource
                && ExpectedSlot->Generation==2 && ExpectedSlot->Current==EcologyAnimal && ExpectedSlot->Due>Expected.CalendarMinutes
                && ExpectedAnimal->Health==0 && ExpectedAnimal->Rewarded && !ExpectedAnimal->Loot.IsEmpty()
                && ExpectedFirstCorpse->Health==0 && ExpectedFirstCorpse->Rewarded && !ExpectedFirstCorpse->Loot.IsEmpty()
                && ExpectedSource->Remaining==0 && ExpectedSource->Due>Expected.CalendarMinutes && !ExpectedSource->Blocked))return false;
        }
        const auto CheckInventory=[&](const TCHAR* Label,const FHearthwardInventorySnapshot& Actual,const FHearthwardInventorySnapshot& Saved)
        {
            TestTrue(Label,Actual.Version==Saved.Version && Actual.BackpackRank==Saved.BackpackRank
                && Actual.Stacks.OrderIndependentCompareEqual(Saved.Stacks) && Actual.Equipped.OrderIndependentCompareEqual(Saved.Equipped)
                && Actual.Instances.Num()==Saved.Instances.Num());
            for(const auto& Item:Saved.Instances)
            {
                const auto* Loaded=Actual.Instances.FindByPredicate([&](const auto& I){return I.Id==Item.Id;});
                TestTrue(Label,Loaded && Loaded->Definition==Item.Definition && Loaded->Durability==Item.Durability && Loaded->UniqueClaim==Item.UniqueClaim);
            }
        };
        if(!TestTrue(TEXT("Read fixture starts with different inventories"),PlayerBag->TryAdd(TEXT("wood"),2)==EHearthwardInventoryResult::Success
            && Store->Adjust({},{{FName(TEXT("wood")),4}}) && Stock->TryRemove(TEXT("wood"),1)==EHearthwardInventoryResult::Success
            && Brother->Bag->TryAdd(TEXT("wood"),1)==EHearthwardInventoryResult::Success && !PlayerBag->FirstInstance(TEXT("axe")).IsValid()))return false;
        for(int32 Pass=0;Pass<2;++Pass)
        {
            const FGuid PreviousEpoch=Store->GetTimelineEpoch();
            if(!TestTrue(FString::Printf(TEXT("Cross-process LoadPoint pass %d: "),Pass+1)+Save->GetStatus(),Save->LoadPoint(SavedId)))return false;
            if(RestartTwoCamps && !CheckTwoCamps(Expected))return false;
            TestTrue(TEXT("Each actual load replaces the live timeline"),Store->GetTimelineEpoch()!=PreviousEpoch);
            int32 RestoredOpponents=0;const auto* Combat=Hero->FindComponentByClass<UHearthwardCombatComponent>();
            for(TActorIterator<AActor> It(World);It;++It)
                if(const auto* Target=It->FindComponentByClass<UHearthwardCombatTargetComponent>();Target && Target->PrototypeEncounter)
                {
                    const auto* SavedTarget=Combat->State.Targets.FindByPredicate([&](const auto& T){return T.Id==Target->Id;});
                    TestTrue(TEXT("Normal load restores living prototype opponents at saved supported scene positions"),SavedTarget && Target->Alive()
                        && Target->Health==SavedTarget->Health && It->GetActorLocation()==SavedTarget->Position
                        && FVector::Dist2D(It->GetActorLocation(),Economy->State.Camps[0].Position)>Economy->State.Radius());
                    ++RestoredOpponents;
                }
            TestEqual(TEXT("Repeated load retains all three living prototype encounter identities"),RestoredOpponents,3);
            TestTrue(TEXT("Loaded real axe retains its exact write-process instance GUID"),PlayerBag->FirstInstance(TEXT("axe"))==AxeId);
            TestEqual(TEXT("Repeated load preserves player wood without gain"),PlayerBag->GetItemCount(TEXT("wood")),Manifest->GetIntegerField(TEXT("player_wood")));
            TestEqual(TEXT("Repeated load preserves shared wood without gain"),Store->GetItemCount(TEXT("wood")),Manifest->GetIntegerField(TEXT("shared_wood")));
            TestEqual(TEXT("Repeated load preserves Brother wood without gain"),Brother->Bag->GetItemCount(TEXT("wood")),Manifest->GetIntegerField(TEXT("brother_wood")));
            TestEqual(TEXT("Repeated load preserves source wood without gain"),Stock->GetItemCount(TEXT("wood")),Manifest->GetIntegerField(TEXT("source_wood")));
            TestTrue(TEXT("Repeated load preserves saved campaign and command"),Save->GetCampaignId()==CampaignId && Brother->GetCommandId()==CommandId);
            TestEqual(TEXT("Requested work is restored"),Brother->GetRequested(),Manifest->GetIntegerField(TEXT("requested")));
            TestEqual(TEXT("Acquired cargo is restored without replay"),Brother->GetAcquired(),Manifest->GetIntegerField(TEXT("acquired")));
            TestEqual(TEXT("Carried cargo is restored without replay"),Brother->GetCarried(),Manifest->GetIntegerField(TEXT("carried")));
            TestEqual(TEXT("Delivered work is restored without replay"),Brother->GetDelivered(),Manifest->GetIntegerField(TEXT("delivered")));
            const auto Time=Clock->GetSnapshot();
            TestEqual(TEXT("Repeated load freezes A at saved boundary"),Time.ActivePlaySeconds,Expected.ActiveSeconds);
            TestEqual(TEXT("Repeated load freezes W at saved boundary"),Time.ElapsedCalendarMinutes,Expected.CalendarMinutes);
            TestEqual(TEXT("Restored clock derives display day from saved origin"),Time.DisplayDay,Expected.InitialDay+FMath::FloorToInt64((Expected.InitialMinute+Expected.CalendarMinutes)/1440.0));
            TestEqual(TEXT("Restored clock derives minute from saved origin"),Time.MinuteOfDay,FMath::Fmod(Expected.InitialMinute+Expected.CalendarMinutes,1440.0));
            const auto& Memory=AI->GetMemorySnapshot();
            TestEqual(TEXT("Repeated load cannot append NPC records"),Memory.Records.Num(),1);
            const auto* Record=Memory.Records.FindByPredicate([&](const auto& R){return R.Id==MemoryId;});
            TestTrue(TEXT("Exact original agreement and campaign survive restart"),Record && Record->Kind==TEXT("agreement")
                && Record->Text==TEXT("Task071 restart agreement") && Record->Campaign==CampaignId && !Record->Revoked && Record->RecordedAt==Expected.ActiveSeconds);
            TestEqual(TEXT("Repeated load cannot change NPC revision"),Memory.Revision,Expected.NPCMemory.Revision);
            TestEqual(TEXT("Repeated load cannot append NPC events"),Memory.Events.Num(),Expected.NPCMemory.Events.Num());
            for(int32 I=0;I<FMath::Min(Memory.Events.Num(),Expected.NPCMemory.Events.Num());++I)
            {
                const auto& E=Memory.Events[I];const auto& S=Expected.NPCMemory.Events[I];
                TestTrue(TEXT("NPC event identity and payload are restored without replay"),E.Id==S.Id && E.Command==S.Command && E.Campaign==S.Campaign
                    && E.Kind==S.Kind && E.Item==S.Item && E.Count==S.Count && E.At==S.At && E.Reason==S.Reason);
            }
            TestEqual(TEXT("Repeated load preserves command coverage"),Memory.CommandCoverage.Num(),Expected.NPCMemory.CommandCoverage.Num());
            for(int32 I=0;I<FMath::Min(Memory.CommandCoverage.Num(),Expected.NPCMemory.CommandCoverage.Num());++I)
            {
                const auto& C=Memory.CommandCoverage[I];const auto& S=Expected.NPCMemory.CommandCoverage[I];
                TestTrue(TEXT("Coverage keeps the original command and completeness"),C.Command==S.Command && C.Coverage==S.Coverage && C.Active==S.Active);
            }
            TestTrue(TEXT("Conversation clock flags and last contact survive restart"),Memory.ConversationClockStarted==Expected.NPCMemory.ConversationClockStarted
                && Memory.ConversationClockAwaitingFirstMeeting==Expected.NPCMemory.ConversationClockAwaitingFirstMeeting && Memory.LastConversationCalendar==Expected.NPCMemory.LastConversationCalendar);
            TestEqual(TEXT("Repeated load restores exact exchanges"),Save->GetKnowledge(),FString::Join(Expected.Knowledge,TEXT("\n")));
            const auto* Region=Economy->State.Regions.FindByPredicate([&](const auto& R){return R.Id==RegionId;});
            const auto* Facility=Economy->State.Facilities.FindByPredicate([&](const auto& F){return F.Id==WorkbenchId;});
            if(!TestTrue(TEXT("Actual load rebuilds the original workbench actor and invested region"),Region && Facility && Building->ResolveFacility(WorkbenchId)))return false;
            TestTrue(TEXT("Repeated load preserves queue, worker and paid facility identity"),Region->Facility==ExpectedRegion->Facility
                && Region->Camp==ExpectedRegion->Camp && Region->Job==ExpectedRegion->Job && Region->Workers==ExpectedRegion->Workers
                && Region->Enabled==ExpectedRegion->Enabled && Region->Completed==ExpectedRegion->Completed && Region->BatchStopAt==ExpectedRegion->BatchStopAt
                && Facility->Kind==ExpectedFacility->Kind && Facility->Level==ExpectedFacility->Level && Facility->Camp==ExpectedFacility->Camp
                && Facility->Paid.OrderIndependentCompareEqual(ExpectedFacility->Paid));
            TestTrue(TEXT("Repeated load retains batch input/output without new debit or output credit"),Region->Batch.Active==ExpectedRegion->Batch.Active
                && Region->Batch.ToRations==ExpectedRegion->Batch.ToRations && Region->Batch.Work==ExpectedRegion->Batch.Work && Region->Batch.Required==ExpectedRegion->Batch.Required
                && Region->Batch.Inputs.OrderIndependentCompareEqual(ExpectedRegion->Batch.Inputs) && Region->Batch.Outputs.OrderIndependentCompareEqual(ExpectedRegion->Batch.Outputs));
            const auto* Treasure=Nature->State.Points.FindByPredicate([&](const auto& P){return P.Id==TreasureId;});
            if(!TestTrue(TEXT("Actual load rebuilds the original unclaimed treasure identity"),Treasure && Nature->Actor(TreasureId)))return false;
            TestTrue(TEXT("Repeated load retains map and unclaimed reward ledger"),Treasure->Key==ExpectedTreasure->Key && Treasure->Kind==ExpectedTreasure->Kind
                && Treasure->Definition==ExpectedTreasure->Definition && Treasure->Position==ExpectedTreasure->Position
                && Nature->State.Maps.Contains(Treasure->Definition) && Nature->State.Rewards.Contains(Treasure->Definition) && !Nature->State.Opened.Contains(Treasure->Definition));
            CheckInventory(TEXT("Every unclaimed stack and instance GUID survives without claim replay"),Treasure->Pending,ExpectedTreasure->Pending);
            if(RestartEcology)
            {
                const auto* Slot=Nature->State.Slots.FindByPredicate([&](const auto& S){return S.Id==EcologySlot;});
                const auto* Animal=Nature->State.Animals.FindByPredicate([&](const auto& A){return A.Id==EcologyAnimal;});
                const auto* First=Nature->State.Animals.FindByPredicate([&](const auto& A){return A.Id==EcologyFirstCorpse;});
                const auto* Source=Economy->Source(EcologySource);const auto* Fire=Economy->State.Facilities.FindByPredicate([&](const auto& F){return F.Id==EcologyFire;});
                if(!TestTrue(TEXT("Load restores actual ecology identities and paid usable fire"),Slot && Animal && First && Source
                    && Fire && Fire->Kind==TEXT("campfire") && Fire->Paid.FindRef(TEXT("wood"))==4 && Fire->Paid.FindRef(TEXT("stone"))==4
                    && Building->ResolveFacility(EcologyFire) && Building->CanUseFacility(EcologyFire) && Nature->Actor(EcologyAnimal) && Nature->Actor(EcologyFirstCorpse)))return false;
                TestTrue(TEXT("Repeated load cannot advance animal generation or rebase its Due"),Slot->Current==ExpectedSlot->Current
                    && Slot->Generation==ExpectedSlot->Generation && Slot->Due==ExpectedSlot->Due && Slot->Position==ExpectedSlot->Position && Slot->Definition==ExpectedSlot->Definition);
                TestTrue(TEXT("Repeated load preserves both unclaimed corpses without reward replay"),Animal->Health==ExpectedAnimal->Health
                    && Animal->Rewarded==ExpectedAnimal->Rewarded && Animal->Loot.OrderIndependentCompareEqual(ExpectedAnimal->Loot)
                    && First->Health==ExpectedFirstCorpse->Health && First->Rewarded==ExpectedFirstCorpse->Rewarded && First->Loot.OrderIndependentCompareEqual(ExpectedFirstCorpse->Loot));
                TestTrue(TEXT("Repeated load preserves exhausted source and exact remaining calendar Due"),Source->Remaining==ExpectedSource->Remaining
                    && Source->Capacity==ExpectedSource->Capacity && Source->RefreshMinutes==ExpectedSource->RefreshMinutes && Source->Due==ExpectedSource->Due
                    && Source->Blocked==ExpectedSource->Blocked && Source->Camp==ExpectedSource->Camp && Source->Position==ExpectedSource->Position);
                TestEqual(TEXT("Repeated load grants no duplicate hunting experience"),double(Gameplay->Experience),Manifest->GetNumberField(TEXT("ecology_experience")));
                TestTrue(TEXT("All restored ecology domains share the actual saved calendar"),Economy->State.Calendar==Expected.CalendarMinutes && Nature->State.Calendar==Expected.CalendarMinutes);
            }
            CheckInventory(TEXT("Player full inventory has zero gain from repeated mature load"),PlayerBag->Snapshot(),Expected.PlayerItems);
            CheckInventory(TEXT("Brother full inventory has zero gain from repeated mature load"),Brother->Bag->Snapshot(),Expected.BrotherItems);
            CheckInventory(TEXT("Shared full inventory has zero gain from repeated mature load"),Store->InventorySnapshot(),Expected.StorageItems);
        }
        if(!TestTrue(TEXT("Restored state still captures through public SavePoint"),Save->SavePoint(true)))return false;
        const auto After=Save->GetPoints().Last().World;
        TestTrue(TEXT("Repeated loads cannot create NPC effect operations"),After.NPCOperations==Expected.NPCOperations);
        TestEqual(TEXT("Repeated loads cannot create NPC effect receipts"),After.NPCReceipts.Num(),Expected.NPCReceipts.Num());
        TestTrue(TEXT("Resave retains exact clock origin and knowledge revision"),After.InitialDay==Expected.InitialDay && After.InitialMinute==Expected.InitialMinute && After.KnowledgeRevision==Expected.KnowledgeRevision);
        TestEqual(TEXT("Resave preserves invested camp state without batch settlement"),After.CampEconomy,Expected.CampEconomy);
        TestEqual(TEXT("Resave preserves pending nature claims without reward settlement"),After.Nature,Expected.Nature);
        if(RestartTwoCamps)
        {
            TestEqual(TEXT("Resave preserves exact continuous campaign without replaying flags or victory gifts"),After.Campaign,Expected.Campaign);
            if(!CheckTwoCamps(Expected))return false;
        }
        if(RestartEcology)
        {
            if(!TestTrue(TEXT("Only after noGain checks does public cancellation release the active command for paid waiting"),Brother->Cancel(Hero)
                && !Brother->EquipmentBusy() && !Gameplay->InCombat()))return false;
            const double BeforeExperience=Gameplay->Experience;const int32 BeforeRope=Store->GetItemCount(TEXT("rope"));
            const int32 BeforeCompleted=Economy->State.Regions.FindByPredicate([&](const auto& R){return R.Id==RegionId;})->Completed;
            const double LastDue=FMath::Max(ExpectedSlot->Due,ExpectedSource->Due);
            if(!TestTrue(TEXT("The saved future Due fits within one two-day calendar preparation"),LastDue>Clock->GetSnapshot().ElapsedCalendarMinutes
                && LastDue-Clock->GetSnapshot().ElapsedCalendarMinutes<=2880))return false;
            for(int32 I=0;I<6;++I)if(!WaitEcology(480))return false;
            const auto* Slot=Nature->State.Slots.FindByPredicate([&](const auto& S){return S.Id==EcologySlot;});
            const auto* Source=Economy->Source(EcologySource);const auto* Region=Economy->State.Regions.FindByPredicate([&](const auto& R){return R.Id==RegionId;});
            if(!TestTrue(TEXT("Paid calendar crossing materializes exactly one live third generation and refreshes source once"),Slot && Source && Region
                && Slot->Generation==3 && Slot->Current!=EcologyAnimal && Slot->Due==-1 && Nature->Actor(Slot->Current)
                && Source->Remaining==Source->Capacity && Source->Due==-1))return false;
            const FGuid ThirdId=Slot->Current;const auto* Third=Nature->State.Animals.FindByPredicate([&](const auto& A){return A.Id==ThirdId;});
            TestTrue(TEXT("The actual replacement is alive and has no pre-earned loot"),Third && Third->Health>0 && !Third->Rewarded && Third->Loot.IsEmpty());
            TestTrue(TEXT("Only the saved invested rope batch settles, without another wood debit"),Region->Completed==BeforeCompleted+1
                && !Region->Batch.Active && Store->GetItemCount(TEXT("rope"))==BeforeRope+1 && Store->GetItemCount(TEXT("wood"))==0);
            TestEqual(TEXT("Ecology refresh grants no duplicate hunting experience"),double(Gameplay->Experience),BeforeExperience);
            if(!WaitEcology(60))return false;
            Slot=Nature->State.Slots.FindByPredicate([&](const auto& S){return S.Id==EcologySlot;});Source=Economy->Source(EcologySource);
            Region=Economy->State.Regions.FindByPredicate([&](const auto& R){return R.Id==RegionId;});
            TestTrue(TEXT("Later calendar boundary cannot accumulate a new living generation, resource stock or rope output"),
                Slot && Source && Region && Slot->Generation==3 && Slot->Current==ThirdId && Slot->Due==-1
                && Source->Remaining==Source->Capacity && Source->Due==-1 && Region->Completed==BeforeCompleted+1
                && Store->GetItemCount(TEXT("rope"))==BeforeRope+1 && Store->GetItemCount(TEXT("wood"))==0);
            for(FGuid Id:{EcologyFirstCorpse,EcologyAnimal})
            {
                const auto* Corpse=Nature->State.Animals.FindByPredicate([&](const auto& A){return A.Id==Id;});
                const auto* Saved=ExpectedNature.Animals.FindByPredicate([&](const auto& A){return A.Id==Id;});
                TestTrue(TEXT("Paid time does not replay or erase unclaimed earlier corpse loot"),Corpse && Saved && Corpse->Health==0
                    && Corpse->Rewarded && Corpse->Loot.OrderIndependentCompareEqual(Saved->Loot));
            }
            TestEqual(TEXT("Subsequent paid waiting grants no ecology reward"),double(Gameplay->Experience),BeforeExperience);
            AddInfo(FString::Printf(TEXT("TASK071 ecology READ slot=%s generation=3 source=%s, exact single refresh and rope completion"),*EcologySlot.ToString(),*EcologySource));
        }
        AddInfo(FString::Printf(TEXT("TASK071 restart READ pid=%u writer=%u save=%s, two actual LoadPoint passes"),FPlatformProcess::GetCurrentProcessId(),uint32(Manifest->GetNumberField(TEXT("writer_pid"))),*SavedId.ToString()));
        return true;
    }
    if(!TestTrue(TEXT("Runner pool is fresh before this actual-world test"),Save->GetPoints().IsEmpty()))return false;
    const bool Started=Save->StartNewProgress();
    if(!TestTrue(FString(TEXT("Normal new progress writes and restores the prepared world: "))+Save->GetStatus(),Started))return false;
    if(RestartPhase==TEXT("write"))
    {
        if(!TestTrue(TEXT("Write phase uses public inventory operations and a real instance"),PlayerBag->TryAdd(TEXT("axe"),1)==EHearthwardInventoryResult::Success
            && PlayerBag->FirstInstance(TEXT("axe")).IsValid() && Store->Adjust({},{{FName(TEXT("wood")),6}})
            && Brother->Bag->TryAdd(TEXT("wood"),2)==EHearthwardInventoryResult::Success))return false;
        // Existing three personal and six shared wood plus this supply fund workbench72 and one rope input2.
        if(!TestTrue(TEXT("Fixture grants exact real construction and single-batch materials"),Store->Adjust({},{{FName(TEXT("wood")),65}})))return false;
        Controller->SetControlRotation(FRotator::ZeroRotator);
        if(!TestTrue(TEXT("Public selection accepts the real first-tier workbench"),Building->SelectBuilding(TEXT("workbench"))))return false;
        Building->TickComponent(.01f,LEVELTICK_All,nullptr);
        if(!TestTrue(FString(TEXT("Actual terrain and occupancy accept the workbench preview: "))+Building->Feedback,Building->ValidPlacement)
            || !TestTrue(FString(TEXT("Public ConfirmPlacement starts real five-second construction: "))+Building->Feedback,Building->ConfirmPlacement()))return false;
        const auto BeforeBuild=Clock->GetSnapshot();Clock->Tick(5.f);Timer->TickComponent(5.f,LEVELTICK_All,nullptr);
        if(!TestTrue(TEXT("Real timer completes one paid workbench without movement or state injection"),!Building->IsBuilding() && !Building->IsPlacing()
            && Building->BuildingCount()==1 && Economy->State.Facilities.Num()==1 && Economy->State.Facilities[0].Kind==TEXT("workbench")
            && Economy->State.Facilities[0].Paid.FindRef(TEXT("wood"))==72 && PlayerBag->GetItemCount(TEXT("wood"))==0 && Store->GetItemCount(TEXT("wood"))==2
            && FMath::IsNearlyEqual(Clock->GetSnapshot().ActivePlaySeconds,BeforeBuild.ActivePlaySeconds+5,1.e-6)))return false;
        if(RestartTwoCamps)
        {
            auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();auto* Combat=Hero->FindComponentByClass<UHearthwardCombatComponent>();
            auto* Loading=Instance->GetSubsystem<UHearthwardLoadingSubsystem>();
            const auto PlaceParticipant=[&](ACharacter* Character,FVector At)
            {
                FVector Ground;
                if(!TestTrue(TEXT("Explicit campaign scene placement has actual tagged terrain"),Campaign->Ground(At,Ground)))return false;
                if(!TestTrue(TEXT("Public participant placement accepts the actual capsule"),Character->SetActorLocation(
                    Ground+FVector(0,0,Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3),false,nullptr,ETeleportType::TeleportPhysics)))return false;
                Character->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
                return TestTrue(TEXT("Campaign participant stands naturally on real queried support"),Character->GetCharacterMovement()->IsMovingOnGround());
            };
            const auto CompleteScriptedTravel=[&]()
            {
                // Native worker RunTest executes before core ticker; dispatch the real loading delegate and its wall-clock fade.
                const double Deadline=FPlatformTime::Seconds()+8;
                while((Campaign->Busy() || Loading->IsLoading()) && FPlatformTime::Seconds()<Deadline)
                {
                    Campaign->Tick(.025f);FTSTicker::GetCoreTicker().Tick(.025f);Clock->Tick(.025f);
                    Hero->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
                    Brother->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
                    FPlatformProcess::Sleep(.005f);
                }
                if(!TestTrue(FString(TEXT("Real scripted travel, intro and loading finish: "))+Campaign->Feedback,
                    !Campaign->Busy() && !Loading->IsLoading() && !Clock->Suspended()))return false;
                for(int32 I=0;I<80 && (!Hero->GetCharacterMovement()->IsMovingOnGround() || !Brother->GetCharacterMovement()->IsMovingOnGround());++I)
                {
                    Hero->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
                    Brother->GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr);
                }
                return TestTrue(TEXT("Normal campaign lifecycle ends with both participants safely grounded"),
                    Hero->FindComponentByClass<UHearthwardSurvivalComponent>()->SafeToSave() && Brother->FindComponentByClass<UHearthwardSurvivalComponent>()->SafeToSave());
            };
            if(!TestTrue(TEXT("Normal campaign setup uses the actual owned axe switch"),Gameplay->EquipInstance(PlayerBag->FirstInstance(TEXT("axe")))))return false;
            Clock->Tick(.4f);Combat->TickComponent(.4f,LEVELTICK_All,nullptr);
            if(!TestTrue(TEXT("Real completed switch exposes the registered axe's attack power"),!Combat->Busy() && Gameplay->AttackPower()>0
                && PlayerBag->EquippedInstance(TEXT("weapon"))==PlayerBag->FirstInstance(TEXT("axe"))))return false;
            Campaign->Start();
            if(!TestTrue(TEXT("Public Start creates the normal prologue registry and pending reinforcements"),Campaign->State.Phase==TEXT("prologue")
                && !Campaign->State.Legacy && !Campaign->State.Victory && Campaign->State.Total()==80 && Campaign->State.Flags.IsEmpty()
                && Campaign->State.Reinforcements.FindRef(TEXT("workshops"))==TEXT("pending") && Campaign->State.Reinforcements.FindRef(TEXT("assembly"))==TEXT("pending")))return false;
            if(!CompleteScriptedTravel())return false;
            if(!TestTrue(TEXT("Public relic interaction grants the actual amulet once"),Campaign->Interact() && Campaign->State.Facts.Contains(TEXT("relic"))
                && PlayerBag->GetItemCount(TEXT("amulet"))==1 && Gameplay->RewardFacts.Contains(TEXT("campaign_start_amulet"))))return false;
            if(!TestTrue(TEXT("Normal public companion directive produces the prologue order"),Gameplay->ApplyCompanionDirective(Hero,TEXT("hold"))
                && Campaign->State.Facts.Contains(TEXT("prologue_order"))))return false;
            const FVector Exit=Campaign->Position(TEXT("prologue_exit"));
            if(!PlaceParticipant(Hero,Exit) || !PlaceParticipant(Brother,Exit+FVector(0,180,0)))return false;
            Campaign->Tick(.5f);
            if(!TestTrue(TEXT("Actual back-alley interaction starts the real escape travel"),Campaign->Interact() && Campaign->IsTraveling()))return false;
            if(!CompleteScriptedTravel())return false;
            if(!TestTrue(TEXT("Actual continuous prologue ends at the previously established first camp"),Campaign->State.Phase==TEXT("occupied")
                && Campaign->State.Facts.Contains(TEXT("prologue_placed")) && Campaign->State.Facts.Contains(TEXT("prologue_intro"))
                && Campaign->State.Facts.Contains(TEXT("prologue_complete")) && Economy->State.CampAt(Hero->GetActorLocation())==TEXT("camp")
                && Economy->State.CampAt(Brother->GetActorLocation())==TEXT("camp")))return false;
            if(!PlaceParticipant(Hero,Campaign->Position(TEXT("home_entry"))))return false;
            Campaign->Tick(.5f);TArray<FName> Registered;
            for(const auto& E:Campaign->State.Enemies)if(E.Group==TEXT("base"))Registered.Add(E.Id);
            if(!TestEqual(TEXT("Actual occupied registry supplies eighty different base identities"),Registered.Num(),80))return false;
            int32 Hits=0;
            for(FName Id:Registered)
            {
                auto* Actor=Campaign->Actor(Id);auto* Target=Actor?Actor->Target.Get():nullptr;FVector Support;
                if(!TestTrue(FString::Printf(TEXT("Actual registered target %s is alive on real support"),*Id.ToString()),Target && Target->Id==Id
                    && Target->CampaignTarget && !Target->Protected && Target->Alive() && Target->Memory.Generation==1
                    && Campaign->Ground(Actor->GetActorLocation(),Support)))return false;
                for(int32 Hit=0;Hit<32 && Target->Alive();++Hit)
                {
                    const float Before=Target->Health;Combat->HitTarget(Target,Gameplay->AttackPower(),TEXT("body"),false,FGuid::NewGuid(),Hero);++Hits;
                    if(!TestTrue(TEXT("Each real attack transaction lowers the registered target's actual health"),Target->Health<Before))return false;
                }
                const auto* Enemy=Campaign->Enemy(Id);
                if(!TestTrue(FString::Printf(TEXT("Actual death and stable defeat reward settle for %s"),*Id.ToString()),Enemy && !Target->Alive() && Enemy->Combat.Health==0
                    && Gameplay->RewardFacts.Contains(FName(*FString::Printf(TEXT("defeat:%s:1"),*Id.ToString())))))return false;
            }
            Campaign->Tick(.5f);
            if(!TestTrue(TEXT("Normal Tick cancels untriggered reinforcements and kills alone leave flags and victory unset"),Campaign->State.Cleared()==80
                && Campaign->State.Total()==80 && Campaign->State.Flags.IsEmpty() && !Campaign->State.Victory
                && Campaign->State.Reinforcements.FindRef(TEXT("workshops"))==TEXT("cancelled") && Campaign->State.Reinforcements.FindRef(TEXT("assembly"))==TEXT("cancelled")))return false;
            for(FName Zone:{FName(TEXT("river_gate")),FName(TEXT("workshops")),FName(TEXT("dwellings")),FName(TEXT("assembly"))})
            {
                const FName Location(*(TEXT("loc_")+Zone.ToString()));
                if(!PlaceParticipant(Hero,Campaign->Position(Location)+FVector(180,0,0)))return false;
                Campaign->Tick(.5f);const int32 BeforeFlags=Campaign->State.Flags.Num();const auto Before=Clock->GetSnapshot();
                if(!TestTrue(FString(TEXT("Public interaction starts genuine five-active-second flag capture: "))+Campaign->Feedback,
                    Campaign->Interact() && Campaign->Busy() && Campaign->State.Flags.Num()==BeforeFlags))return false;
                Clock->Tick(5.f);Campaign->Tick(5.f);
                if(!TestTrue(TEXT("Production active deadline captures exactly the actual control zone"),!Campaign->Busy()
                    && Campaign->State.Flags.Contains(Zone) && Campaign->State.Flags.Num()==BeforeFlags+1
                    && Clock->GetSnapshot().ActivePlaySeconds==Before.ActivePlaySeconds+5))return false;
            }
            if(!TestTrue(FString(TEXT("Normal Tick commits the genuine second camp: "))+Economy->Feedback,Campaign->State.Victory
                && Campaign->State.Phase==TEXT("reclaimed") && Campaign->State.ReadyForVictory() && Campaign->State.Validate()
                && Economy->State.Hometown && Economy->State.Camps.Num()==2 && Gameplay->Activated.Contains(TEXT("hometown"))))return false;
            int32 Gifts=0,Beds=0,Access=0,Fires=0;TSet<FGuid> GiftIds;
            for(const auto& F:Economy->State.Facilities)if(F.Camp==TEXT("hometown"))
            {
                ++Gifts;Beds+=int32(F.Kind==TEXT("bed"));Access+=int32(F.Kind==TEXT("warehouse_access"));Fires+=int32(F.Kind==TEXT("campfire"));GiftIds.Add(F.Id);
                if(!TestTrue(TEXT("Victory creates actual distinct gift actors with no fabricated paid materials"),F.Id.IsValid() && F.Level==1 && F.Paid.IsEmpty()
                    && !F.Paused && Building->ResolveFacility(F.Id)))return false;
            }
            if(!TestTrue(TEXT("Victory grants two different beds, storage access, fire and exactly one unique blade"),Gifts==4 && GiftIds.Num()==4 && Beds==2 && Access==1 && Fires==1
                && Store->GetItemCount(TEXT("hearth_blade"))==1 && Gameplay->RewardFacts.Contains(TEXT("reward:hometown"))))return false;
            Nature->EnsureWorld();
            if(!TestTrue(TEXT("Normal ecology initialization records both real camp markers"),Nature->State.Camps.Contains(TEXT("camp")) && Nature->State.Camps.Contains(TEXT("hometown"))))return false;
            int32 OldCampSources=0,HomeCampSources=0;
            for(const auto& Source:Economy->State.Sources)
            {
                FVector Ground;const FName ExpectedCamp=Economy->State.CampAt(Source.Position);
                OldCampSources+=int32(Source.Camp==TEXT("camp"));HomeCampSources+=int32(Source.Camp==TEXT("hometown"));
                if(!TestTrue(FString::Printf(TEXT("Actual source camp=%s expected=%s position=%s has matching camp radius and terrain"),
                    *Source.Camp.ToString(),*ExpectedCamp.ToString(),*Source.Position.ToString()),Source.Camp==ExpectedCamp
                    && Campaign->Ground(Source.Position,Ground) && Ground.Equals(Source.Position,.01)))return false;
            }
            if(!TestTrue(TEXT("Both actual camps retain distinct manageable sources within their real radii"),OldCampSources>0 && HomeCampSources>0))return false;
            if(!PlaceParticipant(Hero,Campaign->Position(TEXT("hometown"))))return false;
            Economy->Advance(0,false);const FGuid Epoch=Store->GetTimelineEpoch();
            if(!TestTrue(TEXT("Public management genuinely migrates ordinary person two and keeps person one at the old camp"),
                Economy->AssignWorker(TEXT("camp_forage"),2,Epoch) && Economy->AssignWorker(TEXT("hometown_forage"),2,Epoch)
                && Economy->AssignWorker(TEXT("camp_forage"),1,Epoch) && Campaign->State.Facts.Contains(TEXT("home_work"))))return false;
            const auto* OldForage=Economy->State.Regions.FindByPredicate([](const auto& R){return R.Id==TEXT("camp_forage");});
            const auto* HomeForage=Economy->State.Regions.FindByPredicate([](const auto& R){return R.Id==TEXT("hometown_forage");});
            if(!TestTrue(TEXT("Real assignment removes the migrant from the first camp without enabling free production"),OldForage && HomeForage
                && OldForage->Workers==TArray<int32>{1} && HomeForage->Workers==TArray<int32>{2} && !OldForage->Enabled && !HomeForage->Enabled))return false;
            if(!PlaceParticipant(Hero,Economy->State.Camps[0].Position) || !PlaceParticipant(Brother,Economy->State.Camps[0].Position+FVector(0,200,0)))return false;
            Controller->SetControlRotation(FRotator::ZeroRotator);Economy->Advance(0,false);
            if(!TestTrue(TEXT("Continuous campaign returns alive to the real first camp before unchanged mature save preparation"),
                Hero->FindComponentByClass<UHearthwardSurvivalComponent>()->SafeToSave() && Brother->FindComponentByClass<UHearthwardSurvivalComponent>()->SafeToSave()
                && Brother->CanCommunicate(Hero) && !Gameplay->InCombat()))return false;
            AddInfo(FString::Printf(TEXT("TASK071 two-camp progression actual registered=80 hits=%d flags=%d gifts=%d"),Hits,Campaign->State.Flags.Num(),Gifts));
        }
        if(RestartEcology)
        {
            if(!TestTrue(TEXT("Ecology fixture supplies actual campfire cost and food for both participants"),PlayerBag->TryAdd(TEXT("wood"),4)==EHearthwardInventoryResult::Success
                && PlayerBag->TryAdd(TEXT("stone"),4)==EHearthwardInventoryResult::Success && PlayerBag->TryAdd(TEXT("wild_food"),12)==EHearthwardInventoryResult::Success
                && Brother->Bag->TryAdd(TEXT("wild_food"),12)==EHearthwardInventoryResult::Success))return false;
            Controller->SetControlRotation(FRotator(0,180,0));
            if(!TestTrue(TEXT("Public selection starts paid campfire construction on the opposite clear footprint"),Building->SelectBuilding(TEXT("campfire"))))return false;
            Building->TickComponent(.01f,LEVELTICK_All,nullptr);
            if(!TestTrue(FString(TEXT("Real campfire support and occupancy accept placement: "))+Building->Feedback,Building->ValidPlacement && Building->ConfirmPlacement()))return false;
            Clock->Tick(5.f);Timer->TickComponent(5.f,LEVELTICK_All,nullptr);
            const auto* Fire=Economy->State.Facilities.FindByPredicate([](const auto& F){return F.Kind==TEXT("campfire");});
            if(RestartTwoCamps)Fire=Economy->State.Facilities.FindByPredicate([](const auto& F){return F.Kind==TEXT("campfire") && F.Camp==TEXT("camp")
                && F.Paid.FindRef(TEXT("wood"))==4 && F.Paid.FindRef(TEXT("stone"))==4;});
            if(!TestTrue(TEXT("Real five-second construction publishes one fully paid usable campfire"),Fire && !Building->IsBuilding() && !Building->IsPlacing()
                && Fire->Paid.FindRef(TEXT("wood"))==4 && Fire->Paid.FindRef(TEXT("stone"))==4 && Building->CanUseFacility(Fire->Id)
                && PlayerBag->GetItemCount(TEXT("wood"))==0 && PlayerBag->GetItemCount(TEXT("stone"))==0 && Store->GetItemCount(TEXT("wood"))==2))return false;
            EcologyFire=Fire->Id;
            auto* Combat=Hero->FindComponentByClass<UHearthwardCombatComponent>();
            if(PlayerBag->EquippedInstance(TEXT("weapon"))!=PlayerBag->FirstInstance(TEXT("axe")))
            {
                if(!TestTrue(TEXT("Actual owned axe enters the normal equipment switch action"),Gameplay->EquipInstance(PlayerBag->FirstInstance(TEXT("axe")))))return false;
                Clock->Tick(.4f);Combat->TickComponent(.4f,LEVELTICK_All,nullptr);
            }
            if(!TestTrue(TEXT("Normal switch completion makes actual axe attack power available"),!Combat->Busy() && Gameplay->AttackPower()>0
                && PlayerBag->EquippedInstance(TEXT("weapon"))==PlayerBag->FirstInstance(TEXT("axe"))))return false;
            const auto* Slot=Nature->State.Slots.FindByPredicate([](const auto& S){return S.Definition==TEXT("hare");});
            if(!TestTrue(TEXT("Seeded actual hare begins at first generation"),Slot && Slot->Generation==1 && Slot->Due==-1 && Nature->Actor(Slot->Current)))return false;
            EcologySlot=Slot->Id;EcologyFirstCorpse=Slot->Current;
            const auto KillActualHare=[&](FGuid Id)
            {
                auto* Actor=Nature->Actor(Id);auto* Target=Actor?Actor->FindComponentByClass<UHearthwardCombatTargetComponent>():nullptr;
                if(!TestTrue(TEXT("Damage fixture uses the actual living natural target"),Target && Target->Alive() && Target->NaturalTarget))return false;
                for(int32 Hit=0;Hit<8 && Target->Alive();++Hit)
                    Combat->HitTarget(Target,Gameplay->AttackPower(),TEXT("body"),false,FGuid::NewGuid(),Hero);
                const auto* Animal=Nature->State.Animals.FindByPredicate([&](const auto& A){return A.Id==Id;});
                const auto* Current=Nature->State.Slots.FindByPredicate([&](const auto& S){return S.Id==EcologySlot;});
                if(!TestTrue(TEXT("Production death transaction creates unpaid corpse loot and future animal Due"),Animal && Current && !Target->Alive()
                    && Animal->Health==0 && Animal->Rewarded && !Animal->Loot.IsEmpty() && Current->Due==Nature->State.Calendar+2880))return false;
                Clock->Tick(3.f);Gameplay->TickComponent(3.f,LEVELTICK_All,nullptr);
                return TestFalse(TEXT("Normal active update ends actual three-second combat cooldown"),Gameplay->InCombat());
            };
            if(!KillActualHare(EcologyFirstCorpse))return false;
            for(TActorIterator<AActor> It(World);It;++It)
                if(It->FindComponentByClass<UHearthwardSurvivalComponent>() && !TestTrue(TEXT("Actual survival participant is outside wildlife's eighty-meter refresh exclusion"),
                    FVector::Dist2D(It->GetActorLocation(),Slot->Position)>8000))return false;
            for(int32 I=0;I<6;++I)if(!WaitEcology(480))return false;
            Slot=Nature->State.Slots.FindByPredicate([&](const auto& S){return S.Id==EcologySlot;});
            if(!TestTrue(TEXT("Paid waiting naturally creates one actual second-generation identity"),Slot && Slot->Generation==2
                && Slot->Current!=EcologyFirstCorpse && Slot->Due==-1 && Nature->Actor(Slot->Current)))return false;
            EcologyAnimal=Slot->Current;
            if(!KillActualHare(EcologyAnimal))return false;
            const auto* Herb=Nature->State.Points.FindByPredicate([](const auto& P){return P.Kind==TEXT("resource") && P.Definition==TEXT("herb_patch");});
            if(!TestNotNull(TEXT("Seeded world exposes a real hand-harvest herb point"),Herb))return false;
            EcologySource=Herb->Key.ToString();const FGuid HerbId=Herb->Id;
            if(!MoveEcologyPlayer(Herb->Position+FVector(180,0,0)))return false;
            const int32 BeforeHerb=PlayerBag->GetItemCount(TEXT("herb"));
            for(int32 I=0;I<2;++I)
            {
                if(!TestTrue(FString(TEXT("Normal harvest begins on actual herb source: "))+Nature->Feedback,Nature->Act(TEXT("harvest"),HerbId,NAME_None,Store->GetTimelineEpoch())))return false;
                Clock->Tick(5.f);Timer->TickComponent(5.f,LEVELTICK_All,nullptr);
                if(!TestFalse(TEXT("Real five-active-second harvest action completes"),Nature->Busy()))return false;
            }
            const auto* Source=Economy->Source(EcologySource);
            if(!TestTrue(TEXT("Actual harvests consume four herbs and leave future resource Due"),Source && Source->Remaining==0 && Source->Capacity==4
                && Source->Due==Nature->State.Calendar+2880 && PlayerBag->GetItemCount(TEXT("herb"))==BeforeHerb+4))return false;
            if(!MoveEcologyPlayer(Economy->State.Camps[0].Position))return false;
            Controller->SetControlRotation(FRotator::ZeroRotator);
        }
        if(!TestTrue(TEXT("Fixture grants one real treasure map"),PlayerBag->TryAdd(TEXT("treasure_map_1"),1)==EHearthwardInventoryResult::Success)
            || !TestTrue(FString(TEXT("Public ReadMap consumes it and creates actual pending loot: "))+Nature->Feedback,Nature->ReadMap(TEXT("treasure_map_1"))))return false;
        const auto* Treasure=Nature->State.Points.FindByPredicate([](const auto& P){return P.Kind==TEXT("treasure") && P.Definition==TEXT("treasure_map_1");});
        if(!TestTrue(TEXT("Real map transaction keeps bow instance and eight ingots unclaimed"),Treasure && Treasure->Id.IsValid()
            && Treasure->Pending.Instances.Num()==1 && Treasure->Pending.Instances[0].Id.IsValid() && Treasure->Pending.Instances[0].Definition==TEXT("bow_2")
            && Treasure->Pending.Stacks.FindRef(TEXT("metal_ingot"))==8 && PlayerBag->GetItemCount(TEXT("treasure_map_1"))==0
            && Nature->State.Maps.Contains(TEXT("treasure_map_1")) && Nature->State.Rewards.Contains(TEXT("treasure_map_1")) && !Nature->State.Opened.Contains(TEXT("treasure_map_1"))))return false;
        const auto BeforeAdvance=Clock->GetSnapshot();Clock->Tick(7.f);
        if(!TestTrue(TEXT("Seven active seconds follow the actual preparation boundary"),
            FMath::IsNearlyEqual(Clock->GetSnapshot().ActivePlaySeconds,BeforeAdvance.ActivePlaySeconds+7,1.e-6)
            && FMath::IsNearlyEqual(Clock->GetSnapshot().ElapsedCalendarMinutes,BeforeAdvance.ElapsedCalendarMinutes+7,1.e-6)))return false;
        const FGuid WorkbenchId=Economy->State.Facilities[0].Id;
        const auto* Region=Economy->State.Regions.FindByPredicate([&](const auto& R){return R.Facility==WorkbenchId;});
        if(!TestNotNull(TEXT("Real building completion publishes its processing region"),Region))return false;
        const FName RegionId=Region->Id;const FGuid Epoch=Store->GetTimelineEpoch();
        if(!TestTrue(TEXT("Normal known rope selection succeeds"),Economy->SelectProduction(RegionId,WorkbenchId,TEXT("rope"),Epoch))
            || !TestTrue(TEXT("Normal initial-population worker zero joins the queue"),Economy->AssignWorker(RegionId,0,Epoch))
            || !TestTrue(TEXT("Normal production control enables the real rope queue"),Economy->SetProduction(RegionId,true,false,Epoch)))return false;
        Clock->Tick(.5f);
        Region=Economy->State.Regions.FindByPredicate([&](const auto& R){return R.Id==RegionId;});
        if(!TestTrue(TEXT("Real clock debits one input and leaves a partial uncredited rope batch"),Region && Region->Batch.Active && Region->Batch.Work>0
            && Region->Batch.Work<Region->Batch.Required && Region->Batch.Required==360 && Region->Workers==TArray<int32>{0}
            && Region->Batch.Inputs.FindRef(TEXT("wood"))==2 && Region->Batch.Outputs.FindRef(TEXT("rope"))==1
            && Store->GetItemCount(TEXT("wood"))==0 && Store->GetItemCount(TEXT("rope"))==0))return false;
        if(!TestTrue(TEXT("Write phase records an actual player agreement"),AI->PutPlayerMemory(Hero,Brother,FGuid(),TEXT("agreement"),TEXT("Task071 restart agreement"))))return false;
        Save->RememberExchange(TEXT("Player"),TEXT("Task071 restart exchange"));
    }
    FHearthwardAgentGoal Goal;Goal.Intent=TEXT("collect");Goal.Item=TEXT("wood");Goal.Quantity=1;
    Goal.QuantityMode=TEXT("additional_acquired");Goal.SourceRef=TEXT("S1");
    const auto FirstTicket=Brother->Request(Hero,TEXT("Collect one wood"));
    if(!TestTrue(TEXT("Normal goal submission succeeds before saving"),Brother->SubmitGoal(Hero,FirstTicket,Goal)==EHearthwardProposalResult::Accepted))return false;
    const bool Saved=Save->SavePoint(true);
    if(!TestTrue(FString(TEXT("Actual stable command snapshot saves: "))+Save->GetStatus(),Saved))return false;
    const auto Point=Save->GetPoints().Last();const auto OldEpoch=Store->GetTimelineEpoch();
    if(RestartPhase==TEXT("write"))
    {
        const auto& S=Point.World;
        if(!TestTrue(TEXT("Write snapshot has a real active command, record, and event coverage"),S.CommandActive && S.CommandId.IsValid()
            && S.NPCMemory.Records.Num()==1 && !S.NPCMemory.Events.IsEmpty() && !S.NPCMemory.CommandCoverage.IsEmpty()))return false;
        auto Manifest=MakeShared<FJsonObject>();
        Manifest->SetStringField(TEXT("pool"),PoolId.ToString(EGuidFormats::Digits));Manifest->SetNumberField(TEXT("writer_pid"),FPlatformProcess::GetCurrentProcessId());
        Manifest->SetStringField(TEXT("save_id"),Point.SaveId.ToString());Manifest->SetStringField(TEXT("campaign_id"),Point.CampaignId.ToString());
        Manifest->SetStringField(TEXT("command_id"),S.CommandId.ToString());Manifest->SetStringField(TEXT("axe_id"),PlayerBag->FirstInstance(TEXT("axe")).ToString());
        Manifest->SetStringField(TEXT("memory_id"),S.NPCMemory.Records[0].Id.ToString());Manifest->SetNumberField(TEXT("point_count"),Save->GetPoints().Num());
        Manifest->SetNumberField(TEXT("player_wood"),S.PlayerItems.Stacks.FindRef(TEXT("wood")));Manifest->SetNumberField(TEXT("shared_wood"),S.StorageItems.Stacks.FindRef(TEXT("wood")));
        Manifest->SetNumberField(TEXT("brother_wood"),S.BrotherItems.Stacks.FindRef(TEXT("wood")));Manifest->SetNumberField(TEXT("source_wood"),S.Resource.FindRef(TEXT("wood")));
        Manifest->SetNumberField(TEXT("requested"),S.Requested);Manifest->SetNumberField(TEXT("acquired"),S.Acquired);
        Manifest->SetNumberField(TEXT("carried"),S.Carried);Manifest->SetNumberField(TEXT("delivered"),S.Delivered);
        Manifest->SetNumberField(TEXT("active_seconds"),S.ActiveSeconds);Manifest->SetNumberField(TEXT("calendar_minutes"),S.CalendarMinutes);
        Manifest->SetNumberField(TEXT("initial_day"),S.InitialDay);Manifest->SetNumberField(TEXT("initial_minute"),S.InitialMinute);
        Manifest->SetNumberField(TEXT("npc_revision"),S.NPCMemory.Revision);Manifest->SetNumberField(TEXT("npc_events"),S.NPCMemory.Events.Num());
        Manifest->SetNumberField(TEXT("npc_coverage"),S.NPCMemory.CommandCoverage.Num());Manifest->SetNumberField(TEXT("knowledge_revision"),S.KnowledgeRevision);
        TArray<TSharedPtr<FJsonValue>> EventIds;for(const auto& E:S.NPCMemory.Events) EventIds.Add(MakeShared<FJsonValueString>(E.Id.ToString()));
        Manifest->SetArrayField(TEXT("event_ids"),EventIds);Manifest->SetBoolField(TEXT("conversation_started"),S.NPCMemory.ConversationClockStarted);
        Manifest->SetBoolField(TEXT("awaiting_meeting"),S.NPCMemory.ConversationClockAwaitingFirstMeeting);Manifest->SetNumberField(TEXT("last_contact"),S.NPCMemory.LastConversationCalendar);
        Manifest->SetStringField(TEXT("knowledge"),FString::Join(S.Knowledge,TEXT("\n")));
        FHearthwardCampState WrittenCamp;FHearthwardNatureState WrittenNature;
        if(!TestTrue(TEXT("Actual saved mature domains validate before manifest publication"),FHearthwardCampState::Parse(S.CampEconomy,WrittenCamp)
            && FHearthwardNatureState::Parse(S.Nature,WrittenNature)))return false;
        const auto* Region=WrittenCamp.Regions.FindByPredicate([](const auto& R){return R.Job==TEXT("rope") && R.Batch.Active;});
        const auto* Treasure=WrittenNature.Points.FindByPredicate([](const auto& P){return P.Kind==TEXT("treasure") && P.Definition==TEXT("treasure_map_1");});
        if(!TestTrue(TEXT("Manifest publishes actual saved invested batch and pending reward identities"),Region && Treasure
            && Region->Facility.IsValid() && Region->Batch.Work>0 && !Treasure->Pending.Instances.IsEmpty()))return false;
        Manifest->SetStringField(TEXT("workbench_id"),Region->Facility.ToString());Manifest->SetStringField(TEXT("production_region"),Region->Id.ToString());
        Manifest->SetStringField(TEXT("treasure_id"),Treasure->Id.ToString());Manifest->SetStringField(TEXT("camp_economy"),S.CampEconomy);
        Manifest->SetStringField(TEXT("nature"),S.Nature);
        if(RestartEcology)
        {
            const auto* Slot=WrittenNature.Slots.FindByPredicate([&](const auto& V){return V.Id==EcologySlot;});
            const auto* Animal=WrittenNature.Animals.FindByPredicate([&](const auto& V){return V.Id==EcologyAnimal;});
            const auto* Source=WrittenCamp.Sources.FindByPredicate([&](const auto& V){return V.Id==EcologySource;});
            TSharedPtr<FJsonObject> WrittenGameplay;
            if(!TestTrue(TEXT("Actual saved ecology validates nondefault generation and future Due before manifest"),Slot && Animal && Source
                && Slot->Generation==2 && Slot->Current==EcologyAnimal && Slot->Due>S.CalendarMinutes && Animal->Health==0 && Animal->Rewarded && !Animal->Loot.IsEmpty()
                && Source->Remaining==0 && Source->Due>S.CalendarMinutes && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S.Gameplay),WrittenGameplay)))return false;
            Manifest->SetBoolField(TEXT("ecology"),true);Manifest->SetStringField(TEXT("ecology_fire"),EcologyFire.ToString());
            Manifest->SetStringField(TEXT("ecology_slot"),EcologySlot.ToString());Manifest->SetStringField(TEXT("ecology_animal"),EcologyAnimal.ToString());
            Manifest->SetStringField(TEXT("ecology_first_corpse"),EcologyFirstCorpse.ToString());Manifest->SetStringField(TEXT("ecology_source"),EcologySource);
            Manifest->SetNumberField(TEXT("ecology_experience"),WrittenGameplay->GetNumberField(TEXT("experience")));
        }
        if(RestartTwoCamps)
        {
            FHearthwardCampaignState WrittenCampaign;TSharedPtr<FJsonObject> WrittenGameplay;
            const auto* Reward=S.StorageItems.Instances.FindByPredicate([](const auto& I){return I.Definition==TEXT("hearth_blade");});
            if(!TestTrue(TEXT("Manifest derives continuous permanent victory and original unique reward from the actual saved point"),
                FHearthwardCampaignState::Parse(S.Campaign,WrittenCampaign) && WrittenCampaign.Victory && WrittenCampaign.ReadyForVictory()
                && !WrittenCampaign.Legacy && !WrittenCampaign.LegacyHometown && WrittenCampaign.Facts.Contains(TEXT("prologue_complete"))
                && WrittenCampaign.Facts.Contains(TEXT("home_saved")) && WrittenCamp.Hometown && WrittenCamp.Camps.Num()==2
                && WrittenNature.Camps.Contains(TEXT("camp")) && WrittenNature.Camps.Contains(TEXT("hometown"))
                && Reward && Reward->Id.IsValid() && Reward->UniqueClaim==TEXT("hometown_hearth_blade")
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S.Gameplay),WrittenGameplay)))return false;
            int32 Gifts=0,Beds=0,Access=0,Fires=0;TSet<FGuid> GiftIds;TArray<TSharedPtr<FJsonValue>> ManifestGifts;
            for(const auto& F:WrittenCamp.Facilities)if(F.Camp==TEXT("hometown"))
            {
                ++Gifts;Beds+=int32(F.Kind==TEXT("bed"));Access+=int32(F.Kind==TEXT("warehouse_access"));Fires+=int32(F.Kind==TEXT("campfire"));GiftIds.Add(F.Id);
                TestTrue(TEXT("Manifest publishes actual unpaid hometown gift accounting"),F.Paid.IsEmpty() && F.Level==1 && !F.Paused);
                ManifestGifts.Add(MakeShared<FJsonValueString>(F.Id.ToString()));
            }
            if(!TestTrue(TEXT("Actual disk snapshot contains all four distinct victory gift identities"),Gifts==4 && GiftIds.Num()==4 && Beds==2 && Access==1 && Fires==1))return false;
            Manifest->SetBoolField(TEXT("two_camps"),true);Manifest->SetStringField(TEXT("campaign_state"),S.Campaign);
            Manifest->SetArrayField(TEXT("hometown_gift_ids"),ManifestGifts);
            Manifest->SetStringField(TEXT("hometown_reward_id"),Reward->Id.ToString());Manifest->SetNumberField(TEXT("campaign_experience"),WrittenGameplay->GetNumberField(TEXT("experience")));
        }
        FString Json;FJsonSerializer::Serialize(Manifest,TJsonWriterFactory<>::Create(&Json));
        if(!TestTrue(TEXT("Write process persists the isolated QA manifest"),IFileManager::Get().MakeDirectory(*FPaths::GetPath(ManifestPath),true)
            && FFileHelper::SaveStringToFile(Json,*ManifestPath,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)))return false;
        AddInfo(FString::Printf(TEXT("TASK071 restart WRITE pid=%u save=%s manifest=%s"),FPlatformProcess::GetCurrentProcessId(),*Point.SaveId.ToString(),*ManifestPath));
        return true;
    }
    const auto OldTicket=Brother->Request(Hero,TEXT("Late result for another one wood"));
    if(!TestTrue(TEXT("Old pending ticket is genuinely current before LoadPoint"),Brother->IsProposalCurrent(Hero,OldTicket)))return false;
    if(!TestTrue(TEXT("Future inventories really differ before LoadPoint"),
        PlayerBag->TryAdd(TEXT("wood"),2)==EHearthwardInventoryResult::Success && Store->Adjust({},{{FName(TEXT("wood")),4}})
        && PlayerBag->GetItemCount(TEXT("wood"))==5 && Store->GetItemCount(TEXT("wood"))==4))return false;
    if(!TestTrue(TEXT("Future inventory preparation keeps the old ticket current"),Brother->IsProposalCurrent(Hero,OldTicket)))return false;
    const bool Loaded=Save->LoadPoint(Point.SaveId);
    if(!TestTrue(FString(TEXT("Public LoadPoint reads disk and restores the real world: "))+Save->GetStatus(),Loaded))return false;
    TestTrue(TEXT("Actual LoadPoint creates a different epoch"),Store->GetTimelineEpoch()!=OldEpoch);
    TestFalse(TEXT("Actual LoadPoint invalidates the old pending ticket"),Brother->IsProposalCurrent(Hero,OldTicket));
    TestEqual(TEXT("Actual LoadPoint removes future player wood"),PlayerBag->GetItemCount(TEXT("wood")),3);
    TestEqual(TEXT("Actual LoadPoint removes future shared wood"),Store->GetItemCount(TEXT("wood")),0);
    TestTrue(TEXT("Actual LoadPoint preserves the saved active command ID"),Brother->GetCommandId()==Point.World.CommandId);
    const int32 BeforeEvents=AI->GetMemorySnapshot().Events.Num();
    TestTrue(TEXT("Late old goal is rejected at the real submission boundary"),Brother->SubmitGoal(Hero,OldTicket,Goal)==EHearthwardProposalResult::Stale);
    TestEqual(TEXT("Late old goal cannot change player inventory"),PlayerBag->GetItemCount(TEXT("wood")),3);
    TestEqual(TEXT("Late old goal cannot change shared inventory"),Store->GetItemCount(TEXT("wood")),0);
    TestEqual(TEXT("Late old goal cannot change source inventory"),Stock->GetItemCount(TEXT("wood")),8);
    TestEqual(TEXT("Late old goal cannot put future cargo in Brother's bag"),Brother->Bag->GetItemCount(TEXT("wood")),0);
    TestTrue(TEXT("Late old goal cannot supersede the restored command"),Brother->GetCommandId()==Point.World.CommandId);
    TestEqual(TEXT("Late old goal cannot change requested amount"),Brother->GetRequested(),Point.World.Requested);
    TestEqual(TEXT("Late old goal cannot change delivered amount"),Brother->GetDelivered(),Point.World.Delivered);
    TestEqual(TEXT("Late old goal cannot append NPC events"),AI->GetMemorySnapshot().Events.Num(),BeforeEvents);
    TestEqual(TEXT("Load and stale submission preserve saved active time"),Clock->GetSnapshot().ActivePlaySeconds,Point.World.ActiveSeconds);
    TestEqual(TEXT("Load and stale submission preserve saved calendar time"),Clock->GetSnapshot().ElapsedCalendarMinutes,Point.World.CalendarMinutes);
    const bool Resaved=Save->SavePoint(true);
    if(!TestTrue(FString(TEXT("Post-stale actual snapshot remains savable: "))+Save->GetStatus(),Resaved))return false;
    const auto After=Save->GetPoints().Last().World;
    TestTrue(TEXT("Late old goal cannot append settled operations"),After.NPCOperations==Point.World.NPCOperations);
    TestEqual(TEXT("Late old goal cannot append effect receipts"),After.NPCReceipts.Num(),Point.World.NPCReceipts.Num());
    const auto FreshTicket=Brother->Request(Hero,TEXT("Collect one wood on the restored timeline"));
    TestTrue(TEXT("Restored world accepts a genuinely fresh ticket"),Brother->IsProposalCurrent(Hero,FreshTicket));
    TestTrue(TEXT("Same normal goal succeeds on the new timeline"),Brother->SubmitGoal(Hero,FreshTicket,Goal)==EHearthwardProposalResult::Accepted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveFileTest, "Hearthward.Save.FileIntegrityAndSnapshot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveFileTest::RunTest(const FString& Parameters)
{
    const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Task016"), FGuid::NewGuid().ToString() + TEXT(".hws"));
    auto* Pool = NewObject<UHearthwardSaveGame>();
    Pool->Points.Add(Point());
    auto& S = Pool->Points[0].World;
    S.PlayerItems.Stacks.Add(TEXT("wood"), 5); S.StorageItems.Stacks.Add(TEXT("stone"), 120); S.Resource.Add(TEXT("wood"), 16);
    S.Knowledge.Add(TEXT("玩家原话（未核实）: 营地约定")); S.KnowledgeRevision = 1;
    S.ActiveSeconds = 123; S.CalendarMinutes=123; S.Player.SetLocation(FVector(10,20,30));
    FString Error;
    TestTrue(TEXT("Write initial pool"), HearthwardSave::Write(Path, Pool, Error));
    UHearthwardSaveGame* Loaded = nullptr;
    TestTrue(TEXT("Read same snapshot from disk"), HearthwardSave::Read(Path, Loaded, Error));
    if (Loaded)
    {
        const auto& R = Loaded->Points[0].World;
        TestEqual(TEXT("Personal inventory"), R.PlayerItems.Stacks.FindRef(TEXT("wood")), 5);
        TestEqual(TEXT("Unlimited shared storage"), R.StorageItems.Stacks.FindRef(TEXT("stone")), 120);
        TestTrue(TEXT("Knowledge and time at same boundary"), R.Knowledge == S.Knowledge && R.ActiveSeconds == S.ActiveSeconds);
        TestTrue(TEXT("World transform"), R.Player.Equals(S.Player));
    }
    TArray<uint8> Original;
    FFileHelper::LoadFileToArray(Original, *Path);
    // A half-written temporary file must never become a visible save point.
    FFileHelper::SaveArrayToFile(TArray<uint8>{1,2,3}, *(Path + TEXT(".pending")));
    TestTrue(TEXT("Interrupted pending write leaves committed pool readable"), HearthwardSave::Read(Path, Loaded, Error));
    Pool->Schema = 0;
    TestFalse(TEXT("Old schema cannot overwrite valid file"), HearthwardSave::Write(Path, Pool, Error));
    TestFalse(TEXT("Old schema is explicitly incompatible"), HearthwardSave::Validate(*Pool));
    TArray<uint8> OldPayload, OldFile;
    UGameplayStatics::SaveGameToMemory(Pool, OldPayload);
    const uint32 OldHeader[] = {0x48575331, uint32(OldPayload.Num()), FCrc::MemCrc32(OldPayload.GetData(), OldPayload.Num())};
    OldFile.Append(reinterpret_cast<const uint8*>(OldHeader), sizeof(OldHeader)); OldFile.Append(OldPayload);
    FFileHelper::SaveArrayToFile(OldFile, *Path);
    TestFalse(TEXT("Intact old-schema file rejected at load"), HearthwardSave::Read(Path, Loaded, Error));
    FFileHelper::SaveArrayToFile(Original, *Path);
    Pool->Schema = HearthwardSave::CurrentSchema;
    S.PlayerItems.Stacks[TEXT("wood")] = 101;
    TestFalse(TEXT("Invalid capacity rejected before mutation"), HearthwardSave::Write(Path, Pool, Error));
    S.PlayerItems.Stacks[TEXT("wood")] = 5;
    S.Knowledge.Add(TEXT("future exchange")); S.KnowledgeRevision = 2;
    TestTrue(TEXT("Atomic replacement of existing committed pool"), HearthwardSave::Write(Path, Pool, Error));
    TestTrue(TEXT("Read replaced pool"), HearthwardSave::Read(Path, Loaded, Error));
    if (Loaded) TestEqual(TEXT("Replacement includes knowledge"), Loaded->Points[0].World.Knowledge.Num(), 2);
    auto Natural = Point();
    Natural.World.NaturalWorld = true;
    Natural.World.Map = TEXT("L_HearthwardWilds");
    Natural.World.Player.SetLocation(FVector(-97600, -75200, 16195));
    Pool->Points.Add(Natural);
    TestTrue(TEXT("Natural world point shares the pool without a companion fixture"), HearthwardSave::Write(Path, Pool, Error));
    TestTrue(TEXT("Natural world point reads from disk"), HearthwardSave::Read(Path, Loaded, Error));
    if (Loaded) TestTrue(TEXT("Natural world marker and spawn survive serialization"),
        Loaded->Points.Last().World.NaturalWorld && Loaded->Points.Last().World.Player.Equals(Natural.World.Player));
    if (Loaded) TestFalse(TEXT("Old natural saves explicitly lack companion state"), Loaded->Points.Last().World.NaturalCompanion);
    auto& Integrated=Pool->Points.Last().World;
    Integrated.NaturalCompanion=true;
    Integrated.Resource.Add(TEXT("wood"),14);
    Integrated.HarvestedResources.Add(TEXT("tree-instance-42"),6);
    Integrated.Companion.SetLocation(FVector(-97800,-75000,16180));
    TestTrue(TEXT("Integrated camp writes through the same save format"), HearthwardSave::Write(Path, Pool, Error));
    TestTrue(TEXT("Integrated camp reads from disk"), HearthwardSave::Read(Path, Loaded, Error));
    if (Loaded) TestTrue(TEXT("Companion presence and finite resources survive disk round trip"),
        Loaded->Points.Last().World.NaturalCompanion && Loaded->Points.Last().World.Resource.FindRef(TEXT("wood"))==14
        && Loaded->Points.Last().World.Companion.Equals(Integrated.Companion)
        && Loaded->Points.Last().World.HarvestedResources.FindRef(TEXT("tree-instance-42"))==6);
    Integrated.HarvestedResources.Add(TEXT("tree-instance-42"),-1);
    TestFalse(TEXT("Invalid resource consumption rejected"), HearthwardSave::Validate(*Pool));
    auto Corrupt = Original; Corrupt.Last() ^= 1;
    FFileHelper::SaveArrayToFile(Corrupt, *Path);
    TestFalse(TEXT("Payload corruption rejected"), HearthwardSave::Read(Path, Loaded, Error));
    Corrupt.SetNum(Corrupt.Num()/2); FFileHelper::SaveArrayToFile(Corrupt, *Path);
    TestFalse(TEXT("Truncated snapshot rejected"), HearthwardSave::Read(Path, Loaded, Error));
    IFileManager::Get().Delete(*Path); IFileManager::Get().Delete(*(Path + TEXT(".pending")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveNaturalLegacyFixtureTest, "Hearthward.Save.NaturalLegacyFile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveNaturalLegacyFixtureTest::RunTest(const FString& Parameters)
{
    auto* Pool=NewObject<UHearthwardSaveGame>();
    auto Legacy=Point();
    Legacy.World.NaturalWorld=true;
    Legacy.World.Map=TEXT("L_HearthwardWilds");
    Legacy.World.Player.SetLocation(FVector(-97600,-75200,16195));
    Legacy.World.PlayerItems.Stacks.Add(TEXT("wood"),3);
    Legacy.World.StorageItems.Stacks.Add(TEXT("wood"),5);
    Pool->Points.Add(Legacy);
    FString Error;
    const FString Path=FPaths::ProjectSavedDir()/TEXT("NaturalCampValidation/legacy-natural.hws");
    TestTrue(TEXT("Write real pre-integration natural file for runtime migration test"),HearthwardSave::Write(Path,Pool,Error));
    UHearthwardSaveGame* Read=nullptr;
    TestTrue(TEXT("Read pre-integration natural file"),HearthwardSave::Read(Path,Read,Error));
    if(Read) TestFalse(TEXT("Legacy presence marker remains absent"),Read->Points[0].World.NaturalCompanion);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveSchema2MigrationFileTest, "Hearthward.Save.Schema2To3RealFileMigration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveSchema2MigrationFileTest::RunTest(const FString& Parameters)
{
    const FString Dir=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Task040"));
    IFileManager::Get().MakeDirectory(*Dir,true);
    const FString SourcePath=FPaths::Combine(Dir,FGuid::NewGuid().ToString()+TEXT("-schema2.hws"));
    const FString MigratedPath=FPaths::Combine(Dir,FGuid::NewGuid().ToString()+TEXT("-schema3.hws"));

    auto* Legacy=NewObject<UHearthwardSaveGame>();
    Legacy->Schema=2;
    Legacy->Points.Add(Point());
    auto& P=Legacy->Points[0];
    auto& S=P.World;
    S.ActiveSeconds=30;
    S.NPCStateVersion=2;
    S.NPCMemory.Campaign=P.CampaignId;
    S.NPCMemory.Revision=3;

    FHearthwardNPCBelief Belief;
    Belief.Id=FGuid::NewGuid();Belief.Item=TEXT("wood");Belief.Value=7;Belief.Source=EHearthwardNPCBeliefSource::Firsthand;
    Belief.RecordedAt=12;Belief.LastEvidenceAt=0;Belief.Revision=2;Belief.Campaign=P.CampaignId;
    S.NPCMemory.Beliefs.Add(Belief);

    const FGuid HistoricalCommand=FGuid::NewGuid();
    FHearthwardNPCEvent Event;
    Event.Id=FGuid::NewGuid();Event.Command=HistoricalCommand;Event.Campaign=P.CampaignId;Event.Kind=TEXT("blocked");
    Event.Item=TEXT("wood");Event.Count=0;Event.At=14;Event.Reason=TEXT("source_shortage");
    S.NPCMemory.Events.Add(Event);
    TestTrue(TEXT("Schema 2 fixture starts without TASK-040 coverage metadata"),S.NPCMemory.CommandCoverage.IsEmpty());

    TArray<uint8> Payload,Bytes;
    TestTrue(TEXT("Serialize explicit schema 2 payload"),UGameplayStatics::SaveGameToMemory(Legacy,Payload));
    const uint32 Header[]={0x48575332,uint32(Payload.Num()),FCrc::MemCrc32(Payload.GetData(),Payload.Num())};
    Bytes.Append(reinterpret_cast<const uint8*>(Header),sizeof(Header));Bytes.Append(Payload);
    TestTrue(TEXT("Write real schema 2 file"),FFileHelper::SaveArrayToFile(Bytes,*SourcePath));

    UHearthwardSaveGame* Migrated=nullptr;FString Error;
    TestTrue(TEXT("Read and migrate real schema 2 file"),HearthwardSave::Read(SourcePath,Migrated,Error));
    if(Migrated)
    {
        TestEqual(TEXT("Schema promoted to 3"),Migrated->Schema,HearthwardSave::CurrentSchema);
        const auto& MS=Migrated->Points[0].World;
        TestEqual(TEXT("NPC state promoted to v3"),MS.NPCStateVersion,HearthwardSave::NPCStateVersion);
        TestEqual(TEXT("Legacy belief receives evidence timestamp"),MS.NPCMemory.Beliefs[0].LastEvidenceAt,MS.NPCMemory.Beliefs[0].RecordedAt);
        TestTrue(TEXT("Historical episode receives explicit unknown coverage"),
            MS.NPCMemory.CoverageFor(HistoricalCommand)==EHearthwardNPCEpisodeCoverage::Unknown);
        TestTrue(TEXT("Migrated cognition stays bound to original campaign"),MS.NPCMemory.Campaign==P.CampaignId);
        TestTrue(TEXT("Write migrated schema 3 file"),HearthwardSave::Write(MigratedPath,Migrated,Error));
        UHearthwardSaveGame* RoundTrip=nullptr;
        TestTrue(TEXT("Reload migrated schema 3 file"),HearthwardSave::Read(MigratedPath,RoundTrip,Error));
        TestTrue(TEXT("Reloaded migrated file validates strictly"),RoundTrip && HearthwardSave::Validate(*RoundTrip));
    }
    IFileManager::Get().Delete(*SourcePath);IFileManager::Get().Delete(*MigratedPath);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveNPCMemoryTest, "Hearthward.Save.NPCMemoryCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveNPCMemoryTest::RunTest(const FString& Parameters)
{
    UHearthwardSaveGame* Legacy=nullptr; FString Error;
    TestTrue(TEXT("Actual pre-025 file remains readable"),HearthwardSave::Read(FPaths::ProjectDir()/TEXT("docs/qa/evidence/TASK-020/legacy-task019.hws"),Legacy,Error));
    if(Legacy) for(const auto& P:Legacy->Points)
        TestTrue(TEXT("Old file has empty cognition, preserves raw knowledge"),P.World.NPCMemory.Records.IsEmpty() && P.World.NPCMemory.Clarification.IsEmpty() && !P.World.NPCMemory.HasCampObservation);
    TestTrue(TEXT("Actual original-025 pool migrates"),HearthwardSave::Read(FPaths::ProjectDir()/TEXT("docs/qa/evidence/TASK-025/rev2/legacy-v1.hws"),Legacy,Error));
    if(Legacy)
    {
        TestEqual(TEXT("Migrated schema"),Legacy->Schema,HearthwardSave::CurrentSchema);
        TestTrue(TEXT("Existing cognition retained"),Legacy->Points.ContainsByPredicate([](const auto& P){return !P.World.NPCMemory.Records.IsEmpty();}));
        for(const auto& P:Legacy->Points)TestEqual(TEXT("Memory bound to original campaign"),P.World.NPCMemory.Campaign,P.CampaignId);
    }
    auto* Pool=NewObject<UHearthwardSaveGame>(); Pool->Points.Add(Point());
    auto& S=Pool->Points[0].World; S.ActiveSeconds=20; S.CalendarMinutes=20;
    S.NPCMemory.Put({},TEXT("claim"),TEXT("原话不能变成库存"),3);
    S.NPCMemory.Put({},TEXT("collection_ban"),TEXT("不采木材"),3,TEXT("wood"));
    S.NPCMemory.AddClarification(TEXT("采木材，限制未解除"),TEXT("需要多少？"));
    S.NPCMemory.HasCampObservation=true; S.NPCMemory.CampInventory.Add(TEXT("wood"),3); S.NPCMemory.CampObservedAt=10;
    S.NPCMemory.Migrate(S.NPCMemory.Campaign);
    TArray<uint8> Bytes; TestTrue(TEXT("Cognition serialized with world"),UGameplayStatics::SaveGameToMemory(Pool,Bytes));
    auto* Loaded=Cast<UHearthwardSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestTrue(TEXT("Cognition round trip valid"),Loaded && HearthwardSave::Validate(*Loaded));
    if(Loaded)
    {
        const auto& M=Loaded->Points[0].World.NPCMemory;
        TestEqual(TEXT("Record round trip"),M.Records.Num(),2);
        TestTrue(TEXT("Structured restriction round trip"),M.BlocksCollection(TEXT("wood")));
        TestEqual(TEXT("Clarification round trip"),M.Clarification.Num(),1);
        TestEqual(TEXT("Old observation round trip"),M.CampInventory.FindRef(TEXT("wood")),3);
    }
    S.NPCMemory.CampObservedAt=21;
    TestFalse(TEXT("Future cognition rejects entire snapshot before world mutation"),HearthwardSave::Validate(*Pool));
    S.NPCMemory.CampObservedAt=10;
    if(!S.NPCMemory.Beliefs.IsEmpty())
    {
        const double SavedEvidence=S.NPCMemory.Beliefs[0].LastEvidenceAt;
        S.NPCMemory.Beliefs[0].LastEvidenceAt=S.NPCMemory.Beliefs[0].RecordedAt-1;
        TestFalse(TEXT("Damaged current-format evidence time is rejected, not migrated"),HearthwardSave::Validate(*Pool));
        S.NPCMemory.Beliefs[0].LastEvidenceAt=SavedEvidence;
    }
    S.NPCStateVersion=0;
    TestFalse(TEXT("Missing version in new snapshot not silently defaulted"),HearthwardSave::Validate(*Pool));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveLegacyCampMigrationTest, "Hearthward.Save.LegacyCampMigration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveLegacyCampMigrationTest::RunTest(const FString& Parameters)
{
    const FString Path=FPaths::ProjectSavedDir()/TEXT("LegacyCampFix/synthetic.hws");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    auto* Pool=NewObject<UHearthwardSaveGame>();
    Pool->Schema=3;Pool->Points.Add(Point());
    auto& S=Pool->Points[0].World;
    S.NaturalWorld=true;S.Map=TEXT("L_HearthwardWilds");
    S.Inventory.Add(TEXT("wood"),3);S.Storage.Add(TEXT("wood"),5);
    auto WriteLegacy=[&](uint32 Magic)
    {
        TArray<uint8> Payload,Bytes;
        if(!UGameplayStatics::SaveGameToMemory(Pool,Payload))return false;
        const uint32 Header[]={Magic,uint32(Payload.Num()),FCrc::MemCrc32(Payload.GetData(),Payload.Num())};
        Bytes.Append(reinterpret_cast<const uint8*>(Header),sizeof(Header));Bytes.Append(Payload);
        return FFileHelper::SaveArrayToFile(Bytes,*Path);
    };
    TestTrue(TEXT("Write pre-economy natural-world file"),WriteLegacy(0x48575332));
    UHearthwardSaveGame* Loaded=nullptr;FString Error;
    TestTrue(TEXT("Pre-economy natural save migrates"),HearthwardSave::Read(Path,Loaded,Error));
    if(Loaded)
    {
        TestEqual(TEXT("Player inventory preserved"),Loaded->Points[0].World.PlayerItems.Stacks.FindRef(TEXT("wood")),3);
        TestEqual(TEXT("Storage preserved"),Loaded->Points[0].World.StorageItems.Stacks.FindRef(TEXT("wood")),5);
        FHearthwardCampaignState Campaign;
        TestTrue(TEXT("Legacy campaign validates"),FHearthwardCampaignState::Parse(Loaded->Points[0].World.Campaign,Campaign));
        TestTrue(TEXT("Old progress skips new prologue"),Campaign.Legacy && Campaign.Facts.Contains(TEXT("prologue_skipped")));
        TestTrue(TEXT("Camp position carried into campaign"),Campaign.Positions.Contains(TEXT("camp")));
        TestTrue(TEXT("Migrated file can be saved"),HearthwardSave::Write(Path+TEXT(".roundtrip"),Loaded,Error));
        TestTrue(TEXT("Migrated file can be continued"),HearthwardSave::Read(Path+TEXT(".roundtrip"),Loaded,Error));
    }
    {
        TGuardValue<int32> LegacySchema(Pool->Schema,2),LegacyNPC(S.NPCStateVersion,2),LegacySurvival(S.SurvivalVersion,0),LegacyClock(S.ClockVersion,0);
        TGuardValue<int64> LegacyDay(S.InitialDay,0);TGuardValue<double> LegacyMinute(S.InitialMinute,-1.);
        bool Written=false;
        {
            TGuardValue<int32> OldClassDefault(GetMutableDefault<UHearthwardSaveGame>()->Schema,2);
            Written=WriteLegacy(0x48575332);
        }
        TestTrue(TEXT("Schema2 fixture serializes against its real historical class default"),Written);
        TestEqual(TEXT("Historical serialization restores today's class default"),GetDefault<UHearthwardSaveGame>()->Schema,HearthwardSave::CurrentSchema);
        TArray<uint8> Bytes,Payload;FFileHelper::LoadFileToArray(Bytes,*Path);
        if(Bytes.Num()>12)
        {
            Payload.Append(Bytes.GetData()+12,Bytes.Num()-12);
            const auto* Raw=Cast<UHearthwardSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
            if(TestNotNull(TEXT("Historical default-omitting payload uses standard GVAS deserialization"),Raw))
            {
                TestEqual(TEXT("Omitted schema really loads today's class default before migration"),Raw->Schema,HearthwardSave::CurrentSchema);
                TestEqual(TEXT("Old NPC version remains explicit before migration"),Raw->Points[0].World.NPCStateVersion,2);
                TestEqual(TEXT("Old fixture has no survival version"),Raw->Points[0].World.SurvivalVersion,0);
            }
        }
        const bool Migrated=HearthwardSave::Read(Path,Loaded,Error);
        TestTrue(FString(TEXT("Historical omitted schema2 migrates: "))+Error,Migrated);
        if(Loaded)
        {
            TestEqual(TEXT("Existing cognition migration promotes omitted schema2 NPC state"),Loaded->Points[0].World.NPCStateVersion,HearthwardSave::NPCStateVersion);
            TestEqual(TEXT("Omitted schema migration preserves player wood without rewards"),Loaded->Points[0].World.PlayerItems.Stacks.FindRef(TEXT("wood")),3);
            TestEqual(TEXT("Omitted schema migration preserves shared wood without rewards"),Loaded->Points[0].World.StorageItems.Stacks.FindRef(TEXT("wood")),5);
            TestTrue(TEXT("Omitted schema migration writes current-format snapshot"),HearthwardSave::Write(Path+TEXT(".default-schema.roundtrip"),Loaded,Error));
            TestTrue(TEXT("Omitted schema migration reloads current-format snapshot"),HearthwardSave::Read(Path+TEXT(".default-schema.roundtrip"),Loaded,Error));
        }
    }
    S.CampEconomy=TEXT("{broken");
    TestTrue(TEXT("Write damaged legacy economy"),WriteLegacy(0x48575332));
    TestFalse(TEXT("Present but malformed economy remains rejected"),HearthwardSave::Read(Path,Loaded,Error));
    S.CampEconomy.Reset();Pool->Schema=7;
    TestTrue(TEXT("Write schema7 with missing economy"),WriteLegacy(0x48575337));
    TestFalse(TEXT("Missing schema7 economy is not silently invented"),HearthwardSave::Read(Path,Loaded,Error));

    // Optional real-user fixture is supplied only as a local copy, never the live pool.
    FString Fixture;
    if(FParse::Value(FCommandLine::Get(),TEXT("HearthwardLegacySaveFixture="),Fixture))
    {
        TArray<uint8> Before,After;FFileHelper::LoadFileToArray(Before,*Fixture);
        const bool Read=HearthwardSave::Read(Fixture,Loaded,Error);
        if(!Read && Before.Num()>12)
        {
            TArray<uint8> Payload;Payload.Append(Before.GetData()+12,Before.Num()-12);
            const auto* Raw=Cast<UHearthwardSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
            if(Raw)
            {
                AddInfo(FString::Printf(TEXT("Legacy071 raw schema=%d points=%d writerPresent=%d"),Raw->Schema,Raw->Points.Num(),!Raw->WriterVersion.IsEmpty()));
                for(int32 Index=0;Index<Raw->Points.Num();++Index)
                {
                    const auto& P=Raw->Points[Index];const auto& W=P.World;
                    const bool Acquisition=W.Acquired>=0 && W.Carried>=0 && W.Acquired>=W.Delivered
                        && W.Acquired<=W.Requested && W.Carried==W.Acquired-W.Delivered;
                    AddInfo(FString::Printf(TEXT("Legacy071 point=%d survivalVersion=%d npcVersion=%d clockVersion=%d originDay=%lld originMinute=%.3f natural=%d companion=%d active=%.3f calendar=%.3f"),
                        Index,W.SurvivalVersion,W.NPCStateVersion,W.ClockVersion,W.InitialDay,W.InitialMinute,W.NaturalWorld,W.NaturalCompanion,W.ActiveSeconds,W.CalendarMinutes));
                    AddInfo(FString::Printf(TEXT("Legacy071 point=%d idsValid=%d memoryCampaignMatches=%d memoryValid=%d knowledgeCount=%d knowledgeRevision=%lld mapPresent=%d transformsValid=%d safety=%d phase=%d command=%d commandIdValid=%d goalValid=%d requested=%d delivered=%d acquired=%d carried=%d acquisitionValid=%d operations=%d receipts=%d"),
                        Index,P.SaveId.IsValid() && P.CampaignId.IsValid(),W.NPCMemory.Campaign==P.CampaignId,W.NPCMemory.IsValid(W.ActiveSeconds),
                        W.Knowledge.Num(),W.KnowledgeRevision,!W.Map.IsEmpty(),W.Player.IsValid() && W.Companion.IsValid() && W.Camp.IsValid() && W.Source.IsValid(),
                        W.Safety.CanSave(),int32(W.Phase),W.CommandActive,W.CommandId.IsValid(),W.AgentGoal.Intent.IsNone() || HearthwardAgent::Validate(W.AgentGoal).IsEmpty(),
                        W.Requested,W.Delivered,W.Acquired,W.Carried,Acquisition,W.NPCOperations.Num(),W.NPCReceipts.Num()));
                }
            }
        }
        TestTrue(FString(TEXT("Real historical save migrates: "))+Error,Read);
        if(Read)
        {
            AddInfo(FString::Printf(TEXT("Migrated %d real save points"),Loaded->Points.Num()));
            TestTrue(TEXT("Real migrated pool writes"),HearthwardSave::Write(Fixture+TEXT(".migrated"),Loaded,Error));
            TestTrue(TEXT("Real migrated pool reloads"),HearthwardSave::Read(Fixture+TEXT(".migrated"),Loaded,Error));
        }
        FFileHelper::LoadFileToArray(After,*Fixture);
        TestTrue(TEXT("Original fixture bytes unchanged"),Before==After);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveCompatibilityTest, "Hearthward.Save.CompatibilityPreviewAndConsent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveCompatibilityTest::RunTest(const FString& Parameters)
{
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("CompatibilityTests")/FGuid::NewGuid().ToString();
    IFileManager::Get().MakeDirectory(*Dir,true);
    const FString Source=Dir/TEXT("old.hws"),Destination=Dir/TEXT("new.hws");
    auto* Pool=NewObject<UHearthwardSaveGame>();Pool->Points.Add(Point());
    auto& S=Pool->Points[0].World;
    S.PlayerItems.Stacks.Add(TEXT("wood"),7);
    S.PlayerItems.Stacks.Add(TEXT("removed_item_fixture"),3);
    S.StorageItems.Stacks.Add(TEXT("stone"),11);
    const FGuid SaveId=Pool->Points[0].SaveId;
    auto WriteRaw=[&]()
    {
        TArray<uint8> Payload,Bytes;UGameplayStatics::SaveGameToMemory(Pool,Payload);
        const uint32 H[]={0x48575339,uint32(Payload.Num()),FCrc::MemCrc32(Payload.GetData(),Payload.Num())};
        Bytes.Append(reinterpret_cast<const uint8*>(H),12);Bytes.Append(Payload);return FFileHelper::SaveArrayToFile(Bytes,*Source);
    };
    TestTrue(TEXT("Create conflicting file"),WriteRaw());
    TArray<uint8> Before,After;FFileHelper::LoadFileToArray(Before,*Source);
    FHearthwardSaveCompatibility Preview;UHearthwardSaveGame* Candidate=nullptr;
    TestFalse(TEXT("Conflict requires consent"),HearthwardSave::InspectCompatibility(Source,Preview,Candidate));
    TestTrue(TEXT("Safe repair is available"),Preview.CanRepair && Candidate);
    TestEqual(TEXT("Exactly one specific conflict"),Preview.Changes.Num(),1);
    TestTrue(TEXT("Conflict names item and amount"),Preview.Changes[0].Contains(TEXT("removed_item_fixture")) && Preview.Changes[0].Contains(TEXT("×3")));
    FFileHelper::LoadFileToArray(After,*Source);
    TestTrue(TEXT("Preview/cancel never writes source"),Before==After && !IFileManager::Get().FileExists(*Destination));
    FString Status;
    TestTrue(TEXT("Explicit confirmation saves compatible copy"),HearthwardSave::ResolveCompatibility(Preview,Destination,Status));
    UHearthwardSaveGame* Read=nullptr;TestTrue(TEXT("Repaired copy loads"),HearthwardSave::Read(Destination,Read,Status));
    if(Read)
    {
        TestTrue(TEXT("Save identity preserved"),Read->Points[0].SaveId==SaveId);
        TestEqual(TEXT("Compatible personal items preserved"),Read->Points[0].World.PlayerItems.Stacks.FindRef(TEXT("wood")),7);
        TestEqual(TEXT("Compatible shared storage preserved"),Read->Points[0].World.StorageItems.Stacks.FindRef(TEXT("stone")),11);
        TestFalse(TEXT("Only obsolete item removed"),Read->Points[0].World.PlayerItems.Stacks.Contains(TEXT("removed_item_fixture")));
        TestEqual(TEXT("Writer version recorded"),Read->WriterVersion,FString(HearthwardVersion::Current));
    }
    FFileHelper::LoadFileToArray(After,*Source);TestTrue(TEXT("Original remains byte-identical after import"),Before==After);
    TArray<FString> Backups;IFileManager::Get().FindFiles(Backups,*(Dir/TEXT("Backups/*.hws")),true,false);
    TestEqual(TEXT("Verified backup created"),Backups.Num(),1);
    S.PlayerItems.Stacks[TEXT("removed_item_fixture")]=4;WriteRaw();
    TestFalse(TEXT("Stale preview rejected"),HearthwardSave::ResolveCompatibility(Preview,Dir/TEXT("stale.hws"),Status));
    Pool->WriterVersion=TEXT("99.0.0");WriteRaw();
    TestFalse(TEXT("Future save requires update"),HearthwardSave::InspectCompatibility(Source,Preview,Candidate));
    TestFalse(TEXT("Never delete progress to downgrade"),Preview.CanRepair);
    TestTrue(TEXT("Future save gives actionable update message"),Preview.Summary.Contains(TEXT("同步更新")));
    Pool->WriterVersion.Reset();S.PlayerItems.Stacks.Remove(TEXT("removed_item_fixture"));
    S.CampEconomy=TEXT("{damaged");WriteRaw();
    TestFalse(TEXT("Unrepairable structure is reported"),HearthwardSave::InspectCompatibility(Source,Preview,Candidate));
    TestFalse(TEXT("Cannot guess structural deletion"),Preview.CanRepair);
    TestTrue(TEXT("Affected domain identified"),Preview.Summary.Contains(TEXT("营地")));
    S.CampEconomy.Reset();
    S.Gameplay=NewObject<UHearthwardGameplayComponent>()->SaveSnapshot();
    TSharedPtr<FJsonObject> G;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S.Gameplay),G);
    G->GetObjectField(TEXT("skills"))->SetNumberField(TEXT("strong"),1);
    G->GetObjectField(TEXT("skills"))->SetNumberField(TEXT("removed_skill_fixture"),1);
    G->SetArrayField(TEXT("knownRecipes"),{MakeShared<FJsonValueString>(TEXT("removed_recipe_fixture"))});
    auto Building=MakeShared<FJsonObject>();Building->SetStringField(TEXT("id"),FGuid::NewGuid().ToString());
    Building->SetStringField(TEXT("recipe"),TEXT("removed_building_fixture"));Building->SetStringField(TEXT("position"),FVector(100,200,300).ToString());Building->SetNumberField(TEXT("yaw"),0);
    G->SetArrayField(TEXT("buildings"),{MakeShared<FJsonValueObject>(Building)});
    S.Gameplay.Reset();FJsonSerializer::Serialize(G.ToSharedRef(),TJsonWriterFactory<>::Create(&S.Gameplay));WriteRaw();
    TestFalse(TEXT("Content removals require consent"),HearthwardSave::InspectCompatibility(Source,Preview,Candidate));
    TestTrue(TEXT("Skill recipe and building repair is validated"),Preview.CanRepair && Candidate);
    TestEqual(TEXT("Every affected content record is listed"),Preview.Changes.Num(),3);
    if(Candidate)
    {
        TSharedPtr<FJsonObject> Kept;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Candidate->Points[0].World.Gameplay),Kept);
        TestEqual(TEXT("Compatible learned skill preserved"),Kept->GetObjectField(TEXT("skills"))->GetNumberField(TEXT("strong")),1.);
        TestEqual(TEXT("Compatible inventory still preserved"),Candidate->Points[0].World.PlayerItems.Stacks.FindRef(TEXT("wood")),7);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReleaseVersionTest, "Hearthward.Save.ReleaseVersionOrdering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReleaseVersionTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Higher patch"),HearthwardVersion::IsNewer(TEXT("v0.2.1"),TEXT("0.2.0")));
    TestTrue(TEXT("Stable replaces preview"),HearthwardVersion::IsNewer(TEXT("v0.2.0"),HearthwardVersion::Current));
    TestFalse(TEXT("Old demo is not an update"),HearthwardVersion::IsNewer(TEXT("v0.1.0-demo.20260924"),HearthwardVersion::Current));
    TestFalse(TEXT("Same version is not update"),HearthwardVersion::IsNewer(HearthwardVersion::Current,HearthwardVersion::Current));
    TestTrue(TEXT("Numeric prerelease order"),HearthwardVersion::IsNewer(TEXT("0.2.0-preview.10"),TEXT("0.2.0-preview.9")));
    TestFalse(TEXT("Unknown tag is not falsely newer"),HearthwardVersion::IsNewer(TEXT("main"),HearthwardVersion::Current));
    auto* Update=NewObject<UHearthwardUpdateSubsystem>(NewObject<UGameInstance>());
    Update->CompleteCheck(200,TEXT("{\"tag_name\":\"v0.3.0\"}"),true);
    TestTrue(TEXT("Official newer version produces update prompt"),Update->HasUpdate() && Update->GetNotice().Contains(TEXT("存在新版本，请同步更新")));
    Update->CompleteCheck(0,TEXT(""),false);
    TestTrue(TEXT("Offline recheck retains known newer release"),Update->HasUpdate());
    Update->CompleteCheck(200,TEXT("{\"tag_name\":\"v0.1.0\"}"),true);
    TestFalse(TEXT("Older release never prompts a downgrade"),Update->HasUpdate());
    Update->CompleteCheck(403,TEXT(""),true);
    TestTrue(TEXT("Rate limit has actionable offline fallback"),Update->GetNotice().Contains(TEXT("限流")));
    Update->CompleteCheck(200,TEXT("{\"tag_name\":\"main\"}"),true);
    TestTrue(TEXT("Unparseable tag is not falsely called current"),Update->GetNotice().Contains(TEXT("无法自动比较")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveClock9Test,"Hearthward.Save.Clock9OriginAndLegacyBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveClock9Test::RunTest(const FString&)
{
    struct FLegacyWorldArchive : FObjectAndNameAsStringProxyArchive
    {
        explicit FLegacyWorldArchive(FArchive& Inner):FObjectAndNameAsStringProxyArchive(Inner,false) {}
        virtual bool ShouldSkipProperty(const FProperty* Property) const override
        {
            const FName Name=Property->GetFName();
            return Name==TEXT("ClockVersion") || Name==TEXT("InitialDay") || Name==TEXT("InitialMinute");
        }
    };
    FHearthwardWorldSave Historical;Historical.ActiveSeconds=120;Historical.CalendarMinutes=6500;
    TArray<uint8> HistoricalBytes;FMemoryWriter HistoricalWriter(HistoricalBytes);
    FLegacyWorldArchive OldArchive(HistoricalWriter);Historical.Serialize(OldArchive);
    FMemoryReader HistoricalReader(HistoricalBytes);FObjectAndNameAsStringProxyArchive ReadArchive(HistoricalReader,false);
    FHearthwardWorldSave HistoricalLoaded;HistoricalLoaded.Serialize(ReadArchive);
    TestEqual(TEXT("Absent legacy clock metadata is not today's CDO default"),HistoricalLoaded.ClockVersion,0);
    TestEqual(TEXT("Absent legacy origin remains detectable"),HistoricalLoaded.InitialMinute,-1.);
    TestEqual(TEXT("Actual metadata-free tagged snapshot preserves W"),HistoricalLoaded.CalendarMinutes,6500.);
    const FString Path=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Task052"),FGuid::NewGuid().ToString()+TEXT(".hws"));
    auto* Pool=NewObject<UHearthwardSaveGame>();Pool->Points.Add(Point());auto& S=Pool->Points[0].World;
    S.ActiveSeconds=120;S.CalendarMinutes=6500;S.InitialDay=4;S.InitialMinute=1200;
    FHearthwardCampaignState Campaign;Campaign.Initialize(true);
    auto* E=Campaign.Enemies.FindByPredicate([](const auto& Enemy){return Enemy.Group==TEXT("field");});
    if(!TestNotNull(TEXT("Existing field enemy fixture"),E))return false;
    E->Combat.Health=0;E->RefreshDue=10080;const FName EnemyId=E->Id;const int32 Generation=E->Combat.Generation;S.Campaign=Campaign.Snapshot();
    FString Error;UHearthwardSaveGame* Loaded=nullptr;
    TestTrue(TEXT("Schema9 writes origin separately"),HearthwardSave::Write(Path,Pool,Error));
    TestTrue(TEXT("Schema9 reads tagged snapshot"),HearthwardSave::Read(Path,Loaded,Error));
    if(Loaded)
    {
        TestEqual(TEXT("Origin day round trip"),Loaded->Points[0].World.InitialDay,int64(4));
        TestEqual(TEXT("Origin minute round trip"),Loaded->Points[0].World.InitialMinute,1200.);
        TestEqual(TEXT("W is preserved when W exceeds A"),Loaded->Points[0].World.CalendarMinutes,6500.);
    }
    auto WriteEnvelope=[&](uint32 Magic)
    {
        TArray<uint8> Payload,Bytes;UGameplayStatics::SaveGameToMemory(Pool,Payload);
        const uint32 H[]={Magic,uint32(Payload.Num()),FCrc::MemCrc32(Payload.GetData(),Payload.Num())};
        Bytes.Append(reinterpret_cast<const uint8*>(H),12);Bytes.Append(Payload);FFileHelper::SaveArrayToFile(Bytes,*Path);
    };
    Pool->Schema=8;S.ClockVersion=0;S.InitialDay=0;S.InitialMinute=-1;WriteEnvelope(0x48575338);
    TestTrue(TEXT("Schema8 migration succeeds"),HearthwardSave::Read(Path,Loaded,Error));
    if(Loaded)
    {
        const auto& R=Loaded->Points[0].World;
        TestEqual(TEXT("Legacy display starts at midnight"),R.InitialMinute,0.);
        TestEqual(TEXT("Legacy elapsed W is not replaced by A"),R.CalendarMinutes,6500.);
        FHearthwardCampaignState Migrated;FHearthwardCampaignState::Parse(R.Campaign,Migrated);
        const auto* ME=Migrated.Enemies.FindByPredicate([&](const auto& Enemy){return Enemy.Id==EnemyId;});
        TestTrue(TEXT("Old due becomes next global boundary"),ME && ME->RefreshDue==5760);
        TestTrue(TEXT("Loading cannot materialize another generation"),ME && ME->Combat.Health==0 && ME->Combat.Generation==Generation);
        TestTrue(TEXT("Repeat legacy load is stable"),HearthwardSave::Read(Path,Loaded,Error));
        TestTrue(TEXT("Upgrade write preserves schema8 original backup"),HearthwardSave::Write(Path,Loaded,Error) && IFileManager::Get().FileExists(*(Path+TEXT(".pre-schema9"))));
    }
    WriteEnvelope(0x48575337);
    TestFalse(TEXT("Explicit schema8 payload cannot migrate under a schema7 envelope"),HearthwardSave::Read(Path,Loaded,Error));
    E->RefreshDue=4000;S.Campaign=Campaign.Snapshot();WriteEnvelope(0x48575338);
    TestFalse(TEXT("Indeterminate legacy death time cannot be guessed"),HearthwardSave::Read(Path,Loaded,Error));
    TestTrue(TEXT("Migration gives a specific conflict"),Error.Contains(TEXT("死亡时间")));
    Pool->Schema=9;S.Campaign.Reset();WriteEnvelope(0x48575339);
    TestFalse(TEXT("Malformed current clock cannot use a legacy fallback"),HearthwardSave::Read(Path,Loaded,Error));
    Pool->Schema=10;WriteEnvelope(0x48575339);
    TestFalse(TEXT("Future schema cannot migrate as legacy"),HearthwardSave::Read(Path,Loaded,Error));
    Pool->Schema=9;S.ClockVersion=1;S.InitialDay=1;S.InitialMinute=1200;WriteEnvelope(0x48575338);
    TestFalse(TEXT("Schema9 clock metadata cannot be silently downgraded by a schema8 envelope"),HearthwardSave::Read(Path,Loaded,Error));
    Pool->Schema=9;S.ClockVersion=1;S.InitialDay=4;S.InitialMinute=1200;WriteEnvelope(0x48575337);
    const bool AcceptedSchema7=HearthwardSave::Read(Path,Loaded,Error);
    TestFalse(TEXT("Explicit schema9 clock and origin cannot migrate under a schema7 envelope"),AcceptedSchema7);
    if(AcceptedSchema7 && Loaded)
        AddInfo(FString::Printf(TEXT("TASK071 mismatched envelope accepted: original origin=4/1200, loaded origin=%lld/%.0f"),
            Loaded->Points[0].World.InitialDay,Loaded->Points[0].World.InitialMinute));
    IFileManager::Get().Delete(*Path);IFileManager::Get().Delete(*(Path+TEXT(".pre-schema9")));
    return true;
}
#endif
