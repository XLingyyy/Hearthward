#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "HearthwardCampState.generated.h"

USTRUCT()
struct FHearthwardCampSite
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() FVector Position = FVector::ZeroVector;
};

USTRUCT()
struct FHearthwardCampFacility
{
    GENERATED_BODY()
    UPROPERTY() FGuid Id;
    UPROPERTY() FName Kind;
    UPROPERTY() FName Camp;
    UPROPERTY() int32 Level = 1;
    UPROPERTY() TMap<FName,int32> Paid;
    UPROPERTY() bool Paused = false;
};

USTRUCT()
struct FHearthwardCampSource
{
    GENERATED_BODY()
    UPROPERTY() FString Id;
    UPROPERTY() FName Camp;
    UPROPERTY() FName Item;
    UPROPERTY() FVector Position = FVector::ZeroVector;
    UPROPERTY() int32 Capacity = 0;
    UPROPERTY() int32 Remaining = 0;
    UPROPERTY() double RefreshMinutes = 0;
    UPROPERTY() double Due = -1;
    UPROPERTY() bool Blocked = false;
};

USTRUCT()
struct FHearthwardCampBatch
{
    GENERATED_BODY()
    UPROPERTY() bool Active = false;
    UPROPERTY() double Work = 0;
    UPROPERTY() double Required = 0;
    UPROPERTY() TMap<FName,int32> Inputs;
    UPROPERTY() TMap<FName,int32> Outputs;
    UPROPERTY() bool ToRations = false;
};

USTRUCT()
struct FHearthwardCampRegion
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() FName Camp;
    UPROPERTY() FName Job;
    UPROPERTY() FGuid Facility;
    UPROPERTY() TArray<int32> Workers;
    UPROPERTY() bool Player = false;
    UPROPERTY() bool Brother = false;
    UPROPERTY() bool Enabled = false;
    UPROPERTY() bool Safe = true;
    UPROPERTY() bool ToRations = false;
    UPROPERTY() int32 Priority = 0;
    UPROPERTY() FHearthwardCampBatch Batch;
    UPROPERTY() int32 Completed = 0;
    FString Status;
    // Recomputed from real actors each advance; never restored as free labor.
    double PlayerEfficiency = 0, BrotherEfficiency = 0;
};

// Pure economy state. The owner supplies the existing shared inventory transaction.
USTRUCT()
struct FHearthwardCampState
{
    GENERATED_BODY()
    UPROPERTY() int32 Version = 1;
    UPROPERTY() int32 Tier = 1;
    UPROPERTY() int64 RationHalfPoints = 200;
    UPROPERTY() double RationDrainMinutes = 0;
    UPROPERTY() double DonatedPoints = 0;
    UPROPERTY() double Calendar = 0;
    UPROPERTY() bool Hometown = false;
    UPROPERTY() TArray<FName> Rescued;
    UPROPERTY() TArray<FHearthwardCampSite> Camps;
    UPROPERTY() TArray<FHearthwardCampFacility> Facilities;
    UPROPERTY() TArray<FHearthwardCampRegion> Regions;
    UPROPERTY() TArray<FHearthwardCampSource> Sources;

    int32 Population() const { return 20 + Rescued.Num(); }
    double Rations() const { return RationHalfPoints * .5 - RationDrainMinutes / 60; }
    double Radius() const;
    float Bonus(const TCHAR* Field) const;
    FName CampAt(FVector Position) const;
    void AddCamp(FName Id,FVector Position);
    bool Assign(FName Region,int32 Person); // 0..29 ordinary; 30 player; 31 brother.
    FString UpgradeReason() const;
    bool Rescue(FName Person);
    void Drain(double Minutes);
    bool Donate(FName Item,int32 Count);
    bool Eat(float& Hunger);
    using FExchange = TFunction<bool(const TMap<FName,int32>&,const TMap<FName,int32>&)>;
    void Advance(double Minutes,bool Sleeping,const FExchange& Exchange);
    bool Validate() const;
    bool ValidateBuildings(const FString& Gameplay) const;
    FString Snapshot() const;
    static bool Parse(const FString& Json,FHearthwardCampState& Out);
};

namespace HearthwardCamp
{
    const TSharedPtr<FJsonObject>& Table();
    TSharedPtr<FJsonObject> Tier(int32 Level);
    TSharedPtr<FJsonObject> Facility(FName Kind,int32 Level);
    TSharedPtr<FJsonObject> Recipe(FName Id);
    TMap<FName,int32> Counts(const TSharedPtr<FJsonObject>& Row,const TCHAR* Field);
    TMap<FName,int32> BuildCost(FName Kind,int32 Level);
    int32 RequiredTier(FName Kind,int32 Level);
    double FoodPoints(FName Item);
    TMap<FName,int32> Refund(const TMap<FName,int32>& Paid);
}
