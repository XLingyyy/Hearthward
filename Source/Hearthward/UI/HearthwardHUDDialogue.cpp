#include "HearthwardHUD.h"
#include "HearthwardDialogueWidget.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void AHearthwardHUD::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    // R23: temporary prototype key, kept in the UI input component.
    EnableInput(GetOwningPlayerController());
    InputComponent->BindKey(EKeys::T,IE_Pressed,this,&AHearthwardHUD::ToggleDialogue);
#endif
}
void AHearthwardHUD::EndPlay(const EEndPlayReason::Type Reason)
{
    CloseDialogue();
    Super::EndPlay(Reason);
}
void AHearthwardHUD::ToggleDialogue()
{
    if(DialogueWidget) { CloseDialogue(); return; }
    if(!GetOwningPawn() || bInventoryOpen || GetWorld()->IsPaused()) return;
    DialogueCompanion.Reset();
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld()); It; ++It)
        if(It->CanCommunicate(GetOwningPawn())) { DialogueCompanion=*It; break; }
    if(!DialogueCompanion.IsValid()) return;
    auto* Player=GetOwningPlayerController();
    DialogueWidget=CreateWidget<UHearthwardDialogueWidget>(Player);
    DialogueWidget->SetIsFocusable(true);
    DialogueWidget->AddToViewport(20);
    DialogueFeedback.Reset();
    DialogueWidget->BindHUD(this);
    bPreviousCursor=Player->bShowMouseCursor;
    Player->SetIgnoreMoveInput(true);
    Player->SetIgnoreLookInput(true);
    Player->FlushPressedKeys();
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(DialogueWidget->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Player->SetInputMode(Mode);
    Player->bShowMouseCursor=true;
    DialogueWidget->FocusDraft();
}
void AHearthwardHUD::CloseDialogue()
{
    if(!DialogueWidget) return;
    DialogueWidget->RemoveFromParent();
    DialogueWidget=nullptr;
    if(auto* Player=GetOwningPlayerController())
    {
        Player->SetIgnoreMoveInput(false);
        Player->SetIgnoreLookInput(false);
        Player->FlushPressedKeys();
        Player->SetInputMode(FInputModeGameOnly());
        Player->bShowMouseCursor=bPreviousCursor;
    }
    // Prototype mirrors TASK-013: closing presentation does not cancel a command.
    DialogueCompanion.Reset();
    DialogueFeedback.Reset();
}
bool AHearthwardHUD::CanSendDialogue() const
{
    const auto* AI=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();
    return DialogueWidget && DialogueCompanion.IsValid() && DialogueCompanion->CanCommunicate(GetOwningPawn())
        && !GetWorld()->IsPaused() && AI && !AI->IsBusy();
}
bool AHearthwardHUD::SubmitDialogue(const FString& Text)
{
    const FString Value=Text.TrimStartAndEnd();
    if(Value.IsEmpty() || Value.Len()>1000) { DialogueFeedback=TEXT("请输入1—1000字的内容"); return false; }
    if(!CanSendDialogue()) return false;
    DialogueFeedback.Reset();
    return GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->SubmitPlayerText(GetOwningPawn(),DialogueCompanion.Get(),Value);
}
bool AHearthwardHUD::CanCancelDialogueReply() const
{
    const auto* AI=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();
    return DialogueCompanion.IsValid() && DialogueCompanion->CanCommunicate(GetOwningPawn()) && AI && AI->IsBusy();
}
void AHearthwardHUD::CancelDialogueReply()
{
    if(CanCancelDialogueReply()) GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->CancelPending();
    DialogueFeedback.Reset();
}
bool AHearthwardHUD::CanCancelDialogueTask() const
{
    if(!DialogueCompanion.IsValid() || !DialogueCompanion->CanCommunicate(GetOwningPawn()) || GetWorld()->IsPaused()) return false;
    using P=EHearthwardCompanionPhase;
    const auto Phase=DialogueCompanion->GetPhase();
    return Phase!=P::Idle && Phase!=P::Completed && Phase!=P::Cancelled;
}
void AHearthwardHUD::CancelDialogueTask()
{
    if(!CanCancelDialogueTask()) return;
    CancelDialogueReply();
    DialogueFeedback=DialogueCompanion->Cancel(GetOwningPawn()) ? TEXT("已取消委托，保留实际物资") : TEXT("当前无法取消委托");
}
FString AHearthwardHUD::GetDialogueStatus() const
{
    if(!DialogueCompanion.IsValid()) return TEXT("伙伴已不可用");
    if(!DialogueCompanion->CanCommunicate(GetOwningPawn())) return TEXT("已超出30米交流范围，请靠近后重试");
    if(GetWorld()->IsPaused()) return TEXT("游戏已暂停，恢复后可继续交流");
    if(!DialogueFeedback.IsEmpty()) return DialogueFeedback;
    const auto* AI=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();
    if(AI && AI->CanDisplay() && !AI->GetStatus().IsEmpty()) return AI->GetStatus();
    return TEXT("可以输入想说的话");
}
FString AHearthwardHUD::GetDialogueReply() const
{
    if(!DialogueCompanion.IsValid() || !DialogueCompanion->CanCommunicate(GetOwningPawn())) return FString();
    const auto* AI=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();
    return AI && AI->CanDisplay() ? AI->GetNPCLine() : FString();
}
FString AHearthwardHUD::GetDialogueProgress() const
{
    if(!DialogueCompanion.IsValid() || !DialogueCompanion->CanCommunicate(GetOwningPawn())) return FString();
    const auto* C=DialogueCompanion.Get();
    FString Phase;
    using P=EHearthwardCompanionPhase;
    switch(C->GetPhase())
    {
    case P::Idle: Phase=TEXT("等待委托"); break;
    case P::GoingToSource: Phase=TEXT("前往资源点"); break;
    case P::Gathering: Phase=TEXT("正在采集"); break;
    case P::Returning: Phase=TEXT("携带物资返营"); break;
    case P::ReturningBlocked: Phase=TEXT("受阻，尝试安全返营"); break;
    case P::WaitingAtCamp: Phase=TEXT("已返营，等待玩家"); break;
    case P::Completed: Phase=TEXT("目标已交付"); break;
    case P::Cancelled: Phase=TEXT("已取消，保留携带物资"); break;
    }
    if(C->GetRequested()==0) return Phase;
    const auto* Item=HearthwardBasicItems().FindByPredicate([C](const auto& Def){ return Def.Id==C->GetItem(); });
    FString Result=FString::Printf(TEXT("%s · %s入库 %d / %d · 携带 %d\n采集 → 返营 → 入库"),*Phase,
        Item ? *Item->DisplayName.ToString() : TEXT("物资"),C->GetDelivered(),C->GetRequested(),C->Bag->GetItemCount(C->GetItem()));
    if(!C->BlockReason.IsEmpty()) Result+=TEXT("\n")+C->BlockReason;
    return Result;
}
FString AHearthwardHUD::GetDialogueWeight() const
{
    const auto* Pawn=GetOwningPawn();
    const auto* Inventory=Pawn ? Pawn->FindComponentByClass<UHearthwardInventoryComponent>() : nullptr;
    return Inventory ? FString::Printf(TEXT("我的负重 %.2f / %.0f"),Inventory->GetWeight(),Inventory->GetCapacity()) : FString();
}
