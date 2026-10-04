#include "HearthwardCampaignSubsystem.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
using namespace HearthwardData;
APawn* UHearthwardCampaignSubsystem::Player() const { return UGameplayStatics::GetPlayerPawn(GetWorld(),0); }
UHearthwardGameplayComponent* UHearthwardCampaignSubsystem::Gameplay() const
{ return Player()?Player()->FindComponentByClass<UHearthwardGameplayComponent>():nullptr; }
bool UHearthwardCampaignSubsystem::Safe() const
{
    const auto* G=Gameplay();
    return Active() && G && G->CanChangeSkills() && !UHearthwardSurvivalComponent::HasFailed(GetWorld()) && !GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->Busy();
}
void UHearthwardCampaignSubsystem::Record(FName Fact)
{ if(Active() && !Fact.IsNone())State.Facts.Add(Fact); }
bool UHearthwardCampaignSubsystem::Available(FName Id) const
{
    const auto R=HearthwardCampaign::Find(TEXT("quests"),Id);if(!Active() || !R || !Gameplay())return false;
    if(Id==TEXT("main_01") && State.Legacy)return false;
    for(const auto& V:R->GetArrayField(TEXT("requires")))
        if(!(State.Legacy && V->AsString()==TEXT("main_01")) && !Gameplay()->Claimed.Contains(FName(*V->AsString())))return false;
    return State.Phase!=TEXT("prologue") || Id==TEXT("main_01");
}
TArray<bool> UHearthwardCampaignSubsystem::Conditions(FName Id) const
{
    const auto* G=Gameplay();if(!G)return {};
    const auto& C=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    auto Fact=[&](const TCHAR* F){return State.Facts.Contains(FName(F));};
    auto Facility=[&](FName Kind){return C.Facilities.ContainsByPredicate([&](const auto& F){return F.Kind==Kind;});};
    auto Arrived=[&](int32 I){return C.Rescued.Contains(FName(*FString::Printf(TEXT("rescued_%02d"),I)));};
    const auto R=HearthwardCampaign::Find(TEXT("quests"),Id);if(!R)return {};
    if(Id==TEXT("main_01"))return {Fact(TEXT("relic")),Fact(TEXT("prologue_order")),Fact(TEXT("prologue_complete"))};
    if(Id==TEXT("main_02"))return {G->Events.FindRef(TEXT("harvest:wood"))>0 && G->Events.FindRef(TEXT("harvest:stone"))>0,Facility(TEXT("workbench")) && Fact(TEXT("worker_assigned")),Fact(TEXT("camp_order"))};
    if(Id==TEXT("main_03"))return {G->Discovered.Contains(TEXT("slice_rescue")),State.People.ContainsByPredicate([](const auto& P){return P.Id==TEXT("rescued_01") && P.Stage!=TEXT("uncontacted");}),Arrived(1)};
    if(Id==TEXT("main_04"))return {Facility(TEXT("workbench")) && Arrived(1),C.Tier>=2,Fact(TEXT("tier2_viewed"))};
    if(Id==TEXT("main_05"))return {G->Activated.Contains(TEXT("route_ford")),Fact(TEXT("watch_survey")),G->Discovered.Contains(TEXT("home_entry"))};
    if(Id==TEXT("main_06"))return {State.Flags.Num()>=1,State.Flags.Num()>=1,State.Flags.Num()>=2};
    if(Id==TEXT("main_07"))return {State.ReadyForVictory(),State.Flags.Num()==4,State.Victory};
    if(Id==TEXT("main_08"))return {Fact(TEXT("home_storage")),Fact(TEXT("home_work")),Fact(TEXT("home_continued"))};
    if(Id==TEXT("side_01") || Id==TEXT("side_02") || Id==TEXT("side_03") || Id==TEXT("side_04"))
    {
        bool Contact=true,Home=true;
        for(const auto& V:R->GetArrayField(TEXT("rescued_people")))
        {
            const FName Person(*V->AsString());const auto* P=State.People.FindByPredicate([&](const auto& A){return A.Id==Person;});
            Contact&=P && P->Stage!=TEXT("uncontacted");Home&=C.Rescued.Contains(Person);
        }
        return {Contact,Home,Home};
    }
    if(Id==TEXT("side_05"))return {Fact(TEXT("hunter_record")),Fact(TEXT("hunter_confirmed"))};
    if(Id==TEXT("side_06"))return {Fact(TEXT("mine_source")),Fact(TEXT("delivery:side_06"))};
    if(Id==TEXT("side_07"))return {G->Discovered.Contains(TEXT("route_ford")),G->Activated.Contains(TEXT("route_ford"))};
    if(Id==TEXT("side_08"))return {G->Discovered.Contains(TEXT("route_ridge")),Fact(TEXT("ridge_survey"))};
    if(Id==TEXT("side_09"))return {G->Events.FindRef(TEXT("harvest:herb"))>0,Fact(TEXT("delivery:side_09"))};
    if(Id==TEXT("side_10"))return {Fact(TEXT("old_carving")),Fact(TEXT("placed_carving"))};
    if(Id==TEXT("side_11"))return {Fact(TEXT("grain_planted")),Fact(TEXT("grain_harvested"))};
    if(Id==TEXT("side_12"))return {Fact(TEXT("workshop_cache")),Fact(TEXT("forge_visited"))};
    if(Id==TEXT("side_13"))return {Fact(TEXT("family_letter")),Fact(TEXT("delivered_letter"))};
    if(Id==TEXT("side_14"))return {Fact(TEXT("watch_record")),Fact(TEXT("copied_record"))};
    if(Id==TEXT("side_15"))return {C.DonatedPoints>=10,Fact(TEXT("public_meal"))};
    return {};
}
int32 UHearthwardCampaignSubsystem::Progress(FName Id) const
{ int32 N=0;for(bool Done:Conditions(Id))N+=Done?1:0;return N; }
FName UHearthwardCampaignSubsystem::QuestLocation(FName Id) const
{
    if(Id==TEXT("main_01"))return State.Facts.Contains(TEXT("relic"))?FName(TEXT("prologue_exit")):FName(TEXT("prologue_relic"));
    if(Id==TEXT("main_03"))
    {
        const auto* Person=State.People.FindByPredicate([](const auto& P){return P.Id==TEXT("rescued_01");});
        if(Person && Person->Stage==TEXT("following"))return TEXT("camp");
    }
    if(Id==TEXT("main_05"))
    {
        if(!Gameplay()->Activated.Contains(TEXT("route_ford")))return TEXT("route_ford");
        return State.Facts.Contains(TEXT("watch_survey"))?FName(TEXT("home_entry")):FName(TEXT("route_watch"));
    }
    const auto Row=HearthwardCampaign::Find(TEXT("quests"),Id);
    return Row?FName(*Text(Row,TEXT("location"))):NAME_None;
}
bool UHearthwardCampaignSubsystem::Claim(FName Id,FGuid Epoch)
{
    auto* G=Gameplay();auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    const auto R=HearthwardCampaign::Find(TEXT("quests"),Id);
    if(!Safe() || Busy() || !R || Epoch!=Store->GetTimelineEpoch() || !Available(Id) || G->Claimed.Contains(Id))return false;
    TMap<FName,int32> Inputs;FName Delivery;
    auto Ready=Conditions(Id);
    // The journal button explicitly confirms the two material deliveries at camp.
    if(Id==TEXT("side_06") || Id==TEXT("side_09"))
    {
        if(GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(Player()->GetActorLocation()).IsNone() || !Conditions(Id)[0])return false;
        Delivery=FName(*(TEXT("delivery:")+Id.ToString()));
        if(!State.Facts.Contains(Delivery))Inputs.Add(Id==TEXT("side_06")?FName(TEXT("ore")):FName(TEXT("herb")),6);
        Ready[1]=true;
    }
    if(Ready.Num()!=R->GetArrayField(TEXT("steps")).Num() || Ready.Contains(false))return false;
    const auto Reward=R->GetObjectField(TEXT("reward"));const FName Fact(*Text(Reward,TEXT("id")));
    if(G->RewardFacts.Contains(Fact))return false;
    TMap<FName,int32> Items;
    for(const auto& I:Reward->GetObjectField(TEXT("items"))->Values)Items.Add(FName(*I.Key),I.Value->AsNumber());
    if(!Store->CanAdjust(Inputs,Items)){Feedback=TEXT("共享仓储材料不足，未扣取物品");return false;}
    const int32 OldXP=G->Experience;const auto OldFacts=G->RewardFacts;
    if(!G->GrantExperience(Text(R,TEXT("category"))==TEXT("main")?FName(TEXT("main_milestone")):FName(TEXT("side_quest")),Fact,Epoch))return false;
    if(!Store->Adjust(Inputs,Items)){G->Experience=OldXP;G->RewardFacts=OldFacts;return false;}
    if(!Delivery.IsNone())State.Facts.Add(Delivery);
    G->Claimed.Add(Id);
    if(auto* Presentation=Player()->FindComponentByClass<UHearthwardPresentationComponent>()) Presentation->PlayFixedCue(FName(*(TEXT("fixed.")+Id.ToString()+TEXT(".complete"))),FGuid::NewGuid(),true);
    for(const auto& V:HearthwardCampaign::Rows(TEXT("quests")))
    {const FName Next(*Text(V->AsObject(),TEXT("id")));if(Text(V->AsObject(),TEXT("category"))==TEXT("main") && Available(Next) && !G->Claimed.Contains(Next)){G->TrackedQuest=Next;break;}}
    Feedback=TEXT("任务完成，奖励已登记到共享仓储");G->Feedback=Feedback;G->OnChanged.Broadcast();return true;
}
