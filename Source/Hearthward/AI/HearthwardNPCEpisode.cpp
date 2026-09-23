#include "HearthwardNPCEpisode.h"
#include "HearthwardAgentContract.h"

namespace
{
TArray<FHearthwardNPCEpisode> BuildInternal(const TArray<FHearthwardNPCEvent>& Events,
    TFunctionRef<EHearthwardNPCEpisodeCoverage(FGuid)> ResolveCoverage,int32 MaxEpisodes)
{
    TArray<FHearthwardNPCEpisode> Out;
    TMap<FGuid,int32> ByCommand;
    for(const auto& E:Events)
    {
        if(E.Kind==TEXT("directive") || !E.Command.IsValid()) continue;
        int32* Existing=ByCommand.Find(E.Command);
        int32 Index=INDEX_NONE;
        if(Existing)Index=*Existing;
        else
        {
            Index=Out.AddDefaulted();
            Out[Index].Command=E.Command;
            Out[Index].Coverage=ResolveCoverage(E.Command);
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
}

TArray<FHearthwardNPCEpisode> HearthwardEpisodes::Build(const FHearthwardNPCMemory& Memory,int32 MaxEpisodes)
{
    return BuildInternal(Memory.Events,[&](FGuid Command){return Memory.CoverageFor(Command);},MaxEpisodes);
}

TArray<FHearthwardNPCEpisode> HearthwardEpisodes::Build(const TArray<FHearthwardNPCEvent>& Events,int32 MaxEpisodes)
{
    return BuildInternal(Events,[](FGuid){return EHearthwardNPCEpisodeCoverage::Unknown;},MaxEpisodes);
}

FString HearthwardEpisodes::CoverageName(EHearthwardNPCEpisodeCoverage Coverage)
{
    switch(Coverage)
    {
    case EHearthwardNPCEpisodeCoverage::Complete:return TEXT("complete");
    case EHearthwardNPCEpisodeCoverage::Truncated:return TEXT("truncated");
    default:return TEXT("unknown");
    }
}

FString HearthwardEpisodes::Describe(const FHearthwardNPCEpisode& E)
{
    if(!E.HasEvidence())return {};
    TArray<FString> Parts;
    Parts.Add(FString::Printf(TEXT("任务[%s] %s"),*E.Command.ToString().Left(8),*HearthwardAgent::ItemText(E.Item)));
    const FString Prefix=E.IsCompleteCoverage()?TEXT("实际"):TEXT("保留记录中可确认实际");
    if(E.Acquired>0)Parts.Add(FString::Printf(TEXT("%s取得%d"),*Prefix,E.Acquired));
    if(E.Delivered>0)Parts.Add(FString::Printf(TEXT("%s入库%d"),*Prefix,E.Delivered));
    if(E.Crafted>0)Parts.Add(FString::Printf(TEXT("%s制作%d"),*Prefix,E.Crafted));
    if(E.Repaired>0)Parts.Add(FString::Printf(TEXT("%s维修%d"),*Prefix,E.Repaired));
    if(E.Replans>0)Parts.Add(FString::Printf(TEXT("%s重规划%d次"),*Prefix,E.Replans));
    if(!E.Reasons.IsEmpty())Parts.Add(TEXT("原因：")+FString::Join(E.Reasons,TEXT("、")));
    if(E.Completed)Parts.Add(TEXT("已完成"));
    else if(E.Cancelled)Parts.Add(TEXT("已取消"));
    else Parts.Add(TEXT("未完成"));

    if(E.Coverage==EHearthwardNPCEpisodeCoverage::Complete) Parts.Add(TEXT("记录覆盖完整"));
    else if(E.Coverage==EHearthwardNPCEpisodeCoverage::Truncated) Parts.Add(TEXT("记录已截断，不能据此确认全部过程或总量"));
    else Parts.Add(TEXT("记录覆盖未知，不能据此确认全部过程或总量"));

    TArray<FString> Evidence;for(const auto& Id:E.Evidence)Evidence.Add(Id.ToString().Left(8));
    Parts.Add(TEXT("证据：")+FString::Join(Evidence,TEXT(",")));
    return FString::Join(Parts,TEXT("；"));
}
