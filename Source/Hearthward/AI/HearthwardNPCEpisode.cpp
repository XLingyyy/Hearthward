#include "HearthwardNPCEpisode.h"
#include "HearthwardAgentContract.h"

TArray<FHearthwardNPCEpisode> HearthwardEpisodes::Build(const TArray<FHearthwardNPCEvent>& Events,int32 MaxEpisodes)
{
    TArray<FHearthwardNPCEpisode> Out;
    TMap<FGuid,int32> ByCommand;
    for(const auto& E:Events)
    {
        if(E.Kind==TEXT("directive"))continue;
        if(!E.Command.IsValid())continue;
        int32* Existing=ByCommand.Find(E.Command);
        int32 Index=INDEX_NONE;
        if(Existing)Index=*Existing;
        else
        {
            Index=Out.AddDefaulted();
            Out[Index].Command=E.Command;
            ByCommand.Add(E.Command,Index);
        }
        auto& X=Out[Index];
        if(!E.Item.IsNone())X.Item=E.Item;
        X.LastAt=FMath::Max(X.LastAt,E.At);
        X.Evidence.AddUnique(E.Id);
        if(E.Kind==TEXT("acquired"))X.Acquired+=E.Count;
        else if(E.Kind==TEXT("delivered"))X.Delivered+=E.Count;
        else if(E.Kind==TEXT("craft"))X.Crafted+=E.Count;
        else if(E.Kind==TEXT("repair"))X.Repaired+=E.Count;
        else if(E.Kind==TEXT("replanned")){++X.Replans;if(!E.Reason.IsEmpty())X.Reasons.AddUnique(E.Reason);}
        else if(E.Kind==TEXT("blocked")){if(!E.Reason.IsEmpty())X.Reasons.AddUnique(E.Reason);}
        else if(E.Kind==TEXT("completed"))X.Completed=true;
        else if(E.Kind==TEXT("cancelled"))X.Cancelled=true;
    }
    Out.StableSort([](const auto& A,const auto& B){return A.LastAt>B.LastAt;});
    if(MaxEpisodes>=0 && Out.Num()>MaxEpisodes)Out.SetNum(MaxEpisodes);
    return Out;
}

FString HearthwardEpisodes::Describe(const FHearthwardNPCEpisode& E)
{
    if(!E.HasEvidence())return {};
    TArray<FString> Parts;
    Parts.Add(FString::Printf(TEXT("任务[%s] %s"),*E.Command.ToString().Left(8),*HearthwardAgent::ItemText(E.Item)));
    if(E.Acquired>0)Parts.Add(FString::Printf(TEXT("实际取得%d"),E.Acquired));
    if(E.Delivered>0)Parts.Add(FString::Printf(TEXT("实际入库%d"),E.Delivered));
    if(E.Crafted>0)Parts.Add(FString::Printf(TEXT("实际制作%d"),E.Crafted));
    if(E.Repaired>0)Parts.Add(FString::Printf(TEXT("实际维修%d"),E.Repaired));
    if(E.Replans>0)Parts.Add(FString::Printf(TEXT("重规划%d次"),E.Replans));
    if(!E.Reasons.IsEmpty())Parts.Add(TEXT("原因：")+FString::Join(E.Reasons,TEXT("、")));
    if(E.Completed)Parts.Add(TEXT("已完成"));
    else if(E.Cancelled)Parts.Add(TEXT("已取消"));
    else Parts.Add(TEXT("未完成"));
    TArray<FString> Evidence;for(const auto& Id:E.Evidence)Evidence.Add(Id.ToString().Left(8));
    Parts.Add(TEXT("证据：")+FString::Join(Evidence,TEXT(",")));
    return FString::Join(Parts,TEXT("；"));
}
