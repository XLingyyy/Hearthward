#include "HearthwardPresentationComponent.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Engine/World.h"

void UHearthwardPresentationComponent::ObserveStoryCues(float Delta)
{
    StoryCooldown=FMath::Max(0.f,StoryCooldown-Delta);
    if(StoryCooldown>0 || Remaining>0)return;
    auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
    const auto* Gameplay=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    const auto* Survival=GetOwner()->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!Campaign->Active() || Campaign->Busy() || !Survival || !Survival->Alive() || !Gameplay || !Gameplay->CanChangeSkills())return;
    const auto& Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    const FName Here=Camp.CampAt(GetOwner()->GetActorLocation());
    const bool Home=Camp.Camps.ContainsByPredicate([](const auto& Site){return Site.Id==TEXT("hometown");});
    const auto Try=[&](const TCHAR* Id,bool Ready)
    {
        const FName Cue(Id),Fact(*(TEXT("cue:")+Cue.ToString()));
        if(!Ready || Campaign->State.Facts.Contains(Fact))return false;
        if(!PlayFixedCue(Cue,FGuid::NewGuid(),true))return false;
        Campaign->Record(Fact);StoryCooldown=20.f;return true;
    };
    if(Try(TEXT("fixed.story.home_rest"),Home && Here==TEXT("hometown") && Campaign->State.Facts.Contains(TEXT("story:home_rested"))))return;
    if(Try(TEXT("fixed.story.victory"),Home && Here==TEXT("hometown")))return;
    if(Try(TEXT("fixed.story.rescue"),!Camp.Rescued.IsEmpty() && !Here.IsNone()))return;
    if(Try(TEXT("fixed.story.tier"),Camp.Tier>=2 && !Here.IsNone()))return;
    Try(TEXT("fixed.story.home_approach"),!Home && Gameplay && Gameplay->Discovered.Contains(TEXT("home_entry"))
        && FVector::Dist2D(GetOwner()->GetActorLocation(),Campaign->Position(TEXT("home_entry")))<5000);
}
