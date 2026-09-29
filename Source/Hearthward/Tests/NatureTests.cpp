#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Nature/HearthwardNatureState.h"
#include "../Save/HearthwardSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Crc.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureGrowthTest,"Hearthward.Nature048.GrowthAndFeeding",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNatureGrowthTest::RunTest(const FString&)
{
    FHearthwardNatureState S;FHearthwardCrop C;C.Id=FGuid::NewGuid();C.Definition=TEXT("greens");C.Watered=C.Fertilized=true;S.Crops.Add(C);
    TestEqual(TEXT("Care adds once then floors"),S.Yield(C),6);S.Advance(2879);TestFalse(TEXT("Not ripe early"),S.Ready(C));S.Advance(1);TestTrue(TEXT("Two days ripe"),S.Ready(C));
    FHearthwardPen P;P.Id=FGuid::NewGuid();P.Definition=TEXT("goat");P.Feed=8;S.Pens.Add(P);
    for(int I=0;I<2;++I){FHearthwardAnimal A;A.Id=FGuid::NewGuid();A.Domestic=true;A.Captured=true;A.Pen=P.Id;A.Definition=P.Definition;A.Health=60;S.Animals.Add(A);}
    S.Advance(2880);TestEqual(TEXT("One birth per fed pair after two days"),S.Animals.Num(),3);TestEqual(TEXT("Only complete feed windows debited"),S.Pens[0].Feed,0);
    TestEqual(TEXT("Fed adult goats produce one milk per day"),S.Pens[0].Products,4);
    const FString Paused=S.Snapshot();S.Advance(10000);TestEqual(TEXT("No feed no further birth"),S.Animals.Num(),3);
    TestEqual(TEXT("No feed pauses ordinary products"),S.Pens[0].Products,4);
    auto* Baby=S.Animals.FindByPredicate([](const auto& A){return A.Juvenile;});TestTrue(TEXT("Unfed newborn does not grow"),Baby && Baby->Growth==0);
    FHearthwardNatureState Restored;TestTrue(TEXT("Round trip validates"),FHearthwardNatureState::Parse(Paused,Restored));TestEqual(TEXT("Restore retains feeding state"),Restored.Pens[0].Feed,0);
    S.Pens[0].Feed=100;S.Advance(2880);TestTrue(TEXT("Paid two days matures newborn"),S.Animals.ContainsByPredicate([](const auto& A){return A.Growth==2880 && !A.Juvenile;}));
    TestEqual(TEXT("Juvenile produces only after maturity"),S.Pens[0].Products,8);
    TestTrue(TEXT("State valid after births and growth"),S.Valid());
    FHearthwardNatureState H;FHearthwardPen Hen;Hen.Id=FGuid::NewGuid();Hen.Definition=TEXT("hen");Hen.Feed=100;H.Pens.Add(Hen);
    FHearthwardAnimal Bird;Bird.Id=FGuid::NewGuid();Bird.Domestic=true;Bird.Captured=true;Bird.Pen=Hen.Id;Bird.Definition=TEXT("hen");Bird.Health=20;H.Animals.Add(Bird);
    H.Advance(1440*30);TestEqual(TEXT("Pen product storage caps at 24"),H.Pens[0].Products,HearthwardNature::ProductCapacity);
    FHearthwardNatureState ProductRestored;TestTrue(TEXT("Product state round trip"),FHearthwardNatureState::Parse(H.Snapshot(),ProductRestored));
    TestEqual(TEXT("Stored eggs survive save"),ProductRestored.Pens[0].Products,HearthwardNature::ProductCapacity);
    TSharedPtr<FJsonObject> LegacyRoot;TestTrue(TEXT("Read prior nature JSON"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(H.Snapshot()),LegacyRoot));
    if(LegacyRoot)
    {
        for(const auto& Value:LegacyRoot->GetArrayField(TEXT("pens")))Value->AsObject()->RemoveField(TEXT("products"));
        for(const auto& Value:LegacyRoot->GetArrayField(TEXT("animals")))Value->AsObject()->RemoveField(TEXT("productMinutes"));
        FString LegacyJson;FJsonSerializer::Serialize(LegacyRoot.ToSharedRef(),TJsonWriterFactory<>::Create(&LegacyJson));
        FHearthwardNatureState Legacy;TestTrue(TEXT("Prior nature JSON loads with product defaults"),FHearthwardNatureState::Parse(LegacyJson,Legacy));
        if(Legacy.Pens.Num()==1)TestEqual(TEXT("No historical products invented"),Legacy.Pens[0].Products,0);
    }
    S.Pens[0].Feed=-1;TestFalse(TEXT("Corrupt feed rejected"),S.Valid());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureFishingTest,"Hearthward.Nature048.FishingInputAndRewards",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNatureFishingTest::RunTest(const FString&)
{
    for(double Required:{5.,6.,7.})for(double Bite:{2.,4.})
    {
        HearthwardNature::FFishing F;F.Required=Required;F.Bite=Bite;
        for(int I=0;I<1300 && !F.Done && !F.Failed;++I)F.Advance(.01,F.Tension<.48);
        TestTrue(TEXT("Controlled tension finishes every fish before timeout"),F.Done && !F.Failed && F.Elapsed<=12.01);
    }
    HearthwardNature::FFishing Hold;Hold.Advance(12,true);TestTrue(TEXT("Continuous hold breaks line"),Hold.Failed);
    HearthwardNature::FFishing Release;Release.Advance(12,false);TestTrue(TEXT("Continuous release loses fish"),Release.Failed);
    const FName R=HearthwardNature::Reward(48,TEXT("bank"),71);TestEqual(TEXT("Saved success ordinal yields identical reward"),HearthwardNature::Reward(48,TEXT("bank"),71),R);
    int32 Rewards=0;for(int32 I=0;I<10000;++I)if(!HearthwardNature::Reward(48,TEXT("bank"),I).IsNone())++Rewards;
    TestTrue(TEXT("Fixed seed sample within two percent tolerance"),Rewards>140 && Rewards<260);
    FHearthwardNatureState S;FHearthwardNaturePoint P;P.Id=FGuid::NewGuid();P.Key=TEXT("bank");P.Kind=TEXT("fish");P.Definition=TEXT("carp");P.Due=2880;S.Points.Add(P);
    S.Advance(14400);TestEqual(TEXT("Long skip restores only one stock"),S.Points[0].Remaining,24);TestEqual(TEXT("Consumed cooldown clears"),S.Points[0].Due,-1.);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureSaveTest,"Hearthward.Nature048.Schema6MigrationAndDuplicateLoot",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNatureSaveTest::RunTest(const FString&)
{
    auto* Pool=NewObject<UHearthwardSaveGame>();Pool->Schema=6;FHearthwardSavePoint P;P.SaveId=FGuid::NewGuid();P.CampaignId=FGuid::NewGuid();P.Created=FDateTime::UtcNow();
    P.World.Map=TEXT("PROTOTYPE_ONLY");P.World.SurvivalVersion=1;P.World.NPCStateVersion=HearthwardSave::NPCStateVersion;P.World.NPCMemory.Campaign=P.CampaignId;Pool->Points.Add(P);
    TArray<uint8> Payload,Bytes;UGameplayStatics::SaveGameToMemory(Pool,Payload);const uint32 Header[]={0x48575336,uint32(Payload.Num()),FCrc::MemCrc32(Payload.GetData(),Payload.Num())};Bytes.Append(reinterpret_cast<const uint8*>(Header),sizeof(Header));Bytes.Append(Payload);
    const FString Path=FPaths::ProjectSavedDir()/TEXT("Task048")/(FGuid::NewGuid().ToString()+TEXT(".hws"));IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);FFileHelper::SaveArrayToFile(Bytes,*Path);
    UHearthwardSaveGame* New=nullptr;FString Error;TestTrue(TEXT("Read actual schema6 envelope"),HearthwardSave::Read(Path,New,Error));
    if(New)
    {
        TestEqual(TEXT("Migrated to current schema"),New->Schema,HearthwardSave::CurrentSchema);TestTrue(TEXT("Save at original path"),HearthwardSave::Write(Path,New,Error));
        TArray<uint8> Backup;FFileHelper::LoadFileToArray(Backup,*(Path+TEXT(".pre-schema8")));TestTrue(TEXT("Original schema6 preserved"),Backup==Bytes);
        FHearthwardNatureState S;FHearthwardNaturePoint Point;Point.Id=FGuid::NewGuid();Point.Key=TEXT("bank");Point.Kind=TEXT("fish");Point.Definition=TEXT("carp");Point.Remaining=24;
        FHearthwardInventoryState Items;Items.Add(TEXT("bow_2"),1);Point.Pending=Items.Snapshot();S.Points.Add(Point);New->Points[0].World.Nature=S.Snapshot();
        TestTrue(TEXT("Pending exact instances save"),HearthwardSave::Validate(*New));New->Points[0].World.PlayerItems=Items.Snapshot();
        TestFalse(TEXT("Pending loot cannot duplicate a carried GUID"),HearthwardSave::Validate(*New));
    }
    IFileManager::Get().Delete(*Path);IFileManager::Get().Delete(*(Path+TEXT(".pre-schema8")));return true;
}
#endif
