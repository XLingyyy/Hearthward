#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

namespace HearthwardData
{
    HEARTHWARD_API const TSharedPtr<FJsonObject>& Catalog();
    HEARTHWARD_API const TArray<TSharedPtr<FJsonValue>>& Rows(const FString& Table);
    HEARTHWARD_API TSharedPtr<FJsonObject> Find(const FString& Table, const FString& Id);
    HEARTHWARD_API double Number(const TSharedPtr<FJsonObject>& Row, const FString& Key, double Default = 0);
    HEARTHWARD_API FString Text(const TSharedPtr<FJsonObject>& Row, const FString& Key);
    HEARTHWARD_API FVector Position(const TSharedPtr<FJsonObject>& Row);
}
