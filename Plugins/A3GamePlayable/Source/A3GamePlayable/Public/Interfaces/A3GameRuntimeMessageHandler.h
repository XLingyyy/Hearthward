#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "A3GameRuntimeMessageHandler.generated.h"

UINTERFACE(BlueprintType)
class A3GAMEPLAYABLE_API UA3GameRuntimeMessageHandler : public UInterface
{
    GENERATED_BODY()
};

class A3GAMEPLAYABLE_API IA3GameRuntimeMessageHandler
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "A3Game|Runtime")
    bool HandleRuntimeMessage(
        const FString& MessageType,
        const FString& JsonPayload);
};
