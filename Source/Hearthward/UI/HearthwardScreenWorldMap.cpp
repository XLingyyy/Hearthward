#include "HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "HearthwardQuestGuidance.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
using namespace HearthwardData;

void UHearthwardScreenWidget::ComposeWorldMap()
{
    auto* G=Gameplay();auto* Story=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
    const bool Campaign=Story->Active();
    const auto KindOf=[&](const TSharedPtr<FJsonObject>& Row)
    {
        const FName Id(*Text(Row,TEXT("id")));
        return Campaign?Text(HearthwardCampaign::Find(TEXT("locations"),Id),TEXT("kind")):Text(Row,TEXT("kind"));
    };
    const auto Place=[&](const TSharedPtr<FJsonObject>& Row)
    {
        const FString Kind=KindOf(Row);
        return !Kind.IsEmpty() && Kind!=TEXT("persistent_object") && Kind!=TEXT("person") && Kind!=TEXT("rescue");
    };
    const double RadiusSquared=FMath::Square(Number(Catalog()->GetObjectField(TEXT("tuning")),TEXT("fogRadius")));
    const auto Known=[&](FVector World)
    {
        return MapVisible(World) && G->Explored.ContainsByPredicate([&](FVector2D Seen)
        {return FVector2D::DistSquared(FVector2D(World.X,World.Y),Seen)<=RadiusSquared;});
    };
    // Discovered places survive sparse/old exploration samples. Objects and
    // people belong to quest guidance rather than the place/travel catalogue.
    for(const auto& Value:Rows(TEXT("locations")))
    {
        const auto Location=Value->AsObject();const FName Id(*Text(Location,TEXT("id")));
        if(!G->Discovered.Contains(Id) || (Campaign && !Story->HasLocation(Id))
            || !Place(Location))continue;
        const FVector World=G->LocationPosition(Id);if(!MapVisible(World))continue;
        const FVector2D Point=MapPoint(World);
        const bool Flag=KindOf(Location)==TEXT("control_zone");
        Element(Flag?TEXT("mapFlag"):TEXT("tab"),TEXT(""),Point-(Flag?FVector2D(12,32):FVector2D(10,10)),Flag?FVector2D(24,32):FVector2D(20,20),18,TEXT("location:")+Id.ToString());
        Elements.Last().LayoutId=TEXT("map.location.marker.")+Id.ToString();
        Elements.Last().Component=TEXT("map.canvas");Elements.Last().MapClipped=true;
        if(Flag)
        {
            Elements.Last().Color=Story->State.Flags.Contains(FName(*Id.ToString().Mid(4)))?FLinearColor(.55f,.7f,.52f,1):Color(TEXT("gold"));
            Element(TEXT("mapSite"),TEXT(""),Point,{-180,-90});Elements.Last().Component=TEXT("map.canvas");Elements.Last().MapClipped=true;
        }
        const FString Label=Text(Location,TEXT("name"))+(Flag?TEXT("（占领点）"):FString());
        Element(TEXT("mapRegion"),Label,Point+(Flag?FVector2D(-290,-110):FVector2D(-110,17)),{220,38},20);
        Elements.Last().Component=TEXT("map.canvas");Elements.Last().LayoutId=TEXT("map.location.")+Id.ToString();
        Elements.Last().Font=20*FMath::Min(MapZoom,1.25f);Elements.Last().FontRole=TEXT("display");
    }
    // The existing discovery and route rules share the new terrain projection.
    if(Campaign && G->QuestAvailable(TEXT("main_05")))
    {
        const auto& Route=Catalog()->GetObjectField(TEXT("campaign"))->GetArrayField(TEXT("route_trace"));
        for(int32 I=0;I<Route.Num();I+=3)
        {
            const auto& XY=Route[I]->AsArray();const FVector World(XY[0]->AsNumber()*100,XY[1]->AsNumber()*100,0);
            if(!Known(World))continue;
            Element(TEXT("text"),TEXT("·"),MapPoint(World)-FVector2D(4,10),{12,20},18);
            Elements.Last().Color=Color(TEXT("gold"));Elements.Last().Component=TEXT("map.canvas");
        }
    }
    if(G->HasWaypoint && MapVisible(G->Waypoint))
    {
        Element(TEXT("text"),TEXT("◇"),MapPoint(G->Waypoint)-FVector2D(15,20),{40,40},30);
        Elements.Last().Color=Color(TEXT("gold"));Elements.Last().Component=TEXT("map.canvas");
    }
    if(Campaign && Story->State.ShowRemaining())
    {
        TSet<FIntPoint> Areas;
        for(const auto& Enemy:Story->State.Enemies)
            if(Enemy.Combat.Health>0 && (Enemy.Group==TEXT("base") || Enemy.Group==TEXT("reinforcement")))
                Areas.Add(FIntPoint(FMath::FloorToInt(Enemy.Combat.Position.X/5000),FMath::FloorToInt(Enemy.Combat.Position.Y/5000)));
        for(const auto& Area:Areas)
        {
            const FVector World((Area.X+.5)*5000,(Area.Y+.5)*5000,0);if(!MapVisible(World))continue;
            Element(TEXT("text"),TEXT("○"),MapPoint(World)-FVector2D(12,14),{30,30},22);
            Elements.Last().Color=Color(TEXT("gold"));Elements.Last().Component=TEXT("map.canvas");
        }
    }
    if(!G->TrackedQuest.IsNone())
    {
        const auto Guidance=HearthwardQuestGuidance::Resolve(GetOwningPlayer());
        const FVector World=Guidance.World;
        if(Guidance.Visible && G->Discovered.Contains(Guidance.Location) && Known(World))
        {
            Element(TEXT("image"),TEXT(""),MapPoint(World)+FVector2D(12,-35),{24,30},18,TEXT(""),TEXT("mapQuestIcon"));
            Elements.Last().Component=TEXT("map.canvas");Elements.Last().LayoutId=TEXT("map.questTarget");
        }
    }
    if(!WorldMap)return;
    const float FontScale=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f;
    const int32 PageSize=FontScale>1.25f?6:9;const float RowHeight=FontScale>1.25f?67:48;
    Element(TEXT("inventorySurface"),TEXT(""),{36,100},{368,710});Elements.Last().Color=FLinearColor(.025f,.03f,.032f,.96f);
    Element(TEXT("text"),TEXT("已发现地点"),{60,119},{320,58},24);
    Element(TEXT("menuAction"),Category==TEXT("travel")?TEXT("路标 · 显示全部"):TEXT("全部 · 只看路标"),{60,178},{320,54},18,TEXT("mapFilter"));
    TArray<TSharedPtr<FJsonObject>> Locations;
    for(const auto& Value:Rows(TEXT("locations")))
    {
        const auto Location=Value->AsObject();const FName Id(*Text(Location,TEXT("id")));
        if(!G->Discovered.Contains(Id) || (Campaign && !Story->HasLocation(Id)) || !Place(Location))continue;
        if(Category==TEXT("travel") && !G->IsTravelLocation(Id))continue;
        Locations.Add(Location);
    }
    if(!SelectedLocation.IsNone() && !Locations.ContainsByPredicate([&](const auto& Row){return Text(Row,TEXT("id"))==SelectedLocation.ToString();}))SelectedLocation=NAME_None;
    const int32 Pages=FMath::Max(1,FMath::DivideAndRoundUp(Locations.Num(),PageSize));Scroll=FMath::Clamp(Scroll,0,Pages-1);
    for(int32 I=Scroll*PageSize;I<FMath::Min(Locations.Num(),(Scroll+1)*PageSize);++I)
    {
        const auto Location=Locations[I];const FName Id(*Text(Location,TEXT("id")));
        Element(TEXT("menuAction"),Text(Location,TEXT("name")),{60,235+(I%PageSize)*RowHeight},{320,RowHeight-4},19,TEXT("location:")+Id.ToString(),TEXT(""),SelectedLocation==Id);
        Elements.Last().LayoutId=TEXT("map.location.list.")+Id.ToString();
    }
    Element(TEXT("menuAction"),TEXT("‹"),{60,670},{55,54},22,TEXT("map.previous"));Elements.Last().Enabled=Scroll>0;
    Element(TEXT("text"),FString::Printf(TEXT("%d / %d"),Scroll+1,Pages),{140,676},{170,48},18);
    Element(TEXT("menuAction"),TEXT("›"),{330,670},{55,54},22,TEXT("map.next"));Elements.Last().Enabled=Scroll+1<Pages;
    const auto Selected=Find(TEXT("locations"),SelectedLocation.ToString());
    const bool Station=G->IsTravelLocation(SelectedLocation),Flag=KindOf(Selected)==TEXT("control_zone");
    Element(TEXT("menuAction"),Station?(G->Activated.Contains(SelectedLocation)?TEXT("传送至所选地点"):TEXT("路标未激活 · 查看说明"))
        :Flag?TEXT("占领地点 · 无法传送"):TEXT("请选择传送路标"),{60,725},{320,60},18,TEXT("travel"));
    Elements.Last().LayoutId=TEXT("map.travel");Elements.Last().Enabled=Station;
    const auto Info=[&](const FString& TextValue,const FString& Id,double Top)
    {
        Element(TEXT("notice"),TextValue,{440,Top},{1100,64},18);auto& E=Elements.Last();E.LayoutId=Id;E.TextInset=20;E.FontRole=TEXT("body");
        const FSlateFontInfo Font(Typeface,FMath::RoundToInt(E.Font*.75f));
        const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        TArray<FString> Lines;E.Text.ParseIntoArrayLines(Lines,false);int32 Count=0;
        for(FString Line:Lines)
        {
            while(Line.Len()>1 && Measure->Measure(Line,Font).X>E.Size.X-40)
            {
                const int32 N=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,E.Size.X-40)+1,1,Line.Len());
                Line=Line.Mid(N);++Count;
            }
            ++Count;
        }
        E.Size.Y=Count*E.Font*1.6f+24;return Top+E.Size.Y+16;
    };
    double InfoTop=110;
    if(Selected && Place(Selected))
    {
        FString Detail=Text(Selected,TEXT("name"));
        if(Flag)
        {
            const FName Zone(*SelectedLocation.ToString().Mid(4));
            Detail+=TEXT(" · 占领旗帜（地点，不能传送）\n");
            Detail+=Story->State.Flags.Contains(Zone)?TEXT("此区已占领，旗帜显示当前控制状态。")
                :Story->State.Phase==TEXT("prologue")?TEXT("夜袭序章先带弟弟从后巷撤离；返回故乡后，清理该区敌军，靠近旗帜按 E，站定5秒占领。")
                :TEXT("先清理该区敌军，再靠近旗帜按 E，站定5秒完成占领；移动或受袭会中断。");
        }
        else if(Station)
            Detail+=FString(TEXT(" · 传送路标\n"))+(G->Activated.Contains(SelectedLocation)
                ?TEXT("目的地已激活。需在另一个已激活的路标旁选择传送；载入目的地区域后自动抵达。")
                :TEXT("目的地未激活。请先步行到路标旁按 E 激活，再从另一个已激活路标传送。"));
        else Detail+=TEXT(" · 地点（不能传送）\n")+Text(Selected,TEXT("description"));
        InfoTop=Info(Detail,TEXT("map.location.detail"),InfoTop);
    }
    if(!Message.IsEmpty())Info(Message,TEXT("map.travel.feedback"),InfoTop);
    if(G->HasWaypoint)Element(TEXT("menuAction"),TEXT("清除标记"),{430,850},{200,42},18,TEXT("clearWaypoint"));
    double GuidanceTop=850;
    if(!G->TrackedQuest.IsNone())
    {
        const auto Q=Find(TEXT("quests"),G->TrackedQuest.ToString());
        Element(TEXT("text"),TEXT("◎ ")+Text(Q,TEXT("objective")),{450,750},{1080,58},18);
        auto& Objective=Elements.Last();
        const FSlateFontInfo Font(Objective.Font>=30?DisplayTypeface:Typeface,FMath::RoundToInt(Objective.Font*.75f));
        const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        TArray<FString> Lines;Objective.Text.ParseIntoArrayLines(Lines,false);int32 Count=0;
        for(FString Line:Lines)
        {
            while(Line.Len()>1 && Measure->Measure(Line,Font).X>Objective.Size.X)
            {
                const int32 N=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,Objective.Size.X)+1,1,Line.Len());
                Line=Line.Mid(N);++Count;
            }
            ++Count;
        }
        Objective.Size.Y=Count*Objective.Font*1.6f;Objective.Position.Y=GuidanceTop-Objective.Size.Y;
        GuidanceTop=Objective.Position.Y-12;
    }
    if(Campaign && Story->State.ShowRemaining())
    {
        Element(TEXT("text"),TEXT("○ 剩余驻军的大致区域"),{450,GuidanceTop-58},{1000,58},18);
    }
}
