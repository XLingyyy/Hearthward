#include "../Save/HearthwardSaveGame.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Crc.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
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
        TestTrue(FString(TEXT("Real Demo save migrates: "))+Error,Read);
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
    E->RefreshDue=4000;S.Campaign=Campaign.Snapshot();WriteEnvelope(0x48575338);
    TestFalse(TEXT("Indeterminate legacy death time cannot be guessed"),HearthwardSave::Read(Path,Loaded,Error));
    TestTrue(TEXT("Migration gives a specific conflict"),Error.Contains(TEXT("死亡时间")));
    Pool->Schema=9;S.Campaign.Reset();WriteEnvelope(0x48575339);
    TestFalse(TEXT("Malformed current clock cannot use a legacy fallback"),HearthwardSave::Read(Path,Loaded,Error));
    Pool->Schema=10;WriteEnvelope(0x48575339);
    TestFalse(TEXT("Future schema cannot migrate as legacy"),HearthwardSave::Read(Path,Loaded,Error));
    Pool->Schema=9;S.ClockVersion=1;S.InitialDay=1;S.InitialMinute=1200;WriteEnvelope(0x48575338);
    TestFalse(TEXT("Schema9 clock metadata cannot be silently downgraded by a schema8 envelope"),HearthwardSave::Read(Path,Loaded,Error));
    IFileManager::Get().Delete(*Path);IFileManager::Get().Delete(*(Path+TEXT(".pre-schema9")));
    return true;
}
#endif
