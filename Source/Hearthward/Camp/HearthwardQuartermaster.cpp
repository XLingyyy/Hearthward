#include "HearthwardCampSubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
void UHearthwardCampSubsystem::RefreshQuartermasters()
{
    AHearthwardCompanionFixture* Model=nullptr;
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It){Model=*It;break;}
    if(!Model || !Model->GetMesh()->GetSkeletalMeshAsset())return;
    for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("Hearthward.Quartermaster")))It->Destroy();
    for(const auto& Camp:State.Camps)
    {
        FVector Position=Camp.Position+FVector(-300,400,300);
        FHitResult Ground;FCollisionQueryParams Q(SCENE_QUERY_STAT(QuartermasterGround),false,Model);
        if(GetWorld()->LineTraceSingleByChannel(Ground,Position+FVector(0,0,1000),Position-FVector(0,0,3000),ECC_WorldStatic,Q))Position=Ground.ImpactPoint+FVector(0,0,90);
        FActorSpawnParameters Parameters;Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        auto* NPC=GetWorld()->SpawnActor<ACharacter>(Position,FRotator(0,0,0),Parameters);
        if(!NPC)continue;
        NPC->Tags.Add(TEXT("Hearthward.Quartermaster"));NPC->Tags.Add(Camp.Id);
        NPC->GetMesh()->SetSkeletalMeshAsset(Model->GetMesh()->GetSkeletalMeshAsset());
        NPC->GetMesh()->SetRelativeTransform(Model->GetMesh()->GetRelativeTransform());NPC->GetMesh()->SetAnimInstanceClass(Model->GetMesh()->GetAnimClass());
        for(int32 I=0;I<Model->GetMesh()->GetNumMaterials();++I)NPC->GetMesh()->SetMaterial(I,Model->GetMesh()->GetMaterial(I));
        auto* Name=NewObject<UTextRenderComponent>(NPC);NPC->AddInstanceComponent(Name);Name->SetupAttachment(NPC->GetRootComponent());
        Name->SetText(FText::FromString(TEXT("营地后勤员 · Tab → 行装管理")));Name->SetWorldSize(18);Name->SetRelativeLocation(FVector(0,0,120));Name->RegisterComponent();
    }
}
