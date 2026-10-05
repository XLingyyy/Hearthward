// PROTOTYPE_ONLY: opt-in standalone client check, using an isolated save pool.
#if !UE_BUILD_SHIPPING
#include "../Combat/HearthwardCombatComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"

namespace
{
struct FFeedbackClientVerification
{
    FString Output;
    TSharedPtr<FJsonObject> Report=MakeShared<FJsonObject>(),Checks=MakeShared<FJsonObject>();
    bool Passed=true;
    int32 Step=0;
    double Started=FPlatformTime::Seconds(),PromptAt=0,WaitUntil=0;
    void Check(const TCHAR* Name,bool Value)
    { Checks->SetBoolField(Name,Value);Passed &= Value;UE_LOG(LogTemp,Log,TEXT("Feedback client check %s: %s"),Name,Value?TEXT("PASS"):TEXT("FAIL")); }
    bool Finish()
    {
        Report->SetBoolField(TEXT("passed"),Passed);Report->SetObjectField(TEXT("checks"),Checks);
        Report->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        Report->SetStringField(TEXT("scope"),TEXT("Real combat rejection, item-use rejection and damage APIs; standalone rendered HUD and wall-clock timing. Active-progress states are an explicit paused fixture."));
        FString Json;FJsonSerializer::Serialize(Report.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
        FFileHelper::SaveStringToFile(Json,*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        return false;
    }
};

bool VerifyFeedbackClient(float)
{
    static FFeedbackClientVerification Run;
    if(Run.Output.IsEmpty())
    {
        FString Pool;FGuid Id;
        if(!FParse::Value(FCommandLine::Get(),TEXT("HearthwardFeedbackVerify="),Run.Output)
            || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id) || !Id.IsValid())return false;
    }
    const double Now=FPlatformTime::Seconds();
    if(Now-Run.Started>120) {Run.Check(TEXT("completion_within_timeout"),false);return Run.Finish();}
    if(Now<Run.WaitUntil || !GEngine)return true;
    UWorld* World=nullptr;
    for(const auto& Context:GEngine->GetWorldContexts())
        if(Context.World() && Context.WorldType==EWorldType::Game) {World=Context.World();break;}
    if(!World || !World->HasBegunPlay() || !World->GetGameInstance())return true;
    auto* Controller=World->GetFirstPlayerController();
    auto* HUD=Controller?Cast<AHearthwardHUD>(Controller->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr;
    if(!UI)return true;
    auto* Loading=World->GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>();
    if(Loading->IsLoading() || (Run.Step==0 && UI->GetPage()!=TEXT("hud")))return true;
    auto* Pawn=Controller->GetPawn().Get();
    auto* G=Pawn?Pawn->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    auto* Combat=Pawn?Pawn->FindComponentByClass<UHearthwardCombatComponent>():nullptr;
    auto* Survival=Pawn?Pawn->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    if(!G || !Combat || !Survival)return true;
    auto Snapshot=[&]()
    {
        TSharedPtr<FJsonObject> Object;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),Object);
        return Object;
    };
    auto Find=[&](const TCHAR* Id)->TSharedPtr<FJsonObject>
    {
        const auto Object=Snapshot();
        for(const auto& Value:Object->GetArrayField(TEXT("elements")))
            if(Value->AsObject()->GetStringField(TEXT("id"))==Id && Value->AsObject()->GetBoolField(TEXT("visible")))return Value->AsObject();
        return nullptr;
    };
    auto Visible=[&](const TCHAR* Id,const TCHAR* Text)
    { const auto E=Find(Id);return E && E->GetStringField(TEXT("text"))==Text; };
    auto Opacity=[&](const TCHAR* Id)
    { const auto E=Find(Id);return E?E->GetNumberField(TEXT("opacity")):0.; };
    auto Capture=[&](const TCHAR* Name)
    {
        const auto Widget=World->GetGameInstance()->GetGameViewportClient()->GetGameViewportWidget();
        TArray<FColor> Pixels;FIntVector Size=FIntVector::ZeroValue;
        if(!Widget || !FSlateApplication::Get().TakeScreenshot(Widget.ToSharedRef(),Pixels,Size))return;
        TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
        FFileHelper::SaveArrayToFile(PNG,*(FPaths::GetPath(Run.Output)/Name+TEXT(".png")));
    };
    auto TriggerExamples=[&]()
    {
        Combat->Cancel();G->Equipment.Remove(TEXT("weapon"));G->Health=G->MaxHealth();
        Survival->ResetTransient();
        Run.Check(TEXT("unarmed_attack_rejected"),!Combat->Attack());
        Run.Check(TEXT("medicine_at_full_health_rejected"),!G->UseItem(TEXT("medicine")));
        Run.Check(TEXT("damage_event_applied"),Survival->ReceiveDamage(1,FGuid::NewGuid(),World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()));
        UI->Refresh();Run.PromptAt=FPlatformTime::Seconds();
    };
    constexpr const TCHAR* CombatId=TEXT("hud.combat.state"),*GameplayId=TEXT("hud.gameplay.feedback"),*SurvivalId=TEXT("hud.survival.state");
    switch(Run.Step)
    {
    case 0:
        TriggerExamples();
        Run.Check(TEXT("three_example_messages_visible"),Visible(CombatId,TEXT("需要可用近战武器")) && Visible(GameplayId,TEXT("生命已满，无需用药")) && Visible(SurvivalId,TEXT("受到伤害")));
        ++Run.Step;Run.WaitUntil=Run.PromptAt+1;break;
    case 1:
        for(int32 I=0;I<40;++I)UI->Refresh();
        Run.Check(TEXT("messages_readable_at_one_second"),Opacity(CombatId)>.99 && Opacity(GameplayId)>.99 && Opacity(SurvivalId)>.99);
        Capture(TEXT("tips-one-second"));
        ++Run.Step;Run.WaitUntil=Run.PromptAt+1.8;break;
    case 2:
        UI->Refresh();
        Run.Report->SetNumberField(TEXT("fade_sample_elapsed"),Now-Run.PromptAt);
        Run.Check(TEXT("three_messages_fade_before_two_seconds"),Opacity(CombatId)>0 && Opacity(CombatId)<1 && Opacity(GameplayId)>0 && Opacity(GameplayId)<1 && Opacity(SurvivalId)>0 && Opacity(SurvivalId)<1);
        Capture(TEXT("tips-fading"));
        ++Run.Step;Run.WaitUntil=Run.PromptAt+2.15;break;
    case 3:
        UI->Refresh();
        Run.Check(TEXT("three_messages_gone_after_two_seconds"),!Find(CombatId) && !Find(GameplayId) && !Find(SurvivalId));
        Capture(TEXT("tips-expired"));
        TriggerExamples();
        Run.Check(TEXT("identical_results_show_again"),Visible(CombatId,TEXT("需要可用近战武器")) && Visible(GameplayId,TEXT("生命已满，无需用药")) && Visible(SurvivalId,TEXT("受到伤害")));
        UI->OpenPage(TEXT("pause"));
        Run.Check(TEXT("pause_menu_still_operates"),World->IsPaused() && UI->GetPage()==TEXT("pause"));
        ++Run.Step;Run.WaitUntil=FPlatformTime::Seconds()+2.2;break;
    case 4:
        UI->OpenPage(TEXT("hud"));UI->Refresh();
        Run.Check(TEXT("pause_return_does_not_replay_expired_results"),!Find(CombatId) && !Find(GameplayId) && !Find(SurvivalId));
        Run.Check(TEXT("game_input_restored_after_pause"),!World->IsPaused() && !World->GetGameInstance()->GetGameViewportClient()->IgnoreInput());
        G->SetFeedback(TEXT("本次动作已完成"));UI->Refresh();Run.PromptAt=FPlatformTime::Seconds();
        Run.Check(TEXT("new_gameplay_result_visible"),Visible(GameplayId,TEXT("本次动作已完成")));
        ++Run.Step;Run.WaitUntil=Run.PromptAt+1;break;
    case 5:
        Combat->Attack();UI->Refresh();
        Run.Check(TEXT("second_channel_visible"),Visible(CombatId,TEXT("需要可用近战武器")));
        ++Run.Step;Run.WaitUntil=Run.PromptAt+2.15;break;
    case 6:
        UI->Refresh();
        Run.Check(TEXT("channels_keep_independent_deadlines"),!Find(GameplayId) && Find(CombatId) && !Find(SurvivalId));
        // A paused fixture proves current progress remains visible beyond a notice's lifetime.
        UGameplayStatics::SetGamePaused(World,true);
        Survival->State.Medicine=TEXT("medicine");Survival->State.MedicineRemaining=3;
        Combat->Action=TEXT("reload");Combat->Elapsed=0;Combat->Duration=5;
        UI->Refresh();
        ++Run.Step;Run.WaitUntil=FPlatformTime::Seconds()+2.2;break;
    case 7:
        UI->Refresh();
        Run.Check(TEXT("live_medicine_progress_remains_visible"),Visible(SurvivalId,TEXT("用药：剩余 3.0 秒")) && Opacity(SurvivalId)>.99);
        Run.Check(TEXT("live_combat_progress_remains_visible"),Visible(CombatId,TEXT("装填 0.0 秒")) && Opacity(CombatId)>.99);
        Run.Check(TEXT("vitals_and_quick_slots_remain_visible"),Find(TEXT("hud.vitals.0.bar")) && Find(TEXT("hud.quick.name")));
        Capture(TEXT("live-progress"));
        Survival->State.Medicine=NAME_None;Survival->State.MedicineRemaining=0;Combat->Cancel();
        UGameplayStatics::SetGamePaused(World,false);
        return Run.Finish();
    }
    return true;
}
const FTSTicker::FDelegateHandle FeedbackClientVerificationTicker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&VerifyFeedbackClient),.01f);
}
#endif
