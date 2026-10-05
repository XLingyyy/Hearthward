#include "../HearthwardCharacter.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardProjectile.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/ConvexElem.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FScopedBodyProjectileWorld
{
    UWorld* World=nullptr;
    AHearthwardCharacter* Hero=nullptr;
    AHearthwardCompanionFixture* Brother=nullptr;
    AActor* Enemy=nullptr;

    FScopedBodyProjectileWorld()
    {
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Hero=World->SpawnActor<AHearthwardCharacter>(FVector(0,0,200),FRotator::ZeroRotator,Params);
        Hero->Gameplay->Enabled=true;
        auto* Controller=World->SpawnActor<APlayerController>();
        World->AddController(Controller); Controller->Possess(Hero);
        Brother=World->SpawnActor<AHearthwardCompanionFixture>(FVector(0,600,200),FRotator::ZeroRotator,Params);
        Brother->InitializeCompanion(Hero->FindComponentByClass<UHearthwardInventoryComponent>(),Hero);
        Enemy=World->SpawnActor<AActor>();
        auto* Root=NewObject<USceneComponent>(Enemy);
        Enemy->AddInstanceComponent(Root); Enemy->SetRootComponent(Root); Root->RegisterComponent();
        World->GetWorldSettings()->NotifyBeginPlay();
        for(auto* Character:{static_cast<ACharacter*>(Hero),static_cast<ACharacter*>(Brother)})
            Character->GetMesh()->RefreshBoneTransforms();
    }

    ~FScopedBodyProjectileWorld() { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); }

    bool Ready(FAutomationTestBase& Test,ACharacter* Character,const TCHAR* Role)
    {
        auto* Mesh=Character->GetMesh();
        auto* Asset=Mesh->GetSkeletalMeshAsset();
        auto* Survival=Character->FindComponentByClass<UHearthwardSurvivalComponent>();
        if(!Test.TestNotNull(FString::Printf(TEXT("%s uses a real skeletal mesh asset"),Role),Asset)
            || !Test.TestTrue(TEXT("GameplayStatics resolves the real Hero"),UGameplayStatics::GetPlayerPawn(World,0)==Hero)
            || !Test.TestTrue(FString::Printf(TEXT("%s completed actor initialization"),Role),Character->HasActorBegunPlay())
            || !Test.TestTrue(FString::Printf(TEXT("%s uses enabled production Survival"),Role),Survival && Survival->Enabled()))return false;
        const FString Expected=FString::Printf(TEXT("/Game/Characters/%s/UE5/SK_%s.SK_%s"),Role,Role,Role);
        if(!Test.TestEqual(FString::Printf(TEXT("%s is the current formal mesh"),Role),Asset->GetPathName(),Expected))return false;
        for(const FName Bone:{FName(TEXT("head")),FName(TEXT("spine_03")),FName(TEXT("calf_l")),FName(TEXT("foot_l"))})
            if(!Test.TestTrue(FString::Printf(TEXT("%s contains actual bone %s"),Role,*Bone.ToString()),Mesh->GetBoneIndex(Bone)!=INDEX_NONE))return false;
        auto* Physics=Mesh->GetPhysicsAsset();
        Test.AddInfo(FString::Printf(TEXT("Projectile055 %s: mesh=%s componentPhysics=%s assetPhysics=%s bodies=%d meshCollision=%d capsuleVisibility=%d"),
            Role,*Asset->GetPathName(),*GetPathNameSafe(Physics),*GetPathNameSafe(Asset->GetPhysicsAsset()),
            Physics?Physics->SkeletalBodySetups.Num():0,int32(Mesh->GetCollisionEnabled()),
            int32(Character->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Visibility))));
        if(Physics)for(const auto& Body:Physics->SkeletalBodySetups)
        {
            const auto Bounds=Body->AggGeom.CalcAABB(Mesh->GetBoneTransform(Mesh->GetBoneIndex(Body->BoneName)));
            Test.AddInfo(FString::Printf(TEXT("Body055 %s %s center=%s extent=%s"),Role,*Body->BoneName.ToString(),
                *Bounds.GetCenter().ToString(),*Bounds.GetExtent().ToString()));
        }
        return true;
    }

    void CheckBodyRays(FAutomationTestBase& Test,ACharacter* Character,const TCHAR* Role)
    {
        auto* Survival=Character->FindComponentByClass<UHearthwardSurvivalComponent>();
        const auto Bow=HearthwardData::Find(TEXT("items"),TEXT("bow"));
        for(const FName Bone:{FName(TEXT("head")),FName(TEXT("spine_03")),FName(TEXT("calf_l")),FName(TEXT("foot_l"))})
        {
            const FVector Aim=Character->GetMesh()->GetSocketLocation(Bone);
            const FVector Start=Aim-FVector(300,0,0),End=Aim+FVector(300,0,0);
            Enemy->SetActorLocation(Start-FVector(50,0,0));
            Survival->State.Life=EHearthwardLife::Alive;
            Survival->Health()=Survival->MaxHealth();
            if(auto* Combat=Character->FindComponentByClass<UHearthwardCombatComponent>())Combat->Cancel();
            const float Before=Survival->Health();
            FCollisionQueryParams Query(SCENE_QUERY_STAT(Projectile055ActualBody),false,Enemy);
            FHitResult Hit;
            const bool Traced=World->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Query);
            auto* Arrow=World->SpawnActor<AHearthwardProjectile>(Start,FRotator::ZeroRotator);
            Arrow->EnemyShooter=Enemy; Arrow->Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
            Arrow->Event=FGuid::NewGuid(); Arrow->Item=TEXT("arrow");
            Arrow->Power=HearthwardData::Number(Bow,TEXT("attack"));
            Arrow->Velocity=FVector(HearthwardData::Number(Bow,TEXT("projectileSpeed")),0,0);
            Arrow->RemainingRange=2000; Arrow->Lifetime=1;
            // A straight segment isolates body collision; this is not a gravity or part-mapping acceptance test.
            Arrow->Gravity=0;
            Arrow->Tick(.1f);
            Test.TestTrue(FString::Printf(TEXT("Enemy arrow reaches actual %s bone %s: aim=%s trace=%d hitActor=%s hitBone=%s landed=%d hitTarget=%d health=%.2f->%.2f"),
                Role,*Bone.ToString(),*Aim.ToString(),Traced,*GetNameSafe(Hit.GetActor()),*Hit.BoneName.ToString(),Arrow->Landed,Arrow->HitTarget,Before,Survival->Health()),
                Traced && Hit.GetActor()==Character && Arrow->Landed && Arrow->HitTarget && Survival->Health()<Before);
            Arrow->Destroy();
        }
    }

    bool EquipArmorWithSpare(FAutomationTestBase& Test,ACharacter* Character)
    {
        auto* Bag=Character->FindComponentByClass<UHearthwardInventoryComponent>();
        for(const FName Item:{FName(TEXT("hood")),FName(TEXT("armor")),FName(TEXT("leggings")),FName(TEXT("boots"))})
        {
            if(!Test.TestTrue(TEXT("Two real armor instances fit in the character inventory"),
                Bag->TryAdd(Item,2)==EHearthwardInventoryResult::Success))return false;
            TArray<FGuid> Instances;
            for(const auto& Instance:Bag->Snapshot().Instances)
                if(Instance.Definition==Item)Instances.Add(Instance.Id);
            if(!Test.TestEqual(TEXT("Each armor definition has an equipped candidate and a spare"),Instances.Num(),2)
                || !Test.TestTrue(TEXT("Real inventory equips the selected armor GUID"),Bag->EquipInstance(Instances[0])))return false;
        }
        return true;
    }

    AHearthwardProjectile* FireAtBone(FAutomationTestBase& Test,ACharacter* Character,FName Bone,FHitResult& Hit)
    {
        auto* Mesh=Character->GetMesh();
        const auto* Physics=Mesh->GetPhysicsAsset();
        const int32 BodyIndex=Physics?Physics->FindBodyIndex(Bone):INDEX_NONE;
        if(!Test.TestTrue(TEXT("Armor region has a real body on its actual bone"),BodyIndex!=INDEX_NONE))return nullptr;
        const auto* Body=Physics->SkeletalBodySetups[BodyIndex].Get();
        // The ankle body center crosses mixed calf/foot skin; ball_l identifies the forefoot independently of the hull.
        const FName AimBone=Bone==TEXT("foot_l")?FName(TEXT("ball_l")):Bone;
        if(!Test.TestTrue(TEXT("Fixed anatomical aim bone exists"),Mesh->GetBoneIndex(AimBone)!=INDEX_NONE))return nullptr;
        const FVector Aim=Bone==TEXT("foot_l")?Mesh->GetBoneLocation(AimBone)
            :Body->AggGeom.CalcAABB(Mesh->GetBoneTransform(Mesh->GetBoneIndex(Bone))).GetCenter();
        const FVector Start=Aim-FVector(300,0,0),End=Aim+FVector(300,0,0);
        if(Bone==TEXT("foot_l"))for(const FName DiagnosticBone:{FName(TEXT("calf_l")),FName(TEXT("foot_l"))})
        {
            const int32 Index=Mesh->GetBoneIndex(DiagnosticBone),DiagnosticBody=Physics->FindBodyIndex(DiagnosticBone);
            const auto* Instance=Mesh->GetBodyInstance(DiagnosticBone);
            FHitResult BodyHit;
            const bool BodyTraced=Instance && Instance->LineTrace(BodyHit,Start,End,false);
            Test.AddInfo(FString::Printf(TEXT("FixedRay055 %s body=%s hit=%d distance=%.6f impact=%s"),
                *Character->GetName(),*DiagnosticBone.ToString(),BodyTraced,BodyHit.Distance,*BodyHit.ImpactPoint.ToString()));
            if(Index==INDEX_NONE || DiagnosticBody==INDEX_NONE)continue;
            const FTransform Current=Mesh->GetBoneTransform(Index);
            const FTransform Reference=FTransform(FMatrix(Mesh->GetSkeletalMeshAsset()->GetRefBasesInvMatrix()[Index].Inverse()))*Mesh->GetComponentTransform();
            Test.AddInfo(FString::Printf(TEXT("Pose055 %s bone=%s current=%s reference=%s locationDelta=%.6f rotationDelta=%.6f"),
                *Character->GetName(),*DiagnosticBone.ToString(),*Current.ToString(),*Reference.ToString(),
                FVector::Distance(Current.GetLocation(),Reference.GetLocation()),Current.GetRotation().AngularDistance(Reference.GetRotation())));
            for(const auto& Convex:Physics->SkeletalBodySetups[DiagnosticBody]->AggGeom.ConvexElems)
            {
                TArray<FPlane> Planes;Convex.GetPlanes(Planes);
                const FTransform LocalToWorld=Convex.GetTransform()*Current;
                const FVector LocalStart=LocalToWorld.InverseTransformPosition(Start),LocalEnd=LocalToWorld.InverseTransformPosition(End);
                double Enter=0.,Exit=1.,MaxOutside=0.,MaxSupportGap=0.;int32 OutsidePlanes=0;
                bool Intersects=!Planes.IsEmpty();
                for(const auto& Plane:Planes)
                {
                    double MaxPhi=-DBL_MAX;
                    for(const auto& Vertex:Convex.VertexData)MaxPhi=FMath::Max(MaxPhi,Plane.PlaneDot(Vertex));
                    MaxOutside=FMath::Max(MaxOutside,MaxPhi);MaxSupportGap=FMath::Max(MaxSupportGap,-MaxPhi);
                    OutsidePlanes+=MaxPhi>1.e-5;
                    const double Phi=Plane.PlaneDot(LocalStart),Delta=Plane.PlaneDot(LocalEnd)-Phi;
                    if(FMath::Abs(Delta)<1.e-12){if(Phi>0.)Intersects=false;continue;}
                    const double Crossing=-Phi/Delta;
                    if(Delta>0.)Exit=FMath::Min(Exit,Crossing);else Enter=FMath::Max(Enter,Crossing);
                }
                Intersects=Intersects && Enter<=Exit;
                Test.AddInfo(FString::Printf(TEXT("CookedRay055 %s bone=%s inputPoints=%d planes=%d outsidePlanes=%d maxOutside=%.9f maxSupportGap=%.9f intersects=%d enter=%.9f exit=%.9f enterWorld=%s exitWorld=%s"),
                    *Character->GetName(),*DiagnosticBone.ToString(),Convex.VertexData.Num(),Planes.Num(),OutsidePlanes,MaxOutside,MaxSupportGap,
                    Intersects,Enter,Exit,*(Start+(End-Start)*Enter).ToString(),*(Start+(End-Start)*Exit).ToString()));
            }
        }
#if WITH_EDITOR
        if(Bone==TEXT("foot_l"))
        {
            const auto* Data=Mesh->GetSkeletalMeshAsset()->GetResourceForRendering();
            const auto* Skin=Mesh->GetSkinWeightBuffer(0);
            if(Data && Data->LODRenderData.IsValidIndex(0) && Skin)
            {
                const auto& LOD=Data->LODRenderData[0];
                if(LOD.StaticVertexBuffers.PositionVertexBuffer.GetAllowCPUAccess() && Skin->GetNeedsCPUAccess())
                {
                    TArray<FMatrix44f> RefToLocal;Mesh->GetCurrentRefToLocalMatrices(RefToLocal,0);
                    TArray<FVector3f> Positions;USkinnedMeshComponent::ComputeSkinnedPositions(Mesh,Positions,RefToLocal,LOD,*Skin);
                    TArray<uint32> Indices;LOD.MultiSizeIndexContainer.GetIndexBuffer(Indices);
                    TArray<FSkinWeightInfo> Weights;Skin->GetSkinWeights(Weights);
                    double Nearest=DBL_MAX;FVector SkinImpact=FVector::ZeroVector;FString Corners;int32 Triangle=INDEX_NONE;
                    for(const auto& Section:LOD.RenderSections)for(uint32 Face=0;Face<Section.NumTriangles && Section.BaseIndex+Face*3+2<uint32(Indices.Num());++Face)
                    {
                        const uint32 Offset=Section.BaseIndex+Face*3;
                        const uint32 Vertices[]={Indices[Offset],Indices[Offset+1],Indices[Offset+2]};
                        FVector Impact,Normal;
                        if(!FMath::SegmentTriangleIntersection(Start,End,
                            Mesh->GetComponentTransform().TransformPosition(FVector(Positions[Vertices[0]])),
                            Mesh->GetComponentTransform().TransformPosition(FVector(Positions[Vertices[1]])),
                            Mesh->GetComponentTransform().TransformPosition(FVector(Positions[Vertices[2]])),Impact,Normal))continue;
                        const double Distance=FVector::Distance(Start,Impact);
                        if(Distance>=Nearest)continue;
                        Nearest=Distance;SkinImpact=Impact;Triangle=int32(Offset/3);TArray<FString> Bones;
                        for(const uint32 Vertex:Vertices)
                        {
                            const auto& Weight=Weights[Vertex];uint16 Maximum=0;int32 Dominant=INDEX_NONE;
                            for(int32 Influence=0;Influence<MAX_TOTAL_INFLUENCES;++Influence)
                                if(Weight.InfluenceWeights[Influence]>Maximum)
                                {Maximum=Weight.InfluenceWeights[Influence];Dominant=Weight.InfluenceBones[Influence];}
                            const int32 GlobalBone=Section.BoneMap.IsValidIndex(Dominant)?Section.BoneMap[Dominant]:INDEX_NONE;
                            const int32 Control=GlobalBone!=INDEX_NONE?Physics->FindControllingBodyIndex(Mesh->GetSkeletalMeshAsset(),GlobalBone):INDEX_NONE;
                            Bones.Add(FString::Printf(TEXT("v%u:%s/%s"),Vertex,*Mesh->GetBoneName(GlobalBone).ToString(),
                                Control!=INDEX_NONE?*Physics->SkeletalBodySetups[Control]->BoneName.ToString():TEXT("None")));
                        }
                        Corners=FString::Join(Bones,TEXT(","));
                    }
                    Test.AddInfo(FString::Printf(TEXT("SkinRay055 %s fixedFootLine vertices=%d indices=%d triangle=%d distance=%.9f impact=%s dominant/control=%s"),
                        *Character->GetName(),Positions.Num(),Indices.Num(),Triangle,Triangle!=INDEX_NONE?Nearest:-1.,*SkinImpact.ToString(),*Corners));
                }
                else Test.AddInfo(TEXT("SkinRay055 fixedFootLine requires retained editor CPU vertex/skin data"));
            }
        }
#endif
        Enemy->SetActorLocation(Start-FVector(50,0,0));
        FCollisionQueryParams Query(SCENE_QUERY_STAT(Projectile055ArmorBody),false,Enemy);
        const bool Traced=World->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Query);
        if(!Test.TestTrue(FString::Printf(TEXT("Armor ray hits the actual character mesh: aimBone=%s traced=%d hitActor=%s hitComponent=%s hitBone=%s"),
            *Bone.ToString(),Traced,*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()),*Hit.BoneName.ToString()),
            Traced && Hit.GetActor()==Character && Hit.GetComponent()==Character->GetMesh()
                && !Hit.BoneName.IsNone() && Character->GetMesh()->GetBoneIndex(Hit.BoneName)!=INDEX_NONE))return nullptr;
        const auto Bow=HearthwardData::Find(TEXT("items"),TEXT("bow"));
        auto* Arrow=World->SpawnActor<AHearthwardProjectile>(Start,FRotator::ZeroRotator);
        Arrow->EnemyShooter=Enemy;Arrow->Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
        Arrow->Event=FGuid::NewGuid();Arrow->Item=TEXT("arrow");Arrow->Power=HearthwardData::Number(Bow,TEXT("attack"));
        Arrow->Velocity=FVector(HearthwardData::Number(Bow,TEXT("projectileSpeed")),0,0);
        Arrow->RemainingRange=2000;Arrow->Lifetime=1;Arrow->Gravity=0;
        Arrow->Tick(.1f);
        Test.TestTrue(TEXT("Actual enemy projectile lands on the character"),Arrow->Landed && Arrow->HitTarget);
        return Arrow;
    }

    void CheckArmorRays(FAutomationTestBase& Test,ACharacter* Character,const TCHAR* Role)
    {
        auto* Bag=Character->FindComponentByClass<UHearthwardInventoryComponent>();
        auto* Survival=Character->FindComponentByClass<UHearthwardSurvivalComponent>();
        auto* Gameplay=Character->FindComponentByClass<UHearthwardGameplayComponent>();
        const FName Bones[]={TEXT("head"),TEXT("spine_03"),TEXT("calf_l"),TEXT("foot_l")};
        const FName Slots[]={TEXT("head"),TEXT("chest"),TEXT("legs"),TEXT("feet")};
        for(int32 Index=0;Index<4;++Index)
        {
            if(auto* Combat=Character->FindComponentByClass<UHearthwardCombatComponent>())Combat->Cancel();
            Survival->State.Life=EHearthwardLife::Alive;Survival->Health()=Survival->MaxHealth();
            const float HealthBefore=Survival->Health();
            const FHearthwardInventorySnapshot Before=Bag->Snapshot();
            const FGuid ArmorId=Bag->EquippedInstance(Slots[Index]);
            const auto* Armor=Bag->FindInstance(ArmorId);
            const float LocalBonus=Gameplay?Gameplay->Effect(TEXT("local_armor_bonus")):0;
            const float OtherReduction=Gameplay?Gameplay->Effect(TEXT("defense")):0;
            const double Wear=Gameplay?(1-Gameplay->Effect(TEXT("armor_wear_reduction")))/(1+Gameplay->Effect(TEXT("durability"))):1;
            const float Reduction=HearthwardData::Number(HearthwardData::Find(TEXT("items"),Armor->Definition.ToString()),TEXT("defense"))/100+LocalBonus;
            FHitResult Hit;
            auto* Arrow=FireAtBone(Test,Character,Bones[Index],Hit);
            if(!Arrow)return;
            const FString HitBone=Hit.BoneName.ToString();
            const bool CorrectPart=Index==0?(HitBone==TEXT("head") || HitBone.StartsWith(TEXT("neck_")))
                :Index==1?(HitBone==TEXT("pelvis") || HitBone.StartsWith(TEXT("spine_")) || HitBone.StartsWith(TEXT("clavicle_"))
                    || HitBone.StartsWith(TEXT("upperarm_")) || HitBone.StartsWith(TEXT("lowerarm_")) || HitBone.StartsWith(TEXT("hand_")))
                :Index==2?(HitBone.StartsWith(TEXT("thigh_")) || HitBone.StartsWith(TEXT("calf_")))
                :(HitBone.StartsWith(TEXT("foot_")) || HitBone.StartsWith(TEXT("ball_")));
            Test.TestTrue(FString::Printf(TEXT("%s %s ray identifies an actual bone in that armor region: %s"),
                Role,*Slots[Index].ToString(),*HitBone),CorrectPart);
            const float ExpectedDamage=Arrow->Power*(Index==0?3:1)*FMath::Max(.15f,
                (1-FMath::Clamp(Reduction,0.f,1.f))*(1-FMath::Clamp(OtherReduction,0.f,1.f)));
            Test.TestTrue(FString::Printf(TEXT("%s %s arrow uses only the actual struck armor: raw=%.2f expectedDamage=%.4f health=%.4f->%.4f"),
                Role,*Slots[Index].ToString(),Arrow->Power,ExpectedDamage,HealthBefore,Survival->Health()),
                FMath::IsNearlyEqual(Survival->Health(),HealthBefore-ExpectedDamage,.001f));
            for(const auto& Instance:Before.Instances)
            {
                const auto* After=Bag->FindInstance(Instance.Id);
                if(!Test.TestNotNull(TEXT("Armor hit preserves every equipment instance GUID"),After))continue;
                const double ExpectedWear=Instance.Id==ArmorId?Wear:0;
                Test.TestTrue(FString::Printf(TEXT("%s %s GUID %s loses only its own hit wear (spares and other slots stay unchanged)"),
                    Role,*Instance.Definition.ToString(),*Instance.Id.ToString()),
                    FMath::IsNearlyEqual(After->Durability,Instance.Durability-ExpectedWear,.0001));
            }
            Test.TestTrue(TEXT("The struck armor remains equipped by the same GUID"),Bag->EquippedInstance(Slots[Index])==ArmorId);
            Arrow->Destroy();
        }
    }

    void CheckRejectedHit(FAutomationTestBase& Test,ACharacter* Character,const TCHAR* Role)
    {
        auto* Bag=Character->FindComponentByClass<UHearthwardInventoryComponent>();
        auto* Survival=Character->FindComponentByClass<UHearthwardSurvivalComponent>();
        const FHearthwardInventorySnapshot Before=Bag->Snapshot();
        const float HealthBefore=Survival->Health();
        Survival->Settling=true;
        FHitResult Hit;
        auto* Arrow=FireAtBone(Test,Character,TEXT("spine_03"),Hit);
        Survival->Settling=false;
        if(!Arrow)return;
        Test.TestEqual(FString::Printf(TEXT("%s rejects reentrant damage while Survival is settling"),Role),Survival->Health(),HealthBefore);
        for(const auto& Instance:Before.Instances)
        {
            const auto* After=Bag->FindInstance(Instance.Id);
            if(!Test.TestNotNull(TEXT("Rejected hit preserves the equipment GUID"),After))continue;
            Test.TestEqual(FString::Printf(TEXT("%s rejected arrow cannot wear armor GUID %s"),Role,*Instance.Id.ToString()),
                After->Durability,Instance.Durability);
        }
        Arrow->Destroy();
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyArrowHeroCollisionTest,"Hearthward.Combat055.EnemyArrowsHitActualHeroBody",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyArrowHeroCollisionTest::RunTest(const FString&)
{
    FScopedBodyProjectileWorld Fixture;
    if(!Fixture.Ready(*this,Fixture.Hero,TEXT("Hero")))return false;
    Fixture.CheckBodyRays(*this,Fixture.Hero,TEXT("Hero"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyArrowBrotherCollisionTest,"Hearthward.Combat055.EnemyArrowsHitActualBrotherBody",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyArrowBrotherCollisionTest::RunTest(const FString&)
{
    FScopedBodyProjectileWorld Fixture;
    if(!Fixture.Ready(*this,Fixture.Brother,TEXT("Brother")))return false;
    Fixture.CheckBodyRays(*this,Fixture.Brother,TEXT("Brother"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyArrowHeroArmorTest,"Hearthward.Combat055.EnemyArrowsUseHeroArmorInstances",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyArrowHeroArmorTest::RunTest(const FString&)
{
    FScopedBodyProjectileWorld Fixture;
    if(!Fixture.Ready(*this,Fixture.Hero,TEXT("Hero")) || !Fixture.EquipArmorWithSpare(*this,Fixture.Hero))return false;
    Fixture.CheckArmorRays(*this,Fixture.Hero,TEXT("Hero"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyArrowBrotherArmorTest,"Hearthward.Combat055.EnemyArrowsUseBrotherArmorInstances",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyArrowBrotherArmorTest::RunTest(const FString&)
{
    FScopedBodyProjectileWorld Fixture;
    if(!Fixture.Ready(*this,Fixture.Brother,TEXT("Brother")) || !Fixture.EquipArmorWithSpare(*this,Fixture.Brother))return false;
    Fixture.CheckArmorRays(*this,Fixture.Brother,TEXT("Brother"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRejectedEnemyArrowArmorTest,"Hearthward.Combat055.RejectedEnemyArrowCannotWearArmor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRejectedEnemyArrowArmorTest::RunTest(const FString&)
{
    FScopedBodyProjectileWorld Fixture;
    if(!Fixture.Ready(*this,Fixture.Hero,TEXT("Hero")) || !Fixture.EquipArmorWithSpare(*this,Fixture.Hero)
        || !Fixture.Ready(*this,Fixture.Brother,TEXT("Brother")) || !Fixture.EquipArmorWithSpare(*this,Fixture.Brother))return false;
    Fixture.CheckRejectedHit(*this,Fixture.Hero,TEXT("Hero"));
    Fixture.CheckRejectedHit(*this,Fixture.Brother,TEXT("Brother"));
    return true;
}
#if WITH_EDITOR
namespace
{
struct FCombatPhysicsFit055
{
    USkeletalMesh* Mesh=nullptr;
    UPhysicsAsset* Physics=nullptr;
    TArray<FKConvexElem> Convexes;
};

bool PrepareCombatPhysicsFit055(FAutomationTestBase& Test,const TCHAR* Role,FCombatPhysicsFit055& Fit)
{
    const FString MeshPath=FString::Printf(TEXT("/Game/Characters/%s/UE5/SK_%s.SK_%s"),Role,Role,Role);
    Fit.Mesh=LoadObject<USkeletalMesh>(nullptr,*MeshPath);
    if(!Test.TestNotNull(FString::Printf(TEXT("Author055 loads current %s mesh"),Role),Fit.Mesh))return false;
    Fit.Physics=Fit.Mesh->GetPhysicsAsset();
    const FString PhysicsPath=FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-055/Physics/PA_%sCombat.PA_%sCombat"),Role,Role);
    if(!Test.TestNotNull(TEXT("Author055 requires the existing assigned PhysicsAsset"),Fit.Physics)
        || !Test.TestEqual(TEXT("Author055 only writes the locked combat PhysicsAsset"),Fit.Physics->GetPathName(),PhysicsPath)
        || !Test.TestTrue(TEXT("Author055 retains existing body topology"),Fit.Physics->SkeletalBodySetups.Num()>0))return false;
    for(const FName Bone:{FName(TEXT("head")),FName(TEXT("spine_03")),FName(TEXT("calf_l")),FName(TEXT("foot_l"))})
        if(!Test.TestTrue(FString::Printf(TEXT("Author055 %s already has body %s"),Role,*Bone.ToString()),Fit.Physics->FindBodyIndex(Bone)!=INDEX_NONE))return false;

    const auto* RenderData=Fit.Mesh->GetResourceForRendering();
    if(!Test.TestTrue(TEXT("Author055 requires the actual LOD0 render data"),RenderData && RenderData->LODRenderData.IsValidIndex(0)))return false;
    const auto& LOD=RenderData->LODRenderData[0];
    const auto& Positions=LOD.StaticVertexBuffers.PositionVertexBuffer;
    const auto& Skin=LOD.SkinWeightVertexBuffer;
    if(!Test.TestTrue(TEXT("Author055 requires retained CPU vertex and skin data without changing the Mesh"),
        Positions.GetAllowCPUAccess() && Skin.GetNeedsCPUAccess() && Positions.GetNumVertices()>0))return false;
    TArray<FSkinWeightInfo> Weights;Skin.GetSkinWeights(Weights);
    if(!Test.TestEqual(TEXT("Author055 skin weights match actual LOD0 vertices"),Weights.Num(),int32(Positions.GetNumVertices())))return false;
    const auto& Ref=Fit.Mesh->GetRefSkeleton();
    const auto& InvRef=static_cast<const USkeletalMesh*>(Fit.Mesh)->GetRefBasesInvMatrix();
    TArray<int32> BodyBones,PointCounts;
    TArray<FBox> Bounds;
    Bounds.Init(FBox(ForceInit),Fit.Physics->SkeletalBodySetups.Num());
    PointCounts.Init(0,Bounds.Num());Fit.Convexes.SetNum(Bounds.Num());
    for(const auto& Body:Fit.Physics->SkeletalBodySetups)
    {
        if(!Test.TestNotNull(TEXT("Author055 requires an existing skeletal body"),Body.Get()))return false;
        const int32 Bone=Ref.FindBoneIndex(Body->BoneName);
        if(!Test.TestTrue(TEXT("Author055 body belongs to the actual reference skeleton"),InvRef.IsValidIndex(Bone)))return false;
        BodyBones.Add(Bone);
    }
    for(const auto& Section:LOD.RenderSections)
    {
        for(uint32 SectionVertex=0;SectionVertex<Section.NumVertices;++SectionVertex)
        {
            const uint32 Vertex=Section.BaseVertexIndex+SectionVertex;
            if(!Test.TestTrue(TEXT("Author055 section addresses a real LOD0 vertex"),Weights.IsValidIndex(Vertex)))return false;
            const auto& Weight=Weights[Vertex];
            uint16 Maximum=0;int32 Dominant=INDEX_NONE;
            for(int32 Influence=0;Influence<MAX_TOTAL_INFLUENCES;++Influence)
                if(Weight.InfluenceWeights[Influence]>Maximum)
                {Maximum=Weight.InfluenceWeights[Influence];Dominant=Weight.InfluenceBones[Influence];}
            if(!Test.TestTrue(TEXT("Author055 vertex has a positive dominant weight in its actual section"),Maximum>0 && Section.BoneMap.IsValidIndex(Dominant)))return false;
            const int32 Bone=Section.BoneMap[Dominant];
            // Preserve the asset's bone hierarchy: unbodied child vertices belong to their real controlling body.
            const int32 Body=Fit.Physics->FindControllingBodyIndex(Fit.Mesh,Bone);
            if(!Test.TestTrue(TEXT("Author055 dominant bone has an existing controlling body"),BodyBones.IsValidIndex(Body)))return false;
            const FVector3f Local=InvRef[BodyBones[Body]].TransformPosition(Positions.VertexPosition(Vertex));
            if(!Test.TestTrue(TEXT("Author055 bone-local source vertex is finite"),!Local.ContainsNaN()))return false;
            Bounds[Body]+=FVector(Local);++PointCounts[Body];Fit.Convexes[Body].VertexData.AddUnique(FVector(Local));
        }
    }
    for(int32 Body=0;Body<Bounds.Num();++Body)
    {
        const FVector Size=Bounds[Body].GetSize();
        if(!Test.TestTrue(FString::Printf(TEXT("Author055 %s %s has a real nondegenerate skin envelope (%d vertices)"),
            Role,*Fit.Physics->SkeletalBodySetups[Body]->BoneName.ToString(),PointCounts[Body]),
            Bounds[Body].IsValid && PointCounts[Body]>0 && !Size.ContainsNaN() && Size.GetMin()>SMALL_NUMBER))return false;
        auto& Convex=Fit.Convexes[Body];Convex.UpdateElemBox();
        if(!Test.TestTrue(TEXT("Author055 convex has at least four distinct real source points"),Convex.VertexData.Num()>=4))return false;
        // Cook every candidate on a transient setup before either existing PhysicsAsset is changed.
        auto* Cooked=NewObject<USkeletalBodySetup>(GetTransientPackage());
        Cooked->CollisionTraceFlag=Fit.Physics->SkeletalBodySetups[Body]->CollisionTraceFlag;
        Cooked->AggGeom.ConvexElems.Add(Convex);Cooked->InvalidatePhysicsData();Cooked->CreatePhysicsMeshes();
        if(!Test.TestTrue(FString::Printf(TEXT("Author055 %s %s cooks a valid real-point Chaos hull"),
            Role,*Fit.Physics->SkeletalBodySetups[Body]->BoneName.ToString()),!!Cooked->AggGeom.ConvexElems[0].GetChaosConvexMesh()))return false;
        Convex=Cooked->AggGeom.ConvexElems[0];
        Test.AddInfo(FString::Printf(TEXT("Author055 fit %s bone=%s points=%d inputPoints=%d localCenter=%s localSize=%s"),
            Role,*Fit.Physics->SkeletalBodySetups[Body]->BoneName.ToString(),PointCounts[Body],Convex.VertexData.Num(),*Bounds[Body].GetCenter().ToString(),*Size.ToString()));
    }
    return true;
}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAuthorCombatPhysicsFromSkin055Test,"Hearthward.AssetAuthoring055.FitCombatPhysicsFromActualSkin",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
void FAuthorCombatPhysicsFromSkin055Test::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{
    if(FParse::Param(FCommandLine::Get(),TEXT("Hearthward055AuthorPhysics")))
    {Names.Add(TEXT("HeroAndBrother"));Commands.Add(TEXT("HeroAndBrother"));}
}
bool FAuthorCombatPhysicsFromSkin055Test::RunTest(const FString&)
{
    TArray<FCombatPhysicsFit055> Fits;Fits.SetNum(2);
    if(!PrepareCombatPhysicsFit055(*this,TEXT("Hero"),Fits[0])
        || !PrepareCombatPhysicsFit055(*this,TEXT("Brother"),Fits[1]))return false;
    // Both complete candidate sets have been validated before either persistent asset is changed.
    for(auto& Fit:Fits)
    {
        Fit.Physics->Modify();
        for(int32 Body=0;Body<Fit.Convexes.Num();++Body)
        {
            auto* Setup=Fit.Physics->SkeletalBodySetups[Body].Get();
            Setup->Modify();Setup->RemoveSimpleCollision();Setup->AggGeom.ConvexElems.Add(Fit.Convexes[Body]);
            Setup->InvalidatePhysicsData();Setup->CreatePhysicsMeshes();
        }
        Fit.Physics->UpdateBoundsBodiesArray();Fit.Physics->PostEditChange();Fit.Physics->MarkPackageDirty();
        auto* Package=Fit.Physics->GetOutermost();
        const FString Filename=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
        FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.bSlowTask=false;
        if(!TestTrue(TEXT("Author055 saves only the existing combat PhysicsAsset package"),UPackage::SavePackage(Package,Fit.Physics,*Filename,Args)))return false;
        AddInfo(FString::Printf(TEXT("Author055 wrote %s from %s (%d fitted bodies); fresh-process Combat055 projectile tests remain required"),
            *Fit.Physics->GetPathName(),*Fit.Mesh->GetPathName(),Fit.Convexes.Num()));
    }
    return true;
}
#endif
#endif
