#pragma once
#include "CoreMinimal.h"

// Bounds constrain destinations and every movement step, including large DeltaSeconds.
namespace HearthwardAnimalBounds
{
    inline FVector Clamp(const FBox& Box,FVector P)
    {
        return FVector(FMath::Clamp(P.X,Box.Min.X,Box.Max.X),FMath::Clamp(P.Y,Box.Min.Y,Box.Max.Y),FMath::Clamp(P.Z,Box.Min.Z,Box.Max.Z));
    }
    inline FVector FleeGoal(const FBox& Box,FVector P,FVector Threat,FVector Forward)
    {
        FVector Best=P;double BestScore=-DBL_MAX;
        FVector Away=(P-Threat).GetSafeNormal2D();if(Away.IsNearlyZero())Away=Forward.GetSafeNormal2D();
        for(int32 I=0;I<32;++I)
        {
            FVector Candidate=Clamp(Box,P+Away.RotateAngleAxis(I*11.25f,FVector::UpVector)*750);
            Candidate.Z=P.Z;const FVector D=Candidate-P;
            const double Score=FVector::DistSquared2D(Candidate,Threat)+FVector::DotProduct(D.GetSafeNormal2D(),Forward)*20000;
            if(Score>BestScore){BestScore=Score;Best=Candidate;}
        }
        return Best;
    }
}
