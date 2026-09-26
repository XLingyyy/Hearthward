#pragma once
#include "CoreMinimal.h"
#include "HearthwardSurvivalState.generated.h"

UENUM(BlueprintType)
enum class EHearthwardLife : uint8 { Alive, Downed, Dead };

// Timers use active seconds, except SevereDue, which uses elapsed calendar minutes.
USTRUCT(BlueprintType)
struct FHearthwardSurvivalState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) EHearthwardLife Life = EHearthwardLife::Alive;
    UPROPERTY() double SevereDue = -1;
    UPROPERTY() double DownRemaining = 0;
    UPROPERTY() double DrowningRemaining = -1;
    UPROPERTY() double HotRemaining = 0;
    UPROPERTY() double HotRate = 0;
    UPROPERTY() double RecoveryDelay = 0;
    UPROPERTY() double SafeSeconds = 0;
    UPROPERTY(BlueprintReadOnly) FName Medicine;
    UPROPERTY(BlueprintReadOnly) double MedicineRemaining = 0;
    UPROPERTY() bool AutomaticMedicine = false;
    UPROPERTY() TSet<FName> AutoPermissions;

    bool Severe() const { return SevereDue >= 0; }
    void Food(float Hunger) { if (Hunger >= 10) SevereDue = -1; }
    void Kill() { Life=EHearthwardLife::Dead; DownRemaining=0; HotRemaining=HotRate=0; }
    void Damage(float& Health, float Amount)
    {
        if (Amount<=0 || Life==EHearthwardLife::Dead) return;
        SafeSeconds=0;
        if (Life==EHearthwardLife::Downed) { Health=0; Kill(); return; }
        Health=FMath::Max(0.f,Health-Amount);
        if (Health==0) { Life=EHearthwardLife::Downed; DownRemaining=120; HotRemaining=HotRate=0; }
    }
    // Returns the consumed fraction, so a calendar jump can stop at a fatal boundary.
    double Advance(float& Health,float& Hunger,float Maximum,double Active,double Calendar,double StartW,
        double HungerMultiplier=1,double RecoveryFraction=.005)
    {
        if (Life==EHearthwardLife::Dead) return 0;
        if(Hunger==0 && Health<=Maximum*.1+1e-5 && !Severe()) SevereDue=StartW+4320;
        double Fraction=1;
        if (Life==EHearthwardLife::Downed && Active>0) Fraction=FMath::Min(Fraction,DownRemaining/Active);
        if (DrowningRemaining>=0 && Active>0) Fraction=FMath::Min(Fraction,DrowningRemaining/Active);
        if (Severe() && Calendar>0) Fraction=FMath::Min(Fraction,FMath::Max(0.,SevereDue-StartW)/Calendar);
        double Used=0;
        while (Used<Fraction)
        {
            const double Rate=100./2880*HungerMultiplier;
            double Step=Fraction-Used;
            if (Hunger>0 && Calendar>0) Step=FMath::Min(Step,double(Hunger)/(Calendar*Rate));
            if (HotRemaining>0 && Active>0) Step=FMath::Min(Step,HotRemaining/Active);
            const double Healing=Life==EHearthwardLife::Alive?(Hunger>0?Maximum*RecoveryFraction:0)+(HotRemaining>0?HotRate:0):0;
            const double Floor=Maximum*.1;
            if(Health>Floor && Health<Floor+1e-5) Health=float(Floor);
            const double Drain=Hunger<=0 && (Health>Floor || (Health==Floor && Healing>0))?Maximum*.003:0;
            const double Net=Healing*Active-Drain*Calendar;
            if (Hunger<=0 && Health<Floor && Net>0) Step=FMath::Min(Step,(Floor-Health)/Net);
            if (Hunger<=0 && Health>Maximum*.1 && Net<0) Step=FMath::Min(Step,(Health-Maximum*.1)/-Net);
            if (Step<=1e-10) break;
            if (Life==EHearthwardLife::Alive) Health=FMath::Clamp(float(Health+Net*Step),Hunger<=0 && Health>=Floor?float(Floor):0.f,Maximum);
            Hunger=FMath::Max(0.f,float(Hunger-Calendar*Step*Rate));
            if (Hunger<1e-5) Hunger=0;
            HotRemaining=FMath::Max(0.,HotRemaining-Active*Step);
            Used+=Step;
            if (Hunger==0 && Health<=Maximum*.1+1e-5 && !Severe())
            {
                SevereDue=StartW+Calendar*Used+4320;
                if (Calendar>0) Fraction=FMath::Min(Fraction,(SevereDue-StartW)/Calendar);
            }
        }
        if (Hunger==0 && Health<=Maximum*.1+1e-5 && !Severe()) SevereDue=StartW+Calendar*Used+4320;
        if (Life==EHearthwardLife::Downed) DownRemaining=FMath::Max(0.,DownRemaining-Active*Used);
        if (DrowningRemaining>=0) DrowningRemaining=FMath::Max(0.,DrowningRemaining-Active*Used);
        if ((Life==EHearthwardLife::Downed && DownRemaining<=1e-6) || DrowningRemaining==0
            || (Severe() && StartW+Calendar*Used>=SevereDue-1e-6)) { Health=0; Kill(); }
        return Used;
    }
};
