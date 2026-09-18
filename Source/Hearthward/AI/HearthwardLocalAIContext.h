#pragma once
#include "CoreMinimal.h"

struct FHearthwardAIProposal
{
    FString Intent;
    FName Item;
    int32 Quantity = 0;
    TArray<FName> Steps;
    FString Line;
};

// Intent hints select knowledge; only validated model proposals may reach the executor.
namespace HearthwardLocalAI
{
    FString ClassifyHint(const FString& Text);
    FString RetrieveKnowledge(const FString& Text, const FString& Hint, const FString& KnowledgeJson);
    FString ResponseSchema();
    bool ParseProposal(const FString& Json, FHearthwardAIProposal& Out);
}
