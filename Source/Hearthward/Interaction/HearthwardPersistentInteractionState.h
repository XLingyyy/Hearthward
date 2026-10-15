#pragma once

#include "CoreMinimal.h"
#include "HearthwardPersistentInteractionState.generated.h"

UENUM()
enum class EHearthwardPersistentKind : uint8 { Lamp, Shortcut, Cooperation, Person, Weather, Trail, Display };
UENUM()
enum class EHearthwardWeatherPhase : uint8 { Clear, Rain };
UENUM()
enum class EHearthwardCooperationResult : uint8 { Unchanged, Released, Cleared };

USTRUCT()
struct FHearthwardLampState
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() bool Lit = false;
};
USTRUCT()
struct FHearthwardShortcutState
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() bool Open = false;
};
USTRUCT()
struct FHearthwardCooperationState
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() EHearthwardCooperationResult Result = EHearthwardCooperationResult::Unchanged;
};
USTRUCT()
struct FHearthwardPersonChoiceState
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    // Values belong to this person's registered steps/choices, not arbitrary dialogue text.
    UPROPERTY() FName Step;
    UPROPERTY() FName Choice;
};
USTRUCT()
struct FHearthwardWeatherState
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() int32 Seed = 0;
    UPROPERTY() EHearthwardWeatherPhase Phase = EHearthwardWeatherPhase::Clear;
    UPROPERTY() double PhaseStartedAt = 0;
};
USTRUCT()
struct FHearthwardTrailSample
{
    GENERATED_BODY()
    UPROPERTY() FVector Position = FVector::ZeroVector;
    UPROPERTY() double CalendarMinutes = 0;
};
USTRUCT()
struct FHearthwardTrailState
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() FGuid Animal;
    UPROPERTY() FName Species;
    UPROPERTY() int32 Generation = 0;
    UPROPERTY() TArray<FHearthwardTrailSample> Samples;
};
USTRUCT()
struct FHearthwardDisplayState
{
    GENERATED_BODY()
    UPROPERTY() FName Id;
    UPROPERTY() FName Camp;
    UPROPERTY() FName Slot;
    UPROPERTY() FName Definition;
    // A reference to the original equipment identity; never an inventory copy.
    UPROPERTY() FGuid Instance;
    UPROPERTY() FName Fact;
};
USTRUCT()
struct FHearthwardPersistentInteractionState
{
    GENERATED_BODY()
    UPROPERTY() int32 Version = 1;
    UPROPERTY() int64 Revision = 0;
    UPROPERTY() TArray<FHearthwardLampState> Lamps;
    UPROPERTY() TArray<FHearthwardShortcutState> Shortcuts;
    UPROPERTY() TArray<FHearthwardCooperationState> Cooperation;
    UPROPERTY() TArray<FHearthwardPersonChoiceState> People;
    UPROPERTY() TArray<FHearthwardWeatherState> Weather;
    UPROPERTY() TArray<FHearthwardTrailState> Trails;
    UPROPERTY() TArray<FHearthwardDisplayState> Displays;
    bool Validate(double CalendarMinutes, FString& Error) const;
    bool IsEmpty() const;
    bool Serialize(FArchive& Ar);
};
template<> struct TStructOpsTypeTraits<FHearthwardPersistentInteractionState> : TStructOpsTypeTraitsBase2<FHearthwardPersistentInteractionState>
{ enum { WithSerializer=true }; };

// Definitions are registered by content before reading a pool, independently of streamed actors.
struct FHearthwardPersistentDefinition
{
    EHearthwardPersistentKind Kind = EHearthwardPersistentKind::Lamp;
    TSet<FName> Steps;
    TSet<FName> Choices;
    FName Camp;
    FName Slot;
    TSet<FName> DisplayDefinitions;
    TSet<FName> DisplayFacts;
    TMap<FName,FName> DisplayOwners;
    int32 MaxTrailSamples = 0;
    FName TrailSpecies;
};
namespace HearthwardPersistence
{
    bool Register(FName Id, const FHearthwardPersistentDefinition& Definition);
    const FHearthwardPersistentDefinition* Find(FName Id, EHearthwardPersistentKind Kind);
#if WITH_DEV_AUTOMATION_TESTS
    void UnregisterFixture(FName Id);
#endif
}

// Held only in memory. Completion must re-resolve Id and check this ticket against the current epoch/revision.
struct FHearthwardInteractionTicket
{
    FGuid Operation;
    FGuid Epoch;
    FName Id;
    int64 Revision = -1;
    TWeakObjectPtr<AActor> Target;
};
