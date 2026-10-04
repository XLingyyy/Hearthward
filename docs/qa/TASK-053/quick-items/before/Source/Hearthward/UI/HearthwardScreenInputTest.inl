// PROTOTYPE_ONLY: opt-in Development game-client regression, with a disposable save pool.
#if !UE_BUILD_SHIPPING
#include "HearthwardHUD.h"
#include "HearthwardLoadingSubsystem.h"
#include "../HearthwardCharacter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/SViewport.h"

namespace
{
struct FInputClientVerification
{
    TSharedPtr<FJsonObject> Report=MakeShared<FJsonObject>(),Checks=MakeShared<FJsonObject>();
    FString Output;
    int32 Step=0,Cycle=0,Menu=0,PauseMode=0;
    double Started=FPlatformTime::Seconds(),WaitUntil=0;
    float Yaw=0;
    bool Passed=true;
    void Check(const FString& Name,bool Value)
    { Checks->SetBoolField(Name,Value);Passed &= Value;UE_LOG(LogTemp,Log,TEXT("Input client check %s: %s"),*Name,Value?TEXT("PASS"):TEXT("FAIL")); }
    bool Finish()
    {
        Report->SetBoolField(TEXT("passed"),Passed);Report->SetObjectField(TEXT("checks"),Checks);
        Report->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        Report->SetStringField(TEXT("input"),TEXT("Standalone Development -game; keyboard and mouse events through GameViewportClient; Slate menu events. No physical Windows input."));
        FString Json;FJsonSerializer::Serialize(Report.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        return false;
    }
};

bool VerifyInputClient(float Delta)
{
    static FInputClientVerification Run;
    if(Run.Output.IsEmpty())
    {
        FString Pool;FGuid Id;
        if(!FParse::Value(FCommandLine::Get(),TEXT("HearthwardInputVerify="),Run.Output)
            || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id) || !Id.IsValid())return false;
    }
    if(FPlatformTime::Seconds()-Run.Started>240) {Run.Check(TEXT("completion_within_timeout"),false);return Run.Finish();}
    if(FPlatformTime::Seconds()<Run.WaitUntil)return true;
    UWorld* World=nullptr;
    if(!GEngine)return true;
    for(const auto& Context:GEngine->GetWorldContexts())
        if(Context.World() && Context.WorldType==EWorldType::Game) {World=Context.World();break;}
    if(!World || !World->HasBegunPlay() || !World->GetGameInstance())return true;
    auto* Controller=World->GetFirstPlayerController();
    auto* HUD=Controller?Cast<AHearthwardHUD>(Controller->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr;
    auto* Viewport=World->GetGameInstance()->GetGameViewportClient();
    auto* Loading=World->GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>();
    auto* Pawn=Controller?Cast<AHearthwardCharacter>(Controller->GetPawn()):nullptr;
    if(!UI || !Viewport || !Loading || !Pawn)return true;
    const FString Prefix=FString::Printf(TEXT("cycle_%d_"),Run.Cycle);
    const FGeometry Geometry=FGeometry::MakeRoot(FVector2D(1672,941),FSlateLayoutTransform());
    const FModifierKeysState Modifiers;
    auto SlateKey=[&](FKey Key)
    {
        const FKeyEvent Event(Key,Modifiers,0,false,0,0);
        if(!UI->NativeOnPreviewKeyDown(Geometry,Event).IsEventHandled())UI->NativeOnKeyDown(Geometry,Event);
    };
    auto Key=[&](FKey Value,EInputEvent Event)
    {
        Viewport->InputKey(FInputKeyEventArgs(Viewport->Viewport,FInputDeviceId::CreateFromInternalId(0),Value,Event,Event==IE_Released?0.f:1.f,false,FPlatformTime::Cycles64()));
        Controller->PlayerTick(.02f);
    };
    auto Click=[&](const FString& Action)
    {
        TSharedPtr<FJsonObject> Layout;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeLayout()),Layout);
        if(!Layout)return false;
        for(const auto& Value:Layout->GetArrayField(TEXT("components")))
        {
            const auto Row=Value->AsObject();FString Current;
            if(!Row->TryGetStringField(TEXT("action"),Current) || Current!=Action || !Row->GetBoolField(TEXT("visible")))continue;
            const auto& Rect=Row->GetArrayField(TEXT("rect"));
            const FVector2D Point(Rect[0]->AsNumber()+Rect[2]->AsNumber()/2,Rect[1]->AsNumber()+Rect[3]->AsNumber()/2);
            UI->NativeOnMouseButtonDown(Geometry,FPointerEvent(0,Point,Point,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,Modifiers));
            return true;
        }
        return false;
    };
    auto Capture=[&](const FString& Name)
    {
        const auto Widget=Viewport->GetGameViewportWidget();TArray<FColor> Pixels;FIntVector Size=FIntVector::ZeroValue;
        if(!Widget || !FSlateApplication::Get().TakeScreenshot(Widget.ToSharedRef(),Pixels,Size))return;
        TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
        FFileHelper::SaveArrayToFile(PNG,*(FPaths::GetPath(Run.Output)/Name+TEXT(".png")));
    };
    if(Loading->IsLoading())return true;
    if(Run.Step==0)
    {
        Run.Check(TEXT("boot_title"),UI->GetPage()==TEXT("title"));
        Run.Check(TEXT("boot_title_cursor_and_focus"),Controller->bShowMouseCursor && UI->HasKeyboardFocus());
        Capture(TEXT("boot-title"));
        Run.Check(TEXT("new_game_action"),UI->ExecuteAction(TEXT("new")));Run.Step=1;return true;
    }
    if(Run.Step==1)
    {
        if(UGameplayStatics::GetCurrentLevelName(World,true)!=TEXT("L_HearthwardWilds"))return true;
        Run.Check(Prefix+TEXT("hud"),UI->GetPage()==TEXT("hud"));
        Run.Check(Prefix+TEXT("viewport_accepts_game_input"),!Viewport->IgnoreInput());
        Run.Check(Prefix+TEXT("unpaused_movement"),!World->IsPaused() && Pawn->GetCharacterMovement()->MovementMode!=MOVE_None);
        Run.Check(Prefix+TEXT("game_cursor_hidden"),!Controller->bShowMouseCursor);
        // Fail at the actual frozen viewport in the before-fix run; never heal it in the test.
        if(!Run.Passed)return Run.Finish();
        Run.Yaw=Controller->GetControlRotation().Yaw;
        Viewport->InputKey(FInputKeyEventArgs(Viewport->Viewport,FInputDeviceId::CreateFromInternalId(0),EKeys::MouseX,20.f,.02f,1,FPlatformTime::Cycles64()));
        Key(EKeys::D,IE_Pressed);Run.WaitUntil=FPlatformTime::Seconds()+.35;Run.Step=2;return true;
    }
    if(Run.Step==2)
    {
        Run.Check(Prefix+TEXT("mouse_changes_camera"),!FMath::IsNearlyEqual(Run.Yaw,Controller->GetControlRotation().Yaw));
        Run.Check(Prefix+TEXT("keyboard_drives_movement"),!Pawn->GetLastMovementInputVector().IsNearlyZero());
        Key(EKeys::D,IE_Released);Capture(Prefix+TEXT("gameplay"));
        if(Run.Cycle>0) {Run.Step=6;return true;}
        Run.Step=3;Run.Menu=Run.PauseMode=0;
    }
    if(Run.Step==3)
    {
        static const FKey Keys[]={EKeys::Tab,EKeys::K,EKeys::J,EKeys::M,EKeys::F6,EKeys::P,EKeys::B,EKeys::T};
        static const FName Pages[]={TEXT("inventory"),TEXT("skills"),TEXT("journal"),TEXT("map"),TEXT("save"),TEXT("pause"),TEXT("building"),TEXT("dialogue")};
        GConfig->SetBool(TEXT("Hearthward.Survival"),TEXT("MenuPause"),Run.PauseMode==0,GGameUserSettingsIni);
        UI->OpenPage(TEXT("hud"));Key(Keys[Run.Menu],IE_Pressed);Key(Keys[Run.Menu],IE_Released);
        const FString Name=FString::Printf(TEXT("menu_pause_%d_%s"),Run.PauseMode,*Pages[Run.Menu].ToString());
        Run.Check(Name+TEXT("_shortcut"),UI->GetPage()==Pages[Run.Menu]);
        const bool ExpectedPause=Run.Menu==4 || Run.Menu==5 || (Run.PauseMode==0 && Run.Menu!=7);
        Run.Check(Name+TEXT("_pause_policy"),World->IsPaused()==ExpectedPause);
        SlateKey(Keys[Run.Menu]);
        Run.Check(Name+TEXT("_same_shortcut_closes"),UI->GetPage()==TEXT("hud") && !World->IsPaused() && !Viewport->IgnoreInput());
        Key(Keys[Run.Menu],IE_Pressed);Key(Keys[Run.Menu],IE_Released);
        SlateKey(EKeys::Escape);
        Run.Check(Name+TEXT("_escape_restores_game"),UI->GetPage()==TEXT("hud") && !World->IsPaused() && !Viewport->IgnoreInput());
        if(++Run.Menu==UE_ARRAY_COUNT(Keys)) {Run.Menu=0;if(++Run.PauseMode==2)Run.Step=4;}
        Run.WaitUntil=FPlatformTime::Seconds()+.08;return true;
    }
    if(Run.Step==4)
    {
        UI->OpenPage(TEXT("pause"));UI->ExecuteAction(TEXT("page:settings"));UI->ExecuteAction(TEXT("page:save"));
        SlateKey(EKeys::F6);Run.Check(TEXT("nested_save_returns_to_settings"),UI->GetPage()==TEXT("settings"));
        SlateKey(EKeys::Escape);Run.Check(TEXT("nested_settings_returns_to_pause"),UI->GetPage()==TEXT("pause") && World->IsPaused());
        SlateKey(EKeys::Escape);Run.Check(TEXT("nested_pause_returns_to_hud"),UI->GetPage()==TEXT("hud") && !World->IsPaused() && !Viewport->IgnoreInput());
        Run.Check(TEXT("runtime_bindings_valid"),HearthwardInput::Validate(World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings).IsEmpty());
        UI->OpenPage(TEXT("journal"));const FString Category=UI->GetCategory();
        UI->NativeOnKeyDown(Geometry,FKeyEvent(EKeys::E,Modifiers,0,false,0,0));
        Run.Check(TEXT("journal_next_category"),UI->GetCategory()!=Category);
        auto* Settings=World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
        const auto OriginalBindings=Settings->Bindings;
        Settings->Bindings.FindChecked(TEXT("journal.category.next"))[0]={EKeys::Y,FKey()};
        const FString BeforeRebound=UI->GetCategory();SlateKey(EKeys::E);
        Run.Check(TEXT("journal_old_category_key_inactive"),UI->GetCategory()==BeforeRebound);
        SlateKey(EKeys::Y);Run.Check(TEXT("journal_rebound_category_key"),UI->GetCategory()!=BeforeRebound);
        Settings->Bindings=OriginalBindings;
        UI->OpenPage(TEXT("hud"));UI->ExecuteAction(TEXT("page:settings"));
        Run.Check(TEXT("settings_pause_draft"),UI->ExecuteAction(TEXT("settings.change:pause:1")));
        Run.Check(TEXT("settings_apply_pause_on"),UI->ExecuteAction(TEXT("settings.apply")) && World->IsPaused());
        UI->ExecuteAction(TEXT("settings.change:pause:1"));
        Run.Check(TEXT("settings_apply_pause_off"),UI->ExecuteAction(TEXT("settings.apply")) && !World->IsPaused());
        UI->ExecuteAction(TEXT("settings.change:pause:1"));SlateKey(EKeys::Escape);
        UI->ExecuteAction(TEXT("page:settings"));
        Run.Check(TEXT("settings_unapplied_draft_discarded"),UI->ExecuteAction(TEXT("settings.apply")) && !World->IsPaused());
        Run.Check(TEXT("settings_capture_global_conflict"),UI->ExecuteAction(TEXT("settings.bind:ui.map:0")));SlateKey(EKeys::Q);
        Run.Check(TEXT("settings_rejects_category_conflict"),!UI->ExecuteAction(TEXT("settings.apply")) && UI->GetMessage().Contains(TEXT("冲突")));
        Run.Check(TEXT("settings_failed_apply_keeps_live_bindings"),Settings->Bindings.FindChecked(TEXT("ui.map"))[0].Key==EKeys::M);
        SlateKey(EKeys::Escape);UI->ExecuteAction(TEXT("page:settings"));
        UI->ExecuteAction(TEXT("settings.bind:ui.save:0"));SlateKey(EKeys::PageDown);
        Run.Check(TEXT("settings_rejects_navigation_conflict"),!UI->ExecuteAction(TEXT("settings.apply")));
        SlateKey(EKeys::Escape);UI->ExecuteAction(TEXT("page:settings"));
        UI->ExecuteAction(TEXT("settings.bind:ui.skills:0"));SlateKey(EKeys::L);
        Run.Check(TEXT("settings_apply_rebound_menu"),UI->ExecuteAction(TEXT("settings.apply")));
        SlateKey(EKeys::Escape);
        // Enhanced Input applies a rebuilt mapping at the next engine frame, just as after a real click.
        Run.Step=9;Run.WaitUntil=FPlatformTime::Seconds()+.15;return true;
    }
    if(Run.Step==9)
    {
        auto* Settings=World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
        Run.Check(TEXT("rebound_menu_persisted"),Settings->Bindings.FindChecked(TEXT("ui.skills"))[0].Key==EKeys::L);
        Key(EKeys::K,IE_Pressed);Key(EKeys::K,IE_Released);
        Run.Check(TEXT("old_skills_shortcut_inactive"),UI->GetPage()==TEXT("hud"));
        Key(EKeys::L,IE_Pressed);Key(EKeys::L,IE_Released);
        Run.Check(TEXT("rebound_skills_opens_through_viewport"),UI->GetPage()==TEXT("skills"));
        SlateKey(EKeys::L);Run.Check(TEXT("rebound_skills_closes"),UI->GetPage()==TEXT("hud") && !Viewport->IgnoreInput());
        UI->ExecuteAction(TEXT("page:settings"));UI->ExecuteAction(TEXT("settings.bind:ui.skills:0"));SlateKey(EKeys::K);
        Run.Check(TEXT("restore_skills_binding"),UI->ExecuteAction(TEXT("settings.apply")));SlateKey(EKeys::Escape);
        UI->ExecuteAction(TEXT("page:settings"));UI->ExecuteAction(TEXT("settings.bind:ui.map:1"));
        const FModifierKeysState Control(false,false,true,false,false,false,false,false,false);
        UI->NativeOnPreviewKeyDown(Geometry,FKeyEvent(EKeys::L,Control,0,false,0,0));
        Run.Check(TEXT("apply_secondary_modified_map_shortcut"),UI->ExecuteAction(TEXT("settings.apply")));SlateKey(EKeys::Escape);
        Run.Step=10;Run.WaitUntil=FPlatformTime::Seconds()+.15;return true;
    }
    if(Run.Step==10)
    {
        Key(EKeys::LeftControl,IE_Pressed);Key(EKeys::L,IE_Pressed);Key(EKeys::L,IE_Released);Key(EKeys::LeftControl,IE_Released);
        Run.Check(TEXT("secondary_modified_map_opens_through_viewport"),UI->GetPage()==TEXT("map"));
        const FModifierKeysState Control(false,false,true,false,false,false,false,false,false);
        UI->NativeOnKeyDown(Geometry,FKeyEvent(EKeys::L,Control,0,false,0,0));
        Run.Check(TEXT("secondary_modified_map_closes"),UI->GetPage()==TEXT("hud") && !Viewport->IgnoreInput());
        UI->ExecuteAction(TEXT("page:settings"));UI->ExecuteAction(TEXT("settings.clear:ui.map:1"));
        Run.Check(TEXT("restore_secondary_map_binding"),UI->ExecuteAction(TEXT("settings.apply")));SlateKey(EKeys::Escape);
        Run.Step=11;Run.WaitUntil=FPlatformTime::Seconds()+.15;return true;
    }
    if(Run.Step==11)
    {
        UI->OpenPage(TEXT("inventory"));SlateKey(EKeys::H);
        Run.Check(TEXT("inventory_repair_opens_current_equipment_page"),UI->GetPage()==TEXT("equipment"));
        SlateKey(EKeys::F);
        Run.Check(TEXT("equipment_repair_key_reaches_current_action"),!UI->GetMessage().IsEmpty());
        SlateKey(EKeys::Escape);
        // Exercise the load-failure path after a menu changes to gameplay beneath the overlay.
        UI->OpenPage(TEXT("pause"));Loading->BeginLoading();UI->OpenPage(TEXT("hud"));
        Run.Check(TEXT("loading_keeps_game_input_blocked"),Viewport->IgnoreInput());
        Loading->FinishSession(false);
        Run.Check(TEXT("load_failure_restores_current_hud"),!Loading->IsLoading() && !Viewport->IgnoreInput() && !World->IsPaused());
        UI->OpenPage(TEXT("pause"));
        Run.Check(TEXT("same_map_continue_confirmation"),UI->ExecuteAction(TEXT("continuePrompt")) && UI->ExecuteAction(TEXT("confirm")));
        Run.Step=5;return true;
    }
    if(Run.Step==5)
    {
        Run.Check(TEXT("same_map_continue_restores_hud"),UI->GetPage()==TEXT("hud") && !Viewport->IgnoreInput() && !World->IsPaused() && Pawn->GetCharacterMovement()->MovementMode!=MOVE_None);
        Run.Check(TEXT("same_map_new_game_confirmation"),UI->ExecuteAction(TEXT("newPrompt")) && UI->ExecuteAction(TEXT("confirm")));
        Run.Step=8;return true;
    }
    if(Run.Step==8)
    {
        Run.Check(TEXT("same_map_new_game_restores_hud"),UI->GetPage()==TEXT("hud") && !Viewport->IgnoreInput() && !World->IsPaused() && Pawn->GetCharacterMovement()->MovementMode!=MOVE_None);
        Run.Step=6;
    }
    if(Run.Step==6)
    {
        Key(EKeys::Escape,IE_Pressed);Key(EKeys::Escape,IE_Released);
        Run.Check(Prefix+TEXT("escape_opens_pause"),UI->GetPage()==TEXT("pause") && World->IsPaused());
        Run.Check(Prefix+TEXT("click_exit_game"),Click(TEXT("ask:title")));
        Run.Check(Prefix+TEXT("click_confirm_exit"),Click(TEXT("confirm")));
        Run.Step=7;return true;
    }
    if(Run.Step==7)
    {
        if(UGameplayStatics::GetCurrentLevelName(World,true)!=TEXT("L_Bootstrap"))return true;
        Run.Check(Prefix+TEXT("return_title"),UI->GetPage()==TEXT("title"));
        Run.Check(Prefix+TEXT("return_title_cursor_and_focus"),Controller->bShowMouseCursor && UI->HasKeyboardFocus());
        const FString Selection=UI->GetTitleSelection();UI->NativeOnKeyDown(Geometry,FKeyEvent(EKeys::Down,Modifiers,0,false,0,0));
        Run.Check(Prefix+TEXT("title_keyboard_navigation"),Selection!=UI->GetTitleSelection());
        Run.Check(Prefix+TEXT("click_title_settings"),Click(TEXT("page:settings")) && UI->GetPage()==TEXT("settings"));
        SlateKey(EKeys::Escape);Run.Check(Prefix+TEXT("title_settings_return"),UI->GetPage()==TEXT("title"));
        Capture(Prefix+TEXT("returned-title"));
        if(++Run.Cycle==3)return Run.Finish();
        Run.Check(Prefix+TEXT("continue_action"),UI->ExecuteAction(TEXT("continue")));Run.Step=1;return true;
    }
    return true;
}
const FTSTicker::FDelegateHandle InputClientVerificationTicker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&VerifyInputClient));
}
#endif
