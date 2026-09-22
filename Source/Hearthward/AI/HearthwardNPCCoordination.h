#pragma once
#include "CoreMinimal.h"
#include "HearthwardNPCMemory.h"
#include "HearthwardNPCCoordination.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardNPCCoordinationProfile
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 Samples = 0;
    UPROPERTY(BlueprintReadOnly) int32 HoldCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 FollowCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 AssistCount = 0;
    UPROPERTY(BlueprintReadOnly) FName PreferredDirective;
    UPROPERTY(BlueprintReadOnly) float Confidence = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool Stable = false;
    UPROPERTY(BlueprintReadOnly) double LastObservedAt = 0.0;
};

namespace HearthwardCoordination
{
    FHearthwardNPCCoordinationProfile Build(const TArray<FHearthwardNPCEvent>& Events,int32 Window=8);
    FString Describe(const FHearthwardNPCCoordinationProfile& Profile);
}
