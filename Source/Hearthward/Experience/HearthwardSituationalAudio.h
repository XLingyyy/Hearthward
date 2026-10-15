#pragma once
#include "CoreMinimal.h"
namespace HearthwardSituationalAudio
{
enum class EContext:uint8 { Outdoor,Indoor,Combat,Camp };
inline EContext Context(bool Combat,bool Sheltered,bool InCamp)
{return Combat?EContext::Combat:Sheltered?EContext::Indoor:InCamp?EContext::Camp:EContext::Outdoor;}
inline float EnvironmentGain(EContext Context,bool Dialogue,bool ImportantResult)
{
    const float Gain=Context==EContext::Indoor?.30f:Context==EContext::Combat?.58f:Context==EContext::Camp?.82f:1.f;
    return Gain*(Dialogue?.55f:ImportantResult?.65f:1.f);
}
}
