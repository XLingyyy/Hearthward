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
inline double StoneAxeClipTime(double Elapsed,double ClipLength,const FMove& M)
{
    const double T=FMath::Clamp(Elapsed,0.,M.Duration());
    if(T<=M.Windup) return ClipLength*.6*T/M.Windup;
    if(T<=M.Windup+M.Active) return ClipLength*(.6+.2*(T-M.Windup)/M.Active);
    return ClipLength*(.8+.2*(T-M.Windup-M.Active)/M.Recovery);
}
inline double Detection(double Value,double Seconds,double DistanceMeters,bool Visible)
{
    return FMath::Clamp(Value+Seconds*(Visible?.2*FMath::Clamp(10/FMath::Max(1.,DistanceMeters),.4,2.):- .1),0.,1.);
}
inline bool InFront(const FVector& Facing,const FVector& Direction)
{ return FVector::DotProduct(Facing.GetSafeNormal2D(),Direction.GetSafeNormal2D())>=.5; }
inline float ArmorDamage(float Raw,float PartArmor,float OtherArmor=0)
{ return Raw*FMath::Max(.15f,(1-FMath::Clamp(PartArmor,0.f,1.f))*(1-FMath::Clamp(OtherArmor,0.f,1.f))); }
inline FName HumanoidPart(FName Bone)
{
    static const FName HeadBones[]={
        TEXT("neck_01"),TEXT("head")
    };
    for(FName Name:HeadBones)if(Name==Bone)return TEXT("head");
    static const FName BodyBones[]={
        TEXT("pelvis"),TEXT("spine_01"),TEXT("spine_02"),TEXT("spine_03"),
        TEXT("clavicle_l"),TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),
        TEXT("index_01_l"),TEXT("index_02_l"),TEXT("index_03_l"),TEXT("middle_01_l"),
        TEXT("middle_02_l"),TEXT("middle_03_l"),TEXT("pinky_01_l"),TEXT("pinky_02_l"),
        TEXT("pinky_03_l"),TEXT("ring_01_l"),TEXT("ring_02_l"),TEXT("ring_03_l"),
        TEXT("thumb_01_l"),TEXT("thumb_02_l"),TEXT("thumb_03_l"),TEXT("lowerarm_twist_01_l"),
        TEXT("upperarm_twist_01_l"),TEXT("clavicle_r"),TEXT("upperarm_r"),TEXT("lowerarm_r"),
        TEXT("hand_r"),TEXT("index_01_r"),TEXT("index_02_r"),TEXT("index_03_r"),
        TEXT("middle_01_r"),TEXT("middle_02_r"),TEXT("middle_03_r"),TEXT("pinky_01_r"),
        TEXT("pinky_02_r"),TEXT("pinky_03_r"),TEXT("ring_01_r"),TEXT("ring_02_r"),
        TEXT("ring_03_r"),TEXT("thumb_01_r"),TEXT("thumb_02_r"),TEXT("thumb_03_r"),
        TEXT("lowerarm_twist_01_r"),TEXT("upperarm_twist_01_r")
    };
    for(FName Name:BodyBones)if(Name==Bone)return TEXT("body");
    static const FName LegsBones[]={
        TEXT("thigh_l"),TEXT("calf_l"),TEXT("calf_twist_01_l"),TEXT("thigh_twist_01_l"),
        TEXT("thigh_r"),TEXT("calf_r"),TEXT("calf_twist_01_r"),TEXT("thigh_twist_01_r")
    };
    for(FName Name:LegsBones)if(Name==Bone)return TEXT("legs");
    static const FName FeetBones[]={
        TEXT("foot_l"),TEXT("ball_l"),TEXT("foot_r"),TEXT("ball_r")
    };
    for(FName Name:FeetBones)if(Name==Bone)return TEXT("feet");
    return NAME_None;
}
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
