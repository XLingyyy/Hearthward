#include "HearthwardLocalAIRuntime.h"

#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

FHearthwardLocalAIRuntime::~FHearthwardLocalAIRuntime()
{
    Stop();
}

bool FHearthwardLocalAIRuntime::EnsureStarted(FString& OutError)
{
    OutError.Reset();
    if(Process.IsValid())
    {
        if(FPlatformProcess::IsProcRunning(Process))return true;
        Stop();
    }

    BundlePath=FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(),TEXT("Runtime/LocalAI")));
#if !UE_BUILD_SHIPPING
    FString BundleOverride;
    if(FParse::Value(FCommandLine::Get(),TEXT("HearthwardAIBundlePath="),BundleOverride)
        && !BundleOverride.TrimStartAndEnd().IsEmpty())
        BundlePath=FPaths::ConvertRelativePathToFull(BundleOverride.TrimQuotes());
#endif

    FString Backend=TEXT("cpu");
    int32 Layers=16;
    GConfig->GetString(TEXT("Hearthward.LocalAI"),TEXT("Backend"),Backend,GGameIni);
    GConfig->GetInt(TEXT("Hearthward.LocalAI"),TEXT("GpuLayers"),Layers,GGameIni);
    FParse::Value(FCommandLine::Get(),TEXT("HearthwardAIBackend="),Backend);
    FParse::Value(FCommandLine::Get(),TEXT("HearthwardAIGpuLayers="),Layers);
    if(Backend!=TEXT("cpu") && Backend!=TEXT("vulkan"))
    {
        OutError=TEXT("本地模型后端配置无效");
        return false;
    }

    const FString BinDir=FPaths::Combine(BundlePath,TEXT("bin"),Backend);
    const FString Exe=FPaths::Combine(BinDir,TEXT("llama-server.exe"));
    const FString Model=FPaths::Combine(BundlePath,TEXT("models/Qwen3.5-4B-Q4_K_M.gguf"));
    if(!IFileManager::Get().FileExists(*Exe) || !IFileManager::Get().FileExists(*Model))
    {
        OutError=TEXT("本地模型文件缺失，请修复游戏安装");
        return false;
    }

    auto* Sockets=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    FSocket* Socket=Sockets->CreateSocket(NAME_Stream,TEXT("Hearthward local AI port"),false);
    auto Address=Sockets->CreateInternetAddr();
    bool Valid=false;
    Address->SetIp(TEXT("127.0.0.1"),Valid);
    Address->SetPort(0);
    const bool Bound=Socket && Valid && Socket->Bind(*Address);
    const int32 Port=Bound?Socket->GetPortNo():0;
    if(Socket)Sockets->DestroySocket(Socket);
    if(Port==0)
    {
        OutError=TEXT("无法分配本机推理端口");
        return false;
    }

    ApiKey=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    BaseUrl=FString::Printf(TEXT("http://127.0.0.1:%d"),Port);
    const FString Args=FString::Printf(
        TEXT("-m \"%s\" --host 127.0.0.1 --port %d --api-key %s --alias hearthward-qwen-local -c 4096 -np 1 -t 4 -tb 4 -ngl %d --reasoning off --jinja --no-webui --no-cache-prompt"),
        *Model,Port,*ApiKey,Backend==TEXT("cpu")?0:FMath::Clamp(Layers,0,32));

    Process=FPlatformProcess::CreateProc(*Exe,*Args,true,true,true,&ProcessId,0,*BinDir,nullptr);
    if(!Process.IsValid())
    {
        OutError=TEXT("本地推理进程启动失败");
        return false;
    }

#if PLATFORM_WINDOWS
    JobHandle=CreateJobObjectW(nullptr,nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION Limits={};
    Limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(!JobHandle || !SetInformationJobObject(JobHandle,JobObjectExtendedLimitInformation,&Limits,sizeof(Limits))
        || !AssignProcessToJobObject(JobHandle,Process.Get()))
    {
        Stop();
        OutError=TEXT("无法建立本地推理进程生命周期");
        return false;
    }
#endif

    bReady=false;
    bReadyTransition=false;
    UE_LOG(LogTemp,Display,TEXT("Local AI runtime started pid=%u backend=%s"),ProcessId,*Backend);
    StartedAt=FPlatformTime::Seconds();
    NextHealthAt=StartedAt;
    return true;
}

void FHearthwardLocalAIRuntime::Stop()
{
    if(HealthRequest.IsValid())
    {
        HealthRequest->CancelRequest();
        HealthRequest.Reset();
    }
    if(Process.IsValid())
    {
        FPlatformProcess::TerminateProc(Process,true);
        FPlatformProcess::CloseProc(Process);
        Process.Reset();
    }
#if PLATFORM_WINDOWS
    if(JobHandle)CloseHandle(JobHandle);
#endif
    JobHandle=nullptr;
    ProcessId=0;
    bReady=false;
    bReadyTransition=false;
    StartedAt=0;
    NextHealthAt=0;
    BaseUrl.Reset();
    ApiKey.Reset();
}

bool FHearthwardLocalAIRuntime::PollHealth(FString& OutError)
{
    HealthRequest=FHttpModule::Get().CreateRequest();
    HealthRequest->SetURL(BaseUrl+TEXT("/v1/models"));
    HealthRequest->SetVerb(TEXT("GET"));
    HealthRequest->SetHeader(TEXT("Authorization"),TEXT("Bearer ")+ApiKey);
    HealthRequest->SetTimeout(3);
    const uint32 ExpectedProcess=ProcessId;
    HealthRequest->OnProcessRequestComplete().BindLambda([this,ExpectedProcess](FHttpRequestPtr,FHttpResponsePtr Response,bool Success)
    {
        if(ExpectedProcess!=ProcessId)return;
        HealthRequest.Reset();
        if(Success && Response.IsValid() && Response->GetResponseCode()==200)
        {
            bReady=true;
            bReadyTransition=true;
            UE_LOG(LogTemp,Display,TEXT("Local AI runtime ready pid=%u"),ProcessId);
        }
    });
    if(!HealthRequest->ProcessRequest())
    {
        HealthRequest.Reset();
        OutError=TEXT("本机推理连接失败");
        return false;
    }
    return true;
}

bool FHearthwardLocalAIRuntime::Tick(double Now,bool& bBecameReady,FString& OutError)
{
    bBecameReady=false;
    OutError.Reset();

    if(Process.IsValid() && !FPlatformProcess::IsProcRunning(Process))
    {
        Stop();
        OutError=TEXT("本地模型进程已退出，请检查运行配置");
        return false;
    }

    if(Process.IsValid() && !bReady)
    {
        if(Now-StartedAt>120)
        {
            Stop();
            OutError=TEXT("本地模型加载超时");
            return false;
        }
        if(!HealthRequest.IsValid() && Now>=NextHealthAt)
        {
            NextHealthAt=Now+0.5;
            if(!PollHealth(OutError))return false;
        }
    }

    if(bReadyTransition)
    {
        bReadyTransition=false;
        bBecameReady=true;
    }
    return true;
}
