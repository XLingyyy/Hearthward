#include "../Inventory/HearthwardInventoryState.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Building/HearthwardWorkshopService.h"
#include "../Gameplay/HearthwardProgression.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Save/HearthwardSaveGame.h"
#include "Misc/AutomationTest.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Crc.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentInstancesTest,"Hearthward.Inventory.Equipment047IdentityRepair",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEquipmentInstancesTest::RunTest(const FString&)
{
    FHearthwardInventoryState A,B(true);
    TestTrue(TEXT("Two independent axes"),A.Add(TEXT("axe"),2)==EHearthwardInventoryResult::Success);
    const FGuid First=A.Snapshot().Instances[0].Id,Second=A.Snapshot().Instances[1].Id;
    TestTrue(TEXT("Distinct identities"),First!=Second);
    TestTrue(TEXT("Equip chosen copy"),A.Equip(First));
    A.Wear(First,79.75);A.Wear(Second,20);
    TestTrue(TEXT("Fractional remaining durability"),FMath::IsNearlyEqual(A.FindInstance(First)->Durability,.25));
    TestTrue(TEXT("Last valid action may break"),A.Wear(First,2));
    TestEqual(TEXT("Broken retained with zero durability"),A.FindInstance(First)->Durability,0.);
    TestEqual(TEXT("Other copy unaffected"),A.FindInstance(Second)->Durability,60.);
    TestTrue(TEXT("Transfer exact broken copy"),A.TransferInstanceTo(B,First)==EHearthwardInventoryResult::Success);
    TestFalse(TEXT("Transferred gear unequipped"),A.EquippedInstance(TEXT("weapon")).IsValid());
    TestEqual(TEXT("Transfer retains zero"),B.FindInstance(First)->Durability,0.);
    TestTrue(TEXT("Return same identity"),B.TransferInstanceTo(A,First)==EHearthwardInventoryResult::Success);
    auto* Bag=NewObject<UHearthwardInventoryComponent>();Bag->RestoreInventory(A.Snapshot(),false);
    TMap<FName,int32> Cost;double Restore;
    TestTrue(TEXT("Quarter repair quote"),HearthwardWorkshop::RepairQuote(Bag,First,.25,Cost,Restore));
    TestEqual(TEXT("Quarter of axe maximum"),Restore,20.);
    TestFalse(TEXT("No free repair without materials"),Bag->RepairInstance(First,Restore,Cost));
    for(const auto& M:Cost)Bag->TryAdd(M.Key,M.Value);
    TestTrue(TEXT("Atomic paid quarter repair"),Bag->RepairInstance(First,Restore,Cost));
    TestEqual(TEXT("Only selected copy restored"),Bag->FindInstance(First)->Durability,20.);
    TestEqual(TEXT("Second copy still unchanged"),Bag->FindInstance(Second)->Durability,60.);
    TestTrue(TEXT("Upgrade capacity"),Bag->UpgradeBackpack());TestEqual(TEXT("Rank2 capacity"),Bag->GetCapacity(),150.);
    auto Snapshot=Bag->Snapshot();const auto Duplicate=Snapshot.Instances[0];Snapshot.Instances.Add(Duplicate);
    TestFalse(TEXT("Duplicate identity rejected"),FHearthwardInventoryState::Validate(Snapshot,false));
    FHearthwardInventoryState Unique(true);Unique.Add(TEXT("hearth_blade"),1);
    TestFalse(TEXT("Unique claim cannot duplicate in container"),Unique.Add(TEXT("hearth_blade"),1)==EHearthwardInventoryResult::Success);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgression047Test,"Hearthward.Gameplay.Progression047CurvesMigration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FProgression047Test::RunTest(const FString&)
{
    TestEqual(TEXT("New player level"),HearthwardProgression::Level(0),1);
    TestEqual(TEXT("First level threshold"),HearthwardProgression::Level(100),2);
    TestEqual(TEXT("Maximum cumulative XP"),HearthwardProgression::MaximumExperience(),65785);
    TestEqual(TEXT("Maximum level"),HearthwardProgression::Level(65785),60);
    TestEqual(TEXT("Starting skill budget"),HearthwardProgression::Budget(1),2);
    TestEqual(TEXT("Final skill budget"),HearthwardProgression::Budget(60),52);
    TestEqual(TEXT("Final level health bonus"),HearthwardProgression::Attribute(60,TEXT("hp_bonus")),100.f);
    auto* Legacy=NewObject<UHearthwardSaveGame>();Legacy->Schema=5;
    FHearthwardSavePoint P;P.SaveId=FGuid::NewGuid();P.CampaignId=FGuid::NewGuid();P.World.Inventory.Add(TEXT("axe"),2);P.World.Inventory.Add(TEXT("gloves"),1);P.World.Bag.Add(TEXT("axe"),1);P.World.NPCDurability.Add(TEXT("axe"),75);
    auto* G=NewObject<UHearthwardGameplayComponent>();G->Skills.Add(TEXT("strong"),1);G->Health=35;G->Stamina=22;
    TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(G->SaveSnapshot()),J);
    J->GetObjectField(TEXT("durability"))->SetNumberField(TEXT("axe"),25);J->GetObjectField(TEXT("equipment"))->SetStringField(TEXT("weapon"),TEXT("axe"));
    FJsonSerializer::Serialize(J.ToSharedRef(),TJsonWriterFactory<>::Create(&P.World.Gameplay));Legacy->Points.Add(P);
    auto* Again=DuplicateObject<UHearthwardSaveGame>(Legacy,GetTransientPackage());FString Error;
    TestTrue(TEXT("Schema5 migration"),HearthwardSave::MigrateInventory(*Legacy,Error));
    TestTrue(TEXT("Repeated migration of original"),HearthwardSave::MigrateInventory(*Again,Error));
    const auto& S=Legacy->Points[0].World;
    TestEqual(TEXT("Every axe preserves old percentage"),S.PlayerItems.Instances[0].Durability,20.);
    TestEqual(TEXT("Brother own durability wins"),S.BrotherItems.Instances[0].Durability,60.);
    TestTrue(TEXT("Deterministic migrated identity"),S.PlayerItems.Instances[0].Id==Again->Points[0].World.PlayerItems.Instances[0].Id);
    TestFalse(TEXT("Legacy gloves do not become leggings"),S.PlayerItems.Instances.ContainsByPredicate([](const auto& I){return I.Definition==TEXT("leggings");}));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S.Gameplay),J);
    TestTrue(TEXT("Old allocation refunded"),J->GetObjectField(TEXT("skills"))->Values.IsEmpty());
    TestEqual(TEXT("Migration does not heal"),J->GetNumberField(TEXT("health")),35.);
    TestEqual(TEXT("Migration does not refill stamina"),J->GetNumberField(TEXT("stamina")),22.);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentWorkshop047Test,"Hearthward.Inventory.Equipment047SharedAtomicity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEquipmentWorkshop047Test::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Actor=World->SpawnActor<AActor>();auto* Bag=NewObject<UHearthwardInventoryComponent>(Actor);Actor->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Store=World->GetSubsystem<UHearthwardStorageSubsystem>();
    Bag->TryAdd(TEXT("wood"),1);Store->Adjust({},{{TEXT("wood"),5}});
    TestTrue(TEXT("Bag first warehouse fills missing material"),Store->Workshop(Bag,{{TEXT("wood"),2}},{{TEXT("rope"),1}},true));
    TestEqual(TEXT("Own material consumed first"),Bag->GetItemCount(TEXT("wood")),0);
    TestEqual(TEXT("Only one from warehouse"),Store->GetItemCount(TEXT("wood")),4);
    TestFalse(TEXT("Missing second material rejects entire exchange"),Store->Workshop(Bag,{{TEXT("wood"),2},{TEXT("stone"),1}},{{TEXT("rope"),1}},true));
    TestEqual(TEXT("Warehouse unchanged on failure"),Store->GetItemCount(TEXT("wood")),4);
    TestEqual(TEXT("No partial output"),Bag->GetItemCount(TEXT("rope")),1);
    TestFalse(TEXT("Net capacity failure rolls back both containers"),Store->Workshop(Bag,{{TEXT("wood"),1}},{{TEXT("stone"),101}},true));
    TestEqual(TEXT("Capacity failure preserves cost"),Store->GetItemCount(TEXT("wood")),4);
    Bag->TryAdd(TEXT("axe"),1);const FGuid Id=Bag->FirstInstance(TEXT("axe"));Bag->WearInstance(Id,12.5);
    const FGuid Op=FGuid::NewGuid(),Epoch=Store->GetTimelineEpoch();
    TestTrue(TEXT("Instance deposit"),Store->TransferInstance(Bag,true,Id,Op,Epoch).Result==EHearthwardInventoryResult::Success);
    TestTrue(TEXT("Same operation replays without second movement"),Store->TransferInstance(Bag,true,Id,Op,Epoch).Replayed);
    TestTrue(TEXT("Typed operation cannot alias instance receipt"),Store->Transfer(Bag,true,TEXT("axe"),1,Op,Epoch).Result==EHearthwardInventoryResult::OperationConflict);
    TestTrue(TEXT("Instance withdrawal"),Store->TransferInstance(Bag,false,Id,FGuid::NewGuid(),Epoch).Result==EHearthwardInventoryResult::Success);
    TestEqual(TEXT("Storage transfer retains fractional wear"),Bag->FindInstance(Id)->Durability,67.5);
    World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentMigrationFile047Test,"Hearthward.Save.Schema5Equipment047Backup",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEquipmentMigrationFile047Test::RunTest(const FString&)
{
    auto* Pool=NewObject<UHearthwardSaveGame>();Pool->Schema=5;
    FHearthwardSavePoint P;P.SaveId=FGuid::NewGuid();P.CampaignId=FGuid::NewGuid();P.Created=FDateTime::UtcNow();
    P.World.Map=TEXT("PROTOTYPE_ONLY");P.World.SurvivalVersion=1;P.World.NPCStateVersion=HearthwardSave::NPCStateVersion;P.World.NPCMemory.Campaign=P.CampaignId;
    P.World.Inventory.Add(TEXT("axe"),2);Pool->Points.Add(P);
    TArray<uint8> Payload,Bytes;UGameplayStatics::SaveGameToMemory(Pool,Payload);
    const uint32 Header[]={0x48575335,uint32(Payload.Num()),FCrc::MemCrc32(Payload.GetData(),Payload.Num())};
    Bytes.Append(reinterpret_cast<const uint8*>(Header),sizeof(Header));Bytes.Append(Payload);
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("Task047/migration");IFileManager::Get().MakeDirectory(*Directory,true);
    const FString Path=Directory/(FGuid::NewGuid().ToString()+TEXT(".hws"));FFileHelper::SaveArrayToFile(Bytes,*Path);
    UHearthwardSaveGame* Migrated=nullptr;FString Error;
    TestTrue(TEXT("Actual schema5 file migrates"),HearthwardSave::Read(Path,Migrated,Error));
    if(Migrated)
    {
        TestEqual(TEXT("Two physical instances"),Migrated->Points[0].World.PlayerItems.Instances.Num(),2);
        TestTrue(TEXT("Write migrated file at original path"),HearthwardSave::Write(Path,Migrated,Error));
        TArray<uint8> Backup;FFileHelper::LoadFileToArray(Backup,*(Path+TEXT(".pre-schema6")));
        TestTrue(TEXT("Exact original bytes kept before replacement"),Backup==Bytes);
        UHearthwardSaveGame* Reloaded=nullptr;TestTrue(TEXT("Migrated current file reads"),HearthwardSave::Read(Path,Reloaded,Error));
        if(Reloaded)TestTrue(TEXT("Identity survives disk"),Reloaded->Points[0].World.PlayerItems.Instances[0].Id==Migrated->Points[0].World.PlayerItems.Instances[0].Id);
        auto& World=Migrated->Points[0].World;World.StorageItems.Instances.Add(World.PlayerItems.Instances[0]);
        TestFalse(TEXT("Cross-container duplicate rejected"),HearthwardSave::Validate(*Migrated));World.StorageItems.Instances.Reset();
        FHearthwardGroundEquipment Ground;Ground.Transform=FTransform::Identity;Ground.Item=World.PlayerItems.Instances[0];World.GroundEquipment.Add(Ground);
        TestFalse(TEXT("Ground duplicate rejected"),HearthwardSave::Validate(*Migrated));World.GroundEquipment.Reset();
        Migrated->Schema=4;TestFalse(TEXT("Unknown schema4 rejected"),HearthwardSave::Validate(*Migrated));
        Migrated->Schema=7;TestFalse(TEXT("Future schema rejected"),HearthwardSave::Validate(*Migrated));
    }
    IFileManager::Get().Delete(*Path);IFileManager::Get().Delete(*(Path+TEXT(".pre-schema6")));return true;
}
#endif
