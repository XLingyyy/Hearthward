#include "HearthwardNoiseInteraction.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "HearthwardWeatherActor.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

float UHearthwardNoiseInteraction::RemainingCooldown() const
{
    if(Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())return 0;
    return FMath::Max(0.,AvailableAt-GetWorld()->GetTimeSeconds());
}
FString UHearthwardNoiseInteraction::GetInteractionPrompt(AActor* Interactor) const
{
    return RemainingCooldown()>0?FString::Printf(TEXT("金属悬片 · %.0f秒后可再敲击"),FMath::CeilToFloat(RemainingCooldown())):TEXT("E 敲击金属悬片 · 引开附近守军");
}
FString UHearthwardNoiseInteraction::CompleteInteraction(AActor* Interactor)
{
    if(!IsValid(Interactor) || GetWorld()->IsPaused() || Interactor->GetWorld()!=GetWorld()
        || FVector::Dist(Interactor->GetActorLocation(),GetComponentLocation())>MaxDistance || RemainingCooldown()>0)return TEXT("声源暂不可用");
    auto* S=Interactor->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!S || !S->Alive())return TEXT("当前无法敲击");
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Round2NoiseUse),false,Interactor);Query.AddIgnoredActor(GetOwner());
    if(GetWorld()->LineTraceTestByChannel(Interactor->GetActorLocation(),GetComponentLocation(),ECC_Visibility,Query))return TEXT("声源被遮挡");
    Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();AvailableAt=GetWorld()->GetTimeSeconds()+10;
    TArray<AHearthwardCampaignActor*> Listeners;
    const double Radius=3000*(AHearthwardWeatherActor::Sheltered(GetWorld(),GetComponentLocation(),GetOwner())?1.:1.-.15*AHearthwardWeatherActor::Rain(GetWorld()));
    for(TActorIterator<AHearthwardCampaignActor> It(GetWorld());It;++It)
        if(It->CanInvestigateNoise() && FVector::DistSquared(It->GetActorLocation(),GetComponentLocation())<=FMath::Square(Radius))Listeners.Add(*It);
    Listeners.Sort([&](const auto& A,const auto& B)
    {
        const auto DA=FVector::DistSquared(A.GetActorLocation(),GetComponentLocation()),DB=FVector::DistSquared(B.GetActorLocation(),GetComponentLocation());
        return DA==DB?A.Identity.LexicalLess(B.Identity):DA<DB;
    });
    int32 Count=0;
    for(auto* Listener:Listeners)if(Listener->BeginNoiseInvestigation(GetComponentLocation(),Epoch) && ++Count==2)break;
#if !UE_BUILD_SHIPPING
    CSV_CUSTOM_STAT_GLOBAL(HearthwardNoiseEvents,1,ECsvCustomStatOp::Accumulate);
    CSV_CUSTOM_STAT_GLOBAL(HearthwardNoiseListeners,Count,ECsvCustomStatOp::Accumulate);
#endif
    if(auto* Audio=Interactor->FindComponentByClass<UHearthwardPresentationComponent>())Audio->InteractionSucceeded(TEXT("interaction.noise"),FGuid::NewGuid(),GetComponentLocation());
    OnInteractionReady.Broadcast(Interactor);
    return TEXT("金属声传出 · 敌人只会检查声源，注意避开视线");
}
