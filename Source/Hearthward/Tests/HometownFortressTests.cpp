#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Building/HearthwardHometownFortress.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHometownFortressClearanceTest,"Hearthward.Hometown077.BedroomAndEscapeClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHometownFortressClearanceTest::RunTest(const FString&)
{
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Ground=World->SpawnActor<AActor>();
    Ground->Tags.Add(TEXT("Hearthward.NatureGround"));
    auto* Box=NewObject<UBoxComponent>(Ground);Ground->AddInstanceComponent(Box);Ground->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(20000,20000,50));Box->SetRelativeLocation(FVector(0,0,-50));
    Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();
    auto* Home=World->SpawnActor<AHearthwardHometownFortress>();Home->DispatchBeginPlay();
    TArray<UStaticMeshComponent*> VisualParts;Home->GetComponents(VisualParts);
    int32 VisualBeds=0,Joinery=0;
    for(auto* Part:VisualParts)
    {
        if(Part->GetName()==TEXT("BedroomJoinery"))
        {
            ++Joinery;
            TestNotNull(TEXT("Bedroom joinery asset resolves"),Part->GetStaticMesh().Get());
            TestEqual(TEXT("Joinery stays decorative"),Part->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
        }
        if(Part->GetName()==TEXT("PlayerBed") || Part->GetName()==TEXT("BrotherBed"))
        {
            ++VisualBeds;
            TestNotNull(TEXT("Bed mesh resolves"),Part->GetStaticMesh().Get());
            const FVector Half=Part->Bounds.BoxExtent;
            TestTrue(TEXT("Authored bed fits existing collision footprint"),Half.X<=75 && Half.Y<=115);
        }
    }
    TestEqual(TEXT("One joinery assembly"),Joinery,1);
    TestEqual(TEXT("Two correctly oriented bedroom beds"),VisualBeds,2);
    TestFalse(TEXT("Player starts clear of bed, wall and ceiling"),World->OverlapBlockingTestByChannel(Home->BedroomLanding(),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96)));
    TestFalse(TEXT("Brother has separate clear bedroom landing"),World->OverlapBlockingTestByChannel(Home->BedroomLanding()+FVector(160,100,0),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96)));
    FHitResult Hit;
    TestFalse(TEXT("Bedroom doorway passes full character capsule"),World->SweepSingleByChannel(Hit,Home->BedroomLanding(),FVector(0,850,220),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96)));
    TestTrue(TEXT("Door threshold connects both floors without a navigation gap"),World->LineTraceSingleByChannel(Hit,FVector(0,550,150),FVector(0,550,90),ECC_WorldStatic));
    TestFalse(TEXT("Gallery connects bedroom to stairs"),World->SweepSingleByChannel(Hit,FVector(0,850,220),FVector(1500,850,220),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96)));
    TestFalse(TEXT("Postern connects court to existing escape route"),World->SweepSingleByChannel(Hit,FVector(0,6100,100),FVector(0,7000,100),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96)));
    TArray<UStaticMeshComponent*> Parts;Home->GetComponents(Parts);
    int32 Beds=0,Steps=0;
    for(const auto* Part:Parts)
    {
        TestTrue(*FString::Printf(TEXT("Mesh exists: %s"),*Part->GetName()),Part->GetStaticMesh()!=nullptr);
        if(Part->GetName()==TEXT("PlayerBed") || Part->GetName()==TEXT("BrotherBed"))++Beds;
        if(Part->ComponentHasTag(TEXT("EscapeStair")))++Steps;
    }
    TestEqual(TEXT("Both brothers have beds"),Beds,2);TestTrue(TEXT("Stairs are present"),Steps>0);
    auto* Controller=World->SpawnActor<APlayerController>();
    World->AddController(Controller);
    auto* Player=World->SpawnActor<ADefaultPawn>(Home->BedroomLanding(),FRotator::ZeroRotator);Controller->Possess(Player);
    TestEqual(TEXT("Fixture has registered player controller"),World->GetFirstPlayerController(),Controller);
    TestEqual(TEXT("Fixture player is possessed"),Controller->GetPawn().Get(),static_cast<APawn*>(Player));
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign->State.Phase=TEXT("prologue");
    const auto CountEffects=[Home]()
    {
        TArray<UNiagaraComponent*> Effects;Home->GetComponents(Effects);return Effects.Num();
    };
    Home->Tick(0);TestEqual(TEXT("Three raid pockets each have flame and smoke"),CountEffects(),6);
    Home->Tick(0);Home->Tick(0);TestEqual(TEXT("Repeated update does not duplicate effects"),CountEffects(),6);
    TArray<UNiagaraComponent*> Effects;Home->GetComponents(Effects);
    for(auto* Effect:Effects)
    {
        TestNotNull(TEXT("Raid system is bound"),Effect->GetAsset());
        TestFalse(TEXT("Raid particles do not affect navigation"),Effect->CanEverAffectNavigation());
    }
    Campaign->State.Phase=TEXT("occupied");Home->Tick(0);
    TestEqual(TEXT("Leaving prologue removes raid effects"),CountEffects(),0);
    TArray<UPointLightComponent*> Lights;Home->GetComponents(Lights);
    TestFalse(TEXT("Leaving prologue removes raid lighting"),Lights.ContainsByPredicate([](auto* Light){return Light->ComponentHasTag(TEXT("HearthwardRaidLight"));}));
    Campaign->State.Phase=TEXT("prologue");Home->Tick(0);TestEqual(TEXT("Returning to prologue recreates one set"),CountEffects(),6);
    Player->SetActorLocation(FVector(25000,0,200));Home->Tick(0);TestEqual(TEXT("Leaving the fortress removes effects"),CountEffects(),0);
    Player->SetActorLocation(Home->BedroomLanding());Home->Tick(0);TestEqual(TEXT("Returning to fortress recreates one set"),CountEffects(),6);
    Home->EndPlay(EEndPlayReason::RemovedFromWorld);
    TestEqual(TEXT("EndPlay removes owned effects"),CountEffects(),0);
    Home->Destroy();
    World->DestroyWorld(false);GEngine->DestroyWorldContext(World);
    return true;
}
#endif
