#include "HearthwardAnimalDemo.h"
#include "HearthwardAnimalMotionComponent.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Misc/EngineVersion.h"
#include "UnrealClient.h"

namespace
{
void Place(APawn* P,FVector At)
{
    if(!P)return;
    if(auto* C=Cast<ACharacter>(P))C->GetCharacterMovement()->StopMovementImmediately();
    P->SetActorLocation(At,false,nullptr,ETeleportType::TeleportPhysics);
}
}
void AHearthwardAnimalDemoGameMode::Check(FString Name,bool Passed,FString Detail)
{
    auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("pass"),Passed);C->SetStringField(TEXT("detail"),Detail);
    Checks.Add(MakeShared<FJsonValueObject>(C));UE_LOG(LogTemp,Display,TEXT("ANIMAL_CHECK %s %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),*Name,*Detail);
}
void AHearthwardAnimalDemoGameMode::WriteVerification()
{
    auto Root=MakeShared<FJsonObject>();Root->SetArrayField(TEXT("checks"),Checks);Root->SetStringField(TEXT("map"),GetWorld()->GetMapName());Root->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
    Root->SetNumberField(TEXT("elapsed_seconds"),VerifyTime);Root->SetNumberField(TEXT("animal_count"),Animals.Num());Root->SetStringField(TEXT("execution"),TEXT("Standalone -game, real rendered world; scripted threat/health probes, not manual keyboard acceptance."));
    Root->SetNumberField(TEXT("measured_player_sprint_cm_s"),VerifyPlayerSprintSpeed);
    Root->SetNumberField(TEXT("measured_player_skilled_sprint_cm_s"),VerifyBoostedSprintSpeed);
    bool Passed=true;for(const auto& C:Checks)Passed&=C->AsObject()->GetBoolField(TEXT("pass"));Root->SetBoolField(TEXT("pass"),Passed);
    TArray<TSharedPtr<FJsonValue>> Rows;
    for(auto A:Animals)
    {
        auto R=MakeShared<FJsonObject>();auto* M=A->Motion.Get();R->SetStringField(TEXT("species"),M->Species.ToString());R->SetNumberField(TEXT("natural_cycles"),M->NaturalCycles);
        R->SetNumberField(TEXT("boundary_corrections"),M->BoundaryCorrections);R->SetBoolField(TEXT("pose_changed"),PoseChanged.Contains(M->Species));
        TArray<TSharedPtr<FJsonValue>> Clips;for(FName C:SeenClips.FindRef(M->Species))Clips.Add(MakeShared<FJsonValueString>(C.ToString()));R->SetArrayField(TEXT("observed_clips"),Clips);Rows.Add(MakeShared<FJsonValueObject>(R));
    }
    Root->SetArrayField(TEXT("animals"),Rows);FString Text;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Text));
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("AnimalDemo");IFileManager::Get().MakeDirectory(*Dir,true);FFileHelper::SaveStringToFile(Text,*(Dir/TEXT("verification.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("ANIMAL_DEMO_VERIFICATION_COMPLETE pass=%d checks=%d"),Passed,Checks.Num());
}
void AHearthwardAnimalDemoGameMode::TickVerification(float Delta)
{
    VerifyTime+=Delta;SampleAt-=Delta;
    if(SampleAt<=0 && VerifyTime>3)
    {
        SampleAt=.15;
        for(auto A:Animals)
        {
            auto* M=A->Motion.Get();SeenClips.FindOrAdd(M->Species).Add(M->ActiveClip);
            const FBox Box=M->Mesh->Bounds.GetBox(),Region=M->Habitat();
            const bool XY=Box.Min.X>=Region.Min.X-1 && Box.Max.X<=Region.Max.X+1 && Box.Min.Y>=Region.Min.Y-1 && Box.Max.Y<=Region.Max.Y+1;
            const bool Z=!M->Aquatic || (Box.Min.Z>=Region.Min.Z-4 && Box.Max.Z<=Region.Max.Z+4);
            if(!XY || !Z)BoundsFailed.Add(M->Species);
            // Component-relative bone transforms prove actual animation, independent of actor travel.
            const int32 Bone=FMath::Min(8,M->Mesh->GetNumBones()-1);
            const FTransform T=M->Mesh->GetBoneTransform(Bone).GetRelativeTransform(M->Mesh->GetComponentTransform());
            if(!FirstBone.Contains(M->Species))FirstBone.Add(M->Species,T);
            else if(!FirstBone[M->Species].Equals(T,.01f))PoseChanged.Add(M->Species);
        }
    }
    auto* P=UGameplayStatics::GetPlayerPawn(this,0);
    auto* Combat=P?P->FindComponentByClass<UHearthwardCombatComponent>():nullptr;
    auto* Gameplay=P?P->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    auto Remote=[&](){Place(P,FVector(-4200,-3200,100));Place(Brother,FVector(-4500,-3200,100));BrotherWalking=false;};
    auto BoostSprint=[&](){if(Gameplay){Gameplay->Skills=VerifyOriginalSkills;Gameplay->Skills.Add(TEXT("runner"),3);Gameplay->Skills.Add(TEXT("longstride"),3);}};
    // Measure the normal CharacterMovement sprint path on the approach walkway,
    // including acceleration and the real gameplay stamina / skill calculations.
    for(int32 Probe=0;Probe<2 && VerifyStage<0;++Probe)
    {
        const float Begin=Probe==0?5.f:8.f,End=Begin+2.f;
        if(VerifyTime>=Begin && VerifyTime-Delta<Begin && Gameplay)
        {
            if(Probe==0)VerifyOriginalSkills=Gameplay->Skills;else BoostSprint();
            Place(P,FVector(-2000,-1850,100));Gameplay->Stamina=Gameplay->MaxStamina();Gameplay->SetSprinting(true);
        }
        if(VerifyTime>=Begin && VerifyTime<End && P)
        {
            P->AddMovementInput(FVector::ForwardVector,1,true);
            float& Speed=Probe==0?VerifyPlayerSprintSpeed:VerifyBoostedSprintSpeed;
            Speed=FMath::Max(Speed,P->GetVelocity().Size2D());
        }
        if(VerifyTime>=End && VerifyTime-Delta<End && Gameplay)
        {
            const float Expected=HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("tuning")),TEXT("sprintSpeed"))*(1.f+Gameplay->Effect(TEXT("sprint")));
            const float Measured=Probe==0?VerifyPlayerSprintSpeed:VerifyBoostedSprintSpeed;
            Check(Probe==0?TEXT("Actual player full sprint speed"):TEXT("Actual player full sprint with both sprint skills"),FMath::Abs(Measured-Expected)<2,
                FString::Printf(TEXT("measured=%.2f expected=%.2f cm/s"),Measured,Expected));
            Gameplay->SetSprinting(false);Gameplay->Skills=VerifyOriginalSkills;Remote();
        }
    }
    auto Screenshot=[&](const FString& Suffix)
    {const FString Dir=FPaths::ProjectSavedDir()/TEXT("AnimalDemo/screenshots");IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/(Selected()->Motion->Species.ToString()+Suffix+TEXT(".png")),true,false);};
    static float StageAge=0,MaxSpeed=0,PeakPlayRate=0,PeakReferenceSpeed=0;static bool FleeShot=false;StageAge+=Delta;
    if(VerifyStage<0 && VerifyTime>=60 && VerifyTime<88)
    {
        const int32 Index=FMath::Min(13,int32((VerifyTime-60)/2));
        if(SelectedIndex!=Index)
        {
            SelectedIndex=Index;Observing=true;
            if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->SetViewTarget(ObservationCamera);
        }
        if(VerifyTime>=61+Index*2 && VerifyTime-Delta<61+Index*2)Screenshot(TEXT("_natural"));
    }
    if(VerifyStage<0 && VerifyTime>=100)
    {
        Check(TEXT("Exactly fourteen runtime animals"),Animals.Num()==14);
        for(auto A:Animals)
        {
            const auto* M=A->Motion.Get();
            Check(M->Species.ToString()+TEXT(" natural movement/activity alternation"),M->NaturalCycles>=2 && SeenClips[M->Species].Num()>=3);
            Check(M->Species.ToString()+TEXT(" animated skeletal pose"),PoseChanged.Contains(M->Species));
        }
        VerifySpecies=0;VerifyStage=0;StageAge=0;
    }
    if(VerifyStage<0)return;
    SelectedIndex=VerifySpecies;auto* A=Selected();if(!A)return;
    if(!Observing){Observing=true;if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->SetViewTarget(ObservationCamera);}
    if(A->Motion->GroundSpeed>MaxSpeed)
    {MaxSpeed=A->Motion->GroundSpeed;PeakPlayRate=A->Motion->PlayRate;PeakReferenceSpeed=A->Motion->GaitReferenceSpeed();}
    const FString Name=A->Motion->Species.ToString();
    auto ThreatPoint=[&](){return A->GetActorLocation()+FVector(-A->Motion->DetectionRadius()*.15,0,100-A->GetActorLocation().Z);};
    // Engine screenshots render later in this frame: capture while still fleeing,
    // before the next stage teleports the animal home and changes the camera target.
    if(VerifyStage==1 && !FleeShot && StageAge>.6 && A->Motion->Loops() && A->Motion->GroundSpeed>A->Motion->EscapeSpeed()*.9f)
    {Screenshot(TEXT("_flee"));FleeShot=true;}
    if(VerifyStage==0)
    {if(Gameplay)Gameplay->Skills=VerifyOriginalSkills;Remote();A->Motion->ResetAnimal();Place(P,ThreatPoint());VerifyStage=1;StageAge=0;MaxSpeed=0;PeakPlayRate=PeakReferenceSpeed=0;FleeShot=false;}
    else if(VerifyStage==1 && StageAge>3.8)
    {
        Check(Name+TEXT(" flees player"),A->Motion->LastThreat==TEXT("player") && MaxSpeed>A->Motion->DetectionRadius()*.01 && FleeShot);
        Check(Name+TEXT(" escape is five percent faster than full player sprint"),VerifyPlayerSprintSpeed>0 && MaxSpeed>VerifyPlayerSprintSpeed*1.04f && MaxSpeed<VerifyPlayerSprintSpeed*1.06f,
            FString::Printf(TEXT("animal=%.2f player=%.2f cm/s"),MaxSpeed,VerifyPlayerSprintSpeed));
        Check(Name+TEXT(" fast gait playback matches actual travel speed"),PeakReferenceSpeed>0 && FMath::Abs(PeakPlayRate*PeakReferenceSpeed-MaxSpeed)<2,
            FString::Printf(TEXT("play_rate=%.3f reference=%.2f travel=%.2f"),PeakPlayRate,PeakReferenceSpeed,MaxSpeed));
        Remote();A->Motion->ResetAnimal();BoostSprint();Place(Brother,ThreatPoint());VerifyStage=2;StageAge=0;MaxSpeed=0;PeakPlayRate=PeakReferenceSpeed=0;
    }
    else if(VerifyStage==2 && StageAge>3.8)
    {
        Check(Name+TEXT(" flees brother"),A->Motion->LastThreat==TEXT("brother") && MaxSpeed>5);
        Check(Name+TEXT(" skilled sprint keeps the small escape advantage"),VerifyBoostedSprintSpeed>0 && MaxSpeed>VerifyBoostedSprintSpeed*1.04f && MaxSpeed<VerifyBoostedSprintSpeed*1.06f,
            FString::Printf(TEXT("animal=%.2f skilled_player=%.2f cm/s"),MaxSpeed,VerifyBoostedSprintSpeed));
        Check(Name+TEXT(" skilled fast gait playback matches travel speed"),PeakReferenceSpeed>0 && FMath::Abs(PeakPlayRate*PeakReferenceSpeed-MaxSpeed)<2,
            FString::Printf(TEXT("play_rate=%.3f reference=%.2f travel=%.2f"),PeakPlayRate,PeakReferenceSpeed,MaxSpeed));
        if(Gameplay)Gameplay->Skills=VerifyOriginalSkills;
        Remote();A->Motion->ResetAnimal();
        if(Combat)Combat->HitTarget(A->Combat,5,TEXT("body"),false,FGuid::NewGuid(),P);
        VerifyStage=3;StageAge=0;
    }
    else if(VerifyStage==3 && StageAge>1.1)
    {
        Check(Name+TEXT(" nonfatal hit reacts without dying"),A->Combat->Health>0 && A->Combat->Health<100 && !A->Motion->Dead);
        if(Combat)Combat->HitTarget(A->Combat,10000,TEXT("body"),false,FGuid::NewGuid(),P);
        DeadAt=A->GetActorLocation();VerifyStage=4;StageAge=0;
    }
    else if(VerifyStage==4 && StageAge>5)
    {
        Check(Name+TEXT(" lethal transition and corpse hold"),A->Combat->Health==0 && A->Motion->Dead && A->Motion->Behavior==TEXT("死亡保持") && (A->Motion->ActiveClip==TEXT("DisplayStill") || A->Motion->ActiveClip==TEXT("CorpseHold_L")));
        Check(Name+TEXT(" death stops horizontal movement"),FVector::Dist2D(DeadAt,A->GetActorLocation())<.1 && A->Motion->GroundSpeed==0);
        Check(Name+TEXT(" corpse keeps actor upright"),FMath::Abs(A->GetActorRotation().Roll)<.01 && FMath::Abs(A->GetActorRotation().Pitch)<.01);
        Screenshot(TEXT("_dead"));VerifyStage=5;StageAge=0;
    }
    else if(VerifyStage==5 && StageAge>.6)
    {
        A->Motion->ResetAnimal();Check(Name+TEXT(" reset restores live idle"),!A->Motion->Dead && A->Combat->Health==100);
        ++VerifySpecies;VerifyStage=0;StageAge=0;
        if(VerifySpecies==Animals.Num())
        {
            for(auto Animal:Animals)Check(Animal->Motion->Species.ToString()+TEXT(" whole mesh remains inside habitat"),!BoundsFailed.Contains(Animal->Motion->Species));
            WriteVerification();Verify=false;if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->ConsoleCommand(TEXT("quit"));
        }
    }
}
