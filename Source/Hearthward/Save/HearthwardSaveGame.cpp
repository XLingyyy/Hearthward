#include "HearthwardSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

int32 HearthwardSave::SelectSlot(const TArray<FHearthwardSavePoint>& Points)
{
    if (Points.Num() < MaxPoints) return Points.Num();
    int32 Oldest = INDEX_NONE;
    for (int32 I = 0; I < Points.Num(); ++I)
        if (!Points[I].Manual && !Points[I].Locked && (Oldest == INDEX_NONE || Points[I].Created < Points[Oldest].Created)) Oldest = I;
    return Oldest;
}

namespace
{
bool ValidCounts(const TMap<FName, int32>& Counts, bool Unlimited)
{
    FHearthwardInventoryState State(Unlimited);
    for (const auto& Pair : Counts)
        if (State.Add(Pair.Key, Pair.Value) != EHearthwardInventoryResult::Success) return false;
    return true;
}
bool ValidTimer(const FHearthwardSavedTimer& Timer)
{
    return uint8(Timer.Status) <= uint8(EHearthwardTimedActionStatus::Completed)
        && FMath::IsFinite(Timer.Elapsed) && Timer.Elapsed >= 0 && Timer.Elapsed <= 5;
}
bool Decode(const TArray<uint8>& Bytes, UHearthwardSaveGame*& Out, FString& Error)
{
    // Check the envelope before Unreal allocates/deserializes its object payload.
    constexpr uint32 Magic = 0x48575331;
    uint32 Header[3] = {};
    if (Bytes.Num() < sizeof(Header) || Bytes.Num() > 64 * 1024 * 1024) { Error = TEXT("存档大小无效"); return false; }
    FMemory::Memcpy(Header, Bytes.GetData(), sizeof(Header));
    const int32 Length = Bytes.Num() - sizeof(Header);
    if (Header[0] != Magic || Header[1] != uint32(Length) || Header[2] != FCrc::MemCrc32(Bytes.GetData() + sizeof(Header), Length))
    { Error = TEXT("存档完整性校验失败"); return false; }
    TArray<uint8> Payload;
    Payload.Append(Bytes.GetData() + sizeof(Header), Length);
    Out = Cast<UHearthwardSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
    if (!Out || !HearthwardSave::Validate(*Out)) { Error = TEXT("存档版本或快照状态无效"); Out = nullptr; return false; }
    return true;
}
}

bool HearthwardSave::Validate(const UHearthwardSaveGame& Pool)
{
    if (Pool.Schema != 1 || Pool.Points.Num() > MaxPoints) return false;
    TSet<FGuid> Ids;
    for (const auto& P : Pool.Points)
    {
        const auto& S = P.World;
        if (!P.SaveId.IsValid() || !P.CampaignId.IsValid() || Ids.Contains(P.SaveId) || S.Map.IsEmpty()
            || !FMath::IsFinite(S.ActiveSeconds) || S.ActiveSeconds < 0 || S.KnowledgeRevision != S.Knowledge.Num()
            || S.AutoMinutes < 1 || S.AutoMinutes > 60 || !S.Safety.CanSave()
            || !S.Player.IsValid() || !S.Companion.IsValid() || !S.Source.IsValid() || !S.Camp.IsValid() || S.View.ContainsNaN()
            || !ValidCounts(S.Inventory, false) || !ValidCounts(S.Storage, true) || !ValidCounts(S.Bag, false) || !ValidCounts(S.Resource, false)
            || !ValidTimer(S.PlayerTimer) || !ValidTimer(S.CompanionTimer)
            || uint8(S.Phase) > uint8(EHearthwardCompanionPhase::Cancelled)
            || S.Requested < 0 || S.Delivered < 0 || S.Delivered > S.Requested) return false;
        if (S.Requested > 0 && !HearthwardBasicItems().ContainsByPredicate([&S](const auto& I) { return I.Id == S.Item; })) return false;
        if (S.CommandActive && (S.Requested == 0 || S.Delivered == S.Requested)) return false;
        const bool Executing = S.Phase == EHearthwardCompanionPhase::GoingToSource || S.Phase == EHearthwardCompanionPhase::Gathering
            || S.Phase == EHearthwardCompanionPhase::Returning || S.Phase == EHearthwardCompanionPhase::ReturningBlocked;
        if (Executing && !S.CommandActive) return false;
        if (S.Phase == EHearthwardCompanionPhase::Completed && (S.Requested == 0 || S.Delivered != S.Requested || S.CommandActive)) return false;
        if (S.Phase == EHearthwardCompanionPhase::Gathering && S.CompanionTimer.Status != EHearthwardTimedActionStatus::Running
            && S.CompanionTimer.Status != EHearthwardTimedActionStatus::Completed) return false;
        Ids.Add(P.SaveId);
    }
    return true;
}

bool HearthwardSave::Read(const FString& Path, UHearthwardSaveGame*& Out, FString& Error)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path)) { Error = TEXT("无法读取存档池"); return false; }
    return Decode(Bytes, Out, Error);
}

bool HearthwardSave::Write(const FString& Path, UHearthwardSaveGame* Pool, FString& Error)
{
    if (!Pool || !Validate(*Pool)) { Error = TEXT("拒绝写入无效快照"); return false; }
    TArray<uint8> Payload, Bytes;
    if (!UGameplayStatics::SaveGameToMemory(Pool, Payload)) { Error = TEXT("快照序列化失败"); return false; }
    const uint32 Header[] = {0x48575331, uint32(Payload.Num()), FCrc::MemCrc32(Payload.GetData(), Payload.Num())};
    Bytes.Append(reinterpret_cast<const uint8*>(Header), sizeof(Header));
    Bytes.Append(Payload);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
    const FString Temp = Path + TEXT(".pending");
    if (!FFileHelper::SaveArrayToFile(Bytes, *Temp)) { Error = TEXT("临时存档写入失败，原档未替换"); return false; }
    UHearthwardSaveGame* Verified = nullptr;
    if (!Read(Temp, Verified, Error)) return false;
#if PLATFORM_WINDOWS
    const FString From = FPaths::ConvertRelativePathToFull(Temp), To = FPaths::ConvertRelativePathToFull(Path);
    if (MoveFileExW(*From, *To, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
#endif
    Error = TEXT("存档原子替换失败，原档未替换");
    return false;
}
