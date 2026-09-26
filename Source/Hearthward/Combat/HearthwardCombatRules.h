#pragma once
#include "CoreMinimal.h"

namespace HearthwardCombat
{
struct FMove
{
    double Windup=.18, Active=.12, Recovery=.30;
    float Cost=8, Multiplier=1, Reach=180;
    double Duration() const { return Windup+Active+Recovery; }
};
inline FMove Move(FName Kind,bool Heavy)
{
    if(Kind==TEXT("longblade")) return Heavy?FMove{.55,.22,.63,26,2.3f,230}:FMove{.28,.18,.44,12,1.35f,230};
    if(Kind==TEXT("spear")) return Heavy?FMove{.50,.15,.55,24,2,280}:FMove{.25,.12,.38,10,1.15f,280};
    if(Kind==TEXT("blunt")) return Heavy?FMove{.65,.20,.65,30,2.6f,200}:FMove{.35,.18,.47,14,1.5f,200};
    return Heavy?FMove{.40,.16,.44,20,1.8f,180}:FMove{};
}
inline double Detection(double Value,double Seconds,double DistanceMeters,bool Visible)
{
    return FMath::Clamp(Value+Seconds*(Visible?.2*FMath::Clamp(10/FMath::Max(1.,DistanceMeters),.4,2.):- .1),0.,1.);
}
inline bool InFront(const FVector& Facing,const FVector& Direction)
{ return FVector::DotProduct(Facing.GetSafeNormal2D(),Direction.GetSafeNormal2D())>=.5; }
inline float ArmorDamage(float Raw,float PartArmor,float OtherArmor=0)
{ return Raw*FMath::Max(.15f,(1-FMath::Clamp(PartArmor,0.f,1.f))*(1-FMath::Clamp(OtherArmor,0.f,1.f))); }
inline bool SenseVisible(const FVector& From,const FVector& To)
{ return FVector::DistSquared(From,To)<=FMath::Square(1500.) && FMath::Abs(From.Z-To.Z)<=400; }
struct FGuard
{
    bool Held=false, Released=true;
    double RaisedAt=0, BrokenUntil=0;
    bool Raise(double Now,float Stamina,float Maximum)
    {
        if(!Released || Stamina<=0 || Now<BrokenUntil || (BrokenUntil>0 && Stamina<Maximum*.2f)) return false;
        Held=true; Released=false; RaisedAt=Now; return true;
    }
    void Release() { Held=false; Released=true; }
    bool Hit(double Now,float Cost,float& Stamina)
    {
        if(!Held || Now<RaisedAt+.15) return false;
        Stamina=FMath::Max(0.f,Stamina-Cost);
        if(Stamina==0) { Held=false; BrokenUntil=Now+1; }
        return true;
    }
};
}
