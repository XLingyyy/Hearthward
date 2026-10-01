#include "../Input/HearthwardInputBindings.h"
#include "../Experience/HearthwardTraversalComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExperienceBindingsTest,"Hearthward.Experience.BindingConflictsAndContexts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExperienceBindingsTest::RunTest(const FString&)
{
    auto B=HearthwardInput::Defaults();
    TestTrue(TEXT("Defaults preserve shared R and E priorities"),HearthwardInput::Validate(B).IsEmpty());
    B[TEXT("storage.open")][0]={EKeys::Y,FKey()};TestTrue(TEXT("R actions can split"),HearthwardInput::Validate(B).IsEmpty());
    TestTrue(TEXT("Changed binding affects semantic lookup"),HearthwardInput::Matches(B,TEXT("storage.open"),EKeys::Y,false,false,false));
    TestFalse(TEXT("Old physical key is no longer storage"),HearthwardInput::Matches(B,TEXT("storage.open"),EKeys::R,false,false,false));
    B[TEXT("interact")][0]={EKeys::W,FKey()};TestFalse(TEXT("Simultaneous world conflict is rejected"),HearthwardInput::Validate(B).IsEmpty());
    B=HearthwardInput::Defaults();B[TEXT("ui.inventory")][0]={EKeys::F,FKey()};TestFalse(TEXT("UI global shortcut cannot mask inventory use"),HearthwardInput::Validate(B).IsEmpty());
    B=HearthwardInput::Defaults();B[TEXT("move.forward")][0]={};TestFalse(TEXT("Movement entry must survive"),HearthwardInput::Validate(B).IsEmpty());
    B=HearthwardInput::Defaults();B[TEXT("combat.throw")][0]={EKeys::Enter,FKey()};TestFalse(TEXT("Text submit key reserved"),HearthwardInput::Validate(B).IsEmpty());
    B[TEXT("combat.throw")][0]={EKeys::Y,EKeys::LeftControl};TestTrue(TEXT("A one-modifier chord is valid"),HearthwardInput::Validate(B).IsEmpty());
    TestFalse(TEXT("Chord cannot trigger from bare key"),HearthwardInput::Matches(B,TEXT("combat.throw"),EKeys::Y,false,false,false));
    TestTrue(TEXT("Chord triggers from actual modifier"),HearthwardInput::Matches(B,TEXT("combat.throw"),EKeys::Y,false,true,false));
    auto Conflict=HearthwardInput::Defaults();Conflict[TEXT("combat.throw")][0]={EKeys::W,EKeys::LeftControl};
    TestFalse(TEXT("Chord cannot also move through its bare key"),HearthwardInput::Validate(Conflict).IsEmpty());
    const auto Encoded=FHearthwardKeyBinding::Decode(TEXT("LeftControl+Y"));TestTrue(TEXT("Binding round trip retains chord"),Encoded==B[TEXT("combat.throw")][0]);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExperienceFallTest,"Hearthward.Experience.FallHeightAndMaximumHealth",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExperienceFallTest::RunTest(const FString&)
{
    auto Damage=[](float Height,float Gravity,float Health){return UHearthwardTraversalComponent::FallDamage(FMath::Sqrt(2*FMath::Abs(Gravity)*Height*100),Gravity,Health);};
    TestEqual(TEXT("Stationary landing costs nothing"),UHearthwardTraversalComponent::FallDamage(0,-980,100),0.f);
    TestTrue(TEXT("Three metres remains harmless"),Damage(3,-980,100)<.0001);
    TestTrue(TEXT("7.5 metres costs half maximum health"),FMath::IsNearlyEqual(Damage(7.5,-980,180),90.f,.001f));
    TestTrue(TEXT("Uses actual gravity"),FMath::IsNearlyEqual(Damage(7.5,-490,180),90.f,.001f));
    TestTrue(TEXT("Twelve metres caps at maximum health"),FMath::IsNearlyEqual(Damage(12,-980,180),180.f,.001f));
    TestEqual(TEXT("Longer falls still cap"),Damage(30,-980,180),180.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExperienceSwimTest,"Hearthward.Experience.SwimmingFloatExhaustionAndGroundFall",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExperienceSwimTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* C=World->SpawnActor<ACharacter>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(C);C->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* G=NewObject<UHearthwardGameplayComponent>(C);C->AddInstanceComponent(G);G->RegisterComponent();G->Enabled=true;
    auto* S=NewObject<UHearthwardSurvivalComponent>(C);C->AddInstanceComponent(S);S->RegisterComponent();
    C->GetCharacterMovement()->SetMovementMode(MOVE_Swimming);G->Stamina=G->MaxStamina();
    S->AdvanceContinuous(3,0,0);TestEqual(TEXT("Positive floating neither costs nor regenerates"),G->Stamina,G->MaxStamina());
    C->GetCharacterMovement()->Velocity={300,0,0};S->AdvanceContinuous(25,0,0);
    TestEqual(TEXT("25 active seconds exhaust unloaded swimmer"),G->Stamina,0.f);
    TestTrue(TEXT("Drowning starts at exhaustion boundary"),FMath::IsNearlyEqual(S->State.DrowningRemaining,10.));
    C->GetCharacterMovement()->SetMovementMode(MOVE_Falling);S->AdvanceContinuous(9,0,0);
    TestTrue(TEXT("Brief surfacing does not reset drowning"),S->Alive() && FMath::IsNearlyEqual(S->State.DrowningRemaining,1.));
    S->AdvanceContinuous(1,0,0);TestTrue(TEXT("Drowning at ten seconds is true death"),S->State.Life==EHearthwardLife::Dead);
    S->State={};G->Health=G->MaxHealth();C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    S->FallImpact(FMath::Sqrt(2.f*980.f*1200.f));TestTrue(TEXT("Ground fall to zero remains rescuable"),S->State.Life==EHearthwardLife::Downed);
    World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return true;
}
#endif
