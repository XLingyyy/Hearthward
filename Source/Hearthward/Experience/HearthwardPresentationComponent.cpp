#include "HearthwardPresentationComponent.h"
#include "HearthwardPlayerSettings.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../UI/HearthwardHUD.h"
#include "../UI/HearthwardScreenWidget.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Sound/SoundWaveProcedural.h"
#include "Audio.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
UHearthwardPresentationComponent::UHearthwardPresentationComponent()
{PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostPhysics;}
void UHearthwardPresentationComponent::BeginPlay()
{
    Super::BeginPlay();
    FString Text;TSharedPtr<FJsonObject> Root;
    if(FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("Resources/Data/experience.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root))
        for(const auto& Value:Root->GetArrayField(TEXT("fixed_dialogue")))
        {const auto Row=Value->AsObject();Cues.Add(FName(*Row->GetStringField(TEXT("cue_id"))),{Row->GetStringField(TEXT("speaker")),Row->GetStringField(TEXT("text")),Row->GetStringField(TEXT("audio_group"))});}
    GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.AddDynamic(this,&UHearthwardPresentationComponent::Restored);
    Restored();
}
void UHearthwardPresentationComponent::EndPlay(const EEndPlayReason::Type Reason)
{StopFixedCue();GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.RemoveDynamic(this,&UHearthwardPresentationComponent::Restored);Super::EndPlay(Reason);}
void UHearthwardPresentationComponent::Restored()
{StopFixedCue();PlayedEvents.Reset();ObservedBrother.Reset();PreviousInitiative=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->GetInitiativeKind();Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();PreviousHealth=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->Health;ShakeRemaining=0;}
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
    if(GetWorld()->IsPaused()) return;
    auto* Settings=GetWorld()->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
    const auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()) Restored();
    if(G->Health<=0) StopFixedCue();
    if(Remaining>0 && (Remaining-=Delta)<=0) StopFixedCue();
    auto* Player=Cast<APlayerController>(CastChecked<APawn>(GetOwner())->GetController());const auto* HUD=Player?Cast<AHearthwardHUD>(Player->GetHUD()):nullptr;
    const bool WorldView=HUD && HUD->Screen && HUD->Screen->GetPage()==TEXT("hud") && Player->GetViewTarget()==GetOwner();
    if(G->Health<PreviousHealth && WorldView) ShakeRemaining=.3f;
    PreviousHealth=G->Health;
    AHearthwardCompanionFixture* Brother=nullptr;for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It) {Brother=*It;break;}
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
