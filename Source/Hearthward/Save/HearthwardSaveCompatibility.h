#pragma once
#include "CoreMinimal.h"
class UHearthwardSaveGame;
struct FHearthwardSaveCompatibility
{
    FString SourcePath,SourceHash,Summary;
    TArray<FString> Changes;
    bool CanRepair=false;
};
namespace HearthwardSave
{
    bool ReadBytes(const TArray<uint8>& Bytes,UHearthwardSaveGame*& Out,FString& Error);
    bool Backup(const FString& Path,FString& BackupPath,FString& Error);
    bool InspectCompatibility(const FString& Path,FHearthwardSaveCompatibility& Report,UHearthwardSaveGame*& Candidate);
    bool ResolveCompatibility(const FHearthwardSaveCompatibility& Preview,const FString& Destination,FString& Status);
    FString Diagnose(const UHearthwardSaveGame& Pool);
}
