#include "HearthwardScreenWidget.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "HearthwardHUD.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
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
        for(const auto& V:Catalog()->GetObjectField(TEXT("loadout"))->Values)
            Inventory()->TryAdd(FName(*V.Key),V.Value->AsNumber());
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
            return PrepareSession() && Save->LoadPoint(Point.SaveId);
        const FString Options=TEXT("game=/Script/Hearthward.HearthwardGameMode?HearthwardLoad=")
            +Point.SaveId.ToString(EGuidFormats::Digits);
        UGameplayStatics::OpenLevel(this,NaturalMap,true,Options);
        return true;
    }
    return PrepareSession() && Save->LoadPoint(Point.SaveId);
}
bool UHearthwardScreenWidget::ExecuteAction(const FString& InAction)
{
    const FString Action=InAction; // Refresh can invalidate the element that supplied this string.
    MessageUntil=FPlatformTime::Seconds()+4;
    auto* G=Gameplay(); auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
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
        else return false;
        MemoryRevision=AI->GetMemoryRevision(); Refresh(); return Success;
    }
    if(Action.StartsWith(TEXT("agentConfirm:")) || Action.StartsWith(TEXT("agentMore:")) || Action.StartsWith(TEXT("agentLess:")))
    {
        if(Page!=TEXT("dialogue"))return false;FString Verb,IdText;Action.Split(TEXT(":"),&Verb,&IdText);FGuid Id;if(!FGuid::Parse(IdText,Id))return false;
        Success=Verb==TEXT("agentConfirm")?AI->ConfirmCandidate(Id):AI->AdjustCandidate(Id,Verb==TEXT("agentMore")?1:-1);Message=AI->GetStatus();Refresh();return Success;
    }
    if(Action==TEXT("agentInventory")) {if(Page!=TEXT("dialogue"))return false;Success=AI->QueryInventory(GetOwningPlayerPawn(),Companion(GetWorld()),TEXT("wood"));Refresh();return Success;}
    if(Action==TEXT("agentRetryPath")) {if(Page!=TEXT("dialogue"))return false;auto* C=Companion(GetWorld());Success=C && C->ResumeBlocked(GetOwningPlayerPawn());Refresh();return Success;}
    if(Action==TEXT("agentTypeNext") || Action==TEXT("agentItemNext"))
    {
        if(Page!=TEXT("dialogue"))return false;
        TArray<const FHearthwardAgentCapability*> Caps;for(const auto& C:HearthwardAgent::Capabilities())if(C.Writes)Caps.Add(&C);
        if(Action==TEXT("agentTypeNext")){AgentCapabilityIndex=(AgentCapabilityIndex+1)%Caps.Num();AgentItemIndex=0;}
        else AgentItemIndex=(AgentItemIndex+1)%Caps[AgentCapabilityIndex]->Items.Num();
        Refresh();return true;
    }
    if(Action==TEXT("agentCollectCard"))
    {
        if(Page!=TEXT("dialogue"))return false;
        TArray<const FHearthwardAgentCapability*> Caps;for(const auto& C:HearthwardAgent::Capabilities())if(C.Writes)Caps.Add(&C);
        const auto& C=*Caps[AgentCapabilityIndex];FHearthwardAgentGoal Goal;Goal.Intent=C.Id;Goal.Item=C.Items[AgentItemIndex];Goal.Quantity=1;Goal.QuantityMode=C.QuantityMode;Goal.SourceRef=C.Sources[0];Success=AI->SetStructuredGoal(GetOwningPlayerPawn(),Companion(GetWorld()),Goal);Refresh();return Success;
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
    { return ExecuteAction(TEXT("page:")+(Page==TEXT("settings") || Page==TEXT("save")?ReturnPage.ToString():FString(TEXT("hud")))); }
    if(Action.StartsWith(TEXT("ask:"))) { ConfirmAction=Action.Mid(4); Refresh(); return true; }
    if(Action==TEXT("cancel")) { ConfirmAction.Reset(); Refresh(); return true; }
    if(Action==TEXT("confirm")) { const FString Confirmed=ConfirmAction; ConfirmAction.Reset(); return ExecuteAction(Confirmed); }
    if(Action.StartsWith(TEXT("page:")))
    {
        const FName Next(*Action.Mid(5));
        if(Next==TEXT("save") && !Save->LoadPointIndex()) { Message=Save->GetStatus(); Refresh(); return false; }
        if(Next==TEXT("storage") && !NearStorage(GetWorld(),GetOwningPlayerPawn())) { Message=TEXT("请靠近营地仓储"); Refresh(); return false; }
        if(Next==TEXT("dialogue") || Next==TEXT("memory"))
        { auto* C=Companion(GetWorld()); if(!C || !C->CanCommunicate(GetOwningPlayerPawn())) { Message=TEXT("请靠近弟弟，交流范围30米"); Refresh(); return false; } }
        if(Next==TEXT("hud") && !Save->GetCampaignId().IsValid()) { OpenPage(TEXT("title")); return false; }
        Category.Reset(); OpenPage(Next); return Page==Next;
    }
    if(Action==TEXT("new"))
    {
        if(UGameplayStatics::GetCurrentLevelName(GetWorld(),true)!=TEXT("L_HearthwardWilds"))
        {
            UGameplayStatics::OpenLevel(this,NaturalMap,true,TEXT("game=/Script/Hearthward.HearthwardGameMode?HearthwardNewGame=1"));
            return true;
        }
        Success=PrepareSession() && Save->StartNewProgress(); Message=Save->GetStatus();
        if(Success) OpenPage(TEXT("hud"));
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
        { UGameplayStatics::OpenLevel(this,TEXT("/Game/Hearthward/Bootstrap/L_Bootstrap")); return true; }
        OpenPage(TEXT("title"));
    }
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
    else if(Action.StartsWith(TEXT("item:"))) SelectedItem=FName(*Action.Mid(5));
    else if(Action.StartsWith(TEXT("filter:")) || Action.StartsWith(TEXT("category:"))) { Category=Action.Mid(Action.Find(TEXT(":"))+1); Scroll=0; Hover=KeyboardFocus=INDEX_NONE; }
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
    else if(Action==TEXT("use")) { Success=G->UseItem(SelectedItem); Message=G->Feedback; }
    else if(Action==TEXT("repair"))
    {
        SelectedRepair=SelectedItem;
        OpenPage(TEXT("repairing")); Success=Page==TEXT("repairing");
    }
    else if(Action.StartsWith(TEXT("quick:")))
    {
        const auto& Slots=Theme->GetObjectField(TEXT("inventory"))->GetArrayField(TEXT("quickSlots"));
        const int32 SlotIndex=FCString::Atoi(*Action.Mid(6));
        Success=Slots.IsValidIndex(SlotIndex) && G->UseItem(FName(*Slots[SlotIndex]->AsString())); Message=G->Feedback;
    }
    else if(Action==TEXT("drop")) { Success=G->Drop(SelectedItem,Quantity); Message=G->Feedback; }
    else if(Action.StartsWith(TEXT("skill:"))) SelectedSkill=FName(*Action.Mid(6));
    else if(Action==TEXT("learn")) { Success=G->Learn(SelectedSkill); Message=G->Feedback; }
    else if(Action==TEXT("respec")) { G->ResetSkills(); Message=G->Feedback; }
    else if(Action.StartsWith(TEXT("quest:"))) SelectedQuest=FName(*Action.Mid(6));
    else if(Action.StartsWith(TEXT("codex:"))) SelectedCodex=FName(*Action.Mid(6));
    else if(Action==TEXT("questMap"))
    {
        const FName Location(*Text(Find(TEXT("quests"),SelectedQuest.ToString()),TEXT("location")));
        if(G->Discovered.Contains(Location)) { SelectedLocation=Location; OpenPage(TEXT("map")); }
        else { Success=false; Message=TEXT("任务地点尚未发现，请先探索"); }
    }
    else if(Action==TEXT("track")) { Success=G->Track(SelectedQuest); Message=G->Feedback; }
    else if(Action==TEXT("claim")) { Success=G->Claim(SelectedQuest); Message=G->Feedback; }
    else if(Action.StartsWith(TEXT("location:"))) SelectedLocation=FName(*Action.Mid(9));
    else if(Action==TEXT("mapFilter")) { Category=Category==TEXT("travel")?TEXT(""):TEXT("travel"); Message=Category.IsEmpty()?TEXT("显示全部已发现地点"):TEXT("仅显示传送路标"); }
    else if(Action==TEXT("travel")) { Success=G->Travel(SelectedLocation); Message=G->Feedback; if(Success) OpenPage(TEXT("hud")); }
    else if(Action==TEXT("clearWaypoint")) { G->HasWaypoint=false; Message=TEXT("地图标记已清除"); }
    else if(Action==TEXT("cancelReply")) AI->CancelPending();
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
