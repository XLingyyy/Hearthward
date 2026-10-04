#include "../Nature/HearthwardNatureSubsystem.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "HearthwardScreenWidget.h"
#include "../Experience/HearthwardTraversalComponent.h"
#include "Engine/GameInstance.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Save/HearthwardSaveSubsystem.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../AI/HearthwardLocalAISubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "../Interaction/HearthwardInteractionComponent.h"
#include "../Interaction/HearthwardInteractionTargetComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"

using namespace HearthwardData;
FString UHearthwardScreenWidget::Resolve(const FString& Bind) const
{
    const auto* G=Gameplay();
    if(Bind==TEXT("points")) return FString::FromInt(G->SkillPoints());
    if(Bind==TEXT("exploration")) return FString::Printf(TEXT("探索进度  %.0f%%"),100.f*G->Discovered.Num()/Rows(TEXT("locations")).Num());
    if(Bind==TEXT("category")) return Category.IsEmpty()?TEXT("全部"):Category;
    if(Bind==TEXT("quest")) return Text(Find(TEXT("quests"),G->TrackedQuest.ToString()),TEXT("name"));
    if(Bind==TEXT("objective")) return Text(Find(TEXT("quests"),G->TrackedQuest.ToString()),TEXT("objective"));
    if(Bind==TEXT("location"))
    {
        const FName Nearby=G->NearbyLocation();
        return Nearby.IsNone()?TEXT("荒野"):Text(Find(TEXT("locations"),Nearby.ToString()),TEXT("name"));
    }
    if(Bind==TEXT("autosave")) return FString::Printf(TEXT("%d 分钟"),GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->GetAutoMinutes());
    if(Bind==TEXT("time"))
    { const int32 S=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds; return FString::Printf(TEXT("%d时%02d分"),S/3600,(S/60)%60); }
    return FString();
}
void UHearthwardScreenWidget::ComposeInventory(bool Storage)
{
    const auto* G=Gameplay(); const auto* Bag=Inventory(); auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    TArray<FString> Categories; const auto Layout=Theme->GetObjectField(TEXT("inventory"));
    for(const auto& V:Layout->GetArrayField(TEXT("categories"))) Categories.Add(V->AsString());
    for(int32 I=0;I<Categories.Num();++I)
    {
        const float X=Storage?70:57, Y=Storage?232:151, Width=Storage?83:70;
        for(int32 Side=0;Side<(Storage?2:1);++Side)
        {
            const int32 FirstCategoryElement=Elements.Num();
            const FVector2D P(X+Side*1029+I*Width,Y);
            Element(TEXT("image"),TEXT(""),P+FVector2D(Storage?24:14,Storage?-9:6),FVector2D(31,31),18,TEXT(""),Layout->GetArrayField(TEXT("categoryIcons"))[I]->AsString());
            Element(TEXT("tab"),TEXT(""),P-FVector2D(0,Storage?10:0),FVector2D(Width-3,Storage?62:44),16,TEXT("filter:")+Categories[I],TEXT(""),Category==Categories[I] || (Category.IsEmpty() && I==0));
            if(Storage) Element(TEXT("text"),Categories[I],P+FVector2D(23,25),FVector2D(72,26),15);
            for(int32 N=FirstCategoryElement;N<Elements.Num();++N) Elements[N].Component=Storage?(Side?TEXT("storage.stock"):TEXT("storage.bag")):TEXT("inventory.bag");
        }
    }
    TArray<TSharedPtr<FJsonObject>> Owned,Stock;
    for(const auto& V:Rows(TEXT("items")))
    {
        const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id")));
        if(!Category.IsEmpty() && Category!=TEXT("全部") && Category!=Text(R,TEXT("category"))) continue;
        if(Bag->GetItemCount(Id)>0) Owned.Add(R);
        if(Store->GetItemCount(Id)>0) Stock.Add(R);
    }
    if(!Storage) Owned.StableSort([](const auto& A,const auto& B){return Number(A,TEXT("displayOrder"))<Number(B,TEXT("displayOrder"));});
    if(!Storage && !Owned.ContainsByPredicate([&](const auto& R){return FName(*Text(R,TEXT("id")))==SelectedItem;}))
        SelectedItem=Owned.IsEmpty()?NAME_None:FName(*Text(Owned[0],TEXT("id")));
    if(Storage) Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,FMath::Max(Owned.Num(),Stock.Num())-11));
    if(Storage)
    {
        for(int32 Side=0;Side<2;++Side)
        {
            const auto& List=Side?Stock:Owned; const float X=Side?1091:62;
            for(int32 I=Scroll;I<FMath::Min(List.Num(),Scroll+11);++I)
            {
                const auto R=List[I]; const FString Id=Text(R,TEXT("id")); const float Y=291+(I-Scroll)*49;
                Element(TEXT("button"),TEXT("       ")+Text(R,TEXT("name")),FVector2D(X,Y),FVector2D(510,47),20,(Side?TEXT("withdraw:"):TEXT("deposit:"))+Id,TEXT(""),SelectedItem==FName(*Id) && StorageToCamp==(Side==0));
                Element(TEXT("image"),TEXT(""),FVector2D(X+5,Y+3),FVector2D(39,39),18,TEXT(""),Text(R,TEXT("icon")));
                Element(TEXT("text"),FString::FromInt(Side?Store->GetItemCount(FName(*Id)):Bag->GetItemCount(FName(*Id))),FVector2D(X+447,Y+12),FVector2D(60,30),19);
            }
            if(List.IsEmpty()) Element(TEXT("text"),TEXT("暂无物品"),FVector2D(X+26,310),FVector2D(440,40),20);
        }
        Element(TEXT("text"),FString::Printf(TEXT("%.1f / %.0f"),Bag->GetWeight(),Bag->GetCapacity()),FVector2D(422,176),FVector2D(165,35),17);
        Element(TEXT("text"),FString::Printf(TEXT("%.1f / 无限"),Store->GetWeight()),FVector2D(1440,176),FVector2D(190,35),17);
    }
    else
    {
        const auto& Rect=Theme->GetObjectField(TEXT("regions"))->GetArrayField(TEXT("inventoryGrid"));
        const FVector2D Start(Rect[0]->AsNumber(),Rect[1]->AsNumber());
        const int32 Columns=Number(Layout,TEXT("columns")),RowCount=Number(Layout,TEXT("rows"));
        const auto& Cell=Layout->GetArrayField(TEXT("cell")); const auto& SlotSize=Layout->GetArrayField(TEXT("slotSize"));
        Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,FMath::DivideAndRoundUp(Owned.Num(),Columns)-RowCount));
        for(int32 N=0;N<Columns*RowCount;++N)
        {
            const FVector2D P=Start+FVector2D((N%Columns)*Cell[0]->AsNumber(),(N/Columns)*Cell[1]->AsNumber());
            Element(TEXT("slot"),TEXT(""),P,FVector2D(SlotSize[0]->AsNumber(),SlotSize[1]->AsNumber()));
        }
        for(int32 I=Scroll*Columns;I<FMath::Min(Owned.Num(),Scroll*Columns+Columns*RowCount);++I)
        {
            const auto R=Owned[I]; const FString Id=Text(R,TEXT("id")); const int32 N=I-Scroll*Columns;
            const FVector2D P=Start+FVector2D((N%Columns)*Cell[0]->AsNumber(),(N/Columns)*Cell[1]->AsNumber());
            Element(TEXT("image"),TEXT(""),P+FVector2D(5,5),FVector2D(66,65),18,TEXT(""),Text(R,TEXT("icon")));
            Element(TEXT("slot"),FString::FromInt(Bag->GetItemCount(FName(*Id))),P,FVector2D(SlotSize[0]->AsNumber(),SlotSize[1]->AsNumber()),16,TEXT("item:")+Id,TEXT(""),SelectedItem==FName(*Id));
        }
        if(Owned.IsEmpty()) Element(TEXT("text"),TEXT("背包为空"),Start+FVector2D(70,150),FVector2D(260,40),21);
        for(const auto& V:Layout->GetArrayField(TEXT("equipmentSlots")))
        {
            const auto SlotRow=V->AsObject(); const auto& Coordinates=SlotRow->GetArrayField(TEXT("position"));
            const FVector2D P(Coordinates[0]->AsNumber(),Coordinates[1]->AsNumber());
            Element(TEXT("text"),Text(SlotRow,TEXT("name")),P-FVector2D(5,28),FVector2D(90,30),15); Elements.Last().Align=TEXT("center");
            const FName Item=G->Equipment.FindRef(FName(*Text(SlotRow,TEXT("id")))); const auto R=Find(TEXT("items"),Item.ToString());
            if(R) Element(TEXT("image"),TEXT(""),P+FVector2D(7,7),FVector2D(65,65),18,TEXT(""),Text(R,TEXT("icon")));
            Element(TEXT("slot"),TEXT(""),P,FVector2D(80,82),18,R?TEXT("item:")+Item.ToString():TEXT(""));
            if(R) Element(TEXT("image"),TEXT(""),P+FVector2D(62,64),FVector2D(15,16),18,TEXT(""),TEXT("equippedMark"));
        }
        Element(TEXT("text"),TEXT("等级"),FVector2D(1340,249),FVector2D(70,30),16);
        Element(TEXT("text"),FString::FromInt(G->Level()),FVector2D(1405,234),FVector2D(110,50),32);
        const auto Tuning=Catalog()->GetObjectField(TEXT("tuning"));
        int32 LevelExperience=G->Experience;
        for(int32 L=1;L<G->Level();++L) LevelExperience-=Number(Tuning,TEXT("xpBase"))+(L-1)*Number(Tuning,TEXT("xpGrowth"));
        const int32 NextLevel=Number(Tuning,TEXT("xpBase"))+(G->Level()-1)*Number(Tuning,TEXT("xpGrowth"));
        Element(TEXT("bar"),TEXT(""),FVector2D(1340,279),FVector2D(180,7)); Elements.Last().Value=float(LevelExperience)/NextLevel; Elements.Last().Color=Color(TEXT("gold"));
        Element(TEXT("text"),FString::Printf(TEXT("%d / %d"),LevelExperience,NextLevel),FVector2D(1538,274),FVector2D(140,30),14);
        const float Values[]={G->Health,G->Hunger,G->Stamina}; const float Max[]={G->MaxHealth(),100,G->MaxStamina()};
        const FString Labels[]={TEXT("生命值"),TEXT("饱食度"),TEXT("体力")},Colors[]={TEXT("health"),TEXT("hunger"),TEXT("stamina")};
        for(int32 I=0;I<3;++I)
        {
            const float Y=330+I*48;
            Element(TEXT("text"),Labels[I],FVector2D(1380,Y),FVector2D(150,30),16);
            Element(TEXT("text"),FString::Printf(TEXT("%.0f / %.0f"),Values[I],Max[I]),FVector2D(1500,Y),FVector2D(119,30),17); Elements.Last().Align=TEXT("right");
            Element(TEXT("bar"),TEXT(""),FVector2D(1380,Y+28),FVector2D(239,7)); Elements.Last().Value=Values[I]/Max[I]; Elements.Last().Color=Color(Colors[I]);
        }
        float Defense=G->Effect(TEXT("defense"))*100;
        Element(TEXT("text"),TEXT("攻击力\n全身减伤\n重击增伤\n耐力消耗"),FVector2D(1381,528),FVector2D(230,140),18);
        Element(TEXT("text"),FString::Printf(TEXT("%.0f\n%.0f%%\n+%.0f%%\n−%.0f%%"),G->AttackPower(),FMath::Min(85.f,Defense),G->Effect(TEXT("heavy_damage"))*100,G->Effect(TEXT("cost"))*100),FVector2D(1519,528),FVector2D(100,140),18); Elements.Last().Align=TEXT("right");
        Element(TEXT("text"),TEXT("负重"),FVector2D(1381,706),FVector2D(110,30),16);
        Element(TEXT("text"),FString::Printf(TEXT("%.1f / %.0f"),Bag->GetWeight(),Bag->GetCapacity()),FVector2D(1480,705),FVector2D(139,30),18); Elements.Last().Align=TEXT("right");
        Element(TEXT("bar"),TEXT(""),FVector2D(1380,730),FVector2D(239,6)); Elements.Last().Value=Bag->GetWeight()/Bag->GetCapacity(); Elements.Last().Color=Color(TEXT("gold"));
        Element(TEXT("text"),TEXT("移动速度"),FVector2D(1381,748),FVector2D(140,30),16);
        Element(TEXT("text"),FString::Printf(TEXT("%.0f%%"),Bag->GetMoveSpeedMultiplier()*100),FVector2D(1519,748),FVector2D(100,30),18); Elements.Last().Align=TEXT("right");
        Element(TEXT("text"),FString::Printf(TEXT("负重   %.1f / %.0f"),Bag->GetWeight(),Bag->GetCapacity()),FVector2D(92,768),FVector2D(282,30),16);
        Element(TEXT("bar"),TEXT(""),FVector2D(64,795),FVector2D(310,8)); Elements.Last().Value=Bag->GetWeight()/Bag->GetCapacity(); Elements.Last().Color=Color(TEXT("gold"));
    }
    const auto R=Find(TEXT("items"),SelectedItem.ToString()); if(!R) return;
    bool Rare=false,KeyItem=false; R->TryGetBoolField(TEXT("rare"),Rare); R->TryGetBoolField(TEXT("key"),KeyItem);
    if(const auto Base=Find(TEXT("items"),Text(R,TEXT("medicineBase"))))
    {
        bool BaseRare=false,BaseKey=false; Base->TryGetBoolField(TEXT("rare"),BaseRare); Base->TryGetBoolField(TEXT("key"),BaseKey);
        Rare|=BaseRare; KeyItem|=BaseKey;
    }
    if(!Storage && (Rare || KeyItem) && (Number(R,TEXT("healing"))>0 || Number(R,TEXT("food"))>0))
        for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        {
            const auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>();
            Element(TEXT("button"),S->Permitted(SelectedItem)?TEXT("撤销弟弟自动使用授权"):TEXT("允许弟弟自动使用此物"),FVector2D(430,740),FVector2D(270,46),16,TEXT("autoPermission")); break;
        }
    const FVector2D P=Storage?FVector2D(645,366):FVector2D(439,411);
    Element(TEXT("text"),Text(R,TEXT("name")),P,FVector2D(280,45),Storage?27:23); Elements.Last().Color=Color(TEXT("gold"));
    Element(TEXT("text"),Text(R,TEXT("category")),P+FVector2D(0,33),FVector2D(240,32),16);
    Element(TEXT("text"),Text(R,TEXT("description")),P+FVector2D(0,69),FVector2D(Storage?290:228,150),15);
    const FVector2D Art=Storage?FVector2D(864,372):FVector2D(440,224);
    Element(TEXT("image"),TEXT(""),Art,Storage?FVector2D(130,125):FVector2D(226,180),18,TEXT(""),SelectedItem==TEXT("axe")?TEXT("axeLarge"):Text(R,TEXT("icon")));
    FString Properties;
    if(!Text(R,TEXT("slot")).IsEmpty())
        Properties=FString::Printf(TEXT("%s    %.0f\n耐久度    %.0f / %.0f\n重量      %.2f"),Number(R,TEXT("attack"))>0?TEXT("攻击力"):TEXT("防御力"),Number(R,TEXT("attack"),Number(R,TEXT("defense"))),G->Durability.Contains(SelectedItem)?G->Durability.FindRef(SelectedItem):Number(R,TEXT("durability"),100),Number(R,TEXT("durability"),100),Number(R,TEXT("weight"))/100);
    else Properties=FString::Printf(TEXT("单重       %.2f\n拥有       %d\n%s"),Number(R,TEXT("weight"))/100,Bag->GetItemCount(SelectedItem),Number(R,TEXT("food"))>0?*FString::Printf(TEXT("饱食恢复   +%.0f"),Number(R,TEXT("food"))):TEXT("用于营地和旅途"));
    Element(TEXT("text"),Properties,P+FVector2D(Storage?0:31,165),FVector2D(Storage?290:211,100),18);
    if(Storage)
    {
        Element(TEXT("button"),TEXT("−"),FVector2D(646,697),FVector2D(60,45),25,TEXT("quantity:-1"));
        Element(TEXT("text"),FString::FromInt(Quantity),FVector2D(762,705),FVector2D(150,40),25);
        Element(TEXT("button"),TEXT("+"),FVector2D(935,697),FVector2D(60,45),25,TEXT("quantity:1"));
        Element(TEXT("button"),StorageToCamp?TEXT("存入仓库"):TEXT("取到背包"),FVector2D(647,759),FVector2D(348,49),23,TEXT("transfer"));
    }
    else Element(TEXT("button"),TEXT("装备 / 使用"),FVector2D(442,683),FVector2D(228,43),19,TEXT("use"));
}
void UHearthwardScreenWidget::ComposeSkills()
{
    auto* G=Gameplay(); const auto& Branches=Theme->GetArrayField(TEXT("branches"));
    for(int32 I=0;I<Branches.Num();++I)
    {
        const auto B=Branches[I]->AsObject(); const FString Id=Text(B,TEXT("id"));
        Element(TEXT("image"),TEXT(""),FVector2D(369+I*243,113),FVector2D(83,78),18,TEXT(""),Id+TEXT("Icon"));
        Element(TEXT("text"),Text(B,TEXT("name")),FVector2D(379+I*243,197),FVector2D(150,40),24); Elements.Last().Tracking=180;
        Element(TEXT("text"),Text(B,TEXT("subtitle")),FVector2D(337+I*245,237),FVector2D(220,35),14); Elements.Last().Tracking=120;
        Element(TEXT("image"),TEXT(""),FVector2D(282+I*245,642),FVector2D(222,89),18,TEXT(""),Id+TEXT("Ornament"));
        Element(TEXT("text"),Text(B,TEXT("flavor")),FVector2D(325+I*245,735),FVector2D(200,50),14);
        int32 Learned=0,Total=0;
        for(const auto& V:Rows(TEXT("skills"))) if(Text(V->AsObject(),TEXT("branch"))==Id)
        { Learned+=G->Skills.FindRef(FName(*Text(V->AsObject(),TEXT("id")))); Total+=Number(V->AsObject(),TEXT("maxRank")); }
        Element(TEXT("text"),FString::Printf(TEXT("%d / %d"),Learned,Total),FVector2D(366+I*245,794),FVector2D(180,30),17);
    }
    for(const auto& V:Rows(TEXT("skills")))
    {
        const auto R=V->AsObject(); const FString Id=Text(R,TEXT("id")); const FVector2D P(Number(R,TEXT("x")),Number(R,TEXT("y")));
        const auto Parent=Find(TEXT("skills"),Text(R,TEXT("requires")));
        if(Parent)
        {
            const FVector2D Start(Number(Parent,TEXT("x"))+30,Number(Parent,TEXT("y"))+60);
            Element(TEXT("connection"),TEXT(""),Start,P+FVector2D(30,0)-Start);
            Elements.Last().Color=Color(G->Skills.FindRef(FName(*Text(Parent,TEXT("id"))))>0?TEXT("teal"):TEXT("bronze"));
        }
        const int32 LearnedRank=G->Skills.FindRef(FName(*Id));
        const FString LearnedArt=Text(R,TEXT("learnedIcon"));
        Element(TEXT("image"),TEXT(""),P,FVector2D(60,60),18,TEXT(""),LearnedRank>0 && !LearnedArt.IsEmpty()?LearnedArt:Id+TEXT("Icon"));
        Element(TEXT("node"),TEXT(""),P,FVector2D(60,60),13,TEXT("skill:")+Id,TEXT(""),SelectedSkill==FName(*Id));
        Elements.Last().Value=LearnedRank;
    }
    const auto R=Find(TEXT("skills"),SelectedSkill.ToString()); if(!R) return;
    Element(TEXT("text"),Text(R,TEXT("name")),FVector2D(1284,306),FVector2D(285,42),27);
    Element(TEXT("text"),FString::Printf(TEXT("%d / %d"),G->Skills.FindRef(SelectedSkill),int32(Number(R,TEXT("maxRank")))),FVector2D(1578,313),FVector2D(70,35),18);
    const bool Active=Text(R,TEXT("effect"))==TEXT("heavyAttack");
    Element(TEXT("text"),Text(R,TEXT("branchName"))+TEXT(" · ")+(Active?Text(R,TEXT("activation")):TEXT("成长 · 被动")),FVector2D(1291,352),FVector2D(330,35),18);
    Element(TEXT("text"),Text(R,TEXT("description")),FVector2D(1284,389),FVector2D(340,95),18);
    const int32 Rank=G->Skills.FindRef(SelectedSkill);
    for(int32 I=1;I<=Number(R,TEXT("maxRank"));++I)
    {
        const float Y=470+(I-1)*88;
        Element(TEXT("text"),FString::Printf(TEXT("◇  等级 %d  %s"),I,I<=Rank?TEXT("已学习"):I==Rank+1?TEXT("下一等级"):TEXT("未解锁")),FVector2D(1295,Y),FVector2D(320,30),18);
        const FString Detail=Active?FString::Printf(TEXT("%.0f%% 武器伤害 · %.0f%% 几率\n使敌人失衡 %.1f 秒"),R->GetArrayField(TEXT("multipliers"))[I-1]->AsNumber()*100,R->GetArrayField(TEXT("stunChance"))[I-1]->AsNumber()*100,R->GetArrayField(TEXT("stunSeconds"))[I-1]->AsNumber()):FString::Printf(TEXT("累计增益 %.0f%s"),I*Number(R,TEXT("amount"))*(Number(R,TEXT("amount"))<1?100:1),Number(R,TEXT("amount"))<1?TEXT("%"):TEXT("点"));
        Element(TEXT("text"),Detail,FVector2D(1340,Y+30),FVector2D(280,50),16);
    }
    Element(TEXT("text"),FString::Printf(TEXT("所需技能点 %.0f"),Number(R,TEXT("cost"))),FVector2D(1414,726),FVector2D(225,28),17);
    Element(TEXT("choice"),TEXT("学习技能"),FVector2D(1332,759),FVector2D(228,40),21,TEXT("learn")); Elements.Last().TextInset=64;
    Element(TEXT("text"),TEXT("F 学习 / Enter 确认"),FVector2D(1400,815),FVector2D(230,30),15);
}
void UHearthwardScreenWidget::ComposeMap()
{
    auto* G=Gameplay();
    const bool Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Active();
    const double Extent=Campaign?403200:6000;
    const FVector2D Center(962,475),MapSize(1110,760);
    const auto MapPoint=[&](FVector World){ return Center+FVector2D(World.X,-World.Y)*FVector2D(MapSize.X/Extent,MapSize.Y/Extent)*MapZoom+MapPan; };
    const int32 FirstMapElement=Elements.Num();
    Element(TEXT("image"),TEXT(""),Center-MapSize*.5*MapZoom+MapPan,MapSize*MapZoom,18,TEXT(""),TEXT("mapTerrain"));
    const float FogRadius=Number(Catalog()->GetObjectField(TEXT("tuning")),TEXT("fogRadius"));
    const FVector Origin=Campaign?FVector::ZeroVector:G->LocationPosition(TEXT("camp"));
    const int32 Cell=Extent/20;
    for(int32 X=-Extent/2;X<Extent/2;X+=Cell)
        for(int32 Y=-Extent/2;Y<Extent/2;Y+=Cell)
        {
            const FVector2D World(Origin.X+X+Cell/2,Origin.Y+Y+Cell/2);
            if(G->Explored.ContainsByPredicate([&](FVector2D Seen){ return FVector2D::Distance(World,Seen)<=FMath::Max(double(FogRadius),Cell*.72); })) continue;
            Element(TEXT("fog"),TEXT(""),MapPoint(FVector(X,Y+Cell,0)),FVector2D(55.5,38)*MapZoom+FVector2D(1,1));
        }
    if(Campaign && G->QuestAvailable(TEXT("main_05")))
    {
        const auto& Route=Catalog()->GetObjectField(TEXT("campaign"))->GetArrayField(TEXT("route_trace"));
        for(int32 I=0;I<Route.Num();I+=3)
        {
            const auto& P=Route[I]->AsArray();
            Element(TEXT("text"),TEXT("·"),MapPoint(FVector(P[0]->AsNumber()*100,P[1]->AsNumber()*100,0))-FVector2D(4,10),FVector2D(12,20),18);
            Elements.Last().Color=Color(TEXT("gold"));
        }
    }
    for(const auto& V:Rows(TEXT("locations")))
    {
        const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id"))); if(!G->Discovered.Contains(Id)) continue;
        if(Category==TEXT("travel") && Text(R,TEXT("kind"))==TEXT("landmark")) continue;
        if(Campaign && !GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->HasLocation(Id))continue;
        const FVector P=Campaign?G->LocationPosition(Id):Position(R); const FVector2D UI=MapPoint(P)-FVector2D(27,27);
        Element(TEXT("image"),TEXT(""),UI+FVector2D(10,9),FVector2D(34,36),18,TEXT(""),G->Activated.Contains(Id)?TEXT("mapTravelIcon"):Text(R,TEXT("kind"))==TEXT("landmark")?TEXT("mapLandmarkIcon"):TEXT("mapCampIcon"));
        Element(TEXT("tab"),TEXT(""),UI,FVector2D(54,54),32,TEXT("location:")+Id.ToString(),TEXT(""),SelectedLocation==Id);
        Element(TEXT("text"),Text(R,TEXT("name")),UI+FVector2D(-27,58),FVector2D(200,35),19);
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
    if(!G->TrackedQuest.IsNone())
    {
        const auto Q=Find(TEXT("quests"),G->TrackedQuest.ToString());
        Element(TEXT("text"),TEXT("◎ ")+Text(Q,TEXT("objective")),FVector2D(535,816),FVector2D(910,42),19);
        const auto Target=Find(TEXT("locations"),Campaign?GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->QuestLocation(G->TrackedQuest).ToString():Text(Q,TEXT("location")));
        if(Target)
        {
            Element(TEXT("image"),TEXT(""),MapPoint(Campaign?G->LocationPosition(FName(*Text(Target,TEXT("id")))):Position(Target))+FVector2D(25,-40),FVector2D(33,40),18,TEXT(""),TEXT("mapQuestIcon")); Elements.Last().MapClipped=true;
        }
    }
}
void UHearthwardScreenWidget::ComposeJournal()
{
    if(Category!=TEXT("main") && Category!=TEXT("side")) { ComposeCodex(); return; }
    auto* G=Gameplay();
    TArray<FName> Visible,Timeline;
    for(const auto& V:Rows(TEXT("quests")))
    {
        const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id")));
        if(Text(R,TEXT("kind"))==Category && (bool(HearthwardCampaign::Find(TEXT("quests"),Id))==GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Active()))
        {
            Timeline.Add(Id);
            if(G->QuestAvailable(Id)) Visible.Add(Id);
        }
    }
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Timeline.Num()-5));
    if(!Visible.Contains(SelectedQuest)) SelectedQuest=Visible.IsEmpty()?NAME_None:Visible[0];
    const auto Q=Find(TEXT("quests"),SelectedQuest.ToString());
    if(!Q) { Element(TEXT("text"),TEXT("暂无可显示任务"),FVector2D(408,310),FVector2D(550,60),24); return; }
    Element(TEXT("text"),Category==TEXT("main")?TEXT("◇ 主线任务"):TEXT("◇ 支线任务"),FVector2D(398,157),FVector2D(400,30),17);
    Element(TEXT("text"),Text(Q,TEXT("name")),FVector2D(398,192),FVector2D(700,70),40);
    Element(TEXT("text"),Text(Q,TEXT("description")),FVector2D(398,307),FVector2D(420,210),18);
    Element(TEXT("text"),Text(Q,TEXT("companion")),FVector2D(398,425),FVector2D(352,96),17); Elements.Last().Color=Color(TEXT("muted"));
    const auto Location=Find(TEXT("locations"),Text(Q,TEXT("location")));
    Element(TEXT("text"),TEXT("相关地点"),FVector2D(1285,175),FVector2D(260,35),19);
    Element(TEXT("button"),Text(Location,TEXT("name")),FVector2D(1276,320),FVector2D(300,38),20,TEXT("questMap"));
    Element(TEXT("text"),Text(Location,TEXT("description")),FVector2D(1298,359),FVector2D(280,52),15);
    Element(TEXT("text"),TEXT("当前目标"),FVector2D(425,574),FVector2D(340,40),20);
    Element(TEXT("text"),TEXT("◇  ")+Text(Q,TEXT("objective")),FVector2D(407,621),FVector2D(815,105),17); Elements.Last().Color=Color(TEXT("gold"));
    Element(TEXT("text"),FString::Printf(TEXT("进度 %d / %.0f"),G->QuestProgress(SelectedQuest),Number(Q,TEXT("required"))),FVector2D(443,695),FVector2D(400,30),15);
    Element(TEXT("text"),TEXT("任务进度"),FVector2D(425,720),FVector2D(340,35),20);
    const int32 Count=FMath::Min(5,Timeline.Num()-Scroll);
    if(Count>1) { Element(TEXT("line"),TEXT(""),FVector2D(452,785),FVector2D((Count-1)*171,1)); Elements.Last().Color=Color(TEXT("bronze")); }
    for(int32 N=0;N<Count;++N)
    {
        const FName Id=Timeline[Scroll+N]; const auto R=Find(TEXT("quests"),Id.ToString());
        const bool Available=Visible.Contains(Id);
        const FVector2D P(431+N*171,764);
        Element(TEXT("image"),TEXT(""),P,FVector2D(44,44),18,TEXT(""),Available?TEXT("questMilestoneActive"):TEXT("questMilestoneLocked"));
        Element(TEXT("tab"),TEXT(""),P,FVector2D(44,44),18,Available?TEXT("quest:")+Id.ToString():TEXT(""),TEXT(""),Id==SelectedQuest);
        Element(TEXT("text"),Available?Text(R,TEXT("name")):TEXT("???"),P+FVector2D(-50,50),FVector2D(144,30),15); Elements.Last().Align=TEXT("center");
    }
    Element(TEXT("text"),FString::Printf(TEXT("经验值\n+%.0f"),Number(Q,TEXT("xp"))),FVector2D(1290,798),FVector2D(135,70),16);
    Element(TEXT("choice"),G->Claimed.Contains(SelectedQuest)?TEXT("已完成"):(SelectedQuest==TEXT("side_06") || SelectedQuest==TEXT("side_09"))?TEXT("交付 6 份并领奖"):TEXT("领取奖励"),FVector2D(1403,740),FVector2D(180,46),18,TEXT("claim"));
    Element(TEXT("choice"),G->TrackedQuest==SelectedQuest?TEXT("取消追踪"):TEXT("追踪任务"),FVector2D(1403,805),FVector2D(180,46),18,TEXT("track"));
}
void UHearthwardScreenWidget::ComposeCodex()
{
    const auto* G=Gameplay(); TArray<TSharedPtr<FJsonObject>> Entries;
    const bool Collection=Category==TEXT("collection");
    for(const auto& V:Rows(Collection?TEXT("items"):TEXT("codex")))
        if(Collection || Text(V->AsObject(),TEXT("category"))==Category) Entries.Add(V->AsObject());
    const auto Unlocked=[&](const TSharedPtr<FJsonObject>& R)
    {
        if(Collection) return G->Events.FindRef(FName(*(TEXT("collected:")+Text(R,TEXT("id")))))>0;
        const FString Location=Text(R,TEXT("location")),Event=Text(R,TEXT("event")),Quest=Text(R,TEXT("quest"));
        return (!Location.IsEmpty() && G->Discovered.Contains(FName(*Location))) || (!Event.IsEmpty() && G->Events.FindRef(FName(*Event))>0) || (!Quest.IsEmpty() && G->Claimed.Contains(FName(*Quest)));
    };
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Entries.Num()-5));
    if(!Entries.ContainsByPredicate([&](const auto& R){return FName(*Text(R,TEXT("id")))==SelectedCodex;})) SelectedCodex=Entries.IsEmpty()?NAME_None:FName(*Text(Entries[0],TEXT("id")));
    const FString Heading=Category==TEXT("world")?TEXT("世界见闻"):Category==TEXT("people")?TEXT("人物档案"):Category==TEXT("factions")?TEXT("势力阵营"):TEXT("收集要素");
    Element(TEXT("text"),Heading,FVector2D(398,192),FVector2D(700,70),40);
    int32 Known=0; for(const auto& R:Entries) if(Unlocked(R)) ++Known;
    Element(TEXT("text"),FString::Printf(TEXT("已记录 %d / %d"),Known,Entries.Num()),FVector2D(398,300),FVector2D(500,35),18);
    const auto* Selected=Entries.FindByPredicate([&](const auto& R){return FName(*Text(R,TEXT("id")))==SelectedCodex;});
    if(Selected)
    {
        const bool KnownEntry=Unlocked(*Selected);
        Element(TEXT("text"),KnownEntry?Text(*Selected,TEXT("name")):TEXT("尚未发现"),FVector2D(398,350),FVector2D(470,50),27); Elements.Last().Color=Color(TEXT("gold"));
        Element(TEXT("text"),KnownEntry?Text(*Selected,TEXT("description")):Collection?TEXT("获得这件物品后记录在此。"):TEXT("通过探索、交流与委托，\n逐步了解这段旅途。"),FVector2D(398,408),FVector2D(420,170),18);
        if(KnownEntry)
        {
            Element(TEXT("image"),TEXT(""),FVector2D(1280,726),FVector2D(90,90),18,TEXT(""),Text(*Selected,TEXT("icon")));
            if(Collection) Element(TEXT("text"),FString::Printf(TEXT("当前持有\n%d 件"),Inventory()->GetItemCount(SelectedCodex)),FVector2D(1404,745),FVector2D(160,80),18);
            else Element(TEXT("text"),TEXT("已收录"),FVector2D(1404,745),FVector2D(160,80),18);
        }
    }
    Element(TEXT("text"),TEXT("旅途记录"),FVector2D(425,574),FVector2D(340,40),20);
    for(int32 I=Scroll;I<FMath::Min(Entries.Num(),Scroll+5);++I)
    {
        const auto R=Entries[I]; const FString Id=Text(R,TEXT("id"));
        Element(TEXT("button"),Unlocked(R)?Text(R,TEXT("name")):TEXT("未知条目"),FVector2D(410,620+(I-Scroll)*44),FVector2D(730,40),18,TEXT("codex:")+Id,TEXT(""),SelectedCodex==FName(*Id));
    }
}
void UHearthwardScreenWidget::ComposeDialogue()
{
    const auto* AI=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();
    const FString CardText=AI->GetCandidateText();const bool ShowCard=!CardText.IsEmpty();
    Element(TEXT("choice"),TEXT("记忆与约定"),FVector2D(1405,240),FVector2D(150,36),16,TEXT("page:memory"));
    Elements.Last().Component=TEXT("dialogue.panel");
    if(AI->GetClarificationTurns()>0)
    {
        Element(TEXT("choice"),TEXT("结束本次澄清"),FVector2D(1370,160),FVector2D(185,36),16,TEXT("clearClarification"));
        Elements.Last().Component=TEXT("dialogue.panel");
    }
    FString Reply=AI->CanDisplay()?AI->GetNPCLine():FString();
    if(Reply.IsEmpty()) Reply=TEXT("我在这里。有什么需要一起做的？");
    if(!ShowCard)Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,FMath::DivideAndRoundUp(Reply.Len(),23)-4));
    const int32 ReplyScroll=ShowCard?0:Scroll;
    TArray<FString> Lines; for(int32 I=ReplyScroll*23;I<FMath::Min(Reply.Len(),(ReplyScroll+4)*23);I+=23) Lines.Add(Reply.Mid(I,23));
    Element(TEXT("text"),FString::Join(Lines,TEXT("\n")),FVector2D(993,362),FVector2D(510,128),20);
    if(!ShowCard)
    {
        Element(TEXT("choice"),TEXT("刷新建议"),FVector2D(1260,240),FVector2D(140,36),16,TEXT("suggestRefresh"));
        Elements.Last().Component=TEXT("dialogue.panel");
        const auto Suggestions=AI->GetSuggestions();
        const auto& Slots=Theme->GetArrayField(TEXT("dialogueChoices"));
        if(Suggestions.IsEmpty())
        {
            Element(TEXT("text"),TEXT("建议不会自动刷新。点击上方按钮后生成；未选择的内容不会传给弟弟。"),
                FVector2D(968,505),FVector2D(518,100),17);
            Elements.Last().Component=TEXT("dialogue.panel");
        }
        for(int32 I=0;I<FMath::Min(3,Suggestions.Num()) && Slots.IsValidIndex(I);++I)
        {
            const auto Choice=Slots[I]->AsObject(); const auto& Rect=Choice->GetArrayField(TEXT("rect"));
            const FVector2D P(Rect[0]->AsNumber(),Rect[1]->AsNumber());
            Element(TEXT("choice"),Suggestions[I].Label,P,FVector2D(Rect[2]->AsNumber(),Rect[3]->AsNumber()),19,
                TEXT("suggest:")+Suggestions[I].Id.ToString());
            Elements.Last().TextInset=76;
            Element(TEXT("image"),TEXT(""),P+FVector2D(23,11),FVector2D(32,33),18,TEXT(""),Text(Choice,TEXT("icon")));
        }
    }
    if(ShowCard)
    {
        TArray<FString> Paragraphs,CardLines;CardText.ParseIntoArrayLines(Paragraphs,false);
        for(const auto& P:Paragraphs)for(int32 I=0;I<P.Len();I+=30)CardLines.Add(P.Mid(I,30));
        Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,CardLines.Num()-7));TArray<FString> Visible;
        for(int32 I=Scroll;I<FMath::Min(Scroll+7,CardLines.Num());++I)Visible.Add(CardLines[I]);
        Element(TEXT("text"),FString::Join(Visible,TEXT("\n")),FVector2D(975,495),FVector2D(590,174),16);Elements.Last().Component=TEXT("dialogue.panel");
        if(CardLines.Num()>7){Element(TEXT("text"),TEXT("滚轮查看完整任务卡"),FVector2D(1150,675),FVector2D(400,36),14);Elements.Last().Component=TEXT("dialogue.panel");}
    }
    if(AI->HasCandidate())
    {
        const FString Id=AI->GetCandidateId().ToString();
        Element(TEXT("choice"),TEXT("−"),FVector2D(975,675),FVector2D(65,36),19,TEXT("agentLess:")+Id);Elements.Last().Component=TEXT("dialogue.panel");
        Element(TEXT("choice"),TEXT("+"),FVector2D(1050,675),FVector2D(65,36),19,TEXT("agentMore:")+Id);Elements.Last().Component=TEXT("dialogue.panel");
        Element(TEXT("choice"),TEXT("确认这项任务"),FVector2D(975,716),FVector2D(285,42),19,TEXT("agentConfirm:")+Id);Elements.Last().Component=TEXT("dialogue.panel");
        Element(TEXT("choice"),TEXT("放弃提案"),FVector2D(1280,716),FVector2D(275,42),19,TEXT("cancelReply"));Elements.Last().Component=TEXT("dialogue.panel");
    }
    TArray<const FHearthwardAgentCapability*> Caps;
    for(const auto& C:HearthwardAgent::Capabilities())
        if(C.Id==TEXT("collect") || C.Id==TEXT("store") || C.Id==TEXT("retrieve") || C.Id==TEXT("give") || C.Id==TEXT("fetch") || C.Id==TEXT("receive") || C.Id==TEXT("craft") || C.Id==TEXT("repair") || C.Id==TEXT("escort")) Caps.Add(&C);
    AgentCapabilityIndex=FMath::Clamp(AgentCapabilityIndex,0,Caps.Num()-1);
    AgentItemIndex=FMath::Clamp(AgentItemIndex,0,Caps[AgentCapabilityIndex]->Items.Num()-1);
    const auto& Cap=*Caps[AgentCapabilityIndex];
    Element(TEXT("choice"),Cap.Id==TEXT("craft")?TEXT("制作"):Cap.Id==TEXT("repair")?TEXT("维修"):Cap.Id==TEXT("store")?TEXT("入库"):Cap.Id==TEXT("retrieve")?TEXT("仓库交付"):Cap.Id==TEXT("give")?TEXT("弟弟交付"):Cap.Id==TEXT("fetch")?TEXT("取入弟弟背包"):Cap.Id==TEXT("receive")?TEXT("玩家交给弟弟"):TEXT("采集"),FVector2D(955,240),FVector2D(140,36),16,TEXT("agentTypeNext"));Elements.Last().Component=TEXT("dialogue.panel");
    Element(TEXT("choice"),HearthwardAgent::ItemText(Cap.Items[AgentItemIndex]),FVector2D(1100,240),FVector2D(150,36),16,TEXT("agentItemNext"));Elements.Last().Component=TEXT("dialogue.panel");
    if(Cap.Id==TEXT("store"))
    {
        AgentSourceIndex=FMath::Clamp(AgentSourceIndex,0,Cap.Sources.Num()-1);
        Element(TEXT("choice"),Cap.Sources[AgentSourceIndex]==TEXT("player_bag")?TEXT("来源：玩家背包（需3米内）"):TEXT("来源：弟弟背包"),
            FVector2D(955,285),FVector2D(330,36),15,TEXT("agentSourceNext"));Elements.Last().Component=TEXT("dialogue.panel");
    }
    if(Cap.Id==TEXT("repair"))
    {
        TArray<FHearthwardItemInstance> Instances;
        for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        {
            for(const auto& Instance:It->Bag->Snapshot().Instances)
                if(Instance.Definition==Cap.Items[AgentItemIndex])Instances.Add(Instance);
            break;
        }
        FString Label=TEXT("弟弟没有这件装备");
        if(!Instances.IsEmpty())
        {
            AgentInstanceIndex=FMath::Clamp(AgentInstanceIndex,0,Instances.Num()-1);
            const auto& Selected=Instances[AgentInstanceIndex];
            Label=FString::Printf(TEXT("装备实例 %d/%d · 耐久 %.0f · %s"),AgentInstanceIndex+1,Instances.Num(),
                Selected.Durability,*Selected.Id.ToString().Left(8));
        }
        Element(TEXT("choice"),Label,FVector2D(955,285),FVector2D(300,36),15,TEXT("agentInstanceNext"));Elements.Last().Component=TEXT("dialogue.panel");
    }
    Element(TEXT("choice"),TEXT("新建手动任务卡"),FVector2D(955,200),FVector2D(190,36),16,TEXT("agentCollectCard"));Elements.Last().Component=TEXT("dialogue.panel");
    Element(TEXT("choice"),TEXT("查看木材库存"),FVector2D(1150,200),FVector2D(210,36),16,TEXT("agentInventory"));Elements.Last().Component=TEXT("dialogue.panel");
    Element(TEXT("choice"),TEXT("重试返营"),FVector2D(1370,200),FVector2D(185,36),16,TEXT("agentRetryPath"));Elements.Last().Component=TEXT("dialogue.panel");
    FString Status=AI->GetStatus();if(AI->IsBusy())Status+=FString::Printf(TEXT(" · %.1f秒"),AI->GetElapsedSeconds());
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        if(!AI->IsBusy() && !AI->HasCandidate() && !It->BlockReason.IsEmpty()
            && It->GetPhase()!=EHearthwardCompanionPhase::Idle
            && It->GetPhase()!=EHearthwardCompanionPhase::Completed
            && It->GetPhase()!=EHearthwardCompanionPhase::Cancelled)
        { Status=TEXT("委托受阻：")+It->BlockReason; break; }
    Element(TEXT("text"),Status,FVector2D(965,824),FVector2D(620,30),15);
    Element(TEXT("choice"),AI->IsBusy()?TEXT("取消回复"):TEXT("发送"),FVector2D(1484,766),FVector2D(105,51),19,AI->IsBusy()?TEXT("cancelReply"):TEXT("send"));
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        if(It->CanCommunicate(GetOwningPlayerPawn()))
            Element(TEXT("text"),TEXT("T 与弟弟交流 / 下达委托"),FVector2D(1310,650),FVector2D(330,40),18);
        if(It->GetRequested()>0)
        {
            const FString Progress=It->GetGoal().Intent==TEXT("repair")?FString::Printf(TEXT("维修%s：完成%d/%d"),*HearthwardAgent::ItemText(It->GetGoal().Item),It->GetDelivered(),It->GetRequested()):FString::Printf(TEXT("%s：取得%d · 携带%d · 完成%d/%d"),*HearthwardAgent::ItemText(It->GetGoal().Item),It->GetAcquired(),It->GetCarried(),It->GetDelivered(),It->GetRequested());
            Element(TEXT("text"),Progress,FVector2D(960,858),FVector2D(440,32),15);
            Element(TEXT("choice"),TEXT("取消委托"),FVector2D(1420,854),FVector2D(168,39),17,TEXT("cancelTask"));
        }
        break;
    }
}
void UHearthwardScreenWidget::ComposeMemory()
{
    const auto* AI=GetWorld()->GetSubsystem<UHearthwardLocalAISubsystem>();
    TArray<FHearthwardPlayerMemory> Active;
    for(const auto& R:AI->GetPlayerMemories()) if(!R.Revoked) Active.Add(R);
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Active.Num()-6));
    auto Label=[](FName Kind){return Kind==TEXT("typed_constraint")?TEXT("已确认规则"):Kind==TEXT("collection_ban")?TEXT("采集限制"):Kind==TEXT("agreement")?TEXT("文字约定"):Kind==TEXT("preference")?TEXT("偏好"):TEXT("陈述");};
    for(int32 I=Scroll;I<FMath::Min(Active.Num(),Scroll+6);++I)
    {
        const auto& R=Active[I];
        Element(TEXT("button"),FString::Printf(TEXT("[%s] %s"),Label(R.Kind),*R.Text.Left(15)),FVector2D(315,285+(I-Scroll)*62),FVector2D(390,54),18,TEXT("memorySelect:")+R.Id.ToString(),TEXT(""),R.Id==SelectedMemory);
        Elements.Last().Component=TEXT("memory.list");
    }
    const auto* Selected=Active.FindByPredicate([&](const auto& R){return R.Id==SelectedMemory;});
    FString Detail=Selected?Selected->Text:TEXT("选择左侧记录可修改或撤销。采集限制会阻止对应的新委托；文字约定供交流参考。陈述仅代表你说过，无法改写事实或允许危险行动。");
    TArray<FString> Lines; for(int32 I=0;I<Detail.Len();I+=25) Lines.Add(Detail.Mid(I,25));
    Element(TEXT("text"),FString::Join(Lines,TEXT("\n")),FVector2D(795,255),FVector2D(570,200),20); Elements.Last().Component=TEXT("memory.details");
    if(Selected)
    {
        Element(TEXT("text"),FString::Printf(TEXT("来源：你的记录 · 记录于 %.0f 秒"),Selected->RecordedAt),FVector2D(795,448),FVector2D(560,30),17); Elements.Last().Component=TEXT("memory.details");
    }
    const FName Kinds[]={TEXT("claim"),TEXT("preference"),TEXT("agreement"),TEXT("collection_ban")};
    for(int32 I=0;I<4;++I)
    {
        Element(TEXT("button"),Label(Kinds[I]),FVector2D(785+I*146,490),FVector2D(142,45),18,TEXT("memoryKind:")+Kinds[I].ToString(),TEXT(""),MemoryKind==Kinds[I]); Elements.Last().Component=TEXT("memory.details");
    }
    if(MemoryKind==TEXT("collection_ban"))
    {
        const auto* Item=HearthwardBasicItems().FindByPredicate([&](const auto& I){return I.Id==MemoryBlockedItem;});
        Element(TEXT("button"),TEXT("禁止采集：")+(Item?Item->DisplayName.ToString():MemoryBlockedItem.ToString())+TEXT("  · 点击切换物品"),FVector2D(785,535),FVector2D(585,40),18,TEXT("memoryNextItem")); Elements.Last().Component=TEXT("memory.details");
    }
    Element(TEXT("button"),Selected?TEXT("保存修改"):TEXT("记下这条"),FVector2D(785,650),FVector2D(270,50),21,TEXT("memorySave")); Elements.Last().Component=TEXT("memory.details");
    Element(TEXT("button"),TEXT("撤销所选记录"),FVector2D(1090,650),FVector2D(280,50),21,TEXT("memoryRevoke")); Elements.Last().Enabled=Selected!=nullptr; Elements.Last().Component=TEXT("memory.details");
    Element(TEXT("button"),TEXT("取消所有任务和约定"),FVector2D(785,717),FVector2D(585,50),20,TEXT("memoryReset")); Elements.Last().Component=TEXT("memory.details");
}
void UHearthwardScreenWidget::ComposeHUD()
{
    auto* G=Gameplay();
    const auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
    const auto* Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
    const bool Natural=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->IsNaturalWorldEnabled();
    const float TextScale=FMath::Clamp(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f,1.f,1.5f);
    auto HUD=[&](const TCHAR* Type,FString Label,FVector2D P,FVector2D Size,float Font,const FString& Id,const TCHAR* Component)
        ->FHearthwardUIElement&
    {
        Element(Type,Label,P,Size,Font);
        auto& E=Elements.Last(); E.Font=Font*TextScale; E.FontRole=TEXT("body"); E.TextInset=0;
        E.LayoutId=Id; E.Component=Component; return E;
    };

    // Vitals precede the quest. Text scaling also increases the vertical spacing.
    const float Values[]={G->Health,G->Hunger,G->Stamina},Maximum[]={G->MaxHealth(),100,G->MaxStamina()};
    const TCHAR* Labels[]={TEXT("生命"),TEXT("饱食"),TEXT("体力")},*Colors[]={TEXT("health"),TEXT("hunger"),TEXT("stamina")};
    for(int32 I=0;I<3;++I)
    {
        const float Y=32+I*35*TextScale,BarX=48+90*TextScale,BarWidth=168*TextScale;
        const FString Id=FString::Printf(TEXT("hud.vitals.%d"),I);
        HUD(TEXT("text"),Labels[I],{48,Y},{84*TextScale,30*TextScale},20,Id+TEXT(".label"),TEXT("hud.vitals")).Color=Color(Colors[I]);
        auto& Bar=HUD(TEXT("bar"),TEXT(""),{BarX,Y+11*TextScale},{BarWidth,11},18,Id+TEXT(".bar"),TEXT("hud.vitals"));
        Bar.Color=Color(Colors[I]); Bar.Value=Maximum[I]>0?Values[I]/Maximum[I]:0;
        HUD(TEXT("text"),FString::Printf(TEXT("%.0f / %.0f"),Values[I],Maximum[I]),{BarX+BarWidth+14,Y},{180,30*TextScale},18,Id+TEXT(".value"),TEXT("hud.vitals"));
    }
    const float QuestY=62+105*TextScale,DescriptionY=QuestY+45*TextScale,DescriptionHeight=96*TextScale;
    const auto Quest=Find(TEXT("quests"),G->TrackedQuest.ToString());
    FString Heading=TEXT("◇  ")+Text(Quest,TEXT("name")),Objective=Text(Quest,TEXT("objective"));
    if(Natural && !Campaign->Active())
    {
        Heading=TEXT("◇  新营地 · 兄弟同行");
        Objective=FString::Printf(TEXT("距营地 %.0f 米\n%s工作台    %s绳索    %s床"),
            FVector::Dist2D(G->LocationPosition(TEXT("camp")),GetOwningPlayerPawn()->GetActorLocation())/100,
            G->Events.FindRef(TEXT("build:workbench"))>0?TEXT("✓"):TEXT("○"),
            G->Events.FindRef(TEXT("craft:rope"))>0?TEXT("✓"):TEXT("○"),
            G->Events.FindRef(TEXT("build:bed"))>0?TEXT("✓"):TEXT("○"));
    }
    HUD(TEXT("text"),Heading,{48,QuestY},{650,42*TextScale},24,TEXT("hud.quest.heading"),TEXT("hud.quest")).Color=Color(TEXT("gold"));
    HUD(TEXT("text"),Objective,{72,DescriptionY},{650,DescriptionHeight},20,TEXT("hud.quest.objective"),TEXT("hud.quest"));

    const float CompanionY=DescriptionY+DescriptionHeight+24,CompanionTextX=48+106*TextScale;
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        auto& Portrait=HUD(TEXT("portrait"),TEXT(""),{48,CompanionY},{89*TextScale,87*TextScale},18,TEXT("hud.companion.portrait"),TEXT("hud.companion"));
        Portrait.Asset=TEXT("portrait");
        HUD(TEXT("text"),TEXT("弟弟"),{CompanionTextX,CompanionY+4},{370,32*TextScale},22,TEXT("hud.companion.name"),TEXT("hud.companion"));
        FString RoutineLabel;
        const FName RoutineActivity=G->GetCompanionRoutineActivity();
        if(RoutineActivity==TEXT("patrol")) RoutineLabel=TEXT("巡营");
        else if(RoutineActivity==TEXT("check_camp")) RoutineLabel=TEXT("查看营地");
        else if(RoutineActivity==TEXT("return_camp")) RoutineLabel=TEXT("回营");
        else if(RoutineActivity==TEXT("rest")) RoutineLabel=TEXT("休息");
        const FString Order=G->CompanionOrder==TEXT("follow")?TEXT("跟随中"):G->CompanionOrder==TEXT("attack")?TEXT("协助进攻")
            :It->GetRequested()>0?FString::Printf(TEXT("委托 %d / %d"),It->GetDelivered(),It->GetRequested())
            :G->IsCompanionRoutineEnabled()?TEXT("自由活动")+(!RoutineLabel.IsEmpty()?TEXT(" · ")+RoutineLabel:TEXT("")):TEXT("原地等待");
        const float Required=Number(Quest,TEXT("required"));
        HUD(TEXT("text"),FString::Printf(TEXT("进度 %d / %.0f"),G->QuestProgress(G->TrackedQuest),Required),{CompanionTextX,CompanionY+37*TextScale},{370,30*TextScale},18,TEXT("hud.companion.progress"),TEXT("hud.companion"));
        auto& Progress=HUD(TEXT("bar"),TEXT(""),{CompanionTextX,CompanionY+69*TextScale},{168*TextScale,7},18,TEXT("hud.companion.bar"),TEXT("hud.companion"));
        Progress.Color=Color(TEXT("bronze")); Progress.Value=It->GetRequested()>0?float(It->GetDelivered())/It->GetRequested():Required>0?G->QuestProgress(G->TrackedQuest)/Required:0;
        HUD(TEXT("text"),Order,{CompanionTextX,CompanionY+81*TextScale},{390,32*TextScale},18,TEXT("hud.companion.order"),TEXT("hud.companion"));
        break;
    }

    const auto& Slots=Theme->GetObjectField(TEXT("inventory"))->GetArrayField(TEXT("quickSlots"));
    HUDQuickSelection=FMath::Clamp(HUDQuickSelection,0,FMath::Max(0,Slots.Num()-1));
    // Selected item is always below; the following three circulate left, above and right.
    const FVector2D Positions[]={FVector2D(158,784),FVector2D(57,707),FVector2D(170,633),FVector2D(283,707)};
    for(int32 Offset=0;Offset<Slots.Num() && Offset<4;++Offset)
    {
        const int32 Index=(HUDQuickSelection+Offset)%Slots.Num(); const FString ItemId=Slots[Index]->AsString();
        const auto Item=Find(TEXT("items"),ItemId); const bool Selected=Offset==0;
        const FString Art=Text(Item,TEXT("quickIcon"));
        auto& E=HUD(TEXT("hudQuickItem"),FString::FromInt(Inventory()->GetItemCount(FName(*ItemId))),Positions[Offset],Selected?FVector2D(104,112):FVector2D(80,88),20,TEXT("hud.quick.")+ItemId,TEXT("hud.quickslots"));
        E.Id=ItemId; E.Asset=Art.IsEmpty()?Text(Item,TEXT("icon")):Art; E.Selected=Selected;
        E.Color=Selected?FLinearColor(1.f,.96f,.88f,1.f):FLinearColor(.58f,.56f,.53f,1.f);
        if(Selected) HUD(TEXT("text"),Text(Item,TEXT("name")),{72,902},{276,39},20,TEXT("hud.quick.name"),TEXT("hud.quickslots")).Align=TEXT("center");
    }

    if(!Natural)
    {
        const auto Mini=Theme->GetObjectField(TEXT("minimap")); const auto& Rect=Mini->GetArrayField(TEXT("rect"));
        const FVector2D MiniPosition(Rect[0]->AsNumber(),Rect[1]->AsNumber()),MiniSize(Rect[2]->AsNumber(),Rect[3]->AsNumber()),MiniCenter=MiniPosition+MiniSize*.5;
        Element(TEXT("minimap"),TEXT(""),MiniPosition,MiniSize,18,TEXT(""),TEXT("mapTerrain"));
        const FVector Origin=G->LocationPosition(TEXT("camp")); const float WorldSize=Number(Mini,TEXT("worldSize"));
        const auto ProjectMini=[&](FVector P){const FVector Local=P-Origin; return MiniCenter+FVector2D(Local.X,-Local.Y)*(MiniSize.X/WorldSize);};
        for(const auto& Entry:Rows(TEXT("locations")))
        {
            const auto R=Entry->AsObject(); const FName Id(*Text(R,TEXT("id"))); if(!G->Discovered.Contains(Id)) continue;
            const FVector2D P=ProjectMini(G->LocationPosition(Id)); if((P-MiniCenter).Size()>MiniSize.X*.46) continue;
            Element(TEXT("text"),G->Activated.Contains(Id)?TEXT("♧"):TEXT("◇"),P-FVector2D(9,12),FVector2D(25,30),22); Elements.Last().Color=Color(TEXT("teal"));
        }
        const FVector2D PlayerPoint=ProjectMini(GetOwningPlayerPawn()->GetActorLocation());
        if((PlayerPoint-MiniCenter).Size()<MiniSize.X*.47)
        {Element(TEXT("arrow"),TEXT(""),PlayerPoint-FVector2D(10,10),FVector2D(20,20));Elements.Last().Value=GetOwningPlayer()->GetControlRotation().Yaw+90;Elements.Last().Color=Color(TEXT("gold"));}
        Element(TEXT("text"),TEXT("北"),MiniPosition+FVector2D(MiniSize.X*.5-9,4),FVector2D(25,25),15);
    }
    if(G->HasWaypoint) HUD(TEXT("text"),FString::Printf(TEXT("◇  %.0f 米"),FVector::Dist2D(GetOwningPlayerPawn()->GetActorLocation(),G->Waypoint)/100),{1370,267},{230,45},18,TEXT("hud.waypoint"),TEXT("hud.minimap"));
    const int32 Time=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes,Hour=(Time/60)%24;
    HUD(TEXT("text"),FString::Printf(TEXT("第 %d 天  %02d:%02d  %s"),Time/1440+1,Hour,Time%60,Hour<6 || Hour>=18?TEXT("夜"):TEXT("晴")),{1260,31},{364,50},18,TEXT("hud.clock"),TEXT("hud.feedback")).Align=TEXT("right");

    // Only live state and action results remain here, without tutorial panels or key lists.
    float FeedbackY=388; TSet<FString> Seen;
    auto Feedback=[&](FString Label,const TCHAR* Id)
    {
        Label=Label.TrimStartAndEnd(); if(Label.IsEmpty() || Seen.Contains(Label)) return;
        Seen.Add(Label); HUD(TEXT("text"),Label,{1120,FeedbackY},{504,90*TextScale},18,Id,TEXT("hud.feedback")); FeedbackY+=98*TextScale;
    };
    const auto* Combat=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardCombatComponent>();
    if(Combat)
    {
        Feedback(Combat->Describe(),TEXT("hud.combat.state"));
        if(Combat->Discovery>0)
        {auto& Bar=HUD(TEXT("bar"),TEXT(""),{1120,358},{504,8},18,TEXT("hud.combat.discovery"),TEXT("hud.feedback"));Bar.Value=Combat->Discovery;Bar.Color=Color(TEXT("gold"));}
    }
    Feedback(G->Feedback,TEXT("hud.gameplay.feedback"));
    if(const auto* B=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>()) Feedback(B->Feedback,TEXT("hud.construction.feedback"));
    if(const auto* S=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardSurvivalComponent>())
    {
        Feedback(S->Describe(),TEXT("hud.survival.state"));
        if(S->State.Life==EHearthwardLife::Downed) HUD(TEXT("text"),TEXT("等待弟弟救援"),{620,700},{520,45*TextScale},20,TEXT("hud.survival.downed"),TEXT("hud.feedback"));
    }
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        if(const auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>();S && S->State.Life==EHearthwardLife::Downed)
            HUD(TEXT("text"),TEXT("弟弟倒地"),{620,640},{520,45*TextScale},20,TEXT("hud.companion.downed"),TEXT("hud.feedback"));
        break;
    }
    if(const auto* Traversal=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardTraversalComponent>()) Feedback(Traversal->GetStatus(),TEXT("hud.traversal.state"));
    if(const auto* Interaction=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardInteractionComponent>();Interaction && Interaction->GetStatus()==EHearthwardInteractionStatus::Ready)
        Feedback(Interaction->GetCompletionFeedback(),TEXT("hud.interaction.feedback"));
    if(N->Busy())
    {
        HUD(TEXT("text"),N->FishingStatus(),{560,670},{1000,80*TextScale},20,TEXT("hud.fishing.state"),TEXT("hud.feedback"));
        if(N->IsFishing())
        {auto& Bar=HUD(TEXT("bar"),TEXT(""),{620,775},{650,18},18,TEXT("hud.fishing.tension"),TEXT("hud.feedback"));Bar.Value=N->FishingTension();Bar.Color=N->FishingTension()>=.15 && N->FishingTension()<=.85?FLinearColor(.2f,.7f,.3f):FLinearColor(.8f,.2f,.1f);}
    }
    if(const auto* Timer=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardTimedActionComponent>();Timer && Timer->GetStatus()==EHearthwardTimedActionStatus::Running)
    {
        HUD(TEXT("text"),FString::Printf(TEXT("进行中  %.1f / 5.0 秒"),Timer->GetElapsedSeconds()),{636,750},{400,50*TextScale},20,TEXT("hud.action.state"),TEXT("hud.feedback"));
        auto& Bar=HUD(TEXT("bar"),TEXT(""),{654,825},{364,6},18,TEXT("hud.action.progress"),TEXT("hud.feedback"));Bar.Value=Timer->GetElapsedSeconds()/5;Bar.Color=Color(TEXT("gold"));
    }
}

void UHearthwardScreenWidget::ComposeBuilding()
{
    const auto& Buildings=Rows(TEXT("buildings"));
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Buildings.Num()-4));
    int32 Index=0;
    for(const auto& V:Buildings)
    {
        if(Index++<Scroll)continue;if(Index>Scroll+4)break;
        const auto R=V->AsObject(); const float Y=230+(Index-Scroll-1)*115;
        const FString Id=Text(R,TEXT("id"));
        FString Cost;
        for(const auto& M:R->GetObjectField(TEXT("materials"))->Values)
            Cost+=Text(Find(TEXT("items"),FString(*M.Key)),TEXT("name"))+FString::Printf(TEXT(" %d / %.0f  "),Inventory()->Available(FName(*M.Key))+GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Available(FName(*M.Key)),M.Value->AsNumber());
        Element(TEXT("button"),Text(R,TEXT("name")),FVector2D(510,Y),FVector2D(650,55),26,TEXT("build:")+Id);
        Elements.Last().Component=TEXT("building.sheet"); Elements.Last().LayoutId=TEXT("building.")+Id+TEXT(".select");
        Elements.Last().Enabled=HearthwardCamp::RequiredTier(FName(*Id),1)<=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Tier;
        Element(TEXT("text"),Cost+TEXT("\n")+Text(R,TEXT("description")),FVector2D(528,Y+55),FVector2D(660,54),16);
        Elements.Last().Component=TEXT("building.sheet"); Elements.Last().LayoutId=TEXT("building.")+Id+TEXT(".description");
    }
    Element(TEXT("button"),TEXT("上一组"),FVector2D(530,700),FVector2D(270,45),20,TEXT("buildPrev"));
    Element(TEXT("button"),TEXT("下一组"),FVector2D(860,700),FVector2D(280,45),20,TEXT("buildNext"));
}
void UHearthwardScreenWidget::ComposeSave()
{
    auto* S=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>(); const auto Points=S->GetPoints();
    const bool Failed=UHearthwardSurvivalComponent::HasFailed(GetWorld());
    const bool HasProgress=S->GetCampaignId().IsValid();
    const bool FromTitle=!ReturnPages.IsEmpty() && ReturnPages.Last()==TEXT("title");
    ComposeMenuChrome(FromTitle?TEXT("载入存档"):TEXT("存档管理"),Failed?TEXT("兄弟已无法继续，世界已暂停。请选择保存节点回档。"):HasProgress && !FromTitle?S->GetSafetyDescription():TEXT("选择一个保存节点，继续归途。"));
    MenuElement(TEXT("text"),FString::Printf(TEXT("已用存档位  %d / 50"),Points.Num()),{112,168},{620,32},20).Color=Color(TEXT("muted"));
    auto& Save=MenuElement(TEXT("menuAction"),TEXT("保存当前进度"),{1240,158},{320,44},22,TEXT("save"));
    Save.Align=TEXT("right"); Save.Enabled=HasProgress && !Failed;
    MenuElement(TEXT("text"),TEXT("保存节点"),{132,235},{680,31},20).Color=Color(TEXT("muted"));
    MenuElement(TEXT("text"),TEXT("存档类型"),{838,235},{205,31},20).Align=TEXT("center");
    MenuElement(TEXT("text"),TEXT("保护状态"),{1087,235},{253,31},20).Align=TEXT("center");
    MenuElement(TEXT("text"),TEXT("操作"),{1360,235},{184,31},20).Align=TEXT("center");
    MenuElement(TEXT("line"),TEXT(""),{112,274},{1448,1}).Color=Color(TEXT("bronze"));
    const int32 PageSize=MenuPageSize(); const float Height=MenuRowHeight();
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Points.Num()-PageSize));
    if(!Points.IsEmpty())
    {
        const int32 SelectedIndex=Points.IndexOfByPredicate([&](const auto& P){return P.SaveId==SaveSelection;});
        const int32 DisplayIndex=Points.Num()-1-SelectedIndex;
        if(SelectedIndex==INDEX_NONE || DisplayIndex<Scroll || DisplayIndex>=Scroll+PageSize)
            SaveSelection=Points[Points.Num()-1-Scroll].SaveId;
    }
    for(int32 I=Scroll;I<FMath::Min(Points.Num(),Scroll+PageSize);++I)
    {
        const auto& P=Points[Points.Num()-1-I]; const FString Id=P.SaveId.ToString(); const float Y=286+(I-Scroll)*Height;
        const bool Selected=P.SaveId==SaveSelection;
        MenuElement(TEXT("menuRow"),TEXT(""),{112,Y},{1448,Height-1},18,TEXT("ask:load:")+Id,Selected).Id=Id;
        auto& Label=MenuElement(TEXT("text"),FString::Printf(TEXT("%02d    "),I+1)+P.Created.ToString(TEXT("%Y-%m-%d   %H:%M")),{132,Y+8},{680,Height-12},23);
        Label.Id=Id; Label.LayoutId=TEXT("save.label.")+Id; if(Selected) Label.Color=Color(TEXT("gold"));
        auto& Kind=MenuElement(TEXT("menuAction"),P.Manual?TEXT("手动存档"):TEXT("自动存档"),{838,Y},{205,Height-1},22);
        Kind.Align=TEXT("center"); Kind.Id=Id;
        auto& Lock=MenuElement(TEXT("menuAction"),P.Locked?TEXT("已锁定 · 解锁"):TEXT("锁定"),{1087,Y},{253,Height-1},21,TEXT("lock:")+Id);
        Lock.Align=TEXT("center"); Lock.Id=Id;
        auto& Delete=MenuElement(TEXT("menuAction"),TEXT("删除"),{1360,Y},{184,Height-1},21,TEXT("ask:delete:")+Id);
        Delete.Align=TEXT("center"); Delete.Id=Id; Delete.Enabled=!P.Locked;
    }
    if(Points.IsEmpty())
    {
        auto& Empty=MenuElement(TEXT("text"),TEXT("尚无保存节点"),{250,399},{1172,45},29);
        Empty.Align=TEXT("center"); Empty.LayoutId=TEXT("save.empty");
        auto& Hint=MenuElement(TEXT("text"),TEXT("开始新游戏后，可在安全时保存进度。"),{250,465},{1172,35},20);
        Hint.Align=TEXT("center"); Hint.Color=Color(TEXT("muted"));
    }
    else
    {
        MenuElement(TEXT("menuAction"),TEXT("‹  上一页"),{112,690},{180,37},19,TEXT("save.prev")).Enabled=Scroll>0;
        auto& Range=MenuElement(TEXT("text"),FString::Printf(TEXT("%d — %d / %d"),Scroll+1,FMath::Min(Scroll+PageSize,Points.Num()),Points.Num()),{650,697},{372,30},18);
        Range.Align=TEXT("center"); Range.Color=Color(TEXT("muted"));
        MenuElement(TEXT("menuAction"),TEXT("下一页  ›"),{1380,690},{180,37},19,TEXT("save.next")).Enabled=Scroll+PageSize<Points.Num();
    }
    MenuElement(TEXT("line"),TEXT(""),{112,734},{1448,1}).Color=Color(TEXT("bronze"));
    MenuElement(TEXT("text"),TEXT("选择节点载入进度"),{132,748},{1408,32},20).Color=Color(TEXT("gold"));
    MenuElement(TEXT("text"),TEXT("所有进度共享50个存档位。锁定的节点不会被自动存档轮换，解锁后才可删除。"),{132,788},{1408,48},18).Color=Color(TEXT("muted"));
    MenuElement(TEXT("text"),TEXT("滚轮翻阅   /   Enter 确认"),{1000,880},{560,30},18).Align=TEXT("right");
}
