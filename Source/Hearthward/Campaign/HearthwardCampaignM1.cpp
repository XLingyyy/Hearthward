#include "HearthwardCampaignSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
using namespace HearthwardData;

bool UHearthwardCampaignSubsystem::M1Complete(FName Id) const
{
    const auto R=HearthwardCampaign::Find(TEXT("locations"),Id);
    if(!R || !R->HasField(TEXT("m1_action")))return false;
    const FString Fact=Text(R,TEXT("fact"));
    if(!Fact.IsEmpty() && State.Facts.Contains(FName(*Fact)))return true;
    const FString Required=Text(R,TEXT("requires_fact"));
    return !Required.IsEmpty() && !State.Facts.Contains(FName(*Required));
}

bool UHearthwardCampaignSubsystem::UseM1(FName Id)
{
    const auto R=HearthwardCampaign::Find(TEXT("locations"),Id);
    if(!R || M1Complete(Id))return false;
    const FString Action=Text(R,TEXT("m1_action"));
    auto* G=Gameplay();
    if(Action==TEXT("bundle"))
    {
        auto* Bag=Player()->FindComponentByClass<UHearthwardInventoryComponent>();
        FHearthwardInventoryState After;
        if(!Bag || !After.Restore(Bag->Snapshot()) || After.Add(FName(*Text(R,TEXT("item"))),Number(R,TEXT("count")))!=EHearthwardInventoryResult::Success)
        {Feedback=TEXT("行装装不下这包物资；货包留在现场。存放物品或腾出负重后再来。");return false;}
        if(!Bag->RestoreInventory(After.Snapshot(),false))return false;
        Record(FName(*Text(R,TEXT("fact"))));
        Bag->OnInventoryChanged.Broadcast();
    }
    else if(Action==TEXT("token"))
    {
        Record(FName(*Text(R,TEXT("fact"))));
        const FName Quest(*Text(R,TEXT("quest")));
        const auto Q=HearthwardCampaign::Find(TEXT("quests"),Quest);
        G->Discovered.Add(FName(*Text(Q,TEXT("location"))));G->TrackedQuest=Quest;
    }
    else if(Action==TEXT("deliver"))Record(FName(*Text(R,TEXT("fact"))));
    else if(Action==TEXT("survey"))
    {
        G->Discovered.Add(FName(*Text(R,TEXT("reveal"))));
        Record(TEXT("ridge_survey"));
    }
    Feedback=Text(R,TEXT("m1_text"));
    if(Action==TEXT("tracks"))
    {
        const auto* Nature=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
        const FHearthwardAnimal* NearestAnimal=nullptr;double Distance=12000;
        for(const auto& A:Nature->State.Animals)if(!A.Domestic && A.Health>0)
        {
            const double D=FVector::Dist2D(A.Position,Position(Id));
            if(D<Distance){Distance=D;NearestAnimal=&A;}
        }
        if(NearestAnimal)
        {
            const FVector Delta=NearestAnimal->Position-Position(Id);
            const FString Direction=FMath::Abs(Delta.X)>FMath::Abs(Delta.Y)?(Delta.X>0?TEXT("东"):TEXT("西")):(Delta.Y>0?TEXT("北"):TEXT("南"));
            Feedback+=FString::Printf(TEXT(" %s方约 %.0f 米有%s活动；这是此刻的观察。可沿痕迹寻找，也可绕行。"),*Direction,Distance/100,
                *Text(HearthwardNature::Definition(TEXT("wildlife"),NearestAnimal->Definition),TEXT("name")));
        }
        else Feedback+=TEXT(" 附近暂时没有活跃动物；旧痕迹不会产生猎物，稍后再来或继续赶路。");
    }
    if(Action==TEXT("briefing"))
    {
        TArray<FString> Pages;
        for(const auto& Page:R->GetArrayField(TEXT("m1_pages")))Pages.Add(Page->AsString());
        if(State.Facts.Contains(TEXT("m1:materials_stored")))Pages.Add(TEXT("仓储旁已经整理出带回的物资。入库记录可在仓储查看。"));
        if(!GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Rescued.IsEmpty())Pages.Add(TEXT("新住客的铺位已收拾好。营地管理中可以安排他们的工作。"));
        if(State.Facts.Contains(TEXT("m1:tools_delivered")))Pages.Add(TEXT("工匠：取回的锉和木柄放在工作区了。下次修东西时，省得再去翻找。"));
        if(State.Facts.Contains(TEXT("public_meal")))Pages.Add(TEXT("火边已经摆好用餐器具。忙完这一趟，可以回来歇一会儿。"));
        const int32 Page=M1BriefingPage++%Pages.Num();
        Feedback=FString::Printf(TEXT("%d/%d · 再按 E 翻页\n%s"),Page+1,Pages.Num(),*Pages[Page]);
    }
    G->SetFeedback(Feedback);
    RefreshM1();return true;
}

void UHearthwardCampaignSubsystem::RefreshM1()
{
    if(State.Phase==TEXT("prologue") || !Player())return;
    const FVector PlayerAt=Player()->GetActorLocation();
    static const TCHAR* Chest=TEXT("/Game/Hearthward/Assets/TASK-028/furniture/wood_chest/SM_wood_chest.SM_wood_chest");
    static const TCHAR* Table=TEXT("/Game/Hearthward/Assets/TASK-028/furniture/wood_table/SM_wood_table.SM_wood_table");
    static const TCHAR* Chair=TEXT("/Game/Hearthward/Assets/TASK-028/furniture/wood_chair/SM_wood_chair.SM_wood_chair");
    static const TCHAR* Bed=TEXT("/Game/Hearthward/Assets/TASK-028/furniture/rope_wood_bed/SM_rope_wood_bed.SM_rope_wood_bed");
    static const TCHAR* Sign=TEXT("/Game/Hearthward/Assets/TASK-097/Route/SM_RouteSign.SM_RouteSign");
    static const TCHAR* Cairn=TEXT("/Game/Hearthward/Assets/TASK-097/Route/SM_RouteCairn.SM_RouteCairn");
    static const TCHAR* Lantern=TEXT("/Game/Hearthward/Assets/TASK-028/furniture/metal_lantern/SM_metal_lantern.SM_metal_lantern");
    static const TCHAR* Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
    const auto& Labor=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State;
    for(const auto& V:HearthwardCampaign::Rows(TEXT("locations")))
    {
        const auto R=V->AsObject();if(!R->HasField(TEXT("m1_action")))continue;
        const FName Id(*Text(R,TEXT("id")));const FString Action=Text(R,TEXT("m1_action"));
        const FName Tag(*(TEXT("M1Node:")+Id.ToString()));
        auto* Existing=Scenery.FindByPredicate([&](const auto& A){return A.IsValid() && A->ActorHasTag(Tag);});
        const bool Far=FVector::Dist2D(Position(Id),PlayerAt)>35000;
        if(Existing)
        {
            if(Far){Existing->Get()->Destroy();continue;}
            const bool Taken=(Action==TEXT("bundle") || Action==TEXT("token")) && M1Complete(Id);
            Existing->Get()->SetActorHiddenInGame(Taken);
            TInlineComponentArray<UStaticMeshComponent*> Parts(Existing->Get());
            for(auto* Part:Parts)
            {
                bool Show=true;
                if(Part->ComponentHasTag(TEXT("M1Stored")))Show=State.Facts.Contains(TEXT("m1:materials_stored"));
                if(Part->ComponentHasTag(TEXT("M1Resident")))Show=!Labor.Rescued.IsEmpty();
                if(Part->ComponentHasTag(TEXT("M1Tools")))Show=State.Facts.Contains(TEXT("m1:tools_delivered"));
                if(Part->ComponentHasTag(TEXT("M1Relic")))Show=State.Facts.Contains(TEXT("m1:river_delivered"));
                if(Part->ComponentHasTag(TEXT("M1Meal")))Show=State.Facts.Contains(TEXT("public_meal"));
                if(Part->ComponentHasTag(TEXT("M1Ore")))Show=State.Facts.Contains(TEXT("delivery:side_06"));
                if(Part->ComponentHasTag(TEXT("M1Herbs")))Show=State.Facts.Contains(TEXT("delivery:side_09"));
                Part->SetVisibility(Show);
            }
            continue;
        }
        if(Far)continue;
        FVector Floor;if(!Ground(Position(Id),Floor))continue;
        State.Positions.Add(Id,Floor+FVector(0,0,80));
        auto* A=GetWorld()->SpawnActor<AActor>();if(!A)continue;
        A->Tags.Add(Tag);Scenery.Add(A);
        auto* Root=NewObject<USceneComponent>(A);A->AddInstanceComponent(Root);A->SetRootComponent(Root);Root->RegisterComponent();A->SetActorLocation(Floor);
        auto Prop=[&](const TCHAR* Asset,FVector Offset,FVector Size,FRotator Rotation=FRotator::ZeroRotator,FName Event=NAME_None,bool Block=false)
        {
            auto* Mesh=LoadObject<UStaticMesh>(nullptr,Asset);if(!Mesh)return;
            auto* Part=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(Part);Part->SetupAttachment(Root);Part->SetStaticMesh(Mesh);
            const FBox Bounds=Mesh->GetBoundingBox();const FVector Extent=Bounds.GetSize();
            const FVector Scale(Size.X/FMath::Max(1.,Extent.X),Size.Y/FMath::Max(1.,Extent.Y),Size.Z/FMath::Max(1.,Extent.Z));
            Part->SetRelativeScale3D(Scale);Part->SetRelativeRotation(Rotation);
            const FBox Placed=Bounds.TransformBy(FTransform(Rotation,FVector::ZeroVector,Scale));
            FVector Base;
            if(Ground(Floor+Offset,Base))Offset.Z+=Base.Z-Floor.Z;
            Part->SetRelativeLocation(Offset-FVector(Placed.GetCenter().X,Placed.GetCenter().Y,Placed.Min.Z));
            Part->SetCollisionEnabled(Block?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
            if(Block)Part->SetCollisionProfileName(TEXT("BlockAll"));
            if(Asset==Cube || FString(Asset).Contains(TEXT("BasicShapes/Cylinder")))Part->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber.M_OldTimber")));
            if(!Event.IsNone()){Part->ComponentTags.Add(Event);Part->SetVisibility(false);}
            Part->RegisterComponent();
        };
        if(Action==TEXT("bundle") || Action==TEXT("token"))Prop(Chest,FVector::ZeroVector,FVector(110,70,65));
        else if(Action==TEXT("deliver"))Prop(Table,FVector(0,0,0),FVector(150,85,80));
        else Prop(Sign,FVector(0,0,0),FVector(120,45,220));
        if(Id==TEXT("poi_river_01"))
        {
            // Mooring remains frame the existing shallow ford; the walking surface is the landscape.
            for(int32 I=0;I<5;++I)
            {
                Prop(Cube,FVector(-220+I*110,240,0),FVector(24,24,160));
                Prop(Cube,FVector(-220+I*110,240,100),FVector(120,18,20),FRotator(0,0,6));
            }
            Prop(Cairn,FVector(-550,150,0),FVector(110,110,180));
            Prop(Cairn,FVector(500,-130,0),FVector(100,100,150));
        }
        if(Id==TEXT("poi_cart_01"))
        {
            Prop(Cube,FVector(0,360,50),FVector(310,180,24),FRotator(0,12,22));
            for(int32 I=0;I<4;++I)Prop(Cube,FVector(-130+I*85,360,80),FVector(20,180,130),FRotator(0,12,22));
            for(int32 X:{-100,100})for(int32 Y:{240,480})
                Prop(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),FVector(X,Y,0),FVector(120,120,18),FRotator(0,0,90));
            for(int32 I=0;I<5;++I)for(int32 X:{-95,95})
                Prop(Cube,FVector(X,-280-I*170,0),FVector(18,120,3),FRotator(0,5,0));
            Prop(Chest,FVector(180,340,0),FVector(140,85,70),FRotator(0,30,0));
        }
        if(Id==TEXT("poi_workshop_01"))
        {
            double Roof=340;
            for(int32 X:{-450,450})for(int32 Y:{300,1050})
            {FVector Base;if(Ground(Floor+FVector(X,Y,0),Base))Roof=FMath::Max(Roof,Base.Z-Floor.Z+340);}
            auto HeightAt=[&](double X,double Y){FVector Base;return Ground(Floor+FVector(X,Y,0),Base)?Base.Z-Floor.Z:0.;};
            for(int32 X:{-450,450})for(int32 Y:{300,1050})
                Prop(Cube,FVector(X,Y,0),FVector(35,35,Roof-HeightAt(X,Y)),FRotator::ZeroRotator,NAME_None,true);
            for(int32 Y:{300,1050})Prop(Cube,FVector(0,Y,Roof-HeightAt(0,Y)-30),FVector(950,40,35));
            for(int32 I=0;I<5;++I)
            {const int32 X=-360+I*180;Prop(Cube,FVector(X,700,Roof-HeightAt(X,700)),FVector(130,840,20),FRotator(0,0,-8));}
            // Individual fence sections follow the slope, preserving the west entrance and front retreat.
            for(int32 Y:{340,450})Prop(Cube,FVector(-450,Y,-15),FVector(30,120,185),FRotator::ZeroRotator,NAME_None,true);
            for(int32 Y=340;Y<1120;Y+=110)Prop(Cube,FVector(450,Y,-15),FVector(30,120,185),FRotator::ZeroRotator,NAME_None,true);
            for(int32 X=-400;X<=400;X+=100)Prop(Cube,FVector(X,1120,-15),FVector(110,30,185),FRotator::ZeroRotator,NAME_None,true);
            Prop(Table,FVector(-200,850,0),FVector(180,90,80));
            Prop(TEXT("/Game/Hearthward/Assets/TASK-098/Forge/SM_Forge_Practical.SM_Forge_Practical"),FVector(-230,550,0),FVector(210,150,130));
        }
        if(Id==TEXT("poi_herb_01"))for(int32 I=0;I<4;++I)Prop(Cairn,FVector(-320+I*200,300,0),FVector(110,80,70));
        if(Id==TEXT("poi_tracks_01"))for(int32 I=0;I<5;++I)Prop(Cube,FVector(220+I*110,(I%2)*45,0),FVector(24,13,2),FRotator(0,20,0));
        if(Id==TEXT("m1_camp_relic"))Prop(Sign,FVector(0,0,82),FVector(42,12,60),FRotator::ZeroRotator,TEXT("M1Relic"));
        if(Id==TEXT("m1_camp_tools"))
        {
            Prop(Chest,FVector(0,0,82),FVector(65,40,35),FRotator::ZeroRotator,TEXT("M1Tools"));
            Prop(Chest,FVector(180,0,0),FVector(100,70,60),FRotator::ZeroRotator,TEXT("M1Ore"));
            Prop(Chest,FVector(-180,0,0),FVector(90,60,55),FRotator::ZeroRotator,TEXT("M1Herbs"));
        }
        if(Id==TEXT("m1_camp_board"))
        {
            Prop(Chest,FVector(-300,60,0),FVector(130,80,80),FRotator::ZeroRotator,TEXT("M1Stored"));
            Prop(Bed,FVector(300,100,0),FVector(100,210,50),FRotator(0,90,0),TEXT("M1Resident"));
            FVector Meal=Position(TEXT("camp"))-Floor+FVector(-300,180,0);Meal.Z=0;
            Prop(Table,Meal,FVector(160,95,75),FRotator::ZeroRotator,TEXT("M1Meal"));
            for(int32 X:{-120,120})Prop(Chair,Meal+FVector(X,0,0),FVector(60,60,100),FRotator(0,X>0?90:-90,0),TEXT("M1Meal"));
        }
        if(Id==TEXT("poi_river_01") || Id==TEXT("poi_workshop_01") || Id==TEXT("m1_camp_board"))
        {
            Prop(Cube,FVector(90,0,0),FVector(25,25,100));
            Prop(Lantern,FVector(90,0,100),FVector(25,25,45));
            auto* Light=NewObject<UPointLightComponent>(A);A->AddInstanceComponent(Light);Light->SetupAttachment(Root);
            Light->SetRelativeLocation(FVector(90,0,140));Light->SetIntensity(2500);Light->SetAttenuationRadius(900);Light->SetLightColor(FLinearColor(1.f,.63f,.3f));Light->SetCastShadows(false);Light->RegisterComponent();
        }
        A->SetActorHiddenInGame((Action==TEXT("bundle") || Action==TEXT("token")) && M1Complete(Id));
    }
    Scenery.RemoveAll([](const auto& A){return !A.IsValid();});
}
