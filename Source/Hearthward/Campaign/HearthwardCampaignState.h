#pragma once
#include "CoreMinimal.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "HearthwardCampaignState.generated.h"

USTRUCT()
struct FHearthwardCampaignEnemy
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() FName Zone;
    UPROPERTY() FName Kind;
    UPROPERTY() FName Group;
    UPROPERTY() int32 Stage=1;
    UPROPERTY() FVector Home=FVector::ZeroVector;
    UPROPERTY() FHearthwardCombatTargetSave Combat;
    UPROPERTY() bool Located=false;
    UPROPERTY() double RefreshDue=-1;
    UPROPERTY() int32 PatrolPoint=0;
};
USTRUCT()
struct FHearthwardCampaignPerson
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() FName Location;
    UPROPERTY() FName Stage=TEXT("uncontacted");
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() bool Located=false;
};
USTRUCT()
struct FHearthwardCampaignState
{
    GENERATED_BODY()
    UPROPERTY() int32 Version=1;
    UPROPERTY() FName Phase;
    UPROPERTY() bool Legacy=false;
    UPROPERTY() bool LegacyHometown=false;
    UPROPERTY() bool Victory=false;
    UPROPERTY() TArray<FHearthwardCampaignEnemy> Enemies;
    UPROPERTY() TArray<FHearthwardCampaignPerson> People;
    UPROPERTY() TMap<FName,FName> Reinforcements;
    UPROPERTY() TSet<FName> Flags;
    UPROPERTY() TSet<FName> Facts;
    UPROPERTY() TMap<FName,FVector> Positions;
    int32 Total() const;
    int32 Cleared() const;
    bool ZoneClear(FName Zone,bool BaseOnly=false) const;
    bool ReadyForVictory() const;
    bool ShowRemaining() const { return Total()>0 && Cleared()*100>Total()*95; }
    void Initialize(bool Old=false,bool Home=false);
    void ResolveUntriggered();
    bool RegisterReinforcement(FName Zone);
    bool Validate() const;
    FString Snapshot() const;
    static bool Parse(const FString& Json,FHearthwardCampaignState& Out);
};
namespace HearthwardCampaign
{
    const TArray<TSharedPtr<class FJsonValue>>& Rows(const FString& Name);
    TSharedPtr<class FJsonObject> Find(const FString& Table,FName Id);
    FVector XY(const TSharedPtr<class FJsonObject>& Row,const FString& Key);
    double Health(FName Kind,int32 Stage);
}
