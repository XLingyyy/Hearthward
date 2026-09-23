#pragma once

#include "CoreMinimal.h"
#include "HearthwardNPCInitiative.h"

class AHearthwardCompanionFixture;

class FHearthwardNPCInitiativeQueue
{
public:
    void Reset();
    void DismissActive();
    void Enqueue(const FHearthwardNPCInitiative& Initiative);
    void Tick(double Now,bool bInteractionBlocked,bool bPaused,AActor* Speaker,AHearthwardCompanionFixture* Companion);

    bool HasActive() const { return Active.IsValid(); }
    const FHearthwardNPCInitiative& GetActive() const { return Active; }
    bool CanDisplay() const;

private:
    TArray<FHearthwardNPCInitiative> Pending;
    TArray<FString> History;
    FHearthwardNPCInitiative Active;
    TWeakObjectPtr<AActor> ActiveSpeaker;
    TWeakObjectPtr<AHearthwardCompanionFixture> ActiveCompanion;
    double VisibleUntil=0;
    double CooldownUntil=0;
};
