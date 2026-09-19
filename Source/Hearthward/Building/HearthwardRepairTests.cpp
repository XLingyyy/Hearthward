#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRepairCatalogTest,"Hearthward.Gameplay.Repair.CatalogAndMaterialTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRepairCatalogTest::RunTest(const FString& Parameters)
{
    using namespace HearthwardData;
    TestTrue(TEXT("Prototype repair economy identified"),Text(Catalog()->GetObjectField(TEXT("repair")),TEXT("status")).StartsWith(TEXT("PROTOTYPE_ONLY")));
    TSet<FString> Seen;
    for(const auto& V:Rows(TEXT("repairRecipes")))
    {
        const auto R=V->AsObject(); const FString Id=Text(R,TEXT("id"));
        const auto Item=Find(TEXT("items"),Id);
        TestTrue(TEXT("Repair recipe has registered durable equipment"),Item.IsValid() && Number(Item,TEXT("durability"))>0 && !Text(Item,TEXT("slot")).IsEmpty());
        TestFalse(TEXT("No duplicate repair recipe"),Seen.Contains(Id)); Seen.Add(Id);
        const auto Cost=R->GetObjectField(TEXT("materials"));
        TestFalse(TEXT("Repair has an explicit cost"),Cost->Values.IsEmpty());
        for(const auto& M:Cost->Values)
        {
            const double N=M.Value->AsNumber();
            TestTrue(TEXT("Known repair material"),Find(TEXT("items"),FString(*M.Key)).IsValid());
            TestTrue(TEXT("Integral positive cost"),N>=1 && N==FMath::FloorToDouble(N) && N<=MAX_int32);
            TestTrue(TEXT("Does not consume the repaired item"),FString(*M.Key)!=Id);
        }
    }
    for(const auto& V:Rows(TEXT("items")))
        if(Number(V->AsObject(),TEXT("durability"))>0)
            TestTrue(TEXT("Current durable item has repair support"),Seen.Contains(Text(V->AsObject(),TEXT("id"))));
    auto* Bag=NewObject<UHearthwardInventoryComponent>(); Bag->TryAdd(TEXT("wood"),3);
    TestEqual(TEXT("Second missing material rejects full debit"),Bag->TryConsume({{TEXT("wood"),2},{TEXT("rope"),1}}),EHearthwardInventoryResult::InsufficientItems);
    TestEqual(TEXT("Wood retained on missing rope"),Bag->GetItemCount(TEXT("wood")),3);
    Bag->TryAdd(TEXT("rope"),1);
    TestEqual(TEXT("Whole repair cost consumed together"),Bag->TryConsume({{TEXT("wood"),2},{TEXT("rope"),1}}),EHearthwardInventoryResult::Success);
    TestEqual(TEXT("Correct remaining wood"),Bag->GetItemCount(TEXT("wood")),1);
    TestEqual(TEXT("Correct remaining rope"),Bag->GetItemCount(TEXT("rope")),0);
    return true;
}
#endif
