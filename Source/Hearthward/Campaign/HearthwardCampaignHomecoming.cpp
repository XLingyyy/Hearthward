#include "HearthwardCampaignSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
using namespace HearthwardData;

bool UHearthwardCampaignSubsystem::UseHomecoming(FName Id)
{
    auto* Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    if(!M2HomeReady() || Camp->State.CampAt(Player()->GetActorLocation())!=TEXT("hometown") || !Camp->CanManage(Store->GetTimelineEpoch()))
    {Feedback=TEXT("先夺回故乡，并在安全的第二营地内处理生活区。 ");return false;}
    const auto R=HearthwardCampaign::Find(TEXT("locations"),Id);
    const FString Action=Text(R,TEXT("exploration_action"));
    if(Action==TEXT("home_repair"))
    {
        const auto Cost=HearthwardCamp::Counts(R,TEXT("materials"));
        if(!Store->Adjust(Cost,{}))
        {Feedback=TEXT("修好灯架需要共享仓储中的木材 2、绳索 2；已预留的材料不能使用。材料不足，尚未扣除。");return false;}
    }
    if(Action==TEXT("home_meal"))
    {
        if(!Camp->EatMeal(false,Store->GetTimelineEpoch())){Feedback=Camp->Feedback;return false;}
        Feedback=TEXT("族人：桌子还在，人也回来了。慢慢吃，今晚不用再赶路。\n本次用餐消耗口粮 5 点，按 J 领取支线奖励。");
    }
    else
    {
        Record(FName(*Text(R,TEXT("fact"))));Feedback=Text(R,TEXT("exploration_text"));
        const FName Quest(*Text(R,TEXT("quest")));
        const auto Q=HearthwardCampaign::Find(TEXT("quests"),Quest);
        Gameplay()->Discovered.Add(FName(*Text(Q,TEXT("location"))));Gameplay()->TrackedQuest=Quest;
    }
    Gameplay()->SetFeedback(Feedback);RefreshCampLife();return true;
}

void UHearthwardCampaignSubsystem::RefreshCampLife()
{
    if(State.Phase==TEXT("prologue") || !Player())return;
    const auto& Camp=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    const int32 Tier=Camp.Tier<=3?1:Camp.Tier<=6?2:3;
    const bool Home=M2HomeReady();
    const bool Lamp=Home && State.Facts.Contains(TEXT("m3:lamp_repaired"));
    const bool TableReady=Home && State.Facts.Contains(TEXT("m3:table_ready"));
    const bool Meal=Home && State.Facts.Contains(TEXT("m3:home_meal"));
    const double Hour=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().MinuteOfDay/60.;
    const bool Night=Hour>=18 || Hour<6;
    static const TCHAR* Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
    static const TCHAR* Table=TEXT("/Game/Hearthward/Assets/TASK-028/furniture/wood_table/SM_wood_table.SM_wood_table");
    static const TCHAR* Chair=TEXT("/Game/Hearthward/Assets/TASK-028/furniture/wood_chair/SM_wood_chair.SM_wood_chair");
    static const TCHAR* Chest=TEXT("/Game/Hearthward/Assets/TASK-028/furniture/wood_chest/SM_wood_chest.SM_wood_chest");
    static const TCHAR* Lantern=TEXT("/Game/Hearthward/Assets/TASK-028/furniture/metal_lantern/SM_metal_lantern.SM_metal_lantern");
    auto AtGround=[&](FVector P){FVector Floor;return Ground(P,Floor)?Floor:P;};
    auto Scene=[&](FName Tag,FVector Desired,bool Available)->AActor*
    {
        auto* Found=Scenery.FindByPredicate([&](const auto& A){return A.IsValid() && A->ActorHasTag(Tag);});
        if(!Available || FVector::Dist2D(Desired,Player()->GetActorLocation())>35000)
        {if(Found)Found->Get()->Destroy();return nullptr;}
        if(Found)return Found->Get();
        FVector Floor;if(!Ground(Desired,Floor))return nullptr;
        auto* A=GetWorld()->SpawnActor<AActor>();if(!A)return nullptr;
        auto* Root=NewObject<USceneComponent>(A);A->AddInstanceComponent(Root);A->SetRootComponent(Root);Root->RegisterComponent();A->SetActorLocation(Floor);
        A->Tags.Add(Tag);Scenery.Add(A);return A;
    };
    // Positions are world-space bases. Stacked pieces share the same measured support.
    auto Prop=[&](AActor* A,const TCHAR* Asset,FVector Base,FVector Size,FName Tag=NAME_None,float Yaw=0.f)
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,Asset);if(!Mesh)return;
        auto* Part=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(Part);Part->SetupAttachment(A->GetRootComponent());Part->SetStaticMesh(Mesh);
        const FBox B=Mesh->GetBoundingBox();const FVector S=Size/B.GetSize();const FRotator Rotation(0,Yaw,0);
        const FBox Placed=B.TransformBy(FTransform(Rotation,FVector::ZeroVector,S));
        Part->SetRelativeScale3D(S);Part->SetRelativeRotation(Rotation);
        Part->SetRelativeLocation(Base-A->GetActorLocation()-FVector(Placed.GetCenter().X,Placed.GetCenter().Y,Placed.Min.Z));
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCanEverAffectNavigation(false);
        if(Asset==Cube || FString(Asset).Contains(TEXT("BasicShapes/Cylinder")))Part->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber.M_OldTimber")));
        if(!Tag.IsNone())Part->ComponentTags.Add(Tag);Part->RegisterComponent();
    };
    auto Light=[&](AActor* A,FVector P,FName Tag)
    {
        auto* L=NewObject<UPointLightComponent>(A);A->AddInstanceComponent(L);L->SetupAttachment(A->GetRootComponent());
        L->SetRelativeLocation(P-A->GetActorLocation());L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(Tag==TEXT("HomeMeal")?60:1800);L->SetAttenuationRadius(850);L->SetLightColor(FLinearColor(1.f,.67f,.38f));L->SetCastShadows(false);L->ComponentTags.Add(Tag);L->RegisterComponent();
    };
    for(const auto& Site:Camp.Camps)
    {
        const bool IsHome=Site.Id==TEXT("hometown");
        const FName Tag(*(TEXT("CampLife:")+Site.Id.ToString()));
        auto* A=Scene(Tag,Site.Position+FVector(1200,-1600,0),!IsHome || Home);if(!A)continue;
        if(!A->ActorHasTag(TEXT("CampLifeBuilt")))
        {
            const FVector Origin=A->GetActorLocation();
            const FVector TableBase=AtGround(Origin+FVector(-280,0,0));
            Prop(A,Table,TableBase,FVector(170,90,78));
            Prop(A,Chair,AtGround(Origin+FVector(-280,150,0)),FVector(60,60,95));
            // A simple frame surrounds the living corner while leaving its front open.
            const FVector Left=AtGround(Origin+FVector(-500,350,0)),Right=AtGround(Origin+FVector(450,350,0));
            const double Roof=FMath::Max(Left.Z,Right.Z)+230;
            Prop(A,Cube,Left,FVector(22,22,Roof-Left.Z));Prop(A,Cube,Right,FVector(22,22,Roof-Right.Z));
            Prop(A,Cube,FVector(Origin.X-25,Origin.Y+350,Roof-20),FVector(990,24,24));
            for(int32 X:{-500,450})
            {const FVector Base=AtGround(Origin+FVector(X,-320,0));Prop(A,Cube,Base,FVector(22,22,Roof-Base.Z));}
            Prop(A,TEXT("/Game/Hearthward/Assets/TASK-028/house/thatched_roof_slope/SM_thatched_roof_slope.SM_thatched_roof_slope"),FVector(Origin.X-25,Origin.Y+15,Roof),FVector(1050,760,80));
            Prop(A,TEXT("/Game/Hearthward/Assets/TASK-028/furniture/rope_wood_bed/SM_rope_wood_bed.SM_rope_wood_bed"),AtGround(Origin+FVector(-200,-200,0)),FVector(210,90,45),TEXT("CampBed"));
            Prop(A,Chest,AtGround(Origin+FVector(270,100,0)),FVector(130,80,70),TEXT("CampTier2"));
            Prop(A,Table,AtGround(Origin+FVector(300,400,0)),FVector(160,80,78),TEXT("CampWork"));
            Prop(A,Chest,AtGround(Origin+FVector(350,300,0)),FVector(75,55,45),TEXT("CampWork"));
            for(int32 X:{-620,580})
            {
                const FVector Base=AtGround(Origin+FVector(X,-130,0));
                Prop(A,Cube,Base,FVector(18,18,175),TEXT("CampTier3"));
                Prop(A,Lantern,Base+FVector(0,0,175),FVector(30,30,45),TEXT("CampTier3"));
                Light(A,Base+FVector(0,0,200),TEXT("CampTierLight"));
            }
            A->Tags.Add(TEXT("CampLifeBuilt"));
        }
        const bool Work=Camp.Facilities.ContainsByPredicate([&](const auto& F){return F.Camp==Site.Id && F.Kind==TEXT("workbench") && !F.Paused;});
        const bool Bed=Camp.Facilities.ContainsByPredicate([&](const auto& F){return F.Camp==Site.Id && F.Kind==TEXT("bed");});
        TInlineComponentArray<UStaticMeshComponent*> Parts(A);
        for(auto* P:Parts)
        {
            if(P->ComponentHasTag(TEXT("CampTier2")))P->SetVisibility(Tier>=2);
            if(P->ComponentHasTag(TEXT("CampBed")))P->SetVisibility(Bed);
            if(P->ComponentHasTag(TEXT("CampWork")))P->SetVisibility(Tier>=2 && Work);
            if(P->ComponentHasTag(TEXT("CampTier3")))P->SetVisibility(Tier>=3 && (!IsHome || Lamp));
        }
        TInlineComponentArray<UPointLightComponent*> Lights(A);for(auto* L:Lights)L->SetVisibility(Tier>=3 && Night && (!IsHome || Lamp));
    }
    for(const FName Id:{FName(TEXT("m3_home_lamp")),FName(TEXT("m3_home_table"))})
    {
        auto* A=Scene(FName(*(TEXT("HomeLife:")+Id.ToString())),Position(Id),Home);if(!A)continue;
        const FVector Origin=A->GetActorLocation();
        State.Positions.Add(Id,Origin+FVector(0,0,80));
        const bool IsLamp=Id==TEXT("m3_home_lamp");
        State.Positions.Add(IsLamp?FName(TEXT("m3_home_lamp_repair")):FName(TEXT("m3_home_meal")),Origin+FVector(0,0,80));
        if(!A->ActorHasTag(TEXT("HomeLifeBuilt")))
        {
            const FVector Base=AtGround(Origin+FVector(0,200,0));
            if(IsLamp)
            {
                Prop(A,Cube,Base,FVector(25,25,190));
                Prop(A,Lantern,Base+FVector(0,0,190),FVector(38,38,50));
                Prop(A,Cube,Base+FVector(0,0,170),FVector(95,22,18),TEXT("HomeLamp"));
                Light(A,Base+FVector(0,0,215),TEXT("HomeLamp"));
            }
            else
            {
                Prop(A,Table,Base,FVector(220,100,78));
                for(int32 X:{-155,155})Prop(A,Chair,AtGround(Origin+FVector(X,200,0)),FVector(60,60,95),NAME_None,X>0?90:-90);
                for(int32 X:{-60,60})Prop(A,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),Base+FVector(X,0,78),FVector(28,28,8),TEXT("HomeTable"));
                Prop(A,Lantern,Base+FVector(0,20,78),FVector(25,25,35),TEXT("HomeMeal"));
                Light(A,Base+FVector(0,20,103),TEXT("HomeMeal"));
            }
            A->Tags.Add(TEXT("HomeLifeBuilt"));
        }
        TInlineComponentArray<UStaticMeshComponent*> Parts(A);
        for(auto* P:Parts)
        {
            if(P->ComponentHasTag(TEXT("HomeLamp")))P->SetVisibility(Lamp);
            if(P->ComponentHasTag(TEXT("HomeTable")))P->SetVisibility(TableReady);
            if(P->ComponentHasTag(TEXT("HomeMeal")))P->SetVisibility(Meal);
        }
        TInlineComponentArray<UPointLightComponent*> Lights(A);for(auto* L:Lights)L->SetVisibility(Night && (IsLamp?Lamp:Meal));
    }
    Scenery.RemoveAll([](const auto& A){return !A.IsValid();});
}
