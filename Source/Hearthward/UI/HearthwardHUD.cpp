#include "HearthwardHUD.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

void AHearthwardHUD::DrawHUD()
{
    Super::DrawHUD();
    APawn* Pawn = GetOwningPawn();
    if (ObservedPawn.Get() != Pawn)
    {
        ObservedPawn = Pawn;
        PreviousStatus = EHearthwardTimedActionStatus::Idle;
        InterruptionVisibleUntil = 0.0;
    }
    const auto* Action = Pawn ? Pawn->FindComponentByClass<UHearthwardTimedActionComponent>() : nullptr;
    if (!Canvas || !Action) return;

    const auto Status = Action->GetStatus();
    const double Now = GetWorld()->GetTimeSeconds();
    if (Status == EHearthwardTimedActionStatus::Interrupted && PreviousStatus != Status)
    {
        InterruptionVisibleUntil = Now + 1.5;
    }
    PreviousStatus = Status;
    const bool Running = Status == EHearthwardTimedActionStatus::Running;
    const bool Interrupted = Status == EHearthwardTimedActionStatus::Interrupted && Now < InterruptionVisibleUntil;
    if (!Running && !Interrupted) return;

    const float Scale = FMath::Clamp(Canvas->SizeY / 900.0f, 0.65f, 1.25f);
    const float Width = FMath::Min(360.0f * Scale, Canvas->SizeX - 32.0f);
    const float Left = (Canvas->SizeX - Width) * 0.5f;
    const float Top = Canvas->SizeY * 0.82f;
    const float Pad = 18.0f * Scale;
    const FLinearColor Accent = Interrupted ? FLinearColor(0.88f, 0.48f, 0.30f) : FLinearColor(0.92f, 0.73f, 0.38f);
    DrawRect(FLinearColor(0.035f, 0.042f, 0.042f, 0.94f), Left, Top, Width, 68.0f * Scale);
    DrawRect(Accent, Left, Top, 3.0f * Scale, 68.0f * Scale);

    const FText Label = Interrupted ? NSLOCTEXT("Hearthward", "ActionInterrupted", "已中断")
        : GetWorld()->IsPaused() ? NSLOCTEXT("Hearthward", "ActionPaused", "已暂停")
        : NSLOCTEXT("Hearthward", "ActionInProgress", "进行中");
    UFont* Font = GEngine->GetMediumFont();
    const float FontScale = 1.6f * Scale;
    DrawText(Label.ToString(), FLinearColor(0.95f, 0.94f, 0.88f), Left + Pad, Top + 10.0f * Scale, Font, FontScale);
    if (Running)
    {
        const double Elapsed = Action->GetElapsedSeconds();
        const double Duration = FHearthwardTimedActionState::DurationSeconds;
        const float Fraction = static_cast<float>(FMath::Clamp(Elapsed / Duration, 0.0, 1.0));
        FNumberFormattingOptions NumberFormat;
        NumberFormat.SetMinimumFractionalDigits(1).SetMaximumFractionalDigits(1);
        const FString Remaining = FText::Format(NSLOCTEXT("Hearthward", "ActionSecondsRemaining", "{0} 秒"),
            FText::AsNumber(FMath::Max(0.0, Duration - Elapsed), &NumberFormat)).ToString();
        float TextWidth = 0.0f, TextHeight = 0.0f;
        GetTextSize(Remaining, TextWidth, TextHeight, Font, FontScale);
        DrawText(Remaining, FLinearColor(0.95f, 0.94f, 0.88f), Left + Width - Pad - TextWidth,
            Top + 10.0f * Scale, Font, FontScale);
        const float BarWidth = Width - 2.0f * Pad;
        DrawRect(FLinearColor(0.20f, 0.22f, 0.21f), Left + Pad, Top + 45.0f * Scale, BarWidth, 5.0f * Scale);
        DrawRect(Accent, Left + Pad, Top + 45.0f * Scale, BarWidth * Fraction, 5.0f * Scale);
    }
}
