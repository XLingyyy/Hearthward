#include "HearthwardLocalAIContext.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace HearthwardLocalAI
{
FString ClassifyHint(const FString& Text)
{
    TArray<FString> Hints;
    if (Text.Contains(TEXT("采")) || Text.Contains(TEXT("收集")) || Text.Contains(TEXT("木材")) || Text.Contains(TEXT("石材"))) Hints.Add(TEXT("gather"));
    if (Text.Contains(TEXT("取消")) || Text.Contains(TEXT("停下")) || Text.Contains(TEXT("别再"))) Hints.Add(TEXT("cancel"));
    if (Text.Contains(TEXT("仓库")) || Text.Contains(TEXT("库存")) || Text.Contains(TEXT("多少"))) Hints.Add(TEXT("inventory"));
    if (Hints.IsEmpty() && (Text.Contains(TEXT("准备")) || Text.Contains(TEXT("替我")) || Text.Contains(TEXT("帮我")) || Text.Contains(TEXT("弄"))))
        return TEXT("request_unspecified");
    return Hints.Num() == 1 ? Hints[0] : TEXT("unknown");
}

FString RetrieveKnowledge(const FString& Text, const FString& Hint, const FString& KnowledgeJson)
{
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(KnowledgeJson), Root)) return {};
    const TArray<TSharedPtr<FJsonValue>>* Chunks = nullptr;
    if (!Root->TryGetArrayField(TEXT("chunks"), Chunks)) return {};
    struct FMatch { int32 Score; FString Id, Text; };
    TArray<FMatch> Matches;
    for (const auto& Value : *Chunks)
    {
        const auto Object = Value->AsObject();
        if (!Object.IsValid() || Object->GetStringField(TEXT("visibility")) != TEXT("initial_known")) continue;
        int32 Score = Object->GetStringField(TEXT("topic")) == Hint ? 3 : 0;
        for (const auto& Term : Object->GetArrayField(TEXT("terms"))) if (Text.Contains(Term->AsString())) ++Score;
        if (Score > 0) Matches.Add({Score, Object->GetStringField(TEXT("id")), Object->GetStringField(TEXT("text"))});
    }
    Matches.StableSort([](const FMatch& A, const FMatch& B) { return A.Score > B.Score; });
    FString Result;
    for (int32 I = 0; I < FMath::Min(3, Matches.Num()); ++I) Result += FString::Printf(TEXT("[%s] %s\n"), *Matches[I].Id, *Matches[I].Text);
    return Result;
}

FString ResponseSchema()
{
    return TEXT(R"({"oneOf":[{"type":"object","additionalProperties":false,"properties":{"intent":{"const":"collect"},"item":{"type":"string","enum":["wood","stone","ore","meat","arrow"]},"quantity":{"type":"integer","minimum":1,"maximum":2147483647},"steps":{"const":["collect","return","deposit"]},"npc_line":{"type":"string","minLength":1,"maxLength":180}},"required":["intent","item","quantity","steps","npc_line"]},{"type":"object","additionalProperties":false,"properties":{"intent":{"type":"string","enum":["cancel","clarify","dialogue","refuse"]},"item":{"const":"none"},"quantity":{"const":0},"steps":{"const":[]},"npc_line":{"type":"string","minLength":1,"maxLength":180}},"required":["intent","item","quantity","steps","npc_line"]}]})");
}

bool ParseProposal(const FString& Json, FHearthwardAIProposal& Out)
{
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object) || !Object.IsValid() || Object->Values.Num() != 5) return false;
    FString Intent, Item, Line;
    double Quantity;
    const TArray<TSharedPtr<FJsonValue>>* Steps = nullptr;
    if (!Object->TryGetStringField(TEXT("intent"), Intent) || !Object->TryGetStringField(TEXT("item"), Item)
        || !Object->TryGetStringField(TEXT("npc_line"), Line) || !Object->TryGetNumberField(TEXT("quantity"), Quantity)
        || !Object->TryGetArrayField(TEXT("steps"), Steps)) return false;
    if (!FMath::IsFinite(Quantity) || Quantity < 0 || Quantity > MAX_int32 || FMath::FloorToDouble(Quantity) != Quantity
        || Line.IsEmpty() || Line.Len() > 180 || Steps->Num() > 3) return false;
    const TArray<FString> Intents = {TEXT("collect"), TEXT("cancel"), TEXT("clarify"), TEXT("dialogue"), TEXT("refuse")};
    const TArray<FString> Items = {TEXT("wood"), TEXT("stone"), TEXT("ore"), TEXT("meat"), TEXT("arrow"), TEXT("none")};
    if (!Intents.Contains(Intent) || !Items.Contains(Item)) return false;
    TArray<FName> Plan;
    for (const auto& Value : *Steps)
    {
        FString Step;
        if (!Value->TryGetString(Step)) return false;
        Plan.Add(FName(*Step));
    }
    const TArray<FName> Required = {TEXT("collect"), TEXT("return"), TEXT("deposit")};
    if (Intent == TEXT("collect"))
    {
        if (Item == TEXT("none") || Quantity == 0 || Plan != Required) return false;
    }
    else if (Item != TEXT("none") || Quantity != 0 || !Plan.IsEmpty()) return false;
    Out = {Intent, FName(*Item), static_cast<int32>(Quantity), Plan, Line};
    return true;
}
}
