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
    Pool->Schema = 1;
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
#endif
