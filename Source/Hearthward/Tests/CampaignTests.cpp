#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Campaign/HearthwardCampaignState.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Save/HearthwardSaveGame.h"
#include "../Camp/HearthwardCampState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Crc.h"
#include "HAL/FileManager.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignRegistryTest,"Hearthward.Campaign049.RegistryAndVictory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignRegistryTest::RunTest(const FString&)
{
    for(int32 Groups=0;Groups<=2;++Groups)
    {
        FHearthwardCampaignState S;S.Initialize();S.Phase=TEXT("occupied");
        TestTrue(TEXT("New registry validates"),S.Validate());
        if(Groups>0)TestTrue(TEXT("First alarm registers four"),S.RegisterReinforcement(TEXT("workshops")));
        if(Groups>1)TestTrue(TEXT("Second alarm registers four"),S.RegisterReinforcement(TEXT("assembly")));
        TestFalse(TEXT("Alarm retry cannot register again"),Groups>0 && S.RegisterReinforcement(TEXT("workshops")));
        TestEqual(TEXT("Denominator comes from identities"),S.Total(),80+4*Groups);
        const int32 Threshold=Groups==0?77:Groups==1?80:84;
        int32 Count=0;
        for(auto& E:S.Enemies)if((E.Group==TEXT("base") || E.Group==TEXT("reinforcement")) && Count++<Threshold-1)E.Combat.Health=0;
        TestFalse(TEXT("Strict 95 percent threshold"),S.ShowRemaining());
        for(auto& E:S.Enemies)if((E.Group==TEXT("base") || E.Group==TEXT("reinforcement")) && E.Combat.Health>0){E.Combat.Health=0;break;}
        TestTrue(TEXT("Threshold now crossed"),S.ShowRemaining());
        for(auto& E:S.Enemies)if(E.Group==TEXT("base") || E.Group==TEXT("reinforcement"))E.Combat.Health=0;
        S.ResolveUntriggered();TestFalse(TEXT("Kills alone do not win"),S.ReadyForVictory());
        for(const auto& V:HearthwardCampaign::Rows(TEXT("zones")))S.Flags.Add(FName(*HearthwardData::Text(V->AsObject(),TEXT("id"))));
        TestTrue(TEXT("All identities and flags win without quest claims"),S.ReadyForVictory());
        S.Victory=true;S.Phase=TEXT("reclaimed");
        FHearthwardCampaignState R;TestTrue(TEXT("Victory survives serialization"),FHearthwardCampaignState::Parse(S.Snapshot(),R));TestEqual(TEXT("Reload retains denominator"),R.Total(),S.Total());
        R.Enemies[0].Combat.Health=1;TestFalse(TEXT("Victory cannot conceal a survivor"),R.Validate());
    }
    FHearthwardCampaignState S;S.Initialize();S.Phase=TEXT("occupied");
    for(auto& E:S.Enemies)if(E.Zone==TEXT("workshops"))E.Combat.Health=0;
    TestFalse(TEXT("Last base kill precedes same-frame alarm"),S.RegisterReinforcement(TEXT("workshops")));
    TestEqual(TEXT("Cancelled is terminal"),S.Reinforcements[TEXT("workshops")],FName(TEXT("cancelled")));
    S.People[0].Id=TEXT("unknown");TestFalse(TEXT("Unknown rescued identity rejected"),S.Validate());
    S.Initialize(true,true);TestTrue(TEXT("Legacy hometown remains safe"),S.Validate());TestEqual(TEXT("Legacy victory does not invent kills"),S.Total(),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignContentTest,"Hearthward.Campaign049.ContentContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignContentTest::RunTest(const FString&)
{
    TestEqual(TEXT("23 formal quests"),HearthwardCampaign::Rows(TEXT("quests")).Num(),23);
    int32 Main=0,Side=0;TSet<FName> Rescues;
    for(const auto& V:HearthwardCampaign::Rows(TEXT("quests")))
    {
        const auto R=V->AsObject();const FString Id=HearthwardData::Text(R,TEXT("id"));
        const auto Runtime=HearthwardData::Find(TEXT("quests"),Id);
        TestTrue(TEXT("Formal quest exposed through existing journal catalog"),bool(Runtime));
        if(HearthwardData::Text(R,TEXT("category"))==TEXT("main"))++Main;else ++Side;
        TestEqual(TEXT("Runtime requirement matches real steps"),int32(HearthwardData::Number(Runtime,TEXT("required"))),R->GetArrayField(TEXT("steps")).Num());
        for(const auto& P:R->GetArrayField(TEXT("rescued_people")))Rescues.Add(FName(*P->AsString()));
        for(const auto& I:R->GetObjectField(TEXT("reward"))->GetObjectField(TEXT("items"))->Values)TestTrue(TEXT("Reward item exists"),bool(HearthwardData::Find(TEXT("items"),FString(*I.Key))));
    }
    TestEqual(TEXT("8 main"),Main,8);TestEqual(TEXT("15 side"),Side,15);TestEqual(TEXT("10 stable people"),Rescues.Num(),10);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignMigrationTest,"Hearthward.Campaign049.Schema7Migration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignMigrationTest::RunTest(const FString&)
{
    auto* Pool=NewObject<UHearthwardSaveGame>();Pool->Schema=7;
    FHearthwardSavePoint P;P.SaveId=FGuid::NewGuid();P.CampaignId=FGuid::NewGuid();P.Created=FDateTime::UtcNow();
    P.World.Map=TEXT("L_HearthwardWilds");P.World.NaturalWorld=P.World.NaturalCompanion=true;P.World.SurvivalVersion=1;
    P.World.NPCStateVersion=HearthwardSave::NPCStateVersion;P.World.NPCMemory.Campaign=P.CampaignId;
    FHearthwardCampState Camp;Camp.AddCamp(TEXT("camp"),FVector(-98000,-75000,16100));Camp.Rescue(TEXT("rescued_01"));P.World.CampEconomy=Camp.Snapshot();
    FHearthwardInventoryState Bag;Bag.Add(TEXT("amulet"),1);P.World.PlayerItems=Bag.Snapshot();Pool->Points.Add(P);
    const FString Path=FPaths::ProjectSavedDir()/TEXT("Task049")/(FGuid::NewGuid().ToString()+TEXT(".hws"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    auto Legacy=[&]()
    {
        TArray<uint8> Payload,Bytes;UGameplayStatics::SaveGameToMemory(Pool,Payload);
        const uint32 Header[]={0x48575337,uint32(Payload.Num()),FCrc::MemCrc32(Payload.GetData(),Payload.Num())};
        Bytes.Append(reinterpret_cast<const uint8*>(Header),sizeof(Header));Bytes.Append(Payload);FFileHelper::SaveArrayToFile(Bytes,*Path);return Bytes;
    };
    const auto Original=Legacy();UHearthwardSaveGame* Loaded=nullptr;FString Error;
    TestTrue(TEXT("Real schema7 envelope loads"),HearthwardSave::Read(Path,Loaded,Error));
    if(Loaded)
    {
        FHearthwardCampaignState S;TestTrue(TEXT("Migrated campaign validates"),FHearthwardCampaignState::Parse(Loaded->Points[0].World.Campaign,S));
        TestTrue(TEXT("Legacy skips prologue"),S.Legacy && S.Phase==TEXT("occupied"));TestEqual(TEXT("Existing rescue mapped"),S.People[0].Stage,FName(TEXT("arrived")));
        TestEqual(TEXT("Existing protected amulet retained"),Loaded->Points[0].World.PlayerItems.Stacks.FindRef(TEXT("amulet")),1);
        TestTrue(TEXT("Write migrated same path"),HearthwardSave::Write(Path,Loaded,Error));
        TArray<uint8> Backup;FFileHelper::LoadFileToArray(Backup,*(Path+TEXT(".pre-schema8")));TestTrue(TEXT("Schema7 original preserved byte for byte"),Backup==Original);
        TestTrue(TEXT("Migrated file reads back"),HearthwardSave::Read(Path,Loaded,Error));
    }
    Camp.Rescued[0]=TEXT("fixture_unknown");Pool->Points[0].World.CampEconomy=Camp.Snapshot();Legacy();
    TestFalse(TEXT("Unknown rescued identity blocks migration"),HearthwardSave::Read(Path,Loaded,Error));
    Camp.Rescued[0]=TEXT("rescued_01");Camp.Hometown=true;Camp.AddCamp(TEXT("hometown"),FVector(82000,62000,24000));Pool->Points[0].World.CampEconomy=Camp.Snapshot();Legacy();
    TestTrue(TEXT("Legacy second camp is retained"),HearthwardSave::Read(Path,Loaded,Error));
    if(Loaded){FHearthwardCampaignState S;FHearthwardCampaignState::Parse(Loaded->Points[0].World.Campaign,S);TestTrue(TEXT("Safe legacy hometown without fake kills"),S.LegacyHometown && S.Victory && S.Total()==0);TestEqual(TEXT("Existing camp location preserved"),S.Positions[TEXT("hometown")],FVector(82000,62000,24000));}
    IFileManager::Get().Delete(*Path);IFileManager::Get().Delete(*(Path+TEXT(".pre-schema8")));return true;
}
#endif
