#include "HearthwardNatureSubsystem.h"
#include "HearthwardNatureActor.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
using namespace HearthwardData;
APawn* UHearthwardNatureSubsystem::Player() const{return UGameplayStatics::GetPlayerPawn(GetWorld(),0);}
bool UHearthwardNatureSubsystem::WorkingOn(FGuid Id) const
{
    if(!PendingAction.IsNone() && PendingId==Id)return true;
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        if(It->CapturingAnimal(Id))return true;
    return false;
}
UHearthwardInventoryComponent* UHearthwardNatureSubsystem::Bag() const
{auto* Actor=ActionActor.IsValid()?ActionActor.Get():Player();return Actor?Actor->FindComponentByClass<UHearthwardInventoryComponent>():nullptr;}
UHearthwardGameplayComponent* UHearthwardNatureSubsystem::Gameplay() const{return Player()?Player()->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;}
bool UHearthwardNatureSubsystem::DoesSupportWorldType(EWorldType::Type Type) const{return Type==EWorldType::Game || Type==EWorldType::PIE;}
bool UHearthwardNatureSubsystem::IsTickable() const{return IsInitialized() && GetWorld()->HasBegunPlay() && !IsTemplate();}
TStatId UHearthwardNatureSubsystem::GetStatId() const{RETURN_QUICK_DECLARE_CYCLE_STAT(UHearthwardNatureSubsystem,STATGROUP_Tickables);}
bool UHearthwardNatureSubsystem::Safe(FGuid Epoch) const
{
    const auto* G=Gameplay();const auto* P=Player();if(!P || !G || !G->Enabled || G->Health<=0 || G->InCombat() || Epoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())return false;
    if(ActionActor.IsValid() && ActionActor.Get()!=P)
    {
        const auto* S=ActionActor->FindComponentByClass<UHearthwardSurvivalComponent>();
        return S && S->Alive() && S->SafeToSave() && !ActionActor->IsActorBeingDestroyed();
    }
    const auto* S=P->FindComponentByClass<UHearthwardSurvivalComponent>();const auto* C=P->FindComponentByClass<UHearthwardCombatComponent>();
    const auto* B=P->FindComponentByClass<UHearthwardBuildingComponent>();
    return (!S || S->SafeToSave()) && (!C || !C->Busy()) && (!B || (!B->IsBuilding() && !B->IsPlacing()));
}
FVector UHearthwardNatureSubsystem::Position(FGuid Id) const
{
    for(const auto& P:State.Points)if(P.Id==Id)return P.Position;
    for(const auto& C:State.Crops)if(C.Id==Id)return C.Position;
    for(const auto& P:State.Pens)if(P.Id==Id)return P.Position;
    for(const auto& A:State.Animals)if(A.Id==Id)return A.Position;
    return FVector(MAX_flt);
}
bool UHearthwardNatureSubsystem::Near(FGuid Id,double Distance) const
{
    if(State.Points.ContainsByPredicate([&](const auto& P){return P.Id==Id && P.Kind==TEXT("resource");}))Distance=FMath::Min(Distance,240.);
    const auto* Actor=ActionActor.IsValid()?ActionActor.Get():Player();
    return Actor && FVector::Dist(Actor->GetActorLocation(),Position(Id))<=Distance;
}
AHearthwardNatureActor* UHearthwardNatureSubsystem::Actor(FGuid Id) const{const auto* A=Actors.Find(Id);return A?A->Get():nullptr;}
bool UHearthwardNatureSubsystem::Ground(FVector Desired,FVector& Result,bool Flat) const
{
    TArray<FHitResult> Hits;FCollisionQueryParams Q(SCENE_QUERY_STAT(NatureGround),false,Player());
    GetWorld()->LineTraceMultiByObjectType(Hits,Desired+FVector(0,0,12000),Desired-FVector(0,0,30000),FCollisionObjectQueryParams(ECC_WorldStatic),Q);
    for(const auto& Hit:Hits)
    {
        const auto* A=Hit.GetActor();if(!A)continue;
        // Plant and resource roots belong to terrain, never a roof, foliage canopy or prop.
        const bool Terrain=A->GetClass()->GetName().Contains(TEXT("Landscape")) || A->ActorHasTag(TEXT("Hearthward.NatureGround"));
        if(!Terrain)continue;
        if(Hit.ImpactNormal.Z<(Flat?.966:.65))return false;
        Result=Hit.ImpactPoint;return true;
    }
    return false;
}
bool UHearthwardNatureSubsystem::ClearPlot(FVector Point,double Radius,FGuid Ignore) const
{
    if(GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(Point).IsNone())return false;
    for(const auto& C:State.Crops)if(FVector::Dist2D(Point,C.Position)<Radius+140)return false;
    for(const auto& P:State.Pens)if(P.Id!=Ignore && FVector::Dist2D(Point,P.Position)<Radius+200)return false;
    for(auto* Building:Player()->FindComponentByClass<UHearthwardBuildingComponent>()->GetBuildings())
    {FVector O,E;Building->GetActorBounds(true,O,E);if(FMath::Abs(Point.X-O.X)<E.X+Radius && FMath::Abs(Point.Y-O.Y)<E.Y+Radius)return false;}
    for(TActorIterator<AActor> It(GetWorld());It;++It)
    {
        const FString Name=It->GetName();
        if(!Name.Contains(TEXT("Road")) && !Name.Contains(TEXT("Water")) && !It->ActorHasTag(TEXT("Hearthward.Road")) && !It->ActorHasTag(TEXT("water")))continue;
        FVector O,E;It->GetActorBounds(false,O,E);if(FMath::Abs(Point.X-O.X)<E.X+Radius && FMath::Abs(Point.Y-O.Y)<E.Y+Radius && FMath::Abs(Point.Z-O.Z)<E.Z+50)return false;
    }
    return true;
}
void UHearthwardNatureSubsystem::EnsureWorld()
{
    if(!Gameplay() || !Gameplay()->Enabled)return;
    auto* Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();if(Camp->State.Camps.IsEmpty())return;
    if(!State.Seed)State.Seed=int32(GetTypeHash(FGuid::NewGuid()))|1;
    State.Calendar=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes;
    bool Added=false;
    for(const auto& Site:Camp->State.Camps)
    {
        if(State.Camps.Contains(Site.Id))continue;
        FVector First;if(!Ground(Site.Position,First))continue;bool Complete=true;
        auto AddPoint=[&](FName Key,FName Kind,FName Def,FVector Desired,int32 Stock=0)
        {
            if(State.Points.ContainsByPredicate([&](const auto& P){return P.Key==Key;}))return;
            FVector At;bool Located=Ground(Desired,At);
            if(!Located && Kind==TEXT("resource"))
            {
                const FVector Offset=Desired-Site.Position;
                for(int32 Attempt=1;Attempt<=24 && !Located;++Attempt)
                    Located=Ground(Site.Position+Offset.RotateAngleAxis(Attempt*15.f,FVector::UpVector),At);
            }
            if(!Located){Complete=false;return;}
            FHearthwardNaturePoint P;P.Id=FGuid::NewGuid();P.Key=Key;P.Kind=Kind;P.Definition=Def;P.Position=At;P.Remaining=Stock;State.Points.Add(P);Added=true;
        };
        int32 Offset=0;
        for(const auto& V:HearthwardNature::Rows(TEXT("resources")))
        {
            const auto D=V->AsObject();const FName Def(Text(D,TEXT("id")));
            const int32 Count=Def==TEXT("food_patch")?4:Def==TEXT("fallen_branches")?6:Def==TEXT("loose_stones")?4:Def==TEXT("tree")?12:Def==TEXT("stone_outcrop")?6:Def==TEXT("herb_patch")?4:Def==TEXT("ore_vein")?4:2;
            for(int32 I=0;I<Count;++I)
            {
                const double Angle=(Offset++*2.399963);const double Radius=Def==TEXT("rich_ore_vein")?50000+I*14000:Def==TEXT("ore_vein")?16000+I*4000:Def.ToString().StartsWith(TEXT("wild_seed_"))?6500+I*4500:2200+(Offset%5)*500;
                const FVector Desired=Site.Position+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
                AddPoint(FName(Site.Id.ToString()+TEXT("_048_")+Def.ToString()+FString::FromInt(I)),TEXT("resource"),Def,Desired);
            }
        }
        {
            int32 Index=0;
            for(const auto& V:HearthwardNature::Rows(TEXT("wildlife")))
            {
                const auto D=V->AsObject();const FName Def(Text(D,TEXT("id")));const double Angle=(Index++*.73);
                const double Distance=(Def==TEXT("wolf") || Def==TEXT("black_bear") || Def==TEXT("boar"))?38000:12000+Index*1500;
                for(int32 I=0;I<Number(D,TEXT("group_size"));++I)
                {
                    const FName SlotId(TEXT("nature_")+Def.ToString()+FString::FromInt(I));
                    if(State.Slots.ContainsByPredicate([&](const auto& S){return S.Id==SlotId;}))continue;
                    FVector At;if(!Ground(Site.Position+FVector(FMath::Cos(Angle)*Distance+I*500,FMath::Sin(Angle)*Distance,0),At)){Complete=false;continue;}
                    FHearthwardWildSlot S;S.Id=SlotId;S.Definition=Def;S.Position=At;S.Current=FGuid::NewGuid();
                    FHearthwardAnimal A;A.Id=S.Current;A.Slot=S.Id;A.Definition=Def;A.Position=A.Destination=At;A.Health=Number(D,TEXT("health"));State.Animals.Add(A);State.Slots.Add(S);Added=true;
                }
            }
            Index=0;
            for(const auto& V:HearthwardNature::Rows(TEXT("domestic")))
            {
                const auto D=V->AsObject();const FName Def(Text(D,TEXT("id")));
                for(int32 I=0;I<4;++I)
                {
                    const FName Starter(TEXT("starter_")+Def.ToString()+FString::FromInt(I));
                    if(State.Animals.ContainsByPredicate([&](const auto& A){return A.Slot==Starter;}))continue;
                    FVector At;const double Angle=2.8+Index*.9;
                    if(!Ground(Site.Position+FVector(FMath::Cos(Angle)*(11000+(I/2)*8000)+I*200,FMath::Sin(Angle)*(11000+(I/2)*8000),0),At)){Complete=false;continue;}
                    FHearthwardAnimal A;A.Id=FGuid::NewGuid();A.Domestic=true;A.Slot=Starter;A.Definition=Def;A.Position=A.Destination=At;A.Health=Number(D,TEXT("health"));State.Animals.Add(A);Added=true;
                }
                ++Index;
            }
        }
        {
            const bool Natural=UGameplayStatics::GetCurrentLevelName(GetWorld(),true)==TEXT("L_HearthwardWilds");
            for(int32 I=0;I<4;++I)
            {
                const FName FishKey(TEXT("nature_fishing_")+FString::FromInt(I));
                if(State.Points.ContainsByPredicate([&](const auto& P){return P.Key==FishKey;}))continue;
                FVector At=Site.Position+FVector(5000+I*4500,6500,0);
                if(Natural)
                {
                    const double A=FMath::DegreesToRadians(-120.+I*18.);bool Found=false;
                    for(double R=1.;R<=1.35;R+=.015)
                    {
                        FVector Candidate(-93000+24000*R*FMath::Cos(A),-36000+33000*R*FMath::Sin(A),15700),GroundAt;
                        if(Ground(Candidate,GroundAt,true) && GroundAt.Z>=15710 && GroundAt.Z<16200){At=GroundAt;Found=true;break;}
                    }
                    if(!Found){Complete=false;continue;}
                }
                AddPoint(FishKey,TEXT("fish"),TEXT("carp"),At,24);
            }
        }
        if(Complete)State.Camps.Add(Site.Id);
    }
    for(const auto& P:State.Points)if(P.Kind==TEXT("resource"))
    {
        const auto D=HearthwardNature::Definition(TEXT("resources"),P.Definition);const int32 N=Number(D,TEXT("capacity"));
        Camp->RegisterSource(P.Key.ToString(),FName(Text(D,TEXT("item"))),N,N,P.Position,Number(D,TEXT("refresh_days"))*1440);
    }
    if(Added)RebuildActors();
}
void UHearthwardNatureSubsystem::RebuildActors()
{
    TSet<FGuid> Existing;
    auto Ensure=[&](FGuid Id,FName Kind,FName Def,FVector At)
    {
        Existing.Add(Id);auto* A=Actor(Id);
        if(!A){A=GetWorld()->SpawnActor<AHearthwardNatureActor>(At,FRotator::ZeroRotator);if(!A)return;A->Configure(Id,Kind,Def);Actors.Add(Id,A);}
        A->Refresh();
    };
    for(const auto& P:State.Points)if(P.Kind!=TEXT("treasure") || (State.Maps.Contains(P.Definition) && !State.Opened.Contains(P.Definition)))Ensure(P.Id,P.Kind,P.Definition,P.Position);
    for(const auto& C:State.Crops)Ensure(C.Id,TEXT("crop"),C.Definition,C.Position);
    for(const auto& P:State.Pens)Ensure(P.Id,TEXT("pen"),P.Definition,P.Position);
    for(const auto& A:State.Animals)if(A.Health>0 || !A.Loot.IsEmpty())Ensure(A.Id,TEXT("animal"),A.Definition,A.Position);
    for(auto It=Actors.CreateIterator();It;++It)if(!Existing.Contains(It.Key())){if(It.Value().IsValid())It.Value()->Destroy();It.RemoveCurrent();}
}
void UHearthwardNatureSubsystem::SyncAnimals()
{
    for(auto& A:State.Animals)if(auto* World=Actor(A.Id))A.Position=World->GetActorLocation();
}
void UHearthwardNatureSubsystem::Advance(double Minutes)
{if(Settling || !State.Seed)return;TGuardValue<bool> Guard(Settling,true);State.Advance(Minutes);}
void UHearthwardNatureSubsystem::Tick(float Delta)
{
    if(GetWorld()->IsPaused())return;
    RefreshIn-=Delta;
    if(RefreshIn<=0){RefreshIn=.5;EnsureWorld();RebuildActors();}
    const auto* Actor=ActionActor.IsValid()?ActionActor.Get():Player();
    if(!PendingAction.IsNone() && (!Actor || !Safe(ActionEpoch) || !Actor->GetVelocity().IsNearlyZero() || FVector::Dist(Actor->GetActorLocation(),ActionPosition)>350))Cancel();
    if(IsFishing())TickFishing(Delta);
    SyncAnimals();
    for(auto& S:State.Slots)if(S.Due>=0 && S.Due<=State.Calendar && Player())
    {
        bool Clear=FVector::Dist2D(Player()->GetActorLocation(),S.Position)>8000;
        for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->FindComponentByClass<UHearthwardSurvivalComponent>() && FVector::Dist2D(It->GetActorLocation(),S.Position)<=8000)Clear=false;
        for(auto* Building:Player()->FindComponentByClass<UHearthwardBuildingComponent>()->GetBuildings())if(FVector::Dist2D(Building->GetActorLocation(),S.Position)<500)Clear=false;
        for(const auto& Pen:State.Pens)if(FVector::Dist2D(Pen.Position,S.Position)<500)Clear=false;
        if(!Clear)continue;
        FHearthwardAnimal A;A.Id=FGuid::NewGuid();A.Slot=S.Id;A.Definition=S.Definition;A.Position=A.Destination=S.Position;A.Health=Number(HearthwardNature::Definition(TEXT("wildlife"),A.Definition),TEXT("health"));
        S.Current=A.Id;S.Generation++;S.Due=-1;State.Animals.Add(A);
    }
}
void UHearthwardNatureSubsystem::Restore(const FString& Json,double Calendar)
{
    Cancel();for(const auto& P:Actors)if(P.Value.IsValid())P.Value->Destroy();Actors.Reset();
    FHearthwardNatureState New;if(FHearthwardNatureState::Parse(Json,New)){State=MoveTemp(New);State.Calendar=Calendar;}
    RefreshIn=0;RebuildActors();
}
