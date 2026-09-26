#include "HearthwardHarvestSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Components/BoxComponent.h"
#include "String/LexFromString.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"

int32 UHearthwardHarvestSubsystem::Remaining(const FString& Key,int32 Capacity) const
{ return FMath::Max(0,Capacity-Used.FindRef(Key)); }
bool UHearthwardHarvestSubsystem::Validate(const TMap<FString,int32>& Snapshot)
{
    for(const auto& Entry:Snapshot)
        if(Entry.Key.IsEmpty() || Entry.Key.Len()>512 || Entry.Value<0 || Entry.Value>12) return false;
    return true;
}
void UHearthwardHarvestSubsystem::Restore(const TMap<FString,int32>& Snapshot,
    const TMap<FString,FHearthwardResourceRefresh>& Due)
{ Used=Snapshot; Refreshes=Due; NextRefresh=0; }

bool UHearthwardHarvestSubsystem::TreePosition(const FString& Key, FVector& Position)
{
    TArray<FString> Parts, Coordinates;
    Key.ParseIntoArray(Parts,TEXT("|"),false);
    if(Parts.Num()!=5) return false;
    const auto& Name=Parts[2];
    if(Name!=TEXT("SM_Tree") && Name!=TEXT("SM_S1_IslandTree")
        && !Name.StartsWith(TEXT("SM_CampFir")) && !Name.StartsWith(TEXT("SM_CampPine"))) return false;
    Parts[4].ParseIntoArray(Coordinates,TEXT(","),false);
    int32 X,Y,Z;
    if(Coordinates.Num()!=3 || !LexTryParseString(X,*Coordinates[0])
        || !LexTryParseString(Y,*Coordinates[1]) || !LexTryParseString(Z,*Coordinates[2])) return false;
    Position=FVector(X,Y,Z);
    return true;
}

bool UHearthwardHarvestSubsystem::ValidateRefreshes(const TMap<FString,int32>& UsedSnapshot,
    const TMap<FString,FHearthwardResourceRefresh>& Due, double CalendarMinutes)
{
    for(const auto& Entry:Due)
    {
        FVector Position;
        if(UsedSnapshot.FindRef(Entry.Key)!=12 || !TreePosition(Entry.Key,Position)
            || !Entry.Value.IsValid(CalendarMinutes) || !Entry.Value.Position.Equals(Position,1)) return false;
    }
    for(const auto& Entry:UsedSnapshot)
    {
        FVector Position;
        if(Entry.Value==12 && TreePosition(Entry.Key,Position) && !Due.Contains(Entry.Key)) return false;
    }
    return true;
}

void UHearthwardHarvestSubsystem::RefreshDue(double CalendarMinutes)
{
    if(Settling || GetWorld()->IsPaused()
        || GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsRestoring()) return;
    for(auto It=Refreshes.CreateIterator();It;++It)
    {
        if(CalendarMinutes<It.Value().DueAt) continue;
        bool Occupied=false;
        for(TActorIterator<AActor> Actor(GetWorld());Actor;++Actor)
        {
            if(!Actor->ActorHasTag(TEXT("Hearthward.Building.Completed"))) continue;
            const auto* Box=Cast<UBoxComponent>(Actor->GetRootComponent());
            if(!Box) continue;
            const FVector Local=Box->GetComponentTransform().InverseTransformPosition(It.Value().Position);
            const FVector Half=Box->GetUnscaledBoxExtent();
            // Building footprint, including its ground plane; height does not allow trees inside floors.
            if(FMath::Abs(Local.X)<=Half.X && FMath::Abs(Local.Y)<=Half.Y) { Occupied=true; break; }
        }
        if(It.Value().CanRefresh(CalendarMinutes,Occupied))
        {
            Used.Remove(It.Key());
            It.RemoveCurrent();
        }
    }
}

void UHearthwardHarvestSubsystem::RefreshNearby(AActor* Player)
{
    if(!Player || !GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsNaturalWorldEnabled()) return;
    const double Now=GetWorld()->GetTimeSeconds();
    if(Now<NextRefresh) return;
    NextRefresh=Now+.5;
    const FVector Location=Player->GetActorLocation();
    for(TActorIterator<AActor> It(GetWorld());It;++It)
    {
        TInlineComponentArray<UStaticMeshComponent*> Meshes; It->GetComponents(Meshes);
        for(auto* Mesh:Meshes)
        {
            if(!Mesh->GetStaticMesh()) continue;
            const FString Path=Mesh->GetStaticMesh()->GetPathName();
            if(!Path.StartsWith(TEXT("/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes/"))) continue;
            const FString Name=Mesh->GetStaticMesh()->GetName();
            FName Item; int32 Capacity=0;
            if(Name==TEXT("SM_Rock")) { Item=TEXT("stone"); Capacity=8; }
            else if(Name==TEXT("SM_Shrub")) { Item=TEXT("herb"); Capacity=4; }
            else if(Name==TEXT("SM_Tree") || Name==TEXT("SM_S1_IslandTree") || Name.StartsWith(TEXT("SM_CampFir")) || Name.StartsWith(TEXT("SM_CampPine")))
            { Item=TEXT("wood"); Capacity=12; }
            else continue;
            TArray<int32> Instances;
            auto* ISM=Cast<UInstancedStaticMeshComponent>(Mesh);
            if(ISM) Instances=ISM->GetInstancesOverlappingSphere(Location,900,true);
            else if(Mesh->Bounds.GetBox().ComputeSquaredDistanceToPoint(Location)<FMath::Square(900.0)) Instances.Add(INDEX_NONE);
            for(int32 Index:Instances)
            {
                FTransform Transform=Mesh->GetComponentTransform();
                if(ISM && !ISM->GetInstanceTransform(Index,Transform,true)) continue;
                const FVector Base=Transform.GetLocation();
                // Authored instance transforms are stable across streaming and save/load; no instance removal.
                const FString Key=FString::Printf(TEXT("%s|%s|%s|%d|%d,%d,%d"),*It->GetName(),*Mesh->GetName(),*Name,Index,
                    FMath::RoundToInt(Base.X),FMath::RoundToInt(Base.Y),FMath::RoundToInt(Base.Z));
                if(Targets.FindRef(Key).IsValid()) continue;
                auto* Target=NewObject<UHearthwardHarvestTargetComponent>(*It);
                It->AddInstanceComponent(Target); Target->SetupAttachment(It->GetRootComponent());
                Target->ResourceKey=Key; Target->Item=Item; Target->Capacity=Capacity; Target->MaxDistance=240;
                Target->RegisterComponent(); Target->SetWorldLocation(Base+FVector(0,0,80));
                Targets.Add(Key,Target);
            }
        }
    }
    for(auto It=Targets.CreateIterator();It;++It)
        if(!It.Value().IsValid()) It.RemoveCurrent();
}
FString UHearthwardHarvestTargetComponent::GetInteractionPrompt(AActor* Interactor) const
{
    const int32 Count=GetWorld()->GetSubsystem<UHearthwardHarvestSubsystem>()->Remaining(ResourceKey,Capacity);
    const FString Label=HearthwardData::Text(HearthwardData::Find(TEXT("items"),Item.ToString()),TEXT("name"));
    return Count>0?FString::Printf(TEXT("E 采集%s ×%d · 5秒\n剩余 %d · 移动可中断"),*Label,FMath::Min(Yield,Count),Count)
        :Label+(Item==TEXT("wood")?TEXT("已采尽 · 两个游戏日后再生，占地时顺延"):TEXT("已采尽 · 这处资源不会自动刷新"));
}
FString UHearthwardHarvestTargetComponent::CompleteInteraction(AActor* Player)
{ return GetWorld()->GetSubsystem<UHearthwardHarvestSubsystem>()->Harvest(this,Player); }
FString UHearthwardHarvestSubsystem::Harvest(UHearthwardHarvestTargetComponent* Target,AActor* Player)
{
    if(Settling || !IsValid(Target) || !IsValid(Player) || Target->GetWorld()!=GetWorld() || Player->GetWorld()!=GetWorld()
        || GetWorld()->IsPaused() || FVector::Dist(Player->GetActorLocation(),Target->GetComponentLocation())>Target->MaxDistance)
        return TEXT("目标不可用，未采集");
    auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();
    auto* Bag=Player->FindComponentByClass<UHearthwardInventoryComponent>();
    if(!G || !G->Enabled || G->Health<=0 || G->InCombat() || !Bag) return TEXT("请在安全处采集");
    const int32 Count=FMath::Min(Target->Yield,Remaining(Target->ResourceKey,Target->Capacity));
    if(Count<=0) return TEXT("资源已耗尽，未采集");
    TGuardValue<bool> Guard(Settling,true);
    // Publish depletion before inventory callbacks; failed additions restore the original resource count.
    const int32 Before=Used.FindRef(Target->ResourceKey);
    Used.Add(Target->ResourceKey,Before+Count);
    if(Bag->TryAdd(Target->Item,Count)!=EHearthwardInventoryResult::Success)
    { if(Before) Used.Add(Target->ResourceKey,Before); else Used.Remove(Target->ResourceKey); return TEXT("背包容量不足，资源未消耗"); }
    if(Target->Item==TEXT("wood") && Before+Count==Target->Capacity)
    {
        FVector Position;
        if(TreePosition(Target->ResourceKey,Position))
            Refreshes.Add(Target->ResourceKey,FHearthwardResourceRefresh::DepletedTree(
                GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes,Position));
    }
    G->Record(TEXT("harvest"),Target->Item,Count);
    return FString::Printf(TEXT("已采集%s ×%d"),*HearthwardData::Text(HearthwardData::Find(TEXT("items"),Target->Item.ToString()),TEXT("name")),Count);
}
