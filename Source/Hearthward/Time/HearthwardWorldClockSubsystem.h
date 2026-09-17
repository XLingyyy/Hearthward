#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HearthwardClockState.h"
#include "HearthwardWorldClockSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FHearthwardClockSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Hearthward|Time")
    double ActivePlaySeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Hearthward|Time")
    double ElapsedCalendarMinutes = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Hearthward|Time")
    int64 ElapsedDays = 0;

    UPROPERTY(BlueprintReadOnly, Category="Hearthward|Time")
    double MinuteOfDay = 0.0;
};

UCLASS()
class HEARTHWARD_API UHearthwardWorldClockSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { return false; }
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category="Hearthward|Time")
    FHearthwardClockSnapshot GetSnapshot() const;

protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
    FHearthwardClockState Clock;
};
