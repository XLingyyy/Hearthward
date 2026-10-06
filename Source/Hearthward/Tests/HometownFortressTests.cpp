#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Building/HearthwardHometownFortress.h"
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
    World->DestroyWorld(false);GEngine->DestroyWorldContext(World);
    return true;
}
#endif
