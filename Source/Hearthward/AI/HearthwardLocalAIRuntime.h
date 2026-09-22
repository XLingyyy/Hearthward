#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IHttpRequest.h"

class FHearthwardLocalAIRuntime
{
public:
    ~FHearthwardLocalAIRuntime();

    bool EnsureStarted(FString& OutError);
    bool Tick(double Now,bool& bBecameReady,FString& OutError);
    void Stop();

    bool IsReady() const { return bReady; }
    uint32 GetProcessId() const { return ProcessId; }
    const FString& GetBaseUrl() const { return BaseUrl; }
    const FString& GetApiKey() const { return ApiKey; }
    const FString& GetBundlePath() const { return BundlePath; }

private:
    bool PollHealth(FString& OutError);

    FProcHandle Process;
    void* JobHandle=nullptr;
    uint32 ProcessId=0;
    FString BaseUrl;
    FString ApiKey;
    FString BundlePath;
    FHttpRequestPtr HealthRequest;
    bool bReady=false;
    bool bReadyTransition=false;
    double StartedAt=0;
    double NextHealthAt=0;
};
