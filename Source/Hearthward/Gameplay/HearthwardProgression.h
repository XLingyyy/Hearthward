#pragma once
#include "HearthwardGameData.h"
namespace HearthwardProgression
{
inline int32 Level(int32 Experience)
{
    int32 Result=1;
    for(const auto& V:HearthwardData::Rows(TEXT("levels")))
        if(Experience>=HearthwardData::Number(V->AsObject(),TEXT("total_xp")))Result=HearthwardData::Number(V->AsObject(),TEXT("level"));
    return Result;
}
inline int32 Budget(int32 Level)
{ return HearthwardData::Number(HearthwardData::Find(TEXT("levels"),FString::FromInt(Level)),TEXT("skill_budget")); }
inline float Attribute(int32 Level,const TCHAR* Key)
{ return HearthwardData::Number(HearthwardData::Find(TEXT("levels"),FString::FromInt(Level)),Key); }
inline int32 MaximumExperience()
{ return HearthwardData::Number(HearthwardData::Catalog()->GetObjectField(TEXT("progression")),TEXT("maxExperience")); }
}
