#include "HearthwardScreenWidget.h"
#include "../HearthwardCharacter.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
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
    FString Key;
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
    auto* PlayerSettings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
    SettingsBindings=PlayerSettings->Bindings;SettingsComfort=PlayerSettings->Comfort;
    SettingsResolution=Settings->GetScreenResolution();SettingsMode=int32(Settings->GetFullscreenMode());
    BindingCapture=NAME_None;SettingsDirty=false;
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
    UGameplayStatics::SetSoundMixClassOverride(this,SettingsSoundMix,Master,1.f,1.f,.15f,true);
}

bool UHearthwardScreenWidget::ExecuteSettingsAction(const FString& Action)
{
    if(Page!=TEXT("settings")) return false;
    if(Action==TEXT("settings.prev") || Action==TEXT("settings.next"))
    { Scroll=FMath::Max(0,Scroll+(Action==TEXT("settings.next")?5:-5));Refresh();return true; }
    if(Action.StartsWith(TEXT("settings.bind:")) || Action.StartsWith(TEXT("settings.clear:")))
    {
        TArray<FString> Parts;Action.ParseIntoArray(Parts,TEXT(":"));if(Parts.Num()!=3) return false;
        const FName Id(*Parts[1]);const int32 BindingIndex=FCString::Atoi(*Parts[2]);
        if(!SettingsBindings.Contains(Id) || BindingIndex<0 || BindingIndex>1) return false;
        if(Action.StartsWith(TEXT("settings.clear:"))) {SettingsBindings[Id][BindingIndex]={};SettingsDirty=true;Refresh();return true;}
        BindingCapture=Id;BindingSlot=BindingIndex;Message=TEXT("按下新键（支持一个Shift/Ctrl/Alt组合）；Esc取消，Delete清除");MessageUntil=FPlatformTime::Seconds()+60;Refresh();return true;
    }
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
        SettingsBindings=HearthwardInput::Defaults();SettingsComfort={};SettingsMode=int32(EWindowMode::WindowedFullscreen);SettingsResolution=FIntPoint(1920,1080);
        SettingsDirty=true;
        Message=TEXT("默认值已填入，点击“应用更改”后生效");
        MessageUntil=FPlatformTime::Seconds()+4;
        Refresh();
        return true;
    }
    if(Action==TEXT("settings.apply"))
    {
        const FString Conflict=HearthwardInput::Validate(SettingsBindings);
        if(!Conflict.IsEmpty()) {Message=Conflict;MessageUntil=FPlatformTime::Seconds()+8;Refresh();return false;}
        auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
        if(!Save->SetAutoMinutes(SettingsAutoMinutes)) return false;
        MenuPause=SettingsMenuPause;
        GConfig->SetBool(TEXT("Hearthward.Survival"),TEXT("MenuPause"),MenuPause,GGameUserSettingsIni);
        GConfig->SetInt(TEXT("Hearthward.Audio"),TEXT("MasterVolume"),SettingsVolume,GGameUserSettingsIni);
        GConfig->SetInt(TEXT("Hearthward.Controls"),TEXT("LookSensitivity"),SettingsSensitivity,GGameUserSettingsIni);
        GConfig->SetBool(TEXT("Hearthward.Controls"),TEXT("InvertLookY"),SettingsInvertY,GGameUserSettingsIni);
        GConfig->Flush(false,GGameUserSettingsIni);
        auto* Settings=GEngine->GetGameUserSettings();
        PreviousMode=Settings->GetFullscreenMode();PreviousResolution=Settings->GetScreenResolution();
        const bool ChangedDisplay=PreviousMode!=EWindowMode::Type(SettingsMode) || PreviousResolution!=SettingsResolution;
        Settings->SetFullscreenMode(EWindowMode::Type(SettingsMode));Settings->SetScreenResolution(SettingsResolution);
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
        auto* PlayerSettings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
        PlayerSettings->Bindings=SettingsBindings;PlayerSettings->Comfort=SettingsComfort;PlayerSettings->Persist();
        if(auto* Character=Cast<AHearthwardCharacter>(GetOwningPlayerPawn()))
        {Character->SetLookSettings(SettingsSensitivity,SettingsInvertY);Character->RebuildInputBindings();PlayerSettings->ApplyTo(Character);}
        if(ChangedDisplay) {DisplayDeadline=FPlatformTime::Seconds()+15;ConfirmAction=TEXT("settings.display");ConfirmMessage=TEXT("保留新的显示模式与分辨率？15秒后自动恢复。");KeyboardFocus=INDEX_NONE;}
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
    else if(Key==TEXT("mode")) SettingsMode=(SettingsMode+Step+3)%3;
    else if(Key==TEXT("resolution"))
    {
        const TArray<FIntPoint> Sizes={FIntPoint(1280,720),FIntPoint(1920,1080),FIntPoint(1920,1200),FIntPoint(2560,1440),FIntPoint(3440,1440),FIntPoint(3840,2160)};
        const int32 Index=Sizes.IndexOfByKey(SettingsResolution);SettingsResolution=Sizes[(FMath::Max(0,Index)+Step+Sizes.Num())%Sizes.Num()];
    }
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
    else if(Key==TEXT("music")) SettingsComfort.Music=FMath::Clamp(SettingsComfort.Music+Step*10,0,100);
    else if(Key==TEXT("voice")) SettingsComfort.Voice=FMath::Clamp(SettingsComfort.Voice+Step*10,0,100);
    else if(Key==TEXT("soundeffects")) SettingsComfort.Effects=FMath::Clamp(SettingsComfort.Effects+Step*10,0,100);
    else if(Key==TEXT("environment")) SettingsComfort.Environment=FMath::Clamp(SettingsComfort.Environment+Step*10,0,100);
    else if(Key==TEXT("sprintToggle")) SettingsComfort.SprintToggle=!SettingsComfort.SprintToggle;
    else if(Key==TEXT("guardToggle")) SettingsComfort.GuardToggle=!SettingsComfort.GuardToggle;
    else if(Key==TEXT("aimToggle")) SettingsComfort.AimToggle=!SettingsComfort.AimToggle;
    else if(Key==TEXT("subtitles")) SettingsComfort.Subtitles=!SettingsComfort.Subtitles;
    else if(Key==TEXT("speaker")) SettingsComfort.SubtitleSpeaker=!SettingsComfort.SubtitleSpeaker;
    else if(Key==TEXT("textscale")) SettingsComfort.TextScale=FMath::Clamp(SettingsComfort.TextScale+Step*25,100,150);
    else if(Key==TEXT("subtitleSize")) SettingsComfort.SubtitleSize=FMath::Clamp(SettingsComfort.SubtitleSize+Step*8,32,48);
    else if(Key==TEXT("subtitleBackground")) SettingsComfort.SubtitleBackground=FMath::Clamp(SettingsComfort.SubtitleBackground+Step*10,0,100);
    else if(Key==TEXT("shake")) SettingsComfort.Shake=FMath::Clamp(SettingsComfort.Shake+Step*10,0,100);
    else if(Key==TEXT("blur")) SettingsComfort.MotionBlur=!SettingsComfort.MotionBlur;
    else if(Key==TEXT("hunger")) SettingsComfort.HungerVisual=FMath::Clamp(SettingsComfort.HungerVisual+Step*10,0,100);
    else if(Key==TEXT("fov")) SettingsComfort.FOV=FMath::Clamp(SettingsComfort.FOV+Step*5,70,110);
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
        Add(TEXT("savekey"),TEXT("手动存档快捷键"),HearthwardInput::Label(SettingsBindings,TEXT("ui.save")),TEXT("使用当前绑定打开存档页面。"));
        Add(TEXT("returnkey"),TEXT("返回游戏"),TEXT("Esc"),TEXT("按 Esc 返回上一页；在游戏中可打开或关闭暂停菜单。"));
    }
    else if(Category==TEXT("显示"))
    {
        const TCHAR* Modes[]={TEXT("全屏"),TEXT("无边框全屏"),TEXT("窗口")};
        Add(TEXT("mode"),TEXT("显示模式"),Modes[SettingsMode],TEXT("应用后需在15秒内确认，否则仅恢复显示模式与分辨率。"),true);
        Add(TEXT("resolution"),TEXT("分辨率"),FString::Printf(TEXT("%d × %d"),SettingsResolution.X,SettingsResolution.Y),TEXT("应用后由引擎更新渲染尺寸；不支持的显示模式可恢复。"),true);
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
        Add(TEXT("music"),TEXT("音乐"),FString::Printf(TEXT("%d%%"),SettingsComfort.Music),TEXT("与主音量相乘。"),true);
        Add(TEXT("voice"),TEXT("固定对白"),FString::Printf(TEXT("%d%%"),SettingsComfort.Voice),TEXT("固定人声与字幕独立；动态回复只有文字。"),true);
        Add(TEXT("soundeffects"),TEXT("效果"),FString::Printf(TEXT("%d%%"),SettingsComfort.Effects),TEXT("与主音量相乘。"),true);
        Add(TEXT("environment"),TEXT("环境"),FString::Printf(TEXT("%d%%"),SettingsComfort.Environment),TEXT("与主音量相乘。"),true);
    }
    else if(Category==TEXT("控制"))
    {
        Add(TEXT("sensitivity"),TEXT("鼠标灵敏度"),FString::Printf(TEXT("%d / 10"),SettingsSensitivity),TEXT("调整鼠标转动镜头的速度，默认值为 5。"),true);
        Add(TEXT("invert"),TEXT("反转纵向视角"),SettingsInvertY?TEXT("开启"):TEXT("关闭"),TEXT("开启后，上下移动鼠标时镜头的纵向转动方向相反。"),true);
        Add(TEXT("sprintToggle"),TEXT("冲刺方式"),SettingsComfort.SprintToggle?TEXT("切换"):TEXT("按住"),TEXT("退出页面、读档和失去焦点会释放冲刺。"),true);
        Add(TEXT("guardToggle"),TEXT("格挡方式"),SettingsComfort.GuardToggle?TEXT("切换"):TEXT("按住"),TEXT("切换格挡不改变费用与防御规则。"),true);
        Add(TEXT("aimToggle"),TEXT("瞄准方式"),SettingsComfort.AimToggle?TEXT("切换"):TEXT("按住"),TEXT("切换瞄准不自动发射。"),true);
    }
    else if(Category==TEXT("键位"))
    {
        for(const auto& D:HearthwardInput::Definitions()) Add(*D.Id.ToString(),*D.Label,HearthwardInput::Label(SettingsBindings,D.Id),TEXT("点击主／副键后按新键；同时可用的冲突阻止应用。Esc和Enter为保留键。"));
    }
    else if(Category==TEXT("辅助功能"))
    {
        Add(TEXT("subtitles"),TEXT("固定对白字幕"),SettingsComfort.Subtitles?TEXT("开启"):TEXT("关闭"),TEXT("静音后仍保留任务、生命与行动反馈。"),true);
        Add(TEXT("speaker"),TEXT("字幕说话人"),SettingsComfort.SubtitleSpeaker?TEXT("显示"):TEXT("隐藏"),TEXT("以文字标注说话人。"),true);
        Add(TEXT("subtitleSize"),TEXT("字幕字号"),FString::Printf(TEXT("%d px"),SettingsComfort.SubtitleSize),TEXT("1080p目标字身；其他分辨率保持视觉比例。"),true);
        Add(TEXT("subtitleBackground"),TEXT("字幕背景"),FString::Printf(TEXT("%d%%"),SettingsComfort.SubtitleBackground),TEXT("调节文字底色遮罩。"),true);
        Add(TEXT("textscale"),TEXT("界面文字"),FString::Printf(TEXT("%d%%"),SettingsComfort.TextScale),TEXT("放大正文、按钮和输入框。"),true);
        Add(TEXT("shake"),TEXT("镜头震动"),FString::Printf(TEXT("%d%%"),SettingsComfort.Shake),TEXT("0%关闭；菜单与固定镜头不震动。"),true);
        Add(TEXT("blur"),TEXT("运动模糊"),SettingsComfort.MotionBlur?TEXT("开启"):TEXT("关闭"),TEXT("默认关闭。"),true);
        Add(TEXT("hunger"),TEXT("饥饿视觉强度"),FString::Printf(TEXT("%d%%"),SettingsComfort.HungerVisual),TEXT("0%仅移除视觉限制，实际饥饿惩罚仍有效。"),true);
        Add(TEXT("fov"),TEXT("水平视野角"),FString::Printf(TEXT("%d°"),SettingsComfort.FOV),TEXT("70—110°；严重饥饿最多减少10°且不低于70°。"),true);
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
    const int32 PageSize=5;
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Rows.Num()-1));
    for(int32 I=Scroll;I<FMath::Min(Rows.Num(),Scroll+PageSize);++I)
    {
        const FSettingRow& Row=Rows[I];
        const int32 Y=190+(I-Scroll)*78;
        Element(TEXT("settingsRow"),TEXT(""),FVector2D(677,Y),FVector2D(674,72),18,FString(TEXT("settings.select:"))+Row.Key,TEXT(""),SettingsSelection==Row.Key);
        Element(TEXT("text"),Row.Label,FVector2D(702,Y+10),FVector2D(320,61),23);
        if(Category==TEXT("键位"))
        {
            const auto& Slots=SettingsBindings.FindChecked(FName(*Row.Key));
            for(int32 BindingIndex=0;BindingIndex<2;++BindingIndex)
            {
                const FString Suffix=FString(Row.Key)+TEXT(":")+FString::FromInt(BindingIndex);
                Element(TEXT("button"),Slots[BindingIndex].Label(),FVector2D(1045+BindingIndex*151,Y+5),FVector2D(142,60),18,TEXT("settings.bind:")+Suffix);
            }
        }
        else if(Row.Adjustable)
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
    if(Rows.Num()>PageSize)
    {
        Element(TEXT("button"),TEXT("上一页"),FVector2D(697,595),FVector2D(160,38),18,TEXT("settings.prev"));Elements.Last().Enabled=Scroll>0;
        Element(TEXT("button"),TEXT("下一页"),FVector2D(1150,595),FVector2D(160,38),18,TEXT("settings.next"));Elements.Last().Enabled=Scroll+PageSize<Rows.Num();
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

bool UHearthwardScreenWidget::CaptureBinding(FKey Key,bool Shift,bool Control,bool Alt)
{
    if(BindingCapture.IsNone()) return false;
    if(Key==EKeys::Escape) {BindingCapture=NAME_None;Message=TEXT("已取消键位捕获");return true;}
    if(Key.IsModifierKey()) return true;
    if(int32(Shift)+int32(Control)+int32(Alt)>1) {Message=TEXT("只支持一个修饰键");return true;}
    SettingsBindings.FindChecked(BindingCapture)[BindingSlot]=Key==EKeys::Delete?FHearthwardKeyBinding():FHearthwardKeyBinding{Key,Shift?EKeys::LeftShift:Control?EKeys::LeftControl:Alt?EKeys::LeftAlt:FKey()};
    BindingCapture=NAME_None;SettingsDirty=true;Message=HearthwardInput::Validate(SettingsBindings);MessageUntil=FPlatformTime::Seconds()+8;Refresh();return true;
}
void UHearthwardScreenWidget::FinishDisplayChange(bool Keep)
{
    if(DisplayDeadline<=0) return;
    DisplayDeadline=0;ConfirmAction.Reset();
    auto* Settings=GEngine->GetGameUserSettings();
    if(!Keep) {Settings->SetFullscreenMode(PreviousMode);Settings->SetScreenResolution(PreviousResolution);Settings->ApplyResolutionSettings(false);}
    Settings->ConfirmVideoMode();Settings->SaveSettings();SettingsMode=int32(Settings->GetFullscreenMode());SettingsResolution=Settings->GetScreenResolution();
    Message=Keep?TEXT("已保留显示设置"):TEXT("已恢复原显示模式与分辨率");MessageUntil=FPlatformTime::Seconds()+5;Refresh();
}
