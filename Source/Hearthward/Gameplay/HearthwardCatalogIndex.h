#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// Internal acceleration for the process-lifetime gameplay catalog, not world/save state.
// Table membership and row IDs must remain immutable after construction, just as the
// production Catalog() is loaded once. Row objects are shared, never copied or rewritten.
// Keep FString keys: changing them to FName would change the legacy comparison contract.
class FHearthwardCatalogIndex
{
public:
    explicit FHearthwardCatalogIndex(const TSharedPtr<FJsonObject>& Root)
    {
        if (!Root.IsValid()) return;
        Tables.Reserve(Root->Values.Num());
        for (const auto& Field : Root->Values)
        {
            if (!Field.Value.IsValid() || Field.Value->Type != EJson::Array) continue;
            const auto& Values = Field.Value->AsArray();
            FRows ById;
            ById.Reserve(Values.Num());
            bool Indexable = true;
            for (const auto& Value : Values)
            {
                if (!Value.IsValid() || Value->Type != EJson::Object)
                {
                    Indexable = false;
                    break;
                }
                const auto Row = Value->AsObject();
                FString Id;
                if (!Row.IsValid() || !Row->TryGetStringField(TEXT("id"), Id))
                {
                    Indexable = false;
                    break;
                }
                // The linear implementation returns the FIRST matching row.
                // TMap::Add would otherwise replace an earlier duplicate.
                if (!ById.Contains(Id)) ById.Add(Id, Row);
            }
            // Leave non-ID and malformed arrays to the caller's original code path;
            // do not turn an invalid catalog into a silently valid cache miss.
            if (Indexable) Tables.Add(FString(Field.Key), MoveTemp(ById));
        }
    }

    // true = table indexed (Out may be null on a normal miss).
    // false = caller must use its original lookup/error path.
    bool TryFind(const FString& Table, const FString& Id, TSharedPtr<FJsonObject>& Out) const
    {
        Out.Reset();
        const FRows* Rows = Tables.Find(Table);
        if (!Rows) return false;
        if (const auto* Row = Rows->Find(Id)) Out = *Row;
        return true;
    }

private:
    using FRows = TMap<FString, TSharedPtr<FJsonObject>>;
    TMap<FString, FRows> Tables;
};
