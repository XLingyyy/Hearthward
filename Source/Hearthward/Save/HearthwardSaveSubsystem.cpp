#include "HearthwardSaveSubsystem.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Interaction/HearthwardInteractionComponent.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace
{
TMap<FName, int32> Counts(const FHearthwardInventoryState& State)
{
    TMap<FName, int32> Out;
    for (const auto& I : HearthwardBasicItems()) if (State.GetCount(I.Id) > 0) Out.Add(I.Id, State.GetCount(I.Id));
    return Out;
}
FHearthwardInventoryState Inventory(const TMap<FName, int32>& Values, bool Unlimited = false)
{
    FHearthwardInventoryState Out(Unlimited);
    for (const auto& V : Values) Out.Add(V.Key, V.Value); // Already validated before any world mutation.
    return Out;
}
FHearthwardSavedTimer TimerSnapshot(const FHearthwardTimedActionState& State, double Now)
{
    FHearthwardSavedTimer Out;
    Out.Status = State.Status;
    Out.Elapsed = State.Status == EHearthwardTimedActionStatus::Running ? FMath::Clamp(Now - State.StartedAt, 0.0, 5.0) : State.ElapsedSeconds;
    return Out;
}
FHearthwardTimedActionState TimerState(const FHearthwardSavedTimer& Saved, double Now)
{
    FHearthwardTimedActionState Out;
    Out.Status = Saved.Status;
    Out.ElapsedSeconds = Saved.Elapsed;
    Out.StartedAt = Now - Saved.Elapsed;
    return Out;
}
}

bool UHearthwardSaveSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{ return Type == EWorldType::Game || Type == EWorldType::PIE; }
bool UHearthwardSaveSubsystem::IsTickable() const { return !IsTemplate() && bEnabled && CampaignId.IsValid() && !bRestoring; }
TStatId UHearthwardSaveSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UHearthwardSaveSubsystem, STATGROUP_Tickables); }

FString UHearthwardSaveSubsystem::PoolPath() const
{
    FString Name = TEXT("pool");
#if !UE_BUILD_SHIPPING
    FString TestId;
    FGuid Id;
    if (FParse::Value(FCommandLine::Get(), TEXT("HearthwardSaveTestPool="), TestId) && FGuid::Parse(TestId, Id))
        Name = TEXT("test-") + Id.ToString(EGuidFormats::Digits);
#endif
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames/HearthwardPrototype"), Name + TEXT(".hws"));
}

bool UHearthwardSaveSubsystem::ReloadPool()
{
    if (!IFileManager::Get().FileExists(*PoolPath())) { Pool = NewObject<UHearthwardSaveGame>(this); return true; }
    UHearthwardSaveGame* Loaded = nullptr;
    if (!HearthwardSave::Read(PoolPath(), Loaded, Status)) { Pool = nullptr; return false; }
    Pool = Loaded;
    return true;
}
bool UHearthwardSaveSubsystem::CommitPool(UHearthwardSaveGame* Candidate)
{
    if (!HearthwardSave::Write(PoolPath(), Candidate, Status)) return false;
    Pool = Candidate;
    return true;
}

bool UHearthwardSaveSubsystem::Participants(APawn*& Player, AHearthwardCompanionFixture*& Companion) const
{
    Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    Companion = nullptr;
    for (TActorIterator<AHearthwardCompanionFixture> It(GetWorld()); It; ++It)
    {
        if (Companion) return false;
        Companion = *It;
    }
    return Player && Player->FindComponentByClass<UHearthwardInventoryComponent>()
        && Player->FindComponentByClass<UHearthwardTimedActionComponent>() && Companion && Companion->bFixtureEnabled
        && !Companion->bSettling && Companion->IsSourceValid() && IsValid(Companion->Camp) && !Companion->Camp->IsActorBeingDestroyed();
}

bool UHearthwardSaveSubsystem::EnablePrototype()
{
#if UE_BUILD_SHIPPING
    return false;
#else
    if (bEnabled) return true;
    APawn* Player; AHearthwardCompanionFixture* Companion;
    if (!Participants(Player, Companion)) { Status = TEXT("先创建单一伙伴测试夹具"); return false; }
    if (!ReloadPool() || !Capture(InitialWorld)) return false;
    // New progress starts from this explicitly prepared fixture, never from another progress's knowledge.
    InitialWorld.Knowledge.Reset(); InitialWorld.KnowledgeRevision = 0;
    InitialWorld.NPCMemory = {};
    bEnabled = true;
    Status = TEXT("PROTOTYPE_ONLY 存档已启用；请创建新进度或加载节点");
    return true;
#endif
}

bool UHearthwardSaveSubsystem::Capture(FHearthwardWorldSave& S)
{
    APawn* Player; AHearthwardCompanionFixture* Companion;
    if (!Participants(Player, Companion)) { Status = TEXT("快照参与者缺失或正在结算"); return false; }
    if(const auto* B=Player->FindComponentByClass<UHearthwardBuildingComponent>(); B && B->IsBuilding())
    { Status=TEXT("建造中，保存将在完成后可用"); return false; }
    if(const auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>(); G && G->Enabled && (G->InCombat() || G->Health<=0))
    { Status=TEXT("战斗或倒地期间无法保存"); return false; }
    if (auto* Interaction = Player->FindComponentByClass<UHearthwardInteractionComponent>(); Interaction && Interaction->bActive)
    { Status = TEXT("当前交互目标尚未接入存档，请结束交互后保存"); return false; }
    // Reject additional inventory-bearing actors instead of silently losing their state.
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        if (It->FindComponentByClass<UHearthwardInventoryComponent>() && *It != Player && *It != Companion && *It != Companion->Source->GetOwner())
        { Status = TEXT("场景存在未接入快照的容器"); return false; }
    S.Map = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
    S.ActiveSeconds = GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->Clock.GetActivePlaySeconds();
    S.Player = Player->GetActorTransform();
    S.View = Player->GetControlRotation();
    S.Inventory = Counts(Player->FindComponentByClass<UHearthwardInventoryComponent>()->State);
    if (const auto* Gameplay=Player->FindComponentByClass<UHearthwardGameplayComponent>()) S.Gameplay=Gameplay->SaveSnapshot();
    S.Storage = Counts(GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->State.Shared);
    S.PlayerTimer = TimerSnapshot(Player->FindComponentByClass<UHearthwardTimedActionComponent>()->State, S.ActiveSeconds);
    S.Companion = Companion->GetActorTransform(); S.Camp = Companion->Camp->GetActorTransform();
    S.Source = Companion->Source->GetOwner()->GetActorTransform();
    S.Bag = Counts(Companion->Bag->State); S.Resource = Counts(Companion->Source->State);
    S.SourceSafe = Companion->bSourceSafe; S.Phase = Companion->Phase;
    S.Item = Companion->Command.ItemId; S.Requested = Companion->Command.Requested; S.Delivered = Companion->Command.Delivered;
    S.CommandActive = Companion->Command.bActive; S.Statement = Companion->Statement; S.BlockReason = Companion->BlockReason;
    S.CompanionTimer = TimerSnapshot(Companion->Action->State, S.ActiveSeconds);
    S.Knowledge = Knowledge; S.KnowledgeRevision = KnowledgeRevision; S.AutoMinutes = AutoMinutes; S.Safety = Safety;
    S.NPCMemory = GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->GetMemorySnapshot();
    S.NPCStateVersion=HearthwardSave::NPCStateVersion;S.AgentGoal=Companion->Command.Goal;S.Acquired=Companion->Command.Acquired;S.Carried=Companion->Command.Carried;
    S.CommandId=Companion->Command.GetActive().Id;S.NPCDurability=Companion->OwnedDurability;S.NPCSpent=Companion->Spent;S.NPCOperations=Companion->AppliedOperations.Array();
    S.NPCReceipts=Companion->Receipts;
    return true;
}

bool UHearthwardSaveSubsystem::WritePoint(bool Manual, bool NewCampaign)
{
    if (!bEnabled || bRestoring || (!NewCampaign && !CampaignId.IsValid())) { Status = TEXT("尚未创建或加载进度"); return false; }
    if (!Safety.CanSave()) { Status = TEXT("无法保存：") + GetSafetyDescription(); return false; }
    if (!ReloadPool()) return false;
    const int32 Slot = HearthwardSave::SelectSlot(Pool->Points);
    if (Slot == INDEX_NONE) { Status = TEXT("档池无可用位置，请先主动删除一个节点"); return false; }
    FHearthwardWorldSave S;
    if (!Capture(S)) return false;
    if (NewCampaign) S = InitialWorld;
    FHearthwardSavePoint Point;
    Point.SaveId = FGuid::NewGuid(); Point.CampaignId = NewCampaign ? FGuid::NewGuid() : CampaignId;
    Point.Created = FDateTime::UtcNow(); Point.Manual = Manual; Point.World = S;
    Point.World.NPCMemory.Campaign=Point.CampaignId;
    if(NewCampaign){Point.World.NPCMemory.Migrate(Point.CampaignId);S=Point.World;}
    Point.Location = S.Map; Point.Stage = TEXT("PROTOTYPE_ONLY / companion fixture");
    Point.Build = FEngineVersion::Current().ToString() + TEXT(" / ") + FApp::GetBuildVersion();
    auto* Candidate = DuplicateObject<UHearthwardSaveGame>(Pool, this);
    if (Slot == Candidate->Points.Num()) Candidate->Points.Add(Point); else Candidate->Points[Slot] = Point;
    if (!CommitPool(Candidate)) return false;
    if (NewCampaign)
    {
        CampaignId = Point.CampaignId;
        if (!Restore(S)) return false;
    }
    NextAutoSeconds = S.ActiveSeconds + AutoMinutes * 60.0;
    Status = S.Safety.SevereHunger ? TEXT("已保存；严重饥饿，仍可能难以脱困") : TEXT("快照已保存");
    return true;
}
bool UHearthwardSaveSubsystem::StartNewProgress() { return WritePoint(false, true); }
bool UHearthwardSaveSubsystem::SavePoint(bool Manual) { return WritePoint(Manual, false); }

bool UHearthwardSaveSubsystem::Restore(const FHearthwardWorldSave& S)
{
    if (S.Map != UGameplayStatics::GetCurrentLevelName(GetWorld(), true)) { Status = TEXT("请先打开存档所属测试地图"); return false; }
    APawn* Player; AHearthwardCompanionFixture* Companion;
    if (!Participants(Player, Companion)) { Status = TEXT("恢复参与者缺失，请重新创建测试夹具"); return false; }
    TGuardValue<bool> Guard(bRestoring, true);
    if (!UHearthwardGameplayComponent::ValidateSnapshot(S.Gameplay)) { Status=TEXT("玩法快照无效"); return false; }
    auto* Storage = GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    Storage->AdvanceTimeline();
    if(auto* B=Player->FindComponentByClass<UHearthwardBuildingComponent>()) B->CancelPlacement();
    GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->ResetForSnapshot();
    if (auto* Interaction = Player->FindComponentByClass<UHearthwardInteractionComponent>())
    {
        Interaction->bActive = false; Interaction->PendingTarget.Reset();
        Interaction->SetComponentTickEnabled(false); Interaction->SetStatus(EHearthwardInteractionStatus::Idle);
    }
    auto* Personal = Player->FindComponentByClass<UHearthwardInventoryComponent>();
    Personal->State = Inventory(S.Inventory); Storage->State.Shared = Inventory(S.Storage, true);
    Companion->Bag->State = Inventory(S.Bag); Companion->Source->State = Inventory(S.Resource);
    GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->Clock.ActivePlaySeconds = S.ActiveSeconds;
    Knowledge = S.Knowledge; KnowledgeRevision = S.KnowledgeRevision; AutoMinutes = S.AutoMinutes; Safety = S.Safety;
    GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>()->RestoreMemory(S.NPCMemory);
    Player->SetActorTransform(S.Player, false, nullptr, ETeleportType::TeleportPhysics);
    if (auto* Character = Cast<ACharacter>(Player)) Character->GetCharacterMovement()->StopMovementImmediately();
    if (Player->GetController()) Player->GetController()->SetControlRotation(S.View);
    Companion->StopNavigation();
    Companion->SetActorTransform(S.Companion, false, nullptr, ETeleportType::TeleportPhysics);
    Companion->Camp->SetActorTransform(S.Camp); Companion->Source->GetOwner()->SetActorTransform(S.Source);
    Companion->bSourceSafe = S.SourceSafe; Companion->Phase = S.Phase;
    Companion->Statement = S.Statement; Companion->BlockReason = S.BlockReason; Companion->RequestSpeaker = Player;
    Companion->Command = FHearthwardCompanionCommand();
    Companion->Command.ItemId = S.Item; Companion->Command.Requested = S.Requested; Companion->Command.Delivered = S.Delivered;
    Companion->Command.bActive = S.CommandActive;
    Companion->Command.Active = {S.CommandId.IsValid()?S.CommandId:FGuid::NewGuid(), Storage->GetTimelineEpoch(), 1};
    Companion->Command.Acquired=S.Acquired;Companion->Command.Carried=S.Carried;Companion->Command.Goal=S.AgentGoal;
    Companion->OwnedDurability=S.NPCDurability;Companion->Spent=S.NPCSpent;Companion->AppliedOperations=TSet<FGuid>(S.NPCOperations);
    Companion->Receipts=S.NPCReceipts;
    Companion->NavigationFailures=0;Companion->LastProgressAt=GetWorld()->GetTimeSeconds();Companion->LastProgressPosition=Companion->GetActorLocation();
    auto* PlayerTimer = Player->FindComponentByClass<UHearthwardTimedActionComponent>();
    PlayerTimer->State = TimerState(S.PlayerTimer, S.ActiveSeconds);
    PlayerTimer->SetComponentTickEnabled(S.PlayerTimer.Status == EHearthwardTimedActionStatus::Running);
    Companion->Action->State = TimerState(S.CompanionTimer, S.ActiveSeconds);
    Companion->Action->SetComponentTickEnabled(S.CompanionTimer.Status == EHearthwardTimedActionStatus::Running);
    Companion->RestoreExecutionPlan();
    NextAutoSeconds = S.ActiveSeconds + AutoMinutes * 60.0;
    // All state is committed before consumers may observe it. No gameplay settlement events replay.
    if (auto* Gameplay=Player->FindComponentByClass<UHearthwardGameplayComponent>())
    {
        const bool UpgradeLegacy=S.Gameplay.IsEmpty() && Gameplay->Enabled;
        Gameplay->Restore(S.Gameplay);
        if(UpgradeLegacy)
        {
            Gameplay->EnableAdventure();
            // Old saves predate equipment. Put the initial kit in unlimited storage so a full bag stays intact.
            for(const auto& Item:HearthwardData::Catalog()->GetObjectField(TEXT("loadout"))->Values)
                Storage->State.Shared.Add(FName(*Item.Key),Item.Value->AsNumber());
        }
    }
    Personal->OnInventoryChanged.Broadcast(); Companion->Bag->OnInventoryChanged.Broadcast(); Companion->Source->OnInventoryChanged.Broadcast();
    OnSnapshotRestored.Broadcast();
    return true;
}

bool UHearthwardSaveSubsystem::LoadPoint(FGuid SaveId)
{
    if (!bEnabled || bRestoring || !ReloadPool()) return false;
    const auto* Point = Pool->Points.FindByPredicate([SaveId](const auto& P) { return P.SaveId == SaveId; });
    if (!Point) { Status = TEXT("节点不存在"); return false; }
    // Validate the full pool and all participants before invalidating current work.
    const auto Saved = *Point;
    const FGuid PreviousCampaign = CampaignId;
    CampaignId = Saved.CampaignId;
    if (!Restore(Saved.World)) { CampaignId = PreviousCampaign; return false; }
    Status = TEXT("世界与知识已恢复；旧时间线请求已废止");
    return true;
}

bool UHearthwardSaveSubsystem::DeletePoint(FGuid SaveId)
{
    if (!bEnabled || bRestoring || !ReloadPool()) return false;
    auto* Candidate = DuplicateObject<UHearthwardSaveGame>(Pool, this);
    if (Candidate->Points.RemoveAll([SaveId](const auto& P) { return P.SaveId == SaveId; }) != 1) { Status = TEXT("节点不存在"); return false; }
    if (!CommitPool(Candidate)) return false;
    Status = TEXT("已按明确请求删除节点"); return true;
}
bool UHearthwardSaveSubsystem::SetPointLocked(FGuid SaveId, bool Locked)
{
    if (!bEnabled || bRestoring || !ReloadPool()) return false;
    auto* Candidate = DuplicateObject<UHearthwardSaveGame>(Pool, this);
    auto* Point = Candidate->Points.FindByPredicate([SaveId](const auto& P) { return P.SaveId == SaveId; });
    if (!Point) { Status = TEXT("节点不存在"); return false; }
    Point->Locked = Locked;
    if (!CommitPool(Candidate)) return false;
    Status = Locked ? TEXT("节点已锁定") : TEXT("节点已解锁"); return true;
}
bool UHearthwardSaveSubsystem::SetAutoMinutes(int32 Minutes)
{
    if (Minutes < 1 || Minutes > 60 || bRestoring) return false;
    AutoMinutes = Minutes;
    NextAutoSeconds = GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds + Minutes * 60.0;
    return true;
}
void UHearthwardSaveSubsystem::Tick(float DeltaTime)
{
    if (GetWorld()->IsPaused()) return;
    const double Now = GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds;
    if (Now >= NextAutoSeconds)
    {
        if (!Safety.CanSave()) { Status = TEXT("自动保存已延后：") + GetSafetyDescription(); return; }
        if (!SavePoint(false)) NextAutoSeconds = Now + 1.0; // Bound disk retries; safety resumes on the next tick.
    }
}
FString UHearthwardSaveSubsystem::GetSafetyDescription() const
{
    TArray<FString> Reasons;
    if(const auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0))
        if(const auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();G && G->Enabled)
        { if(G->InCombat()) Reasons.Add(TEXT("正在战斗")); if(G->Health<=0) Reasons.Add(TEXT("玩家倒地")); }
    if (Safety.Combat) Reasons.Add(TEXT("正在战斗"));
    if (Safety.EitherDowned) Reasons.Add(TEXT("兄弟有人倒地"));
    if (Safety.Pursued) Reasons.Add(TEXT("正在被追击"));
    if (Safety.Drowning) Reasons.Add(TEXT("正在溺水"));
    if (Safety.Falling) Reasons.Add(TEXT("正在坠落"));
    if (Safety.CompanionDanger) Reasons.Add(TEXT("弟弟处于危险"));
    if (!Reasons.IsEmpty()) return FString::Join(Reasons, TEXT("、"));
    return Safety.SevereHunger ? TEXT("允许保存；严重饥饿，回档后仍可能难以脱困") : TEXT("当前安全条件允许保存");
}
void UHearthwardSaveSubsystem::RememberExchange(const FString& Speaker, const FString& Text)
{
    if (bRestoring || Text.IsEmpty()) return;
    Knowledge.Add(Speaker + TEXT(": ") + Text); ++KnowledgeRevision;
}
TArray<FString> UHearthwardSaveSubsystem::RecentKnowledge() const
{
    TArray<FString> Out;
    // The local model has a 4096-token context; saved history itself remains complete.
    int32 RemainingCharacters = 512;
    for (int32 I = Knowledge.Num() - 1; I >= 0 && Out.Num() < 8 && RemainingCharacters > 0; --I)
    {
        const int32 Take = FMath::Min(RemainingCharacters, Knowledge[I].Len());
        Out.Insert(Knowledge[I].Left(Take) + (Take < Knowledge[I].Len() ? TEXT("（节选）") : TEXT("")), 0);
        RemainingCharacters -= Take;
    }
    return Out;
}
