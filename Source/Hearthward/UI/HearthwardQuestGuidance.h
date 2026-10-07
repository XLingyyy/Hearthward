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
    bool HasRoute=false;
    FName RouteId;
    FString RouteLabel;
    FVector RouteWorld=FVector::ZeroVector;
};

namespace HearthwardQuestGuidance
{
    FHearthwardQuestMarker Resolve(APlayerController* Player,FName Quest=NAME_None);
    void Place(FHearthwardQuestMarker& Marker,FVector2D Size,FVector2D Projected,bool InFront,FVector2D CameraDirection);
}
