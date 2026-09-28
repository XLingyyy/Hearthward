#include "HearthwardNatureSubsystem.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "HearthwardNatureActor.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardHarvestTools.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "GameFramework/Pawn.h"
using namespace HearthwardData;
namespace { using NatureResult=EHearthwardInventoryResult; }
bool UHearthwardNatureSubsystem::PrepareBag(const TMap<FName,int32>& In,const TMap<FName,int32>& Out,FHearthwardInventoryState& Result) const
{
    if(!Bag() || !Result.Restore(Bag()->Snapshot()))return false;
    for(const auto& M:In)if(Bag()->Available(M.Key)<M.Value || Result.Remove(M.Key,M.Value)!=NatureResult::Success)return false;
    for(const auto& M:Out)if(Result.Add(M.Key,M.Value)!=NatureResult::Success)return false;
    return true;
}
void UHearthwardNatureSubsystem::PublishBag(const FHearthwardInventoryState& Result)
{Bag()->RestoreInventory(Result.Snapshot(),false);Bag()->OnInventoryChanged.Broadcast();}
bool UHearthwardNatureSubsystem::CommitCompanion(APawn* Actor,FName Action,FGuid Target,FGuid Epoch,int32 Count)
{
    if(!IsValid(Actor) || Actor==Player() || Busy() || Count<=0)return false;
    const bool Crop=State.Crops.ContainsByPredicate([&](const auto& C){return C.Id==Target;});
    const bool Pen=State.Pens.ContainsByPredicate([&](const auto& P){return P.Id==Target;});
    if(Action==TEXT("deposit_feed") ? !Pen
        : !Crop || (Action!=TEXT("water") && Action!=TEXT("fertilize") && Action!=TEXT("harvest")))return false;
    ActionActor=Actor;
    const bool Allowed=Safe(Epoch) && Near(Target);
    bool Success=false;
    if(Allowed)
    {
        TGuardValue<bool> Guard(Settling,true);
        Success=Commit(Action,Target,NAME_None,Count,Actor->GetActorLocation());
    }
    ActionActor.Reset();
    if(Success)RebuildActors();
    return Success;
}
bool UHearthwardNatureSubsystem::Act(FName Action,FGuid Target,FName Option,FGuid Epoch,int32 Count)
{
    if(Busy() || !Safe(Epoch) || Count<=0){Feedback=TEXT("当前无法操作，请在安全处重新打开面板");return false;}
    if(Action==TEXT("fish"))return StartFishing(Target,Epoch);
    const bool Place=Action==TEXT("plant") || Action==TEXT("build_pen");
    if(!Place && !Near(Target)){Feedback=TEXT("请走到目标三米内");return false;}
    FVector At=Player()->GetActorLocation();
    if(Place || Action==TEXT("move_pen"))
    {
        if(!Ground(At+Player()->GetActorForwardVector()*250,At,true) || !ClearPlot(At,Action==TEXT("plant")?140:200,Action==TEXT("move_pen")?Target:FGuid()))
        {Feedback=TEXT("前方需要营地内平坦、无建筑的空地");return false;}
    }
    auto* Timer=Player()->FindComponentByClass<UHearthwardTimedActionComponent>();
    if(!Timer || !Timer->StartAction()){Feedback=TEXT("已有动作正在进行");return false;}
    PendingAction=Action;PendingId=Target;PendingOption=Option;PendingCount=Count;ActionPosition=At;ActionEpoch=Epoch;
    Timer->OnTimerCompleted.AddUniqueDynamic(this,&UHearthwardNatureSubsystem::ActionCompleted);
    Timer->OnInterrupted.AddUniqueDynamic(this,&UHearthwardNatureSubsystem::ActionInterrupted);
    Feedback=TEXT("操作中，保持静止五秒");return true;
}
void UHearthwardNatureSubsystem::Cancel()
{
    const bool WasBusy=Busy();PendingAction=NAME_None;FishingId.Invalidate();LineHeld=false;
    if(Player())if(auto* T=Player()->FindComponentByClass<UHearthwardTimedActionComponent>())
    {T->OnTimerCompleted.RemoveDynamic(this,&UHearthwardNatureSubsystem::ActionCompleted);T->OnInterrupted.RemoveDynamic(this,&UHearthwardNatureSubsystem::ActionInterrupted);T->InterruptAction();}
    if(WasBusy)Feedback=TEXT("操作已取消；已抛出的鱼饵不返还");
}
void UHearthwardNatureSubsystem::ActionInterrupted(){Cancel();}
void UHearthwardNatureSubsystem::ActionCompleted()
{
    if(PendingAction.IsNone())return;
    const FName Action=PendingAction;PendingAction=NAME_None;
    auto* T=Player()->FindComponentByClass<UHearthwardTimedActionComponent>();
    T->OnTimerCompleted.RemoveDynamic(this,&UHearthwardNatureSubsystem::ActionCompleted);T->OnInterrupted.RemoveDynamic(this,&UHearthwardNatureSubsystem::ActionInterrupted);
    if(!Safe(ActionEpoch) || (PendingId.IsValid() && !Near(PendingId))){Feedback=TEXT("目标或状态已改变，未结算");return;}
    TGuardValue<bool> Guard(Settling,true);
    const bool Success=Commit(Action,PendingId,PendingOption,PendingCount,ActionPosition);
    Feedback=Success?TEXT("操作完成"):TEXT("条件不足：检查材料、工具、背包容量、成熟状态与栏舍空位");
    RebuildActors();
}
bool UHearthwardNatureSubsystem::Commit(FName Action,FGuid Id,FName Option,int32 Count,FVector Site)
{
    auto* Crop=State.Crops.FindByPredicate([&](const auto& C){return C.Id==Id;});
    auto* Pen=State.Pens.FindByPredicate([&](const auto& P){return P.Id==Id;});
    auto* Animal=State.Animals.FindByPredicate([&](const auto& A){return A.Id==Id;});
    auto* Point=State.Points.FindByPredicate([&](const auto& P){return P.Id==Id;});
    auto* Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    FHearthwardInventoryState Next;
    if(Action==TEXT("harvest") && Point && Point->Kind==TEXT("resource"))
    {
        const auto D=HearthwardNature::Definition(TEXT("resources"),Point->Definition);const FName Item(Text(D,TEXT("item")));FGuid Tool;
        const int32 Yield=Text(D,TEXT("tool"))==TEXT("hand")?2:HearthwardHarvestTools::Yield(Bag(),Item,Tool);
        auto* Source=Camp->Source(Point->Key.ToString());if(!Source || Source->Blocked || Yield<=0 || Source->Remaining<=0)return false;
        const int32 N=FMath::Min(Yield,Source->Remaining);if(!PrepareBag({},{{Item,N}},Next))return false;
        if(Tool.IsValid() && !Next.Wear(Tool,1/(1+Gameplay()->Effect(TEXT("durability")))))return false;
        Source->Remaining-=N;if(Source->Remaining==0)Source->Due=State.Calendar+Source->RefreshMinutes;
        PublishBag(Next);Gameplay()->Record(TEXT("harvest"),Item,N);
        if(Item==TEXT("ore"))GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Record(TEXT("mine_source"));return true;
    }
    if(Action==TEXT("plant"))
    {
        const auto D=HearthwardNature::Definition(TEXT("crops"),Option);
        if(!D || !ClearPlot(Site,140) || !PrepareBag({{FName(Text(D,TEXT("seed"))),1}},{},Next))return false;
        FHearthwardCrop C;C.Id=FGuid::NewGuid();C.Definition=Option;C.Position=Site;C.Planted=State.Calendar;State.Crops.Add(C);PublishBag(Next);if(Option==TEXT("grain"))GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Record(TEXT("grain_planted"));return true;
    }
    if(Crop)
    {
        if(Action==TEXT("water") && !Crop->Watered){Crop->Watered=true;return true;}
        if(Action==TEXT("fertilize") && !Crop->Fertilized){Crop->Fertilized=true;return true;}
        if(Action==TEXT("harvest") && State.Ready(*Crop))
        {
            const auto D=HearthwardNature::Definition(TEXT("crops"),Crop->Definition);
            if(!PrepareBag({},{{FName(Text(D,TEXT("output"))),State.Yield(*Crop)},{FName(Text(D,TEXT("seed"))),1}},Next))return false;
            if(Crop->Definition==TEXT("grain"))GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Record(TEXT("grain_harvested"));
            State.Crops.RemoveAll([&](const auto& C){return C.Id==Id;});PublishBag(Next);return true;
        }
    }
    if(Action==TEXT("build_pen") || (Action==TEXT("upgrade_pen") && Pen))
    {
        const int32 Level=Pen?Pen->Level+1:1;const FName Species=Pen?Pen->Definition:Option;
        if(Level>3 || Camp->State.Tier<Level*2 || !HearthwardNature::Definition(TEXT("domestic"),Species) || (!Pen && !ClearPlot(Site,200)))return false;
        const TMap<FName,int32> Cost={{TEXT("wood"),60*Level},{TEXT("stone"),20*Level},{TEXT("rope"),10*Level}};
        if(!GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Workshop(Bag(),Cost,{},!Camp->State.CampAt(Pen?Pen->Position:Site).IsNone()))return false;
        if(!Pen){FHearthwardPen New;New.Id=FGuid::NewGuid();New.Position=Site;New.Definition=Species;State.Pens.Add(New);Pen=&State.Pens.Last();}
        Pen->Level=Level;for(const auto& M:Cost)Pen->Paid.FindOrAdd(M.Key)+=M.Value;return true;
    }
    if(Pen)
    {
        if(Action==TEXT("deposit_feed") && Pen->Feed<=MAX_int32-Count && PrepareBag({{TEXT("feed"),Count}},{},Next))
        {Pen->Feed+=Count;PublishBag(Next);return true;}
        if(Action==TEXT("withdraw_feed") && Pen->Feed>=Count && PrepareBag({},{{TEXT("feed"),Count}},Next))
        {Pen->Feed-=Count;PublishBag(Next);return true;}
        if(Pen->Feed==0 && State.Occupants(Id)==0)
        {
            if(Action==TEXT("move_pen") && ClearPlot(Site,200,Id)){Pen->Position=Site;return true;}
            if(Action==TEXT("demolish_pen") && Option==TEXT("confirmed") && PrepareBag({},HearthwardCamp::Refund(Pen->Paid),Next))
            {State.Pens.RemoveAll([&](const auto& P){return P.Id==Id;});PublishBag(Next);return true;}
        }
    }
    if(Animal)
    {
        if(Action==TEXT("loot") && Animal->Health<=0 && !Animal->Loot.IsEmpty() && PrepareBag({},Animal->Loot,Next))
        {Animal->Loot.Reset();PublishBag(Next);return true;}
        if(Action==TEXT("slaughter") && Option==TEXT("confirmed") && Animal->Domestic && Animal->Captured && Animal->Health>0)
        {
            const auto D=HearthwardNature::Definition(TEXT("domestic"),Animal->Definition);const int32 N=Number(D,Animal->Juvenile?TEXT("juvenile_meat"):TEXT("meat"));
            if(!PrepareBag({},{{TEXT("meat"),N}},Next))return false;
            Animal->Health=0;Animal->Rewarded=true;Animal->Pen.Invalidate();Animal->ReservedPen.Invalidate();Animal->Following=false;
            PublishBag(Next);return true;
        }
        if(Action==TEXT("capture") && Animal->Domestic && Animal->Health>0 && !Animal->Captured)
        {
            auto* Home=State.Pens.FindByPredicate([&](const auto& P){return P.Definition==Animal->Definition && State.Occupants(P.Id)<HearthwardNature::Capacity(P.Level);});
            if(!Home || !PrepareBag({{TEXT("feed"),1},{TEXT("rope"),1}},{},Next))return false;
            Animal->ReservedPen=Home->Id;Animal->Captured=true;Animal->Following=true;PublishBag(Next);return true;
        }
        if(Action==TEXT("lead") && Animal->Domestic && Animal->Captured && Animal->Health>0)
        {
            if(!Animal->ReservedPen.IsValid())
            {
                auto* Home=State.Pens.FindByPredicate([&](const auto& P){return P.Id!=Animal->Pen && P.Definition==Animal->Definition && State.Occupants(P.Id)<HearthwardNature::Capacity(P.Level);});
                if(!Home)return false;Animal->Pen.Invalidate();Animal->ReservedPen=Home->Id;
            }
            Animal->Following=true;return true;
        }
    }
    if(Action==TEXT("claim") && Point && (!Point->Pending.Stacks.IsEmpty() || !Point->Pending.Instances.IsEmpty()))
    {
        if(Point->Kind==TEXT("treasure") && (!State.Maps.Contains(Point->Definition) || State.Opened.Contains(Point->Definition)))return false;
        if(!PrepareBag({},Point->Pending.Stacks,Next))return false;
        for(const auto& I:Point->Pending.Instances)if(Next.InsertInstance(I)!=NatureResult::Success)return false;
        Point->Pending={};if(Point->Kind==TEXT("treasure"))State.Opened.Add(Point->Definition);PublishBag(Next);return true;
    }
    return false;
}
bool UHearthwardNatureSubsystem::ReadMap(FName Item)
{
    if(Busy() || !Safe(GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch()) || State.Maps.Contains(Item))return false;
    const auto D=HearthwardNature::Definition(TEXT("rewards"),Item);if(!D || Text(D,TEXT("kind"))!=TEXT("map"))return false;
    FHearthwardInventoryState Next;if(!PrepareBag({{Item,1}},{},Next))return false;
    auto* Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();if(Camp->State.Camps.IsEmpty())return false;
    FVector At=FVector::ZeroVector;const int32 Index=FCString::Atoi(*Item.ToString().Right(1));bool Found=false;
    for(int32 I=0;I<24 && !Found;++I)
    {const double A=Index*1.8+I*.13;Found=Ground(Camp->State.Camps[0].Position+FVector(FMath::Cos(A)*(23000+Index*9000),FMath::Sin(A)*(23000+Index*9000),0),At,true);}
    if(!Found)return false;
    FHearthwardInventoryState Loot(true);for(const auto& M:HearthwardCamp::Counts(D,TEXT("outputs")))if(Loot.Add(M.Key,M.Value)!=NatureResult::Success)return false;
    FHearthwardNaturePoint P;P.Id=FGuid::NewGuid();P.Key=Item;P.Kind=TEXT("treasure");P.Definition=Item;P.Position=At;P.Pending=Loot.Snapshot();State.Points.Add(P);
    State.Rewards.Add(Item);State.Maps.Add(Item);PublishBag(Next);Gameplay()->SetWaypoint(At);RebuildActors();Feedback=TEXT("藏宝地点已标记");return true;
}
bool UHearthwardNatureSubsystem::DamageAnimal(FName Target,float Health)
{
    FGuid Id;if(!FGuid::Parse(Target.ToString(),Id))return false;
    auto* A=State.Animals.FindByPredicate([&](const auto& X){return X.Id==Id;});if(!A)return false;
    const float Before=A->Health;A->Health=FMath::Clamp(Health,0.f,Before);A->AlertRemaining=15;A->Following=false;
    if(!A->Domestic && Before>A->Health)Gameplay()->NotifyCombat();
    if(Before>0 && A->Health<=0 && !A->Rewarded)
    {
        A->Rewarded=true;const auto D=HearthwardNature::Definition(A->Domestic?TEXT("domestic"):TEXT("wildlife"),A->Definition);
        A->Loot.Add(TEXT("meat"),Number(D,A->Juvenile?TEXT("juvenile_meat"):TEXT("meat")));
        if(Number(D,TEXT("hide"))>0)A->Loot.Add(TEXT("hide"),Number(D,TEXT("hide")));
        A->Pen.Invalidate();A->ReservedPen.Invalidate();
        if(!A->Domestic)
        {
            if(auto* Slot=State.Slots.FindByPredicate([&](const auto& S){return S.Current==Id;}))Slot->Due=State.Calendar+2880;
            Gameplay()->GrantExperience(TEXT("hunt"),FName(TEXT("nature_")+Id.ToString()),GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch());
        }
    }
    return true;
}
