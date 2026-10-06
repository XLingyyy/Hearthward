#pragma once
#include "CoreMinimal.h"
class APlayerController;

struct FHearthwardQuestMarker
{
    bool Visible=false,Offscreen=false;
    FName Location;
    FString Label;
    FVector World=FVector::ZeroVector;
    FVector2D Position=FVector2D::ZeroVector,Direction=FVector2D::ZeroVector;
    double Distance=0;
};

namespace HearthwardQuestGuidance
{
    FHearthwardQuestMarker Resolve(APlayerController* Player);
    void Place(FHearthwardQuestMarker& Marker,FVector2D Size,FVector2D Projected,bool InFront,FVector2D CameraDirection);
}
