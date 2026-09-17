#pragma once

#include "CoreMinimal.h"
#include "DataTypes/A3GameRuntimeTypes.h"
#include "UObject/Interface.h"
#include "A3GameEntityFactory.generated.h"

class AActor;

UINTERFACE(BlueprintType)
class A3GAMEPLAYABLE_API UA3GameEntityFactory : public UInterface
{
    GENERATED_BODY()
};

class A3GAMEPLAYABLE_API IA3GameEntityFactory
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "A3Game|Runtime")
    AActor* SpawnRuntimeEntity(const FA3GameEntitySpawnRequest& Request);
};
