#include "HearthwardNPCInitiativeQueue.h"

#include "HearthwardAgentContract.h"
#include "../Companion/HearthwardCompanionFixture.h"

void FHearthwardNPCInitiativeQueue::Reset()
{
    Pending.Reset();
    History.Reset();
    Active={};
    ActiveSpeaker.Reset();
    ActiveCompanion.Reset();
    VisibleUntil=0;
    CooldownUntil=0;
}

void FHearthwardNPCInitiativeQueue::DismissActive()
{
    Active={};
    ActiveSpeaker.Reset();
    ActiveCompanion.Reset();
    VisibleUntil=0;
}

void FHearthwardNPCInitiativeQueue::Enqueue(const FHearthwardNPCInitiative& Initiative)
{
    if(!Initiative.IsValid() || History.Contains(Initiative.DedupeKey)
        || Pending.ContainsByPredicate([&](const auto& X){return X.DedupeKey==Initiative.DedupeKey;})
        || (Active.IsValid() && Active.DedupeKey==Initiative.DedupeKey))
        return;

    History.Add(Initiative.DedupeKey);
    if(History.Num()>64)History.RemoveAt(0);

    const int32 MaxQueue=HearthwardAgent::Policy(TEXT("initiative_queue_max"));
    if(Pending.Num()>=MaxQueue)Pending.RemoveAt(0);
    Pending.Add(Initiative);
}

void FHearthwardNPCInitiativeQueue::Tick(double Now,bool bInteractionBlocked,bool bPaused,AActor* Speaker,
    AHearthwardCompanionFixture* Companion)
{
    if(Active.IsValid() && Now>=VisibleUntil)DismissActive();
    if(Active.IsValid() || Pending.IsEmpty() || bInteractionBlocked || bPaused || Now<CooldownUntil)
        return;
    if(!IsValid(Speaker) || !IsValid(Companion) || !Companion->CanCommunicate(Speaker))
        return;

    Active=Pending[0];
    Pending.RemoveAt(0);
    ActiveSpeaker=Speaker;
    ActiveCompanion=Companion;
    VisibleUntil=Now+Active.VisibleSeconds;
    CooldownUntil=Now+HearthwardAgent::Policy(TEXT("initiative_cooldown_seconds"));
}

bool FHearthwardNPCInitiativeQueue::CanDisplay() const
{
    return Active.IsValid() && ActiveCompanion.IsValid() && ActiveCompanion->CanCommunicate(ActiveSpeaker.Get());
}
