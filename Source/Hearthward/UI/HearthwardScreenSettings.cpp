#include "HearthwardScreenWidget.h"
#include "../HearthwardCharacter.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/AudioSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
struct FSettingRow
{
    const TCHAR* Key;
    FString Label,Value,Description;
    bool Adjustable=false;
};

const TCHAR* QualityName(int32 Level)
{
    static const TCHAR* Names[]={TEXT("低"),TEXT("中"),TEXT("高"),TEXT("极高"),TEXT("影视级")};
    return Level>=0 && Level<UE_ARRAY_COUNT(Names)?Names[Level]:TEXT("自定义");
}
}

void UHearthwardScreenWidget::LoadSettingsDraft()
{
    const UGameUserSettings* Settings=GEngine->GetGameUserSettings();
    SettingsAutoMinutes=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetAutoMinutes();
    SettingsMenuPause=MenuPause;
    SettingsFullscreen=Settings->GetFullscreenMode()!=EWindowMode::Windowed;
    SettingsVSync=Settings->IsVSyncEnabled();
    SettingsFrameLimit=FMath::RoundToInt(Settings->GetFrameRateLimit());
    SettingsQuality=Settings->GetOverallScalabilityLevel();
    SettingsShadow=Settings->GetShadowQuality();
    SettingsTexture=Settings->GetTextureQuality();
    SettingsViewDistance=Settings->GetViewDistanceQuality();
    SettingsEffects=Settings->GetVisualEffectQuality();
    GConfig->GetInt(TEXT("Hearthward.Audio"),TEXT("MasterVolume"),SettingsVolume,GGameUserSettingsIni);
    SettingsVolume=FMath::Clamp(SettingsVolume,0,100);
    GConfig->GetInt(TEXT("Hearthward.Controls"),TEXT("LookSensitivity"),SettingsSensitivity,GGameUserSettingsIni);
    SettingsSensitivity=FMath::Clamp(SettingsSensitivity,1,10);
    GConfig->GetBool(TEXT("Hearthward.Controls"),TEXT("InvertLookY"),SettingsInvertY,GGameUserSettingsIni);
    SettingsDirty=false;
}

void UHearthwardScreenWidget::ApplyMasterVolume()
{
    USoundClass* Master=GetDefault<UAudioSettings>()->GetDefaultSoundClass();
    if(!Master) return;
    if(!SettingsSoundMix)
    {
        SettingsSoundMix=NewObject<USoundMix>(this);
        UGameplayStatics::PushSoundMixModifier(this,SettingsSoundMix);
    }
    UGameplayStatics::SetSoundMixClassOverride(this,SettingsSoundMix,Master,SettingsVolume/100.f,1.f,.15f,true);
}

bool UHearthwardScreenWidget::ExecuteSettingsAction(const FString& Action)
{
    if(Page!=TEXT("settings")) return false;
    if(Action.StartsWith(TEXT("settings.select:")))
    {
        SettingsSelection=Action.Mid(16);
        Refresh();
        return true;
    }
    if(Action==TEXT("settings.defaults"))
    {
        SettingsAutoMinutes=10;
        SettingsMenuPause=true;
        SettingsFullscreen=true;
        SettingsVSync=false;
        SettingsFrameLimit=0;
        SettingsQuality=SettingsShadow=SettingsTexture=SettingsViewDistance=SettingsEffects=3;
        SettingsVolume=100;
        SettingsSensitivity=5;
        SettingsInvertY=false;
        SettingsDirty=true;
        Message=TEXT("默认值已填入，点击“应用更改”后生效");
        MessageUntil=FPlatformTime::Seconds()+4;
        Refresh();
        return true;
    }
    if(Action==TEXT("settings.apply"))
    {
        auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
        if(!Save->SetAutoMinutes(SettingsAutoMinutes)) return false;
        MenuPause=SettingsMenuPause;
        GConfig->SetBool(TEXT("Hearthward.Survival"),TEXT("MenuPause"),MenuPause,GGameUserSettingsIni);
        GConfig->SetInt(TEXT("Hearthward.Audio"),TEXT("MasterVolume"),SettingsVolume,GGameUserSettingsIni);
        GConfig->SetInt(TEXT("Hearthward.Controls"),TEXT("LookSensitivity"),SettingsSensitivity,GGameUserSettingsIni);
        GConfig->SetBool(TEXT("Hearthward.Controls"),TEXT("InvertLookY"),SettingsInvertY,GGameUserSettingsIni);
        GConfig->Flush(false,GGameUserSettingsIni);
        auto* Settings=GEngine->GetGameUserSettings();
        Settings->SetFullscreenMode(SettingsFullscreen?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);
        Settings->SetVSyncEnabled(SettingsVSync);
        Settings->SetFrameRateLimit(SettingsFrameLimit);
        if(SettingsQuality>=0) Settings->SetOverallScalabilityLevel(SettingsQuality);
        Settings->SetShadowQuality(SettingsShadow);
        Settings->SetTextureQuality(SettingsTexture);
        Settings->SetViewDistanceQuality(SettingsViewDistance);
        Settings->SetVisualEffectQuality(SettingsEffects);
        Settings->ApplySettings(false);
        Settings->SaveSettings();
        ApplyMasterVolume();
        if(auto* Character=Cast<AHearthwardCharacter>(GetOwningPlayerPawn()))
            Character->SetLookSettings(SettingsSensitivity,SettingsInvertY);
        SettingsDirty=false;
        Message=TEXT("设置已应用");
        MessageUntil=FPlatformTime::Seconds()+4;
        Refresh();
        return true;
    }
    if(!Action.StartsWith(TEXT("settings.change:"))) return false;
    TArray<FString> Parts;
    Action.ParseIntoArray(Parts,TEXT(":"));
    if(Parts.Num()!=3) return false;
    const FString& Key=Parts[1];
    const int32 Step=FCString::Atoi(*Parts[2]);
    if(Step==0) return false;
    SettingsSelection=Key;
    if(Key==TEXT("autosave")) SettingsAutoMinutes=FMath::Clamp(SettingsAutoMinutes+Step,1,60);
    else if(Key==TEXT("pause")) SettingsMenuPause=!SettingsMenuPause;
    else if(Key==TEXT("mode")) SettingsFullscreen=!SettingsFullscreen;
    else if(Key==TEXT("vsync")) SettingsVSync=!SettingsVSync;
    else if(Key==TEXT("fps"))
    {
        const TArray<int32> Limits={0,30,60,120,144};
        const int32 Index=Limits.IndexOfByKey(SettingsFrameLimit);
        SettingsFrameLimit=Limits[(FMath::Max(0,Index)+Step+Limits.Num())%Limits.Num()];
    }
    else if(Key==TEXT("quality"))
    {
        SettingsQuality=FMath::Clamp((SettingsQuality<0?2:SettingsQuality)+Step,0,4);
        SettingsShadow=SettingsTexture=SettingsViewDistance=SettingsEffects=SettingsQuality;
    }
    else if(Key==TEXT("shadow")) SettingsShadow=FMath::Clamp(SettingsShadow+Step,0,4);
    else if(Key==TEXT("texture")) SettingsTexture=FMath::Clamp(SettingsTexture+Step,0,4);
    else if(Key==TEXT("distance")) SettingsViewDistance=FMath::Clamp(SettingsViewDistance+Step,0,4);
    else if(Key==TEXT("effects")) SettingsEffects=FMath::Clamp(SettingsEffects+Step,0,4);
    else if(Key==TEXT("volume")) SettingsVolume=FMath::Clamp(SettingsVolume+Step*10,0,100);
    else if(Key==TEXT("sensitivity")) SettingsSensitivity=FMath::Clamp(SettingsSensitivity+Step,1,10);
    else if(Key==TEXT("invert")) SettingsInvertY=!SettingsInvertY;
    else return false;
    if(Key==TEXT("shadow") || Key==TEXT("texture") || Key==TEXT("distance") || Key==TEXT("effects")) SettingsQuality=-1;
    SettingsDirty=true;
    Refresh();
    return true;
}

void UHearthwardScreenWidget::ComposeSettings()
{
    const TCHAR* Tabs[]={TEXT("游戏"),TEXT("显示"),TEXT("图形"),TEXT("音频"),
        TEXT("控制"),TEXT("键位"),TEXT("辅助功能"),TEXT("教程")};
    Element(TEXT("panel"),TEXT(""),FVector2D(417,180),FVector2D(227,587));
    Element(TEXT("panel"),TEXT(""),FVector2D(668,180),FVector2D(692,462));
    Element(TEXT("panel"),TEXT(""),FVector2D(668,651),FVector2D(692,116));
    Element(TEXT("line"),TEXT(""),FVector2D(677,139),FVector2D(125,1)); Elements.Last().Color=Color(TEXT("bronze"));
    Element(TEXT("line"),TEXT(""),FVector2D(987,139),FVector2D(125,1)); Elements.Last().Color=Color(TEXT("bronze"));
    Element(TEXT("text"),TEXT("设置"),FVector2D(814,111),FVector2D(155,56),44); Elements.Last().Align=TEXT("center");
    for(int32 I=0;I<UE_ARRAY_COUNT(Tabs);++I)
    {
        const int32 Y=188+I*57;
        const bool Active=Category==Tabs[I];
        Element(TEXT("settingsTab"),TEXT(""),FVector2D(423,Y),FVector2D(215,53),18,FString(TEXT("category:"))+Tabs[I],TEXT(""),Active);
        Element(TEXT("text"),FString(Tabs[I]).Left(1),FVector2D(439,Y+12),FVector2D(32,32),24);
        Elements.Last().Align=TEXT("center"); Elements.Last().Color=Color(TEXT("gold"));
        Element(TEXT("text"),Tabs[I],FVector2D(493,Y+11),FVector2D(131,32),25);
    }

    TArray<FSettingRow> Rows;
    auto Add=[&](const TCHAR* Key,const TCHAR* Label,FString Value,const TCHAR* Description,bool Adjustable=false)
    { Rows.Add({Key,Label,MoveTemp(Value),Description,Adjustable}); };
    if(Category==TEXT("游戏"))
    {
        Add(TEXT("autosave"),TEXT("自动保存间隔"),FString::Printf(TEXT("%d 分钟"),SettingsAutoMinutes),TEXT("设置游戏自动保存的时间间隔。较短的间隔可以降低进度丢失的风险。"),true);
        Add(TEXT("pause"),TEXT("菜单暂停世界"),SettingsMenuPause?TEXT("开启"):TEXT("关闭"),TEXT("打开普通游戏菜单时是否暂停世界。标题页和存档页始终会暂停。"),true);
        Add(TEXT("savekey"),TEXT("手动存档快捷键"),TEXT("F6"),TEXT("在游戏中按 F6 打开存档页面。"));
        Add(TEXT("returnkey"),TEXT("返回游戏"),TEXT("Esc"),TEXT("按 Esc 返回上一页；在游戏中可打开或关闭暂停菜单。"));
    }
    else if(Category==TEXT("显示"))
    {
        const FIntPoint Resolution=GEngine->GetGameUserSettings()->GetScreenResolution();
        Add(TEXT("mode"),TEXT("显示模式"),SettingsFullscreen?TEXT("无边框全屏"):TEXT("窗口"),TEXT("切换窗口与无边框全屏模式。点击应用后由引擎更新游戏窗口。"),true);
        Add(TEXT("resolution"),TEXT("当前分辨率"),FString::Printf(TEXT("%d × %d"),Resolution.X,Resolution.Y),TEXT("当前由操作系统或游戏窗口决定的渲染尺寸。"));
        Add(TEXT("vsync"),TEXT("垂直同步"),SettingsVSync?TEXT("开启"):TEXT("关闭"),TEXT("开启后将画面更新与显示器刷新同步，可减少画面撕裂。"),true);
        Add(TEXT("fps"),TEXT("帧率上限"),SettingsFrameLimit==0?TEXT("无限制"):FString::Printf(TEXT("%d FPS"),SettingsFrameLimit),TEXT("限制游戏每秒绘制的帧数；无限制由硬件性能决定。"),true);
    }
    else if(Category==TEXT("图形"))
    {
        Add(TEXT("quality"),TEXT("整体画质"),QualityName(SettingsQuality),TEXT("一次调整各项图形质量；单独修改下方项目后会显示为自定义。"),true);
        Add(TEXT("distance"),TEXT("视距质量"),QualityName(SettingsViewDistance),TEXT("决定远处物体细节的保留程度。"),true);
        Add(TEXT("shadow"),TEXT("阴影质量"),QualityName(SettingsShadow),TEXT("控制阴影的精细程度及绘制开销。"),true);
        Add(TEXT("texture"),TEXT("纹理质量"),QualityName(SettingsTexture),TEXT("控制纹理细节；更高等级会占用更多显存。"),true);
        Add(TEXT("effects"),TEXT("特效质量"),QualityName(SettingsEffects),TEXT("控制粒子与其他视觉效果的质量。"),true);
    }
    else if(Category==TEXT("音频"))
    {
        Add(TEXT("volume"),TEXT("主音量"),FString::Printf(TEXT("%d%%"),SettingsVolume),TEXT("调整游戏的整体音量；点击应用后立即作用于所有声音。"),true);
        Add(TEXT("audioinfo"),TEXT("音量范围"),TEXT("0 - 100%"),TEXT("0% 为静音，100% 为原始音量。"));
    }
    else if(Category==TEXT("控制"))
    {
        Add(TEXT("sensitivity"),TEXT("鼠标灵敏度"),FString::Printf(TEXT("%d / 10"),SettingsSensitivity),TEXT("调整鼠标转动镜头的速度，默认值为 5。"),true);
        Add(TEXT("invert"),TEXT("反转纵向视角"),SettingsInvertY?TEXT("开启"):TEXT("关闭"),TEXT("开启后，上下移动鼠标时镜头的纵向转动方向相反。"),true);
        Add(TEXT("move"),TEXT("移动与冲刺"),TEXT("WASD / Shift"),TEXT("WASD 控制移动，按住 Shift 冲刺。"));
        Add(TEXT("interact"),TEXT("交互"),TEXT("E"),TEXT("靠近可交互对象后按 E。"));
    }
    else if(Category==TEXT("键位"))
    {
        Add(TEXT("keys1"),TEXT("移动 / 冲刺"),TEXT("WASD / Shift"),TEXT("角色移动与冲刺。"));
        Add(TEXT("keys2"),TEXT("跳跃 / 交互"),TEXT("Space / E"),TEXT("跳跃及与附近对象交互。"));
        Add(TEXT("keys3"),TEXT("背包 / 地图"),TEXT("Tab / M"),TEXT("打开背包和地图。"));
        Add(TEXT("keys4"),TEXT("技能 / 任务"),TEXT("K / J"),TEXT("打开技能和任务页面。"));
        Add(TEXT("keys5"),TEXT("暂停 / 存档"),TEXT("P / F6"),TEXT("打开暂停菜单与存档页面。"));
        Add(TEXT("keys6"),TEXT("攻击 / 防御"),TEXT("鼠标左键 / 右键"),TEXT("战斗时使用主攻击与防御。"));
        Add(TEXT("keys7"),TEXT("与伙伴交谈"),TEXT("T"),TEXT("靠近伙伴后按 T 打开交流页面。"));
        Add(TEXT("keys8"),TEXT("返回"),TEXT("Esc"),TEXT("关闭当前页面或返回上一页。"));
    }
    else if(Category==TEXT("辅助功能"))
    {
        Add(TEXT("access1"),TEXT("界面操作"),TEXT("鼠标 / 键盘"),TEXT("设置页面支持鼠标选择，也支持方向键移动焦点和 Enter 确认。"));
        Add(TEXT("access2"),TEXT("界面返回"),TEXT("Esc"),TEXT("按 Esc 可从当前页面返回。"));
        Add(TEXT("access3"),TEXT("声音控制"),TEXT("主音量"),TEXT("在音频分类中可调整整体音量。"));
    }
    else
    {
        Add(TEXT("tutorial1"),TEXT("开始探索"),TEXT("WASD / 鼠标"),TEXT("使用 WASD 移动，移动鼠标查看周围。"));
        Add(TEXT("tutorial2"),TEXT("收集资源"),TEXT("E"),TEXT("靠近资源点后按 E，观察交互提示。"));
        Add(TEXT("tutorial3"),TEXT("管理行装"),TEXT("Tab"),TEXT("查看背包中的材料和装备。"));
        Add(TEXT("tutorial4"),TEXT("查看方向"),TEXT("M / J"),TEXT("地图和任务页可帮助确定下一步目标。"));
        Add(TEXT("tutorial5"),TEXT("保存进度"),TEXT("F6"),TEXT("在安全时打开存档页保存游戏。"));
    }
    if(!Rows.ContainsByPredicate([&](const FSettingRow& Row){return SettingsSelection==Row.Key;})) SettingsSelection=Rows[0].Key;
    for(int32 I=0;I<Rows.Num();++I)
    {
        const FSettingRow& Row=Rows[I];
        const int32 Y=190+I*55;
        Element(TEXT("settingsRow"),TEXT(""),FVector2D(677,Y),FVector2D(674,51),18,FString(TEXT("settings.select:"))+Row.Key,TEXT(""),SettingsSelection==Row.Key);
        Element(TEXT("text"),Row.Label,FVector2D(702,Y+10),FVector2D(365,34),23);
        if(Row.Adjustable)
        {
            Element(TEXT("button"),TEXT("‹"),FVector2D(1093,Y+6),FVector2D(37,38),25,FString(TEXT("settings.change:"))+Row.Key+TEXT(":-1"));
            Element(TEXT("text"),Row.Value,FVector2D(1138,Y+10),FVector2D(145,34),23); Elements.Last().Align=TEXT("center");
            Element(TEXT("button"),TEXT("›"),FVector2D(1291,Y+6),FVector2D(37,38),25,FString(TEXT("settings.change:"))+Row.Key+TEXT(":1"));
        }
        else
        {
            Element(TEXT("text"),Row.Value,FVector2D(1094,Y+10),FVector2D(225,34),21); Elements.Last().Align=TEXT("right"); Elements.Last().Color=Color(TEXT("muted"));
        }
    }
    const FSettingRow* Selected=Rows.FindByPredicate([&](const FSettingRow& Row){return SettingsSelection==Row.Key;});
    Element(TEXT("text"),Selected->Label,FVector2D(696,669),FVector2D(625,28),23); Elements.Last().Color=Color(TEXT("gold"));
    Element(TEXT("text"),Selected->Description,FVector2D(696,702),FVector2D(635,55),19);
    if(SettingsDirty)
    {
        Element(TEXT("text"),TEXT("更改待应用"),FVector2D(916,801),FVector2D(180,27),17);
        Elements.Last().Color=Color(TEXT("gold"));
    }
    Element(TEXT("button"),TEXT("Esc  返回"),FVector2D(438,793),FVector2D(163,44),20,TEXT("back"));
    Element(TEXT("button"),TEXT("R  恢复默认"),FVector2D(616,793),FVector2D(185,44),20,TEXT("settings.defaults"));
    Element(TEXT("button"),TEXT("应用更改  ›"),FVector2D(1108,793),FVector2D(235,44),21,TEXT("settings.apply"));
}
