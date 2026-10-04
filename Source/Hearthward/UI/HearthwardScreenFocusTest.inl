// Explicit, isolated PIE regression fixture; never included in Shipping behavior.
#include "HearthwardScreenWidget.h"
#if !UE_BUILD_SHIPPING
#include "HearthwardHUD.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace
{
void VerifyModalFocus(UWorld* World)
{
    const FString Run=FPlatformMisc::GetEnvironmentVariable(TEXT("HEARTHWARD_TITLE_RUN"));
    // Do not run against a player's ordinary session or write without the QA run context.
    if(!World || World->WorldType!=EWorldType::PIE || !Run.StartsWith(TEXT("verify_"))) return;
    auto Report=MakeShared<FJsonObject>();
    auto Checks=MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Captures;
    bool Passed=true;
    auto Check=[&](const TCHAR* Name,bool Value){Checks->SetBoolField(Name,Value);Passed &= Value;};
    auto* Controller=World->GetFirstPlayerController();
    auto* HUD=Controller?Cast<AHearthwardHUD>(Controller->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr;
    Check(TEXT("screen_available"),UI!=nullptr);
    if(UI)
    {
        const FGeometry Geometry=FGeometry::MakeRoot(FVector2D(1672,941),FSlateLayoutTransform());
        const FModifierKeysState Modifiers;
        FVector2D Pointer(100,100);
        auto Event=[&](FVector2D Position,FVector2D Previous,FKey Button=FKey())
        {
            TSet<FKey> Pressed;if(Button.IsValid()) Pressed.Add(Button);
            return FPointerEvent(0,Position,Previous,Pressed,Button,0,Modifiers);
        };
        auto Move=[&](FVector2D Position)
        { UI->NativeOnMouseMove(Geometry,Event(Position,Pointer));Pointer=Position; };
        auto Click=[&](FVector2D Position)
        { UI->NativeOnMouseButtonDown(Geometry,Event(Position,Pointer,EKeys::LeftMouseButton));Pointer=Position; };
        auto Key=[&](FKey Value)
        { UI->NativeOnKeyDown(Geometry,FKeyEvent(Value,Modifiers,0,false,0,0)); };
        auto State=[&](bool Confirm,bool Cancel)
        { return UI->IsActionHighlighted(TEXT("confirm"))==Confirm && UI->IsActionHighlighted(TEXT("cancel"))==Cancel; };
        auto Stable=[&](const TCHAR* Name,bool Confirm,bool Cancel,bool SendStationaryMove=false)
        {
            bool Correct=State(Confirm,Cancel);
            for(int32 Tick=0;Tick<30;++Tick)
            {
                UI->NativeTick(Geometry,.21f);
                Correct &= State(Confirm,Cancel);
                if(SendStationaryMove) UI->NativeOnMouseMove(Geometry,Event(Pointer,Pointer));
                Correct &= State(Confirm,Cancel);
            }
            Check(Name,Correct);
        };
        auto Capture=[&](const TCHAR* Name)
        {
            const FString Filename=Run+TEXT("-")+Name;
            const bool Saved=UI->CaptureUI(Filename,1672,941,true);
            Check(*(FString(TEXT("capture_"))+Name),Saved);
            if(Saved) Captures.Add(MakeShared<FJsonValueString>(Filename+TEXT(".png")));
        };
        UI->OpenPage(TEXT("title"));
        for(int32 I=0;I<5 && UI->GetTitleSelection()!=TEXT("ask:quit");++I) UI->ExecuteAction(TEXT("title.next"));
        Check(TEXT("quit_click_target"),UI->ActionAt({836,760})==TEXT("ask:quit"));
        Click({836,760});
        Check(TEXT("mouse_opened_modal"),UI->ActionAt({960,535})==TEXT("cancel"));
        Stable(TEXT("mouse_open_no_default_glow_30_refreshes"),false,false);
        Capture(TEXT("quit-mouse-outside"));
        Move({960,535});
        Stable(TEXT("cancel_hover_stays_on_30_refreshes"),false,true,true);
        Capture(TEXT("quit-cancel-hover"));
        Move({100,100});
        Stable(TEXT("cancel_mouse_leave_stays_off_30_refreshes"),false,false,true);
        Capture(TEXT("quit-mouse-away"));
        Move({660,535});
        Stable(TEXT("confirm_hover_only_30_refreshes"),true,false);
        Move({960,535});
        Stable(TEXT("switch_hover_to_cancel_30_refreshes"),false,true);
        UI->NativeOnMouseLeave(Event(Pointer,Pointer));
        Stable(TEXT("widget_leave_clears_glow_30_refreshes"),false,false);
        Move({100,100});
        Key(EKeys::Left);
        Stable(TEXT("keyboard_confirm_stays_on_30_refreshes"),true,false,true);
        Capture(TEXT("quit-keyboard-confirm"));
        UI->NativeOnMouseWheel(Geometry,FPointerEvent(0,Pointer,Pointer,TSet<FKey>(),FKey(),-1,Modifiers));
        Stable(TEXT("ignored_modal_wheel_keeps_keyboard_selection"),true,false,true);
        Key(EKeys::Right);
        Stable(TEXT("keyboard_cancel_stays_on_30_refreshes"),false,true,true);
        Capture(TEXT("quit-keyboard-cancel"));
        Move({101,100});
        Stable(TEXT("mouse_takes_over_clears_keyboard_glow"),false,false,true);
        Key(EKeys::Enter);
        Check(TEXT("mouse_mode_enter_safely_cancels"),UI->ActionAt({960,535})!=TEXT("cancel") && UI->GetPage()==TEXT("title"));
        Check(TEXT("quit_selection_preserved"),UI->GetTitleSelection()==TEXT("ask:quit"));
        Key(EKeys::Enter);
        Check(TEXT("keyboard_enter_opens_modal"),UI->ActionAt({960,535})==TEXT("cancel"));
        Stable(TEXT("keyboard_open_default_cancel_stable"),false,true,true);
        Key(EKeys::Escape);
        Check(TEXT("keyboard_escape_closes_modal"),UI->ActionAt({960,535})!=TEXT("cancel"));
        Click({836,760});Move({960,535});Click({960,535});
        Check(TEXT("mouse_cancel_closes_modal"),UI->ActionAt({960,535})!=TEXT("cancel") && UI->GetPage()==TEXT("title"));
        Click({836,760});Key(EKeys::Left);Click({100,100});
        Stable(TEXT("blank_mouse_click_clears_keyboard_glow"),false,false);
        Key(EKeys::Enter);
        Check(TEXT("blank_mouse_click_enter_safely_cancels"),UI->ActionAt({960,535})!=TEXT("cancel"));
        UI->OpenPage(TEXT("title"));
    }
    Report->SetBoolField(TEXT("passed"),Passed);
    Report->SetObjectField(TEXT("checks"),Checks);
    Report->SetArrayField(TEXT("captures"),Captures);
    Report->SetStringField(TEXT("input"),TEXT("Native widget pointer/key events, not physical Windows input"));
    Report->SetNumberField(TEXT("refreshes_per_stage"),30);
    FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("TitleWheel")/Run/TEXT("modal-focus.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
FAutoConsoleCommandWithWorld VerifyModalFocusCommand(
    TEXT("Hearthward.UI.VerifyModalFocus"),TEXT("Run the isolated PIE quit-modal focus regression."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&VerifyModalFocus));
}
#endif
