#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HearthwardUpdateSubsystem.generated.h"

namespace HearthwardVersion
{
    inline const TCHAR* Current=TEXT("0.2.0-preview.20261007.4");
    inline const TCHAR* Releases=TEXT("https://github.com/XLingyyy/Hearthward/releases");
    bool IsNewer(const FString& Candidate,const FString& Installed);
}

UCLASS()
class HEARTHWARD_API UHearthwardUpdateSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    void Check(bool Retry=false);
    void CompleteCheck(int32 Code,const FString& Body,bool Success);
    void OpenReleasePage() const;
    FString GetNotice() const { return Notice; }
    bool HasUpdate() const { return bUpdate; }
    bool IsChecking() const { return bChecking; }
private:
    bool bChecked=false,bChecking=false,bUpdate=false;
    FString Notice=TEXT("正在检查正式发布版本…");
};
