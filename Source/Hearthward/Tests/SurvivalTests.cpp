#include "../Survival/HearthwardSurvivalState.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Interaction/HearthwardFurnitureInteractionComponent.h"
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
    TestFalse(TEXT("Active sustained medicine forbids equal repeated dose"),S->BeginMedicine(TEXT("medicine")));
    TestFalse(TEXT("Gameplay reports active medicine for repeated use"),G->UseItem(TEXT("medicine")));
    TestTrue(TEXT("Rejection names the active and attempted item"),G->Feedback==TEXT("当前正在使用药草膏，不能同时使用药草膏"));
    TestEqual(TEXT("Rejected dose preserves remaining effect"),S->State.HotRemaining,5.);
    TestEqual(TEXT("Repeated dose is not consumed"),Bag->GetItemCount(TEXT("medicine")),1);
    TestEqual(TEXT("Rejected dose does not cash out effect"),G->Health,10.f);
    Row->SetNumberField(TEXT("healing"),25);
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
    G->Hunger=20;Bag->TryAdd(TEXT("roast"),2);Bag->TryAdd(TEXT("arrow"),2);Bag->TryAdd(TEXT("firepot"),1);
    TestTrue(TEXT("Food slot accepts an actual food effect"),G->AssignQuickItem(1,TEXT("roast")));
    TestFalse(TEXT("Food cannot be placed in medicine slot"),G->AssignQuickItem(0,TEXT("roast")));
    TestFalse(TEXT("Medicine cannot be placed in food slot despite shared category"),G->AssignQuickItem(1,TEXT("medicine")));
    TestFalse(TEXT("Ammunition cannot be placed in food slot"),G->AssignQuickItem(1,TEXT("arrow")));
    TestTrue(TEXT("Manual food starts a three second reservation"),G->UseQuickItem(1) && S->State.FoodRemaining==3);
    TestEqual(TEXT("Starting food does not debit inventory"),Bag->GetItemCount(TEXT("roast")),2);
    TestEqual(TEXT("Starting food has not yet restored hunger"),G->Hunger,20.f);
    TestFalse(TEXT("Food prevents a duplicate food use"),G->UseQuickItem(1));
    TestFalse(TEXT("Food prevents medicine use"),G->UseQuickItem(0));
    TestFalse(TEXT("Food prevents throwing from the public item command"),G->UseQuickItem(3));
    TestFalse(TEXT("Food prevents direct ammunition use"),G->UseItem(TEXT("arrow")));
    TestTrue(TEXT("Rejection identifies active food and selected ammunition"),G->Feedback==TEXT("当前正在使用烤肉，不能同时使用箭矢"));
    S->CompleteBoundary(2.99);TestEqual(TEXT("Food remains reserved before completion"),Bag->GetItemCount(TEXT("roast")),2);
    S->CompleteBoundary(.01);TestEqual(TEXT("Completed food is charged once"),Bag->GetItemCount(TEXT("roast")),1);
    TestEqual(TEXT("Completed food restores actual data amount"),G->Hunger,60.f);
    S->CompleteBoundary(3);TestEqual(TEXT("Repeating settlement cannot double-charge"),Bag->GetItemCount(TEXT("roast")),1);
    TestTrue(TEXT("A new food action can begin after completion"),G->UseQuickItem(1));
    S->CancelAction();TestTrue(TEXT("Cancellation releases food without charge"),Bag->Available(TEXT("roast"))==1 && S->State.FoodItem.IsNone());
    TestTrue(TEXT("Food state restores its inventory reservation"),G->UseQuickItem(1));
    S->ResetTransient();TestEqual(TEXT("Restored food stays unavailable to recipes"),Bag->Available(TEXT("roast")),0);
    S->CancelAction();
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurvivalFailureTransactionsTest,"Hearthward.Survival.FailureRejectsTransactions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSurvivalFailureTransactionsTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Actor=World->SpawnActor<AActor>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Actor); Actor->AddInstanceComponent(Bag); Bag->RegisterComponent();
    auto* G=NewObject<UHearthwardGameplayComponent>(Actor); Actor->AddInstanceComponent(G); G->RegisterComponent(); G->Enabled=true; G->Health=20; G->Hunger=10;
    auto* S=NewObject<UHearthwardSurvivalComponent>(Actor); Actor->AddInstanceComponent(S); S->RegisterComponent();
    auto* Dead=World->SpawnActor<AActor>();
    auto* DeadG=NewObject<UHearthwardGameplayComponent>(Dead); Dead->AddInstanceComponent(DeadG); DeadG->RegisterComponent(); DeadG->Enabled=true; DeadG->Health=0;
    auto* DeadS=NewObject<UHearthwardSurvivalComponent>(Dead); Dead->AddInstanceComponent(DeadS); DeadS->RegisterComponent(); DeadS->State.Kill();
    Bag->TryAdd(TEXT("roast"),1); Bag->TryAdd(TEXT("medicine"),1);
    TestTrue(TEXT("One dead brother makes the world fail"),UHearthwardSurvivalComponent::HasFailed(World));
    TestFalse(TEXT("The surviving brother cannot eat after failure"),S->Eat(TEXT("roast")));
    TestEqual(TEXT("Failure preserves food inventory"),Bag->GetItemCount(TEXT("roast")),1);
    TestEqual(TEXT("Failure preserves hunger"),G->Hunger,10.f);
    TestFalse(TEXT("The surviving brother cannot start medicine after failure"),S->BeginMedicine(TEXT("medicine")));
    S->CompleteBoundary(3);
    TestEqual(TEXT("Failure cannot commit a medicine debit"),Bag->GetItemCount(TEXT("medicine")),1);
    TestEqual(TEXT("Failure cannot commit medicine healing"),G->Health,20.f);
    DeadS->State={};DeadG->Health=100;
    TestTrue(TEXT("Medicine may start again in a live world"),S->BeginMedicine(TEXT("medicine")));
    DeadS->State.Kill();DeadG->Health=0;S->CompleteBoundary(3);
    TestTrue(TEXT("Failure cancels a medicine action that began beforehand"),S->State.Medicine.IsNone());
    TestEqual(TEXT("A pending medicine is not spent by failure"),Bag->GetItemCount(TEXT("medicine")),1);
    TestEqual(TEXT("A pending medicine supplies no healing after failure"),G->Health,20.f);
    auto* Target=World->SpawnActor<AActor>();
    auto* TargetG=NewObject<UHearthwardGameplayComponent>(Target); Target->AddInstanceComponent(TargetG); TargetG->RegisterComponent(); TargetG->Enabled=true; TargetG->Health=0;
    auto* TargetS=NewObject<UHearthwardSurvivalComponent>(Target); Target->AddInstanceComponent(TargetS); TargetS->RegisterComponent(); TargetS->State.Life=EHearthwardLife::Downed; TargetS->State.DownRemaining=30;
    TestFalse(TEXT("An otherwise reachable rescue cannot begin in a failed world"),S->BeginRescue(TargetS));
    GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurvivalFacilityRecoveryTest,"Hearthward.Survival.FacilityRecoveryBinding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSurvivalFacilityRecoveryTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Actor=World->SpawnActor<AActor>();
    auto* Root=NewObject<USceneComponent>(Actor); Actor->AddInstanceComponent(Root); Actor->SetRootComponent(Root); Root->RegisterComponent();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Actor); Actor->AddInstanceComponent(Bag); Bag->RegisterComponent();
    auto* G=NewObject<UHearthwardGameplayComponent>(Actor); Actor->AddInstanceComponent(G); G->RegisterComponent(); G->Enabled=true; G->Health=20;
    auto* S=NewObject<UHearthwardSurvivalComponent>(Actor); Actor->AddInstanceComponent(S); S->RegisterComponent();
    auto Facility=[&](FName Kind)
    {
        auto* Owner=World->SpawnActor<AActor>();
        auto* Interaction=NewObject<UHearthwardFurnitureInteractionComponent>(Owner); Owner->AddInstanceComponent(Interaction); Owner->SetRootComponent(Interaction); Interaction->Kind=Kind; Interaction->MaxDistance=220; Interaction->RegisterComponent();
        return Interaction;
    };
    auto* Medical=Facility(TEXT("medical_area"));
    Medical->CompleteInteraction(Actor); S->AdvanceContinuous(1,1,0);
    TestTrue(TEXT("The real treatment interaction supplies three percent recovery"),FMath::IsNearlyEqual(G->Health,23.f));
    Medical->GetOwner()->Destroy(); S->AdvanceContinuous(1,1,1);
    TestFalse(TEXT("Demolishing the treatment facility ends its recovery"),S->Treatment || S->Resting);
    TestTrue(TEXT("A demolished facility leaves only ordinary recovery"),FMath::IsNearlyEqual(G->Health,23.5f));
    auto* Bed=Facility(TEXT("bed")); G->Health=20; Bed->CompleteInteraction(Actor);
    Bed->GetOwner()->SetActorLocation(FVector(500,0,0)); S->AdvanceContinuous(1,1,2);
    TestFalse(TEXT("Moving the bed beyond reach ends resting"),S->Resting);
    TestTrue(TEXT("A moved bed cannot heal the stationary former user"),FMath::IsNearlyEqual(G->Health,20.5f));
    auto* CombatMedical=Facility(TEXT("medical_area")); G->Health=20; CombatMedical->CompleteInteraction(Actor);
    G->NotifyCombat(); S->AdvanceContinuous(1,1,3);
    TestFalse(TEXT("Entering combat ends treatment even without a damage event"),S->Treatment || S->Resting);
    G->TickComponent(3,LEVELTICK_All,nullptr);
    TestFalse(TEXT("The fixture has left combat"),G->InCombat());
    G->Health=20; S->AdvanceContinuous(1,1,4);
    TestTrue(TEXT("Leaving combat does not reactivate the old treatment"),FMath::IsNearlyEqual(G->Health,20.5f));
    GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return true;
}
#endif
