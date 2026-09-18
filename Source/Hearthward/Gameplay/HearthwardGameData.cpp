#include "HearthwardGameData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

const TSharedPtr<FJsonObject>& HearthwardData::Catalog()
{
    static TSharedPtr<FJsonObject> Data = []
    {
        FString Source;
        TSharedPtr<FJsonObject> Result;
        const FString Path = FPaths::ProjectDir() / TEXT("Resources/Data/gameplay.json");
        if (!FFileHelper::LoadFileToString(Source, *Path))
            UE_LOG(LogTemp, Fatal, TEXT("Missing gameplay catalog: %s"), *Path);
        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source), Result) || !Result)
            UE_LOG(LogTemp, Fatal, TEXT("Invalid gameplay catalog: %s"), *Path);
        return Result;
    }();
    return Data;
}
const TArray<TSharedPtr<FJsonValue>>& HearthwardData::Rows(const FString& Table)
{
    return Catalog()->GetArrayField(Table);
}
TSharedPtr<FJsonObject> HearthwardData::Find(const FString& Table, const FString& Id)
{
    for (const auto& Value : Rows(Table))
        if (Value->AsObject()->GetStringField(TEXT("id")) == Id) return Value->AsObject();
    return nullptr;
}
double HearthwardData::Number(const TSharedPtr<FJsonObject>& Row, const FString& Key, double Default)
{
    double Value; return Row && Row->TryGetNumberField(Key, Value) ? Value : Default;
}
FString HearthwardData::Text(const TSharedPtr<FJsonObject>& Row, const FString& Key)
{
    FString Value; if (Row) Row->TryGetStringField(Key, Value); return Value;
}
FVector HearthwardData::Position(const TSharedPtr<FJsonObject>& Row)
{
    const auto& P = Row->GetArrayField(TEXT("position"));
    return FVector(P[0]->AsNumber(), P[1]->AsNumber(), P[2]->AsNumber());
}
