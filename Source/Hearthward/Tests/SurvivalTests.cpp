#include "../Survival/HearthwardSurvivalState.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurvivalBoundaryTest,"Hearthward.Survival.CalendarAndDeadlines",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSurvivalBoundaryTest::RunTest(const FString&)
{
    FHearthwardSurvivalState S; float HP=100,Hunger=100;
    S.Advance(HP,Hunger,100,0,2880,0);
    TestEqual(TEXT("Two calendar days empty hunger"),Hunger,0.f);
    TestEqual(TEXT("Skip supplies no passive healing"),HP,100.f);
    S.Advance(HP,Hunger,100,0,300,2880);
    TestTrue(TEXT("Five hours drain to ten percent"),FMath::IsNearlyEqual(HP,10.f));
    TestTrue(TEXT("Deadline starts at threshold"),FMath::IsNearlyEqual(S.SevereDue,7500.));
    HP=80; Hunger=5; S.Food(Hunger);
    TestTrue(TEXT("Treatment and small meal preserve severe deadline"),S.SevereDue==7500);
    const double Fraction=S.Advance(HP,Hunger,100,0,5000,3180);
    TestTrue(TEXT("Skip stops at fatal boundary"),FMath::IsNearlyEqual(Fraction,4320./5000));
    TestTrue(TEXT("Severe deadline causes true death"),S.Life==EHearthwardLife::Dead);
    S={}; HP=10; Hunger=0; S.Advance(HP,Hunger,100,0,0,100);
    TestEqual(TEXT("Initial threshold uses current time"),S.SevereDue,4420.);
    Hunger=10; S.Food(Hunger); TestFalse(TEXT("Ten food clears severe"),S.Severe());
    S.Damage(HP,10); TestTrue(TEXT("First lethal hit downs"),S.Life==EHearthwardLife::Downed);
    S.Advance(HP,Hunger,100,119,119,100);
    TestTrue(TEXT("Still rescuable before due"),S.Life==EHearthwardLife::Downed && S.DownRemaining==1);
    S.Advance(HP,Hunger,100,1,1,219);
    TestTrue(TEXT("Exactly 120 seconds is death"),S.Life==EHearthwardLife::Dead);
    S={}; HP=1; S.Damage(HP,1); S.Damage(HP,1);
    TestTrue(TEXT("Fresh residual damage kills downed"),S.Life==EHearthwardLife::Dead);
    S={}; HP=20; Hunger=100; S.HotRemaining=15; S.HotRate=50./15;
    S.Advance(HP,Hunger,100,0,480,0);
    TestEqual(TEXT("Sleep preserves sustained duration"),S.HotRemaining,15.);
    TestEqual(TEXT("Sleep does not apply medicine"),HP,20.f);
    S.Advance(HP,Hunger,100,15,15,480,1,0);
    TestTrue(TEXT("Sustained dose heals fifty percent over active time"),FMath::IsNearlyEqual(HP,70.f));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurvivalMedicineTest,"Hearthward.Survival.MedicineReservationAndDamage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSurvivalMedicineTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Actor=World->SpawnActor<AActor>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Actor); Actor->AddInstanceComponent(Bag); Bag->RegisterComponent();
    auto* G=NewObject<UHearthwardGameplayComponent>(Actor); Actor->AddInstanceComponent(G); G->RegisterComponent(); G->Enabled=true; G->Health=20;
    auto* S=NewObject<UHearthwardSurvivalComponent>(Actor); Actor->AddInstanceComponent(S); S->RegisterComponent();
    auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();
    Bag->TryAdd(TEXT("medicine"),1);
    TestTrue(TEXT("Medicine starts without debit"),S->BeginMedicine(TEXT("medicine")));
    TestEqual(TEXT("Reserved dose still weighs half unit"),Bag->GetWeight(),.5);
    TestTrue(TEXT("Reserved dose cannot be dropped"),Bag->TryRemove(TEXT("medicine"),1)==EHearthwardInventoryResult::InsufficientItems);
    TestTrue(TEXT("Reserved dose cannot enter a recipe"),Bag->TryConsume({{TEXT("medicine"),1}})==EHearthwardInventoryResult::InsufficientItems);
    TestEqual(TEXT("Storage cannot transfer reserved dose"),Storage->Transfer(Bag,true,TEXT("medicine"),1,FGuid::NewGuid(),Storage->GetTimelineEpoch()).MovedCount,0);
    const FGuid Hit=FGuid::NewGuid();
    S->CancelAction();
    TestTrue(TEXT("Damage wins over same-frame manual cancellation"),S->ReceiveDamage(1,Hit,Storage->GetTimelineEpoch()));
    TestEqual(TEXT("Interrupted full dose leaves half"),Bag->GetItemCount(TEXT("medicine_half")),1);
    TestEqual(TEXT("Half weighs half"),Bag->GetWeight(),.25);
    TestFalse(TEXT("Repeated event cannot charge or damage twice"),S->ReceiveDamage(1,Hit,Storage->GetTimelineEpoch()));
    TestTrue(TEXT("Half dose is usable"),S->BeginMedicine(TEXT("medicine_half")));
    S->CompleteBoundary(3);
    TestTrue(TEXT("Half dose heals fifteen percent"),FMath::IsNearlyEqual(G->Health,34.f));
    TestEqual(TEXT("Half is consumed"),Bag->GetItemCount(TEXT("medicine_half")),0);
    Bag->TryAdd(TEXT("medicine"),1); S->BeginMedicine(TEXT("medicine")); G->Health=G->MaxHealth(); S->CompleteBoundary(3);
    TestEqual(TEXT("Full health at commit incurs no debit"),Bag->GetItemCount(TEXT("medicine")),1);
    S->State.Medicine=NAME_None; S->ResetTransient(); G->Health=10;
    // Fixture-only sustained variant of the existing dose; no unapproved recipe/catalog item is shipped.
    const auto Row=HearthwardData::Find(TEXT("items"),TEXT("medicine"));
    Row->SetNumberField(TEXT("medicineDuration"),15); Row->SetNumberField(TEXT("healing"),50);
    TestTrue(TEXT("Sustained medicine starts"),S->BeginMedicine(TEXT("medicine")));
    S->CompleteBoundary(3);
    TestEqual(TEXT("Sustained commit starts fifteen active seconds"),S->State.HotRemaining,15.);
    Bag->TryAdd(TEXT("medicine"),1);
    S->State.HotRemaining=5;
    TestTrue(TEXT("Equal sustained medicine may refresh"),S->BeginMedicine(TEXT("medicine")));
    S->CompleteBoundary(3);
    TestEqual(TEXT("Equal dose resets duration"),S->State.HotRemaining,15.);
    TestEqual(TEXT("Refresh does not instantly cash out old effect"),G->Health,10.f);
    Row->SetNumberField(TEXT("healing"),25); Bag->TryAdd(TEXT("medicine"),1);
    TestFalse(TEXT("Weaker sustained dose cannot overwrite stronger active effect"),S->BeginMedicine(TEXT("medicine")));
    TestEqual(TEXT("Rejected weaker dose is not consumed"),Bag->GetItemCount(TEXT("medicine")),1);
    Row->RemoveField(TEXT("medicineDuration")); Row->SetNumberField(TEXT("healing"),30);
    Row->SetBoolField(TEXT("rare"),true);
    TestFalse(TEXT("Rare automatic medicine requires permission"),S->BeginMedicine(TEXT("medicine"),true));
    TestFalse(TEXT("Half dose inherits rare original permission"),S->Permitted(TEXT("medicine_half")));
    S->SetAutoPermission(TEXT("medicine"),true);
    TestTrue(TEXT("Original permission covers half dose"),S->Permitted(TEXT("medicine_half")));
    TestTrue(TEXT("Explicit permission allows automatic medicine"),S->BeginMedicine(TEXT("medicine"),true));
    S->SetAutoPermission(TEXT("medicine_half"),false); S->CompleteBoundary(3);
    TestEqual(TEXT("Revocation before commit preserves inventory"),Bag->GetItemCount(TEXT("medicine")),1);
    S->ResetTransient(); Row->RemoveField(TEXT("rare"));
    S->State.HotRemaining=S->State.HotRate=0;
    auto* Target=World->SpawnActor<AActor>();
    auto* TargetG=NewObject<UHearthwardGameplayComponent>(Target); Target->AddInstanceComponent(TargetG); TargetG->RegisterComponent(); TargetG->Enabled=true; TargetG->Health=0;
    auto* TargetS=NewObject<UHearthwardSurvivalComponent>(Target); Target->AddInstanceComponent(TargetS); TargetS->RegisterComponent();
    TargetS->State.Life=EHearthwardLife::Downed; TargetS->State.DownRemaining=5;
    TestTrue(TEXT("Rescue can start within reach"),S->BeginRescue(TargetS));
    TargetS->State.Advance(TargetG->Health,TargetG->Hunger,100,5,5,0);
    S->CompleteBoundary(5);
    TestTrue(TEXT("Deadline wins over exact rescue completion"),TargetS->State.Life==EHearthwardLife::Dead);
    TargetS->State={}; TargetS->State.Life=EHearthwardLife::Downed; TargetS->State.DownRemaining=6;
    TestTrue(TEXT("Rescue can start with sufficient time"),S->BeginRescue(TargetS));
    TargetS->State.Advance(TargetG->Health,TargetG->Hunger,100,5,5,0); S->CompleteBoundary(5);
    TestTrue(TEXT("Successful rescue restores ten percent"),TargetS->Alive() && TargetG->Health==10);
    G->Health=1;
    const FGuid DownHit=FGuid::NewGuid(); S->ReceiveDamage(1,DownHit,Storage->GetTimelineEpoch());
    TestFalse(TEXT("The downing event cannot also kill"),S->ReceiveDamage(1,DownHit,Storage->GetTimelineEpoch()));
    TestTrue(TEXT("Downed medicine rejected"),!S->BeginMedicine(TEXT("medicine")));
    S->ReceiveDamage(1,FGuid::NewGuid(),Storage->GetTimelineEpoch());
    TestTrue(TEXT("Independent residual hit kills"),S->State.Life==EHearthwardLife::Dead);
    GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return true;
}
#endif
