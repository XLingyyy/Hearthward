#pragma once
#include "CoreMinimal.h"

class UHearthwardGameplayComponent;
class UHearthwardCampaignSubsystem;

struct FHearthwardPreparationView
{
    FName Quest,Location;
    bool Visible=false,ReadyToClaim=false;
    FString Heading,NextStep,LocationLabel;
    FString Action;
};

namespace HearthwardPreparation
{
    FHearthwardPreparationView Read(const UHearthwardGameplayComponent* Gameplay,
        const UHearthwardCampaignSubsystem* Campaign,FName Quest);
}
