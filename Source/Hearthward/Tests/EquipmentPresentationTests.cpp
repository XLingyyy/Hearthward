#include "../HearthwardCharacter.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AttributesRuntime.h"
#include "Animation/AnimCurveTypes.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Misc/MemStack.h"
#include "Engine/StaticMeshSocket.h"
#include "../Animation/HearthwardHeroAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "SkeletalRenderPublic.h"
#include "Serialization/JsonSerializer.h"
#include "../UI/HearthwardScreenWidget.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Widgets/SViewport.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentRepairEntry055Test,
    "Hearthward.Equipment055.RepairEntryKeepsSelectedInstance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEquipmentRepairEntry055Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);
    Instance->InitializeStandalone();
    auto* World=Instance->GetWorld();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);
    const TSharedRef<SViewport> SlateViewport=SNew(SViewport);
    TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SlateViewport);
    const auto Cleanup=[&]()
    {
        SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    };
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);
    Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();
    Controller->SetPlayer(LocalPlayer);
    // This standalone world has not initialized actors for play, so register the controller explicitly.
    World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();
    Controller->Possess(Player);
    Player->SetActorLocation(FVector(0,0,90));
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);
    Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);
    Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);
    Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Combat=NewObject<UHearthwardCombatComponent>(Player);
    Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
    auto* Building=NewObject<UHearthwardBuildingComponent>(Player);
    Player->AddInstanceComponent(Building);Building->RegisterComponent();

    auto* Floor=World->SpawnActor<AActor>();
    auto* FloorBox=NewObject<UBoxComponent>(Floor);
    Floor->AddInstanceComponent(FloorBox);Floor->SetRootComponent(FloorBox);
    FloorBox->SetBoxExtent(FVector(1000,1000,10));
    FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FloorBox->SetCollisionResponseToAllChannels(ECR_Block);FloorBox->RegisterComponent();
    Floor->SetActorLocation(FVector(0,0,-10));
    World->GetSubsystem<UHearthwardCampSubsystem>()->State.AddCamp(TEXT("camp"),FVector::ZeroVector);
    TestTrue(TEXT("Real nearby workbench is available"),Building->AddGift(TEXT("workbench"),FVector(200,0,0)));

    Bag->TryAdd(TEXT("axe"),2);Bag->TryAdd(TEXT("bow"),1);
    const auto Gear=Bag->Snapshot().Instances;
    const FGuid Equipped=Gear[0].Id,Spare=Gear[1].Id,Bow=Gear[2].Id;
    Bag->WearInstance(Equipped,30);Bag->WearInstance(Spare,10);
    Bag->EquipInstance(Equipped);Gameplay->InventoryChanged();
    auto* Storage=World->GetSubsystem<UHearthwardStorageSubsystem>();
    Storage->Adjust({},{{TEXT("wood"),4},{TEXT("stone"),4}});
    auto* Screen=CreateWidget<UHearthwardScreenWidget>(Controller);
    if(!TestNotNull(TEXT("Real equipment widget is created"),Screen))
    {
        Cleanup();return false;
    }
    Screen->SetIsFocusable(true);
    if(!TestEqual(TEXT("Widget resolves its local owning controller"),Screen->GetOwningPlayer(),Controller))
    {
        Cleanup();return false;
    }
    Screen->TakeWidget();Screen->AddToViewport();
    if(!TestTrue(TEXT("Equipment widget is attached to its standalone viewport"),Screen->IsInViewport()))
    {
        Screen->RemoveFromParent();Cleanup();return false;
    }

    // Prime an unrelated previous selection through the same action the equipment UI uses.
    Screen->OpenPage(TEXT("equipment"));
    TestTrue(TEXT("Select bow through real equipment action"),Screen->ExecuteAction(TEXT("gear.select:")+Bow.ToString()));
    Screen->OpenPage(TEXT("inventory"));
    TestTrue(TEXT("Select axe in normal inventory"),Screen->ExecuteAction(TEXT("item:axe")));
    TestTrue(TEXT("Normal inventory repair opens equipment management"),Screen->ExecuteAction(TEXT("repair")));
    TestEqual(TEXT("Repair retains the current equipment page"),Screen->GetPage(),FName(TEXT("equipment")));
    TestTrue(TEXT("Repair quote follows the displayed worn axe, not the previous bow"),Screen->DescribeLayout().Contains(TEXT("恢复20.00")));
    TestTrue(TEXT("Repair uses actual warehouse materials through the UI"),Screen->ExecuteAction(TEXT("gear.repair:25")));
    TestEqual(TEXT("Only the equipped worn axe is repaired"),Bag->FindInstance(Equipped)->Durability,70.);
    TestEqual(TEXT("The spare axe retains its own wear"),Bag->FindInstance(Spare)->Durability,70.);
    TestEqual(TEXT("An unrelated bow remains untouched"),Bag->FindInstance(Bow)->Durability,80.);
    TestEqual(TEXT("The quoted wood cost is paid once"),Storage->GetItemCount(TEXT("wood")),3);
    TestEqual(TEXT("The quoted stone cost is paid once"),Storage->GetItemCount(TEXT("stone")),3);

    // A deliberate same-type instance choice must survive a return through the inventory entry.
    TestTrue(TEXT("Explicitly select the spare through the real UI"),Screen->ExecuteAction(TEXT("gear.select:")+Spare.ToString()));
    Bag->WearInstance(Spare,30);Gameplay->InventoryChanged();
    Screen->OpenPage(TEXT("inventory"));Screen->ExecuteAction(TEXT("item:axe"));
    TestTrue(TEXT("Return to repair for the same item"),Screen->ExecuteAction(TEXT("repair")));
    TestTrue(TEXT("The explicit spare retains its own quote"),Screen->DescribeLayout().Contains(TEXT("恢复20.00")));
    TestTrue(TEXT("Repair the explicitly selected spare"),Screen->ExecuteAction(TEXT("gear.repair:25")));
    TestEqual(TEXT("The explicit spare alone gains durability"),Bag->FindInstance(Spare)->Durability,60.);
    TestEqual(TEXT("Returning through inventory does not switch to the equipped copy"),Bag->FindInstance(Equipped)->Durability,70.);

    TestTrue(TEXT("Browse a different equipment container"),Screen->ExecuteAction(TEXT("gear.owner:storage")));
    Screen->OpenPage(TEXT("inventory"));Screen->ExecuteAction(TEXT("item:axe"));
    TestTrue(TEXT("Inventory repair returns to the player container"),Screen->ExecuteAction(TEXT("repair")));
    TestTrue(TEXT("Player repair quote replaces the previous storage selection"),Screen->DescribeLayout().Contains(TEXT("恢复10.00")));
    TestTrue(TEXT("Repair remains available after browsing storage"),Screen->ExecuteAction(TEXT("gear.repair:25")));
    TestEqual(TEXT("Container reset repairs the equipped player copy"),Bag->FindInstance(Equipped)->Durability,80.);
    TestEqual(TEXT("Container reset leaves the spare unchanged"),Bag->FindInstance(Spare)->Durability,60.);

    Screen->RemoveFromParent();
    Cleanup();
    return true;
}

// Component display configuration only; scene visibility and rendered frames need separate acceptance.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeldAxeInstance055Test,
    "Hearthward.Equipment055.HeldAxeFollowsCurrentInstanceAndMode",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHeldAxeInstance055Test::RunTest(const FString&)
{
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    const auto Cleanup=[&]() { World->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(World);World->DestroyWorld(false); };
    World->InitializeActorsForPlay(FURL());
    if(!TestTrue(TEXT("Normal world initialization permits actor delegate callbacks"),World->AreActorsInitialized()))
    {Cleanup();return false;}
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Hero=World->SpawnActor<AHearthwardCharacter>(FVector(0,0,200),FRotator::ZeroRotator,Params);
    auto* Controller=World->SpawnActor<APlayerController>();
    World->AddController(Controller);Controller->Possess(Hero);
    Hero->Gameplay->Enabled=true;
    World->GetWorldSettings()->NotifyBeginPlay();
    Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    auto* Bag=Hero->FindComponentByClass<UHearthwardInventoryComponent>();
    auto* Combat=Hero->FindComponentByClass<UHearthwardCombatComponent>();
    auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
    if(!TestTrue(TEXT("Real Hero and production inventory completed normal BeginPlay"),Hero->HasActorBegunPlay() && Bag && Bag->HasBegunPlay())
        || !TestNotNull(TEXT("Real Hero owns the production combat component"),Combat)
        || !TestNotNull(TEXT("Timed equipment changes use the production clock"),Clock))
    {Cleanup();return false;}

    TArray<UStaticMeshComponent*> Meshes;
    Hero->GetComponents<UStaticMeshComponent>(Meshes);
    UStaticMeshComponent* Held=nullptr;
    int32 HeldCount=0;
    for(auto* Mesh:Meshes)if(Mesh->GetFName()==TEXT("HeldAxe")) {Held=Mesh;++HeldCount;}
    if(!TestEqual(TEXT("Hero has exactly one existing HeldAxe component"),HeldCount,1)
        || !TestNotNull(TEXT("HeldAxe uses the actual imported stone axe"),Held->GetStaticMesh().Get()))
    {Cleanup();return false;}
    TestEqual(TEXT("HeldAxe retains the formal existing asset"),Held->GetStaticMesh()->GetPathName(),
        FString(TEXT("/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.SM_stone_bone_axe")));
    TestEqual(TEXT("HeldAxe is attached to the real Hero mesh"),Held->GetAttachParent(),static_cast<USceneComponent*>(Hero->GetMesh()));
    TestEqual(TEXT("HeldAxe uses the existing actual hand bone"),Held->GetAttachSocketName(),FName(TEXT("hand_r")));
    TestTrue(TEXT("The attachment bone exists on the formal Hero"),Hero->GetMesh()->GetBoneIndex(TEXT("hand_r"))!=INDEX_NONE);
    TestFalse(TEXT("An empty equipment slot initially hides the axe"),Held->GetVisibleFlag());
    if(!TestTrue(TEXT("Two axe GUIDs fit in the real inventory"),Bag->TryAdd(TEXT("axe"),2)==EHearthwardInventoryResult::Success)
        || !TestTrue(TEXT("A real bow fits in the inventory"),Bag->TryAdd(TEXT("bow"),1)==EHearthwardInventoryResult::Success)
        || !TestTrue(TEXT("A different melee weapon fits in the inventory"),Bag->TryAdd(TEXT("shortblade"),1)==EHearthwardInventoryResult::Success))
    {Cleanup();return false;}
    TArray<FGuid> Axes;
    FGuid Bow,Blade;
    for(const auto& Item:Bag->Snapshot().Instances)
    {
        if(Item.Definition==TEXT("axe"))Axes.Add(Item.Id);
        else if(Item.Definition==TEXT("bow"))Bow=Item.Id;
        else if(Item.Definition==TEXT("shortblade"))Blade=Item.Id;
    }
    if(!TestEqual(TEXT("The axe definition has two distinct actual GUIDs"),Axes.Num(),2)
        || !TestTrue(TEXT("The bow and shortblade have real instance GUIDs"),Bow.IsValid() && Blade.IsValid()))
    {Cleanup();return false;}
    const FGuid Equipped=Axes[0],Spare=Axes[1];
    const auto Switch=[&](FGuid Id)
    {
        if(!TestTrue(TEXT("Normal gameplay starts a timed equipment instance switch"),Hero->Gameplay->EquipInstance(Id)))return false;
        Clock->Tick(.5f);Combat->TickComponent(.5f,LEVELTICK_All,nullptr);
        return TestFalse(TEXT("Normal production clock completes the equipment switch"),Combat->Busy());
    };
    if(!Switch(Equipped)) {Cleanup();return false;}
    TestEqual(TEXT("A full axe equips its selected GUID"),Bag->EquippedInstance(TEXT("weapon")),Equipped);
    TestEqual(TEXT("The gameplay mirror follows the actual equipped axe"),Hero->Gameplay->Equipment.FindRef(TEXT("weapon")),FName(TEXT("axe")));
    TestEqual(TEXT("The positive control axe has full durability"),Hero->Gameplay->EquippedDurability(TEXT("weapon")),80.f);
    TestFalse(TEXT("Equipping melee selects the actual melee mode"),Combat->RangedSelected());
    TestTrue(TEXT("The full currently equipped axe is displayed"),Held->GetVisibleFlag());
    const auto* Collection=Hero->GetLevel()->GetCachedLevelCollection();
    AddInfo(FString::Printf(TEXT("HeldAxe055 component configuration only: flag=%d contextualVisible=%d hiddenInGame=%d collectionVisible=%d"),
        Held->GetVisibleFlag(),Held->IsVisible(),Held->bHiddenInGame,Collection?int32(Collection->IsVisible()):-1));
    if(FParse::Param(FCommandLine::Get(),TEXT("Task055PalmSkinCapture")))
    {
        auto* Mesh=Hero->GetMesh();
        auto* Anim=Cast<UHearthwardHeroAnimInstance>(Mesh->GetAnimInstance());
        auto* Clip=Anim && Anim->Clips.IsValidIndex(6)?Anim->Clips[6].Get():nullptr;
        const auto PreviousMode=Mesh->GetAnimationMode();
        if(!TestNotNull(TEXT("Palm capture uses the actual native attack clip"),Clip)) {Cleanup();return false;}
        Mesh->PlayAnimation(Clip,false);Mesh->SetPlayRate(0);Mesh->SetPosition(.55f,false);
        Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();
        const auto RestoreAnimation=[&]() {Mesh->SetAnimationMode(PreviousMode);};
        const auto* Data=Mesh->GetSkeletalMeshAsset()->GetResourceForRendering();
        const auto* Skin=Mesh->GetSkinWeightBuffer(0);
        if(!TestFalse(TEXT("Actual palm skin capture requires a real RHI"),FParse::Param(FCommandLine::Get(),TEXT("nullrhi")))
            || !TestTrue(TEXT("Palm capture has actual CPU-readable LOD0 weights"),
            Data && Data->LODRenderData.IsValidIndex(0) && Skin
            && Skin->GetNeedsCPUAccess())) {RestoreAnimation();Cleanup();return false;}
        const auto& LOD=Data->LODRenderData[0];
        TArray<FFinalSkinVertex> Positions;Mesh->GetCPUSkinnedVertices(Positions,0);
        TArray<FSkinWeightInfo> Weights;Skin->GetSkinWeights(Weights);
        if(!TestEqual(TEXT("Actual LOD0 skin positions retain every original weight vertex ID"),Positions.Num(),Weights.Num()))
        {RestoreAnimation();Cleanup();return false;}
        const auto VectorJson=[](FVector V)
        {
            TArray<TSharedPtr<FJsonValue>> Result;
            for(double N:{V.X,V.Y,V.Z})Result.Add(MakeShared<FJsonValueNumber>(N));
            return Result;
        };
        const auto TransformJson=[&](const FTransform& T)
        {
            auto Result=MakeShared<FJsonObject>();Result->SetArrayField(TEXT("translation_cm"),VectorJson(T.GetLocation()));
            Result->SetArrayField(TEXT("scale_xyz"),VectorJson(T.GetScale3D()));
            TArray<TSharedPtr<FJsonValue>> Rotation;const FQuat Q=T.GetRotation();
            for(double N:{Q.X,Q.Y,Q.Z,Q.W})Rotation.Add(MakeShared<FJsonValueNumber>(N));
            Result->SetArrayField(TEXT("rotation_quat_xyzw"),Rotation);return Result;
        };
        const auto MatrixJson=[](const FMatrix44f& Matrix)
        {
            TArray<TSharedPtr<FJsonValue>> Result;
            for(int32 Row=0;Row<4;++Row)for(int32 Column=0;Column<4;++Column)
                Result.Add(MakeShared<FJsonValueNumber>(Matrix.M[Row][Column]));
            return Result;
        };
        const FTransform MeshWorld=Mesh->GetComponentTransform(),HandWorld=Mesh->GetSocketTransform(TEXT("hand_r"));
        TSet<int32> HandBones;TArray<TSharedPtr<FJsonValue>> Bones;
        for(int32 Bone=0;Bone<Mesh->GetNumBones();++Bone)
        {
            const FString Name=Mesh->GetBoneName(Bone).ToString();
            if(Name!=TEXT("hand_r") && !(Name.EndsWith(TEXT("_r")) &&
                (Name.Contains(TEXT("thumb")) || Name.Contains(TEXT("index")) || Name.Contains(TEXT("middle"))
                || Name.Contains(TEXT("ring")) || Name.Contains(TEXT("pinky")))))continue;
            HandBones.Add(Bone);auto Entry=MakeShared<FJsonObject>();Entry->SetNumberField(TEXT("mesh_bone_index"),Bone);
            Entry->SetStringField(TEXT("name"),Name);Entry->SetObjectField(TEXT("actual_world"),TransformJson(Mesh->GetSocketTransform(*Name)));
            Bones.Add(MakeShared<FJsonValueObject>(Entry));
        }
        TArray<TSharedPtr<FJsonValue>> Vertices;TSet<uint32> Selected;TSet<int32> UsedSkinningBones;
        for(int32 SectionIndex=0;SectionIndex<LOD.RenderSections.Num();++SectionIndex)
        {
            const auto& Section=LOD.RenderSections[SectionIndex];
            for(uint32 Vertex=Section.BaseVertexIndex;Vertex<Section.BaseVertexIndex+Section.NumVertices;++Vertex)
            {
                const auto& W=Weights[Vertex];int32 Dominant=INDEX_NONE;uint16 Maximum=0;uint32 Total=0;
                for(int32 Influence=0;Influence<MAX_TOTAL_INFLUENCES;++Influence)
                {
                    Total+=W.InfluenceWeights[Influence];
                    if(W.InfluenceWeights[Influence]>Maximum) {Maximum=W.InfluenceWeights[Influence];Dominant=W.InfluenceBones[Influence];}
                }
                const int32 Global=Section.BoneMap.IsValidIndex(Dominant)?Section.BoneMap[Dominant]:INDEX_NONE;
                if(!HandBones.Contains(Global))continue;
                auto Entry=MakeShared<FJsonObject>();Entry->SetNumberField(TEXT("lod0_vertex_id"),Vertex);
                Entry->SetNumberField(TEXT("section_index"),SectionIndex);Entry->SetStringField(TEXT("dominant_bone"),Mesh->GetBoneName(Global).ToString());
                Entry->SetNumberField(TEXT("raw_weight_sum"),Total);
                const FVector Component(Positions[Vertex].Position),WorldPosition=MeshWorld.TransformPosition(Component);
                Entry->SetArrayField(TEXT("source_reference_mesh_cm"),VectorJson(FVector(LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(Vertex))));
                Entry->SetArrayField(TEXT("skinned_component_cm"),VectorJson(Component));
                Entry->SetArrayField(TEXT("skinned_world_cm"),VectorJson(WorldPosition));
                Entry->SetArrayField(TEXT("actual_hand_r_local"),VectorJson(HandWorld.InverseTransformPosition(WorldPosition)));
                TArray<TSharedPtr<FJsonValue>> Influences;
                for(int32 Influence=0;Influence<MAX_TOTAL_INFLUENCES;++Influence)
                {
                    const uint16 Raw=W.InfluenceWeights[Influence];if(!Raw)continue;
                    const int32 Local=W.InfluenceBones[Influence];
                    if(!Section.BoneMap.IsValidIndex(Local)) {RestoreAnimation();Cleanup();AddError(TEXT("Actual skin influence is outside its section BoneMap"));return false;}
                    const int32 Bone=Section.BoneMap[Local];UsedSkinningBones.Add(Bone);auto Weight=MakeShared<FJsonObject>();
                    Weight->SetNumberField(TEXT("section_bone_index"),Local);Weight->SetNumberField(TEXT("mesh_bone_index"),Bone);
                    Weight->SetStringField(TEXT("bone_name"),Mesh->GetBoneName(Bone).ToString());Weight->SetNumberField(TEXT("raw_uint16_weight"),Raw);
                    Influences.Add(MakeShared<FJsonValueObject>(Weight));
                }
                Entry->SetArrayField(TEXT("actual_influences"),Influences);Vertices.Add(MakeShared<FJsonValueObject>(Entry));Selected.Add(Vertex);
            }
        }
        TArray<uint32> Indices;LOD.MultiSizeIndexContainer.GetIndexBuffer(Indices);TArray<TSharedPtr<FJsonValue>> Triangles;
        for(int32 I=0;I+2<Indices.Num();I+=3)
        {
            if(!Selected.Contains(Indices[I]) || !Selected.Contains(Indices[I+1]) || !Selected.Contains(Indices[I+2]))continue;
            TArray<TSharedPtr<FJsonValue>> Triangle;for(int32 Corner=0;Corner<3;++Corner)Triangle.Add(MakeShared<FJsonValueNumber>(Indices[I+Corner]));
            Triangles.Add(MakeShared<FJsonValueArray>(Triangle));
        }
        TArray<int32> UsedIndices=UsedSkinningBones.Array();UsedIndices.Sort();
        TArray<FBoneIndexType> ExtraRequiredBones;for(const int32 Bone:UsedIndices)ExtraRequiredBones.Add(FBoneIndexType(Bone));
        const auto LocalTransforms=Mesh->GetBoneSpaceTransforms();
        const auto& ComponentTransforms=Mesh->GetComponentSpaceTransforms();
        const auto& InvRef=Mesh->GetSkeletalMeshAsset()->GetRefBasesInvMatrix();
        const auto& Reference=Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
        TArray<FMatrix44f> RefToLocal;Mesh->GetCurrentRefToLocalMatrices(RefToLocal,0,&ExtraRequiredBones);
        TArray<TSharedPtr<FJsonValue>> SkinningBones;
        for(const int32 Bone:UsedIndices)
        {
            if(!TestTrue(TEXT("Every actual influence has complete sampled skinning inputs"),
                LocalTransforms.IsValidIndex(Bone) && ComponentTransforms.IsValidIndex(Bone)
                && InvRef.IsValidIndex(Bone) && RefToLocal.IsValidIndex(Bone)))
            {RestoreAnimation();Cleanup();return false;}
            auto Entry=MakeShared<FJsonObject>();Entry->SetNumberField(TEXT("mesh_bone_index"),Bone);
            Entry->SetStringField(TEXT("name"),Mesh->GetBoneName(Bone).ToString());
            const int32 Parent=Reference.GetParentIndex(Bone);Entry->SetNumberField(TEXT("parent_mesh_bone_index"),Parent);
            Entry->SetStringField(TEXT("parent_name"),Parent!=INDEX_NONE?Mesh->GetBoneName(Parent).ToString():FString());
            Entry->SetObjectField(TEXT("actual_local"),TransformJson(LocalTransforms[Bone]));
            Entry->SetObjectField(TEXT("actual_component"),TransformJson(ComponentTransforms[Bone]));
            Entry->SetArrayField(TEXT("inverse_reference_matrix_row_major"),MatrixJson(InvRef[Bone]));
            Entry->SetArrayField(TEXT("actual_ref_to_local_matrix_row_major"),MatrixJson(RefToLocal[Bone]));
            SkinningBones.Add(MakeShared<FJsonValueObject>(Entry));
        }
        auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("method"),TEXT("Normal GUID Equip; actual attack SingleNode .55; public GetCPUSkinnedVertices LOD0 with actual component SkinWeightBuffer"));
        Report->SetArrayField(TEXT("all_used_influence_bone_skinning_inputs"),SkinningBones);
        Report->SetStringField(TEXT("skinning_matrix_convention"),TEXT("FMatrix44f M[row][column], flattened row-major; homogeneous row vector [x,y,z,1] * matrix. Reference-to-component skinning is InvRef * current component ToMatrixWithScale; actual rendering RefToLocal is also recorded independently."));
        Report->SetStringField(TEXT("source_reference_position_meaning"),TEXT("Original LOD0 StaticVertexBuffers.PositionVertexBuffer positions before skinning, indexed by original render vertex ID, in reference mesh centimetres."));
        Report->SetStringField(TEXT("meaning"),TEXT("Measured dominant hand/finger skin surface; no palm-centre or grip-fit acceptance"));
        Report->SetStringField(TEXT("hero_mesh"),Mesh->GetSkeletalMeshAsset()->GetPathName());Report->SetStringField(TEXT("attack_clip"),Clip->GetPathName());
        Report->SetNumberField(TEXT("actual_asset_morph_target_count"),Mesh->GetSkeletalMeshAsset()->GetMorphTargets().Num());
        Report->SetNumberField(TEXT("actual_single_node_time_seconds"),Mesh->GetPosition());
        Report->SetObjectField(TEXT("mesh_world"),TransformJson(MeshWorld));Report->SetObjectField(TEXT("actual_hand_r_world"),TransformJson(HandWorld));
        Report->SetObjectField(TEXT("actual_axe_world"),TransformJson(Held->GetComponentTransform()));Report->SetObjectField(TEXT("actual_axe_relative"),TransformJson(Held->GetRelativeTransform()));
        Report->SetStringField(TEXT("equipped_guid"),Equipped.ToString());Report->SetArrayField(TEXT("actual_right_hand_bones"),Bones);
        Report->SetArrayField(TEXT("lod0_skin_vertices"),Vertices);Report->SetArrayField(TEXT("triangles_fully_inside_selected_vertices"),Triangles);
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("Task055/stone-axe-palm-skin");IFileManager::Get().MakeDirectory(*Directory,true);
        FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
        TestTrue(TEXT("Palm capture saves measured QA JSON only"),FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("actual-hand-skin.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
        TestTrue(TEXT("Actual right-hand surface has measured vertices"),!Vertices.IsEmpty());
        AddInfo(FString::Printf(TEXT("PalmSkin055 originalLOD0=%d selected=%d actualHandBones=%d triangles=%d output=%s"),Positions.Num(),Vertices.Num(),Bones.Num(),Triangles.Num(),*Directory));
        RestoreAnimation();
    }

    TestTrue(TEXT("Wear targets a specific spare instance"),Bag->WearInstance(Spare,1));
    TestEqual(TEXT("Spare wear leaves the currently equipped axe intact"),Bag->FindInstance(Equipped)->Durability,80.);
    TestTrue(TEXT("Wear on a non-current GUID keeps the full current axe visible"),Held->GetVisibleFlag());

    if(!Switch(Equipped)) {Cleanup();return false;}
    TestFalse(TEXT("Selecting the equipped GUID normally unequips the weapon slot"),Bag->EquippedInstance(TEXT("weapon")).IsValid());
    TestEqual(TEXT("Unequipping clears the normal gameplay equipment mirror"),Hero->Gameplay->Equipment.FindRef(TEXT("weapon")),FName());
    TestTrue(TEXT("A usable spare remains owned after normal unequip"),Bag->FindInstance(Spare)->Durability>0);
    TestFalse(TEXT("Owned non-equipped axe GUIDs do not display a held axe"),Held->GetVisibleFlag());
    if(!Switch(Blade)) {Cleanup();return false;}
    TestEqual(TEXT("The normal weapon slot now points to the different weapon GUID"),Bag->EquippedInstance(TEXT("weapon")),Blade);
    TestFalse(TEXT("A different current weapon does not display the owned spare axes"),Held->GetVisibleFlag());

    if(!Switch(Equipped)) {Cleanup();return false;}
    TestTrue(TEXT("Selecting the full axe again restores its visible positive control"),Held->GetVisibleFlag());
    Hero->Gameplay->WearEquipment(TEXT("weapon"),80);
    TestEqual(TEXT("Production wear leaves the broken current axe GUID equipped"),Bag->EquippedInstance(TEXT("weapon")),Equipped);
    TestEqual(TEXT("The equipped GUID has genuinely reached zero durability"),Bag->FindInstance(Equipped)->Durability,0.);
    TestEqual(TEXT("The gameplay mirror remains correctly synchronized to the broken axe"),Hero->Gameplay->Equipment.FindRef(TEXT("weapon")),FName(TEXT("axe")));
    TestEqual(TEXT("The equipped durability query follows the broken GUID"),Hero->Gameplay->EquippedDurability(TEXT("weapon")),0.f);
    TestTrue(TEXT("The different spare GUID remains usable"),Bag->FindInstance(Spare)->Durability>0);
    TestEqual(TEXT("Type inventory still owns both distinct axe instances"),Bag->GetItemCount(TEXT("axe")),2);
    TestFalse(TEXT("A broken current axe stays hidden despite a usable same-type spare"),Held->GetVisibleFlag());

    if(!Switch(Spare)) {Cleanup();return false;}
    TestEqual(TEXT("Normal switch equips the usable spare GUID explicitly"),Bag->EquippedInstance(TEXT("weapon")),Spare);
    TestTrue(TEXT("The usable explicitly equipped spare restores axe visibility"),Held->GetVisibleFlag());
    if(!Switch(Bow)) {Cleanup();return false;}
    TestEqual(TEXT("Normal ranged switch equips the actual bow GUID"),Bag->EquippedInstance(TEXT("ranged")),Bow);
    TestTrue(TEXT("The selected ranged GUID is a usable owned bow"),Bag->FindInstance(Bow)->Durability>0);
    TestTrue(TEXT("Normal ranged equipment switch selects combat ranged mode"),Combat->RangedSelected());
    TestEqual(TEXT("The melee slot legitimately retains its usable axe GUID"),Bag->EquippedInstance(TEXT("weapon")),Spare);
    TestEqual(TEXT("The legacy melee mirror remains correctly synchronized while ranged is selected"),Hero->Gameplay->Equipment.FindRef(TEXT("weapon")),FName(TEXT("axe")));
    TestFalse(TEXT("Ranged bow selection hides the separately equipped melee axe"),Held->GetVisibleFlag());
    Cleanup();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneAxePhaseAndBlade055Test,
    "Hearthward.Equipment055.StoneAxeLightUsesClipPhaseAndPhysicalBlade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoneAxePhaseAndBlade055Test::RunTest(const FString&)
{
    for(const int32 Case:{0,1,2,3,4})
    {
        const int32 FPS=Case==0 || Case==4?30:1;
        const bool Moving=Case==2,StationaryNegative=Case==3,BlockingWall=Case==4;
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
        auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        const auto Cleanup=[&]()
        {
            World->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
        };
        World->InitializeActorsForPlay(FURL());
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Hero=World->SpawnActor<AHearthwardCharacter>(FVector(0,0,90),FRotator(0,Moving?-45:0,0),Params);
        auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Hero);
        Hero->Gameplay->Enabled=true;World->GetWorldSettings()->NotifyBeginPlay();
        Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        auto* Bag=Hero->FindComponentByClass<UHearthwardInventoryComponent>();
        auto* Combat=Hero->FindComponentByClass<UHearthwardCombatComponent>();
        auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
        auto* Mesh=Hero->GetMesh();auto* Anim=Cast<UHearthwardHeroAnimInstance>(Mesh->GetAnimInstance());
        UStaticMeshComponent* Held=nullptr;TArray<UStaticMeshComponent*> Components;Hero->GetComponents(Components);
        for(auto* Component:Components)if(Component->GetFName()==TEXT("HeldAxe"))Held=Component;
        if(!TestTrue(TEXT("Real Hero owns the normal production inventory/combat/animation fixture"),
            Bag && Combat && Clock && Anim && Held && Held->GetStaticMesh() && Anim->Clips.IsValidIndex(6) && Anim->Clips[6]))
        {Cleanup();return false;}
        for(const FName Socket:{FName(TEXT("Grip")),FName(TEXT("BladeBase")),FName(TEXT("BladeTip"))})
            if(!TestNotNull(TEXT("Physical stone axe uses its actually authored socket"),Held->GetStaticMesh()->FindSocket(Socket)))
            {Cleanup();return false;}
        if(!TestTrue(TEXT("Two normal stone axe GUIDs fit in the Hero inventory"),Bag->TryAdd(TEXT("axe"),2)==EHearthwardInventoryResult::Success))
        {Cleanup();return false;}
        TArray<FGuid> Axes;for(const auto& Item:Bag->Snapshot().Instances)if(Item.Definition==TEXT("axe"))Axes.Add(Item.Id);
        if(!TestEqual(TEXT("The normal inventory creates a current axe and a spare GUID"),Axes.Num(),2)
            || !TestTrue(TEXT("Normal gameplay starts the current axe equipment switch"),Hero->Gameplay->EquipInstance(Axes[0])))
        {Cleanup();return false;}
        Clock->Tick(.5f);Combat->TickComponent(.5f,LEVELTICK_All,nullptr);
        if(!TestEqual(TEXT("The timed normal switch equips the chosen GUID"),Bag->EquippedInstance(TEXT("weapon")),Axes[0])
            || !TestTrue(TEXT("Normal current axe equipment displays the actual mesh"),Held->GetVisibleFlag()))
        {Cleanup();return false;}
        Mesh->TickAnimation(.2f,false);Mesh->RefreshBoneTransforms();
        auto* Enemy=World->SpawnActor<AActor>();auto* Body=NewObject<UBoxComponent>(Enemy);
        Enemy->AddInstanceComponent(Body);Enemy->SetRootComponent(Body);
        Body->SetBoxExtent(Case==2 || Case==3?FVector(3,3,3):FVector(30,25,45));Body->ComponentTags.Add(TEXT("body"));
        Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Body->SetCollisionResponseToAllChannels(ECR_Ignore);
        Body->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Body->RegisterComponent();
        Enemy->SetActorLocation(Case==2 || Case==3?FVector(72,30,150):FVector(110,-35,125));
        if(BlockingWall)
        {
            auto* Wall=World->SpawnActor<AActor>();auto* Collision=NewObject<UBoxComponent>(Wall);
            Wall->AddInstanceComponent(Collision);Wall->SetRootComponent(Collision);
            Collision->SetBoxExtent(FVector(1,35,65));Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Collision->SetCollisionResponseToAllChannels(ECR_Ignore);Collision->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
            Collision->RegisterComponent();Wall->SetActorLocation(FVector(74,-35,145));
            FCollisionQueryParams Query(SCENE_QUERY_STAT(StoneAxe055ActualBlockingWall),false,Hero);
            FHitResult WallHit;
            TestTrue(TEXT("The fixed physical wall actually blocks visibility in front of the same body box"),
                World->LineTraceSingleByChannel(WallHit,FVector(60,-35,145),FVector(110,-35,145),ECC_Visibility,Query)
                && WallHit.GetActor()==Wall);
        }
        auto* Target=NewObject<UHearthwardCombatTargetComponent>(Enemy);Enemy->AddInstanceComponent(Target);
        Target->Id=TEXT("stone_axe_physical_test");Target->Health=Target->MaximumHealth=100;Target->RegisterComponent();
        Hero->Gameplay->Opponents.Add(Target->Id,Target->Health);
        const float InitialHealth=Target->Health,Power=Hero->Gameplay->AttackPower();
        const double CurrentWear=Bag->FindInstance(Axes[0])->Durability,SpareWear=Bag->FindInstance(Axes[1])->Durability;
        if(!TestTrue(TEXT("Actual normal stone axe light attack is accepted"),Combat->Attack(false))) {Cleanup();return false;}
        if(Moving)
        {
            const FTransform Before=Mesh->GetComponentTransform();
            Hero->SetActorLocationAndRotation(FVector(0,120,90),FRotator(0,45,0),false,nullptr,ETeleportType::TeleportPhysics);
            AddInfo(FString::Printf(TEXT("StoneAxe055 actual one-frame movement: previousMesh=%s currentMesh=%s fixedTarget=%s"),
                *Before.ToString(),*Mesh->GetComponentTransform().ToString(),*Enemy->GetActorLocation().ToString()));
        }
        double Advanced=0;
        const auto AdvanceTo=[&](double End)
        {
            while(Advanced<End-1.e-7)
            {
                const float Delta=FMath::Min(1.f/FPS,float(End-Advanced));
                Clock->Tick(Delta);Combat->TickComponent(Delta,LEVELTICK_All,nullptr);
                Mesh->TickAnimation(Delta,false);Mesh->RefreshBoneTransforms();Advanced+=Delta;
            }
        };
        if(Case==0)
        {
            AdvanceTo(.35);
            FTransform Expected;
            {
                FMemMark Mark(FMemStack::Get());const auto& Bones=Anim->GetRequiredBones();
                FCompactPose Pose;Pose.SetBoneContainer(&Bones);Pose.ResetToRefPose();FBlendedCurve Curve;Curve.InitFrom(Bones);
                UE::Anim::FStackAttributeContainer Attributes;FAnimationPoseData Data(Pose,Curve,Attributes);
                Anim->Clips[6]->GetAnimationPose(Data,FAnimExtractContext(Anim->Clips[6]->GetPlayLength()*.6,false));
                FCSPose<FCompactPose> ComponentPose;ComponentPose.InitPose(MoveTemp(Pose));
                const auto Hand=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Mesh->GetBoneIndex(TEXT("hand_r"))));
                Expected=ComponentPose.GetComponentSpaceTransform(Hand)*Mesh->GetComponentTransform();
            }
            const FTransform Actual=Mesh->GetSocketTransform(TEXT("hand_r"));
            const double LocationError=FVector::Distance(Actual.GetLocation(),Expected.GetLocation());
            const double RotationError=Actual.GetRotation().AngularDistance(Expected.GetRotation());
            TestTrue(FString::Printf(TEXT("At logical .35 the real native graph has reached source .6L (.55), after its .12 blend: elapsed=%.9f locationError=%.6f rotationError=%.6f"),
                Combat->Elapsed,LocationError,RotationError),LocationError<=.05 && RotationError<=.002);
            AdvanceTo(.40);
            TestEqual(TEXT("At .40 the measured physical blade has not reached the fixed body box"),Target->Health,InitialHealth);
            TestEqual(TEXT("No pre-contact physical strike wears the current axe GUID"),Bag->FindInstance(Axes[0])->Durability,CurrentWear);
            AdvanceTo(.55);
        }
        else AdvanceTo(1.);
        const float ExpectedHealth=InitialHealth-(StationaryNegative || BlockingWall?0:Power);
        const double ExpectedWear=CurrentWear-(StationaryNegative || BlockingWall?0:1.);
        TestTrue(FString::Printf(TEXT("Physical blade uses real pose and previous/current world movement at %dfps case%d: health=%.3f expected=%.3f actualPower=%.3f"),FPS,Case,Target->Health,ExpectedHealth,Power),
            FMath::IsNearlyEqual(Target->Health,ExpectedHealth,.001f));
        TestEqual(TEXT("Current weapon wear follows an actual physical contact"),Bag->FindInstance(Axes[0])->Durability,ExpectedWear);
        TestEqual(TEXT("The same-type spare GUID is unaffected by the real blade strike"),Bag->FindInstance(Axes[1])->Durability,SpareWear);
        AdvanceTo(1.1);
        TestTrue(TEXT("The entire recovery cannot repeat physical damage"),FMath::IsNearlyEqual(Target->Health,ExpectedHealth,.001f));
        TestEqual(TEXT("Recovery does not charge the current GUID again"),Bag->FindInstance(Axes[0])->Durability,ExpectedWear);
        TestFalse(TEXT("Actual production time completes the light attack"),Combat->Busy());
        Cleanup();
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneAxeHeavyPhaseAndBlade055Test,
    "Hearthward.Equipment055.StoneAxeHeavyUsesCurrentMoveAndPhysicalBlade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoneAxeHeavyPhaseAndBlade055Test::RunTest(const FString&)
{
    for(const int32 Case:{0,1,2})
    {
        const int32 FPS=Case==1?1:30;
        const bool OutsideBlade=Case==2;
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).RequiresHitProxies(false).EnableTraceCollision(true);
        auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        const auto Cleanup=[&]()
        {
            World->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
        };
        World->InitializeActorsForPlay(FURL());
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Hero=World->SpawnActor<AHearthwardCharacter>(FVector(0,0,90),FRotator::ZeroRotator,Params);
        auto* Controller=World->SpawnActor<APlayerController>();World->AddController(Controller);Controller->Possess(Hero);
        Hero->Gameplay->Enabled=true;World->GetWorldSettings()->NotifyBeginPlay();
        Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        auto* Bag=Hero->FindComponentByClass<UHearthwardInventoryComponent>();
        auto* Combat=Hero->FindComponentByClass<UHearthwardCombatComponent>();
        auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
        auto* Mesh=Hero->GetMesh();auto* Anim=Cast<UHearthwardHeroAnimInstance>(Mesh->GetAnimInstance());
        UStaticMeshComponent* Held=nullptr;TArray<UStaticMeshComponent*> Components;Hero->GetComponents(Components);
        for(auto* Component:Components)if(Component->GetFName()==TEXT("HeldAxe"))Held=Component;
        if(!TestTrue(TEXT("Heavy fixture uses the actual Hero animation and held axe"),
            Bag && Combat && Clock && Anim && Held && Held->GetStaticMesh() && Anim->Clips.IsValidIndex(6) && Anim->Clips[6]))
        {Cleanup();return false;}
        for(const FName Socket:{FName(TEXT("Grip")),FName(TEXT("BladeBase")),FName(TEXT("BladeTip"))})
            if(!TestNotNull(TEXT("Heavy strike requires its actual mesh socket"),Held->GetStaticMesh()->FindSocket(Socket)))
            {Cleanup();return false;}
        if(!TestTrue(TEXT("Normal inventory creates two stone axes"),Bag->TryAdd(TEXT("axe"),2)==EHearthwardInventoryResult::Success))
        {Cleanup();return false;}
        TArray<FGuid> Axes;for(const auto& Item:Bag->Snapshot().Instances)if(Item.Definition==TEXT("axe"))Axes.Add(Item.Id);
        if(!TestEqual(TEXT("Heavy fixture has current and spare instance GUIDs"),Axes.Num(),2)
            || !TestTrue(TEXT("Normal gameplay requests the actual instance switch"),Hero->Gameplay->EquipInstance(Axes[0])))
        {Cleanup();return false;}
        Clock->Tick(.5f);Combat->TickComponent(.5f,LEVELTICK_All,nullptr);
        if(!TestEqual(TEXT("The real timed switch equipped the chosen axe GUID"),Bag->EquippedInstance(TEXT("weapon")),Axes[0])
            || !TestTrue(TEXT("The actually equipped axe is visible"),Held->GetVisibleFlag()))
        {Cleanup();return false;}
        if(Case==0 && !TestTrue(TEXT("Production Learn spends the existing budget on the approved strong skill"),Hero->Gameplay->Learn(TEXT("strong"))))
        {Cleanup();return false;}
        const float Skill=Hero->Gameplay->Effect(TEXT("heavy_damage"));
        if(Case==0)TestTrue(TEXT("The normally learned strong rank has a real nonzero heavy damage effect"),Skill>0);
        Mesh->TickAnimation(.2f,false);Mesh->RefreshBoneTransforms();
        auto* Enemy=World->SpawnActor<AActor>();auto* Body=NewObject<UBoxComponent>(Enemy);
        Enemy->AddInstanceComponent(Body);Enemy->SetRootComponent(Body);
        Body->SetBoxExtent(OutsideBlade?FVector(3):FVector(30,25,45));Body->ComponentTags.Add(TEXT("body"));
        Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Body->SetCollisionResponseToAllChannels(ECR_Ignore);
        Body->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Body->RegisterComponent();
        Enemy->SetActorLocation(OutsideBlade?FVector(150,0,100):FVector(110,-35,125));
        if(OutsideBlade)
        {
            FHitResult ReachHit;FCollisionQueryParams Query(SCENE_QUERY_STAT(StoneAxe055HeavyReachNegative),false,Hero);
            TestTrue(TEXT("The distant negative body actually blocks the legacy reach through its centre"),
                World->LineTraceSingleByChannel(ReachHit,FVector(0,0,100),FVector(200,0,100),ECC_Visibility,Query)
                && ReachHit.GetActor()==Enemy);
        }
        auto* Target=NewObject<UHearthwardCombatTargetComponent>(Enemy);Enemy->AddInstanceComponent(Target);
        Target->Id=TEXT("stone_axe_heavy_physical_test");Target->Health=Target->MaximumHealth=100;Target->RegisterComponent();
        Hero->Gameplay->Opponents.Add(Target->Id,Target->Health);
        const auto Move=HearthwardCombat::Move(TEXT("blunt"),true);
        const float Power=Hero->Gameplay->AttackPower();
        const float ExpectedDamage=Power*Move.Multiplier/HearthwardCombat::Move(TEXT("blunt"),false).Multiplier*(1+Skill);
        const float InitialHealth=Target->Health,InitialStamina=Hero->Gameplay->Stamina;
        const float StaminaCost=Move.Cost*Bag->GetStaminaCostMultiplier()*FMath::Max(.1f,1-Hero->Gameplay->Effect(TEXT("cost")));
        const double CurrentWear=Bag->FindInstance(Axes[0])->Durability,SpareWear=Bag->FindInstance(Axes[1])->Durability;
        if(!TestTrue(TEXT("The production heavy input starts the normal action"),Hero->Gameplay->HeavyAttack()))
        {Cleanup();return false;}
        TestEqual(TEXT("Heavy action retains the actual rules duration"),Combat->Duration,Move.Duration());
        TestTrue(TEXT("Heavy action pays its own stamina cost once"),FMath::IsNearlyEqual(Hero->Gameplay->Stamina,InitialStamina-StaminaCost,.001f));
        double Advanced=0;
        const auto AdvanceTo=[&](double End)
        {
            while(Advanced<End-1.e-7)
            {
                const float Delta=FMath::Min(1.f/FPS,float(End-Advanced));
                Clock->Tick(Delta);Combat->TickComponent(Delta,LEVELTICK_All,nullptr);
                Mesh->TickAnimation(Delta,false);Mesh->RefreshBoneTransforms();Advanced+=Delta;
            }
        };
        if(Case==0)
        {
            AdvanceTo(Move.Windup);
            FTransform Expected;
            {
                FMemMark Mark(FMemStack::Get());const auto& Bones=Anim->GetRequiredBones();
                FCompactPose Pose;Pose.SetBoneContainer(&Bones);Pose.ResetToRefPose();FBlendedCurve Curve;Curve.InitFrom(Bones);
                UE::Anim::FStackAttributeContainer Attributes;FAnimationPoseData Data(Pose,Curve,Attributes);
                Anim->Clips[6]->GetAnimationPose(Data,FAnimExtractContext(Anim->Clips[6]->GetPlayLength()*.6,false));
                FCSPose<FCompactPose> ComponentPose;ComponentPose.InitPose(MoveTemp(Pose));
                const auto Hand=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Mesh->GetBoneIndex(TEXT("hand_r"))));
                Expected=ComponentPose.GetComponentSpaceTransform(Hand)*Mesh->GetComponentTransform();
            }
            const FTransform Actual=Mesh->GetSocketTransform(TEXT("hand_r"));
            const double LocationError=FVector::Distance(Actual.GetLocation(),Expected.GetLocation());
            const double RotationError=Actual.GetRotation().AngularDistance(Expected.GetRotation());
            TestTrue(FString::Printf(TEXT("Heavy native graph reaches source .6L at its own windup boundary: elapsed=%.9f locationError=%.6f rotationError=%.6f"),
                Combat->Elapsed,LocationError,RotationError),LocationError<=.05 && RotationError<=.002);
            TestEqual(TEXT("Heavy preparation has not hit the fixed physical body"),Target->Health,InitialHealth);
            AdvanceTo(Move.Windup+Move.Active*.25);
            TestEqual(TEXT("The early heavy active pose still has no physical blade contact"),Target->Health,InitialHealth);
            TestEqual(TEXT("No early heavy physical contact means no current GUID wear"),Bag->FindInstance(Axes[0])->Durability,CurrentWear);
            AdvanceTo(Move.Windup+Move.Active+.02);
        }
        else AdvanceTo(Move.Duration()+.02);
        const float ExpectedHealth=InitialHealth-(OutsideBlade?0:ExpectedDamage);
        const double ExpectedWear=CurrentWear-(OutsideBlade?0:2.);
        TestTrue(FString::Printf(TEXT("Heavy physical blade at %dfps case%d: health=%.3f expected=%.3f power=%.3f skill=%.3f"),
            FPS,Case,Target->Health,ExpectedHealth,Power,Skill),FMath::IsNearlyEqual(Target->Health,ExpectedHealth,.001f));
        TestEqual(TEXT("A physical heavy hit wears only its current axe GUID by two durability units once"),Bag->FindInstance(Axes[0])->Durability,ExpectedWear);
        TestEqual(TEXT("The same-definition spare never pays the heavy wear"),Bag->FindInstance(Axes[1])->Durability,SpareWear);
        AdvanceTo(Move.Duration()+.05);
        TestTrue(TEXT("Heavy recovery never repeats body damage"),FMath::IsNearlyEqual(Target->Health,ExpectedHealth,.001f));
        TestEqual(TEXT("Heavy recovery never repeats current GUID wear"),Bag->FindInstance(Axes[0])->Durability,ExpectedWear);
        TestTrue(TEXT("Heavy recovery never repeats the initial stamina payment"),FMath::IsNearlyEqual(Hero->Gameplay->Stamina,InitialStamina-StaminaCost,.001f));
        TestFalse(TEXT("Normal heavy time completes the actual action"),Combat->Busy());
        Cleanup();
    }
    return true;
}

#endif
