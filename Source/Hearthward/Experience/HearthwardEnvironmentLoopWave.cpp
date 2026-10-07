#include "HearthwardEnvironmentLoopWave.h"

UHearthwardEnvironmentLoopWave::UHearthwardEnvironmentLoopWave(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
    NumChannels=1;Duration=INDEFINITELY_LOOPING_DURATION;bLooping=true;
}
bool UHearthwardEnvironmentLoopWave::InitializePCM(const uint8* Data,int32 Bytes,int32 Rate)
{
    if(!PCM.IsEmpty() || !Data || Bytes<=0 || Bytes%2!=0 || Rate<=0)return false;
    PCM.Append(Data,Bytes);SetSampleRate(Rate);return true;
}
int32 UHearthwardEnvironmentLoopWave::OnGeneratePCMAudio(TArray<uint8>& OutAudio,int32 NumSamples)
{
    if(PCM.IsEmpty() || NumSamples<=0 || NumSamples>MAX_int32/2)return 0;
    const int32 Bytes=NumSamples*2;OutAudio.SetNumUninitialized(Bytes,EAllowShrinking::No);
    int32 Written=0;
    while(Written<Bytes)
    {
        const int32 Count=FMath::Min(Bytes-Written,PCM.Num()-Cursor);
        FMemory::Memcpy(OutAudio.GetData()+Written,PCM.GetData()+Cursor,Count);
        Written+=Count;Cursor=(Cursor+Count)%PCM.Num();
    }
    return NumSamples;
}
