#include "HearthwardNPCSuggestions.h"

namespace
{
FHearthwardNPCSuggestion Suggestion(FName Kind, const FString& Label, const FString& Message,
    FName Item = NAME_None, int32 ObservedCount = INDEX_NONE)
{
    FHearthwardNPCSuggestion Out;
    Out.Kind = Kind;
    Out.Label = Label;
    Out.Message = Message;
    Out.Item = Item;
    Out.ObservedCount = ObservedCount;
    return Out;
}
}

TArray<FHearthwardNPCSuggestion> HearthwardSuggestions::Build(const FHearthwardSuggestionContext& Context)
{
    TArray<FHearthwardNPCSuggestion> Out;

    if (Context.bHasActiveCommand)
    {
        Out.Add(Suggestion(TEXT("progress"),
            TEXT("问问当前委托进度"),
            TEXT("现在这个委托进行到哪一步了？")));
    }
    else if (Context.bCanCollectWood)
    {
        Out.Add(Suggestion(TEXT("collect"),
            TEXT("采集4份木材并送回营地"),
            TEXT("帮我采集4份木材并送回营地。"),
            TEXT("wood")));
    }
    else
    {
        Out.Add(Suggestion(TEXT("capabilities"),
            TEXT("看看现在能一起做什么"),
            TEXT("你现在能帮我做哪些事情？")));
    }

    if (Context.bHasCampWood)
    {
        Out.Add(Suggestion(TEXT("camp_stock"),
            FString::Printf(TEXT("营地木材：%d份"), Context.CampWood),
            FString::Printf(TEXT("营地现在有%d份木材。我们接下来需要补木材吗？"), Context.CampWood),
            TEXT("wood"), Context.CampWood));
    }
    else
    {
        Out.Add(Suggestion(TEXT("camp_status"),
            TEXT("聊聊营地现在的情况"),
            TEXT("我们聊聊营地现在的情况吧。")));
    }

    // Deliberately general: no hidden-map, enemy, quest-answer or economy-threshold knowledge.
    Out.Add(Suggestion(TEXT("capabilities"),
        TEXT("说说你现在能帮我什么"),
        TEXT("说说你现在能帮我什么，以及哪些事情现在还做不了。")));

    // Keep the GDD invariant even if future branches accidentally add more candidates.
    if (Out.Num() > 3) Out.SetNum(3);
    return Out;
}
