#include "HearthwardInputBindings.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ConfigCacheIni.h"
FString FHearthwardKeyBinding::Encode() const
{ return Key.IsValid()?(Modifier.IsValid()?Modifier.GetFName().ToString()+TEXT("+"):FString())+Key.GetFName().ToString():FString(); }
FString FHearthwardKeyBinding::Label() const
{
    auto Name=[](FKey K)
    {
        if(K==EKeys::LeftMouseButton) return FString(TEXT("鼠标左键"));
        if(K==EKeys::RightMouseButton) return FString(TEXT("鼠标右键"));
        if(K==EKeys::MiddleMouseButton) return FString(TEXT("鼠标中键"));
        if(K==EKeys::LeftShift || K==EKeys::RightShift) return FString(TEXT("Shift"));
        if(K==EKeys::LeftControl || K==EKeys::RightControl) return FString(TEXT("Ctrl"));
        if(K==EKeys::LeftAlt || K==EKeys::RightAlt) return FString(TEXT("Alt"));
        if(K==EKeys::SpaceBar) return FString(TEXT("Space"));
        return K.GetDisplayName().ToString();
    };
    return Key.IsValid()?(Modifier.IsValid()?Name(Modifier)+TEXT(" + "):FString())+Name(Key):TEXT("未绑定");
}
FHearthwardKeyBinding FHearthwardKeyBinding::Decode(const FString& Text)
{
    FString M,K;if(Text.Split(TEXT("+"),&M,&K)) return {FKey(*K),FKey(*M)};
    return Text.IsEmpty()?FHearthwardKeyBinding():FHearthwardKeyBinding{FKey(*Text),FKey()};
}
bool FHearthwardKeyBinding::Matches(FKey Pressed,bool Shift,bool Control,bool Alt) const
{ return Key==Pressed && (!Modifier.IsValid() || (Modifier==EKeys::LeftShift && Shift) || (Modifier==EKeys::LeftControl && Control) || (Modifier==EKeys::LeftAlt && Alt)); }
bool FHearthwardKeyBinding::Held(const APlayerController* Player) const
{ return Player && Key.IsValid() && Player->IsInputKeyDown(Key) && (!Modifier.IsValid() || Player->IsInputKeyDown(Modifier)); }
const TArray<FHearthwardInputDefinition>& HearthwardInput::Definitions()
{
    static const TArray<FHearthwardInputDefinition> Result=[]
    {
        TArray<FHearthwardInputDefinition> Rows;
        auto Add=[&](const TCHAR* Id,const TCHAR* Label,FKey Key,const TCHAR* Contexts,const TCHAR* Shared,bool Essential,FKey Secondary=FKey())
        {
            TArray<FString> Parts;FString(Contexts).ParseIntoArray(Parts,TEXT("|"));
            TArray<FName> Pages;for(const auto& P:Parts) Pages.Add(FName(*P));
            Rows.Add({FName(Id),Label,Pages,FName(Shared),{Key,FKey()},{Secondary,FKey()},Essential});
        };
        Add(TEXT("move.forward"),TEXT("向前"),EKeys::W,TEXT("hud"),TEXT(""),true);
        Add(TEXT("move.back"),TEXT("向后"),EKeys::S,TEXT("hud"),TEXT(""),true);
        Add(TEXT("move.left"),TEXT("向左"),EKeys::A,TEXT("hud"),TEXT(""),true);
        Add(TEXT("move.right"),TEXT("向右"),EKeys::D,TEXT("hud"),TEXT(""),true);
        Add(TEXT("sprint"),TEXT("冲刺"),EKeys::LeftShift,TEXT("hud"),TEXT("modifier"),false);
        Add(TEXT("combat.heavyModifier"),TEXT("重击／背尸修饰"),EKeys::LeftShift,TEXT("hud"),TEXT("modifier"),false);
        Add(TEXT("jump"),TEXT("跳跃"),EKeys::SpaceBar,TEXT("hud"),TEXT(""),false);
        Add(TEXT("interact"),TEXT("交互"),EKeys::E,TEXT("hud"),TEXT("interaction"),true);
        Add(TEXT("traversal.vault"),TEXT("攀越低障"),EKeys::E,TEXT("hud"),TEXT("interaction"),false);
        Add(TEXT("combat.attack"),TEXT("攻击／钓鱼／确认建造"),EKeys::LeftMouseButton,TEXT("hud"),TEXT(""),true);
        Add(TEXT("combat.guardAim"),TEXT("格挡／瞄准／取消建造"),EKeys::RightMouseButton,TEXT("hud"),TEXT(""),false);
        Add(TEXT("combat.execute"),TEXT("处决"),EKeys::F,TEXT("hud"),TEXT(""),false);
        Add(TEXT("combat.stun"),TEXT("击晕处决"),EKeys::R,TEXT("hud"),TEXT("contextR"),false);
        Add(TEXT("storage.open"),TEXT("打开仓储"),EKeys::R,TEXT("hud|storage"),TEXT("contextR"),false);
        Add(TEXT("combat.reload"),TEXT("弩装填"),EKeys::R,TEXT("hud"),TEXT("contextR"),false);
        Add(TEXT("combat.dodge"),TEXT("闪避"),EKeys::LeftAlt,TEXT("hud"),TEXT(""),false);
        Add(TEXT("combat.lock"),TEXT("锁定"),EKeys::MiddleMouseButton,TEXT("hud"),TEXT(""),false);
        Add(TEXT("combat.sense"),TEXT("感应"),EKeys::V,TEXT("hud"),TEXT(""),false);
        Add(TEXT("combat.throw"),TEXT("快捷投掷物"),EKeys::G,TEXT("hud"),TEXT(""),false,EKeys::Four);
        Add(TEXT("survival.medicine"),TEXT("快捷药品"),EKeys::One,TEXT("hud"),TEXT(""),false);
        Add(TEXT("survival.food"),TEXT("快捷食物"),EKeys::Two,TEXT("hud"),TEXT(""),false);
        Add(TEXT("companion.wait"),TEXT("伙伴等待"),EKeys::Z,TEXT("hud"),TEXT(""),false);
        Add(TEXT("companion.follow"),TEXT("伙伴跟随"),EKeys::X,TEXT("hud"),TEXT(""),false);
        Add(TEXT("companion.attack"),TEXT("伙伴进攻"),EKeys::C,TEXT("hud"),TEXT(""),false);
        Add(TEXT("ui.inventory"),TEXT("背包"),EKeys::Tab,TEXT("hud|inventory|equipment|storage|map|skills|journal|building|camp|crafting|repairing|nature|pause|settings|save|dialogue|memory|codex"),TEXT(""),false);
        Add(TEXT("ui.map"),TEXT("地图"),EKeys::M,TEXT("hud|inventory|equipment|storage|map|skills|journal|building|camp|crafting|repairing|nature|pause|settings|save|dialogue|memory|codex"),TEXT(""),false);
        Add(TEXT("ui.skills"),TEXT("技能"),EKeys::K,TEXT("hud|inventory|equipment|storage|map|skills|journal|building|camp|crafting|repairing|nature|pause|settings|save|dialogue|memory|codex"),TEXT(""),false);
        Add(TEXT("ui.journal"),TEXT("任务"),EKeys::J,TEXT("hud|inventory|equipment|storage|map|skills|journal|building|camp|crafting|repairing|nature|pause|settings|save|dialogue|memory|codex"),TEXT(""),false);
        Add(TEXT("ui.save"),TEXT("存读档"),EKeys::F6,TEXT("hud|inventory|equipment|storage|map|skills|journal|building|camp|crafting|repairing|nature|pause|settings|save|dialogue|memory|codex"),TEXT(""),false);
        Add(TEXT("ui.pause"),TEXT("暂停／设置入口"),EKeys::P,TEXT("hud|inventory|equipment|storage|map|skills|journal|building|camp|crafting|repairing|nature|pause|settings|save|dialogue|memory|codex"),TEXT(""),true);
        Add(TEXT("companion.dialogue"),TEXT("交流"),EKeys::T,TEXT("hud|inventory|equipment|storage|map|skills|journal|building|camp|crafting|repairing|nature|pause|settings|save|dialogue|memory|codex"),TEXT(""),false);
        Add(TEXT("building.catalogue"),TEXT("建筑目录"),EKeys::B,TEXT("hud|building"),TEXT(""),false);
        Add(TEXT("building.rotate"),TEXT("旋转预览"),EKeys::Q,TEXT("hud"),TEXT(""),false);
        Add(TEXT("inventory.use"),TEXT("使用物品"),EKeys::F,TEXT("inventory"),TEXT(""),false);
        Add(TEXT("inventory.drop"),TEXT("放下物品"),EKeys::R,TEXT("inventory"),TEXT(""),false);
        Add(TEXT("inventory.repair"),TEXT("维修物品"),EKeys::H,TEXT("inventory"),TEXT(""),false);
        Add(TEXT("crafting.commit"),TEXT("制作"),EKeys::F,TEXT("crafting"),TEXT(""),false);
        Add(TEXT("repair.commit"),TEXT("行装管理：全部修复选中装备"),EKeys::F,TEXT("equipment"),TEXT(""),false);
        Add(TEXT("storage.transfer"),TEXT("仓储转移"),EKeys::E,TEXT("storage"),TEXT(""),false);
        Add(TEXT("skills.learn"),TEXT("学习技能"),EKeys::F,TEXT("skills"),TEXT(""),false);
        Add(TEXT("journal.locate"),TEXT("任务地图"),EKeys::F,TEXT("journal"),TEXT(""),false);
        Add(TEXT("journal.track"),TEXT("追踪任务"),EKeys::V,TEXT("journal"),TEXT(""),false);
        Add(TEXT("journal.category.prev"),TEXT("日志上一分类"),EKeys::Q,TEXT("journal"),TEXT(""),false);
        Add(TEXT("journal.category.next"),TEXT("日志下一分类"),EKeys::E,TEXT("journal"),TEXT(""),false);
        Add(TEXT("settings.defaults"),TEXT("设置默认草稿"),EKeys::R,TEXT("settings"),TEXT(""),false);
        Add(TEXT("map.marker"),TEXT("地图标记"),EKeys::RightMouseButton,TEXT("map"),TEXT(""),false);
        return Rows;
    }();return Result;
}
FHearthwardBindings HearthwardInput::Defaults()
{ FHearthwardBindings Result;for(const auto& D:Definitions()) Result.Add(D.Id,{D.Primary,D.Secondary});return Result; }
FHearthwardBindings HearthwardInput::Load()
{
    auto Result=Defaults();
    for(auto& Pair:Result) for(int32 Slot=0;Slot<2;++Slot)
    { FString Text;if(GConfig->GetString(TEXT("Hearthward.Bindings"),*(Pair.Key.ToString()+FString::FromInt(Slot)),Text,GGameUserSettingsIni)) Pair.Value[Slot]=FHearthwardKeyBinding::Decode(Text); }
    return Validate(Result).IsEmpty()?Result:Defaults();
}
void HearthwardInput::Save(const FHearthwardBindings& Bindings)
{ for(const auto& Pair:Bindings) for(int32 Slot=0;Slot<2;++Slot) GConfig->SetString(TEXT("Hearthward.Bindings"),*(Pair.Key.ToString()+FString::FromInt(Slot)),*Pair.Value[Slot].Encode(),GGameUserSettingsIni); }
FString HearthwardInput::Validate(const FHearthwardBindings& Bindings)
{
    const auto& Rows=Definitions();
    for(int32 I=0;I<Rows.Num();++I)
    {
        const auto& D=Rows[I];const auto* Slots=Bindings.Find(D.Id);
        if(!Slots || Slots->Num()!=2) return TEXT("动作绑定缺失");
        if(D.Essential && !(*Slots)[0].Key.IsValid() && !(*Slots)[1].Key.IsValid()) return D.Label+TEXT("必须保留入口");
        for(const auto& B:*Slots)
        {
            if(!B.Key.IsValid()) continue;
            if(B.Key.IsGamepadKey() || B.Key.IsAxis1D() || B.Key.IsAxis2D() || B.Key==EKeys::Escape || B.Key==EKeys::Enter || B.Key==EKeys::F10 || B.Key==EKeys::LeftCommand || B.Key==EKeys::RightCommand) return D.Label+TEXT("使用了保留键或不支持的输入");
            const bool MenuContext=D.Contexts.ContainsByPredicate([](FName Context){return Context!=TEXT("hud");});
            const bool Navigation=B.Key==EKeys::Up || B.Key==EKeys::Down || B.Key==EKeys::Left || B.Key==EKeys::Right
                || B.Key==EKeys::PageUp || B.Key==EKeys::PageDown || B.Key==EKeys::Home || B.Key==EKeys::End;
            if(MenuContext && (Navigation || (B.Key==EKeys::Tab && (D.Id!=TEXT("ui.inventory") || B.Modifier.IsValid()))))
                return D.Label+TEXT("与界面导航保留键冲突");
            if(B.Modifier.IsValid() && B.Modifier!=EKeys::LeftShift && B.Modifier!=EKeys::LeftControl && B.Modifier!=EKeys::LeftAlt) return TEXT("只支持一个Shift/Ctrl/Alt修饰键");
            if((B.Modifier==EKeys::LeftAlt && (B.Key==EKeys::F4 || B.Key==EKeys::Tab)) || (B.Modifier==EKeys::LeftControl && B.Key==EKeys::Escape)) return TEXT("系统快捷键不能绑定");
            for(int32 J=I+1;J<Rows.Num();++J)
            {
                const auto& Other=Rows[J];if(!D.SharedPriority.IsNone() && D.SharedPriority==Other.SharedPriority) continue;
                if(!D.Contexts.ContainsByPredicate([&](FName C){return Other.Contexts.Contains(C);})) continue;
                if(const auto* OS=Bindings.Find(Other.Id)) for(const auto& OB:*OS) if(B.Key==OB.Key && (!B.Modifier.IsValid() || !OB.Modifier.IsValid() || B.Modifier==OB.Modifier)) return FString::Printf(TEXT("%s与%s冲突：%s"),*D.Label,*Other.Label,*B.Label());
            }
        }
    }return FString();
}
bool HearthwardInput::Matches(const FHearthwardBindings& Bindings,FName Id,FKey Key,bool Shift,bool Control,bool Alt)
{ const auto* Slots=Bindings.Find(Id);return Slots && Slots->ContainsByPredicate([&](const auto& B){return B.Matches(Key,Shift,Control,Alt);}); }
bool HearthwardInput::Held(const FHearthwardBindings& Bindings,FName Id,const APlayerController* Player)
{ const auto* Slots=Bindings.Find(Id);return Slots && Slots->ContainsByPredicate([&](const auto& B){return B.Held(Player);}); }
FString HearthwardInput::Label(const FHearthwardBindings& Bindings,FName Id)
{ const auto* Slots=Bindings.Find(Id);FString Result;if(Slots) for(const auto& B:*Slots) if(B.Key.IsValid()) {if(!Result.IsEmpty()) Result+=TEXT(" / ");Result+=B.Label();}return Result.IsEmpty()?TEXT("未绑定"):Result; }
