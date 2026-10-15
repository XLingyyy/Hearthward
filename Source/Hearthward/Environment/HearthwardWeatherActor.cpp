#include "HearthwardWeatherActor.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Experience/HearthwardPlayerSettings.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

AHearthwardWeatherActor::AHearthwardWeatherActor()
{
    PrimaryActorTick.bCanEverTick=true;
    Drops=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LightRain"));RootComponent=Drops;
    Drops->SetCollisionEnabled(ECollisionEnabled::NoCollision);Drops->SetCanEverAffectNavigation(false);Drops->SetCastShadow(false);
}
double AHearthwardWeatherActor::RainAt(double Calendar,int32 Seed)
{
    if(Seed==0)return 0;
    // World-calendar minutes: clear 190, rain 50, with six-minute fades. Sleep uses the same calendar.
    const double Phase=FMath::Fmod(Calendar+double(uint32(Seed)%40),240.);
    if(Phase<190)return 0;
    return FMath::Clamp(FMath::Min((Phase-190)/6.,(240-Phase)/6.),0.,1.);
}
double AHearthwardWeatherActor::Rain(UWorld* World)
{
    const auto& State=World->GetSubsystem<UHearthwardSaveSubsystem>()->InteractionState();
    const auto* S=State.Weather.FindByPredicate([](const auto& V){return V.Id==TEXT("world_light_rain");});
    return S?RainAt(World->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes,S->Seed):0;
}
bool AHearthwardWeatherActor::Sheltered(UWorld* World,const FVector& Position,const AActor* Subject)
{
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Round2Roof),false,Subject);
    return World->LineTraceTestByChannel(Position+FVector(0,0,20),Position+FVector(0,0,2500),ECC_Visibility,Query);
}
double AHearthwardWeatherActor::VisibilityFactor(UWorld* World,const FVector& Position,const AActor* Subject)
{
    const double Amount=Rain(World);return Amount<=0 || Sheltered(World,Position,Subject)?1.:1.-.1*Amount;
}
void AHearthwardWeatherActor::Tick(float Delta)
{
    Super::Tick(Delta);
    auto* Clock=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>();if(Clock->Suspended())return;
    auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);if(!Player)return;
    auto* Save=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>();
    const auto Time=Clock->GetSnapshot();
    const auto* Existing=Save->InteractionState().Weather.FindByPredicate([](const auto& V){return V.Id==TEXT("world_light_rain");});
    const int32 Seed=Existing?Existing->Seed:1+int32(GetTypeHash(Save->GetCampaignId())%2147483646u);
    const double Strength=RainAt(Time.ElapsedCalendarMinutes,Seed);
    const auto Phase=Strength>0?EHearthwardWeatherPhase::Rain:EHearthwardWeatherPhase::Clear;
    if(!Existing || Existing->Phase!=Phase)
    {
        if(Save->BindInteractionTarget(TEXT("world_light_rain"),EHearthwardPersistentKind::Weather,this))
        {
            const auto Ticket=Save->BeginInteraction(TEXT("world_light_rain"));
            if(Ticket.Operation.IsValid())
            {
                auto Next=Save->InteractionState();auto* S=Next.Weather.FindByPredicate([](const auto& V){return V.Id==TEXT("world_light_rain");});
                if(!S){FHearthwardWeatherState V;V.Id=TEXT("world_light_rain");S=&Next.Weather.Add_GetRef(V);}
                S->Seed=Seed;S->Phase=Phase;S->PhaseStartedAt=Time.ElapsedCalendarMinutes;
                if(!Save->CommitInteraction(Ticket,Next))Save->CancelInteraction();
            }
        }
    }
    const auto* Settings=GetWorld()->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
    const double Visible=Strength*Settings->Comfort.RainVisual/100.;Drops->SetVisibility(Visible>0);
    if(Visible<=0)return;
    if(DropPositions.IsEmpty())
    {
        Drops->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        FRandomStream Random(Seed);
        for(int32 I=0;I<96;++I)
        {DropPositions.Add(FVector(Random.FRandRange(-900,900),Random.FRandRange(-900,900),Random.FRandRange(0,800)));Covered.Add(false);Drops->AddInstance(FTransform());}
    }
    const FVector Origin=Player->GetActorLocation();SetActorLocation(Origin);
    RoofCheck-=Delta;
    if(RoofCheck<=0)
    {
        RoofCheck=.4f;
        for(int32 I=0;I<DropPositions.Num();++I)Covered[I]=Sheltered(GetWorld(),Origin+FVector(DropPositions[I].X,DropPositions[I].Y,0),Player);
    }
    for(int32 I=0;I<DropPositions.Num();++I)
    {
        const auto P=DropPositions[I];const double Height=FMath::Fmod(P.Z+Time.ActivePlaySeconds*900,800.);
        const FVector At(P.X,P.Y,700-Height);
        const FVector Scale=Covered[I] || I>=FMath::CeilToInt(96*Visible)?FVector::ZeroVector:FVector(.003,.003,.17);
        Drops->UpdateInstanceTransform(I,FTransform(FRotator(0,0,5),At,Scale),false,I==DropPositions.Num()-1);
    }
}
