#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"
#include "../Companion/HearthwardCompanionFixture.h"

class UHearthwardCampSubsystem;
class UHearthwardStorageSubsystem;

struct FHearthwardCompanionWorkView
{
    bool Available=false,HasTask=false,Terminal=false,CanResume=false,CanCancel=false;
    FGuid Epoch,CommandId;
    FName Item,Intent;
    EHearthwardCompanionPhase Phase=EHearthwardCompanionPhase::Idle;
    TOptional<int32> Requested,Delivered,Acquired,Carried,RemainingToDeliver;
    FString DestinationText,BlockReason,Reason,ResumeReason,CancelReason;

    bool Matches(FGuid CurrentEpoch,FGuid CurrentCommand) const;
};

struct FHearthwardCampWorkView
{
    bool Available=false,HasTeam=false,Enabled=false,Safe=false,AssignedBrother=false,BrotherWorking=false;
    FGuid Epoch;
    FName Camp,Region,Job;
    TArray<int32> Workers;
    TOptional<int32> CompletedBatches;
    FString Status,Reason,BrotherStatus;
};

namespace HearthwardPresentation
{
    FString CompanionPhaseText(EHearthwardCompanionPhase Phase);
    FString CompanionBlockText(const FString& Reason);
    FString CompanionWorkText(const FHearthwardCompanionWorkView& View);
    FString WorkPartyText(const FHearthwardCampWorkView& View);
    FString PersonalActionToken(const FString& Action,const FHearthwardCompanionWorkView& View);
    FString WorkPartyActionToken(const FString& Action,const FHearthwardCampWorkView& View);
    FHearthwardCompanionWorkView ProjectCompanion(const FHearthwardCompanionCommand& Command,
        EHearthwardCompanionPhase Phase,FGuid CurrentEpoch,const FString& BlockReason=FString());
    FHearthwardCompanionWorkView ReadCompanion(const AHearthwardCompanionFixture* Companion,
        const UHearthwardStorageSubsystem* Storage,AActor* Speaker=nullptr);
    FHearthwardCampWorkView ReadWorkParty(const UHearthwardCampSubsystem* Camp,
        const UHearthwardStorageSubsystem* Storage,FName CampId);
    bool CanApplyPersonalAction(const FHearthwardCompanionWorkView& View,
        const AHearthwardCompanionFixture* Companion,const UHearthwardStorageSubsystem* Storage,
        AActor* Speaker,bool Resume);
}
