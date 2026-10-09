#include "HearthwardFurnitureInteractionComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Engine/World.h"

FString UHearthwardFurnitureInteractionComponent::GetInteractionPrompt(AActor* Interactor) const
{
    if(Kind==TEXT("medical_area"))return TEXT("E 进入治疗区 · 每秒恢复3%最大生命，移动离开");
    if(Kind==TEXT("bed"))return TEXT("E 就座休息 · 就座后每秒恢复2%最大生命\n普通饱食消耗 · 移动离开");
    const auto* Bag=Interactor?Interactor->FindComponentByClass<UHearthwardInventoryComponent>():nullptr;
    if(Kind!=TEXT("campfire") || !Bag)return {};
    FHearthwardInventoryState Prospective;Prospective.Restore(Bag->Snapshot());
    if(Prospective.Exchange({{TEXT("meat"),1},{TEXT("wood"),1}},{{TEXT("roast"),1}},1)!=EHearthwardInventoryResult::Success)return {};
    return TEXT("E 烤肉5秒 · 鲜肉1 + 木材1 → 烤肉1\n移动可中断，完成时消耗材料");
}
FString UHearthwardFurnitureInteractionComponent::CompleteInteraction(AActor* Interactor)
{
    if(!IsValid(Interactor) || GetWorld()!=Interactor->GetWorld() || GetWorld()->IsPaused()
        || FVector::Dist(Interactor->GetActorLocation(),GetComponentLocation())>MaxDistance) return TEXT("设施不可用");
    auto* G=Interactor->FindComponentByClass<UHearthwardGameplayComponent>();
    if(!G || !G->Enabled || G->Health<=0 || G->InCombat() || UHearthwardSurvivalComponent::HasFailed(GetWorld())) return TEXT("请在安全处使用设施");
    if(Kind==TEXT("bed") || Kind==TEXT("medical_area"))
    {
        auto* S=Interactor->FindComponentByClass<UHearthwardSurvivalComponent>();
        if(!S->BeginRest(this)) return TEXT("设施已不可用于休息");
        G->Record(TEXT("rest"),Kind);
        return S->Status+TEXT("；零饱食时停止自然恢复");
    }
    if(Kind==TEXT("campfire"))
    {
        auto* Bag=Interactor->FindComponentByClass<UHearthwardInventoryComponent>();
        const auto Result=Bag->TryExchange({{TEXT("meat"),1},{TEXT("wood"),1}},{{TEXT("roast"),1}},1);
        if(Result!=EHearthwardInventoryResult::Success) return TEXT("需要鲜肉1、木材1和背包余量，未消耗材料");
        G->Record(TEXT("cook"),TEXT("roast")); return TEXT("烤肉完成：烤肉 ×1");
    }
    return TEXT("设施未配置");
}
