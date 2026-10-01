#include "HearthwardUpdateSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformProcess.h"

namespace
{
bool Version(const FString& Input,TArray<int32>& Numbers,FString& Suffix)
{
    FString S=Input;S.RemoveFromStart(TEXT("v"));
    FString Core;if(!S.Split(TEXT("-"),&Core,&Suffix))Core=S;
    TArray<FString> Parts;Core.ParseIntoArray(Parts,TEXT("."));
    if(Parts.Num()!=3)return false;
    for(const auto& P:Parts)
    {
        if(P.IsEmpty() || P.Len()>8 || !P.IsNumeric() || P.Contains(TEXT("-")))return false;
        Numbers.Add(FCString::Atoi(*P));
    }
    return true;
}
}
bool HearthwardVersion::IsNewer(const FString& Candidate,const FString& Installed)
{
    TArray<int32> A,B;FString AS,BS;
    if(!Version(Candidate,A,AS) || !Version(Installed,B,BS))return false;
    for(int32 I=0;I<3;++I)if(A[I]!=B[I])return A[I]>B[I];
    if(AS.IsEmpty()!=BS.IsEmpty())return AS.IsEmpty();
    TArray<FString> AP,BP;AS.ParseIntoArray(AP,TEXT("."));BS.ParseIntoArray(BP,TEXT("."));
    for(int32 I=0;I<FMath::Min(AP.Num(),BP.Num());++I)
    {
        if(AP[I]==BP[I])continue;
        if(AP[I].IsNumeric() && BP[I].IsNumeric())return FCString::Atoi64(*AP[I])>FCString::Atoi64(*BP[I]);
        if(AP[I].IsNumeric()!=BP[I].IsNumeric())return !AP[I].IsNumeric();
        return AP[I]>BP[I];
    }
    return AP.Num()>BP.Num();
}
void UHearthwardUpdateSubsystem::Check(bool Retry)
{
    if(bChecking || (bChecked && !Retry))return;
    bChecked=true;bChecking=true;Notice=TEXT("正在检查正式发布版本…");
    auto Request=FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("https://api.github.com/repos/XLingyyy/Hearthward/releases/latest"));
    Request->SetVerb(TEXT("GET"));Request->SetTimeout(8.f);
    Request->SetHeader(TEXT("Accept"),TEXT("application/vnd.github+json"));
    Request->SetHeader(TEXT("User-Agent"),TEXT("Hearthward-UpdateCheck"));
    Request->OnProcessRequestComplete().BindWeakLambda(this,[this](FHttpRequestPtr,FHttpResponsePtr Response,bool Success)
    {
        CompleteCheck(Response.IsValid()?Response->GetResponseCode():0,Response.IsValid()?Response->GetContentAsString():FString(),Success);
    });
    if(!Request->ProcessRequest()){bChecking=false;Notice=TEXT("更新检查未能启动；可离线游玩或手动查看发布页。");}
}
void UHearthwardUpdateSubsystem::CompleteCheck(int32 Code,const FString& Body,bool Success)
{
        bChecking=false;
        if(Success && Code==404)
        {bUpdate=false;Notice=TEXT("尚无新的正式发布包。已有存档保留，可离线游玩。");return;}
        if(Success && (Code==403 || Code==429))
        {Notice=bUpdate?TEXT("存在新版本，请同步更新（检查服务暂时限流）"):TEXT("更新服务暂时限流，请稍后重试或手动打开发布页；可离线游玩。");return;}
        TSharedPtr<FJsonObject> Data;FString Tag;
        if(!Success || Code!=200
            || Body.Len()>1024*1024
            || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body),Data)
            || !Data || !Data->TryGetStringField(TEXT("tag_name"),Tag))
        {Notice=bUpdate?TEXT("存在新版本，请同步更新（本次网络检查失败）"):TEXT("暂时无法检查更新；可离线游玩，也可手动查看发布页。");return;}
        TArray<int32> Parsed;FString Suffix;
        if(!Version(Tag,Parsed,Suffix))
        {Notice=TEXT("发布版本号无法自动比较，请打开正式发布页核对；本地进度保留。");return;}
        bUpdate=HearthwardVersion::IsNewer(Tag,HearthwardVersion::Current);
        Notice=bUpdate?TEXT("存在新版本，请同步更新\n安装新版会保留兼容进度；冲突内容将另行列出并征求确认。")
            :TEXT("当前无需更新。存档在本机保留。");
}
void UHearthwardUpdateSubsystem::OpenReleasePage() const
{ FPlatformProcess::LaunchURL(HearthwardVersion::Releases,nullptr,nullptr); }
