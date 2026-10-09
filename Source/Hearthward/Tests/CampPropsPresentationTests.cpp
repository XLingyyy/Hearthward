#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampFirePresentationTest,"Hearthward.Camp098.ProductionFire",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampFirePresentationTest::RunTest(const FString&)
{
    // PROTOTYPE_ONLY: isolated presentation fixture, no inventory or production transaction bypass in gameplay.
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Owner=World->SpawnActor<AActor>();
    auto* Build=NewObject<UHearthwardBuildingComponent>(Owner);Owner->AddInstanceComponent(Build);Build->RegisterComponent();
    const FGuid Id=FGuid::NewGuid();
    auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),Id.ToString());Row->SetStringField(TEXT("recipe"),TEXT("smelter"));
    Row->SetStringField(TEXT("position"),TEXT("X=0 Y=0 Z=0"));Row->SetNumberField(TEXT("yaw"),0);
    TArray<TSharedPtr<FJsonValue>> Rows;Rows.Add(MakeShared<FJsonValueObject>(Row));Build->Restore(Rows);
    auto& State=World->GetSubsystem<UHearthwardCampSubsystem>()->State;
    auto* F=State.Facilities.FindByPredicate([&](const auto& V){return V.Id==Id;});
    if(!TestNotNull(TEXT("Restored facility registered"),F)){World->DestroyWorld(false);return false;}
    FHearthwardCampRegion Region;Region.Facility=Id;Region.Camp=F->Camp;Region.Enabled=true;Region.Workers.Add(0);
    State.Regions.Add(Region);auto& R=State.Regions.Last();
    auto Refresh=[&](){Build->TickComponent(.3f,LEVELTICK_All,nullptr);};
    // NullRHI does not simulate Niagara; verify its shared light gate here and particles in rendered PIE.
    auto Active=[&](){TArray<UPointLightComponent*> Lights;Build->ResolveFacility(Id)->GetComponents(Lights);return Lights.Num() && Lights[0]->IsVisible()?2:0;};
    Refresh();TestEqual(TEXT("No materials: no fire"),Active(),0);
    R.Batch.Active=true;R.Batch.Required=360;R.Batch.Work=5;Refresh();TestEqual(TEXT("Working batch lights flame and smoke"),Active(),2);
    F->Paused=true;Refresh();TestEqual(TEXT("Moving facility extinguishes fire"),Active(),0);
    F->Paused=false;R.Enabled=false;Refresh();TestEqual(TEXT("Paused queue stays dark"),Active(),0);
    R.Enabled=true;R.Workers.Reset();Refresh();TestEqual(TEXT("No workers stays dark"),Active(),0);
    R.Workers.Add(0);R.Safe=false;Refresh();TestEqual(TEXT("Unsafe region stays dark"),Active(),0);
    R.Safe=true;R.Batch.Work=360;Refresh();TestEqual(TEXT("Blocked output stays dark"),Active(),0);
    R.Batch.Work=10;Refresh();TestEqual(TEXT("Resume relights existing effects"),Active(),2);
    TArray<UNiagaraComponent*> FX;Build->ResolveFacility(Id)->GetComponents(FX);TestEqual(TEXT("No duplicate emitters"),FX.Num(),2);
    for(auto* E:FX)TestNotNull(TEXT("Fire asset resolves"),E->GetAsset());
    Build->Restore(Rows);Refresh();TestEqual(TEXT("Restore rebuilds presentation from real state"),Active(),2);
    for(const FString Kind:{TEXT("workbench"),TEXT("smelter"),TEXT("forge"),TEXT("cooking")})
    {
        Row->SetStringField(TEXT("recipe"),Kind);State.Facilities.Reset();State.Regions.Reset();Build->Restore(Rows);
        auto& Facility=State.Facilities.Last();
        for(int32 Level=1;Level<=3;++Level)
        {
            Facility.Level=Level;Refresh();
            TArray<UStaticMeshComponent*> Meshes;Build->ResolveFacility(Id)->GetComponents(Meshes);
            TestEqual(*FString::Printf(TEXT("%s level %d distinct assemblies"),*Kind,Level),Meshes.Num(),Level+(Kind==TEXT("forge")?1:0));
            for(auto* Mesh:Meshes)
            {
                TestNotNull(TEXT("Upgrade mesh resolves"),Mesh->GetStaticMesh().Get());
                TestEqual(TEXT("Upgrade keeps original collision"),Mesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
            }
        }
        Build->Restore(Rows);Refresh();
        TArray<UStaticMeshComponent*> Meshes;Build->ResolveFacility(Id)->GetComponents(Meshes);
        TestEqual(TEXT("Level three rebuilds after restore without duplicates"),Meshes.Num(),3+(Kind==TEXT("forge")?1:0));
    }
    World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return true;
}
#endif
