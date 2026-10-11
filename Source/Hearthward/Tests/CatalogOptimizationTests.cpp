#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardCatalogIndex.h"
#include "../Campaign/HearthwardCampaignState.h"

namespace
{
TSharedPtr<FJsonObject> Row(const FString& Id)
{
    auto Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("id"), Id);
    return Result;
}
TSharedPtr<FJsonObject> RootWithRows(const TArray<TSharedPtr<FJsonObject>>& Rows)
{
    auto Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Values;
    for (const auto& R : Rows) Values.Add(MakeShared<FJsonValueObject>(R));
    Root->SetArrayField(TEXT("items"), Values);
    return Root;
}
TSharedPtr<FJsonObject> Linear(const TArray<TSharedPtr<FJsonValue>>& Rows, const FString& Id)
{
    for (const auto& V : Rows)
        if (V->AsObject()->GetStringField(TEXT("id")) == Id) return V->AsObject();
    return nullptr;
}
void AddEnemy(FHearthwardCampaignState& State, FName Group, float Health)
{
    FHearthwardCampaignEnemy Enemy;
    Enemy.Group = Group;
    Enemy.Combat.Health = Health;
    State.Enemies.Add(Enemy);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardIndexIdentityTest,
    "Hearthward.Engineering.Task105.Index.IdentityAndMiss",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHearthwardIndexIdentityTest::RunTest(const FString& Parameters)
{
    const auto Wood = Row(TEXT("wood"));
    const auto Root = RootWithRows({Wood, Row(TEXT("stone"))});
    const FHearthwardCatalogIndex Index(Root);
    TSharedPtr<FJsonObject> Found;
    TestTrue(TEXT("Known table is indexed"), Index.TryFind(TEXT("items"), TEXT("wood"), Found));
    TestTrue(TEXT("Returns original row, not a copy"), Found == Wood);
    TestTrue(TEXT("Missing ID is an indexed miss"), Index.TryFind(TEXT("items"), TEXT("missing"), Found));
    TestFalse(TEXT("Miss clears previous output"), Found.IsValid());
    TestEqual(TEXT("No source rows were added or removed"), Root->GetArrayField(TEXT("items")).Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardIndexLegacyTest,
    "Hearthward.Engineering.Task105.Index.DuplicatesAndCaseMatchLegacy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHearthwardIndexLegacyTest::RunTest(const FString& Parameters)
{
    const auto First = Row(TEXT("wood"));
    const auto Root = RootWithRows({First, Row(TEXT("wood")), Row(TEXT("WOOD")), Row(TEXT("石材"))});
    const FHearthwardCatalogIndex Index(Root);
    for (const FString Id : {FString(TEXT("wood")), FString(TEXT("WOOD")), FString(TEXT("Wood")), FString(TEXT("石材")), FString(TEXT("absent"))})
    {
        TSharedPtr<FJsonObject> Found;
        TestTrue(TEXT("Indexed table"), Index.TryFind(TEXT("items"), Id, Found));
        TestTrue(*FString::Printf(TEXT("Same FString equality and first match for %s"), *Id),
            Found == Linear(Root->GetArrayField(TEXT("items")), Id));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardIndexFallbackTest,
    "Hearthward.Engineering.Task105.Index.NonIdAndMalformedFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHearthwardIndexFallbackTest::RunTest(const FString& Parameters)
{
    const auto Root = RootWithRows({Row(TEXT("wood")), MakeShared<FJsonObject>()});
    Root->SetArrayField(TEXT("empty"), TArray<TSharedPtr<FJsonValue>>());
    Root->SetArrayField(TEXT("numbers"), {MakeShared<FJsonValueNumber>(1)});
    Root->SetStringField(TEXT("not_table"), TEXT("text"));
    const FHearthwardCatalogIndex Index(Root);
    TSharedPtr<FJsonObject> Found = Row(TEXT("stale"));
    for (const TCHAR* Name : {TEXT("items"), TEXT("numbers"), TEXT("not_table"), TEXT("unknown")})
    {
        TestFalse(TEXT("Caller retains original error/lookup path"), Index.TryFind(Name, TEXT("wood"), Found));
        TestFalse(TEXT("Fallback does not leak prior output"), Found.IsValid());
    }
    TestTrue(TEXT("Empty valid table is indexed"), Index.TryFind(TEXT("empty"), TEXT("wood"), Found));
    TestFalse(TEXT("Empty table misses"), Found.IsValid());
    const FHearthwardCatalogIndex NullIndex{TSharedPtr<FJsonObject>()};
    TestFalse(TEXT("Null root requires fallback"), NullIndex.TryFind(TEXT("items"), TEXT("wood"), Found));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardIndexIsolationTest,
    "Hearthward.Engineering.Task105.Index.RootIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHearthwardIndexIsolationTest::RunTest(const FString& Parameters)
{
    const auto A = Row(TEXT("wood")), B = Row(TEXT("wood"));
    const FHearthwardCatalogIndex First(RootWithRows({A})), Second(RootWithRows({B}));
    TSharedPtr<FJsonObject> Found;
    TestTrue(TEXT("First lookup"), First.TryFind(TEXT("items"), TEXT("wood"), Found));
    TestTrue(TEXT("First root identity"), Found == A);
    TestTrue(TEXT("Second lookup"), Second.TryFind(TEXT("items"), TEXT("wood"), Found));
    TestTrue(TEXT("Independent root identity"), Found == B);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardCatalogEquivalenceTest,
    "Hearthward.Engineering.Task105.Index.ProductionCatalogEquivalence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHearthwardCatalogEquivalenceTest::RunTest(const FString& Parameters)
{
    int32 Compared = 0;
    for (const TCHAR* Table : {TEXT("items"), TEXT("skills"), TEXT("craftingRecipes"), TEXT("repairRecipes"), TEXT("locations"), TEXT("quests"), TEXT("encounters")})
    {
        const auto& Values = HearthwardData::Rows(Table);
        for (const auto& Value : Values)
        {
            const FString Id = Value->AsObject()->GetStringField(TEXT("id"));
            TestTrue(TEXT("Production lookup matches original linear lookup"), HearthwardData::Find(Table, Id) == Linear(Values, Id));
            ++Compared;
        }
        const FString Missing(TEXT("__task105_missing_id__"));
        TestTrue(TEXT("Production miss matches original"), HearthwardData::Find(Table, Missing) == Linear(Values, Missing));
    }
    for (const TCHAR* Table : {TEXT("quests"), TEXT("locations"), TEXT("zones"), TEXT("enemies"), TEXT("people")})
    {
        const auto& Values = HearthwardCampaign::Rows(Table);
        for (const auto& Value : Values)
        {
            const FName Id(*Value->AsObject()->GetStringField(TEXT("id")));
            TestTrue(TEXT("Campaign lookup retains FName-to-FString behavior"), HearthwardCampaign::Find(Table, Id) == Linear(Values, Id.ToString()));
            ++Compared;
        }
    }
    TestTrue(TEXT("Compared real production rows"), Compared > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardCampaignCountTest,
    "Hearthward.Engineering.Task105.Campaign.CountsTrackCurrentState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHearthwardCampaignCountTest::RunTest(const FString& Parameters)
{
    FHearthwardCampaignState State;
    TestEqual(TEXT("Empty total"), State.Total(), 0);
    TestEqual(TEXT("Empty cleared"), State.Cleared(), 0);
    AddEnemy(State, TEXT("base"), 100);
    AddEnemy(State, TEXT("base"), 0);
    AddEnemy(State, TEXT("reinforcement"), -1);
    AddEnemy(State, TEXT("field"), 0);
    AddEnemy(State, TEXT("prologue"), 0);
    TestEqual(TEXT("Only base and reinforcement count"), State.Total(), 3);
    TestEqual(TEXT("Non-positive health keeps original predicate"), State.Cleared(), 2);
    State.Enemies[0].Combat.Health = 0;
    TestEqual(TEXT("Mutation is observed without cache invalidation"), State.Cleared(), 3);
    State.Enemies.RemoveAt(1);
    TestEqual(TEXT("Removal changes total immediately"), State.Total(), 2);
    State.Enemies[0].Group = TEXT("field");
    TestEqual(TEXT("Group change observed"), State.Total(), 1);
    TestEqual(TEXT("Group change observed by cleared"), State.Cleared(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHearthwardCampaignVictoryBoundaryTest,
    "Hearthward.Engineering.Task105.Campaign.VictoryAndStrict95Percent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHearthwardCampaignVictoryBoundaryTest::RunTest(const FString& Parameters)
{
    for (const int32 N : {80, 84, 88})
    {
        FHearthwardCampaignState State;
        for (int32 I = 0; I < N; ++I) AddEnemy(State, I < 80 ? FName(TEXT("base")) : FName(TEXT("reinforcement")), 100);
        const int32 Boundary = N * 95 / 100;
        for (int32 I = 0; I < Boundary; ++I) State.Enemies[I].Combat.Health = 0;
        TestFalse(TEXT("At or below 95 percent stays hidden"), State.ShowRemaining());
        State.Enemies[Boundary].Combat.Health = 0;
        TestTrue(TEXT("Strictly above 95 percent is shown"), State.ShowRemaining());
        for (auto& E : State.Enemies) E.Combat.Health = 0;
        for (const TCHAR* Zone : {TEXT("river_gate"), TEXT("workshops"), TEXT("dwellings"), TEXT("assembly")}) State.Flags.Add(FName(Zone));
        State.Reinforcements.Add(TEXT("workshops"), TEXT("pending"));
        State.Reinforcements.Add(TEXT("assembly"), TEXT("cancelled"));
        TestFalse(TEXT("Pending reinforcement prevents victory"), State.ReadyForVictory());
        State.Reinforcements[TEXT("workshops")] = TEXT("cancelled");
        TestTrue(TEXT("Original victory predicate still succeeds"), State.ReadyForVictory());
        State.Enemies.Last().Combat.Health = 1;
        TestFalse(TEXT("Living counted enemy prevents victory"), State.ReadyForVictory());
    }
    return true;
}
#endif
