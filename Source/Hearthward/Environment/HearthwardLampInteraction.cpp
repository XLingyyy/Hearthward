#include "HearthwardLampInteraction.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Experience/HearthwardPresentationComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
namespace { TArray<TWeakObjectPtr<UHearthwardLampInteraction>> RegisteredLamps; }
void UHearthwardLampInteraction::BeginPlay(){Super::BeginPlay();RegisteredLamps.AddUnique(this);}
void UHearthwardLampInteraction::EndPlay(const EEndPlayReason::Type Reason){RegisteredLamps.Remove(this);Super::EndPlay(Reason);}

bool UHearthwardLampInteraction::IsLit() const
{
    const auto& Lamps=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->InteractionState().Lamps;
    const auto* State=Lamps.FindByPredicate([&](const auto& S){return S.Id==Identity;});
    return State?State->Lit:InitiallyLit;
}
FString UHearthwardLampInteraction::GetInteractionPrompt(AActor*) const
{
    return IsLit()?TEXT("E 熄灭油灯 · 减少局部照明"):TEXT("E 点亮油灯");
}
FString UHearthwardLampInteraction::CompleteInteraction(AActor* Interactor)
{
    if(!IsValid(Interactor) || GetWorld()->IsPaused() || FVector::Dist(Interactor->GetActorLocation(),GetComponentLocation())>MaxDistance)return TEXT("已离开灯具");
    auto* Survival=Interactor->FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!Survival || !Survival->Alive())return TEXT("当前无法操作灯具");
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Round2Lamp),false,Interactor);Query.AddIgnoredActor(GetOwner());
    if(GetWorld()->LineTraceTestByChannel(Interactor->GetActorLocation(),GetComponentLocation(),ECC_Visibility,Query))return TEXT("灯具被遮挡");
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    if(!Save->BindInteractionTarget(Identity,EHearthwardPersistentKind::Lamp,GetOwner()))return TEXT("灯具身份尚未准备好");
    const auto Ticket=Save->BeginInteraction(Identity);if(!Ticket.Operation.IsValid())return TEXT("请先完成当前交互");
    auto Next=Save->InteractionState();auto* State=Next.Lamps.FindByPredicate([&](const auto& S){return S.Id==Identity;});
    if(!State){FHearthwardLampState S;S.Id=Identity;S.Lit=InitiallyLit;State=&Next.Lamps.Add_GetRef(S);}
    State->Lit=!State->Lit;
    if(!Save->CommitInteraction(Ticket,Next)){Save->CancelInteraction();return TEXT("灯具操作取消，原状态保留");}
    if(Light)Light->SetVisibility(IsLit());
    if(auto* Audio=Interactor->FindComponentByClass<UHearthwardPresentationComponent>())Audio->InteractionSucceeded(TEXT("interaction.lamp"),Ticket.Operation,GetComponentLocation());
    OnInteractionReady.Broadcast(Interactor);return IsLit()?TEXT("油灯已点亮"):TEXT("油灯已熄灭 · 已发现你的敌人仍会追踪");
}
void UHearthwardLampInteraction::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Delta,Type,Tick);if(Light && Light->IsVisible()!=IsLit())Light->SetVisibility(IsLit());
}
double UHearthwardLampInteraction::Illumination(UWorld* World,const FVector& Position,double Ambient,const AActor* Subject)
{
    double Result=FMath::Clamp(Ambient,.15,1.);
    RegisteredLamps.RemoveAll([](const auto& Lamp){return !Lamp.IsValid();});
    for(const auto& Weak:RegisteredLamps)
        if(const auto* Lamp=Weak.Get();Lamp && Lamp->GetWorld()==World && Lamp->IsLit())
        {
            const double Distance=FVector::Dist(Position,Lamp->GetComponentLocation());if(Distance>=800)continue;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(Round2LampLight),false,Lamp->GetOwner());
            Query.AddIgnoredActor(Subject);
            if(!World->LineTraceTestByChannel(Lamp->GetComponentLocation(),Position,ECC_Visibility,Query))Result=FMath::Max(Result,1.-Distance/800.);
        }
    return Result;
}
