#include "HearthwardBuildingComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuildingSnapshotTest,"Hearthward.Gameplay.BuildingSnapshotCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBuildingSnapshotTest::RunTest(const FString& Parameters)
{
    const FString Old=NewObject<UHearthwardGameplayComponent>()->SaveSnapshot();
    TestTrue(TEXT("Pre-022 gameplay with no buildings field is valid"),UHearthwardGameplayComponent::ValidateSnapshot(Old));
    auto Row=MakeShared<FJsonObject>();
    Row->SetStringField(TEXT("id"),FGuid::NewGuid().ToString()); Row->SetStringField(TEXT("recipe"),TEXT("workbench"));
    Row->SetStringField(TEXT("position"),FVector(100,200,0).ToString()); Row->SetNumberField(TEXT("yaw"),45);
    TSharedPtr<FJsonObject> Root; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Old),Root);
    auto Valid=[&]() { FString Json; FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<>::Create(&Json)); return UHearthwardGameplayComponent::ValidateSnapshot(Json); };
    Root->SetArrayField(TEXT("buildings"),{MakeShared<FJsonValueObject>(Row)});
    TestTrue(TEXT("Completed building with stable ID round-trips"),Valid());
    Root->SetArrayField(TEXT("buildings"),{MakeShared<FJsonValueObject>(Row),MakeShared<FJsonValueObject>(Row)});
    TestFalse(TEXT("Duplicate building ID rejected before restore"),Valid());
    Root->SetArrayField(TEXT("buildings"),{MakeShared<FJsonValueObject>(Row)});
    Row->SetStringField(TEXT("recipe"),TEXT("missing")); TestFalse(TEXT("Unknown recipe rejected"),Valid());
    Row->SetStringField(TEXT("recipe"),TEXT("workbench")); Row->SetStringField(TEXT("position"),TEXT("bad")); TestFalse(TEXT("Invalid position rejected"),Valid());
    Row->SetStringField(TEXT("position"),FVector::ZeroVector.ToString()); Row->SetStringField(TEXT("yaw"),TEXT("bad")); TestFalse(TEXT("Invalid rotation rejected"),Valid());
    Root->SetArrayField(TEXT("buildings"),{MakeShared<FJsonValueNumber>(1)}); TestFalse(TEXT("Non-object entry rejected"),Valid());
    Root->SetStringField(TEXT("buildings"),TEXT("bad")); TestFalse(TEXT("Non-array field rejected"),Valid());
    return true;
}
#endif
