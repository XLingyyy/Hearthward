#include "HearthwardCombatTargetComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"

FName UHearthwardCombatTargetComponent::HitPart(const FHitResult& Hit) const
{
    if(const auto* P=BoneParts.Find(Hit.BoneName)) return *P;
    if(Hit.GetComponent()) for(FName Part:{FName("head"),FName("body"),FName("legs"),FName("feet")})
        if(Hit.GetComponent()->ComponentHasTag(Part)) return Part;
    return NAME_None;
}
void UHearthwardCombatTargetComponent::CreateBodyCollision()
{
    const FName Parts[]={TEXT("feet"),TEXT("legs"),TEXT("body"),TEXT("head")};
    const float Z[]={-44,-23,13,43}; const float Half[]={6,15,21,9};
    for(int32 I=0;I<4;++I)
    {
        auto* Box=NewObject<UBoxComponent>(GetOwner()); GetOwner()->AddInstanceComponent(Box);
        Box->SetupAttachment(GetOwner()->GetRootComponent()); Box->SetBoxExtent(FVector(25,25,Half[I]));
        Box->SetRelativeLocation(FVector(0,0,Z[I])); Box->ComponentTags.Add(Parts[I]);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionResponseToAllChannels(ECR_Ignore);
        Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block); Box->RegisterComponent();
    }
}
void UHearthwardCombatTargetComponent::SetCorpse()
{
    Health=0; ExecutionOwner.Reset(); Memory.Seen.Reset(); Awareness=TEXT("已清除");
    if(auto* C=Cast<ACharacter>(GetOwner()))
    {
        C->GetCharacterMovement()->StopMovementImmediately();
        if(auto* AI=Cast<AAIController>(C->GetController())) AI->StopMovement();
    }
    GetOwner()->SetActorRotation(FRotator(0,GetOwner()->GetActorRotation().Yaw,90));
}
FHearthwardCombatTargetSave UHearthwardCombatTargetComponent::Snapshot() const
{
    auto S=Memory; S.ArmorDurability=ArmorDurability; S.Id=Id; S.Region=Region; S.Health=Health;
    S.Position=GetOwner()->GetActorLocation(); S.Rotation=GetOwner()->GetActorRotation(); return S;
}
void UHearthwardCombatTargetComponent::Restore(const FHearthwardCombatTargetSave& S)
{
    Memory=S; ArmorDurability=S.ArmorDurability; Health=S.Health; Region=S.Region; ExecutionOwner.Reset(); Carrier.Reset();
    GetOwner()->SetActorLocationAndRotation(S.Position,S.Rotation,false,nullptr,ETeleportType::TeleportPhysics);
    if(Health<=0) SetCorpse();
}
