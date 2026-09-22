#include "HearthwardAgentPlan.h"

namespace
{
FHearthwardAgentAction Action(EHearthwardAgentActionType Type, EHearthwardAgentTarget Target)
{
    FHearthwardAgentAction Out;
    Out.Type = Type;
    Out.Target = Target;
    return Out;
}
}

bool HearthwardPlan::Build(const FHearthwardAgentGoal& Goal, FHearthwardAgentPlan& Out, FString& Error)
{
    Out = {};
    Error = HearthwardAgent::Validate(Goal);
    if (!Error.IsEmpty()) return false;

    if (Goal.Intent == TEXT("collect"))
    {
        Out.Actions = {
            Action(EHearthwardAgentActionType::MoveTo, EHearthwardAgentTarget::Source),
            Action(EHearthwardAgentActionType::Gather, EHearthwardAgentTarget::Source),
            Action(EHearthwardAgentActionType::MoveTo, EHearthwardAgentTarget::Camp),
            Action(EHearthwardAgentActionType::Deposit, EHearthwardAgentTarget::Camp)
        };
        return true;
    }

    if (Goal.Intent == TEXT("craft"))
    {
        if (Goal.SourceRef == TEXT("camp"))
        {
            Out.Actions = {
                Action(EHearthwardAgentActionType::MoveTo, EHearthwardAgentTarget::Camp),
                Action(EHearthwardAgentActionType::TakeMaterials, EHearthwardAgentTarget::Camp),
                Action(EHearthwardAgentActionType::MoveTo, EHearthwardAgentTarget::Workshop),
                Action(EHearthwardAgentActionType::CommitWorkshop, EHearthwardAgentTarget::Workshop),
                Action(EHearthwardAgentActionType::MoveTo, EHearthwardAgentTarget::Camp),
                Action(EHearthwardAgentActionType::Deposit, EHearthwardAgentTarget::Camp)
            };
        }
        else
        {
            Out.Actions = {
                Action(EHearthwardAgentActionType::MoveTo, EHearthwardAgentTarget::Workshop),
                Action(EHearthwardAgentActionType::CommitWorkshop, EHearthwardAgentTarget::Workshop),
                Action(EHearthwardAgentActionType::MoveTo, EHearthwardAgentTarget::Camp),
                Action(EHearthwardAgentActionType::Deposit, EHearthwardAgentTarget::Camp)
            };
        }
        return true;
    }

    if (Goal.Intent == TEXT("repair"))
    {
        if (Goal.SourceRef == TEXT("camp"))
        {
            Out.Actions = {
                Action(EHearthwardAgentActionType::MoveTo, EHearthwardAgentTarget::Camp),
                Action(EHearthwardAgentActionType::TakeMaterials, EHearthwardAgentTarget::Camp),
                Action(EHearthwardAgentActionType::MoveTo, EHearthwardAgentTarget::Workshop),
                Action(EHearthwardAgentActionType::CommitWorkshop, EHearthwardAgentTarget::Workshop)
            };
        }
        else
        {
            Out.Actions = {
                Action(EHearthwardAgentActionType::MoveTo, EHearthwardAgentTarget::Workshop),
                Action(EHearthwardAgentActionType::CommitWorkshop, EHearthwardAgentTarget::Workshop)
            };
        }
        return true;
    }

    Error = TEXT("NO_EXECUTABLE_PLAN");
    Out = {};
    return false;
}

int32 HearthwardPlan::Find(const FHearthwardAgentPlan& Plan, EHearthwardAgentActionType Type,
    EHearthwardAgentTarget Target, int32 StartAt)
{
    for (int32 Index = FMath::Max(0, StartAt); Index < Plan.Actions.Num(); ++Index)
        if (Plan.Actions[Index].Matches(Type, Target)) return Index;
    return INDEX_NONE;
}

int32 HearthwardPlan::FindLast(const FHearthwardAgentPlan& Plan, EHearthwardAgentActionType Type,
    EHearthwardAgentTarget Target)
{
    for (int32 Index = Plan.Actions.Num() - 1; Index >= 0; --Index)
        if (Plan.Actions[Index].Matches(Type, Target)) return Index;
    return INDEX_NONE;
}

FString HearthwardPlan::ActionName(const FHearthwardAgentAction& Action)
{
    const TCHAR* Type = TEXT("MoveTo");
    switch (Action.Type)
    {
    case EHearthwardAgentActionType::MoveTo: Type = TEXT("MoveTo"); break;
    case EHearthwardAgentActionType::Gather: Type = TEXT("Gather"); break;
    case EHearthwardAgentActionType::TakeMaterials: Type = TEXT("TakeMaterials"); break;
    case EHearthwardAgentActionType::CommitWorkshop: Type = TEXT("CommitWorkshop"); break;
    case EHearthwardAgentActionType::Deposit: Type = TEXT("Deposit"); break;
    }

    const TCHAR* Target = TEXT("None");
    switch (Action.Target)
    {
    case EHearthwardAgentTarget::Source: Target = TEXT("Source"); break;
    case EHearthwardAgentTarget::Camp: Target = TEXT("Camp"); break;
    case EHearthwardAgentTarget::Workshop: Target = TEXT("Workshop"); break;
    default: break;
    }
    return FString::Printf(TEXT("%s:%s"), Type, Target);
}
