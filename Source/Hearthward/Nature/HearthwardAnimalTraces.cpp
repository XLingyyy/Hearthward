#include "HearthwardAnimalTraces.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "HearthwardNatureSubsystem.h"
#include "HearthwardNatureActor.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"

FString UHearthwardTrailInteraction::CompleteInteraction(AActor* Player)
{
    const auto* S=IsValid(Player)?Player->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
    if(!S || !S->Alive() || GetWorld()->IsPaused() || FVector::Dist(Player->GetActorLocation(),GetComponentLocation())>MaxDistance)return TEXT("已经离开踪迹");
    return AHearthwardAnimalTraces::DescribeAt(GetWorld(),GetComponentLocation(),300);
}
AHearthwardAnimalTraces::AHearthwardAnimalTraces()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=2;
    Prints=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("AnimalPrints"));RootComponent=Prints;
    Prints->SetCollisionEnabled(ECollisionEnabled::NoCollision);Prints->SetCanEverAffectNavigation(false);Prints->SetCastShadow(false);
    Interaction=CreateDefaultSubobject<UHearthwardTrailInteraction>(TEXT("TraceReading"));Interaction->SetupAttachment(RootComponent);Interaction->MaxDistance=0;
}
bool AHearthwardAnimalTraces::Sample(const FHearthwardNatureState& Nature,double Calendar,const FVector& Center,FHearthwardPersistentInteractionState& State)
{
    bool Changed=false;
    for(const auto& Slot:Nature.Slots)
    {
        const auto* Definition=HearthwardPersistence::Find(Slot.Id,EHearthwardPersistentKind::Trail);if(!Definition)continue;
        const auto* Animal=Nature.Animals.FindByPredicate([&](const auto& A){return A.Id==Slot.Current;});if(!Animal)continue;
        auto* Trail=State.Trails.FindByPredicate([&](const auto& T){return T.Id==Slot.Id;});
        const bool Eligible=Animal->Health>0 && !Animal->Domestic && FVector::Dist2D(Animal->Position,Center)<=50000;
        if(!Trail && !Eligible)continue;
        if(!Trail){FHearthwardTrailState New;New.Id=Slot.Id;Trail=&State.Trails.Add_GetRef(New);Changed=true;}
        if(Trail->Animal!=Slot.Current || Trail->Generation!=Slot.Generation)
        {Trail->Animal=Slot.Current;Trail->Generation=Slot.Generation;Trail->Species=Slot.Definition;Trail->Samples.Reset();Changed=true;}
        Changed|=Trail->Samples.RemoveAll([&](const auto& S){return Calendar-S.CalendarMinutes>90;})>0;
        if(!Eligible)continue;
        if(!Trail->Samples.IsEmpty() && (Calendar-Trail->Samples.Last().CalendarMinutes<2 || FVector::Dist2D(Trail->Samples.Last().Position,Animal->Position)<200))continue;
        FHearthwardTrailSample S;S.Position=Animal->Position;S.CalendarMinutes=Calendar;Trail->Samples.Add(S);Changed=true;
        if(Trail->Samples.Num()>Definition->MaxTrailSamples)Trail->Samples.RemoveAt(0,Trail->Samples.Num()-Definition->MaxTrailSamples);
    }
    return Changed;
}
FString AHearthwardAnimalTraces::DescribeAt(UWorld* World,const FVector& Position,double Radius)
{
    const auto& State=World->GetSubsystem<UHearthwardSaveSubsystem>()->InteractionState();
    const double Calendar=World->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes;
    const FHearthwardTrailState* Closest=nullptr;int32 Index=INDEX_NONE;double Distance=Radius;
    for(const auto& Trail:State.Trails)for(int32 I=0;I<Trail.Samples.Num();++I)
    {
        const auto& Sample=Trail.Samples[I];if(Calendar-Sample.CalendarMinutes>90)continue;
        const double D=FVector::Dist2D(Sample.Position,Position);if(D<Distance){Distance=D;Closest=&Trail;Index=I;}
    }
    if(!Closest)return TEXT("这里没有可辨认的新旧踪迹。观察附近地面，不会凭空出现猎物。");
    const double Age=Calendar-Closest->Samples[Index].CalendarMinutes;
    FString Direction=TEXT("方向尚不清楚");
    if(Index>0)
    {
        const FVector Delta=Closest->Samples[Index].Position-Closest->Samples[Index-1].Position;
        Direction=FMath::Abs(Delta.X)>FMath::Abs(Delta.Y)?(Delta.X>0?TEXT("大致向东"):TEXT("大致向西")):(Delta.Y>0?TEXT("大致向北"):TEXT("大致向南"));
    }
    return FString::Printf(TEXT("%s的%s踪迹，%s。这是过去留下的痕迹，动物可能已经离开。"),Closest->Species==TEXT("deer")?TEXT("鹿"):TEXT("野猪"),Age<=15?TEXT("新鲜"):TEXT("陈旧"),*Direction);
}
void AHearthwardAnimalTraces::Tick(float Delta)
{
    Super::Tick(Delta);auto* Clock=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>();if(Clock->Suspended())return;
    auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);if(!Player)return;
    auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();if(!Nature->State.Seed)return;
    auto Observed=Nature->State;
    for(auto& Animal:Observed.Animals)if(auto* Actual=Nature->Actor(Animal.Id))Animal.Position=Actual->GetActorLocation();
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();auto Next=Save->InteractionState();
    const double Calendar=Clock->GetSnapshot().ElapsedCalendarMinutes;
    if(Sample(Observed,Calendar,GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Position(TEXT("camp")),Next))
    {
        const FName Id=Next.Trails[0].Id;
        if(Save->BindInteractionTarget(Id,EHearthwardPersistentKind::Trail,this))
        {
            const auto Ticket=Save->BeginInteraction(Id);
            if(Ticket.Operation.IsValid() && !Save->CommitInteraction(Ticket,Next))Save->CancelInteraction();
        }
    }
    const auto& State=Save->InteractionState();const auto Epoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    if(!Prints->GetStaticMesh())
    {
        Prints->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        Prints->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Assets/NaturalWorld/Materials/M_Soil.M_Soil")));
    }
    if(RenderedRevision!=State.Revision || RenderedEpoch!=Epoch)
    {
        Prints->ClearInstances();RenderedRevision=State.Revision;RenderedEpoch=Epoch;
        for(const auto& Trail:State.Trails)for(int32 I=0;I<Trail.Samples.Num();++I)
        {
            const auto& S=Trail.Samples[I];if(Calendar-S.CalendarMinutes>90)continue;
            FVector Ground;if(!Nature->Ground(S.Position,Ground))continue;
            const FVector Heading=I>0?(S.Position-Trail.Samples[I-1].Position).GetSafeNormal2D():FVector::ForwardVector;
            const FVector Side=FVector::CrossProduct(Heading,FVector::UpVector);
            for(const int32 Foot:{-1,1})for(const int32 Toe:{-1,1})
            {
                const FVector At=Ground+Side*(Foot*7+Toe*1.8)+Heading*(Foot*9)+FVector(0,0,.35);
                Prints->AddInstance(FTransform(Heading.Rotation(),At,FVector(Trail.Species==TEXT("deer")?.055:.08,.023,.006)));
            }
        }
    }
#if !UE_BUILD_SHIPPING
    CSV_CUSTOM_STAT_GLOBAL(HearthwardTrailInstances,Prints->GetInstanceCount(),ECsvCustomStatOp::Set);
#endif
    double Distance=350;FVector Nearest;bool Found=false;
    for(const auto& Trail:State.Trails)for(const auto& S:Trail.Samples)
    {
        const double D=FVector::Dist(Player->GetActorLocation(),S.Position);if(Calendar-S.CalendarMinutes<=90 && D<Distance){Distance=D;Nearest=S.Position;Found=true;}
    }
    Interaction->MaxDistance=Found?250:0;if(Found)Interaction->SetWorldLocation(Nearest);
}
