#include "../Save/HearthwardSaveGame.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Crc.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FHearthwardSavePoint Point(bool Manual = false)
{
    FHearthwardSavePoint P;
    P.SaveId = FGuid::NewGuid(); P.CampaignId = FGuid::NewGuid(); P.Created = FDateTime::UtcNow(); P.Manual = Manual;
    P.World.Map = TEXT("PROTOTYPE_ONLY");
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
    S.Inventory.Add(TEXT("wood"), 5); S.Storage.Add(TEXT("stone"), 120); S.Resource.Add(TEXT("wood"), 16);
    S.Knowledge.Add(TEXT("玩家原话（未核实）: 营地约定")); S.KnowledgeRevision = 1;
    S.ActiveSeconds = 123; S.Player.SetLocation(FVector(10,20,30));
    FString Error;
    TestTrue(TEXT("Write initial pool"), HearthwardSave::Write(Path, Pool, Error));
    UHearthwardSaveGame* Loaded = nullptr;
    TestTrue(TEXT("Read same snapshot from disk"), HearthwardSave::Read(Path, Loaded, Error));
    if (Loaded)
    {
        const auto& R = Loaded->Points[0].World;
        TestEqual(TEXT("Personal inventory"), R.Inventory.FindRef(TEXT("wood")), 5);
        TestEqual(TEXT("Unlimited shared storage"), R.Storage.FindRef(TEXT("stone")), 120);
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
    S.Inventory[TEXT("wood")] = 101;
    TestFalse(TEXT("Invalid capacity rejected before mutation"), HearthwardSave::Write(Path, Pool, Error));
    S.Inventory[TEXT("wood")] = 5;
    S.Knowledge.Add(TEXT("future exchange")); S.KnowledgeRevision = 2;
    TestTrue(TEXT("Atomic replacement of existing committed pool"), HearthwardSave::Write(Path, Pool, Error));
    TestTrue(TEXT("Read replaced pool"), HearthwardSave::Read(Path, Loaded, Error));
    if (Loaded) TestEqual(TEXT("Replacement includes knowledge"), Loaded->Points[0].World.Knowledge.Num(), 2);
    auto Corrupt = Original; Corrupt.Last() ^= 1;
    FFileHelper::SaveArrayToFile(Corrupt, *Path);
    TestFalse(TEXT("Payload corruption rejected"), HearthwardSave::Read(Path, Loaded, Error));
    Corrupt.SetNum(Corrupt.Num()/2); FFileHelper::SaveArrayToFile(Corrupt, *Path);
    TestFalse(TEXT("Truncated snapshot rejected"), HearthwardSave::Read(Path, Loaded, Error));
    IFileManager::Get().Delete(*Path); IFileManager::Get().Delete(*(Path + TEXT(".pending")));
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
    auto& S=Pool->Points[0].World; S.ActiveSeconds=20;
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
#endif
