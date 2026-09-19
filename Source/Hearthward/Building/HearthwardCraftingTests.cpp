#include "../Inventory/HearthwardInventoryState.h"
#include "../Gameplay/HearthwardGameData.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCraftingTransactionTest,"Hearthward.Gameplay.Crafting.AtomicExchange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCraftingTransactionTest::RunTest(const FString& Parameters)
{
    using R=EHearthwardInventoryResult;
    FHearthwardInventoryState Bag;
    Bag.Add(TEXT("wood"),100);
    TestEqual(TEXT("Full backpack uses capacity released by ingredients"),Bag.Exchange({{TEXT("wood"),1}},{{TEXT("arrow"),4}},3),R::Success);
    TestEqual(TEXT("Batch ingredients"),Bag.GetCount(TEXT("wood")),97);
    TestEqual(TEXT("Batch products"),Bag.GetCount(TEXT("arrow")),12);
    const int64 Weight=Bag.GetWeightHundredths();
    TestEqual(TEXT("Missing second ingredient"),Bag.Exchange({{TEXT("wood"),1},{TEXT("stone"),1}},{{TEXT("rope"),1}},1),R::InsufficientItems);
    TestEqual(TEXT("No partial cost"),Bag.GetCount(TEXT("wood")),97);
    TestEqual(TEXT("No partial product"),Bag.GetCount(TEXT("rope")),0);
    TestEqual(TEXT("Final output exceeds capacity"),Bag.Exchange({{TEXT("wood"),1}},{{TEXT("ore"),10},{TEXT("rope"),1}},1),R::CapacityExceeded);
    TestEqual(TEXT("Failed exchange preserves complete weight"),Bag.GetWeightHundredths(),Weight);
    TestEqual(TEXT("Unknown output rolls back"),Bag.Exchange({{TEXT("wood"),1}},{{TEXT("absent"),1}},1),R::UnknownItem);
    TestEqual(TEXT("Zero batch"),Bag.Exchange({{TEXT("wood"),1}},{{TEXT("arrow"),4}},0),R::InvalidCount);
    TestEqual(TEXT("Negative batch"),Bag.Exchange({{TEXT("wood"),1}},{{TEXT("arrow"),4}},-1),R::InvalidCount);
    TestEqual(TEXT("Multiplication overflow"),Bag.Exchange({{TEXT("wood"),1}},{{TEXT("arrow"),4}},MAX_int32),R::QuantityOverflow);
    TestEqual(TEXT("Invalid recipe count"),Bag.Exchange({{TEXT("wood"),0}},{{TEXT("arrow"),4}},1),R::InvalidCount);
    TestEqual(TEXT("Empty recipe"),Bag.Exchange({},{{TEXT("arrow"),4}},1),R::InvalidArgument);
    TestEqual(TEXT("Every failure leaves original inventory"),Bag.GetWeightHundredths(),Weight);
    FHearthwardInventoryState Unlimited(true); Unlimited.Add(TEXT("wood"),1); Unlimited.Add(TEXT("arrow"),MAX_int32);
    TestEqual(TEXT("Existing quantity overflow"),Unlimited.Exchange({{TEXT("wood"),1}},{{TEXT("arrow"),1}},1),R::QuantityOverflow);
    TestEqual(TEXT("Overflow retains material"),Unlimited.GetCount(TEXT("wood")),1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCraftingCatalogTest,"Hearthward.Gameplay.Crafting.Catalog",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCraftingCatalogTest::RunTest(const FString& Parameters)
{
    using namespace HearthwardData;
    const auto Config=Catalog()->GetObjectField(TEXT("crafting"));
    TestTrue(TEXT("Prototype tuning identified"),Text(Config,TEXT("status")).StartsWith(TEXT("PROTOTYPE_ONLY")));
    TestTrue(TEXT("Usable reach"),Number(Config,TEXT("reach"))>0);
    const double Max=Number(Config,TEXT("maxBatches"));
    TestTrue(TEXT("Finite positive integer batch limit"),FMath::IsFinite(Max) && Max>=1 && Max<=MAX_int32 && Max==FMath::FloorToDouble(Max));
    TSet<FString> Seen;
    for(const auto& V:Rows(TEXT("craftingRecipes")))
    {
        const auto Recipe=V->AsObject(); const FString Id=Text(Recipe,TEXT("id"));
        TestTrue(TEXT("Unique nonempty recipe ID"),!Id.IsEmpty() && !Seen.Contains(Id)); Seen.Add(Id);
        for(const auto* Field:{TEXT("materials"),TEXT("outputs")})
        {
            const auto Entries=Recipe->GetObjectField(Field);
            TestFalse(TEXT("Nonempty recipe items"),Entries->Values.IsEmpty());
            for(const auto& E:Entries->Values)
            {
                const double Count=E.Value->AsNumber();
                TestTrue(TEXT("Registered item"),Find(TEXT("items"),FString(*E.Key)).IsValid());
                TestTrue(TEXT("Positive integral counts safe at maximum batch"),Count>=1 && Count==FMath::FloorToDouble(Count) && Count*Max<=MAX_int32);
            }
        }
    }
    TestTrue(TEXT("Available recipes"),Seen.Num()>0);
    return true;
}
#endif
