#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Actions/HearthwardTimedActionState.h"
#include "../AI/HearthwardNPCMemory.h"
#include "HearthwardSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardSaveSafety
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Combat = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool EitherDowned = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Pursued = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Drowning = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool Falling = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool CompanionDanger = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool SevereHunger = false;
    bool CanSave() const { return !(Combat || EitherDowned || Pursued || Drowning || Falling || CompanionDanger); }
};

USTRUCT()
struct FHearthwardSavedTimer
{
    GENERATED_BODY()
    UPROPERTY() EHearthwardTimedActionStatus Status = EHearthwardTimedActionStatus::Idle;
    UPROPERTY() double Elapsed = 0;
};

USTRUCT()
struct FHearthwardWorldSave
{
    GENERATED_BODY()
    UPROPERTY() FString Map;
    UPROPERTY() double ActiveSeconds = 0;
    UPROPERTY() FTransform Player = FTransform::Identity;
    UPROPERTY() FRotator View = FRotator::ZeroRotator;
    UPROPERTY() TMap<FName, int32> Inventory;
    UPROPERTY() TMap<FName, int32> Storage;
    UPROPERTY() FHearthwardSavedTimer PlayerTimer;
    UPROPERTY() FTransform Companion = FTransform::Identity;
    UPROPERTY() FTransform Camp = FTransform::Identity;
    UPROPERTY() FTransform Source = FTransform::Identity;
    UPROPERTY() TMap<FName, int32> Bag;
    UPROPERTY() TMap<FName, int32> Resource;
    UPROPERTY() bool SourceSafe = false;
    UPROPERTY() EHearthwardCompanionPhase Phase = EHearthwardCompanionPhase::Idle;
    UPROPERTY() FName Item;
    UPROPERTY() int32 Requested = 0;
    UPROPERTY() int32 Delivered = 0;
    UPROPERTY() bool CommandActive = false;
    UPROPERTY() FString Statement;
    UPROPERTY() FString BlockReason;
    UPROPERTY() FHearthwardSavedTimer CompanionTimer;
    // Raw recoverable exchanges, never a path to a future external index.
    UPROPERTY() TArray<FString> Knowledge;
    UPROPERTY() int64 KnowledgeRevision = 0;
    UPROPERTY() FHearthwardNPCMemory NPCMemory;
    UPROPERTY() int32 AutoMinutes = 10;
    UPROPERTY() FHearthwardSaveSafety Safety;
    UPROPERTY() FString Gameplay;
};

USTRUCT(BlueprintType)
struct FHearthwardSavePoint
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid SaveId;
    UPROPERTY(BlueprintReadOnly) FGuid CampaignId;
    UPROPERTY(BlueprintReadOnly) FDateTime Created;
    UPROPERTY(BlueprintReadOnly) bool Manual = false;
    UPROPERTY(BlueprintReadOnly) bool Locked = false;
    UPROPERTY(BlueprintReadOnly) FString Location;
    UPROPERTY(BlueprintReadOnly) FString Stage;
    UPROPERTY() FString Build;
    UPROPERTY() FHearthwardWorldSave World;
};

// TASK-016 isolated development format. Not a promise of full GDD world coverage.
UCLASS()
class HEARTHWARD_API UHearthwardSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 Schema = 1;
    UPROPERTY() TArray<FHearthwardSavePoint> Points;
};

namespace HearthwardSave
{
    constexpr int32 MaxPoints = 50;
    // INDEX_NONE means no capacity. An index equal to Num means append.
    int32 SelectSlot(const TArray<FHearthwardSavePoint>& Points);
    bool Validate(const UHearthwardSaveGame& Pool);
    bool Read(const FString& Path, UHearthwardSaveGame*& Out, FString& Error);
    bool Write(const FString& Path, UHearthwardSaveGame* Pool, FString& Error);
}
