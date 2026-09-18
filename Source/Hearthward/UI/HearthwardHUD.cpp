#include "HearthwardHUD.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Interaction/HearthwardInteractionComponent.h"
#include "../Interaction/HearthwardInteractionTargetComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

void AHearthwardHUD::ToggleInventory()
{
    CloseSaveMenu();
    CloseDialogue();
    if (bInventoryOpen)
    {
        if (bPausedByInventory) UGameplayStatics::SetGamePaused(this, false);
        bPausedByInventory = false;
        bInventoryOpen = false;
        return;
    }
    if (!GetOwningPawn()) return;
    // Preserve a pause that was already active before opening this panel.
    bPausedByInventory = !GetWorld()->IsPaused() && UGameplayStatics::SetGamePaused(this, true);
    bInventoryOpen = GetWorld()->IsPaused();
}

void AHearthwardHUD::DrawInventory(const UHearthwardInventoryComponent& Inventory)
{
    const float Scale = FMath::Clamp(Canvas->SizeY / 900.0f, 0.65f, 1.25f);
    const float Width = 560.0f * Scale;
    const float Left = (Canvas->SizeX - Width) * 0.5f;
    const float Top = Canvas->SizeY * 0.15f;
    const float Pad = 24.0f * Scale;
    const FLinearColor TextColor(0.95f, 0.94f, 0.88f);
    const FLinearColor Muted(0.70f, 0.74f, 0.71f);
    auto Text = [&](const FString& Value, float X, float Y, FLinearColor Color)
    {
        DrawText(Value, Color, Left + X * Scale, Top + Y * Scale, GEngine->GetMediumFont(), 1.6f * Scale);
    };
    DrawRect(FLinearColor(0.035f, 0.042f, 0.042f, 0.98f), Left, Top, Width, 410.0f * Scale);
    DrawRect(FLinearColor(0.92f, 0.73f, 0.38f), Left, Top, Width, 3.0f * Scale);
    Text(NSLOCTEXT("Hearthward", "InventoryTitle", "背包").ToString(), 24, 18, TextColor);
    Text(NSLOCTEXT("Hearthward", "InventoryPaused", "已暂停").ToString(), 420, 18, Muted);
    Text(NSLOCTEXT("Hearthward", "InventoryItem", "物品").ToString(), 24, 72, Muted);
    Text(NSLOCTEXT("Hearthward", "InventoryCount", "数量").ToString(), 290, 72, Muted);
    Text(NSLOCTEXT("Hearthward", "InventoryUnitWeight", "单重").ToString(), 420, 72, Muted);
    DrawRect(FLinearColor(0.20f, 0.22f, 0.21f), Left + Pad, Top + 105 * Scale, Width - 2 * Pad, Scale);
    FNumberFormattingOptions Format;
    Format.SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);
    int32 Row = 0;
    for (const auto& Item : HearthwardBasicItems())
    {
        const int32 Count = Inventory.GetItemCount(Item.Id);
        if (Count == 0) continue;
        const float Y = 116.0f + Row++ * 36.0f;
        Text(Item.DisplayName.ToString(), 24, Y, TextColor);
        Text(FText::AsNumber(Count).ToString(), 290, Y, TextColor);
        Text(FText::AsNumber(Item.WeightHundredths / 100.0, &Format).ToString(), 420, Y, TextColor);
    }
    if (Row == 0) Text(NSLOCTEXT("Hearthward", "InventoryEmpty", "背包为空").ToString(), 24, 132, Muted);
    Text(FText::Format(NSLOCTEXT("Hearthward", "InventoryTotalWeight", "负重 {0} / {1}"),
        FText::AsNumber(Inventory.GetWeight(), &Format), FText::AsNumber(Inventory.GetCapacity())).ToString(), 24, 326, TextColor);
    Text(NSLOCTEXT("Hearthward", "InventoryClose", "Tab 关闭").ToString(), 24, 372, Muted);
}

void AHearthwardHUD::DrawHUD()
{
    Super::DrawHUD();
    if (IsSaveMenuOpen()) return;
    APawn* Pawn = GetOwningPawn();
#if !UE_BUILD_SHIPPING
    if (Canvas && Pawn && !bInventoryOpen && !IsDialogueOpen())
    {
        bool Nearby = false;
        for (TActorIterator<AHearthwardCompanionFixture> It(GetWorld()); It; ++It)
            if (It->CanCommunicate(Pawn)) { Nearby = true; break; }
        const float Scale = FMath::Clamp(Canvas->SizeY / 900.0f, 0.65f, 1.25f);
        DrawText(Nearby ? TEXT("T 与弟弟交流 · 开发测试") : TEXT("需有30米内的伙伴才能交流"),
            FLinearColor(.95f,.94f,.88f), Canvas->SizeX - 390 * Scale, Canvas->SizeY - 53 * Scale,
            GEngine->GetMediumFont(), 1.5f * Scale);
    }
#endif
    if (Canvas && Pawn && !bInventoryOpen && !IsDialogueOpen())
    {
        const auto* AI = GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();
        if (AI && AI->CanDisplay() && !AI->GetStatus().IsEmpty())
        {
            const float Scale = FMath::Clamp(Canvas->SizeY / 900.0f, 0.65f, 1.25f);
            const float Left = 20 * Scale, Top = 28 * Scale;
            DrawRect(FLinearColor(0.035f, 0.042f, 0.042f, 0.94f), Left, Top, 620 * Scale, 156 * Scale);
            DrawText(AI->GetStatus(), FLinearColor(0.92f, 0.73f, 0.38f), Left + 16 * Scale, Top + 12 * Scale, GEngine->GetMediumFont(), 1.5f * Scale);
            FString Line = AI->GetNPCLine();
            if (Line.Len() > 96) Line = Line.Left(93) + TEXT("…");
            for (int32 Index = 0; Index * 32 < Line.Len(); ++Index)
                DrawText(Line.Mid(Index * 32, 32), FLinearColor::White, Left + 16 * Scale, Top + (48 + Index * 30) * Scale, GEngine->GetMediumFont(), 1.5f * Scale);
        }
    }
#if !UE_BUILD_SHIPPING
    if (Canvas && Pawn && !bInventoryOpen && !IsDialogueOpen())
    {
        for (TActorIterator<AHearthwardCompanionFixture> It(GetWorld()); It; ++It)
        {
            const auto* Companion = *It;
            if (!Companion->CanCommunicate(Pawn)) continue;
            const float Scale = FMath::Clamp(Canvas->SizeY / 900.0f, 0.65f, 1.25f);
            const float Left = Canvas->SizeX - 460 * Scale;
            const float Top = 28 * Scale;
            DrawRect(FLinearColor(0.035f, 0.042f, 0.042f, 0.94f), Left, Top, 440 * Scale, 184 * Scale);
            auto Line = [&](const FString& Text, float Y, FLinearColor Color)
            {
                DrawText(Text, Color, Left + 16 * Scale, Top + Y * Scale, GEngine->GetMediumFont(), 1.5f * Scale);
            };
            Line(TEXT("伙伴委托 · 开发测试"), 12, FLinearColor(0.92f, 0.73f, 0.38f));
            FString Status;
            using P = EHearthwardCompanionPhase;
            switch (Companion->GetPhase())
            {
            case P::Idle: Status = TEXT("等待委托"); break;
            case P::GoingToSource: Status = TEXT("前往资源点"); break;
            case P::Gathering: Status = TEXT("正在采集"); break;
            case P::Returning: Status = TEXT("携带物资返营"); break;
            case P::ReturningBlocked: Status = TEXT("受阻，尝试安全返营"); break;
            case P::WaitingAtCamp: Status = TEXT("已返营，等待玩家"); break;
            case P::Completed: Status = TEXT("目标已交付"); break;
            case P::Cancelled: Status = TEXT("已取消，保留携带物资"); break;
            }
            Line(Status, 44, FLinearColor::White);
            const auto* Item = HearthwardBasicItems().FindByPredicate([Companion](const auto& Def) { return Def.Id == Companion->GetItem(); });
            Line(FString::Printf(TEXT("%s入库 %d / %d   携带 %d"), Item ? *Item->DisplayName.ToString() : TEXT("物资"),
                Companion->GetDelivered(), Companion->GetRequested(), Companion->Bag->GetItemCount(Companion->GetItem())), 76, FLinearColor::White);
            Line(TEXT("采集 → 返营 → 入库"), 108, FLinearColor(0.70f, 0.74f, 0.71f));
            Line(Companion->BlockReason, 140, FLinearColor(0.88f, 0.65f, 0.50f));
            break;
        }
    }
#endif
    if (Canvas && Pawn)
    {
        if (const auto* Inventory = Pawn->FindComponentByClass<UHearthwardInventoryComponent>())
        {
            FNumberFormattingOptions Format;
            Format.SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);
            const FString Weight = FText::Format(NSLOCTEXT("Hearthward", "CarriedWeight", "负重 {0} / {1}"),
                FText::AsNumber(Inventory->GetWeight(), &Format), FText::AsNumber(Inventory->GetCapacity())).ToString();
            const float UIScale = FMath::Clamp(Canvas->SizeY / 900.0f, 0.65f, 1.25f);
            DrawRect(FLinearColor(0.035f, 0.042f, 0.042f, 0.94f), 20.0f * UIScale,
                Canvas->SizeY - 58.0f * UIScale, 260.0f * UIScale, 38.0f * UIScale);
            DrawText(Weight, FLinearColor(0.95f, 0.94f, 0.88f), 32.0f * UIScale,
                Canvas->SizeY - 53.0f * UIScale, GEngine->GetMediumFont(), 1.6f * UIScale);
            if (bInventoryOpen) DrawInventory(*Inventory);
        }
    }
    if (ObservedPawn.Get() != Pawn)
    {
        ObservedPawn = Pawn;
        PreviousStatus = EHearthwardTimedActionStatus::Idle;
        InterruptionVisibleUntil = 0.0;
        InteractionFeedbackRevision = 0;
        InteractionFeedbackUntil = 0.0;
    }
    if (Canvas && Pawn)
    {
        if (const auto* Interaction = Pawn->FindComponentByClass<UHearthwardInteractionComponent>())
        {
            if (!bInventoryOpen && !IsDialogueOpen())
            {
                if (const auto* Target = Interaction->GetNearestTarget())
                {
                    TArray<FString> Lines;
                    Target->GetInteractionPrompt(Pawn).ParseIntoArrayLines(Lines);
                    if (!Lines.IsEmpty())
                    {
                        const float Scale = FMath::Clamp(Canvas->SizeY / 900.0f, 0.65f, 1.25f);
                        const float Left = (Canvas->SizeX - 540 * Scale) * .5f, Top = Canvas->SizeY * .57f;
                        DrawRect(FLinearColor(.035f,.042f,.042f,.94f), Left, Top, 540 * Scale, 82 * Scale);
                        for (int32 I = 0; I < Lines.Num(); ++I)
                            DrawText(Lines[I], I == 0 ? FLinearColor(.92f,.73f,.38f) : FLinearColor::White,
                                Left + 18 * Scale, Top + (10 + I * 32) * Scale, GEngine->GetMediumFont(), 1.5f * Scale);
                    }
                }
            }
            if (InteractionFeedbackRevision != Interaction->GetFeedbackRevision())
            {
                InteractionFeedbackRevision = Interaction->GetFeedbackRevision();
                InteractionFeedbackUntil = GetWorld()->GetTimeSeconds() + 1.5;
            }
            FText Feedback;
            using S = EHearthwardInteractionStatus;
            switch (Interaction->GetStatus())
            {
            case S::Ready: Feedback = FText::FromString(Interaction->GetCompletionFeedback()); break;
            case S::InvalidTarget: Feedback = NSLOCTEXT("Hearthward", "InteractionNoTarget", "无有效交互目标"); break;
            case S::Unconfigured: Feedback = NSLOCTEXT("Hearthward", "InteractionUnconfigured", "交互距离未配置"); break;
            case S::OutOfRange: Feedback = NSLOCTEXT("Hearthward", "InteractionTooFar", "距离过远"); break;
            case S::Busy: Feedback = NSLOCTEXT("Hearthward", "InteractionBusy", "当前动作未结束"); break;
            case S::Paused: Feedback = NSLOCTEXT("Hearthward", "InteractionPaused", "已暂停"); break;
            case S::Interrupted:
            {
                const auto* Timer = Pawn->FindComponentByClass<UHearthwardTimedActionComponent>();
                const bool TimerNoticeVisible = Timer && Timer->GetStatus() == EHearthwardTimedActionStatus::Interrupted
                    && (PreviousStatus != EHearthwardTimedActionStatus::Interrupted || GetWorld()->GetTimeSeconds() < InterruptionVisibleUntil);
                if (!TimerNoticeVisible) Feedback = NSLOCTEXT("Hearthward", "InteractionStopped", "交互已中断");
                break;
            }
            default: break;
            }
            if (!bInventoryOpen && !Feedback.IsEmpty() && GetWorld()->GetTimeSeconds() < InteractionFeedbackUntil)
            {
                const float Scale = FMath::Clamp(Canvas->SizeY / 900.0f, 0.65f, 1.25f);
                const float Left = (Canvas->SizeX - 360.0f * Scale) * 0.5f;
                const float Top = Canvas->SizeY * 0.72f;
                DrawRect(FLinearColor(0.035f, 0.042f, 0.042f, 0.94f), Left, Top, 360.0f * Scale, 40.0f * Scale);
                DrawText(Feedback.ToString(), FLinearColor(0.95f, 0.94f, 0.88f), Left + 18.0f * Scale,
                    Top + 6.0f * Scale, GEngine->GetMediumFont(), 1.6f * Scale);
            }
        }
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
