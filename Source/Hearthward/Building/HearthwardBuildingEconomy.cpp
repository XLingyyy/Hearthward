#include "HearthwardBuildingComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "Engine/World.h"

AActor* UHearthwardBuildingComponent::ResolveFacility(FGuid Id) const
{const auto* B=Built.FindByPredicate([&](const auto& Entry){return Entry.Id==Id;});return B?B->Actor.Get():nullptr;}
bool UHearthwardBuildingComponent::CanUseFacility(FGuid Id) const
{
    const auto* A=ResolveFacility(Id);const auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    if(!A || !G || !G->Enabled || G->Health<=0 || G->InCombat() || IsBuilding() || IsPlacing() || FVector::Dist(GetOwner()->GetActorLocation(),A->GetActorLocation())>400)return false;
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(CampFacility),false,GetOwner());Query.AddIgnoredActor(A);
    return !GetWorld()->LineTraceSingleByChannel(Hit,GetOwner()->GetActorLocation(),A->GetActorLocation(),ECC_Visibility,Query);
}
bool UHearthwardBuildingComponent::ReserveMaterials()
{
    ReleaseMaterials();auto* Bag=GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>();auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    for(const auto& M:Materials())
    {
        const int32 Personal=FMath::Min(M.Value,Bag->Available(M.Key));
        if(Personal>0)PersonalMaterials.Add(M.Key,Personal);
        if(M.Value>Personal)SharedMaterials.Add(M.Key,M.Value-Personal);
    }
    MaterialTicket=FGuid::NewGuid();
    if(!Bag->ReserveMaterials(PersonalMaterials) || !Store->Reserve(MaterialTicket,SharedMaterials)) {ReleaseMaterials();return false;}
    return true;
}
void UHearthwardBuildingComponent::ReleaseMaterials()
{
    if(auto* Bag=GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>())Bag->ReleaseMaterials();
    GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Release(MaterialTicket);
    MaterialTicket.Invalidate();PersonalMaterials.Reset();SharedMaterials.Reset();
}
bool UHearthwardBuildingComponent::CommitMaterials()
{
    auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();auto* Bag=GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>();
    if(!Store->Adjust(SharedMaterials,{},MaterialTicket))return false;
    if(!Bag->CommitMaterials(false)){Store->Adjust({},SharedMaterials);return false;}
    MaterialTicket.Invalidate();PersonalMaterials.Reset();SharedMaterials.Reset();
    return true;
}
bool UHearthwardBuildingComponent::UpgradeFacility(FGuid Id,FGuid Epoch)
{
    auto* Economy=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    auto* F=Economy->State.Facilities.FindByPredicate([&](const auto& Entry){return Entry.Id==Id;});
    const auto* B=Built.FindByPredicate([&](const auto& Entry){return Entry.Id==Id;});
    if(!Economy->CanManage(Epoch) || !F || !B || !CanUseFacility(Id) || HearthwardCamp::RequiredTier(F->Kind,F->Level+1)>Economy->State.Tier)
    {SetFeedback(TEXT("等级尚未解锁或请先靠近设施"));return false;}
    if(!SelectBuilding(F->Kind))return false;
    Editing=Id;Upgrading=true;Placement=B->Position;Yaw=B->Rotation;F->Paused=true;
    if(!ConfirmPlacement()){ClearPreview();return false;}return true;
}
bool UHearthwardBuildingComponent::MoveFacility(FGuid Id,FGuid Epoch)
{
    auto* Economy=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    auto* F=Economy->State.Facilities.FindByPredicate([&](const auto& Entry){return Entry.Id==Id;});
    if(!Economy->CanManage(Epoch) || !F || !CanUseFacility(Id))return false;
    if(!SelectBuilding(F->Kind))return false;Editing=Id;F->Paused=true;SetFeedback(TEXT("免费移动：选择合法新位置，原生产进度保留"));return true;
}
bool UHearthwardBuildingComponent::DemolishFacility(FGuid Id,bool ConfirmLoss,FGuid Epoch)
{
    auto* Economy=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    if(!Economy->CanManage(Epoch) || !CanUseFacility(Id))return false;
    TGuardValue<bool> Guard(Settling,true);TGuardValue<bool> EconomyGuard(Economy->Settling,true);
    if(!Economy->RemoveFacility(Id,ConfirmLoss)){SetFeedback(TEXT("需要确认未完成批次投入损失，或仓储数量已满"));return false;}
    for(auto& B:Built)if(B.Id==Id && B.Actor.IsValid())B.Actor->Destroy();
    Built.RemoveAll([&](const auto& B){return B.Id==Id;});SetFeedback(TEXT("已拆除，按累计实付材料向下返还80%至共享仓储"));return true;
}
bool UHearthwardBuildingComponent::AddGift(FName Kind,FVector Position)
{
    const auto Recipe=HearthwardData::Find(TEXT("buildings"),Kind.ToString());if(!Recipe)return false;
    auto* Economy=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    const bool Hometown=Economy->State.CampAt(Position)==TEXT("hometown");
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CampGift),false);
    // Try the preferred spot first, then a bounded layout within the same hometown camp.
    const FVector Offsets[]={FVector::ZeroVector,FVector(200,0,0),FVector(-200,0,0),FVector(0,200,0),FVector(0,-200,0),
        FVector(200,200,0),FVector(-200,200,0),FVector(200,-200,0),FVector(-200,-200,0),
        FVector(400,0,0),FVector(-400,0,0),FVector(0,400,0),FVector(0,-400,0)};
    for(const FVector Offset:Offsets)
    {
        if(!Hometown && !Offset.IsZero())break;
        FVector Candidate=Position+Offset;
        if(Hometown)
        {
            if(Economy->State.CampAt(Candidate)!=TEXT("hometown")
                || !GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Ground(Candidate,Candidate))continue;
        }
        else
        {
            FHitResult Ground;
            if(!GetWorld()->LineTraceSingleByChannel(Ground,Candidate+FVector(0,0,1000),Candidate-FVector(0,0,3000),ECC_Visibility,Query))continue;
            Candidate=Ground.ImpactPoint;
        }
        FString Reason;if(!CheckGeometry(Recipe,Candidate,0,Query,Reason))continue;
        auto* Actor=SpawnBuilding(Kind,Candidate,0,false);if(!Actor)return false;
        const FGuid Id=FGuid::NewGuid();Built.Add({Id,Kind,Candidate,0,Actor});
        Economy->RegisterFacility(Id,Kind,Candidate,{});return true;
    }
    return false;
}
