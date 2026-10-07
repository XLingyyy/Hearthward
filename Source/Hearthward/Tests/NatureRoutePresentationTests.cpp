#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "../Animals/HearthwardAnimalMotionComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoarR3PresentationTest,
    "Hearthward.Iteration.Task097.BoarKeepsR3AndPig",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)

bool FBoarR3PresentationTest::RunTest(const FString&)
{
    const auto Values=UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    auto Create=[&](FName Species)
    {
        auto* Actor=World->SpawnActor<AActor>();
        auto* Root=NewObject<USceneComponent>(Actor);
        Actor->AddInstanceComponent(Root);Actor->SetRootComponent(Root);Root->RegisterComponent();
        auto* Motion=NewObject<UHearthwardAnimalMotionComponent>(Actor);
        Actor->AddInstanceComponent(Motion);Motion->RegisterComponent();
        if(!Motion->Configure(Species))return static_cast<UHearthwardAnimalMotionComponent*>(nullptr);
        Motion->SetHabitat(FBox(FVector(-1800,-1800,-400),FVector(1800,1800,400)));
        return Motion;
    };
    auto* Pig=Create(TEXT("pig"));auto* Boar=Create(TEXT("boar"));
    if(!TestNotNull(TEXT("Original pig configures"),Pig)
        || !TestNotNull(TEXT("Boar configures with all existing clip skeletons"),Boar))return false;
    auto* PigAsset=Pig->Mesh->GetSkeletalMeshAsset();
    auto* BoarAsset=Boar->Mesh->GetSkeletalMeshAsset();
    TestEqual(TEXT("Domestic pig retains its original mesh"),PigAsset->GetPathName(),
        FString(TEXT("/Game/Hearthward/Animals/MotionR3/pig/SK_pig.SK_pig")));
    TestEqual(TEXT("Only boar selects the approved derivative"),BoarAsset->GetPathName(),
        FString(TEXT("/Game/Hearthward/Assets/TASK-097/Boar/SK_Boar_Practical.SK_Boar_Practical")));
    TestEqual(TEXT("Both meshes use the same R3 skeleton"),BoarAsset->GetSkeleton(),PigAsset->GetSkeleton());
    TestEqual(TEXT("Physics asset stays shared"),BoarAsset->GetPhysicsAsset(),PigAsset->GetPhysicsAsset());
    TestTrue(TEXT("Visual snout and mane do not change movement bounds"),
        Boar->MovementBounds().Min.Equals(Pig->MovementBounds().Min,.001)
        && Boar->MovementBounds().Max.Equals(Pig->MovementBounds().Max,.001));
    TestEqual(TEXT("Detection range is unchanged"),Boar->DetectionRadius(),Pig->DetectionRadius());
    TestEqual(TEXT("Every original clip remains available"),Boar->Clips.Num(),Pig->Clips.Num());
    for(const auto& Clip:Pig->Clips)
        TestEqual(*FString::Printf(TEXT("Shared clip %s"),*Clip.Key.ToString()),Boar->Clips.FindRef(Clip.Key).Get(),Clip.Value.Get());

    for(FName Name:{FName(TEXT("IdleWild")),FName(TEXT("RunWild")),FName(TEXT("SnoutStrikeFull")),FName(TEXT("Collapse_L"))})
    {
        auto* Clip=Pig->Clips.FindRef(Name).Get();
        if(!TestNotNull(*FString::Printf(TEXT("Actual R3 clip %s"),*Name.ToString()),Clip))return false;
        for(auto* Mesh:{Pig->Mesh.Get(),Boar->Mesh.Get()})
        {
            Mesh->PlayAnimation(Clip,false);
            Mesh->SetPosition(Clip->GetPlayLength()*.5f,false);
            Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();
        }
        for(int32 Index=0;Index<Pig->Mesh->GetNumBones();++Index)
        {
            const FName Bone=Pig->Mesh->GetBoneName(Index);
            TestTrue(*FString::Printf(TEXT("%s/%s keeps the R3 component pose"),*Name.ToString(),*Bone.ToString()),
                Pig->Mesh->GetSocketTransform(Bone,RTS_Component).Equals(
                    Boar->Mesh->GetSocketTransform(Bone,RTS_Component),.01));
        }
    }
    return true;
}
#endif
