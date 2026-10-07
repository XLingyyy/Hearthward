#pragma once
#include "CoreMinimal.h"
class UWorld;

struct FHearthwardRouteStep
{
    bool Visible=false;
    FName Id;
    FString Label;
    FVector World=FVector::ZeroVector;
};

namespace HearthwardGuidanceRoutes
{
    FHearthwardRouteStep Resolve(UWorld* World,FName Quest,FName Destination,FVector PlayerPosition);
}
