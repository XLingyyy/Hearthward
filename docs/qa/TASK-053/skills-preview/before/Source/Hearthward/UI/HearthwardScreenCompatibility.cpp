#include "HearthwardScreenWidget.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Update/HearthwardUpdateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UHearthwardScreenWidget::ComposeUpdateNotice()
{
    const auto* Update=GetGameInstance()->GetSubsystem<UHearthwardUpdateSubsystem>();
    const auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    Element(TEXT("panel"),TEXT(""),{640,550},{890,315});
    Element(TEXT("text"),TEXT("版本与本地进度"),{680,575},{805,38},25);
    Element(TEXT("text"),TEXT("当前版本 ")+FString(HearthwardVersion::Current),{680,619},{805,32},17);
    Element(TEXT("text"),Update->GetNotice(),{680,660},{805,83},18);
    Element(TEXT("button"),Update->HasUpdate()?TEXT("同步更新（GitHub）"):TEXT("正式发布页"),{670,746},{285,46},19,TEXT("update.open"));
    Element(TEXT("button"),TEXT("重新检查"),{1010,746},{210,46},19,TEXT("update.check"));Elements.Last().Enabled=!Update->IsChecking();
    if(Save->HasSaveConflicts())Element(TEXT("button"),TEXT("查看存档兼容冲突"),{670,805},{440,42},19,TEXT("compat.show"));
    else Element(TEXT("text"),TEXT("旧档原件保留；新版进度独立保存，旧程序不会覆盖新版进度。"),{680,810},{805,40},16);
}
void UHearthwardScreenWidget::ComposeCompatibility()
{
    const auto& Report=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetCompatibility();
    Element(TEXT("panel"),TEXT(""),{260,115},{1152,720});
    Element(TEXT("text"),TEXT("存档兼容检查"),{305,145},{1050,45},29);
    Element(TEXT("text"),Report.Summary,{305,202},{1050,110},18);
    if(Report.Changes.IsEmpty())Element(TEXT("text"),TEXT("本次不会删除任何进度。请更新到支持此存档的版本，或从备份恢复。"),{305,330},{1040,90},19);
    for(int32 I=CompatibilityScroll;I<FMath::Min(CompatibilityScroll+2,Report.Changes.Num());++I)
        Element(TEXT("text"),FString::Printf(TEXT("%d/%d  %s"),I+1,Report.Changes.Num(),*Report.Changes[I]),{305,325.+(I-CompatibilityScroll)*125},{1040,120},17);
    Element(TEXT("button"),TEXT("上一页"),{305,590},{165,44},18,TEXT("compat.prev"));Elements.Last().Enabled=CompatibilityScroll>0;
    Element(TEXT("button"),TEXT("下一页"),{490,590},{165,44},18,TEXT("compat.next"));Elements.Last().Enabled=CompatibilityScroll+2<Report.Changes.Num();
    Element(TEXT("button"),TEXT("同步更新（GitHub）"),{720,590},{310,44},18,TEXT("update.open"));
    Element(TEXT("button"),TEXT("存档目录"),{1130,590},{210,44},18,TEXT("compat.folder"));
    Element(TEXT("text"),TEXT("确认前先校验并备份原档。仅删除所列记录；其余进度保留。取消不会改动文件。"),{305,658},{1050,56},17);
    Element(TEXT("button"),TEXT("备份并删除以上冲突"),{305,746},{430,55},21,TEXT("confirm"));
    Elements.Last().Enabled=Report.CanRepair && CompatibilityScroll+2>=Report.Changes.Num();
    Element(TEXT("button"),TEXT("取消，保留原进度"),{970,746},{365,55},21,TEXT("cancel"));
}
