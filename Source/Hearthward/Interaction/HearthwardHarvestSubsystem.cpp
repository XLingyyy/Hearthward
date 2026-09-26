#include "HearthwardHarvestSubsystem.h"
#include "../Inventory/HearthwardHarvestTools.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"

int32 UHearthwardHarvestSubsystem::Remaining(const FString& Key,int32 Capacity) const
{ if(const auto* S=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->Source(Key))return S->Remaining;return FMath::Max(0,Capacity-Used.FindRef(Key)); }
bool UHearthwardHarvestSubsystem::Validate(const TMap<FString,int32>& Snapshot)
{
    for(const auto& Entry:Snapshot)
        if(Entry.Key.IsEmpty() || Entry.Key.Len()>512 || Entry.Value<0 || Entry.Value>12) return false;
    return true;
}
void UHearthwardHarvestSubsystem::Restore(const TMap<FString,int32>& Snapshot)
{ Used=Snapshot; NextRefresh=0; }

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
                GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->RegisterSource(Key,Item,Capacity,FMath::Max(0,Capacity-Used.FindRef(Key)),Base,Item==TEXT("wood")?2880:0);
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
    return Count>0?FString::Printf(TEXT("E 采集%s ×%d · 5秒\n剩余 %d · 移动可中断"),*Label,FMath::Min([&](){FGuid Tool;const auto* Bag=Interactor?Interactor->FindComponentByClass<UHearthwardInventoryComponent>():nullptr;return Bag?HearthwardHarvestTools::Yield(Bag,Item,Tool):0;}(),Count),Count)
        :Label+TEXT("已采尽 · 这处资源不会自动刷新");
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
    FGuid Tool;const int32 ToolYield=HearthwardHarvestTools::Yield(Bag,Target->Item,Tool);
    if(!ToolYield)return TEXT("需要耐久大于零且等级足够的采集工具");
    const int32 Count=FMath::Min(ToolYield,Remaining(Target->ResourceKey,Target->Capacity));
    if(Count<=0) return TEXT("资源已耗尽，未采集");
    TGuardValue<bool> Guard(Settling,true);
    // Publish depletion before inventory callbacks; failed additions restore the original resource count.
    const int32 Before=Used.FindRef(Target->ResourceKey);
    auto* Economy=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    auto* Source=Economy->Source(Target->ResourceKey);
    const double OldDue=Source?Source->Due:-1;
    if(Source){Source->Remaining-=Count;if(Source->Remaining==0 && Source->RefreshMinutes>0)Source->Due=Economy->State.Calendar+Source->RefreshMinutes;}
    else Used.Add(Target->ResourceKey,Before+Count);
    if(Bag->TryAdd(Target->Item,Count)!=EHearthwardInventoryResult::Success)
    { if(Source){Source->Remaining+=Count;Source->Due=OldDue;}else if(Before) Used.Add(Target->ResourceKey,Before); else Used.Remove(Target->ResourceKey); return TEXT("背包容量不足，资源未消耗"); }
    if(Tool.IsValid())Bag->WearInstance(Tool,1/(1+G->Effect(TEXT("durability"))));
    G->Record(TEXT("harvest"),Target->Item,Count);
    return FString::Printf(TEXT("已采集%s ×%d"),*HearthwardData::Text(HearthwardData::Find(TEXT("items"),Target->Item.ToString()),TEXT("name")),Count);
}
