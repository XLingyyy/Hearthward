#include "HearthwardWorldClockSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

bool UHearthwardWorldClockSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UHearthwardWorldClockSubsystem::IsTickable() const
{
    return IsInitialized() && GetWorld()->HasBegunPlay();
}

void UHearthwardWorldClockSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if(!GetWorld()->IsPaused()) AdvanceSurvival(DeltaTime,DeltaTime);
}

TStatId UHearthwardWorldClockSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UHearthwardWorldClockSubsystem, STATGROUP_Tickables);
}

FHearthwardClockSnapshot UHearthwardWorldClockSubsystem::GetSnapshot() const
{
    FHearthwardClockSnapshot Result;
    Result.ActivePlaySeconds = Clock.GetActivePlaySeconds();
    Result.ElapsedCalendarMinutes = Clock.GetElapsedCalendarMinutes();
    Result.ElapsedDays = Clock.GetElapsedDays();
    Result.MinuteOfDay = Clock.GetMinuteOfDay();
    return Result;
}

double UHearthwardWorldClockSubsystem::AdvanceSurvival(double Active,double Calendar)
{
    if(UHearthwardSurvivalComponent::HasFailed(GetWorld())) return 0;
    TArray<UHearthwardSurvivalComponent*> Participants;
    double Fraction=1;
    for(TActorIterator<AActor> It(GetWorld());It;++It)
        if(auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>();S && S->Enabled())
        {
            Participants.Add(S);
            if(Active==0)
            {
                auto Preview=S->State; float Health=S->Health(),Hunger=S->Hunger();
                Fraction=FMath::Min(Fraction,Preview.Advance(Health,Hunger,S->MaxHealth(),0,Calendar,Clock.CalendarMinutes));
            }
        }
    for(auto* S:Participants) S->AdvanceContinuous(Active*Fraction,Calendar*Fraction,Clock.CalendarMinutes);
    GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->Advance(Calendar*Fraction,Active==0);
    Clock.ActivePlaySeconds+=Active*Fraction; Clock.CalendarMinutes+=Calendar*Fraction;
    if(!UHearthwardSurvivalComponent::HasFailed(GetWorld()))
        for(auto* S:Participants) S->CompleteBoundary(Active*Fraction);
    return Calendar*Fraction;
}
double UHearthwardWorldClockSubsystem::AdvanceCalendar(double Minutes)
{
    if(!FMath::IsFinite(Minutes) || Minutes<=0 || GetWorld()->IsPaused()) return 0;
    return AdvanceSurvival(0,Minutes);
}
