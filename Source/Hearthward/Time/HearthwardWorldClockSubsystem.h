#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HearthwardClockState.h"
#include "HearthwardWorldClockSubsystem.generated.h"

UENUM(BlueprintType)
enum class EHearthwardTimeAdvanceKind : uint8 { Sleep, Campfire };
UENUM(BlueprintType)
enum class EHearthwardTimeAdvanceStatus : uint8 { Completed, Rejected, StoppedAtFailure };
USTRUCT(BlueprintType)
struct FHearthwardTimeAdvanceRequest
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EHearthwardTimeAdvanceKind Kind=EHearthwardTimeAdvanceKind::Sleep;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FGuid Facility;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FGuid Campaign;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FGuid Epoch;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FGuid OperationId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double StartW=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double Minutes=480;
    bool operator==(const FHearthwardTimeAdvanceRequest& Other) const
    { return Kind==Other.Kind && Facility==Other.Facility && Campaign==Other.Campaign && Epoch==Other.Epoch
        && OperationId==Other.OperationId && StartW==Other.StartW && Minutes==Other.Minutes; }
};
USTRUCT(BlueprintType)
struct FHearthwardTimeAdvanceReceipt
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool Accepted=false;
    UPROPERTY(BlueprintReadOnly) bool Completed=false;
    UPROPERTY(BlueprintReadOnly) EHearthwardTimeAdvanceStatus Status=EHearthwardTimeAdvanceStatus::Rejected;
    UPROPERTY(BlueprintReadOnly) FGuid OperationId;
    UPROPERTY(BlueprintReadOnly) double StartW=0;
    UPROPERTY(BlueprintReadOnly) double EndW=0;
    UPROPERTY(BlueprintReadOnly) double CommittedMinutes=0;
    UPROPERTY(BlueprintReadOnly) FName ReasonCode;
    UPROPERTY(BlueprintReadOnly) FString Reason;
};

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
    UPROPERTY(BlueprintReadOnly, Category="Hearthward|Time") int64 DisplayDay=1;
    UPROPERTY(BlueprintReadOnly, Category="Hearthward|Time") double Daylight=0;
};

UCLASS()
class HEARTHWARD_API UHearthwardWorldClockSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { return false; }
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category="Hearthward|Time")
    FHearthwardClockSnapshot GetSnapshot() const;
    UFUNCTION(BlueprintCallable,Category="Hearthward|Time") FHearthwardTimeAdvanceReceipt RequestTimeAdvance(const FHearthwardTimeAdvanceRequest& Request);
    bool Busy() const { return Advancing || Restoring; }
    bool Suspended() const;
    static double DaylightAt(double Minute);
#if !UE_BUILD_SHIPPING
    // Nonshipping fixture only; production jumps require a real facility and a request receipt.
    double AdvanceCalendar(double Minutes);
#endif
private:
    double AdvanceSurvival(double Active,double Calendar);
    void Install(double Active,double Calendar,int64 InitialDay,double InitialMinute);

protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
    friend class UHearthwardSaveSubsystem;
    FHearthwardClockState Clock;
    bool Advancing=false,Restoring=false;
    FGuid ReceiptEpoch;
    struct FCachedReceipt { FHearthwardTimeAdvanceRequest Request;FHearthwardTimeAdvanceReceipt Receipt; };
    TMap<FGuid,FCachedReceipt> Receipts;
};
