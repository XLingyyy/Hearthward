#include "HearthwardFurnitureInteractionComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "Engine/World.h"

FString UHearthwardFurnitureInteractionComponent::GetInteractionPrompt(AActor* Interactor) const
{
    return Kind==TEXT("bed")?TEXT("E 休息5秒 · 饱食-10，生命+30，耐力恢复\n至少需要15饱食 · 移动可中断")
        :TEXT("E 烤肉5秒 · 鲜肉1 + 木材1 → 烤肉1\n移动可中断，完成时消耗材料");
}
FString UHearthwardFurnitureInteractionComponent::CompleteInteraction(AActor* Interactor)
{
    if(!IsValid(Interactor) || GetWorld()!=Interactor->GetWorld() || GetWorld()->IsPaused()
        || FVector::Dist(Interactor->GetActorLocation(),GetComponentLocation())>MaxDistance) return TEXT("设施不可用");
    auto* G=Interactor->FindComponentByClass<UHearthwardGameplayComponent>();
    if(!G || !G->Enabled || G->Health<=0 || G->InCombat()) return TEXT("请在安全处使用设施");
    if(Kind==TEXT("bed"))
    {
        if(G->Hunger<15) return TEXT("饱食不足15，请先吃些食物");
        if(G->Health>=G->MaxHealth() && G->Stamina>=G->MaxStamina()) return TEXT("状态已满，无需休息");
        G->Hunger-=10; G->Health=FMath::Min(G->MaxHealth(),G->Health+30); G->Stamina=G->MaxStamina();
        G->Record(TEXT("rest"),TEXT("bed"));
        return TEXT("休息完成：生命+30，耐力恢复，饱食-10");
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
