#include "HearthwardNPCCoordination.h"

namespace
{
bool IsDirective(FName Item)
{
    return Item==TEXT("hold") || Item==TEXT("follow") || Item==TEXT("assist");
}
}

FHearthwardNPCCoordinationProfile HearthwardCoordination::Build(const TArray<FHearthwardNPCEvent>& Events,int32 Window)
{
    FHearthwardNPCCoordinationProfile Out;
    const int32 Limit=FMath::Max(1,Window);
    for(int32 I=Events.Num()-1;I>=0 && Out.Samples<Limit;--I)
    {
        const auto& E=Events[I];
        if(E.Kind!=TEXT("directive") || !IsDirective(E.Item))continue;
        ++Out.Samples;
        Out.LastObservedAt=FMath::Max(Out.LastObservedAt,E.At);
        if(E.Item==TEXT("hold"))++Out.HoldCount;
        else if(E.Item==TEXT("follow"))++Out.FollowCount;
        else if(E.Item==TEXT("assist"))++Out.AssistCount;
    }

    struct FCount{FName Id;int32 Count;};
    TArray<FCount> Counts={{TEXT("hold"),Out.HoldCount},{TEXT("follow"),Out.FollowCount},{TEXT("assist"),Out.AssistCount}};
    Counts.Sort([](const FCount& A,const FCount& B)
    {
        if(A.Count!=B.Count)return A.Count>B.Count;
        return A.Id.ToString()<B.Id.ToString();
    });
    if(Out.Samples>0)
    {
        Out.PreferredDirective=Counts[0].Id;
        Out.Confidence=float(Counts[0].Count)/float(Out.Samples);
    }
    const int32 Runner=Counts.Num()>1?Counts[1].Count:0;
    Out.Stable=Out.Samples>=3 && Counts[0].Count>=3 && Out.Confidence>=0.67f && Counts[0].Count-Runner>=2;
    if(!Out.Stable)Out.PreferredDirective=NAME_None;
    return Out;
}

FString HearthwardCoordination::Describe(const FHearthwardNPCCoordinationProfile& P)
{
    if(!P.Stable)return TEXT("近期协作方式还没有形成稳定倾向。");
    const FString Name=P.PreferredDirective==TEXT("follow")?TEXT("跟随")
        :P.PreferredDirective==TEXT("assist")?TEXT("协助战斗"):TEXT("原地等待");
    return FString::Printf(TEXT("最近%d次已确认协作指令中，%s占比约%.0f%%；这是行为观察，不是玩家显式偏好。"),
        P.Samples,*Name,P.Confidence*100.0f);
}
