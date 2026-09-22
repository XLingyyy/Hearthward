#include "HearthwardNPCInitiative.h"
#include "HearthwardAgentContract.h"

namespace
{
FHearthwardNPCInitiative Make(FName Kind,FName Item,const FString& Message,const FString& Key,FGuid Evidence,double Now)
{
    FHearthwardNPCInitiative Out;
    Out.Id=FGuid::NewGuid();Out.Kind=Kind;Out.Item=Item;Out.Message=Message;Out.DedupeKey=Key;
    Out.EvidenceId=Evidence;Out.CreatedAt=Now;return Out;
}
}

FHearthwardNPCInitiative HearthwardInitiative::FromEvent(FName Kind,FName Item,int32 Count,const FString& Reason,FGuid EvidenceId,double Now)
{
    const FString ItemName=HearthwardAgent::ItemText(Item);
    if(Kind==TEXT("completed"))
        return Make(TEXT("task_completed"),Item,
            FString::Printf(TEXT("%s已经处理好了，实际完成 %d。"),*ItemName,Count),
            TEXT("event:")+EvidenceId.ToString(),EvidenceId,Now);

    if(Kind==TEXT("replanned"))
        return Make(TEXT("task_replanned"),Item,
            FString::Printf(TEXT("%s那边情况变了，我已经重新调整路线继续做。"),*ItemName),
            TEXT("event:")+EvidenceId.ToString(),EvidenceId,Now);

    if(Kind==TEXT("blocked"))
        return Make(TEXT("task_blocked"),Item,
            Reason.IsEmpty()?FString::Printf(TEXT("%s这件事现在卡住了，我先停在安全状态。"),*ItemName)
                :FString::Printf(TEXT("%s这件事现在卡住了：%s。"),*ItemName,*Reason),
            TEXT("event:")+EvidenceId.ToString(),EvidenceId,Now);

    return {};
}

FHearthwardNPCInitiative HearthwardInitiative::BeliefCorrection(FName Item,int32 Reported,int32 Confirmed,double Now)
{
    if(Reported==Confirmed)return {};
    const FString ItemName=HearthwardAgent::ItemText(Item);
    return Make(TEXT("belief_corrected"),Item,
        FString::Printf(TEXT("我刚回营亲自确认了：%s实际是 %d 份，不是之前报告的 %d 份。"),*ItemName,Confirmed,Reported),
        FString::Printf(TEXT("belief:%s:%d:%d"),*Item.ToString(),Reported,Confirmed),{},Now);
}
