#pragma once

#include "CoreMinimal.h"
#include "DataTypes/A3GameRuntimeTypes.h"
#include "UObject/Interface.h"
#include "A3GameControllableEntity.generated.h"

UINTERFACE(BlueprintType)
class A3GAMEPLAYABLE_API UA3GameControllableEntity : public UInterface
{
    GENERATED_BODY()
};

class A3GAMEPLAYABLE_API IA3GameControllableEntity
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "A3Game|Runtime")
    FString GetRuntimeEntityId() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "A3Game|Runtime")
    void SetRuntimeEntityId(const FString& EntityId);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "A3Game|Runtime")
    bool ApplyRuntimeInput(const FA3GameRuntimeInputState& InputState);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "A3Game|Runtime")
    FA3GameEntitySnapshot GetRuntimeSnapshot() const;
};
