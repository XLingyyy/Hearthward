#pragma once
#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include "HearthwardEnvironmentLoopWave.generated.h"

UCLASS()
class HEARTHWARD_API UHearthwardEnvironmentLoopWave : public USoundWaveProcedural
{
    GENERATED_BODY()
public:
    UHearthwardEnvironmentLoopWave(const FObjectInitializer& ObjectInitializer);
    bool InitializePCM(const uint8* Data,int32 Bytes,int32 Rate);
    virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio,int32 NumSamples) override;
private:
    // Initialized before Play; only the audio render thread advances the cursor.
    TArray<uint8> PCM;
    int32 Cursor=0;
};
