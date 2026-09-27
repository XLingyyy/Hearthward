#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "../Inventory/HearthwardInventoryState.h"
#include "HearthwardNatureState.generated.h"

USTRUCT()
struct FHearthwardNaturePoint
{
    GENERATED_BODY()
    UPROPERTY() FGuid Id;
    UPROPERTY() FName Key;
    UPROPERTY() FName Kind;
    UPROPERTY() FName Definition;
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() int32 Remaining=0;
    UPROPERTY() int32 Successes=0;
    UPROPERTY() double Due=-1;
    UPROPERTY() FHearthwardInventorySnapshot Pending;
};
USTRUCT()
struct FHearthwardCrop
{
    GENERATED_BODY()
    UPROPERTY() FGuid Id;
    UPROPERTY() FName Definition;
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() double Planted=0;
    UPROPERTY() bool Watered=false;
    UPROPERTY() bool Fertilized=false;
};
USTRUCT()
struct FHearthwardAnimal
{
    GENERATED_BODY()
    UPROPERTY() FGuid Id;
    UPROPERTY() FGuid Pen;
    UPROPERTY() FGuid ReservedPen;
    UPROPERTY() FName Definition;
    UPROPERTY() FName Slot;
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() FVector Destination=FVector::ZeroVector;
    UPROPERTY() float Health=0;
    UPROPERTY() bool Domestic=false;
    UPROPERTY() bool Juvenile=false;
    UPROPERTY() bool Captured=false;
    UPROPERTY() bool Following=false;
    UPROPERTY() bool Rewarded=false;
    UPROPERTY() double Growth=0;
    UPROPERTY() double FedRemaining=0;
    UPROPERTY() double AlertRemaining=0;
    UPROPERTY() TMap<FName,int32> Loot;
};
USTRUCT()
struct FHearthwardWildSlot
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() FName Definition;
    UPROPERTY() FGuid Current;
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() int32 Generation=1;
    UPROPERTY() double Due=-1;
};
USTRUCT()
struct FHearthwardPen
{
    GENERATED_BODY()
    UPROPERTY() FGuid Id;
    UPROPERTY() FName Definition;
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() int32 Level=1;
    UPROPERTY() int32 Feed=0;
    UPROPERTY() TMap<FName,int32> Paid;
    UPROPERTY() TMap<FString,double> Pairs;
};
USTRUCT()
struct FHearthwardNatureState
{
    GENERATED_BODY()
    UPROPERTY() int32 Version=1;
    UPROPERTY() int32 Seed=0;
    UPROPERTY() double Calendar=0;
    UPROPERTY() TSet<FName> Camps;
    UPROPERTY() TArray<FHearthwardNaturePoint> Points;
    UPROPERTY() TArray<FHearthwardCrop> Crops;
    UPROPERTY() TArray<FHearthwardAnimal> Animals;
    UPROPERTY() TArray<FHearthwardWildSlot> Slots;
    UPROPERTY() TArray<FHearthwardPen> Pens;
    UPROPERTY() TSet<FName> Rewards;
    UPROPERTY() TSet<FName> Maps;
    UPROPERTY() TSet<FName> Opened;
    FString Snapshot() const;
    static bool Parse(const FString& Json,FHearthwardNatureState& Out);
    bool Valid() const;
    void Advance(double Minutes);
    int32 Occupants(FGuid Pen,bool IncludeReserved=true) const;
    bool Ready(const FHearthwardCrop& Crop) const;
    int32 Yield(const FHearthwardCrop& Crop) const;
};
namespace HearthwardNature
{
    TSharedPtr<FJsonObject> Definition(const TCHAR* Table,FName Id);
    const TArray<TSharedPtr<FJsonValue>>& Rows(const TCHAR* Table);
    int32 Capacity(int32 Level);
    FName Reward(int32 Seed,FName Point,int32 Success);
    struct FFishing
    {
        double Elapsed=0,Tension=.45,Progress=0,Bite=2,Required=5;
        bool Cast=false,Done=false,Failed=false;
        void Advance(double Seconds,bool Held);
    };
}
