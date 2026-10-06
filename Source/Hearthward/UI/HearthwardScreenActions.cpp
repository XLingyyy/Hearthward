#include "../Combat/HearthwardCombatComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "HearthwardScreenWidget.h"
#include "../HearthwardCharacter.h"
#include "Misc/ConfigCacheIni.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "HearthwardHUD.h"
#include "HearthwardLoadingSubsystem.h"
#include "Engine/GameInstance.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Update/HearthwardUpdateSubsystem.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Components/EditableTextBox.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

using namespace HearthwardData;
namespace
{
const FName NaturalMap(TEXT("/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds"));
AHearthwardCompanionFixture* Companion(UWorld* World)
{ for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It) return *It; return nullptr; }
bool NearStorage(UWorld* World,AActor* Player)
{
    for(TActorIterator<AActor> It(World);It;++It)
        if(const auto* R=It->FindComponentByClass<UHearthwardResourceInteractionComponent>(); R && R->CanAccessStorage(Player)) return true;
    return false;
}
}
bool UHearthwardScreenWidget::PrepareSession()
{
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    if(Save->IsPrototypeEnabled()) return true;
    if(UGameplayStatics::GetCurrentLevelName(GetWorld(),true)==TEXT("L_HearthwardWilds")) return Save->EnableNaturalWorld();
#if !UE_BUILD_SHIPPING
    if(!Companion(GetWorld())) UKismetSystemLibrary::ExecuteConsoleCommand(this,TEXT("Hearthward.Companion.CreateTest"),GetOwningPlayer());
    auto* G=Gameplay();
    if(!G->Enabled)
    {
        G->EnableAdventure();
        G->GrantInitialEquipment();
    }
    return Save->EnablePrototype();
#else
    return false;
#endif
}
bool UHearthwardScreenWidget::OpenSavePoint(const FHearthwardSavePoint& Point)
{
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    if(Point.World.NaturalWorld)
    {
        if(UGameplayStatics::GetCurrentLevelName(GetWorld(),true)==TEXT("L_HearthwardWilds"))
        {
            auto* Loading=GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>();
            Loading->BeginLoading();
            const bool FromTitle=Page==TEXT("title");
            const bool Loaded=PrepareSession() && Save->LoadPoint(Point.SaveId);
            Loading->FinishSession(Loaded);
            if(Loaded && FromTitle)
            {
                auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
                if(Campaign->State.Victory && Campaign->State.Facts.Contains(TEXT("home_saved")))Campaign->Record(TEXT("home_continued"));
            }
            return Loaded;
        }
        const FString Options=TEXT("game=/Script/Hearthward.HearthwardGameMode?HearthwardLoad=")
            +Point.SaveId.ToString(EGuidFormats::Digits);
        BeginMapTravel();
        UGameplayStatics::OpenLevel(this,NaturalMap,true,Options);
        return true;
    }
    return PrepareSession() && Save->LoadPoint(Point.SaveId);
}
void UHearthwardScreenWidget::BeginMapTravel()
{
    GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>()->BeginLoading();
    if(OwnPause)UGameplayStatics::SetGamePaused(this,false);
    OwnPause=false;
    if(auto* Character=Cast<AHearthwardCharacter>(GetOwningPlayerPawn()))Character->ResetHeldInput();
}
bool UHearthwardScreenWidget::ExecuteAction(const FString& InAction)
{
    const FString Action=InAction; // Refresh can invalidate the element that supplied this string.
    if(GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>()->IsLoading())return false;
    if(Action==TEXT("title.prev") || Action==TEXT("title.next"))
        return NavigateTitle(Action==TEXT("title.next")?1:-1);
    // A confirmation owns input until the player confirms or cancels it.
    if(!ConfirmAction.IsEmpty() && Action!=TEXT("confirm") && Action!=TEXT("cancel")
        && !(ConfirmAction==TEXT("compat.resolve") && (Action==TEXT("compat.next") || Action==TEXT("compat.prev") || Action==TEXT("compat.folder") || Action==TEXT("update.open")))
        && !(ConfirmAction==TEXT("resetAgreements") && (Action==TEXT("resetPrev") || Action==TEXT("resetNext")))) return false;
    if(Action==TEXT("resetPrev") || Action==TEXT("resetNext"))
    { ResetScroll=FMath::Clamp(ResetScroll+(Action==TEXT("resetNext")?2:-2),0,FMath::Max(0,ResetItems.Num()-2));Refresh();return true; }
    if(ConfirmAction==TEXT("settings.display") && (Action==TEXT("confirm") || Action==TEXT("cancel")))
    {FinishDisplayChange(Action==TEXT("confirm"));return true;}
    MessageUntil=FPlatformTime::Seconds()+4;
    if(Action.StartsWith(TEXT("hud.quick.")))
    {
        if(Page!=TEXT("hud")) return false;
        const int32 Count=Theme->GetObjectField(TEXT("inventory"))->GetArrayField(TEXT("quickSlots")).Num();
        if(Count<=0) return false;
        int32 Selection=HUDQuickSelection;
        if(Action.StartsWith(TEXT("hud.quick.select:"))) Selection=FCString::Atoi(*Action.Mid(17));
        else return false;
        if(Selection<0 || Selection>=Count) return false;
        // Selection is UI state. A different slot also cancels a pending bow release.
        if(Selection!=HUDQuickSelection) if(auto* C=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardCombatComponent>()) C->Aim(false);
        HUDQuickSelection=Selection; Refresh(); return true;
    }
    if(Action.StartsWith(TEXT("dialogue.")))return ExecuteDialogueAction(Action);
    if(Action.StartsWith(TEXT("camp.")))return ExecuteCampAction(Action);
    if(Action.StartsWith(TEXT("gear.")))return ExecuteEquipmentAction(Action);
    if(Action.StartsWith(TEXT("nature.")))return ExecuteNatureAction(Action);
    if(Action.StartsWith(TEXT("settings.")))return ExecuteSettingsAction(Action);
    if(Action==TEXT("buildPrev") || Action==TEXT("buildNext"))
    {
        const auto& Buildings=Rows(TEXT("buildings"));
        Scroll=FMath::Clamp(Scroll+(Action==TEXT("buildPrev")?-4:4),0,FMath::Max(0,Buildings.Num()-4));
        if(Buildings.IsValidIndex(Scroll))SelectedBuilding=FName(*Text(Buildings[Scroll]->AsObject(),TEXT("id")));
        Refresh();return true;
    }
    if(Action.StartsWith(TEXT("building.select:")))
    {
        const FString Id=Action.Mid(16);
        if(!Find(TEXT("buildings"),Id))return false;
        SelectedBuilding=FName(*Id);Refresh();return true;
    }
    auto* G=Gameplay(); auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    if(Action==TEXT("update.open"))
    {GetGameInstance()->GetSubsystem<UHearthwardUpdateSubsystem>()->OpenReleasePage();return true;}
    if(Action==TEXT("update.check"))
    {GetGameInstance()->GetSubsystem<UHearthwardUpdateSubsystem>()->Check(true);Refresh();return true;}
    if(Action==TEXT("compat.folder"))
    {FPlatformProcess::ExploreFolder(*(FPaths::ProjectSavedDir()/TEXT("SaveGames/HearthwardPrototype")));return true;}
    if(Action==TEXT("compat.show"))
    {
        if(Save->LoadPointIndex()){Message=TEXT("存档兼容，原始进度已保留。");Refresh();return true;}
        ConfirmAction=TEXT("compat.resolve");CompatibilityScroll=0;Refresh();return true;
    }
    if(Action==TEXT("compat.next") || Action==TEXT("compat.prev"))
    {CompatibilityScroll=FMath::Clamp(CompatibilityScroll+(Action==TEXT("compat.next")?2:-2),0,FMath::Max(0,((Save->GetCompatibility().Changes.Num()-1)/2)*2));Refresh();return true;}
    if(Action==TEXT("confirm") && ConfirmAction==TEXT("compat.resolve")
        && (!Save->GetCompatibility().CanRepair || CompatibilityScroll+2<Save->GetCompatibility().Changes.Num()))return false;
    if(Action==TEXT("compat.resolve"))
    {
        const bool Done=Save->ResolveSaveConflicts();Message=Save->GetStatus();
        if(!Done){ConfirmAction=TEXT("compat.resolve");CompatibilityScroll=0;}
        Refresh();return Done;
    }
    if(Page==TEXT("title") && (Action==TEXT("new") || Action==TEXT("continue") || Action==TEXT("newPrompt") || Action==TEXT("continuePrompt") || Action==TEXT("page:save")))
        if(!Save->LoadPointIndex()){ConfirmAction=TEXT("compat.resolve");CompatibilityScroll=0;Refresh();return false;}
    auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    auto* AI=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();
    bool Success=true;
    if(Action.StartsWith(TEXT("memory")))
    {
        auto* C=Companion(GetWorld());
        if(Page!=TEXT("memory") || (MemoryEpoch!=Store->GetTimelineEpoch() || MemoryRevision!=AI->GetMemoryRevision()) || !C || !C->CanCommunicate(GetOwningPlayerPawn()))
        { Message=TEXT("记录访问已失效，请重新靠近弟弟打开"); Refresh(); return false; }
        if(Action==TEXT("memoryNew")) { SelectedMemory.Invalidate(); MemoryKind=TEXT("claim"); MemoryBlockedItem=TEXT("wood"); Draft->SetText(FText::GetEmpty()); }
        else if(Action==TEXT("memoryNextItem"))
        {
            const auto& Items=HearthwardBasicItems();
            const int32 Index=Items.IndexOfByPredicate([&](const auto& I){return I.Id==MemoryBlockedItem;});
            MemoryBlockedItem=Items[(Index+1)%Items.Num()].Id;
        }
        else if(Action.StartsWith(TEXT("memoryKind:")))
        {
            const FName Kind(*Action.Mid(11));
            if(Kind!=TEXT("claim") && Kind!=TEXT("preference") && Kind!=TEXT("agreement") && Kind!=TEXT("collection_ban")) return false;
            MemoryKind=Kind;
        }
        else if(Action.StartsWith(TEXT("memorySelect:")))
        {
            FGuid Id; if(!FGuid::Parse(Action.Mid(13),Id)) return false;
            Success=false;
            for(const auto& R:AI->GetPlayerMemories()) if(R.Id==Id && !R.Revoked)
            { SelectedMemory=Id; MemoryKind=R.Kind; MemoryBlockedItem=R.BlockedItem.IsNone()?FName(TEXT("wood")):R.BlockedItem; Draft->SetText(FText::FromString(R.Text)); Success=true; break; }
        }
        else if(Action==TEXT("memorySave"))
        {
            FString Content=Draft->GetText().ToString();
            if(MemoryKind==TEXT("collection_ban") && Content.TrimStartAndEnd().IsEmpty()) Content=TEXT("禁止采集所选物品");
            Success=AI->PutPlayerMemory(GetOwningPlayerPawn(),C,SelectedMemory,MemoryKind,Content,MemoryBlockedItem);
            Message=AI->GetStatus();
            if(Success) { SelectedMemory.Invalidate(); Draft->SetText(FText::GetEmpty()); }
        }
        else if(Action==TEXT("memoryRevoke"))
        {
            Success=AI->RevokePlayerMemory(GetOwningPlayerPawn(),C,SelectedMemory); Message=AI->GetStatus();
            if(Success) { SelectedMemory.Invalidate(); Draft->SetText(FText::GetEmpty()); }
        }
        else if(Action==TEXT("memoryReset"))
        {
            ResetItems.Reset();ResetEpoch=Store->GetTimelineEpoch();ResetRevision=AI->GetMemoryRevision();
            ResetCommand=C->GetCommandId();bResetActive=C->EquipmentBusy();ResetCandidate=AI->GetCandidateId();ResetScroll=0;
            if(bResetActive)ResetItems.Add(FString::Printf(TEXT("当前委托 [%s]：%s，完成 %d/%d"),
                *ResetCommand.ToString().Left(8),*HearthwardAgent::ItemText(C->GetGoal().Item),C->GetDelivered(),C->GetRequested()));
            if(ResetCandidate.IsValid())ResetItems.Add(TEXT("未确认任务卡 [")+ResetCandidate.ToString().Left(8)+TEXT("]"));
            for(const auto& R:AI->GetPlayerMemories())
                if(!R.Revoked && (R.Kind==TEXT("agreement") || R.Kind==TEXT("collection_ban") || R.Kind==TEXT("typed_constraint")))
                    ResetItems.Add(FString::Printf(TEXT("约定 [%s] %s：%s"),*R.Id.ToString().Left(8),
                        R.Kind==TEXT("typed_constraint")?*R.Constraint:(R.Kind==TEXT("collection_ban")?TEXT("采集限制"):TEXT("文字约定")),*R.Text));
            if(ResetItems.IsEmpty()){Message=TEXT("当前没有委托或约定需要取消");Refresh();return false;}
            ConfirmAction=TEXT("resetAgreements");Refresh();return true;
        }
        else return false;
        MemoryRevision=AI->GetMemoryRevision(); Refresh(); return Success;
    }
    if(Action.StartsWith(TEXT("agentConfirm:")) || Action.StartsWith(TEXT("agentMore:")) || Action.StartsWith(TEXT("agentLess:")))
    {
        if(Page!=TEXT("dialogue"))return false;FString Verb,IdText;Action.Split(TEXT(":"),&Verb,&IdText);FGuid Id;if(!FGuid::Parse(IdText,Id))return false;
        Success=Verb==TEXT("agentConfirm")?AI->ConfirmCandidate(Id):AI->AdjustCandidate(Id,Verb==TEXT("agentMore")?1:-1);Message=AI->GetStatus();if(Success && Verb==TEXT("agentConfirm"))DialogueView=TEXT("home");Refresh();return Success;
    }
    if(Action==TEXT("agentInventory")) {if(Page!=TEXT("dialogue"))return false;Success=AI->QueryInventory(GetOwningPlayerPawn(),Companion(GetWorld()),TEXT("wood"));DialogueView=TEXT("home");Refresh();return Success;}
    if(Action==TEXT("agentRetryPath")) {if(Page!=TEXT("dialogue"))return false;auto* C=Companion(GetWorld());Success=C && C->ResumeBlocked(GetOwningPlayerPawn());Message=Success?TEXT("原委托已继续"):C?C->BlockReason:TEXT("请靠近弟弟");Refresh();return Success;}
    if(Action==TEXT("agentTypeNext") || Action==TEXT("agentItemNext") || Action==TEXT("agentInstanceNext") || Action==TEXT("agentSourceNext"))
    {
        if(Page!=TEXT("dialogue"))return false;
        TArray<const FHearthwardAgentCapability*> Caps;
        for(const auto& C:HearthwardAgent::Capabilities())
            if(C.Id==TEXT("collect") || C.Id==TEXT("store") || C.Id==TEXT("retrieve") || C.Id==TEXT("give") || C.Id==TEXT("fetch") || C.Id==TEXT("receive") || C.Id==TEXT("craft") || C.Id==TEXT("repair") || C.Id==TEXT("escort")) Caps.Add(&C);
        if(Action==TEXT("agentTypeNext")){AgentCapabilityIndex=(AgentCapabilityIndex+1)%Caps.Num();AgentItemIndex=0;AgentInstanceIndex=0;AgentSourceIndex=0;}
        else if(Action==TEXT("agentItemNext")){AgentItemIndex=(AgentItemIndex+1)%Caps[AgentCapabilityIndex]->Items.Num();AgentInstanceIndex=0;}
        else if(Action==TEXT("agentSourceNext") && Caps[AgentCapabilityIndex]->Id==TEXT("store"))
            AgentSourceIndex=(AgentSourceIndex+1)%Caps[AgentCapabilityIndex]->Sources.Num();
        else
        {
            auto* Brother=Companion(GetWorld());int32 Count=0;
            if(Brother && Caps[AgentCapabilityIndex]->Id==TEXT("repair"))
                for(const auto& Instance:Brother->Bag->Snapshot().Instances)
                    if(Instance.Definition==Caps[AgentCapabilityIndex]->Items[AgentItemIndex])++Count;
            if(Count>0)AgentInstanceIndex=(AgentInstanceIndex+1)%Count;
        }
        Refresh();return true;
    }
    if(Action==TEXT("agentCollectCard"))
    {
        if(Page!=TEXT("dialogue"))return false;
        TArray<const FHearthwardAgentCapability*> Caps;
        for(const auto& C:HearthwardAgent::Capabilities())
            if(C.Id==TEXT("collect") || C.Id==TEXT("store") || C.Id==TEXT("retrieve") || C.Id==TEXT("give") || C.Id==TEXT("fetch") || C.Id==TEXT("receive") || C.Id==TEXT("craft") || C.Id==TEXT("repair") || C.Id==TEXT("escort")) Caps.Add(&C);
        AgentCapabilityIndex=FMath::Clamp(AgentCapabilityIndex,0,Caps.Num()-1);
        AgentItemIndex=FMath::Clamp(AgentItemIndex,0,Caps[AgentCapabilityIndex]->Items.Num()-1);
        const auto& C=*Caps[AgentCapabilityIndex];FHearthwardAgentGoal Goal;Goal.Intent=C.Id;Goal.Item=C.Items[AgentItemIndex];Goal.Quantity=1;Goal.QuantityMode=C.QuantityMode;
        Goal.SourceRef=C.Id==TEXT("store")?C.Sources[FMath::Clamp(AgentSourceIndex,0,C.Sources.Num()-1)]:C.Sources[0];
        auto* Brother=Companion(GetWorld());
        if(C.Id==TEXT("repair") && Brother)
        {
            TArray<FGuid> Instances;
            for(const auto& Instance:Brother->Bag->Snapshot().Instances)if(Instance.Definition==Goal.Item)Instances.Add(Instance.Id);
            if(!Instances.IsEmpty())Goal.EquipmentId=Instances[FMath::Clamp(AgentInstanceIndex,0,Instances.Num()-1)];
        }
        Success=AI->SetStructuredGoal(GetOwningPlayerPawn(),Brother,Goal);Refresh();return Success;
    }
    if(Action==TEXT("clearClarification"))
    { if(Page!=TEXT("dialogue")) return false; AI->ClearClarification(); Refresh(); return true; }
    if(Action==TEXT("repairEquipment"))
    {
        auto* B=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>();
        Success=Page==TEXT("repairing") && B->RepairEquipment(Workbench,SelectedRepair,CraftingEpoch);
        Message=Page==TEXT("repairing")?B->Feedback:TEXT("请重新打开工作台维修");
        Refresh(); return Success;
    }
    if(Action.StartsWith(TEXT("repairItem:")))
    {
        const FName Id(*Action.Mid(11));
        if(Page!=TEXT("repairing") || !Find(TEXT("repairRecipes"),Id.ToString()) || Inventory()->GetItemCount(Id)<1) return false;
        SelectedRepair=Id; Message.Reset(); Refresh(); return true;
    }
    if(Action==TEXT("craft"))
    {
        auto* B=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>();
        Success=Page==TEXT("crafting") && B->Craft(Workbench,SelectedRecipe,CraftingBatches,CraftingEpoch);
        Message=Success || Page==TEXT("crafting")?B->Feedback:TEXT("请重新打开工作台");
        Refresh(); return Success;
    }
    if(Action.StartsWith(TEXT("recipe:")))
    {
        const FName Id(*Action.Mid(7));
        if(Page!=TEXT("crafting") || !Find(TEXT("craftingRecipes"),Id.ToString())) return false;
        SelectedRecipe=Id; CraftingBatches=1; Message.Reset(); Refresh(); return true;
    }
    if(Action==TEXT("craftMore") || Action==TEXT("craftLess"))
    {
        if(Page!=TEXT("crafting")) return false;
        CraftingBatches=FMath::Clamp(CraftingBatches+(Action==TEXT("craftMore")?1:-1),1,int32(Number(Catalog()->GetObjectField(TEXT("crafting")),TEXT("maxBatches"))));
        Message.Reset(); Refresh(); return true;
    }
    if(Action.StartsWith(TEXT("build:")))
    {
        auto* B=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>();
        Success=B && B->SelectBuilding(FName(*Action.Mid(6)));
        if(Success) OpenPage(TEXT("hud"));
        return Success;
    }
    if(Action==TEXT("newPrompt") || Action==TEXT("continuePrompt"))
    {
        const FString Target=Action==TEXT("newPrompt")?TEXT("new"):TEXT("continue");
        return ExecuteAction(Save->GetCampaignId().IsValid()?TEXT("ask:")+Target:Target);
    }
    if(Action==TEXT("back"))
    {
        if(Page==TEXT("dialogue") && (DialogueView!=TEXT("home") || AI->HasCandidate()))return ExecuteDialogueAction(TEXT("dialogue.home"));
        if(Page==TEXT("equipment"))return ExecuteAction(TEXT("page:inventory"));
        return ExecuteAction(TEXT("page:")+(ReturnPages.IsEmpty()?(Page==TEXT("title")?FString(TEXT("title")):FString(TEXT("hud"))):ReturnPages.Last().ToString()));
    }
    if(Action.StartsWith(TEXT("ask:"))) { ConfirmAction=Action.Mid(4); Refresh(); return true; }
    if(Action==TEXT("cancel")) { ConfirmAction.Reset();GiveUpEpoch.Invalidate();Refresh();return true; }
    if(Action==TEXT("confirm"))
    {
        const FString Confirmed=ConfirmAction;ConfirmAction.Reset();
        if(Confirmed==TEXT("giveUp"))
        {
            auto* S=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardSurvivalComponent>();
            const bool Valid=Page==TEXT("pause") && GiveUpEpoch==Store->GetTimelineEpoch() && S->State.Life==EHearthwardLife::Downed;
            GiveUpEpoch.Invalidate();
            if(!Valid){Message=TEXT("倒地状态已改变，放弃确认已失效");Refresh();return false;}
            S->GiveUp();OpenPage(TEXT("save"));return true;
        }
        return ExecuteAction(Confirmed);
    }
    if(Action==TEXT("resetAgreements"))
    {
        if(Page!=TEXT("memory"))return false;
        auto* C=Companion(GetWorld());
        Success=C && AI->CancelTasksAndAgreements(GetOwningPlayerPawn(),C,ResetEpoch,ResetRevision,
            ResetCommand,bResetActive,ResetCandidate);
        Message=AI->GetStatus();ResetItems.Reset();
        if(Success){SelectedMemory.Invalidate();Draft->SetText(FText::GetEmpty());}
        MemoryRevision=AI->GetMemoryRevision();Refresh();return Success;
    }
    if(Action.StartsWith(TEXT("page:")))
    {
        const FName Next(*Action.Mid(5));
        if(Next==TEXT("save") && !Save->LoadPointIndex()) { Message=Save->GetStatus(); Refresh(); return false; }
        if(Next==TEXT("storage") && !NearStorage(GetWorld(),GetOwningPlayerPawn())) { Message=TEXT("请靠近营地仓储"); Refresh(); return false; }
        if(Next==TEXT("dialogue") || Next==TEXT("memory"))
        { auto* C=Companion(GetWorld()); if(!C || !C->CanCommunicate(GetOwningPlayerPawn())) { Message=TEXT("请靠近弟弟，交流范围30米"); Refresh(); return false; } }
        if(Next==TEXT("hud") && !Save->GetCampaignId().IsValid()) { OpenPage(TEXT("title")); return false; }
        OpenPage(Next);
        if(Page==TEXT("storage") && GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(GetOwningPlayerPawn()->GetActorLocation())==TEXT("hometown"))GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Record(TEXT("home_storage"));
        return Page==Next;
    }
    if(Action==TEXT("new"))
    {
        if(UGameplayStatics::GetCurrentLevelName(GetWorld(),true)!=TEXT("L_HearthwardWilds"))
        {
            BeginMapTravel();
            UGameplayStatics::OpenLevel(this,NaturalMap,true,TEXT("game=/Script/Hearthward.HearthwardGameMode?HearthwardNewGame=1"));
            return true;
        }
        auto* Loading=GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>();
        Loading->BeginLoading();
        Success=PrepareSession() && Save->StartNewProgress(); Message=Save->GetStatus();
        Loading->FinishSession(Success);
        if(Success) { HUDAnnounceInitialQuest=true; OpenPage(TEXT("hud")); }
    }
    else if(Action==TEXT("continue"))
    {
        if(!Save->LoadPointIndex()) { Message=Save->GetStatus(); Refresh(); return false; }
        const auto Points=Save->GetPoints();
        if(Points.IsEmpty()) { Message=TEXT("尚无存档，请先开始新游戏"); Success=false; }
        else { const auto* Latest=&Points[0]; for(const auto& P:Points) if(P.Created>Latest->Created) Latest=&P; Success=OpenSavePoint(*Latest); Message=Save->GetStatus(); }
    }
    else if(Action==TEXT("title"))
    {
        if(UGameplayStatics::GetCurrentLevelName(GetWorld(),true)==TEXT("L_HearthwardWilds"))
        { BeginMapTravel();UGameplayStatics::OpenLevel(this,TEXT("/Game/Hearthward/Bootstrap/L_Bootstrap")); return true; }
        OpenPage(TEXT("title"));
    }
    else if((Action==TEXT("save.prev") || Action==TEXT("save.next")) && Page==TEXT("save"))
    { if(!ConfirmAction.IsEmpty()) return false;Scroll=FMath::Max(0,Scroll+(Action==TEXT("save.next")?MenuPageSize():-MenuPageSize())); }
    else if(Action==TEXT("save")) { Success=Save->SavePoint(true); Message=Save->GetStatus(); }
    else if(Action.StartsWith(TEXT("load:")))
    {
        FGuid Id; if(!FGuid::Parse(Action.Mid(5),Id)) return false;
        if(!Save->LoadPointIndex()) { Message=Save->GetStatus(); Refresh(); return false; }
        const auto Points=Save->GetPoints();
        const auto* Point=Points.FindByPredicate([Id](const auto& P){return P.SaveId==Id;});
        Success=Point && OpenSavePoint(*Point); Message=Save->GetStatus();
    }
    else if(Action.StartsWith(TEXT("delete:"))) { FGuid Id; FGuid::Parse(Action.Mid(7),Id); Success=Save->DeletePoint(Id); Message=Save->GetStatus(); }
    else if(Action.StartsWith(TEXT("lock:")))
    { FGuid Id; FGuid::Parse(Action.Mid(5),Id); for(const auto& P:Save->GetPoints()) if(P.SaveId==Id) Save->SetPointLocked(Id,!P.Locked); Message=Save->GetStatus(); }
    else if(Action.StartsWith(TEXT("auto:"))) { Success=Save->SetAutoMinutes(Save->GetAutoMinutes()+FCString::Atoi(*Action.Mid(5))); if(!Success) Message=TEXT("自动保存间隔为1至60分钟"); }
    else if(Action==TEXT("fullscreen"))
    { auto* S=GEngine->GetGameUserSettings(); S->SetFullscreenMode(S->GetFullscreenMode()==EWindowMode::Windowed?EWindowMode::WindowedFullscreen:EWindowMode::Windowed); S->ApplyResolutionSettings(false); S->SaveSettings(); }
    else if(Action==TEXT("quit")) UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false);
    else if(Action.StartsWith(TEXT("item:")))
    {
        SelectedItem=FName(*Action.Mid(5));
        if(Page==TEXT("inventory"))
        {
            const FString ItemCategory=InventoryCategory(Find(TEXT("items"),SelectedItem.ToString()));
            if(Category!=ItemCategory) { Category=ItemCategory;Scroll=0; }
        }
    }
    else if(Action.StartsWith(TEXT("filter:")) || Action.StartsWith(TEXT("category:")))
    {
        FString Requested=Action.Mid(Action.Find(TEXT(":"))+1);
        if(Page==TEXT("inventory") && Requested==TEXT("食物"))Requested=TEXT("消耗品");
        CancelInventoryDrag();
        if(Page==TEXT("inventory") && Requested!=TEXT("装备") && Requested!=TEXT("材料") && Requested!=TEXT("消耗品") && Requested!=TEXT("工具")) return false;
        if(Page==TEXT("journal") && Requested!=TEXT("main") && Requested!=TEXT("side") && Requested!=TEXT("world") && Requested!=TEXT("people") && Requested!=TEXT("factions") && Requested!=TEXT("collection")) return false;
        Category=Requested; Scroll=0; Hover=KeyboardFocus=INDEX_NONE;
        if(Page==TEXT("journal")) { Message.Reset();PageFocus.Remove(Page); }
    }
    else if(Action==TEXT("journal.category.prev") || Action==TEXT("journal.category.next"))
    {
        if(Page!=TEXT("journal")) return false;
        const TArray<FString> Categories={TEXT("main"),TEXT("side"),TEXT("world"),TEXT("people"),TEXT("factions"),TEXT("collection")};
        const int32 Current=FMath::Max(0,Categories.IndexOfByKey(Category));
        Category=Categories[(Current+(Action==TEXT("journal.category.next")?1:5))%6];Scroll=0;Hover=KeyboardFocus=INDEX_NONE;Message.Reset();PageFocus.Remove(Page);
    }
    else if(Action.StartsWith(TEXT("journal.list.")))
    {
        if(Page!=TEXT("journal")) return false;
        if(Action==TEXT("journal.list.first")) Scroll=0;
        else if(Action==TEXT("journal.list.last")) Scroll=FMath::Max(0,JournalEntries().Num()-JournalPageSize());
        else if(Action==TEXT("journal.list.prev") || Action==TEXT("journal.list.next")) Scroll=FMath::Max(0,Scroll+(Action==TEXT("journal.list.next")?JournalPageSize():-JournalPageSize()));
        else return false;
    }
    else if(Action==TEXT("journal.entry.prev") || Action==TEXT("journal.entry.next"))
    {
        if(Page!=TEXT("journal")) return false;
        const bool Quests=Category==TEXT("main") || Category==TEXT("side");const auto Entries=JournalEntries();
        FName& Selection=Quests?SelectedQuest:SelectedCodex;const int32 Current=Entries.IndexOfByPredicate([&](const auto& R){return FName(*Text(R,TEXT("id")))==Selection;});
        const int32 Direction=Action==TEXT("journal.entry.next")?1:-1;
        for(int32 EntryIndex=Current+Direction;Entries.IsValidIndex(EntryIndex);EntryIndex+=Direction)
            if(JournalEntryKnown(Entries[EntryIndex]))
            {
                Selection=FName(*Text(Entries[EntryIndex],TEXT("id")));Scroll=FMath::Clamp(Scroll,EntryIndex-JournalPageSize()+1,EntryIndex);
                Hover=KeyboardFocus=INDEX_NONE;PageFocus.Add(Page,(Quests?TEXT("quest:"):TEXT("codex:"))+Selection.ToString());break;
            }
    }
    else if(Action==TEXT("inventory.prev") || Action==TEXT("inventory.next"))
    {
        if(Page!=TEXT("inventory")) return false;
        Scroll=FMath::Max(0,Scroll+(Action==TEXT("inventory.next")?1:-1)*(Category==TEXT("材料")?6:3));
    }
    else if(Action==TEXT("storage.prev") || Action==TEXT("storage.next"))
    {
        if(Page!=TEXT("storage")) return false;
        Scroll=FMath::Max(0,Scroll+(Action==TEXT("storage.next")?4:-4));
    }
    else if(Action.StartsWith(TEXT("deposit:"))) { SelectedItem=FName(*Action.Mid(8)); StorageToCamp=true; Quantity=1; }
    else if(Action.StartsWith(TEXT("withdraw:"))) { SelectedItem=FName(*Action.Mid(9)); StorageToCamp=false; Quantity=1; }
    else if(Action.StartsWith(TEXT("quantity:"))) Quantity=FMath::Clamp(Quantity+FCString::Atoi(*Action.Mid(9)),1,9999);
    else if(Action==TEXT("transfer"))
    {
        if(Page!=TEXT("storage") || StorageEpoch!=Store->GetTimelineEpoch() || !NearStorage(GetWorld(),GetOwningPlayerPawn())) { Success=false; Message=TEXT("仓储访问已失效，请重新靠近打开"); }
        else
        {
            const auto R=Store->Transfer(Inventory(),StorageToCamp,SelectedItem,Quantity,FGuid::NewGuid(),StorageEpoch);
            Success=R.Result==EHearthwardInventoryResult::Success;
            Message=Success?TEXT("物品已转移"):R.Result==EHearthwardInventoryResult::CapacityExceeded?TEXT("背包容量不足"):TEXT("物品数量不足，转移未执行");
        }
    }
    else if(Action==TEXT("menuPause"))
    {
        MenuPause=!MenuPause;
        GConfig->SetBool(TEXT("Hearthward.Survival"),TEXT("MenuPause"),MenuPause,GGameUserSettingsIni);
        GConfig->Flush(false,GGameUserSettingsIni);
        ApplyInputMode();
    }
    else if(Action==TEXT("giveUp"))
    {
        const auto* S=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardSurvivalComponent>();
        if(Page!=TEXT("pause") || S->State.Life!=EHearthwardLife::Downed)return false;
        GiveUpEpoch=Store->GetTimelineEpoch();ConfirmAction=TEXT("giveUp");
        ConfirmMessage=TEXT("放弃后本次进度结束，需要载入保存节点才能继续。确认放弃救援？");Refresh();return true;
    }
    else if(Action==TEXT("cancelSurvival")) { GetOwningPlayerPawn()->FindComponentByClass<UHearthwardSurvivalComponent>()->CancelAction(); }
    else if(Action==TEXT("autoPermission"))
    {
        if(auto* C=Companion(GetWorld()))
        {
            auto* S=C->FindComponentByClass<UHearthwardSurvivalComponent>();
            S->SetAutoPermission(SelectedItem,!S->Permitted(SelectedItem));
        }
    }
    else if(Action==TEXT("use"))
    {
        const FName UsedItem=SelectedItem;
        const auto Item=Find(TEXT("items"),UsedItem.ToString());
        if(Number(Item,TEXT("healing"))>0 || Number(Item,TEXT("food"))>0 || !Text(Item,TEXT("slot")).IsEmpty() || Number(Item,TEXT("throwDamage"))>0) OpenPage(TEXT("hud"));
        Success=G->UseItem(UsedItem); Message=G->Feedback;
    }
    else if(Action==TEXT("repair"))
    {
        SelectedRepair=SelectedItem;
        const auto* Bag=Inventory();
        const auto* Selected=EquipmentOwner==TEXT("player")?Bag->FindInstance(EquipmentSelection):nullptr;
        if(!Selected || Selected->Definition!=SelectedItem)
        {
            EquipmentSelection=Bag->FirstInstance(SelectedItem);
            for(const auto& E:Bag->Snapshot().Equipped)
                if(const auto* I=Bag->FindInstance(E.Value);I && I->Definition==SelectedItem)
                {EquipmentSelection=I->Id;break;}
        }
        EquipmentOwner=TEXT("player");EquipmentStack=NAME_None;
        OpenPage(TEXT("repairing")); Success=Page==TEXT("equipment");
    }
    else if(Action.StartsWith(TEXT("quick:")))
    {
        if(Page!=TEXT("hud")) return false;
        const int32 SlotIndex=FCString::Atoi(*Action.Mid(6));
        if(SlotIndex<0 || SlotIndex>=4) return false;
        if(HUDQuickSelection!=SlotIndex)
        {
            if(auto* C=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardCombatComponent>()) C->Aim(false);
            HUDQuickSelection=SlotIndex;
        }
        else if(SlotIndex!=2) {Success=G->UseQuickItem(SlotIndex);Message=G->Feedback;}
    }
    else if(Action.StartsWith(TEXT("quick.assign:")))
    {
        if(Page!=TEXT("inventory")) return false;
        Success=G->AssignQuickItem(FCString::Atoi(*Action.Mid(13)),SelectedItem);Message=G->Feedback;
    }
    else if(Action==TEXT("drop")) { Success=G->Drop(SelectedItem,Quantity); Message=G->Feedback; }
    else if(Action.StartsWith(TEXT("skill:"))) SelectedSkill=FName(*Action.Mid(6));
    else if(Action==TEXT("learn")) { Success=G->Learn(SelectedSkill); Message=G->Feedback; }
    else if(Action==TEXT("respec")) { G->ResetSkills(); Message=G->Feedback; }
    else if(Action.StartsWith(TEXT("quest:"))) SelectedQuest=FName(*Action.Mid(6));
    else if(Action.StartsWith(TEXT("codex:"))) SelectedCodex=FName(*Action.Mid(6));
    else if(Action==TEXT("questMap"))
    {
        if(Page==TEXT("journal") && (Category!=TEXT("main") && Category!=TEXT("side") || SelectedQuest.IsNone())) return false;
        const auto* Story=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
        const FName Location=Story->Active()?Story->QuestLocation(SelectedQuest):FName(*Text(Find(TEXT("quests"),SelectedQuest.ToString()),TEXT("location")));
        if(G->Discovered.Contains(Location))
        { SelectedLocation=Location;OpenPage(TEXT("map"));WorldMap=true;FocusMapLocation(Location); }
        else { Success=false; Message=TEXT("任务地点尚未发现，请先探索"); }
    }
    else if(Action==TEXT("track"))
    { if(Page==TEXT("journal") && (Category!=TEXT("main") && Category!=TEXT("side") || SelectedQuest.IsNone())) return false;Success=G->Track(SelectedQuest);Message=G->Feedback; }
    else if(Action==TEXT("claim"))
    { if(Page==TEXT("journal") && (Category!=TEXT("main") && Category!=TEXT("side") || SelectedQuest.IsNone())) return false;Success=G->Claim(SelectedQuest);Message=G->Feedback; }
    else if(Action.StartsWith(TEXT("location:"))) { SelectedLocation=FName(*Action.Mid(9));if(Page==TEXT("map"))WorldMap=true;FocusMapLocation(SelectedLocation); }
    else if(Action==TEXT("map.next")) ++Scroll;
    else if(Action==TEXT("map.previous")) Scroll=FMath::Max(0,Scroll-1);
    else if(Action==TEXT("map.world") || Action==TEXT("map.local"))
    {
        WorldMap=Action==TEXT("map.world");TextScroll=0;Scroll=0;
        Hover=KeyboardFocus=INDEX_NONE;Message.Reset();
    }
    else if(Action==TEXT("mapFilter")) { Category=Category==TEXT("travel")?TEXT(""):TEXT("travel"); Message=Category.IsEmpty()?TEXT("显示全部已发现地点"):TEXT("仅显示传送路标"); }
    else if(Action==TEXT("travel"))
    {
        // Release only this widget's pause for synchronous travel validation.
        // A rejection keeps the same map, focus and camera without advancing time.
        const bool Resume=Page==TEXT("map") && OwnPause && GetWorld()->IsPaused();
        if(Resume)UGameplayStatics::SetGamePaused(this,false);
        Success=G->Travel(SelectedLocation);
        if(Success)OpenPage(TEXT("hud"));
        else
        {
            if(Resume)UGameplayStatics::SetGamePaused(this,true);
            Message=G->Feedback;MessageUntil=FPlatformTime::Seconds()+12;
        }
    }
    else if(Action==TEXT("clearWaypoint")) { G->HasWaypoint=false; Message=TEXT("地图标记已清除"); }
    else if(Action==TEXT("cancelReply")) AI->CancelPending();
    else if(Action==TEXT("suggestRefresh"))
    {
        auto* C=Companion(GetWorld());
        Success=C && AI->RefreshSuggestions(GetOwningPlayerPawn(),C);
        Message=Success?TEXT("已刷新3条建议；只有点击的那条才会发送给弟弟"):TEXT("当前无法刷新建议");
    }
    else if(Action.StartsWith(TEXT("suggest:")))
    {
        FGuid Id;
        if(!FGuid::Parse(Action.Mid(8),Id)) { Success=false; Message=TEXT("建议ID无效"); }
        else
        {
            auto* C=Companion(GetWorld());
            Success=C && AI->SubmitSuggestion(GetOwningPlayerPawn(),C,Id);
            Message=AI->GetStatus();
            if(Success) G->Record(TEXT("talk"),TEXT("brother"));
        }
    }
    else if(Action==TEXT("cancelTask"))
    { auto* C=Companion(GetWorld()); Success=C && AI->CancelExecution(GetOwningPlayerPawn(),C); Message=Success?TEXT("委托已取消"):TEXT("请靠近弟弟后取消委托"); }
    else if(Action==TEXT("send") || Action.StartsWith(TEXT("say:")))
    {
        if(Page!=TEXT("dialogue"))return false;
        auto* C=Companion(GetWorld()); const FString Input=Action==TEXT("send")?Draft->GetText().ToString():Action.Mid(4);
        if(!C || !C->CanCommunicate(GetOwningPlayerPawn())) { Success=false; Message=TEXT("已超出30米交流范围"); }
        else { Success=AI->SubmitPlayerText(GetOwningPlayerPawn(),C,Input); Message=AI->GetStatus(); if(Success) { G->Record(TEXT("talk"),TEXT("brother"));  } }
    }
    else Success=false;
    Refresh(); return Success;
}
