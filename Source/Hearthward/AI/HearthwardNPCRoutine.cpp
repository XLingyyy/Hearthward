#include "HearthwardNPCRoutine.h"

FHearthwardNPCRoutineDecision HearthwardRoutine::Evaluate(const FHearthwardNPCRoutineContext& C)
{
    FHearthwardNPCRoutineDecision D;
    if(!C.Enabled || !C.bOrderIdle || C.bPlayerInCombat || C.bPlayerDown || C.bTaskActive)return D;

    D.Active=true;
    if(C.CompanionToCampDistance>450.0f)
    {
        D.Activity=TEXT("return_camp");
        D.CampOffset=FVector::ZeroVector;
        D.MoveSpeed=180.0f;
        D.AcceptanceRadius=70.0f;
        return D;
    }

    const int32 Phase=FMath::FloorToInt(FMath::Max(0.0,C.GameSeconds)/6.0)%4;
    switch(Phase)
    {
    case 0:
        D.Activity=TEXT("rest");
        D.CampOffset=FVector(-80,-40,0);
        D.MoveSpeed=100.0f;
        break;
    case 1:
        D.Activity=TEXT("patrol");
        D.CampOffset=FVector(180,80,0);
        D.MoveSpeed=125.0f;
        break;
    case 2:
        D.Activity=TEXT("check_camp");
        D.CampOffset=FVector(60,-140,0);
        D.MoveSpeed=110.0f;
        break;
    default:
        D.Activity=TEXT("patrol");
        D.CampOffset=FVector(-170,90,0);
        D.MoveSpeed=125.0f;
        break;
    }
    return D;
}
