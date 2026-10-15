#pragma once
#include "CoreMinimal.h"

namespace HearthwardEnemyRole
{
struct FAttack
{
    float Windup=.45f,Active=.2f,Recovery=.65f;
    float Duration() const { return Windup+Active+Recovery; }
    float ClipPhase(float Elapsed) const
    {
        if(Elapsed<Windup)return .6f*FMath::Clamp(Elapsed/Windup,0.f,1.f);
        if(Elapsed<Windup+Active)return .6f+.2f*(Elapsed-Windup)/Active;
        return .8f+.2f*FMath::Clamp((Elapsed-Windup-Active)/Recovery,0.f,1.f);
    }
};
inline bool IsElite(FName Id) { return Id==TEXT("home_workshops_18"); }
inline FAttack Attack(FName Id,FName Kind)
{
    if(IsElite(Id))return {1.1f,.2f,1.3f};
    if(Kind==TEXT("heavy"))return {.8f,.2f,.9f};
    return {};
}
inline bool InStrike(float Elapsed,const FAttack& A) { return Elapsed>=A.Windup && Elapsed<A.Windup+A.Active; }
}
