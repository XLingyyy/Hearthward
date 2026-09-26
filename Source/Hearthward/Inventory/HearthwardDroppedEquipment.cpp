#include "HearthwardDroppedEquipment.h"
#include "HearthwardInventoryComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
AHearthwardDroppedEquipment::AHearthwardDroppedEquipment()
{
    auto* Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bundle"));SetRootComponent(Mesh);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Hearthward/Assets/Demo/chest/wood_chest_model.wood_chest_model")));
    Mesh->SetRelativeScale3D(FVector(30.f/FMath::Max(1.f,Mesh->GetStaticMesh()->GetBounds().BoxExtent.GetMax()*2)));Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Target=CreateDefaultSubobject<UHearthwardEquipmentPickup>(TEXT("Pickup"));Target->SetupAttachment(Mesh);Target->MaxDistance=300;
}
bool AHearthwardDroppedEquipment::PickUp(AActor* Player)
{
    auto* G=Player?Player->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    auto* Bag=Player?Player->FindComponentByClass<UHearthwardInventoryComponent>():nullptr;
    if(IsActorBeingDestroyed() || !G || !Bag || !G->CanChangeSkills() || FVector::Dist(Player->GetActorLocation(),GetActorLocation())>300)return false;
    if(Bag->InsertInstance(Item,false)!=EHearthwardInventoryResult::Success)return false;
    Destroy();Bag->OnInventoryChanged.Broadcast();return true;
}
FString UHearthwardEquipmentPickup::GetInteractionPrompt(AActor* Player) const
{
    const auto* Bundle=Cast<AHearthwardDroppedEquipment>(GetOwner());
    return Bundle?TEXT("E 拾取 ")+HearthwardData::Text(HearthwardData::Find(TEXT("items"),Bundle->Item.Definition.ToString()),TEXT("name")):FString();
}
FString UHearthwardEquipmentPickup::CompleteInteraction(AActor* Player)
{
    auto* Bundle=Cast<AHearthwardDroppedEquipment>(GetOwner());
    return Bundle && Bundle->PickUp(Player)?TEXT("已拾取，耐久保留"):TEXT("当前无法拾取，请检查距离、容量与战斗状态");
}
