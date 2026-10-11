#include "HearthwardCampaignSubsystem.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
using namespace HearthwardData;

bool UHearthwardCampaignSubsystem::ExplorationComplete(FName Id) const
{
    const auto R=HearthwardCampaign::Find(TEXT("locations"),Id);
    if(!R || !R->HasField(TEXT("exploration_action")))return false;
    if(Text(R,TEXT("exploration_action"))==TEXT("camp_event"))return !M2EventAvailable(FName(*Text(R,TEXT("event"))));
    const FString Fact=Text(R,TEXT("fact"));
    if(!Fact.IsEmpty() && State.Facts.Contains(FName(*Fact)))return true;
    const FString Required=Text(R,TEXT("requires_fact"));
    return !Required.IsEmpty() && !State.Facts.Contains(FName(*Required));
}

bool UHearthwardCampaignSubsystem::UseExploration(FName Id)
{
    const auto R=HearthwardCampaign::Find(TEXT("locations"),Id);
    if(!R || ExplorationComplete(Id))return false;
    const FString Action=Text(R,TEXT("exploration_action"));
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
    else if(Action==TEXT("deliver") || Action==TEXT("observe"))Record(FName(*Text(R,TEXT("fact"))));
    else if(Action==TEXT("survey"))
    {
        G->Discovered.Add(FName(*Text(R,TEXT("reveal"))));
        Record(TEXT("ridge_survey"));
    }
    Feedback=Text(R,TEXT("exploration_text"));
    if(Action==TEXT("local_survey"))G->Discovered.Add(FName(*Text(R,TEXT("reveal"))));
    if(Action==TEXT("camp_event"))Feedback=UseM2Event(FName(*Text(R,TEXT("event"))));
    if(Action==TEXT("patrol"))Feedback=M2PatrolObservation(Id);
    if(Action==TEXT("home_steps") && M2HomeReady())
    {
        Feedback=State.Facts.Contains(TEXT("m2:home_evening"))?TEXT("石阶旁的灯还在。仓储与工作区已经开放，今晚可以在故乡休息。")
            :TEXT("族人：这回不用走到天黑再离开了。灯留在石阶边，回来的人能看见。第二营地可以使用，记得保存这次归乡。");
        Record(TEXT("m2:home_evening"));
    }
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
        for(const auto& Page:R->GetArrayField(TEXT("exploration_pages")))Pages.Add(Page->AsString());
        if(State.Facts.Contains(TEXT("m1:materials_stored")))Pages.Add(TEXT("仓储旁已经整理出带回的物资。入库记录可在仓储查看。"));
        if(!GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Rescued.IsEmpty())Pages.Add(TEXT("新住客的铺位已收拾好。营地管理中可以安排他们的工作。"));
        if(State.Facts.Contains(TEXT("m1:tools_delivered")))Pages.Add(TEXT("工匠：取回的锉和木柄放在工作区了。下次修东西时，省得再去翻找。"));
        if(State.Facts.Contains(TEXT("public_meal")))Pages.Add(TEXT("火边已经摆好用餐器具。忙完这一趟，可以回来歇一会儿。"));
        const int32 Page=ExplorationBriefingPage++%Pages.Num();
        Feedback=FString::Printf(TEXT("%d/%d · 再按 E 翻页\n%s"),Page+1,Pages.Num(),*Pages[Page]);
    }
    G->SetFeedback(Feedback);
    RefreshExploration();return true;
}

void UHearthwardCampaignSubsystem::RefreshExploration()
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
    const auto* G=Gameplay();
    const bool HomeReady=M2HomeReady();
    const double Hour=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().MinuteOfDay/60.;
    const bool HomeNight=HomeReady && (Hour>=18 || Hour<6);
    for(const auto& V:HearthwardCampaign::Rows(TEXT("locations")))
    {
        const auto R=V->AsObject();if(!R->HasField(TEXT("exploration_action")))continue;
        const FName Id(*Text(R,TEXT("id")));const FString Action=Text(R,TEXT("exploration_action"));
        const FName Tag(*(TEXT("M1Node:")+Id.ToString()));
        auto* Existing=Scenery.FindByPredicate([&](const auto& A){return A.IsValid() && A->ActorHasTag(Tag);});
        const bool Far=FVector::Dist2D(Position(Id),PlayerAt)>35000;
        if(Existing)
        {
            if(Far){Existing->Get()->Destroy();continue;}
            const bool Taken=(Action==TEXT("bundle") || Action==TEXT("token")) && ExplorationComplete(Id);
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
                if(Part->ComponentHasTag(TEXT("M2Well")))Show=State.Facts.Contains(TEXT("m2:well_delivered"));
                if(Part->ComponentHasTag(TEXT("M2Signal")))Show=State.Facts.Contains(TEXT("m2:signal_delivered"));
                if(Part->ComponentHasTag(TEXT("M2Harvest")))Show=G->Claimed.Contains(TEXT("side_11"));
                if(Part->ComponentHasTag(TEXT("M2Letter")))Show=State.Facts.Contains(TEXT("delivered_letter"));
                if(Part->ComponentHasTag(TEXT("M2Record")))Show=State.Facts.Contains(TEXT("copied_record"));
                if(Part->ComponentHasTag(TEXT("M2Work")))Show=M2EventAvailable(TEXT("work"));
                if(Part->ComponentHasTag(TEXT("M2Home")))Show=HomeReady;
                if(Part->ComponentHasTag(TEXT("M2HomeFlag")))Show=HomeReady && State.Facts.Contains(TEXT("m2:signal_delivered"));
                Part->SetVisibility(Show);
            }
            TInlineComponentArray<UPointLightComponent*> Lights(Existing->Get());
            for(auto* Light:Lights)if(Light->ComponentHasTag(TEXT("M2HomeLight")))Light->SetVisibility(HomeNight);
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
            auto* Mesh=LoadObject<UStaticMesh>(nullptr,Asset);if(!Mesh)return static_cast<UStaticMeshComponent*>(nullptr);
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
            Part->RegisterComponent();return Part;
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
        auto Stone=[&](FVector Offset,FVector Size,FRotator Rotation=FRotator::ZeroRotator,bool Block=true)
        {
            if(auto* Part=Prop(Cube,Offset,Size,Rotation,NAME_None,Block))Part->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_RoughStone.M_RoughStone")));
        };
        if(Id==TEXT("poi_well_01"))
        {
            // A closed well is a landmark, not a fall hazard; two 6 m courtyard openings remain clear.
            for(int32 Side:{-1,1})
            {
                Stone(FVector(Side*125,260,0),FVector(55,280,90));
                Stone(FVector(0,260+Side*125,0),FVector(220,55,90));
                for(int32 X:{-650,650})Stone(FVector(X,Side*850,0),FVector(650,65,140));
                Stone(FVector(1000,Side*450,0),FVector(65,650,160));
            }
            Prop(Cube,FVector(0,260,82),FVector(220,230,15));
            for(int32 I=0;I<3;++I)Prop(Cube,FVector(-45+I*45,112,65),FVector(12,8,24));
            Prop(Chair,FVector(-500,450,0),FVector(60,60,95),FRotator(0,40,0));
            Prop(Cairn,FVector(-1020,250,0),FVector(90,90,130));
        }
        if(Id==TEXT("poi_cellar_01"))
        {
            Stone(FVector(-360,350,0),FVector(70,500,200));
            Stone(FVector(360,350,0),FVector(70,500,200));
            Stone(FVector(0,600,0),FVector(700,90,240));
            for(int32 I=0;I<6;++I)Prop(Cube,FVector(-260+I*105,540,20),FVector(100,30,175),FRotator(0,0,12));
            Prop(Cairn,FVector(470,480,0),FVector(190,130,160));
        }
        if(Id==TEXT("poi_backpath_01"))
        {
            for(int32 I=0;I<6;++I)
            {
                const FVector At(I*400,-I*360,0);
                Prop(Cairn,At+FVector(-270,-100,0),FVector(75,80,90));
                Stone(At+FVector(260,160,0),FVector(230,65,110),FRotator(0,-42,0));
            }
        }
        if(Id==TEXT("poi_signal_01"))
        {
            Prop(Cube,FVector(230,520,0),FVector(35,35,650));
            Prop(Cube,FVector(230,520,530),FVector(300,25,25));
            Stone(FVector(230,520,0),FVector(180,180,90));
            for(int32 Y:{-200,350,1000})Stone(FVector(-780,Y,0),FVector(110,300,155));
            for(int32 X:{-320,700})Stone(FVector(X,1150,0),FVector(85,85,300));
            Prop(Sign,FVector(-780,-530,0),FVector(95,30,180),FRotator(0,-30,0));
        }
        if(Id==TEXT("poi_tents_01"))
        {
            auto HeightAt=[&](double X,double Y){FVector Base;return Ground(Floor+FVector(X,Y,0),Base)?Base.Z-Floor.Z:0.;};
            double Roof=260;
            for(int32 X:{-400,400})for(int32 Y:{420,780})Roof=FMath::Max(Roof,HeightAt(X,Y)+260);
            // Shared roof elevation keeps the posts connected on the assembly slope.
            for(int32 X:{-400,400})for(int32 Y:{420,780})
                Prop(Cube,FVector(X,Y,0),FVector(35,35,Roof-HeightAt(X,Y)));
            for(int32 Y:{420,780})Prop(Cube,FVector(0,Y,Roof-HeightAt(0,Y)-25),FVector(880,35,30));
            Prop(Cube,FVector(0,600,Roof-HeightAt(0,600)),FVector(850,400,25));
            Prop(Bed,FVector(-160,610,0),FVector(100,210,40),FRotator(0,90,0));
            Prop(Chest,FVector(420,750,0),FVector(120,80,70));
            for(int32 I=0;I<4;++I)Prop(Cube,FVector(-160+I*70,-230-I*120,0),FVector(24,13,2),FRotator(0,-20,0));
        }
        if(Id==TEXT("m2_camp_well"))
        {
            Prop(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),FVector(0,0,82),FVector(38,38,16),FRotator::ZeroRotator,TEXT("M2Well"));
            Prop(Chest,FVector(190,0,0),FVector(90,65,55),FRotator::ZeroRotator,TEXT("M2Harvest"));
            Prop(Sign,FVector(-35,0,82),FVector(25,8,35),FRotator::ZeroRotator,TEXT("M2Letter"));
        }
        if(Id==TEXT("m2_camp_signal"))
        {
            Prop(Cube,FVector(0,0,82),FVector(85,50,12),FRotator::ZeroRotator,TEXT("M2Signal"));
            Prop(Sign,FVector(50,0,82),FVector(35,10,50),FRotator::ZeroRotator,TEXT("M2Record"));
        }
        if(Id==TEXT("m2_event_work"))Prop(Chair,FVector(200,0,0),FVector(65,65,95),FRotator::ZeroRotator,TEXT("M2Work"));
        if(Id==TEXT("poi_home_steps_01"))
        {
            for(int32 I=0;I<7;++I)Stone(FVector(0,I*180,0),FVector(360,130,12),FRotator::ZeroRotator,false);
            Prop(Bed,FVector(620,700,0),FVector(100,210,50),FRotator::ZeroRotator,TEXT("M2Home"));
            Prop(Table,FVector(620,380,0),FVector(140,85,75),FRotator::ZeroRotator,TEXT("M2Home"));
            Prop(Cube,FVector(-400,350,0),FVector(20,20,280),FRotator::ZeroRotator,TEXT("M2HomeFlag"));
            Prop(Cube,FVector(-350,350,205),FVector(100,8,65),FRotator::ZeroRotator,TEXT("M2HomeFlag"));
            for(int32 X:{-260,260})
            {
                Prop(Cube,FVector(X,150,0),FVector(25,25,100),FRotator::ZeroRotator,TEXT("M2Home"));
                Prop(Lantern,FVector(X,150,100),FVector(28,28,45),FRotator::ZeroRotator,TEXT("M2Home"));
                auto* Light=NewObject<UPointLightComponent>(A);A->AddInstanceComponent(Light);Light->SetupAttachment(Root);
                Light->ComponentTags.Add(TEXT("M2HomeLight"));Light->SetRelativeLocation(FVector(X,150,150));
                Light->SetIntensity(2200);Light->SetAttenuationRadius(1100);Light->SetLightColor(FLinearColor(1,.63f,.3f));Light->SetCastShadows(false);Light->SetVisibility(HomeNight);Light->RegisterComponent();
            }
        }
        if(Id==TEXT("m2_route_river_gate"))for(int32 I=0;I<4;++I)
        {
            Prop(Cube,FVector(-600,200+I*330,0),FVector(70,70,210),FRotator::ZeroRotator,NAME_None,true);
            Prop(Cairn,FVector(500,200+I*330,0),FVector(100,90,80));
        }
        if(Id==TEXT("m2_route_workshops"))for(int32 I=0;I<3;++I)
        {
            Prop(Chest,FVector(I%2?550:-550,300+I*560,0),FVector(340,220,180),FRotator::ZeroRotator,NAME_None,true);
            Prop(Cube,FVector(I%2?550:-550,300+I*560,180),FVector(380,240,25));
        }
        if(Id==TEXT("poi_river_01") || Id==TEXT("poi_workshop_01") || Id==TEXT("m1_camp_board") || Id==TEXT("poi_well_01") || Id==TEXT("poi_signal_01"))
        {
            Prop(Cube,FVector(90,0,0),FVector(25,25,100));
            Prop(Lantern,FVector(90,0,100),FVector(25,25,45));
            auto* Light=NewObject<UPointLightComponent>(A);A->AddInstanceComponent(Light);Light->SetupAttachment(Root);
            Light->SetRelativeLocation(FVector(90,0,140));Light->SetIntensity(2500);Light->SetAttenuationRadius(900);Light->SetLightColor(FLinearColor(1.f,.63f,.3f));Light->SetCastShadows(false);Light->RegisterComponent();
        }
        A->SetActorHiddenInGame((Action==TEXT("bundle") || Action==TEXT("token")) && ExplorationComplete(Id));
    }
    Scenery.RemoveAll([](const auto& A){return !A.IsValid();});
}
