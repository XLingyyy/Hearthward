#include "HearthwardPresentationComponent.h"
#include "HearthwardPlayerSettings.h"
#include "HearthwardFootContactNotify.h"
#include "HearthwardEnvironmentLoopWave.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../UI/HearthwardHUD.h"
#include "../UI/HearthwardScreenWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "CoreGlobals.h"
#include "Sound/SoundWaveProcedural.h"
#include "Audio.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
namespace
{
bool PlayerSuccessKey(FName Key,FName& Kind)
{
    FString Prefix,Target;if(!Key.ToString().Split(TEXT(":"),&Prefix,&Target) || Target==TEXT("any"))return false;
    Kind=FName(*Prefix);return Kind==TEXT("craft") || Kind==TEXT("repair") || Kind==TEXT("harvest");
}
}
UHearthwardPresentationComponent::UHearthwardPresentationComponent()
{PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bTickEvenWhenPaused=true;PrimaryComponentTick.TickGroup=TG_PostPhysics;}
void UHearthwardPresentationComponent::BeginPlay()
{
    Super::BeginPlay();
    FString Text;TSharedPtr<FJsonObject> Root;
    if(FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("Resources/Data/experience.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root))
    {
        for(const auto& Value:Root->GetArrayField(TEXT("fixed_dialogue")))
        {const auto Row=Value->AsObject();Cues.Add(FName(*Row->GetStringField(TEXT("cue_id"))),{Row->GetStringField(TEXT("speaker")),Row->GetStringField(TEXT("text")),Row->GetStringField(TEXT("audio_group")),Row->GetStringField(TEXT("voice_status"))});}
        const TArray<TSharedPtr<FJsonValue>>* Sounds;
        if(Root->TryGetArrayField(TEXT("sound_events"),Sounds))for(const auto& Value:*Sounds)
        {const auto Row=Value->AsObject();SoundCues.Add(FName(*Row->GetStringField(TEXT("event_id"))),{Row->GetStringField(TEXT("file")),FName(*Row->GetStringField(TEXT("channel")))});}
    }
    InitializeEnvironment();
    GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.AddDynamic(this,&UHearthwardPresentationComponent::Restored);
    GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->OnTransferred.AddDynamic(this,&UHearthwardPresentationComponent::StorageTransferred);
    GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->OnChanged.AddDynamic(this,&UHearthwardPresentationComponent::GameplayChanged);
    if(auto* Character=Cast<ACharacter>(GetOwner()))
    {
        Character->LandedDelegate.AddDynamic(this,&UHearthwardPresentationComponent::CharacterLanded);
        Character->MovementModeChangedDelegate.AddDynamic(this,&UHearthwardPresentationComponent::CharacterMovementChanged);
    }
    Restored();
}
void UHearthwardPresentationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    StopFixedCue();StopEffects();ReleaseEnvironment();PendingLanding.Invalidate();
    UnbindCombatSources();
    GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.RemoveDynamic(this,&UHearthwardPresentationComponent::Restored);
    GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->OnTransferred.RemoveDynamic(this,&UHearthwardPresentationComponent::StorageTransferred);
    GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->OnChanged.RemoveDynamic(this,&UHearthwardPresentationComponent::GameplayChanged);
    if(auto* Character=Cast<ACharacter>(GetOwner()))
    {
        Character->LandedDelegate.RemoveDynamic(this,&UHearthwardPresentationComponent::CharacterLanded);
        Character->MovementModeChangedDelegate.RemoveDynamic(this,&UHearthwardPresentationComponent::CharacterMovementChanged);
    }
    Super::EndPlay(Reason);
}
void UHearthwardPresentationComponent::Restored()
{
    StopFixedCue();StopEffects();StopEnvironment();ObservedTransfers.Reset();ObservedCombatEvents.Reset();ObservedFootContacts.Reset();PlayedEvents.Reset();ObservedBrother.Reset();
    PreviousInitiative=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->GetInitiativeKind();
    Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    PreviousHealth=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->Health;ShakeRemaining=0;SeedSuccessEvents();
    AHearthwardCompanionFixture* Brother=nullptr;for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It){Brother=*It;break;}
    BindCombatSources(Brother);
}
void UHearthwardPresentationComponent::SeedSuccessEvents()
{
    ObservedNPCEvents.Reset();ObservedPlayerEvents.Reset();ObservedMovementEvents.Reset();PendingLanding.Invalidate();
    if(const auto* Character=Cast<ACharacter>(GetOwner()))ObservedMovementMode=Character->GetCharacterMovement()->MovementMode;
    AwaitingLanding=ObservedMovementMode==MOVE_Falling;
    for(const auto& Event:GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->GetEvents())if(Event.Id.IsValid())ObservedNPCEvents.Add(Event.Id);
    for(const auto& Event:GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->Events)
    {FName Kind;if(PlayerSuccessKey(Event.Key,Kind))ObservedPlayerEvents.Add(Event.Key,Event.Value);}
    ObserveProgressSuccess(true);
}
void UHearthwardPresentationComponent::ObserveProgressSuccess(bool Seed)
{
    const bool History=Seed || GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring();
    const auto Observe=[this,History](FName Key,int32 Count,FName Event)
    {
        const int32 Before=ObservedPlayerEvents.FindRef(Key);ObservedPlayerEvents.Add(Key,Count);
        if(!History && Count>Before && !Event.IsNone())PlaySoundEvent(Event,FGuid::NewGuid());
    };
    const auto& Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    for(const FName Person:Camp.Rescued)
        Observe(FName(*(TEXT("rescue:")+Person.ToString())),1,TEXT("camp.rescue"));
    for(const auto& Facility:Camp.Facilities)
        Observe(FName(*(TEXT("facility_level:")+Facility.Id.ToString())),Facility.Level,
            Facility.Level>1?FName(TEXT("facility.upgrade")):NAME_None);
    for(const FName Quest:GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->Claimed)
        Observe(FName(*(TEXT("quest_claimed:")+Quest.ToString())),1,TEXT("quest.claimed"));
}
void UHearthwardPresentationComponent::GameplayChanged()
{
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch() || GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring())
    {Restored();return;}
    for(const auto& Event:GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->Events)
    {
        FName Kind;if(!PlayerSuccessKey(Event.Key,Kind))continue;
        const int32 Before=ObservedPlayerEvents.FindRef(Event.Key);ObservedPlayerEvents.Add(Event.Key,Event.Value);
        if(Event.Value>Before)PlaySoundEvent(FName(*(FString(TEXT("player."))+Kind.ToString())),FGuid::NewGuid());
    }
}
void UHearthwardPresentationComponent::CharacterLanded(const FHitResult& Hit)
{
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch() || GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring())
    {Restored();return;}
    if(!AwaitingLanding || !Hit.IsValidBlockingHit())return;
    // Landed is emitted before CharacterMovement changes Falling to Walking.
    AwaitingLanding=false;++ObservedMovementEvents.FindOrAdd(TEXT("movement.landed"));
    // Hero/Brother apply fall damage after Super::Landed; settle audio in PostPhysics after that result.
    if(SoundCues.Contains(TEXT("movement.landed")) && !GetWorld()->IsPaused()
        && GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>()->Alive())PendingLanding=FGuid::NewGuid();
}
void UHearthwardPresentationComponent::CharacterMovementChanged(ACharacter* Character,EMovementMode PreviousMode,uint8 PreviousCustomMode)
{
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch() || GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring())
    {Restored();return;}
    const EMovementMode Mode=Character->GetCharacterMovement()->MovementMode;
    if(Mode==ObservedMovementMode)return;
    const EMovementMode Before=ObservedMovementMode;ObservedMovementMode=Mode;AwaitingLanding=Mode==MOVE_Falling;
    const FName Event=Mode==MOVE_Swimming?FName(TEXT("movement.swim.enter")):Before==MOVE_Swimming?FName(TEXT("movement.swim.exit")):NAME_None;
    if(Event.IsNone())return;
    ++ObservedMovementEvents.FindOrAdd(Event);PlaySoundEvent(Event,FGuid::NewGuid());
}
void UHearthwardPresentationComponent::ObserveNPCSuccess(AHearthwardCompanionFixture* Brother)
{
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    if(Save->IsRestoring()){SeedSuccessEvents();return;}
    for(const auto& Event:GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->GetEvents())
    {
        if(!Event.Id.IsValid() || ObservedNPCEvents.Contains(Event.Id))continue;
        // History is consumed even when its cue is unbound or its source cannot be heard now.
        ObservedNPCEvents.Add(Event.Id);
        if(Event.Campaign!=Save->GetCampaignId() || Event.Count<=0 || !Brother || !Brother->CanCommunicate(GetOwner())
            || (Event.Kind!=TEXT("acquired") && Event.Kind!=TEXT("craft") && Event.Kind!=TEXT("repair") && Event.Kind!=TEXT("delivered")))continue;
        if(Event.Kind==TEXT("delivered") && Event.Reason==TEXT("camp_region_completed"))continue;
        bool StorageAlreadyPlayed=false;
        if(Event.Kind==TEXT("delivered") && Event.Reason.IsEmpty())
        {
            const auto* Item=HearthwardBasicItems().FindByPredicate([&](const auto& Row){return Row.Id==Event.Item;});
            if(Item && Item->IsInstance())for(int32 I=0;I<Event.Count;++I)
                if(PlayedEvents.Contains(FGuid(Event.Id.A,Event.Id.B,Event.Id.C,Event.Id.D^uint32(I+1)))){StorageAlreadyPlayed=true;break;}
        }
        if(!StorageAlreadyPlayed)PlaySoundEvent(FName(*(FString(TEXT("npc."))+Event.Kind.ToString())),Event.Id);
    }
}
void UHearthwardPresentationComponent::StopEffects()
{
    for(UAudioComponent* Effect:Effects)if(IsValid(Effect)){Effect->Stop();Effect->DestroyComponent();}
    Effects.Reset();EffectRemaining.Reset();
}
void UHearthwardPresentationComponent::InitializeEnvironment()
{
    if(!GetWorld()->IsGameWorld())return;
    FString Text;TSharedPtr<FJsonObject> Geometry;
    if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("Resources/Data/TASK-099-water-audio.json")))
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Geometry))return;
    const TArray<TSharedPtr<FJsonValue>> *Vertices,*Triangles;
    if(!Geometry->TryGetStringField(TEXT("mesh"),EnvironmentMesh)
        || !Geometry->TryGetArrayField(TEXT("vertices_local_cm"),Vertices)
        || !Geometry->TryGetArrayField(TEXT("triangles"),Triangles))return;
    for(const auto& Value:*Vertices)
    {
        const TArray<TSharedPtr<FJsonValue>>* Point;
        if(!Value->TryGetArray(Point) || Point->Num()!=3){EnvironmentVertices.Reset();return;}
        const FVector Position((*Point)[0]->AsNumber(),(*Point)[1]->AsNumber(),(*Point)[2]->AsNumber());
        if(Position.ContainsNaN()){EnvironmentVertices.Reset();return;}EnvironmentVertices.Add(Position);
    }
    for(const auto& Value:*Triangles)
    {
        const TArray<TSharedPtr<FJsonValue>>* Index;
        if(!Value->TryGetArray(Index) || Index->Num()!=3){EnvironmentTriangles.Reset();return;}
        const FIntVector Triangle((*Index)[0]->AsNumber(),(*Index)[1]->AsNumber(),(*Index)[2]->AsNumber());
        if(!EnvironmentVertices.IsValidIndex(Triangle.X) || !EnvironmentVertices.IsValidIndex(Triangle.Y)
            || !EnvironmentVertices.IsValidIndex(Triangle.Z)){EnvironmentTriangles.Reset();return;}
        EnvironmentTriangles.Add(Triangle);
    }
    if(EnvironmentVertices.IsEmpty() || EnvironmentTriangles.IsEmpty())return;
    EnvironmentTag=FName(*Geometry->GetStringField(TEXT("required_actor_tag")));
    EnvironmentEvent=FName(*Geometry->GetStringField(TEXT("event_id")));
    EnvironmentSpawnHandle=GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this,&UHearthwardPresentationComponent::EnvironmentActorSpawned));
    EnvironmentRegisterHandle=GetWorld()->AddOnPostRegisterAllActorComponentsHandler(FOnPostRegisterAllActorComponents::FDelegate::CreateUObject(this,&UHearthwardPresentationComponent::CacheEnvironmentActor));
    FWorldDelegates::LevelAddedToWorld.AddUObject(this,&UHearthwardPresentationComponent::EnvironmentLevelAdded);
    FWorldDelegates::LevelRemovedFromWorld.AddUObject(this,&UHearthwardPresentationComponent::EnvironmentLevelRemoved);
    for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)CacheEnvironmentActor(*It);
}
bool UHearthwardPresentationComponent::PrepareEnvironmentWave()
{
    if(EnvironmentWave)return true;
    if(EnvironmentWaveAttempted)return false;
    EnvironmentWaveAttempted=true;
    const auto* Cue=SoundCues.Find(EnvironmentEvent);if(!Cue || Cue->Channel!=TEXT("environment"))return false;
    TArray<uint8> Data;FWaveModInfo Info;const FString File=FPaths::ProjectDir()/TEXT("Resources/Audio")/Cue->File;
    if(!FFileHelper::LoadFileToArray(Data,*File) || !Info.ReadWaveInfo(Data.GetData(),Data.Num())
        || *Info.pFormatTag!=1 || *Info.pBitsPerSample!=16 || *Info.pChannels!=1)
    {UE_LOG(LogTemp,Warning,TEXT("Invalid mono PCM16 environment sound: %s"),*File);return false;}
    EnvironmentWave=NewObject<UHearthwardEnvironmentLoopWave>(this);
    if(!EnvironmentWave->InitializePCM(Info.SampleDataStart,Info.SampleDataSize,*Info.pSamplesPerSec))
    {EnvironmentWave=nullptr;return false;}return true;
}
void UHearthwardPresentationComponent::CacheEnvironmentActor(AActor* Actor)
{
    auto* Static=Cast<AStaticMeshActor>(Actor);if(!Static || Static->GetWorld()!=GetWorld() || Static->IsActorBeingDestroyed())return;
    auto* Component=Static->GetStaticMeshComponent();const UStaticMesh* Mesh=Component?Component->GetStaticMesh().Get():nullptr;
    if(Mesh && Mesh->GetPathName()==EnvironmentMesh)EnvironmentCandidates.AddUnique(Component);
}
void UHearthwardPresentationComponent::EnvironmentActorSpawned(AActor* Actor)
{
    // Spawn returns before callers can assign mesh/tag; consider this static actor once in PostPhysics.
    if(Cast<AStaticMeshActor>(Actor))PendingEnvironmentActors.AddUnique(Actor);
}
void UHearthwardPresentationComponent::EnvironmentLevelAdded(ULevel* Level,UWorld* World)
{
    if(World==GetWorld() && Level)for(AActor* Actor:Level->Actors)CacheEnvironmentActor(Actor);
}
void UHearthwardPresentationComponent::EnvironmentLevelRemoved(ULevel* Level,UWorld* World)
{
    if(World!=GetWorld())return;
    if(!Level || (EnvironmentSurface.IsValid() && EnvironmentSurface->GetOwner()->GetLevel()==Level))StopEnvironment();
    EnvironmentCandidates.RemoveAll([Level](const auto& Weak){const auto* C=Weak.Get();return !C || !Level || C->GetOwner()->GetLevel()==Level;});
    PendingEnvironmentActors.RemoveAll([Level](const auto& Weak){const auto* A=Weak.Get();return !A || !Level || A->GetLevel()==Level;});
}
void UHearthwardPresentationComponent::StopEnvironment()
{
    if(IsValid(EnvironmentSource)){EnvironmentSource->Stop();EnvironmentSource->DestroyComponent();}
    EnvironmentSource=nullptr;EnvironmentSurface.Reset();
}
void UHearthwardPresentationComponent::ReleaseEnvironment()
{
    StopEnvironment();
    GetWorld()->RemoveOnActorSpawnedHandler(EnvironmentSpawnHandle);
    GetWorld()->RemoveOnPostRegisterAllActorComponentsHandler(EnvironmentRegisterHandle);
    FWorldDelegates::LevelAddedToWorld.RemoveAll(this);FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);
    EnvironmentSpawnHandle.Reset();EnvironmentRegisterHandle.Reset();EnvironmentCandidates.Reset();PendingEnvironmentActors.Reset();
    EnvironmentVertices.Reset();EnvironmentTriangles.Reset();EnvironmentWave=nullptr;EnvironmentWaveAttempted=false;
    EnvironmentMesh.Reset();EnvironmentTag=EnvironmentEvent=NAME_None;
}
void UHearthwardPresentationComponent::UpdateEnvironment()
{
    for(const auto& Actor:PendingEnvironmentActors)CacheEnvironmentActor(Actor.Get());PendingEnvironmentActors.Reset();
    EnvironmentCandidates.RemoveAll([](const auto& Weak){return !Weak.IsValid() || !IsValid(Weak->GetOwner()) || Weak->GetOwner()->IsActorBeingDestroyed();});
    const auto* Cue=SoundCues.Find(EnvironmentEvent);
    if(GetWorld()->IsPaused() || GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring()
        || !GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>()->Alive() || !Cue || Cue->Channel!=TEXT("environment"))
    {StopEnvironment();return;}
    UStaticMeshComponent* Best=nullptr;FVector Nearest=FVector::ZeroVector;double Distance=FMath::Square(3000.);
    const FVector Listener=GetOwner()->GetActorLocation();
    for(const auto& Weak:EnvironmentCandidates)
    {
        auto* Component=Weak.Get();auto* Actor=Component->GetOwner();const UStaticMesh* Mesh=Component->GetStaticMesh().Get();
        if(Component->GetWorld()!=GetWorld() || Actor->GetWorld()!=GetWorld() || !Component->IsRegistered()
            || !Actor->HasActorBegunPlay() || !Actor->ActorHasTag(EnvironmentTag) || Actor->IsHidden()
            || !Component->IsVisible() || Component->bHiddenInGame || !Mesh || Mesh->GetPathName()!=EnvironmentMesh)continue;
        const FTransform Transform=Component->GetComponentTransform();
        for(const auto& Triangle:EnvironmentTriangles)
        {
            const FVector Point=FMath::ClosestPointOnTriangleToPoint(Listener,Transform.TransformPosition(EnvironmentVertices[Triangle.X]),
                Transform.TransformPosition(EnvironmentVertices[Triangle.Y]),Transform.TransformPosition(EnvironmentVertices[Triangle.Z]));
            const double Current=FVector::DistSquared(Listener,Point);if(Current<Distance){Distance=Current;Nearest=Point;Best=Component;}
        }
    }
    if(!Best || !PrepareEnvironmentWave()){StopEnvironment();return;}
    if(!EnvironmentSource)
    {
        EnvironmentSource=NewObject<UAudioComponent>(GetOwner());EnvironmentSource->bAutoActivate=false;EnvironmentSource->bAutoDestroy=false;
        EnvironmentSource->bIsUISound=false;EnvironmentSource->bAllowSpatialization=true;EnvironmentSource->bStopWhenOwnerDestroyed=true;
        FSoundAttenuationSettings Attenuation;Attenuation.bAttenuate=true;Attenuation.bSpatialize=true;
        Attenuation.DistanceAlgorithm=EAttenuationDistanceModel::Linear;Attenuation.AttenuationShape=EAttenuationShape::Sphere;
        Attenuation.AttenuationShapeExtents=FVector::ZeroVector;Attenuation.FalloffDistance=3000.f;
        EnvironmentSource->bOverrideAttenuation=true;EnvironmentSource->SetAttenuationOverrides(Attenuation);
        EnvironmentSource->ComponentTags.Add(TEXT("Hearthward.Audio.environment"));EnvironmentSource->RegisterComponent();
        EnvironmentSource->SetSound(EnvironmentWave);EnvironmentSource->SetWorldLocation(Nearest);
        EnvironmentSource->SetVolumeMultiplier(GetWorld()->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Volume(TEXT("environment")));
        EnvironmentSource->Play();
    }
    EnvironmentSurface=Best;EnvironmentSource->SetWorldLocation(Nearest);
}
void UHearthwardPresentationComponent::UnbindCombatSources()
{
    // A replaced source can already be pending kill while its native delegate still exists.
    if(auto* Combat=BoundCombat.Get(true))Combat->OnCombatSucceeded.RemoveAll(this);
    if(auto* Survival=BoundSurvival.Get(true))Survival->OnDamageSucceeded.RemoveAll(this);
    if(auto* Survival=BoundBrotherSurvival.Get(true))Survival->OnDamageSucceeded.RemoveAll(this);
    BoundCombat.Reset();BoundSurvival.Reset();BoundBrotherSurvival.Reset();
}
void UHearthwardPresentationComponent::BindCombatSources(AHearthwardCompanionFixture* Brother)
{
    auto* Combat=GetOwner()->FindComponentByClass<UHearthwardCombatComponent>();
    auto* Survival=GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>();
    auto* BrotherSurvival=Brother?Brother->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    if(BoundCombat.Get()!=Combat)
    {
        if(auto* Previous=BoundCombat.Get(true))Previous->OnCombatSucceeded.RemoveAll(this);
        BoundCombat=Combat;if(Combat)Combat->OnCombatSucceeded.AddUObject(this,&UHearthwardPresentationComponent::CombatSucceeded);
    }
    if(BoundSurvival.Get()!=Survival)
    {
        if(auto* Previous=BoundSurvival.Get(true))Previous->OnDamageSucceeded.RemoveAll(this);
        BoundSurvival=Survival;if(Survival)Survival->OnDamageSucceeded.AddUObject(this,&UHearthwardPresentationComponent::CombatSucceeded);
    }
    if(BoundBrotherSurvival.Get()!=BrotherSurvival)
    {
        if(auto* Previous=BoundBrotherSurvival.Get(true))Previous->OnDamageSucceeded.RemoveAll(this);
        BoundBrotherSurvival=BrotherSurvival;if(BrotherSurvival)BrotherSurvival->OnDamageSucceeded.AddUObject(this,&UHearthwardPresentationComponent::CombatSucceeded);
    }
}
void UHearthwardPresentationComponent::CombatSucceeded(const FHearthwardCombatFeedbackReceipt& Receipt)
{
    if(!HasBegunPlay())return;
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())Restored();
    if(Receipt.Epoch!=Epoch || !Receipt.SuccessId.IsValid() || !Receipt.OperationId.IsValid()
        || ObservedCombatEvents.Contains(Receipt.SuccessId)
        || (Receipt.Kind!=TEXT("hit") && Receipt.Kind!=TEXT("block") && Receipt.Kind!=TEXT("damage") && Receipt.Kind!=TEXT("swing")))return;
    ObservedCombatEvents.Add(Receipt.SuccessId,Receipt.TargetPosition);
    if(GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring()
        || FVector::DistSquared(GetOwner()->GetActorLocation(),Receipt.TargetPosition)>FMath::Square(3000.f))return;
    PlaySoundEvent(FName(*(TEXT("combat.")+Receipt.Kind.ToString())),Receipt.SuccessId,&Receipt.TargetPosition);
}
bool UHearthwardPresentationComponent::FootContactSucceeded(const FHearthwardFootContactReceipt& Receipt)
{
    if(!HasBegunPlay() || Receipt.Frame!=GFrameCounter)return false;
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())Restored();
    if(ObservedFootFrame!=Receipt.Frame){ObservedFootContacts.Reset();ObservedFootFrame=Receipt.Frame;}
    auto* Source=Receipt.Source.Get();AHearthwardCompanionFixture* Brother=nullptr;
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It){Brother=*It;break;}
    if(!Source || (Source!=GetOwner() && Source!=Brother) || Receipt.Epoch!=Epoch || !Receipt.SuccessId.IsValid()
        || Receipt.FootBone.IsNone() || ObservedFootContacts.Contains(Receipt.SuccessId))return false;
    ObservedFootContacts.Add(Receipt.SuccessId,Receipt.Position);
    if(GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring()
        || FVector::DistSquared(GetOwner()->GetActorLocation(),Receipt.Position)>FMath::Square(3000.f))return false;
    return PlaySoundEvent(TEXT("movement.footstep"),Receipt.SuccessId,&Receipt.Position,false);
}
void UHearthwardPresentationComponent::StorageTransferred(FGuid Operation,bool ToCamp,FName Item,int32 Count)
{
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())Restored();
    if(!Operation.IsValid() || Count<=0 || ObservedTransfers.Contains(Operation))return;
    // Suppressed receipts are still observed so a later duplicate cannot replay paused or distant history.
    ObservedTransfers.Add(Operation);
    const auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    if(G->NearStorage())PlaySoundEvent(TEXT("storage.transfer"),Operation);
}
bool UHearthwardPresentationComponent::PlaySoundEvent(FName Event,FGuid Operation,const FVector* Position,bool Remember)
{
    if(!GetWorld()->AllowAudioPlayback())return false;
    const auto* Cue=SoundCues.Find(Event);
    if(!Cue || !Operation.IsValid() || (Remember && PlayedEvents.Contains(Operation)) || GetWorld()->IsPaused()
        || !GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>()->Alive())return false;
    TArray<uint8> Data;FWaveModInfo Info;const FString File=FPaths::ProjectDir()/TEXT("Resources/Audio")/Cue->File;
    if(!FFileHelper::LoadFileToArray(Data,*File) || !Info.ReadWaveInfo(Data.GetData(),Data.Num()) || *Info.pFormatTag!=1 || *Info.pBitsPerSample!=16)
    {UE_LOG(LogTemp,Warning,TEXT("Invalid PCM16 event sound: %s"),*File);return false;}
    auto* Sound=NewObject<USoundWaveProcedural>(this);Sound->NumChannels=*Info.pChannels;Sound->SetSampleRate(*Info.pSamplesPerSec);
    Sound->Duration=Info.SampleDataSize/float(*Info.pSamplesPerSec*Sound->NumChannels*2);Sound->bLooping=false;
    Sound->QueueAudio(Info.SampleDataStart,Info.SampleDataSize);
    auto* Effect=NewObject<UAudioComponent>(GetOwner());Effect->bAutoActivate=false;Effect->bAutoDestroy=false;
    Effect->bIsUISound=false;Effect->bAllowSpatialization=Position!=nullptr;
    if(Position)
    {
        FSoundAttenuationSettings Attenuation;Attenuation.bAttenuate=true;Attenuation.bSpatialize=true;
        Attenuation.DistanceAlgorithm=EAttenuationDistanceModel::Linear;Attenuation.AttenuationShape=EAttenuationShape::Sphere;
        Attenuation.AttenuationShapeExtents=FVector::ZeroVector;Attenuation.FalloffDistance=3000.f;
        Effect->bOverrideAttenuation=true;Effect->SetAttenuationOverrides(Attenuation);
    }
    Effect->ComponentTags.Add(FName(*(FString(TEXT("Hearthward.Audio."))+Cue->Channel.ToString())));Effect->RegisterComponent();
    if(Position)Effect->SetWorldLocation(*Position);
    Effect->SetSound(Sound);Effect->SetVolumeMultiplier(GetWorld()->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Volume(Cue->Channel));
    Effects.Add(Effect);EffectRemaining.Add(Sound->Duration);if(Remember)PlayedEvents.Add(Operation);Effect->Play();return true;
}
void UHearthwardPresentationComponent::StopFixedCue()
{if(Voice) Voice->Stop();CurrentCue=NAME_None;Remaining=0;}
bool UHearthwardPresentationComponent::PlayFixedCue(FName Id,FGuid Event,bool SharedKnowledge)
{
    const FCue* Cue=Cues.Find(Id);if(!Cue || !Event.IsValid() || PlayedEvents.Contains(Event) || !SharedKnowledge || Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()) return false;
    bool Near=false;
    if(Cue->Speaker==TEXT("弟弟"))
    {
        for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It) if(It->CanCommunicate(GetOwner())) {Near=true;break;}
    }
    else
    {
        TArray<FString> Parts;Id.ToString().ParseIntoArray(Parts,TEXT("."));
        const auto Quest=Parts.Num()==3?HearthwardCampaign::Find(TEXT("quests"),FName(*Parts[1])):nullptr;
        if(Quest) for(TActorIterator<AHearthwardCampaignActor> It(GetWorld());It;++It)
            if(FVector::Dist(It->GetActorLocation(),GetOwner()->GetActorLocation())<=3000
                && Quest->GetArrayField(TEXT("rescued_people")).ContainsByPredicate([&](const auto& V){return It->Identity==FName(*V->AsString());})) {Near=true;break;}
    }
    if(!Near || !GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>()->Alive()) return false;
    StopFixedCue();PlayedEvents.Add(Event);CurrentCue=Id;Remaining=FMath::Clamp(Cue->Text.Len()*.16f,3.f,12.f);
    TArray<uint8> Data;FWaveModInfo Info;
    const FString File=FPaths::ProjectDir()/TEXT("Resources/Audio/Fixed")/(Cue->Group+TEXT(".wav"));
    if(Cue->ProductionStatus==TEXT("UNPRODUCED") && !FPaths::FileExists(File)) {VoiceStatus=TEXT("UNPRODUCED");return true;}
    if(!FFileHelper::LoadFileToArray(Data,*File)) {VoiceStatus=TEXT("UNPRODUCED");return true;}
    if(!Info.ReadWaveInfo(Data.GetData(),Data.Num()) || *Info.pFormatTag!=1 || *Info.pBitsPerSample!=16)
    {VoiceStatus=TEXT("INVALID_PCM16");return true;}
    Wave=NewObject<USoundWaveProcedural>(this);Wave->NumChannels=*Info.pChannels;Wave->SetSampleRate(*Info.pSamplesPerSec);
    Wave->Duration=Info.SampleDataSize/float(*Info.pSamplesPerSec*Wave->NumChannels*2);Wave->bLooping=false;
    Wave->QueueAudio(Info.SampleDataStart,Info.SampleDataSize);Remaining=FMath::Max(3.f,Wave->Duration);
    if(!Voice) {Voice=NewObject<UAudioComponent>(GetOwner());Voice->bAutoActivate=false;Voice->bIsUISound=false;Voice->bAllowSpatialization=false;Voice->ComponentTags.Add(TEXT("Hearthward.Audio.voice"));Voice->RegisterComponent();}
    Voice->SetSound(Wave);Voice->SetVolumeMultiplier(GetWorld()->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Volume(TEXT("voice")));Voice->Play();VoiceStatus=TEXT("RECORDED");return true;
}
FString UHearthwardPresentationComponent::GetSubtitle() const
{
    const auto& Comfort=GetWorld()->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort;
    const FCue* Cue=Cues.Find(CurrentCue);return !Comfort.Subtitles || !Cue?FString():(Comfort.SubtitleSpeaker?Cue->Speaker+TEXT("："):FString())+Cue->Text;
}
void UHearthwardPresentationComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Delta,Type,Tick);
    if(!HasBegunPlay())return;
    if(ObservedFootFrame!=GFrameCounter){ObservedFootContacts.Reset();ObservedFootFrame=GFrameCounter;}
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()) Restored();
    AHearthwardCompanionFixture* Brother=nullptr;for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It) {Brother=*It;break;}
    BindCombatSources(Brother);
    // Reward notifications can precede final rescue/quest settlement; observe committed state here.
    ObserveProgressSuccess(false);
    ObserveNPCSuccess(Brother);
    if(PendingLanding.IsValid())
    {const FGuid Landing=PendingLanding;PendingLanding.Invalidate();PlaySoundEvent(TEXT("movement.landed"),Landing);}
    UpdateEnvironment();
    if(GetWorld()->IsPaused()) return;
    auto* Settings=GetWorld()->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
    const auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    // Procedural waves pad an exhausted queue with silence instead of ending naturally.
    for(int32 I=Effects.Num()-1;I>=0;--I)if((EffectRemaining[I]-=Delta)<=0)
    {if(IsValid(Effects[I])){Effects[I]->Stop();Effects[I]->DestroyComponent();}Effects.RemoveAtSwap(I);EffectRemaining.RemoveAtSwap(I);}
    if(G->Health<=0) StopFixedCue();
    if(Remaining>0 && (Remaining-=Delta)<=0) StopFixedCue();
    auto* Player=Cast<APlayerController>(CastChecked<APawn>(GetOwner())->GetController());const auto* HUD=Player?Cast<AHearthwardHUD>(Player->GetHUD()):nullptr;
    const bool WorldView=HUD && HUD->Screen && HUD->Screen->GetPage()==TEXT("hud") && Player->GetViewTarget()==GetOwner();
    if(G->Health<PreviousHealth && WorldView) ShakeRemaining=.3f;
    PreviousHealth=G->Health;
    if(Brother)
    {
        const auto Phase=Brother->GetPhase();const auto Command=Brother->GetCommandId();
        if(ObservedBrother.Get()==Brother && Brother->CanCommunicate(GetOwner()))
        {
            using P=EHearthwardCompanionPhase;
            if(Command.IsValid() && Command!=PreviousCommand && Phase!=P::Idle && Phase!=P::Cancelled && Phase!=P::Completed) PlayFixedCue(TEXT("fixed.task.accepted"),FGuid::NewGuid(),true);
            if(uint8(Phase)!=PreviousPhase)
            {
                if(Phase==P::ReturningBlocked || Phase==P::HoldingSafely) PlayFixedCue(TEXT("fixed.task.blocked"),FGuid::NewGuid(),true);
                if(Phase==P::Cancelled) PlayFixedCue(TEXT("fixed.task.cancelled"),FGuid::NewGuid(),true);
                if(Phase==P::Completed) PlayFixedCue(TEXT("fixed.task.finished"),FGuid::NewGuid(),true);
            }
            if(Brother->GetDelivered()>PreviousDelivered && Command==PreviousCommand && Brother->GetGoal().Intent==TEXT("collect")) PlayFixedCue(TEXT("fixed.task.delivered"),FGuid::NewGuid(),true);
        }
        ObservedBrother=Brother;PreviousCommand=Command;PreviousPhase=uint8(Phase);PreviousDelivered=Brother->GetDelivered();
    }
    const FName Initiative=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->GetInitiativeKind();
    if(Initiative==TEXT("conversation_reminder") && PreviousInitiative!=Initiative) PlayFixedCue(TEXT("fixed.camp.three_days"),FGuid::NewGuid(),true);
    PreviousInitiative=Initiative;
    if(auto* Camera=GetOwner()->FindComponentByClass<UCameraComponent>())
    {
        const float Amount=WorldView?Settings->Comfort.Shake/100.f:0;
        ShakeRemaining=FMath::Max(0.f,ShakeRemaining-Delta);
        Camera->SetRelativeRotation(ShakeRemaining>0?FRotator(FMath::Sin(ShakeRemaining*90)*Amount*.6f,FMath::Sin(ShakeRemaining*73)*Amount*.4f,0):FRotator::ZeroRotator);
    }
    for(TObjectIterator<UAudioComponent> It;It;++It) if(It->GetWorld()==GetWorld())
    {
        FName Channel=TEXT("effects");for(const TCHAR* C:{TEXT("music"),TEXT("voice"),TEXT("environment")}) if(It->ComponentTags.Contains(FName(*(FString(TEXT("Hearthward.Audio."))+C)))) Channel=FName(C);
        It->SetVolumeMultiplier(Settings->Volume(Channel));
    }
}
