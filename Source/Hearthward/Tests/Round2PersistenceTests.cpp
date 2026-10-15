#include "../Interaction/HearthwardPersistentInteractionState.h"
#include "../Save/HearthwardSaveGame.h"
#include "../Save/HearthwardSaveCompatibility.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Misc/Crc.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2DisplayOwnershipTest,"Hearthward.Round2.Persistence.DisplayRequiresCompletedOwnerChoice",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2DisplayOwnershipTest::RunTest(const FString&)
{
    FHearthwardPersistentInteractionState State;FString Error;
    auto& Display=State.Displays.AddDefaulted_GetRef();Display.Id=TEXT("display_camp_1");
    Display.Camp=TEXT("camp");Display.Slot=TEXT("1");Display.Fact=TEXT("craftsman_tools");
    TestFalse(TEXT("Missing owner record cannot grant a display on load"),State.Validate(0,Error));
    auto& Person=State.People.AddDefaulted_GetRef();Person.Id=TEXT("civilian_initial_02");Person.Step=TEXT("met");
    TestFalse(TEXT("Meeting alone cannot grant the memento"),State.Validate(0,Error));
    Person.Step=TEXT("remembered");Person.Choice=TEXT("hometown");
    TestFalse(TEXT("Display camp must match the actual owner choice"),State.Validate(0,Error));
    Person.Choice=TEXT("camp");TestTrue(TEXT("Completed matching choice remains loadable"),State.Validate(0,Error));
    auto Duplicate=Display;Duplicate.Id=TEXT("display_camp_2");Duplicate.Slot=TEXT("2");State.Displays.Add(Duplicate);
    TestFalse(TEXT("A second slot cannot duplicate the memento"),State.Validate(0,Error));State.Displays.SetNum(1);
    State.Displays[0].Fact=NAME_None;State.People.Reset();TestTrue(TEXT("An empty display needs no owner record"),State.Validate(0,Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2EarlyEquipmentTest,"Hearthward.Round2.Persistence.EquipmentBeforeBeginPlay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2EarlyEquipmentTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Player=World->SpawnActor<ACharacter>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Game=NewObject<UHearthwardGameplayComponent>(Player);Player->AddInstanceComponent(Game);Game->RegisterComponent();
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    Bag->TryAdd(TEXT("axe"),1);const auto Id=Bag->FirstInstance(TEXT("axe"));Bag->EquipInstance(Id);
    Bag->WearInstance(Id,7);
    const auto Before=Bag->Snapshot();
    TestTrue(TEXT("Inventory can restore before gameplay subscribes"),!Game->HasBegunPlay() && Game->Equipment.IsEmpty());
    Player->DispatchBeginPlay();
    TestEqual(TEXT("BeginPlay projects existing equipped instance"),Game->Equipment.FindRef(TEXT("weapon")),FName(TEXT("axe")));
    TestTrue(TEXT("Attack power becomes available without re-equipping"),Game->AttackPower()>0);
    TestEqual(TEXT("Projection preserves exact instance durability"),double(Game->Durability.FindRef(TEXT("axe"))),Bag->FindInstance(Id)->Durability);
    TestEqual(TEXT("Projection preserves equipped GUID"),Bag->EquippedInstance(TEXT("weapon")),Id);
    TestEqual(TEXT("Projection never repairs equipment"),Bag->FindInstance(Id)->Durability,Before.Instances[0].Durability);
    Bag->WearInstance(Id,1);
    TestEqual(TEXT("Later inventory notifications remain subscribed"),double(Game->Durability.FindRef(TEXT("axe"))),Bag->FindInstance(Id)->Durability);
    return true;
}
namespace
{
FName FixtureId(int32 Kind) { return FName(*FString::Printf(TEXT("PROTOTYPE_ONLY_Round2_%d"),Kind)); }
struct FDefinitions
{
    FDefinitions()
    {
        for(int32 I=0;I<7;++I)
        {
            FHearthwardPersistentDefinition D;D.Kind=EHearthwardPersistentKind(I);
            D.Steps.Add(TEXT("met"));D.Choices.Add(TEXT("help"));D.Camp=TEXT("camp");D.Slot=TEXT("slot1");
            D.DisplayFacts.Add(TEXT("fixture_memento"));D.MaxTrailSamples=2;
            HearthwardPersistence::Register(FixtureId(I),D);
        }
    }
    ~FDefinitions(){for(int32 I=0;I<7;++I)HearthwardPersistence::UnregisterFixture(FixtureId(I));}
};
UHearthwardSaveGame* Fixture()
{
    auto* Pool=NewObject<UHearthwardSaveGame>();auto& P=Pool->Points.AddDefaulted_GetRef();
    P.SaveId=FGuid::NewGuid();P.CampaignId=FGuid::NewGuid();P.Created=FDateTime::UtcNow();
    P.World.Map=TEXT("PROTOTYPE_ONLY_Round2");P.World.SurvivalVersion=1;
    P.World.NPCStateVersion=HearthwardSave::NPCStateVersion;P.World.NPCMemory.Campaign=P.CampaignId;
    auto& S=P.World.Interactions;
    S.Lamps.AddDefaulted_GetRef().Id=FixtureId(0);S.Lamps[0].Lit=true;
    S.Shortcuts.AddDefaulted_GetRef().Id=FixtureId(1);S.Shortcuts[0].Open=true;
    S.Cooperation.AddDefaulted_GetRef().Id=FixtureId(2);S.Cooperation[0].Result=EHearthwardCooperationResult::Cleared;
    auto& Person=S.People.AddDefaulted_GetRef();Person.Id=FixtureId(3);Person.Step=TEXT("met");Person.Choice=TEXT("help");
    auto& Weather=S.Weather.AddDefaulted_GetRef();Weather.Id=FixtureId(4);Weather.Seed=131;Weather.Phase=EHearthwardWeatherPhase::Rain;
    auto& Trail=S.Trails.AddDefaulted_GetRef();Trail.Id=FixtureId(5);Trail.Generation=2;Trail.Samples.AddDefaulted_GetRef().Position=FVector(100,200,0);
    auto& Display=S.Displays.AddDefaulted_GetRef();Display.Id=FixtureId(6);Display.Camp=TEXT("camp");Display.Slot=TEXT("slot1");Display.Fact=TEXT("fixture_memento");
    return Pool;
}
FString FixturePath(){return FPaths::ProjectDir()/TEXT(".agent-local/qa/TASK-131/cross-process.hws");}
bool Payload(UHearthwardSaveGame* Pool,TArray<uint8>& Bytes){return UGameplayStatics::SaveGameToMemory(Pool,Bytes);}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2StateTest,"Hearthward.Round2.Persistence.SevenKindsAndBounds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2StateTest::RunTest(const FString&)
{
    FDefinitions Definitions;auto* Pool=Fixture();FString Error;
    TestTrue(TEXT("Seven typed fixture domains validate"),HearthwardSave::Validate(*Pool));
    auto& S=Pool->Points[0].World.Interactions;
    S.People[0].Choice=TEXT("unknown");TestFalse(TEXT("Unknown choices rejected"),S.Validate(0,Error));S.People[0].Choice=TEXT("help");
    S.Trails[0].Samples.AddDefaulted(2);TestFalse(TEXT("Registered sample bound enforced"),S.Validate(0,Error));S.Trails[0].Samples.SetNum(1);
    const auto DuplicateLamp=S.Lamps[0];S.Lamps.Add(DuplicateLamp);TestFalse(TEXT("Duplicate identity rejected"),S.Validate(0,Error));S.Lamps.SetNum(1);
    S.Lamps[0].Id=TEXT("unknown");TestFalse(TEXT("Unknown entity rejected"),S.Validate(0,Error));S.Lamps[0].Id=FixtureId(0);
    TestTrue(TEXT("Write seven kinds to the original pool format"),HearthwardSave::Write(FixturePath(),Pool,Error));
    return true;
}
namespace
{
FHearthwardPersistentInteractionState ProductionState(int32 Variant)
{
    FHearthwardPersistentInteractionState S;S.Revision=42+Variant;
    for(const TCHAR* Id:{TEXT("workshops_lamp_entry"),TEXT("cellar_lamp_entry")})
    {auto& V=S.Lamps.AddDefaulted_GetRef();V.Id=Id;V.Lit=(S.Lamps.Num()+Variant)%2==0;}
    auto& Shortcut=S.Shortcuts.AddDefaulted_GetRef();Shortcut.Id=TEXT("cellar_drop_ladder");Shortcut.Open=Variant>=2;
    auto& Cooperation=S.Cooperation.AddDefaulted_GetRef();Cooperation.Id=TEXT("cellar_beam");
    Cooperation.Result=Variant>=2?EHearthwardCooperationResult::Cleared:EHearthwardCooperationResult::Unchanged;
    for(const TCHAR* Id:{TEXT("civilian_initial_02"),TEXT("civilian_initial_03"),TEXT("civilian_initial_04")})
    {
        auto& P=S.People.AddDefaulted_GetRef();P.Id=Id;
        P.Step=Variant==0?TEXT("met"):Variant==1?TEXT("visited"):TEXT("remembered");
        if(Variant>=2)P.Choice=Variant==2?TEXT("camp"):TEXT("hometown");
    }
    auto& Weather=S.Weather.AddDefaulted_GetRef();Weather.Id=TEXT("world_light_rain");Weather.Seed=177;
    Weather.Phase=Variant%2?EHearthwardWeatherPhase::Rain:EHearthwardWeatherPhase::Clear;
    for(const TCHAR* Species:{TEXT("deer"),TEXT("boar")})
    {
        auto& T=S.Trails.AddDefaulted_GetRef();T.Id=FName(*FString::Printf(TEXT("nature_%s0"),Species));
        T.Species=Species;T.Animal=FGuid(1,2,3,S.Trails.Num());T.Generation=3;
        T.Samples.AddDefaulted_GetRef().Position=FVector(100,200,300);
    }
    if(Variant>=2)
    {
        const FName Camp=Variant==2?FName(TEXT("camp")):FName(TEXT("hometown"));int32 Slot=0;
        for(const TCHAR* Fact:{TEXT("craftsman_tools"),TEXT("herbalist_record"),TEXT("watcher_memento")})
        {
            auto& D=S.Displays.AddDefaulted_GetRef();D.Camp=Camp;D.Slot=FName(*FString::FromInt(++Slot));D.Fact=Fact;
            D.Id=FName(*FString::Printf(TEXT("display_%s_%d"),*Camp.ToString(),Slot));
        }
    }
    return S;
}
FString ProductionMatrixPath(int32 Variant)
{return FPaths::ProjectDir()/FString::Printf(TEXT(".agent-local/qa/TASK-131/production-matrix-20261015/phase-%d.hws"),Variant);}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2ProductionWrite,"Hearthward.Round2.ProductionMatrix.Write",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2ProductionWrite::RunTest(const FString&)
{
    for(int32 Variant=0;Variant<4;++Variant)
    {
        auto* Pool=Fixture();Pool->Points[0].World.Interactions=ProductionState(Variant);FString Error;
        TestTrue(FString::Printf(TEXT("Production identities phase %d writes atomically"),Variant),HearthwardSave::Write(ProductionMatrixPath(Variant),Pool,Error));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2ProductionRead,"Hearthward.Round2.ProductionMatrix.Read",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2ProductionRead::RunTest(const FString&)
{
    for(int32 Variant=0;Variant<4;++Variant)
    {
        UHearthwardSaveGame* Pool=nullptr;FString Error;
        if(!TestTrue(FString::Printf(TEXT("Independent process loads production phase %d"),Variant),HearthwardSave::Read(ProductionMatrixPath(Variant),Pool,Error)))continue;
        if(!TestEqual(TEXT("One isolated fixture checkpoint"),Pool->Points.Num(),1))continue;
        const auto Expected=ProductionState(Variant);const auto& Actual=Pool->Points[0].World.Interactions;
        TestTrue(TEXT("Every reflected field in all seven domains matches"),FHearthwardPersistentInteractionState::StaticStruct()->CompareScriptStruct(&Expected,&Actual,0));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2ReadTest,"Hearthward.Round2.Persistence.ReadIndependentFixture",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2ReadTest::RunTest(const FString&)
{
    FDefinitions Definitions;UHearthwardSaveGame* Pool=nullptr;FString Error;
    if(!TestTrue(TEXT("Independent process loads registered fixture: ")+Error,HearthwardSave::Read(FixturePath(),Pool,Error)))return false;
    if(!TestEqual(TEXT("One fixture point"),Pool->Points.Num(),1))return false;
    const auto& S=Pool->Points[0].World.Interactions;
    TestTrue(TEXT("Lamp and shortcut persisted"),S.Lamps.Num()==1 && S.Lamps[0].Lit && S.Shortcuts.Num()==1 && S.Shortcuts[0].Open);
    TestTrue(TEXT("Cooperation result persisted"),S.Cooperation.Num()==1 && S.Cooperation[0].Result==EHearthwardCooperationResult::Cleared);
    TestTrue(TEXT("Choice persisted"),S.People.Num()==1 && S.People[0].Choice==TEXT("help"));
    TestTrue(TEXT("Weather seed persisted"),S.Weather.Num()==1 && S.Weather[0].Seed==131 && S.Weather[0].Phase==EHearthwardWeatherPhase::Rain);
    TestTrue(TEXT("Bounded trail persisted"),S.Trails.Num()==1 && S.Trails[0].Generation==2 && S.Trails[0].Samples.Num()==1 && S.Trails[0].Samples[0].Position==FVector(100,200,0));
    TestTrue(TEXT("Display reference persisted"),S.Displays.Num()==1 && S.Displays[0].Fact==TEXT("fixture_memento"));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2MigrationTest,"Hearthward.Round2.Persistence.RealTask111Migration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2MigrationTest::RunTest(const FString&)
{
    const FString Source=FPaths::ProjectDir()/TEXT(".agent-local/qa/TASK-112/checkpoints/task111-original.hws");
    TArray<uint8> Original;
    if(!TestTrue(TEXT("Actual TASK-111 isolated checkpoint exists"),FFileHelper::LoadFileToArray(Original,*Source)))return false;
    TArray<uint8> RawPayload;RawPayload.Append(Original.GetData()+12,Original.Num()-12);
    auto* Raw=Cast<UHearthwardSaveGame>(UGameplayStatics::LoadGameFromMemory(RawPayload));
    UHearthwardSaveGame* Migrated=nullptr;FString Error;
    if(!TestTrue(TEXT("Real schema9 checkpoint migrates: ")+Error,HearthwardSave::Read(Source,Migrated,Error)) || !Raw)return false;
    TestEqual(TEXT("Schema upgraded once"),Migrated->Schema,10);
    for(const auto& P:Migrated->Points)TestTrue(TEXT("No new progress granted"),P.World.Interactions.IsEmpty());
    Raw->Schema=10;for(auto& P:Raw->Points)P.World.Interactions={};
    TArray<uint8> Expected,Actual,After;Payload(Raw,Expected);Payload(Migrated,Actual);
    TestTrue(TEXT("Every original reflected field preserved exactly"),Expected==Actual);
    FFileHelper::LoadFileToArray(After,*Source);TestTrue(TEXT("Source bytes untouched"),Original==After);
    const FString Copy=FPaths::ProjectDir()/TEXT(".agent-local/qa/TASK-131/task111-migrated.hws");
    TestTrue(TEXT("Migrated copy writes and verifies"),HearthwardSave::Write(Copy,Migrated,Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2TransactionTest,"Hearthward.Round2.Persistence.TransactionsAndEpoch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2TransactionTest::RunTest(const FString&)
{
    FDefinitions Definitions;
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Save=World->GetSubsystem<UHearthwardSaveSubsystem>();auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();
    auto* Target=World->SpawnActor<AActor>();
    TestTrue(TEXT("Stable fixture target registration"),Save->BindInteractionTarget(FixtureId(0),EHearthwardPersistentKind::Lamp,Target));
    auto Candidate=Save->InteractionState();Candidate.Lamps.AddDefaulted_GetRef().Id=FixtureId(0);Candidate.Lamps[0].Lit=true;
    auto Ticket=Save->BeginInteraction(FixtureId(0));
    TestFalse(TEXT("Missing materials do not commit"),Save->CommitInteraction(Ticket,Candidate,{{TEXT("wood"),1}}));
    TestTrue(TEXT("State and stock unchanged"),Save->InteractionState().IsEmpty() && Storage->GetItemCount(TEXT("wood"))==0);
    TestTrue(TEXT("No-cost approved interaction commits"),Save->CommitInteraction(Ticket,Candidate));
    TestFalse(TEXT("Duplicate completion cannot repeat reward"),Save->CommitInteraction(Ticket,Candidate,{},{{TEXT("wood"),1}}));
    TestEqual(TEXT("Duplicate added nothing"),Storage->GetItemCount(TEXT("wood")),0);
    Ticket=Save->BeginInteraction(FixtureId(0));Candidate=Save->InteractionState();Candidate.Lamps[0].Lit=false;
    Save->CancelInteraction();TestFalse(TEXT("Cancelled animation callback rejected"),Save->CommitInteraction(Ticket,Candidate));
    Ticket=Save->BeginInteraction(FixtureId(0));Storage->AdvanceTimeline();
    TestFalse(TEXT("Old navigation/model epoch rejected"),Save->CommitInteraction(Ticket,Candidate));Save->CancelInteraction();
    Ticket=Save->BeginInteraction(FixtureId(0));Target->Destroy();
    TestFalse(TEXT("Destroyed target rejected without mutation"),Save->CommitInteraction(Ticket,Candidate));
    TestTrue(TEXT("Last committed lamp still lit"),Save->InteractionState().Lamps[0].Lit);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRound2RejectedFileTest,"Hearthward.Round2.Persistence.RejectedFilesPreserveOriginal",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRound2RejectedFileTest::RunTest(const FString&)
{
    FDefinitions Definitions;auto* Pool=Fixture();FString Error;UHearthwardSaveGame* Loaded=nullptr;
    const FString Path=FPaths::ProjectDir()/TEXT(".agent-local/qa/TASK-131/rejection.hws");
    auto Envelope=[&](uint32 Magic){TArray<uint8> P,B;Payload(Pool,P);const uint32 H[]={Magic,uint32(P.Num()),FCrc::MemCrc32(P.GetData(),P.Num())};B.Append(reinterpret_cast<const uint8*>(H),12);B.Append(P);FFileHelper::SaveArrayToFile(B,*Path);return B;};
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    auto Original=Envelope(HearthwardSave::CurrentMagic+1);TestFalse(TEXT("Future envelope rejected"),HearthwardSave::Read(Path,Loaded,Error));
    TArray<uint8> After;FFileHelper::LoadFileToArray(After,*Path);TestTrue(TEXT("Future file unchanged"),After==Original);
    Pool->Points[0].World.Interactions.Lamps[0].Id=TEXT("unknown");Original=Envelope(HearthwardSave::CurrentMagic);
    TestFalse(TEXT("Unknown ID rejected"),HearthwardSave::Read(Path,Loaded,Error));
    FFileHelper::LoadFileToArray(After,*Path);TestTrue(TEXT("Unknown file unchanged"),After==Original);
    Original.Last()^=0x80;FFileHelper::SaveArrayToFile(Original,*Path);TestFalse(TEXT("Corrupt file rejected"),HearthwardSave::Read(Path,Loaded,Error));
    FFileHelper::LoadFileToArray(After,*Path);TestTrue(TEXT("Corrupt file unchanged"),After==Original);
    Pool->Points[0].World.Interactions.Lamps[0].Id=FixtureId(0);
    const FString Blocked=Path+TEXT(".pending");IFileManager::Get().MakeDirectory(*Blocked,true);
    ON_SCOPE_EXIT{IFileManager::Get().DeleteDirectory(*Blocked,false,false);};
    TestFalse(TEXT("Injected pending-write failure returns false"),HearthwardSave::Write(Path,Pool,Error));
    FFileHelper::LoadFileToArray(After,*Path);TestTrue(TEXT("Write failure leaves original bytes"),After==Original);
    for(int32 Case=0;Case<4;++Case)
    {
        auto& State=Pool->Points[0].World.Interactions;State=ProductionState(2);
        if(Case==0)State.People.RemoveAt(0);
        if(Case==1)State.People[0].Choice=TEXT("hometown");
        if(Case==2)State.Displays[0].Fact=TEXT("unknown_memento");
        if(Case==3){auto Duplicate=State.Displays[0];State.Displays.Add(Duplicate);}
        Original=Envelope(HearthwardSave::CurrentMagic);Error.Reset();Loaded=nullptr;
        TestFalse(FString::Printf(TEXT("Invalid display file %d is rejected"),Case),HearthwardSave::Read(Path,Loaded,Error));
        TestFalse(FString::Printf(TEXT("Invalid display file %d has a diagnostic"),Case),Error.IsEmpty());
        FFileHelper::LoadFileToArray(After,*Path);
        TestTrue(FString::Printf(TEXT("Invalid display file %d preserves original bytes"),Case),After==Original);
    }
    return true;
}
#endif
