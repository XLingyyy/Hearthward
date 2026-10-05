#include "HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
using namespace HearthwardData;

void UHearthwardScreenWidget::ComposeWorldMap()
{
    auto* G=Gameplay();
    const bool Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Active();
    const double Extent=Campaign?403200:6000;
    const FVector2D Center(962,475),MapSize(1110,760);
    const auto MapPoint=[&](FVector World){ return Center+FVector2D(World.X,-World.Y)*FVector2D(MapSize.X/Extent,MapSize.Y/Extent)*MapZoom+MapPan; };
    const auto TextHeight=[&](const FHearthwardUIElement& E)
    {
        const bool Display=E.FontRole==TEXT("display") || (E.FontRole.IsEmpty() && E.Font>=30);
        FSlateFontInfo Font(Display?DisplayTypeface:Typeface,FMath::RoundToInt(E.Font*.75f));Font.LetterSpacing=E.Tracking;
        const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        TArray<FString> SourceLines;E.Text.ParseIntoArrayLines(SourceLines,false);int32 Lines=0;
        for(FString Line:SourceLines)
        {
            while(Line.Len()>1 && Measure->Measure(Line,Font).X>E.Size.X)
            {
                const int32 Count=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,E.Size.X)+1,1,Line.Len());
                Line=Line.Mid(Count);++Lines;
            }
            ++Lines;
        }
        return Lines*E.Font*1.6f;
    };
    if(ReadableLayout())
    {
        Element(TEXT("text"),TEXT("滚轮阅读侧栏"),FVector2D(94,228),FVector2D(290,48),17);
        Elements.Last().Component=TEXT("map.sidebar");Elements.Last().LayoutId=TEXT("map.scrollHint");
    }
    TArray<TPair<int32,double>> LocationLabels;
    int32 RemainingHint=INDEX_NONE;
    const int32 FirstMapElement=Elements.Num();
    Element(TEXT("image"),TEXT(""),Center-MapSize*.5*MapZoom+MapPan,MapSize*MapZoom,18,TEXT(""),TEXT("mapTerrain"));
    const float FogRadius=Number(Catalog()->GetObjectField(TEXT("tuning")),TEXT("fogRadius"));
    const FVector Origin=Campaign?FVector::ZeroVector:G->LocationPosition(TEXT("camp"));
    const double RadiusSquared=FMath::Square(double(FogRadius));
    const auto ExploredAt=[&](FVector World)
    {
        return G->Explored.ContainsByPredicate([&](FVector2D Seen)
        { return FVector2D::DistSquared(FVector2D(World.X,World.Y),Seen)<=RadiusSquared; });
    };
    const double PixelWorldSize=Extent/(MapSize.X*MapZoom);
    const auto FogCell=[&](FVector2D Local,double Side,const auto& Subdivide)->void
    {
        const FVector2D Min=Local+FVector2D(Origin.X,Origin.Y),Max=Min+FVector2D(Side,Side);
        bool Intersects=false;
        for(const FVector2D Seen:G->Explored)
        {
            const double FarX=FMath::Max(FMath::Abs(Min.X-Seen.X),FMath::Abs(Max.X-Seen.X));
            const double FarY=FMath::Max(FMath::Abs(Min.Y-Seen.Y),FMath::Abs(Max.Y-Seen.Y));
            if(FMath::Square(FarX)+FMath::Square(FarY)<=RadiusSquared)return;
            const FVector2D Nearest(FMath::Clamp(Seen.X,Min.X,Max.X),FMath::Clamp(Seen.Y,Min.Y,Max.Y));
            Intersects|=FVector2D::DistSquared(Nearest,Seen)<=RadiusSquared;
        }
        // Keep distant fog coarse; refine only the explored boundary to a design pixel.
        if(Intersects && Side>PixelWorldSize)
        {
            const double Half=Side*.5;
            Subdivide(Local,Half,Subdivide);Subdivide(Local+FVector2D(Half,0),Half,Subdivide);
            Subdivide(Local+FVector2D(0,Half),Half,Subdivide);Subdivide(Local+FVector2D(Half,Half),Half,Subdivide);
            return;
        }
        Element(TEXT("fog"),TEXT(""),MapPoint(FVector(Local.X,Local.Y+Side,0)),MapSize*(Side/Extent*MapZoom)+FVector2D(1,1));
    };
    const int32 Cell=Extent/20;
    for(int32 X=-Extent/2;X<Extent/2;X+=Cell)
        for(int32 Y=-Extent/2;Y<Extent/2;Y+=Cell)
            FogCell(FVector2D(X,Y),Cell,FogCell);
    if(Campaign && G->QuestAvailable(TEXT("main_05")))
    {
        const auto& Route=Catalog()->GetObjectField(TEXT("campaign"))->GetArrayField(TEXT("route_trace"));
        for(int32 I=0;I<Route.Num();I+=3)
        {
            const auto& P=Route[I]->AsArray();
            const FVector World(P[0]->AsNumber()*100,P[1]->AsNumber()*100,0);
            if(!ExploredAt(World))continue;
            Element(TEXT("text"),TEXT("·"),MapPoint(World)-FVector2D(4,10),FVector2D(12,20),18);
            Elements.Last().Color=Color(TEXT("gold"));
        }
    }
    for(const auto& V:Rows(TEXT("locations")))
    {
        const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id"))); if(!G->Discovered.Contains(Id)) continue;
        if(Category==TEXT("travel") && Text(R,TEXT("kind"))==TEXT("landmark")) continue;
        if(Campaign && !GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->HasLocation(Id))continue;
        const FVector World=G->LocationPosition(Id);if(!ExploredAt(World))continue;
        const FVector2D UI=MapPoint(World-Origin)-FVector2D(27,27);
        Element(TEXT("image"),TEXT(""),UI+FVector2D(10,9),FVector2D(34,36),18,TEXT(""),G->Activated.Contains(Id)?TEXT("mapTravelIcon"):Text(R,TEXT("kind"))==TEXT("landmark")?TEXT("mapLandmarkIcon"):TEXT("mapCampIcon"));
        Element(TEXT("tab"),TEXT(""),UI,FVector2D(54,54),32,TEXT("location:")+Id.ToString(),TEXT(""),SelectedLocation==Id);
        Element(TEXT("text"),Text(R,TEXT("name")),UI+FVector2D(-27,58),FVector2D(200,35),19);
        LocationLabels.Emplace(Elements.Num()-1,UI.Y);
    }
    if(const auto* Story=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();Campaign && Story->State.ShowRemaining())
    {
        TSet<FIntPoint> Areas;
        for(const auto& Enemy:Story->State.Enemies)
            if(Enemy.Combat.Health>0 && (Enemy.Group==TEXT("base") || Enemy.Group==TEXT("reinforcement")))
                Areas.Add(FIntPoint(FMath::FloorToInt(Enemy.Combat.Position.X/5000),FMath::FloorToInt(Enemy.Combat.Position.Y/5000)));
        for(const auto& Area:Areas)
        {
            Element(TEXT("text"),TEXT("○"),MapPoint(FVector((Area.X+.5)*5000,(Area.Y+.5)*5000,0))-FVector2D(16,20),FVector2D(40,40),30);
            Elements.Last().Color=Color(TEXT("gold"));
        }
        Element(TEXT("text"),TEXT("○ 剩余驻军的大致区域"),FVector2D(1150,790),FVector2D(350,32),17);
        RemainingHint=Elements.Num()-1;
    }
    const FVector Player=GetOwningPlayerPawn()->GetActorLocation()-Origin;
    if(G->HasWaypoint)
    {
        Element(TEXT("text"),TEXT("◇"),MapPoint(G->Waypoint-Origin)-FVector2D(15,20),FVector2D(45,45),35);
        Elements.Last().Color=Color(TEXT("gold"));
        Element(TEXT("button"),TEXT("清除标记"),MapPoint(G->Waypoint-Origin)+FVector2D(12,10),FVector2D(150,35),16,TEXT("clearWaypoint"));
    }
    Element(TEXT("arrow"),TEXT(""),MapPoint(Player)+FVector2D(17,14),FVector2D(20,20)); Elements.Last().Value=GetOwningPlayer()->GetControlRotation().Yaw+90; Elements.Last().Color=Color(TEXT("teal"));
    for(int32 I=FirstMapElement;I<Elements.Num();++I) Elements[I].MapClipped=true;
    Element(TEXT("bar"),TEXT(""),FVector2D(94,223),FVector2D(280,4)); Elements.Last().Value=float(G->Discovered.Num())/Rows(TEXT("locations")).Num(); Elements.Last().Color=Color(TEXT("gold"));
    const auto R=Find(TEXT("locations"),SelectedLocation.ToString());
    Element(TEXT("text"),Text(R,TEXT("name")),FVector2D(91,639),FVector2D(320,40),25);
    Element(TEXT("button"),G->Activated.Contains(SelectedLocation)?TEXT("传送至此"):TEXT("靠近路标并按 E 激活"),FVector2D(88,687),FVector2D(295,43),17,TEXT("travel"));
    float GuidanceTop=Center.Y+MapSize.Y*.5f-8;
    if(RemainingHint!=INDEX_NONE)
    {
        auto& Hint=Elements[RemainingHint];Hint.Size.X=600;Hint.Size.Y=TextHeight(Hint);
        Hint.Position={Center.X+MapSize.X*.5f-16-Hint.Size.X,GuidanceTop-Hint.Size.Y};
    }
    if(!G->TrackedQuest.IsNone())
    {
        const auto Q=Find(TEXT("quests"),G->TrackedQuest.ToString());
        Element(TEXT("text"),TEXT("◎ ")+Text(Q,TEXT("objective")),FVector2D(535,816),FVector2D(910,42),19);
        auto& Objective=Elements.Last();
        Objective.Size.Y=TextHeight(Objective);
        Objective.Position.Y=GuidanceTop-Objective.Size.Y;GuidanceTop=Objective.Position.Y;
        Objective.Component=TEXT("map.canvas");
        if(RemainingHint!=INDEX_NONE)
        {
            auto& Hint=Elements[RemainingHint];Hint.Position.Y=GuidanceTop-12-Hint.Size.Y;
        }
        const auto Target=Find(TEXT("locations"),Campaign?GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->QuestLocation(G->TrackedQuest).ToString():Text(Q,TEXT("location")));
        if(Target)
        {
            const FVector World=G->LocationPosition(FName(*Text(Target,TEXT("id"))));
            if(ExploredAt(World))
            {
                Element(TEXT("image"),TEXT(""),MapPoint(World-Origin)+FVector2D(25,-40),FVector2D(33,40),18,TEXT(""),TEXT("mapQuestIcon")); Elements.Last().MapClipped=true;
            }
        }
    }
    for(const auto& Marker:LocationLabels)
    {
        auto& Label=Elements[Marker.Key];Label.Size.Y=TextHeight(Label);
        double ClearTop=GuidanceTop;
        if(RemainingHint!=INDEX_NONE)
        {
            const auto& Hint=Elements[RemainingHint];
            if(Label.Position.X+Label.Size.X>Hint.Position.X && Label.Position.X<Hint.Position.X+Hint.Size.X
                && Label.Position.Y+Label.Size.Y>Hint.Position.Y && Label.Position.Y<Hint.Position.Y+Hint.Size.Y)
                ClearTop=FMath::Min(ClearTop,Hint.Position.Y);
        }
        if(Label.Position.Y>=95 && Label.Position.Y<855 && Label.Position.Y+Label.Size.Y>ClearTop
            && Label.Position.X+Label.Size.X>535 && Label.Position.X<1501)
            Label.Position.Y=FMath::Min(Marker.Value-Label.Size.Y-8,ClearTop-Label.Size.Y-8);
    }
}
