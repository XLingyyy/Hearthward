#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryState.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplaySnapshotTest,"Hearthward.Gameplay.SnapshotValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGameplaySnapshotTest::RunTest(const FString& Parameters)
{
    auto* Gameplay=NewObject<UHearthwardGameplayComponent>();
    const FString Original=Gameplay->SaveSnapshot();
    TestTrue(TEXT("Initial state round-trips"),UHearthwardGameplayComponent::ValidateSnapshot(Original));
    TestTrue(TEXT("Pre-020 saves remain accepted"),UHearthwardGameplayComponent::ValidateSnapshot(TEXT("")));
    TestFalse(TEXT("Truncated input rejected"),UHearthwardGameplayComponent::ValidateSnapshot(TEXT("{")));
    auto Corrupt=[&](const TCHAR* Label,TFunction<void(TSharedPtr<FJsonObject>)> Change)
    {
        TSharedPtr<FJsonObject> Root; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Original),Root);
        Change(Root); FString Json; FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
        TestFalse(Label,UHearthwardGameplayComponent::ValidateSnapshot(Json));
    };
    Corrupt(TEXT("Non-object explored point rejected"),[](auto J){J->SetArrayField(TEXT("explored"),{MakeShared<FJsonValueString>(TEXT("bad"))});});
    Corrupt(TEXT("Non-string location rejected"),[](auto J){J->SetArrayField(TEXT("activated"),{MakeShared<FJsonValueNumber>(1)});});
    Corrupt(TEXT("Unknown equipment rejected"),[](auto J){J->GetObjectField(TEXT("equipment"))->SetStringField(TEXT("weapon"),TEXT("missing"));});
    Corrupt(TEXT("Fractional rank rejected"),[](auto J){J->GetObjectField(TEXT("skills"))->SetNumberField(TEXT("strong"),.5);});
    Corrupt(TEXT("Invalid event counter rejected"),[](auto J){J->GetObjectField(TEXT("events"))->SetStringField(TEXT("learn:any"),TEXT("bad"));});
    Corrupt(TEXT("Hunger outside contract rejected"),[](auto J){J->SetNumberField(TEXT("hunger"),101);});
    Corrupt(TEXT("Unknown quest rejected"),[](auto J){J->SetStringField(TEXT("tracked"),TEXT("missing"));});
    Corrupt(TEXT("Unknown companion order rejected"),[](auto J){J->SetStringField(TEXT("companionOrder"),TEXT("teleport"));});
    Corrupt(TEXT("Malformed map marker rejected"),[](auto J){J->SetStringField(TEXT("waypoint"),TEXT("bad"));});
    FHearthwardInventoryState Bag;
    TestTrue(TEXT("Zero-weight key item can be owned"),Bag.Add(TEXT("amulet"),1)==EHearthwardInventoryResult::Success);
    TestEqual(TEXT("Key item does not consume capacity"),Bag.GetWeightHundredths(),int64(0));
    TestTrue(TEXT("Equipment uses real weight"),Bag.Add(TEXT("axe"),1)==EHearthwardInventoryResult::Success);
    TestEqual(TEXT("Axe weight remains 3.20"),Bag.GetWeightHundredths(),int64(320));
    return true;
}
#endif
