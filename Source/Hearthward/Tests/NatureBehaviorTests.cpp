#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Animals/HearthwardAnimalDemo.h"
#include "../Animals/HearthwardAnimalMotionComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Nature/HearthwardNatureActor.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonSerializer.h"

namespace
{
template<typename T> T* AddNatureTestComponent(AActor* Actor)
{
    auto* Component=NewObject<T>(Actor);
    Actor->AddInstanceComponent(Component);Component->RegisterComponent();return Component;
}

// PROTOTYPE_ONLY flat-world fixture: production actors, paired motion assets and damage entry points.
struct FNatureBehaviorWorld
{
    UWorld* World;
    APawn* Player;
    UHearthwardGameplayComponent* Gameplay;
    UHearthwardCombatComponent* Combat;
    UHearthwardNatureSubsystem* Nature;

    FNatureBehaviorWorld()
    {
        World=UWorld::CreateWorld(EWorldType::Game,false);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        World->InitializeActorsForPlay(FURL());
        auto* Floor=Box(FVector(0,0,-50),FVector(80,80,1));
        Floor->Tags.Add(TEXT("Hearthward.NatureGround"));

        Player=World->SpawnActor<APawn>();
        auto* Root=NewObject<USceneComponent>(Player);
        Player->AddInstanceComponent(Root);Player->SetRootComponent(Root);Root->RegisterComponent();
        AddNatureTestComponent<UHearthwardInventoryComponent>(Player);
        Gameplay=AddNatureTestComponent<UHearthwardGameplayComponent>(Player);
        Gameplay->Enabled=true;Gameplay->Health=100;
        AddNatureTestComponent<UHearthwardSurvivalComponent>(Player);
        Combat=AddNatureTestComponent<UHearthwardCombatComponent>(Player);
        World->SpawnActor<APlayerController>()->Possess(Player);
        Nature=World->GetSubsystem<UHearthwardNatureSubsystem>();
    }
    ~FNatureBehaviorWorld()
    {
        GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    AStaticMeshActor* Box(FVector Position,FVector Scale)
    {
        auto* Actor=World->SpawnActor<AStaticMeshActor>();
        auto* Mesh=Actor->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));Mesh->SetCollisionObjectType(ECC_WorldStatic);
        Actor->SetActorLocation(Position);Actor->SetActorScale3D(Scale);return Actor;
    }
    AHearthwardNatureActor* Animal(FName Definition,bool Domestic=false)
    {
        FHearthwardAnimal State;State.Id=FGuid::NewGuid();State.Definition=Definition;State.Domestic=Domestic;
        State.Health=HearthwardData::Number(HearthwardNature::Definition(Domestic?TEXT("domestic"):TEXT("wildlife"),Definition),TEXT("health"));
        if(!Domestic)
        {
            State.Slot=TEXT("task060_test_slot");
            FHearthwardWildSlot Slot;Slot.Id=State.Slot;Slot.Definition=Definition;Slot.Current=State.Id;
            Nature->State.Slots.Add(Slot);
        }
        Nature->State.Animals.Add(State);
        auto* Actor=World->SpawnActor<AHearthwardNatureActor>();Actor->Configure(State.Id,TEXT("animal"),Definition);
        return Actor;
    }
    void Step(AHearthwardNatureActor* Animal,float Seconds)
    {
        Animal->Tick(Seconds);Animal->AnimalMotion->TickComponent(Seconds,LEVELTICK_All,nullptr);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureWolfAuthorityTest,"Hearthward.Nature060.WolfAttacksWithMotion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNatureWolfAuthorityTest::RunTest(const FString&)
{
    FNatureBehaviorWorld Fixture;auto* Wolf=Fixture.Animal(TEXT("wolf"));
    if(!TestTrue(TEXT("Production wolf paired motion is loaded"),Wolf->AnimalMotion->Ready()))return false;
    if(!TestTrue(TEXT("Fixture pawn is the real player lookup"),UGameplayStatics::GetPlayerPawn(Fixture.World,0)==Fixture.Player))return false;
    Fixture.Player->SetActorLocation(Wolf->GetActorLocation()+FVector(120,0,0));
    const float Before=Fixture.Gameplay->Health;
    for(int32 I=0;I<20;++I)Fixture.Step(Wolf,.05f);
    TestTrue(TEXT("Wolf damages a visible living player in melee range with motion enabled"),Fixture.Gameplay->Health<Before);
    TestFalse(TEXT("Formal motion does not also own animal movement"),Wolf->AnimalMotion->ControlsNatureMovement());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureOccludedThreatTest,"Hearthward.Nature060.OccludedThreatDoesNotFlee",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNatureOccludedThreatTest::RunTest(const FString&)
{
    FNatureBehaviorWorld Fixture;auto* Deer=Fixture.Animal(TEXT("deer"));
    if(!TestTrue(TEXT("Production deer paired motion is loaded"),Deer->AnimalMotion->Ready()))return false;
    const FVector Start=Deer->GetActorLocation();Fixture.Player->SetActorLocation(Start+FVector(700,0,0));
    auto* Wall=Fixture.Box(FVector(350,0,300),FVector(.5,20,6));
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Nature060Occlusion),false,Deer);Query.AddIgnoredActor(Fixture.Player);
    if(!TestTrue(TEXT("Real wall blocks animal-to-player visibility"),Fixture.World->LineTraceTestByChannel(Start,Fixture.Player->GetActorLocation(),ECC_Visibility,Query)))return false;
    Fixture.Step(Deer,.05f);
    TestTrue(TEXT("Occluded player does not become the motion threat"),Deer->AnimalMotion->LastThreat.IsNone());
    TestFalse(TEXT("Occluded player is not recorded as seen"),Deer->Combat->Memory.Seen.Contains(TEXT("player")));

    Wall->Destroy();Fixture.Step(Deer,.05f);
    TestEqual(TEXT("Removing the wall permits observation of the same nearby player"),Deer->AnimalMotion->LastThreat,FName(TEXT("player")));
    const double VisibleDistance=FVector::Dist2D(Deer->GetActorLocation(),Fixture.Player->GetActorLocation());
    for(int32 I=0;I<50;++I)Fixture.Step(Deer,.05f);
    TestTrue(TEXT("Visible player causes actual escape movement"),FVector::Dist2D(Deer->GetActorLocation(),Fixture.Player->GetActorLocation())>VisibleDistance+200);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureRetaliationSourceTest,"Hearthward.Nature060.RetaliatesAgainstDamageSource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNatureRetaliationSourceTest::RunTest(const FString&)
{
    FNatureBehaviorWorld Fixture;auto* Bear=Fixture.Animal(TEXT("black_bear"));
    if(!TestTrue(TEXT("Production bear paired motion is loaded"),Bear->AnimalMotion->Ready()))return false;
    Fixture.Player->SetActorLocation(Bear->GetActorLocation()+FVector(100,0,0));
    FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Brother=Fixture.World->SpawnActor<AHearthwardCompanionFixture>(AHearthwardCompanionFixture::StaticClass(),Bear->GetActorLocation()+FVector(0,150,80),FRotator::ZeroRotator,Spawn);
    auto* Survival=Brother->FindComponentByClass<UHearthwardSurvivalComponent>();
    const float PlayerBefore=Fixture.Gameplay->Health,BrotherBefore=Survival->Health();
    Fixture.Combat->HitTarget(Bear->Combat,1,TEXT("body"),false,FGuid::NewGuid(),Brother);
    TestTrue(TEXT("Actual hit enters the existing nature damage transaction"),Fixture.Nature->State.Animals[0].Health<Bear->Combat->MaximumHealth);
    const auto& Hit=Fixture.Nature->State.Animals[0];
    TestEqual(TEXT("Actual hit persists the stable brother identity"),Hit.Threat,FName(TEXT("brother")));
    TestTrue(TEXT("Actual hit records the source position"),Hit.Destination.Equals(Brother->GetActorLocation()));
    FHearthwardNatureState Restored;
    if(TestTrue(TEXT("Source identity round trips through the current nature snapshot"),FHearthwardNatureState::Parse(Fixture.Nature->State.Snapshot(),Restored)))
    {
        TestEqual(TEXT("Restored source remains brother"),Restored.Animals[0].Threat,FName(TEXT("brother")));
        TestTrue(TEXT("Restored source position is preserved"),Restored.Animals[0].Destination.Equals(Brother->GetActorLocation()));
        Restored.Animals[0].Threat=TEXT("unknown_actor");
        TestFalse(TEXT("Unsupported source identity is rejected"),Restored.Valid());
    }
    TSharedPtr<FJsonObject> LegacyRoot;
    if(TestTrue(TEXT("Read current nature snapshot for prior-field fixture"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Fixture.Nature->State.Snapshot()),LegacyRoot)))
    {
        LegacyRoot->GetArrayField(TEXT("animals"))[0]->AsObject()->RemoveField(TEXT("threat"));
        FString LegacyJson;FJsonSerializer::Serialize(LegacyRoot.ToSharedRef(),TJsonWriterFactory<>::Create(&LegacyJson));
        FHearthwardNatureState Legacy;
        if(TestTrue(TEXT("Prior snapshot without source identity still loads"),FHearthwardNatureState::Parse(LegacyJson,Legacy)))
            TestTrue(TEXT("Prior snapshot does not invent an attacker"),Legacy.Animals[0].Threat.IsNone());
    }
    bool AttackClipSeen=false;
    for(int32 I=0;I<30;++I)
    {
        Fixture.Step(Bear,.05f);
        AttackClipSeen|=Bear->AnimalMotion->ActiveClip==TEXT("SwipeShort_L");
    }
    TestTrue(TEXT("Bear retaliates against the brother who caused the hit"),Survival->Health()<BrotherBefore);
    TestEqual(TEXT("Closer non-attacking player is not substituted for the damage source"),Fixture.Gameplay->Health,PlayerBefore);
    TestTrue(TEXT("Real retaliation consumes the existing paired attack clip"),AttackClipSeen);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureDemoAutonomyTest,"Hearthward.Nature060.DemoKeepsAutonomousMotion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNatureDemoAutonomyTest::RunTest(const FString&)
{
    FNatureBehaviorWorld Fixture;auto* Demo=Fixture.World->SpawnActor<AHearthwardAnimalDemoActor>();
    if(!TestTrue(TEXT("Existing demo wolf loads paired assets"),Demo->Configure(TEXT("wolf"),FBox(FVector(-1800,-1800,-100),FVector(1800,1800,1000)))))return false;
    const FVector Start=Demo->GetActorLocation();Fixture.Player->SetActorLocation(Start+FVector(300,0,0));
    TestTrue(TEXT("Demo continues to own its movement"),Demo->Motion->ControlsNatureMovement());
    TestEqual(TEXT("Demo retains the approved base escape speed"),Demo->Motion->EscapeSpeed(),630.f);
    for(int32 I=0;I<60;++I)Demo->Motion->TickComponent(.05f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Demo actually moves away without a nature actor"),FVector::Dist2D(Demo->GetActorLocation(),Start)>200);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureCapturedAndDeathTest,"Hearthward.Nature060.CapturedLeadAndSingleDeathReward",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNatureCapturedAndDeathTest::RunTest(const FString&)
{
    FNatureBehaviorWorld Fixture;auto* Goat=Fixture.Animal(TEXT("goat"),true);
    if(!TestTrue(TEXT("Existing domestic goat paired assets load"),Goat->AnimalMotion->Ready()))return false;
    FHearthwardPen Pen;Pen.Id=FGuid::NewGuid();Pen.Definition=TEXT("goat");Pen.Position=FVector(1000,0,0);
    Fixture.Nature->State.Pens.Add(Pen);
    auto& Captured=Fixture.Nature->State.Animals[0];Captured.Captured=true;Captured.Following=true;Captured.ReservedPen=Pen.Id;
    Fixture.Player->SetActorLocation(Pen.Position);
    for(int32 I=0;I<70;++I)Fixture.Step(Goat,.05f);
    TestTrue(TEXT("Real captured animal reaches its reserved pen"),Captured.Pen==Pen.Id && !Captured.ReservedPen.IsValid() && !Captured.Following);
    TestFalse(TEXT("Captured animal motion remains a state consumer"),Goat->AnimalMotion->ControlsNatureMovement());

    auto* Wolf=Fixture.Animal(TEXT("wolf"));
    Fixture.Combat->HitTarget(Wolf->Combat,10000,TEXT("body"),false,FGuid::NewGuid(),Fixture.Player);
    const auto& Dead=Fixture.Nature->State.Animals.Last();
    TestTrue(TEXT("Real killing hit creates the existing single reward receipt"),Dead.Health==0 && Dead.Rewarded && !Dead.Loot.IsEmpty());
    const auto Loot=Dead.Loot;const FVector DeadAt=Wolf->GetActorLocation();
    Fixture.Combat->HitTarget(Wolf->Combat,10000,TEXT("body"),false,FGuid::NewGuid(),Fixture.Player);
    for(int32 I=0;I<60;++I)Fixture.Step(Wolf,.05f);
    TestTrue(TEXT("Repeated hit does not duplicate the existing death loot"),Dead.Loot.OrderIndependentCompareEqual(Loot));
    TestTrue(TEXT("Dead formal animal does not move during its paired death presentation"),Wolf->GetActorLocation().Equals(DeadAt));
    TestTrue(TEXT("Captured and dead state remain valid for the current save schema"),Fixture.Nature->State.Valid());
    return true;
}
#endif
