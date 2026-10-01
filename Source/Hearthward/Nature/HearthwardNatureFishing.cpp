#include "HearthwardNatureSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/RandomStream.h"
#include "EngineUtils.h"
using namespace HearthwardData;
bool UHearthwardNatureSubsystem::StartFishing(FGuid Id,FGuid Epoch)
{
    auto* P=State.Points.FindByPredicate([&](const auto& X){return X.Id==Id && X.Kind==TEXT("fish");});
    if(!P || P->Remaining<=0 || !Near(Id) || Bag()->Available(TEXT("bait"))<1)
    {Feedback=TEXT("请靠近有库存的鱼点，并准备鱼饵");return false;}
    FishingRod.Invalidate();
    for(const auto& I:Bag()->Snapshot().Instances)if(I.Durability>0 && Text(Find(TEXT("items"),I.Definition.ToString()),TEXT("toolKind"))==TEXT("fishing_rod")){FishingRod=I.Id;break;}
    if(!FishingRod.IsValid()){Feedback=TEXT("需要完好的鱼竿和鱼饵");return false;}
    FRandomStream Random(HashCombineFast(uint32(State.Seed),HashCombineFast(FCrc::StrCrc32(*P->Key.ToString()),uint32(P->Successes))));
    Fishing={};Fishing.Bite=Random.FRandRange(2,4);int32 Roll=Random.RandRange(1,100);
    for(const auto& V:HearthwardNature::Rows(TEXT("fish")))
    {const auto D=V->AsObject();Roll-=Number(D,TEXT("pool_weight"));if(Roll<=0){FishingSpecies=FName(Text(D,TEXT("id")));Fishing.Required=Number(D,TEXT("struggle_seconds"));break;}}
    FishingId=Id;ActionEpoch=Epoch;ActionPosition=Player()->GetActorLocation();LineHeld=false;Feedback=TEXT("抛竿中");return true;
}
bool UHearthwardNatureSubsystem::CatchCompanion(APawn* Actor,FGuid Id,FGuid Epoch,FName& CaughtItem)
{
    CaughtItem=NAME_None;
    if(!IsValid(Actor) || Actor==Player() || Busy())return false;
    ActionActor=Actor;
    const bool Success=[&]
    {
        if(!Safe(Epoch) || !StartFishing(Id,Epoch))return false;
        while(!Fishing.Done && !Fishing.Failed)Fishing.Advance(.01,Fishing.Tension<.45);
        if(!Fishing.Done){FishingId.Invalidate();Feedback=TEXT("脱钩，本次没有结算渔获");return false;}
        const auto D=HearthwardNature::Definition(TEXT("fish"),FishingSpecies);
        if(!D){FishingId.Invalidate();return false;}
        const FName Item(*Text(D,TEXT("item")));
        FHearthwardInventoryState Check;
        if(!PrepareBag({{TEXT("bait"),1}},{{Item,1}},Check)
            || !Check.Wear(FishingRod,1/(1+Gameplay()->Effect(TEXT("durability")))))
        {FishingId.Invalidate();Feedback=TEXT("鱼饵、鱼竿或背包容量不足，本次未抛竿");return false;}
        auto* Point=State.Points.FindByPredicate([&](const auto& P){return P.Id==Id;});
        if(!Point || Point->Remaining<=0){FishingId.Invalidate();return false;}
        const int32 Before=Point->Successes;
        TGuardValue<bool> Guard(Settling,true);
        FHearthwardInventoryState Bait;
        if(!PrepareBag({{TEXT("bait"),1}},{},Bait)){FishingId.Invalidate();return false;}
        PublishBag(Bait);
        FinishFishing();
        if(Point->Successes!=Before+1)return false;
        CaughtItem=Item;
        return true;
    }();
    FishingId.Invalidate();ActionActor.Reset();
    return Success;
}
FString UHearthwardNatureSubsystem::FishingStatus() const
{
    if(!IsFishing())return Feedback;
    if(!Fishing.Cast)return TEXT("抛竿：一秒后消耗鱼饵");
    if(Fishing.Elapsed<1+Fishing.Bite)return TEXT("等待咬钩……");
    return FString::Printf(TEXT("按住鼠标左键收线，松开降张力 | 绿色区间 15%%—85%% | %.1f / %.1f 秒"),Fishing.Progress,Fishing.Required);
}
void UHearthwardNatureSubsystem::TickFishing(double Delta)
{
    const auto* Rod=Bag()?Bag()->FindInstance(FishingRod):nullptr;
    if(!Safe(ActionEpoch) || !Near(FishingId) || !Rod || Rod->Durability<=0 || FVector::Dist(Player()->GetActorLocation(),ActionPosition)>100){Cancel();return;}
    const bool WasCast=Fishing.Cast;
    const auto* PC=Cast<APlayerController>(Player()->GetController());
    Fishing.Advance(Delta,LineHeld);
    if(!WasCast && Fishing.Cast)
    {
        TGuardValue<bool> Guard(Settling,true);FHearthwardInventoryState Next;
        if(!PrepareBag({{TEXT("bait"),1}},{},Next)){Cancel();Feedback=TEXT("鱼饵不足，抛竿取消");return;}
        PublishBag(Next);
    }
    if(Fishing.Done){FinishFishing();return;}
    if(Fishing.Failed){Cancel();Feedback=TEXT("脱钩：未消耗鱼竿耐久与鱼点库存");}
}
void UHearthwardNatureSubsystem::FinishFishing()
{
    TGuardValue<bool> Guard(Settling,true);
    auto* P=State.Points.FindByPredicate([&](const auto& X){return X.Id==FishingId;});
    const auto D=HearthwardNature::Definition(TEXT("fish"),FishingSpecies);FHearthwardInventoryState Next;
    if(!P || P->Remaining<=0 || !D || !PrepareBag({},{{FName(Text(D,TEXT("item"))),1}},Next)
        || !Next.Wear(FishingRod,1/(1+Gameplay()->Effect(TEXT("durability")))))
    {FishingId.Invalidate();Feedback=TEXT("背包容量不足或鱼竿不可用，本次未结算渔获");return;}
    const FName Reward=HearthwardNature::Reward(State.Seed,P->Key,P->Successes);
    if(!Reward.IsNone())
    {
        const FName Recipe(Text(Find(TEXT("items"),Reward.ToString()),TEXT("blueprint")));
        bool Duplicate=State.Rewards.Contains(Reward) || (!Recipe.IsNone() && Gameplay()->KnownRecipes.Contains(Recipe)) || Bag()->GetItemCount(Reward)>0
            || GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetItemCount(Reward)>0;
        for(TActorIterator<AActor> It(GetWorld());It;++It)if(const auto* Container=It->FindComponentByClass<UHearthwardInventoryComponent>();Container && Container->GetItemCount(Reward)>0)Duplicate=true;
        FHearthwardInventoryState Bonus(true);Bonus.Restore(P->Pending);
        const TMap<FName,int32> Items=Duplicate?TMap<FName,int32>{{TEXT("rope"),2},{TEXT("herb"),2}}:TMap<FName,int32>{{Reward,1}};
        auto WithBonus=Next;bool Fits=true;
        for(const auto& M:Items)if(WithBonus.Add(M.Key,M.Value)!=EHearthwardInventoryResult::Success){Fits=false;break;}
        if(Fits)Next=MoveTemp(WithBonus);else for(const auto& M:Items)Bonus.Add(M.Key,M.Value);
        P->Pending=Bonus.Snapshot();State.Rewards.Add(Reward);
    }
    P->Remaining--;P->Successes++;if(P->Remaining==0)P->Due=State.Calendar+2880;
    FishingId.Invalidate();LineHeld=false;PublishBag(Next);
    Feedback=Reward.IsNone()?TEXT("钓鱼成功"):TEXT("钓鱼成功并获得额外物品；装不下的部分留在鱼点，可稍后领取");
}
