#include "HearthwardScreenWidget.h"
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
        for(const auto& Gear:G->Equipment) if(G->Durability.FindRef(Gear.Value)>0) Defense+=Number(Find(TEXT("items"),Gear.Value.ToString()),TEXT("defense"));
        Element(TEXT("text"),TEXT("攻击力\n伤害减免\n经验加成\n耐力消耗"),FVector2D(1381,528),FVector2D(230,140),18);
        Element(TEXT("text"),FString::Printf(TEXT("%.0f\n%.0f%%\n+%.0f%%\n−%.0f%%"),G->AttackPower(),FMath::Min(85.f,Defense),G->Effect(TEXT("xp"))*100,G->Effect(TEXT("cost"))*100),FVector2D(1519,528),FVector2D(100,140),18); Elements.Last().Align=TEXT("right");
        Element(TEXT("text"),TEXT("负重"),FVector2D(1381,706),FVector2D(110,30),16);
        Element(TEXT("text"),FString::Printf(TEXT("%.1f / %.0f"),Bag->GetWeight(),Bag->GetCapacity()),FVector2D(1480,705),FVector2D(139,30),18); Elements.Last().Align=TEXT("right");
        Element(TEXT("bar"),TEXT(""),FVector2D(1380,730),FVector2D(239,6)); Elements.Last().Value=Bag->GetWeight()/Bag->GetCapacity(); Elements.Last().Color=Color(TEXT("gold"));
        Element(TEXT("text"),TEXT("移动速度"),FVector2D(1381,748),FVector2D(140,30),16);
        Element(TEXT("text"),FString::Printf(TEXT("%.0f%%"),Bag->GetMoveSpeedMultiplier()*100),FVector2D(1519,748),FVector2D(100,30),18); Elements.Last().Align=TEXT("right");
        Element(TEXT("text"),FString::Printf(TEXT("负重   %.1f / %.0f"),Bag->GetWeight(),Bag->GetCapacity()),FVector2D(92,768),FVector2D(282,30),16);
        Element(TEXT("bar"),TEXT(""),FVector2D(64,795),FVector2D(310,8)); Elements.Last().Value=Bag->GetWeight()/Bag->GetCapacity(); Elements.Last().Color=Color(TEXT("gold"));
    }
    const auto R=Find(TEXT("items"),SelectedItem.ToString()); if(!R) return;
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
    const FVector2D Center(962,475),MapSize(1110,760);
    const auto MapPoint=[&](FVector World){ return Center+FVector2D(World.X,-World.Y)*FVector2D(MapSize.X/6000,MapSize.Y/6000)*MapZoom+MapPan; };
    const int32 FirstMapElement=Elements.Num();
    Element(TEXT("image"),TEXT(""),Center-MapSize*.5*MapZoom+MapPan,MapSize*MapZoom,18,TEXT(""),TEXT("mapTerrain"));
    const float FogRadius=Number(Catalog()->GetObjectField(TEXT("tuning")),TEXT("fogRadius"));
    const FVector Origin=G->LocationPosition(TEXT("camp"));
    for(int32 X=-3000;X<3000;X+=300)
        for(int32 Y=-3000;Y<3000;Y+=300)
        {
            const FVector2D World(Origin.X+X+150,Origin.Y+Y+150);
            if(G->Explored.ContainsByPredicate([&](FVector2D Seen){ return FVector2D::Distance(World,Seen)<=FogRadius; })) continue;
            Element(TEXT("fog"),TEXT(""),MapPoint(FVector(X,Y+300,0)),FVector2D(55.5,38)*MapZoom+FVector2D(1,1));
        }
    for(const auto& V:Rows(TEXT("locations")))
    {
        const auto R=V->AsObject(); const FName Id(*Text(R,TEXT("id"))); if(!G->Discovered.Contains(Id)) continue;
        if(Category==TEXT("travel") && Text(R,TEXT("kind"))==TEXT("landmark")) continue;
        const FVector P=Position(R); const FVector2D UI=MapPoint(P)-FVector2D(27,27);
        Element(TEXT("image"),TEXT(""),UI+FVector2D(10,9),FVector2D(34,36),18,TEXT(""),G->Activated.Contains(Id)?TEXT("mapTravelIcon"):Text(R,TEXT("kind"))==TEXT("landmark")?TEXT("mapLandmarkIcon"):TEXT("mapCampIcon"));
        Element(TEXT("tab"),TEXT(""),UI,FVector2D(54,54),32,TEXT("location:")+Id.ToString(),TEXT(""),SelectedLocation==Id);
        Element(TEXT("text"),Text(R,TEXT("name")),UI+FVector2D(-27,58),FVector2D(200,35),19);
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
        const auto Target=Find(TEXT("locations"),Text(Q,TEXT("location")));
        if(Target)
        {
            Element(TEXT("image"),TEXT(""),MapPoint(Position(Target))+FVector2D(25,-40),FVector2D(33,40),18,TEXT(""),TEXT("mapQuestIcon")); Elements.Last().MapClipped=true;
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
        if(Text(R,TEXT("kind"))==Category)
        {
            Timeline.Add(Id);
            if(G->QuestAvailable(Id)) Visible.Add(Id);
        }
    }
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Visible.Num()-5));
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
    Element(TEXT("text"),TEXT("◇  ")+Text(Q,TEXT("objective")),FVector2D(407,621),FVector2D(815,50),19); Elements.Last().Color=Color(TEXT("gold"));
    Element(TEXT("text"),FString::Printf(TEXT("进度 %d / %.0f"),G->QuestProgress(SelectedQuest),Number(Q,TEXT("required"))),FVector2D(443,658),FVector2D(400,30),15);
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
    Element(TEXT("choice"),G->Claimed.Contains(SelectedQuest)?TEXT("已完成"):TEXT("领取奖励"),FVector2D(1403,740),FVector2D(180,46),18,TEXT("claim"));
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
    FString Reply=AI->CanDisplay()?AI->GetNPCLine():FString();
    if(Reply.IsEmpty()) Reply=TEXT("我在这里。有什么需要一起做的？");
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,FMath::DivideAndRoundUp(Reply.Len(),23)-4));
    TArray<FString> Lines; for(int32 I=Scroll*23;I<FMath::Min(Reply.Len(),(Scroll+4)*23);I+=23) Lines.Add(Reply.Mid(I,23));
    Element(TEXT("text"),FString::Join(Lines,TEXT("\n")),FVector2D(993,362),FVector2D(510,128),20);
    for(const auto& Entry:Theme->GetArrayField(TEXT("dialogueChoices")))
    {
        const auto Choice=Entry->AsObject(); const auto& Rect=Choice->GetArrayField(TEXT("rect"));
        const FVector2D P(Rect[0]->AsNumber(),Rect[1]->AsNumber());
        Element(TEXT("choice"),Text(Choice,TEXT("label")),P,FVector2D(Rect[2]->AsNumber(),Rect[3]->AsNumber()),19,TEXT("say:")+Text(Choice,TEXT("message")));
        Elements.Last().TextInset=76;
        Element(TEXT("image"),TEXT(""),P+FVector2D(23,11),FVector2D(32,33),18,TEXT(""),Text(Choice,TEXT("icon")));
    }
    FString Status=AI->IsBusy()?TEXT("正在思考…"):AI->GetStatus();
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        if(!It->BlockReason.IsEmpty()) { Status=TEXT("委托受阻：")+It->BlockReason; break; }
    Element(TEXT("text"),Status,FVector2D(965,824),FVector2D(620,30),15);
    Element(TEXT("choice"),AI->IsBusy()?TEXT("取消回复"):TEXT("发送"),FVector2D(1484,766),FVector2D(105,51),19,AI->IsBusy()?TEXT("cancelReply"):TEXT("send"));
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        if(It->GetRequested()>0)
        {
            Element(TEXT("text"),FString::Printf(TEXT("当前委托：已入库 %d / %d"),It->GetDelivered(),It->GetRequested()),FVector2D(960,858),FVector2D(440,32),17);
            Element(TEXT("choice"),TEXT("取消委托"),FVector2D(1420,854),FVector2D(168,39),17,TEXT("cancelTask"));
        }
        break;
    }
}
void UHearthwardScreenWidget::ComposeHUD()
{
    if(const auto* B=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>();B && !B->Feedback.IsEmpty())
    {
        Element(TEXT("notice"),B->Feedback,FVector2D(500,625),FVector2D(680,55),18);
        Elements.Last().Component=TEXT("hud.construction"); Elements.Last().LayoutId=TEXT("hud.construction.feedback");
    }
    auto* G=Gameplay();
    const auto Q=Find(TEXT("quests"),G->TrackedQuest.ToString());
    Element(TEXT("text"),TEXT("◇  ")+Text(Q,TEXT("name")),FVector2D(46,59),FVector2D(540,48),24); Elements.Last().Color=Color(TEXT("gold"));
    Element(TEXT("text"),Text(Q,TEXT("objective")),FVector2D(90,102),FVector2D(540,55),19);
    Element(TEXT("text"),FString::Printf(TEXT("◇  进度 %d / %.0f"),G->QuestProgress(G->TrackedQuest),Number(Q,TEXT("required"))),FVector2D(96,138),FVector2D(470,35),18);
    const float V[]={G->Health,G->Hunger,G->Stamina},Max[]={G->MaxHealth(),100,G->MaxStamina()}; const FString C[]={TEXT("health"),TEXT("hunger"),TEXT("stamina")};
    for(int32 I=0;I<3;++I)
    {
        const FString Labels[]={TEXT("♡ 生命"),TEXT("饱食"),TEXT("体力")};
        Element(TEXT("text"),Labels[I],FVector2D(48,663+I*35),FVector2D(80,30),18); Elements.Last().Color=Color(C[I]);
        Element(TEXT("bar"),TEXT(""),FVector2D(130,673+I*35),FVector2D(168,11)); Elements.Last().Color=Color(C[I]); Elements.Last().Value=V[I]/Max[I];
        Element(TEXT("text"),FString::Printf(TEXT("%.0f / %.0f"),V[I],Max[I]),FVector2D(310,663+I*35),FVector2D(150,30),16);
    }
    TArray<FName> Quick; for(const auto& Entry:Theme->GetObjectField(TEXT("inventory"))->GetArrayField(TEXT("quickSlots"))) Quick.Add(FName(*Entry->AsString()));
    for(int32 I=0;I<Quick.Num();++I)
    {
        const auto R=Find(TEXT("items"),Quick[I].ToString()); const FVector2D P(48+I*94,789);
        const FString QuickArt=Text(R,TEXT("quickIcon"));
        Element(TEXT("image"),TEXT(""),P+FVector2D(8,4),FVector2D(60,60),18,TEXT(""),QuickArt.IsEmpty()?Text(R,TEXT("icon")):QuickArt);
        Element(TEXT("slot"),FString::Printf(TEXT("%d     %d"),I+1,Inventory()->GetItemCount(Quick[I])),P,FVector2D(80,78),15);
        Element(TEXT("text"),Text(R,TEXT("name")),P+FVector2D(20,84),FVector2D(95,25),15);
    }
    const auto Mini=Theme->GetObjectField(TEXT("minimap")); const auto& Rect=Mini->GetArrayField(TEXT("rect"));
    const FVector2D MiniPosition(Rect[0]->AsNumber(),Rect[1]->AsNumber()),MiniSize(Rect[2]->AsNumber(),Rect[3]->AsNumber());
    const FVector2D MiniCenter=MiniPosition+MiniSize*.5;
    Element(TEXT("minimap"),TEXT(""),MiniPosition,MiniSize,18,TEXT(""),TEXT("mapTerrain"));
    const FVector Origin=G->LocationPosition(TEXT("camp")); const float WorldSize=Number(Mini,TEXT("worldSize"));
    const auto ProjectMini=[&](FVector P){ const FVector Local=P-Origin; return MiniCenter+FVector2D(Local.X,-Local.Y)*(MiniSize.X/WorldSize); };
    for(const auto& Entry:Rows(TEXT("locations")))
    {
        const auto R=Entry->AsObject(); const FName Id(*Text(R,TEXT("id"))); if(!G->Discovered.Contains(Id)) continue;
        const FVector2D P=ProjectMini(G->LocationPosition(Id)); if((P-MiniCenter).Size()>MiniSize.X*.46) continue;
        Element(TEXT("text"),G->Activated.Contains(Id)?TEXT("♧"):TEXT("◇"),P-FVector2D(9,12),FVector2D(25,30),22); Elements.Last().Color=Color(TEXT("teal"));
    }
    const FVector2D PlayerPoint=ProjectMini(GetOwningPlayerPawn()->GetActorLocation());
    if((PlayerPoint-MiniCenter).Size()<MiniSize.X*.47)
    { Element(TEXT("arrow"),TEXT(""),PlayerPoint-FVector2D(10,10),FVector2D(20,20)); Elements.Last().Value=GetOwningPlayer()->GetControlRotation().Yaw+90; Elements.Last().Color=Color(TEXT("gold")); }
    Element(TEXT("text"),TEXT("北"),MiniPosition+FVector2D(MiniSize.X*.5-9,4),FVector2D(25,25),15);
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        const FString Order=G->CompanionOrder==TEXT("follow")?TEXT("跟随中"):G->CompanionOrder==TEXT("attack")?TEXT("协助进攻"):It->GetRequested()>0?FString::Printf(TEXT("委托 %d / %d"),It->GetDelivered(),It->GetRequested()):TEXT("原地等待");
        Element(TEXT("text"),Order,FVector2D(146,282),FVector2D(320,30),18);
        Element(TEXT("text"),TEXT("Z   等待\nX   跟随\nC   进攻"),FVector2D(81,325),FVector2D(250,125),20);
        Element(TEXT("bar"),TEXT(""),FVector2D(119,265),FVector2D(151,7)); Elements.Last().Color=Color(TEXT("teal"));
        Elements.Last().Value=It->GetRequested()>0?float(It->GetDelivered())/It->GetRequested():0;
        break;
    }
    const FName Near=G->NearbyLocation();
    if(G->HasWaypoint) Element(TEXT("text"),FString::Printf(TEXT("◇  %.0f 米"),FVector::Dist2D(GetOwningPlayerPawn()->GetActorLocation(),G->Waypoint)/100),FVector2D(1370,267),FVector2D(200,35),18);
    if(!Near.IsNone()) Element(TEXT("text"),G->Activated.Contains(Near)?TEXT("M 查看地图 / 传送"):TEXT("E 激活路标"),FVector2D(1400,605),FVector2D(270,45),19);
    Element(TEXT("text"),G->Feedback,FVector2D(1220,722),FVector2D(420,70),18);
    const FName Bow=G->Equipment.FindRef(TEXT("ranged"));
    if(!Bow.IsNone())
    {
        Element(TEXT("image"),TEXT(""),FVector2D(717,798),FVector2D(75,95),18,TEXT(""),Text(Find(TEXT("items"),Bow.ToString()),TEXT("icon")));
        Element(TEXT("text"),FString::Printf(TEXT("猎弓   %d\n右键 射击"),Inventory()->GetItemCount(TEXT("arrow"))),FVector2D(805,835),FVector2D(225,60),22);
    }
    if(G->Skills.FindRef(TEXT("strong"))>0) Element(TEXT("text"),TEXT("Q 强力挥击"),FVector2D(720,744),FVector2D(260,35),19);
    const int32 Time=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ActivePlaySeconds;
    Element(TEXT("text"),FString::Printf(TEXT("第 %d 天  %02d:%02d  晴"),Time/1440+1,(Time/60)%24,Time%60),FVector2D(1380,31),FVector2D(290,35),16);
    if(G->Health<=0)
    {
        Element(TEXT("panel"),TEXT(""),FVector2D(530,320),FVector2D(610,230));
        Element(TEXT("text"),TEXT("你已倒下\n按 Esc 打开菜单，载入保存节点"),FVector2D(590,362),FVector2D(540,130),26);
    }
    if(const auto* Interaction=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardInteractionComponent>())
    {
        if(const auto* Target=Interaction->GetNearestTarget())
            Element(TEXT("notice"),Target->GetInteractionPrompt(GetOwningPlayerPawn()),FVector2D(573,536),FVector2D(540,70),18);
        if(Interaction->GetStatus()==EHearthwardInteractionStatus::Ready)
            Element(TEXT("text"),Interaction->GetCompletionFeedback(),FVector2D(1220,765),FVector2D(410,40),18);
    }
    if(const auto* Timer=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardTimedActionComponent>();Timer && Timer->GetStatus()==EHearthwardTimedActionStatus::Running)
    {
        Element(TEXT("notice"),FString::Printf(TEXT("进行中  %.1f / 5.0 秒"),Timer->GetElapsedSeconds()),FVector2D(636,750),FVector2D(400,58),20);
        Element(TEXT("bar"),TEXT(""),FVector2D(654,792),FVector2D(364,6)); Elements.Last().Value=Timer->GetElapsedSeconds()/5; Elements.Last().Color=Color(TEXT("gold"));
    }
}

void UHearthwardScreenWidget::ComposeBuilding()
{
    int32 Index=0;
    for(const auto& V:Rows(TEXT("buildings")))
    {
        const auto R=V->AsObject(); const float Y=245+Index++*175;
        const FString Id=Text(R,TEXT("id"));
        FString Cost;
        for(const auto& M:R->GetObjectField(TEXT("materials"))->Values)
            Cost+=Text(Find(TEXT("items"),FString(*M.Key)),TEXT("name"))+FString::Printf(TEXT(" %d / %.0f  "),Inventory()->GetItemCount(FName(*M.Key)),M.Value->AsNumber());
        Element(TEXT("button"),Text(R,TEXT("name")),FVector2D(510,Y),FVector2D(650,55),26,TEXT("build:")+Id);
        Elements.Last().Component=TEXT("building.sheet"); Elements.Last().LayoutId=TEXT("building.")+Id+TEXT(".select");
        Element(TEXT("text"),Cost+TEXT("\n")+Text(R,TEXT("description")),FVector2D(528,Y+64),FVector2D(660,86),18);
        Elements.Last().Component=TEXT("building.sheet"); Elements.Last().LayoutId=TEXT("building.")+Id+TEXT(".description");
    }
}
void UHearthwardScreenWidget::ComposeSave()
{
    auto* S=GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>(); const auto Points=S->GetPoints();
    for(int32 I=Scroll;I<FMath::Min(Points.Num(),Scroll+7);++I)
    {
        const auto& P=Points[Points.Num()-1-I]; const FString Id=P.SaveId.ToString(); const float Y=302+(I-Scroll)*71;
        Element(TEXT("button"),P.Created.ToString(TEXT("%m-%d %H:%M"))+(P.Manual?TEXT(" 手动"):TEXT(" 自动")),FVector2D(475,Y),FVector2D(390,61),20,TEXT("ask:load:")+Id);
        Element(TEXT("button"),P.Locked?TEXT("解锁"):TEXT("锁定"),FVector2D(893,Y),FVector2D(130,61),19,TEXT("lock:")+Id);
        Element(TEXT("button"),TEXT("删除"),FVector2D(1060,Y),FVector2D(130,61),19,TEXT("ask:delete:")+Id);
    }
    if(Points.IsEmpty()) Element(TEXT("text"),TEXT("尚无保存节点"),FVector2D(505,337),FVector2D(640,60),24);
}
