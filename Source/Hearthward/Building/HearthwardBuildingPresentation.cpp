#include "HearthwardBuildingComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"

void UHearthwardBuildingComponent::UpdatePresentation()
{
    const auto& State=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    for(const auto& B:Built)
    {
        auto* Actor=B.Actor.Get();
        if(!Actor)continue;
        const auto* Facility=State.Facilities.FindByPredicate([&](const auto& F){return F.Id==B.Id;});
        if(!Facility)continue;
        const FString Label=B.Recipe==TEXT("workbench")?TEXT("Workbench"):B.Recipe==TEXT("smelter")?TEXT("Smelter"):
            B.Recipe==TEXT("forge")?TEXT("Forge"):B.Recipe==TEXT("cooking")?TEXT("Cooking"):TEXT("");
        TArray<UStaticMeshComponent*> Parts;Actor->GetComponents(Parts);
        for(int32 Level=1;Level<=3;++Level)
        {
            if(Label.IsEmpty() || (Level==1 && B.Recipe!=TEXT("forge")))continue;
            const FName Tag(*FString::Printf(TEXT("Hearthward.FacilityUpgrade.%d"),Level));
            auto** Existing=Parts.FindByPredicate([&](const auto* P){return P->ComponentHasTag(Tag);});
            UStaticMeshComponent* Part=Existing?*Existing:nullptr;
            if(!Part && Facility->Level>=Level)
            {
                const FString Name=Level==1?TEXT("SM_ForgeHearth"):FString::Printf(TEXT("SM_%s_L%d"),*Label,Level);
                Part=NewObject<UStaticMeshComponent>(Actor);Actor->AddInstanceComponent(Part);Part->SetupAttachment(Actor->GetRootComponent());
                Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-098/Upgrades/%s.%s"),*Name,*Name)));
                Part->SetRelativeLocation(FVector(0,0,B.Position.Z-Actor->GetActorLocation().Z));
                Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCanEverAffectNavigation(false);
                Part->ComponentTags.Add(Tag);Part->RegisterComponent();
            }
            if(Part)Part->SetVisibility(Facility->Level>=Level);
        }
        if(B.Recipe!=TEXT("campfire") && B.Recipe!=TEXT("smelter") && B.Recipe!=TEXT("cooking") && B.Recipe!=TEXT("forge"))continue;
        bool Burning=Facility && !Facility->Paused && B.Recipe==TEXT("campfire");
        if(Facility && !Facility->Paused && !Burning)
            for(const auto& R:State.Regions)
                if(R.Facility==B.Id && R.Camp==Facility->Camp && R.Enabled && R.Safe && R.Batch.Active
                    && R.Batch.Work<R.Batch.Required && (R.Status.IsEmpty() || R.Status==TEXT("生产中"))
                    && (R.Workers.Num()>0 || (R.Player && R.PlayerEfficiency>0) || (R.Brother && R.BrotherEfficiency>0)))
                    {Burning=true;break;}
        // Keep distant facilities free of ticking particle systems; gameplay continues independently.
        Burning=Burning && FVector::DistSquared(Actor->GetActorLocation(),GetOwner()->GetActorLocation())<FMath::Square(5000.f);
        TArray<UNiagaraComponent*> Effects;Actor->GetComponents(Effects);
        if(Burning && Effects.IsEmpty())
        {
            const bool Tall=B.Recipe==TEXT("smelter");
            const float Height=Tall?108.f:B.Recipe==TEXT("forge")?57.f:18.f;
            const float Ground=B.Position.Z-Actor->GetActorLocation().Z;
            const float Side=B.Recipe==TEXT("forge")?-56.f:0.f;
            for(int32 I=0;I<2;++I)
            {
                auto* Effect=NewObject<UNiagaraComponent>(Actor);
                Actor->AddInstanceComponent(Effect);Effect->SetupAttachment(Actor->GetRootComponent());
                Effect->SetAutoActivate(false);Effect->SetAutoDestroy(false);
                Effect->SetAsset(LoadObject<UNiagaraSystem>(nullptr,I?
                    TEXT("/Game/Hearthward/Assets/TASK-096/Fire/NS_HearthSmoke.NS_HearthSmoke"):
                    TEXT("/Game/Hearthward/Assets/TASK-096/Fire/NS_HearthFire.NS_HearthFire")));
                Effect->SetRelativeLocation(FVector(0,Side,Ground+Height+(I?15:0)));
                Effect->SetRelativeScale3D(FVector(I?.22f:Tall?.18f:.3f));
                Effect->SetCanEverAffectNavigation(false);Effect->SetCastShadow(false);
                Effect->ComponentTags.Add(TEXT("Hearthward.FacilityFire"));Effect->RegisterComponent();Effects.Add(Effect);
            }
            auto* Light=NewObject<UPointLightComponent>(Actor);
            Actor->AddInstanceComponent(Light);Light->SetupAttachment(Actor->GetRootComponent());
            Light->SetRelativeLocation(FVector(0,Side,Ground+Height+25));Light->SetIntensity(650);
            Light->SetLightColor(FLinearColor(1,.39f,.1f));Light->SetAttenuationRadius(300);Light->SetCastShadows(false);
            Light->RegisterComponent();
        }
        for(auto* Effect:Effects)
            if(Effect->ComponentHasTag(TEXT("Hearthward.FacilityFire")))
            {
                if(Burning && !Effect->IsActive())Effect->Activate(true);
                else if(!Burning && Effect->IsActive())Effect->DeactivateImmediate();
            }
        TArray<UPointLightComponent*> Lights;Actor->GetComponents(Lights);
        for(auto* Light:Lights)Light->SetVisibility(Burning);
    }
}
