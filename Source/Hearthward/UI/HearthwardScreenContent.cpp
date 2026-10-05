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
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

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
    if(!Storage) { ComposeInventoryScreen(); return; }
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
        const FString StorageCategory=Text(R,TEXT("category"))==TEXT("药品")?TEXT("食物"):Text(R,TEXT("category"));
        if(!Category.IsEmpty() && Category!=TEXT("全部") && Category!=StorageCategory) continue;
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
            Element(TEXT("text"),Text(SlotRow,TEXT("name")),P-FVector2D(40,28),FVector2D(160,32),15); Elements.Last().Align=TEXT("center");
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
    const float TextScale=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f;
    auto Add=[&](const FString& Id,FString Type,FString Label,FVector2D P,FVector2D Size,float Font=18,FString Action=FString(),FString Asset=FString(),bool Selected=false)->FHearthwardUIElement&
    {
        Element(Type,Label,P,Size,Font,Action,Asset,Selected);
        auto& E=Elements.Last();E.LayoutId=Id;E.Font=Font*TextScale;E.FontRole=TEXT("body");E.Tracking=0;E.TextInset=0;
        return E;
    };
    Add(TEXT("background"),TEXT("skillsBackdrop"),TEXT(""),FVector2D::ZeroVector,DesignSize);
    Add(TEXT("skills.background.counterHelix"),TEXT("skillsCounterHelix"),TEXT(""),FVector2D::ZeroVector,DesignSize).Component=TEXT("skills.decoration");
    Add(TEXT("skills.header.heading"),TEXT("text"),TEXT("技能"),{42,32},{240,72},36).Component=TEXT("skills.header");
    Add(TEXT("skills.header.flame"),TEXT("image"),TEXT(""),{1340,34},{32,38},18,TEXT(""),TEXT("continueIcon")).Component=TEXT("skills.header");
    Add(TEXT("skills.header.points"),TEXT("text"),FString::FromInt(G->SkillPoints()),{1388,32},{62,52},34).Component=TEXT("skills.header");
    Add(TEXT("skills.header.pointsLabel"),TEXT("text"),TEXT("可用技能点"),{1450,48},{172,38},16).Component=TEXT("skills.header");
    auto& TopRule=Add(TEXT("skills.header.rule"),TEXT("line"),TEXT(""),{40,112},{1592,1});TopRule.Color=Color(TEXT("bronze"))*.45f;TopRule.Component=TEXT("skills.header");
    auto BranchIndex=[&](const FString& Id)
    {return Branches.IndexOfByPredicate([&](const auto& V){return Text(V->AsObject(),TEXT("id"))==Id;});};
    // Equal leaf columns and prerequisite-depth rows keep each original tree tidy.
    // A single child shares its parent's column; forks center over their children.
    constexpr float ColumnPitch=78,LayerPitch=100,NodeWidth=56,TreeTop=318;
    TMap<FString,TArray<FString>> ChildSkills;
    TMap<FString,FVector2D> NodePoints;
    for(const auto& V:Rows(TEXT("skills")))
    {
        const auto R=V->AsObject();const FString ParentId=Text(R,TEXT("requires"));
        if(!ParentId.IsEmpty()) ChildSkills.FindOrAdd(ParentId).Add(Text(R,TEXT("id")));
    }
    for(int32 TreeIndex=0;TreeIndex<Branches.Num();++TreeIndex)
    {
        const FString BranchId=Text(Branches[TreeIndex]->AsObject(),TEXT("id"));
        FString RootId;
        for(const auto& V:Rows(TEXT("skills")))
            if(Text(V->AsObject(),TEXT("branch"))==BranchId && Text(V->AsObject(),TEXT("requires")).IsEmpty())
                RootId=Text(V->AsObject(),TEXT("id"));
        if(RootId.IsEmpty()) continue;
        float NextLeaf=0;
        TFunction<float(const FString&,int32)> PlaceSkill;
        PlaceSkill=[&](const FString& SkillId,int32 Depth)
        {
            float Column=0;const auto* Children=ChildSkills.Find(SkillId);
            if(Children && !Children->IsEmpty())
            {
                for(const FString& ChildId:*Children) Column+=PlaceSkill(ChildId,Depth+1);
                Column/=Children->Num();
            }
            else Column=NextLeaf++;
            NodePoints.Add(SkillId,FVector2D(Column*ColumnPitch,Depth*LayerPitch));
            return Column;
        };
        const float RootColumn=PlaceSkill(RootId,0);
        const FVector2D Origin(40+TreeIndex*286+135-RootColumn*ColumnPitch-NodeWidth*.5f,TreeTop);
        for(const auto& V:Rows(TEXT("skills")))
            if(Text(V->AsObject(),TEXT("branch"))==BranchId)
                if(auto* Point=NodePoints.Find(Text(V->AsObject(),TEXT("id")))) *Point+=Origin;
    }
    auto NodePoint=[&](const TSharedPtr<FJsonObject>& Row)
    {return NodePoints.FindRef(Text(Row,TEXT("id")));};
    for(int32 TreeIndex=0;TreeIndex<Branches.Num();++TreeIndex)
    {
        const auto B=Branches[TreeIndex]->AsObject();const FString Id=Text(B,TEXT("id")),Prefix=TEXT("skills.branch.")+FString::FromInt(TreeIndex);
        const float Left=40+TreeIndex*286;
        auto& Surface=Add(Prefix+TEXT(".surface"),TEXT("skillsSurface"),TEXT(""),FVector2D(Left,136),{270,706});Surface.Component=Prefix;Surface.Value=0;
        Add(Prefix+TEXT(".icon"),TEXT("image"),TEXT(""),FVector2D(Left+104,152),{62,60},18,TEXT(""),Id+TEXT("Icon")).Component=Prefix;
        auto& Name=Add(Prefix+TEXT(".name"),TEXT("text"),Text(B,TEXT("name")),FVector2D(Left+16,220),{238,45},24);Name.Align=TEXT("center");Name.Component=Prefix;
        auto& Subtitle=Add(Prefix+TEXT(".subtitle"),TEXT("text"),Text(B,TEXT("subtitle")),FVector2D(Left+12,268),{246,36},14);Subtitle.Align=TEXT("center");Subtitle.Color=Color(TEXT("muted"));Subtitle.Component=Prefix;
        int32 Learned=0,Total=0;
        for(const auto& V:Rows(TEXT("skills"))) if(Text(V->AsObject(),TEXT("branch"))==Id)
        {Learned+=G->Skills.FindRef(FName(*Text(V->AsObject(),TEXT("id"))));Total+=Number(V->AsObject(),TEXT("maxRank"));}
        auto& Progress=Add(Prefix+TEXT(".progress"),TEXT("text"),FString::Printf(TEXT("%d / %d"),Learned,Total),FVector2D(Left+16,719),{238,36},17);Progress.Align=TEXT("center");Progress.Color=Color(TEXT("gold"));Progress.Component=Prefix;
        auto& Flavor=Add(Prefix+TEXT(".flavor"),TEXT("text"),Text(B,TEXT("flavor")),FVector2D(Left+14,770),{242,64},13);Flavor.Align=TEXT("center");Flavor.Color=Color(TEXT("muted"));Flavor.Component=Prefix;
    }
    // Each node and edge still comes from the original skill catalogue and prerequisite.
    // Draw all edges first so they stay behind every clickable node.
    for(const auto& V:Rows(TEXT("skills")))
    {
        const auto R=V->AsObject();const FString Id=Text(R,TEXT("id"));const FVector2D P=NodePoint(R);
        const auto Parent=Find(TEXT("skills"),Text(R,TEXT("requires")));
        if(Parent)
        {
            const FVector2D Start=NodePoint(Parent)+FVector2D(28,56);
            auto& Edge=Add(TEXT("skills.edge.")+Id,TEXT("connection"),TEXT(""),Start,P+FVector2D(28,0)-Start);
            Edge.Component=TEXT("skills.branch.")+FString::FromInt(BranchIndex(Text(R,TEXT("branch"))));
            Edge.Color=G->Skills.FindRef(FName(*Text(Parent,TEXT("id"))))>0?Color(TEXT("teal"))*.85f:Color(TEXT("bronze"))*.65f;
        }
    }
    for(const auto& V:Rows(TEXT("skills")))
    {
        const auto R=V->AsObject();const FString Id=Text(R,TEXT("id")),Component=TEXT("skills.branch.")+FString::FromInt(BranchIndex(Text(R,TEXT("branch"))));const FVector2D P=NodePoint(R);
        const int32 LearnedRank=G->Skills.FindRef(FName(*Id));
        const FString LearnedArt=Text(R,TEXT("learnedIcon"));
        Add(TEXT("skills.icon.")+Id,TEXT("image"),TEXT(""),P,{56,56},18,TEXT(""),LearnedRank>0 && !LearnedArt.IsEmpty()?LearnedArt:Id+TEXT("Icon")).Component=Component;
        auto& Node=Add(TEXT("skills.node.")+Id,TEXT("node"),TEXT(""),P,{56,56},13,TEXT("skill:")+Id,TEXT(""),SelectedSkill==FName(*Id));Node.Value=LearnedRank;Node.Component=Component;
        auto& RankLabel=Add(TEXT("skills.rank.")+Id,TEXT("text"),FString::Printf(TEXT("%d/%d"),LearnedRank,int32(Number(R,TEXT("maxRank")))),P+FVector2D(0,57),{56,28},12);RankLabel.Align=TEXT("center");RankLabel.Color=Color(LearnedRank>0?TEXT("gold"):TEXT("muted"));RankLabel.Component=Component;
    }
    const auto R=Find(TEXT("skills"),SelectedSkill.ToString()); if(!R) return;
    auto Detail=[&](const FString& Id,FString Type,FString Label,FVector2D P,FVector2D Size,float Font=18,FString Action=FString(),FString Asset=FString())->FHearthwardUIElement&
    {auto& E=Add(TEXT("skills.detail.")+Id,Type,Label,P,Size,Font,Action,Asset);E.Component=TEXT("skills.detail");return E;};
    Detail(TEXT("surface"),TEXT("skillsSurface"),TEXT(""),{1188,136},{444,706}).Value=1;
    const int32 Rank=G->Skills.FindRef(SelectedSkill);const FString LearnedArt=Text(R,TEXT("learnedIcon"));
    Detail(TEXT("icon"),TEXT("image"),TEXT(""),{1214,157},{96,96},18,TEXT(""),Rank>0 && !LearnedArt.IsEmpty()?LearnedArt:SelectedSkill.ToString()+TEXT("Icon"));
    Detail(TEXT("name"),TEXT("text"),Text(R,TEXT("name")),{1334,166},{278,55},27);
    Detail(TEXT("rank"),TEXT("text"),FString::Printf(TEXT("%d / %d"),Rank,int32(Number(R,TEXT("maxRank")))),{1334,220},{260,40},18).Color=Color(TEXT("gold"));
    const bool Active=Text(R,TEXT("effect"))==TEXT("heavyAttack");
    Detail(TEXT("kind"),TEXT("text"),Text(R,TEXT("branchName"))+TEXT(" · ")+(Active?Text(R,TEXT("activation")):TEXT("成长 · 被动")),{1214,286},{392,40},17).Color=Color(TEXT("muted"));
    Detail(TEXT("rule"),TEXT("line"),TEXT(""),{1214,330},{392,1}).Color=Color(TEXT("bronze"))*.5f;
    Detail(TEXT("description"),TEXT("text"),Text(R,TEXT("description")),{1214,355},{392,144},18);
    for(int32 I=1;I<=Number(R,TEXT("maxRank"));++I)
    {
        const float Y=526+(I-1)*58;
        Detail(TEXT("level.")+FString::FromInt(I),TEXT("text"),FString::Printf(TEXT("◇  等级 %d    %s"),I,I<=Rank?TEXT("已学习"):I==Rank+1?TEXT("下一等级"):TEXT("未解锁")),FVector2D(1214,Y),{392,42},18).Color=Color(I<=Rank?TEXT("gold"):TEXT("text"));
        Detail(TEXT("levelRule.")+FString::FromInt(I),TEXT("line"),TEXT(""),FVector2D(1214,Y+43),{392,1}).Color=Color(TEXT("bronze"))*.3f;
    }
    const auto Prerequisite=Find(TEXT("skills"),Text(R,TEXT("requires")));
    Detail(TEXT("prerequisite"),TEXT("text"),TEXT("前置技能：")+(Prerequisite?Text(Prerequisite,TEXT("name")):TEXT("无")),{1214,706},{392,36},15).Color=Color(TEXT("muted"));
    Detail(TEXT("cost"),TEXT("text"),FString::Printf(TEXT("所需技能点 %.0f"),Number(R,TEXT("cost"))),{1214,746},{392,36},17);
    auto& Learn=Detail(TEXT("learn"),TEXT("menuAction"),TEXT("学习技能"),{1214,790},{392,48},21,TEXT("learn"));Learn.Align=TEXT("center");
    Add(TEXT("skills.footer.back"),TEXT("menuAction"),TEXT("返回"),{40,883},{140,44},18,TEXT("back")).Component=TEXT("skills.footer");
    Add(TEXT("skills.footer.respec"),TEXT("menuAction"),TEXT("免费洗点"),{1380,883},{252,44},18,TEXT("respec")).Component=TEXT("skills.footer");
    const FString Help=TEXT("单击节点查看   ·   F 学习 / Enter 确认   ·   Esc 返回");
    auto& Footer=Add(TEXT("skills.footer.help"),TEXT("text"),Message.IsEmpty()?Help:Message+TEXT("\n")+Help,FVector2D(220,Message.IsEmpty()?883:856),FVector2D(1120,Message.IsEmpty()?44:76),16);Footer.Color=Color(TEXT("muted"));Footer.Component=TEXT("skills.footer");
}
int32 UHearthwardScreenWidget::JournalPageSize() const
{ return GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale>=130?6:8; }
TArray<TSharedPtr<FJsonObject>> UHearthwardScreenWidget::JournalEntries() const
{
    const bool Quests=Category==TEXT("main") || Category==TEXT("side"),Collection=Category==TEXT("collection");
    TArray<TSharedPtr<FJsonObject>> Entries;
    const bool Campaign=GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Active();
    for(const auto& V:Rows(Quests?TEXT("quests"):Collection?TEXT("items"):TEXT("codex")))
    {
        const auto R=V->AsObject();
        const bool InCategory=Quests ? Text(R,TEXT("kind"))==Category && bool(HearthwardCampaign::Find(TEXT("quests"),FName(*Text(R,TEXT("id")))))==Campaign
            : Collection || Text(R,TEXT("category"))==Category;
        if(InCategory && (!Collection || JournalEntryKnown(R))) Entries.Add(R);
    }
    return Entries;
}
bool UHearthwardScreenWidget::JournalEntryKnown(const TSharedPtr<FJsonObject>& R) const
{
    const auto* G=Gameplay();
    if(Category==TEXT("main") || Category==TEXT("side")) return G->QuestAvailable(FName(*Text(R,TEXT("id"))));
    if(Category==TEXT("collection")) return G->Events.FindRef(FName(*(TEXT("collected:")+Text(R,TEXT("id")))))>0;
    const FString Location=Text(R,TEXT("location")),Event=Text(R,TEXT("event")),Quest=Text(R,TEXT("quest"));
    return (!Location.IsEmpty() && G->Discovered.Contains(FName(*Location))) || (!Event.IsEmpty() && G->Events.FindRef(FName(*Event))>0) || (!Quest.IsEmpty() && G->Claimed.Contains(FName(*Quest)));
}
void UHearthwardScreenWidget::ComposeJournal()
{
    auto* G=Gameplay();
    const bool Quests=Category==TEXT("main") || Category==TEXT("side"),Collection=Category==TEXT("collection");
    const float TextScale=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f;
    auto Add=[&](const FString& Id,FString Type,FString Label,FVector2D P,FVector2D Size,float Font=18,FString Action=FString(),FString Asset=FString(),bool Selected=false)->FHearthwardUIElement&
    {
        Element(Type,Label,P,Size,Font,Action,Asset,Selected);
        auto& E=Elements.Last();E.LayoutId=Id;E.Font=Font*TextScale;E.FontRole=TEXT("body");E.Tracking=0;E.TextInset=0;
        E.Component=Id.StartsWith(TEXT("journal.header."))?TEXT("journal.header"):Id.StartsWith(TEXT("journal.list."))?TEXT("journal.list"):Id.StartsWith(TEXT("journal.detail."))?TEXT("journal.detail"):TEXT("journal.footer");
        return E;
    };
    Add(TEXT("background"),TEXT("journalBackdrop"),TEXT(""),FVector2D::ZeroVector,DesignSize).Component.Reset();
    Add(TEXT("journal.header.heading"),TEXT("text"),TEXT("日志"),{42,32},{240,72},36);
    Add(TEXT("journal.header.prev"),TEXT("menuAction"),TEXT("Q  ‹"),{466,35},{78,64},18,TEXT("journal.category.prev")).Align=TEXT("center");
    const FString Categories[]={TEXT("main"),TEXT("side"),TEXT("world"),TEXT("people"),TEXT("factions"),TEXT("collection")};
    const FString Headings[]={TEXT("主线任务"),TEXT("支线任务"),TEXT("世界见闻"),TEXT("人物档案"),TEXT("势力阵营"),TEXT("收集要素")};
    FString Heading;
    for(int32 CategoryIndex=0;CategoryIndex<6;++CategoryIndex)
    {
        const bool Selected=Category==Categories[CategoryIndex];if(Selected)Heading=Headings[CategoryIndex];
        const FVector2D P(560+CategoryIndex*100,28);
        Add(TEXT("journal.header.tab.")+Categories[CategoryIndex],TEXT("menuTab"),TEXT(""),P,{64,74},18,TEXT("category:")+Categories[CategoryIndex],TEXT(""),Selected);
        const FString Asset=TEXT("journalCategory")+Categories[CategoryIndex];const auto Art=Theme->GetObjectField(TEXT("assets"))->GetObjectField(Asset)->GetArrayField(TEXT("uv"));
        const FVector2D ArtSize(42*Art[2]->AsNumber()/Art[3]->AsNumber(),42);
        // The legacy main glyph is baked amber; balance its inactive tint to neutral gray.
        const FLinearColor Inactive=CategoryIndex==0?FLinearColor(.04f,.05f,.083f,1):FLinearColor(.25f,.25f,.25f,1);
        Add(TEXT("journal.header.icon.")+Categories[CategoryIndex],TEXT("image"),TEXT(""),P+FVector2D((64-ArtSize.X)*.5,8),ArtSize,18,TEXT(""),Asset,Selected).Color=Selected?FLinearColor::White:Inactive;
    }
    Add(TEXT("journal.header.next"),TEXT("menuAction"),TEXT("›  E"),{1174,35},{78,64},18,TEXT("journal.category.next")).Align=TEXT("center");
    Add(TEXT("journal.header.rule"),TEXT("line"),TEXT(""),{40,112},{1592,1}).Color=Color(TEXT("bronze"))*.45f;
    Add(TEXT("journal.list.surface"),TEXT("menuSurface"),TEXT(""),{40,136},{664,706});
    Add(TEXT("journal.detail.surface"),TEXT("menuSurface"),TEXT(""),{752,136},{880,706});
    Add(TEXT("journal.list.heading"),TEXT("text"),Heading,{64,157},{360,52},26);
    const auto Entries=JournalEntries();const int32 PageSize=JournalPageSize();
    Scroll=FMath::Clamp(Scroll,0,FMath::Max(0,Entries.Num()-PageSize));
    int32 Known=0;for(const auto& R:Entries)Known+=JournalEntryKnown(R);
    const int32 Total=Collection?Rows(TEXT("items")).Num():Entries.Num();
    const FString CountLabel=Quests?FString::Printf(TEXT("已开启 %d / %d"),Known,Total):FString::Printf(TEXT("已记录 %d / %d"),Known,Total);
    Add(TEXT("journal.list.count"),TEXT("text"),CountLabel,{445,170},{235,36},16).Align=TEXT("right");
    Add(TEXT("journal.list.rule"),TEXT("line"),TEXT(""),{64,213},{616,1}).Color=Color(TEXT("bronze"))*.45f;
    auto Selectable=[&](const auto& R){return JournalEntryKnown(R);};
    FName& Selection=Quests?SelectedQuest:SelectedCodex;
    if(!Entries.ContainsByPredicate([&](const auto& R){return FName(*Text(R,TEXT("id")))==Selection && Selectable(R);}))
    {
        const int32 FirstIndex=Entries.IndexOfByPredicate(Selectable);
        Selection=Entries.IsValidIndex(FirstIndex)?FName(*Text(Entries[FirstIndex],TEXT("id"))):NAME_None;
        if(FirstIndex!=INDEX_NONE)Scroll=FMath::Clamp(Scroll,FMath::Max(0,FirstIndex-PageSize+1),FirstIndex);
    }
    const float RowPitch=PageSize==6?96.f:72.f,RowHeight=RowPitch-8;
    for(int32 EntryIndex=Scroll;EntryIndex<FMath::Min(Entries.Num(),Scroll+PageSize);++EntryIndex)
    {
        const auto R=Entries[EntryIndex];const FString Id=Text(R,TEXT("id"));const bool IsKnown=JournalEntryKnown(R),Selected=Selection==FName(*Id);
        const FVector2D P(64,225+(EntryIndex-Scroll)*RowPitch);
        auto& Row=Add(TEXT("journal.list.entry.")+Id,TEXT("menuRow"),TEXT(""),P,{616,RowHeight},18,Selectable(R)?(Quests?TEXT("quest:"):TEXT("codex:"))+Id:TEXT(""),TEXT(""),Selected);Row.Enabled=Selectable(R);
        // Keep the original row and its separator without revealing an unknown record.
        if(!IsKnown)continue;
        Add(TEXT("journal.list.mark.")+Id,TEXT("text"),Selected?TEXT("◆"):TEXT("◇"),P+FVector2D(8,12),{30,36},18).Color=Color(Selected?TEXT("gold"):TEXT("muted"));
        Add(TEXT("journal.list.name.")+Id,TEXT("text"),Text(R,TEXT("name")),P+FVector2D(44,PageSize==6?12:8),{558,40},20).Color=Color(Selected?TEXT("gold"):TEXT("text"));
        FString Status=TEXT("已收录");
        if(Quests) Status=G->Claimed.Contains(FName(*Id))?TEXT("已完成"):FString::Printf(TEXT("%s进度 %d / %.0f"),G->TrackedQuest==FName(*Id)?TEXT("追踪中 · "):TEXT(""),G->QuestProgress(FName(*Id)),Number(R,TEXT("required")));
        else if(Collection)Status=FString::Printf(TEXT("当前持有 %d 件"),Inventory()->GetItemCount(FName(*Id)));
        Add(TEXT("journal.list.status.")+Id,TEXT("text"),Status,P+FVector2D(44,PageSize==6?54:39),{558,30},14).Color=Color(TEXT("muted"));
    }
    Add(TEXT("journal.list.prev"),TEXT("menuAction"),TEXT("‹ 上一页"),{64,803},{162,34},16,TEXT("journal.list.prev")).Enabled=Scroll>0;
    Add(TEXT("journal.list.range"),TEXT("text"),FString::Printf(TEXT("%d – %d / %d"),Entries.IsEmpty()?0:Scroll+1,FMath::Min(Entries.Num(),Scroll+PageSize),Entries.Num()),{248,806},{248,30},14).Align=TEXT("center");
    auto& Next=Add(TEXT("journal.list.next"),TEXT("menuAction"),TEXT("下一页 ›"),{518,803},{162,34},16,TEXT("journal.list.next"));Next.Enabled=Scroll+PageSize<Entries.Num();Next.Align=TEXT("right");
    const auto* Selected=Entries.FindByPredicate([&](const auto& R){return FName(*Text(R,TEXT("id")))==Selection;});
    if(Selected && JournalEntryKnown(*Selected))
    {
        const auto R=*Selected;
        Add(TEXT("journal.detail.name"),TEXT("text"),Text(R,TEXT("name")),{788,158},{808,66},28).Color=Color(TEXT("gold"));
        if(Quests)
        {
            const auto Location=Find(TEXT("locations"),Text(R,TEXT("location")));
            Add(TEXT("journal.detail.location"),TEXT("text"),TEXT("相关地点 · ")+Text(Location,TEXT("name")),{788,240},{808,38},17).Color=Color(TEXT("muted"));
            Add(TEXT("journal.detail.objectiveHeading"),TEXT("text"),TEXT("当前目标"),{788,294},{390,42},18);
            Add(TEXT("journal.detail.progress"),TEXT("text"),FString::Printf(TEXT("进度 %d / %.0f"),G->QuestProgress(Selection),Number(R,TEXT("required"))),{1330,298},{266,36},16).Align=TEXT("right");
            Add(TEXT("journal.detail.objectiveRule"),TEXT("line"),TEXT(""),{788,338},{808,1}).Color=Color(TEXT("bronze"))*.55f;
            Add(TEXT("journal.detail.objective"),TEXT("text"),Text(R,TEXT("objective")),{788,359},{808,144},19).Color=Color(TEXT("gold"));
            Add(TEXT("journal.detail.descriptionHeading"),TEXT("text"),TEXT("任务详情"),{788,516},{390,42},18);
            Add(TEXT("journal.detail.reward"),TEXT("text"),FString::Printf(TEXT("经验值 +%.0f"),Number(R,TEXT("xp"))),{1315,520},{281,36},16).Align=TEXT("right");
            Add(TEXT("journal.detail.descriptionRule"),TEXT("line"),TEXT(""),{788,560},{808,1}).Color=Color(TEXT("bronze"))*.55f;
            Add(TEXT("journal.detail.description"),TEXT("text"),Text(R,TEXT("description")),{788,580},{808,130},18);
            Add(TEXT("journal.detail.companion"),TEXT("text"),Text(R,TEXT("companion")),{788,716},{808,72},15).Color=Color(TEXT("muted"));
            Add(TEXT("journal.detail.claim"),TEXT("menuAction"),G->Claimed.Contains(Selection)?TEXT("已完成"):(Selection==TEXT("side_06") || Selection==TEXT("side_09"))?TEXT("交付 6 份并领奖"):TEXT("领取奖励"),{788,800},{280,42},17,TEXT("claim")).Enabled=!G->Claimed.Contains(Selection);
            Add(TEXT("journal.detail.track"),TEXT("menuAction"),G->TrackedQuest==Selection?TEXT("取消追踪"):TEXT("追踪任务"),{1100,800},{244,42},17,TEXT("track"));
            Add(TEXT("journal.detail.map"),TEXT("menuAction"),TEXT("地图定位"),{1383,800},{213,42},17,TEXT("questMap")).Align=TEXT("right");
        }
        else
        {
            Add(TEXT("journal.detail.category"),TEXT("text"),Heading+TEXT(" · 已收录"),{788,240},{808,38},17).Color=Color(TEXT("muted"));
            Add(TEXT("journal.detail.descriptionHeading"),TEXT("text"),Collection?TEXT("物品记录"):TEXT("旅途记录"),{788,294},{808,42},18);
            Add(TEXT("journal.detail.descriptionRule"),TEXT("line"),TEXT(""),{788,338},{808,1}).Color=Color(TEXT("bronze"))*.55f;
            Add(TEXT("journal.detail.description"),TEXT("text"),Text(R,TEXT("description")),{788,359},{808,246},19);
            Add(TEXT("journal.detail.icon"),TEXT("image"),TEXT(""),{788,650},{96,96},18,TEXT(""),Text(R,TEXT("icon")));
            Add(TEXT("journal.detail.held"),TEXT("text"),Collection?FString::Printf(TEXT("当前持有 %d 件"),Inventory()->GetItemCount(Selection)):TEXT("已收录"),{916,666},{680,64},21);
        }
    }
    Add(TEXT("journal.footer.back"),TEXT("menuAction"),TEXT("Esc 返回"),{40,883},{150,44},18,TEXT("back"));
    const auto& Bindings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings;
    const FString CategoryKeys=HearthwardInput::Label(Bindings,TEXT("journal.category.prev"))+TEXT(" / ")+HearthwardInput::Label(Bindings,TEXT("journal.category.next"));
    const FString Help=TEXT("滚轮 浏览 · ")+CategoryKeys+TEXT(" 分类 · ")+(Quests?
        HearthwardInput::Label(Bindings,TEXT("journal.locate"))+TEXT(" 地图 · ")+HearthwardInput::Label(Bindings,TEXT("journal.track"))+TEXT(" 追踪"):FString(TEXT("Enter 选择")));
    Add(TEXT("journal.footer.help"),TEXT("text"),Message.IsEmpty()?Help:Message,{230,883},{1178,44},16).Color=Color(Message.IsEmpty()?TEXT("muted"):TEXT("gold"));
    Add(TEXT("journal.footer.settings"),TEXT("menuAction"),TEXT("设置"),{1472,883},{160,44},18,TEXT("page:settings")).Align=TEXT("right");
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

    // Vitals and companion stay fixed; the quest occupies a transient area below the companion.
    const float Values[]={G->Health,G->Hunger,G->Stamina},Maximum[]={G->MaxHealth(),100,G->MaxStamina()};
    const TCHAR* Labels[]={TEXT("生命"),TEXT("饱食"),TEXT("体力")},*Colors[]={TEXT("health"),TEXT("hunger"),TEXT("stamina")};
    const float VitalTop=28,VitalPitch=30*TextScale,VitalHeight=24*TextScale,VitalInset=3*TextScale;
    for(int32 I=0;I<3;++I)
    {
        const float Y=VitalTop+I*VitalPitch,BarX=48+90*TextScale,BarWidth=260*TextScale;
        const FString Id=FString::Printf(TEXT("hud.vitals.%d"),I);
        HUD(TEXT("text"),Labels[I],{48,Y},{84*TextScale,30*TextScale},20,Id+TEXT(".label"),TEXT("hud.vitals")).Color=Color(Colors[I]);
        auto& Bar=HUD(TEXT("hudVitalBar"),TEXT(""),{BarX,Y+VitalInset},{BarWidth,VitalHeight},18,Id+TEXT(".bar"),TEXT("hud.vitals"));
        Bar.Color=Color(Colors[I]); Bar.Value=Maximum[I]>0?Values[I]/Maximum[I]:0;
        HUD(TEXT("text"),FString::Printf(TEXT("%.0f / %.0f"),Values[I],Maximum[I]),{BarX+BarWidth+14,Y},{180,30*TextScale},18,Id+TEXT(".value"),TEXT("hud.vitals"));
    }
    const float CompanionY=VitalTop+2*VitalPitch+VitalInset+VitalHeight+14*TextScale,CompanionTextX=48+106*TextScale;
    const float QuestY=CompanionY+114*TextScale+24,DescriptionY=QuestY+45*TextScale,DescriptionHeight=96*TextScale;
    const auto Quest=Find(TEXT("quests"),G->TrackedQuest.ToString());
    if(GetHUDQuestNoticeRemaining()>0)
    {
        const auto Notice=Find(TEXT("quests"),HUDQuestNotice.ToString());
        HUD(TEXT("text"),TEXT("◇  ")+Text(Notice,TEXT("name")),{48,QuestY},{650,42*TextScale},24,TEXT("hud.quest.heading"),TEXT("hud.quest")).Color=Color(TEXT("gold"));
        HUD(TEXT("text"),Text(Notice,TEXT("objective")),{72,DescriptionY},{650,DescriptionHeight},20,TEXT("hud.quest.objective"),TEXT("hud.quest"));
    }

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
        const int32 Index=(HUDQuickSelection+Offset)%Slots.Num(); const FString RoleId=Slots[Index]->AsString(),ItemId=G->QuickItem(Index).ToString();
        const auto Item=Find(TEXT("items"),ItemId); const bool Selected=Offset==0;
        const FString Art=Text(Item,TEXT("quickIcon"));
        auto& E=HUD(TEXT("hudQuickItem"),FString::FromInt(Inventory()->GetItemCount(FName(*ItemId))),Positions[Offset],Selected?FVector2D(104,112):FVector2D(80,88),20,TEXT("hud.quick.")+RoleId,TEXT("hud.quickslots"));
        E.Id=ItemId; E.Asset=Art.IsEmpty()?Text(Item,TEXT("icon")):Art; E.Selected=Selected;
        E.Color=Selected?FLinearColor(1.f,.96f,.88f,1.f):FLinearColor(.58f,.56f,.53f,1.f);
        const FName Actions[]={TEXT("survival.medicine"),TEXT("survival.food"),TEXT("combat.ammunition"),TEXT("combat.throw")};
        const auto* Settings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
        const auto& Keys=Settings->Bindings.FindChecked(Actions[Index]);
        const FString Key=Keys[0].Key.IsValid()?Keys[0].Label():(Keys[1].Key.IsValid()?Keys[1].Label():TEXT("—"));
        auto& Shortcut=HUD(TEXT("text"),Key,Positions[Offset]+FVector2D(6,3),{50,28},18,TEXT("hud.quick.")+RoleId+TEXT(".key"),TEXT("hud.quickslots"));
        Shortcut.Color=Selected?Color(TEXT("gold")):FLinearColor(.78f,.74f,.64f,1.f);
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
    const auto Clock=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot();
    const int32 Time=Clock.MinuteOfDay,Hour=(Time/60)%24;
    HUD(TEXT("text"),FString::Printf(TEXT("第 %lld 天  %02d:%02d  %s"),Clock.DisplayDay,Hour,Time%60,Hour<6 || Hour>=18?TEXT("夜"):TEXT("晴")),{1260,31},{364,50},18,TEXT("hud.clock"),TEXT("hud.feedback")).Align=TEXT("right");

    // Action results expire after two seconds; active progress remains tied to its real state.
    float FeedbackY=388; TSet<FString> Seen;
    auto Feedback=[&](FString Label,const TCHAR* Id,uint32 Revision=0,bool Transient=true,bool Eligible=true)
    {
        Label=Label.TrimStartAndEnd();
        const double Deadline=Transient?HUDFeedbackDeadline(FName(Id),Label,Revision,Eligible):0;
        if(Label.IsEmpty() || !Eligible || (Transient && Deadline<=FPlatformTime::Seconds()) || Seen.Contains(Label)) return;
        Seen.Add(Label);
        HUD(TEXT("text"),Label,{1120,FeedbackY},{504,90*TextScale},18,Id,TEXT("hud.feedback")).FeedbackUntil=Deadline;
        FeedbackY+=98*TextScale;
    };
    const auto* Combat=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardCombatComponent>();
    if(Combat)
    {
        const FString Description=Combat->Describe();
        Feedback(Description,TEXT("hud.combat.state"),Combat->GetFeedbackRevision(),Description==Combat->Feedback);
        if(Combat->Discovery>0)
        {auto& Bar=HUD(TEXT("bar"),TEXT(""),{1120,358},{504,8},18,TEXT("hud.combat.discovery"),TEXT("hud.feedback"));Bar.Value=Combat->Discovery;Bar.Color=Color(TEXT("gold"));}
    }
    auto DiscoveryInRange=[&](const FString& Label)
    {
        if(!Label.StartsWith(TEXT("发现："))) return true;
        const FString Subject=Label.Mid(3).TrimStartAndEnd();
        for(const auto& V:Rows(TEXT("locations")))
        {
            const auto Location=V->AsObject(); const FName Id(*Text(Location,TEXT("id")));
            if(Text(Location,TEXT("name"))!=Subject || !G->Discovered.Contains(Id)) continue;
            if(Natural && Id!=TEXT("camp") && (!Campaign->Active() || !Campaign->HasLocation(Id))) continue;
            if(FVector::DistSquared(GetOwningPlayerPawn()->GetActorLocation(),G->LocationPosition(Id))<=FMath::Square(1000.)) return true;
        }
        return false;
    };
    const FString GameplayFeedback=G->Feedback.TrimStartAndEnd();
    Feedback(GameplayFeedback,TEXT("hud.gameplay.feedback"),G->GetFeedbackRevision(),true,DiscoveryInRange(GameplayFeedback));
    if(const auto* B=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardBuildingComponent>()) Feedback(B->Feedback,TEXT("hud.construction.feedback"),B->GetFeedbackRevision());
    if(const auto* S=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardSurvivalComponent>())
    {
        const FString Description=S->Describe();
        Feedback(Description,TEXT("hud.survival.state"),S->GetFeedbackRevision(),Description==S->Status);
        if(S->State.Life==EHearthwardLife::Downed) HUD(TEXT("text"),TEXT("等待弟弟救援 · Esc 菜单可选择放弃"),{560,700},{720,90*TextScale},20,TEXT("hud.survival.downed"),TEXT("hud.feedback"));
    }
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
    {
        if(const auto* S=It->FindComponentByClass<UHearthwardSurvivalComponent>();S && S->State.Life==EHearthwardLife::Downed)
            {
            const auto* PlayerSurvival=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardSurvivalComponent>();
            const FString Reason=PlayerSurvival->RescueBlockReason(S);
            const auto& Bindings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings;
            HUD(TEXT("text"),Reason.IsEmpty()?HearthwardInput::Label(Bindings,TEXT("interact"))+TEXT(" 扶起弟弟（5秒；移动或受伤中断）"):Reason,
                {560,600},{720,90*TextScale},20,TEXT("hud.companion.downed"),TEXT("hud.feedback"));
        }
        break;
    }
    if(const auto* Traversal=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardTraversalComponent>()) Feedback(Traversal->GetStatus(),TEXT("hud.traversal.state"),Traversal->GetFeedbackRevision(),!Traversal->IsVaulting());
    if(const auto* Interaction=GetOwningPlayerPawn()->FindComponentByClass<UHearthwardInteractionComponent>())
        Feedback(Interaction->GetCompletionFeedback(),TEXT("hud.interaction.feedback"),Interaction->GetFeedbackRevision());
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
    if(!Find(TEXT("buildings"),SelectedBuilding.ToString()) && !Buildings.IsEmpty())SelectedBuilding=FName(*Text(Buildings[0]->AsObject(),TEXT("id")));
    const float Scale=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale/100.f;
    auto Add=[&](FString Id,FString Type,FString Label,FVector2D P,FVector2D Size,float Font=18,FString Action=FString(),bool Selected=false)->FHearthwardUIElement&
    {
        Element(Type,Label,P,Size,Font,Action,TEXT(""),Selected);
        auto& E=Elements.Last();E.LayoutId=Id;E.Font=Font*Scale;E.FontRole=TEXT("body");E.Tracking=0;E.TextInset=0;return E;
    };
    auto Rule=[&](FString Id,float X,float Y,float W){Add(Id,TEXT("line"),TEXT(""),{X,Y},{W,1}).Color=Color(TEXT("bronze"))*.55f;};
    Add(TEXT("background"),TEXT("journalBackdrop"),TEXT(""),{0,0},DesignSize);
    Add(TEXT("building.heading"),TEXT("text"),TEXT("营地建造"),{42,32},{850,66},36);
    Add(TEXT("building.camp"),TEXT("menuAction"),TEXT("营地管理 ›"),{1300,40},{310,54},20,TEXT("page:camp")).Align=TEXT("right");
    Rule(TEXT("building.header.rule"),40,112,1592);
    Add(TEXT("building.list.surface"),TEXT("menuSurface"),TEXT(""),{40,136},{664,706});
    Add(TEXT("building.detail.surface"),TEXT("menuSurface"),TEXT(""),{752,136},{880,706});
    Add(TEXT("building.list.heading"),TEXT("text"),TEXT("建造物"),{64,160},{390,44},26);
    Rule(TEXT("building.list.rule"),64,214,616);
    for(int32 I=Scroll;I<FMath::Min(Scroll+4,Buildings.Num());++I)
    {
        const auto R=Buildings[I]->AsObject();const FString Id=Text(R,TEXT("id"));const bool Selected=SelectedBuilding==FName(*Id);
        const float Y=225+(I-Scroll)*86;
        Add(TEXT("building.list.row.")+Id,TEXT("menuRow"),TEXT(""),{64,Y},{616,80},18,TEXT("building.select:")+Id,Selected);
        Add(TEXT("building.list.name.")+Id,TEXT("text"),(Selected?TEXT("◆  "):TEXT("◇  "))+Text(R,TEXT("name")),{80,Y+12},{580,44},23).Color=Color(Selected?TEXT("gold"):TEXT("text"));
    }
    Add(TEXT("building.prev"),TEXT("menuAction"),TEXT("‹ 上一页"),{64,798},{160,38},16,TEXT("buildPrev")).Enabled=Scroll>0;
    Add(TEXT("building.range"),TEXT("text"),FString::Printf(TEXT("%d – %d / %d"),Scroll+1,FMath::Min(Scroll+4,Buildings.Num()),Buildings.Num()),{244,806},{248,30},14).Align=TEXT("center");
    auto& Next=Add(TEXT("building.next"),TEXT("menuAction"),TEXT("下一页 ›"),{518,798},{162,38},16,TEXT("buildNext"));Next.Enabled=Scroll+4<Buildings.Num();Next.Align=TEXT("right");
    const auto R=Find(TEXT("buildings"),SelectedBuilding.ToString());if(!R)return;
    Add(TEXT("building.detail.name"),TEXT("text"),Text(R,TEXT("name")),{788,160},{800,56},28).Color=Color(TEXT("gold"));
    Add(TEXT("building.detail.location"),TEXT("text"),R->GetBoolField(TEXT("wilderness"))?TEXT("建造范围 · 营地与野外"):TEXT("建造范围 · 营地"),{788,244},{808,42},17).Color=Color(TEXT("muted"));
    Add(TEXT("building.detail.materialHeading"),TEXT("text"),TEXT("所需材料"),{788,306},{440,44},21);
    Add(TEXT("building.detail.materialColumns"),TEXT("text"),TEXT("可用 / 需求"),{1280,308},{316,42},17).Align=TEXT("right");
    Rule(TEXT("building.detail.materialRule"),788,350,808);
    int32 MaterialIndex=0;
    for(const auto& M:R->GetObjectField(TEXT("materials"))->Values)
    {
        const float Y=370+MaterialIndex++*50;
        Add(FString(TEXT("building.material.name."))+FString(*M.Key),TEXT("text"),Text(Find(TEXT("items"),FString(*M.Key)),TEXT("name")),{788,Y},{440,44},20);
        const int32 Available=Inventory()->Available(FName(*M.Key))+GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->Available(FName(*M.Key));
        Add(FString(TEXT("building.material.count."))+FString(*M.Key),TEXT("text"),FString::Printf(TEXT("%d / %.0f"),Available,M.Value->AsNumber()),{1250,Y},{346,44},20).Align=TEXT("right");
    }
    Add(TEXT("building.detail.descriptionHeading"),TEXT("text"),TEXT("建造说明"),{788,536},{808,44},21);
    Rule(TEXT("building.detail.descriptionRule"),788,582,808);
    Add(TEXT("building.detail.description"),TEXT("text"),Text(R,TEXT("description")),{788,604},{808,112},20);
    const bool Unlocked=HearthwardCamp::RequiredTier(SelectedBuilding,1)<=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Tier;
    Add(TEXT("building.detail.status"),TEXT("text"),Unlocked?TEXT("优先取用背包材料，差额使用共享仓储。"):TEXT("营地等级不足，暂不可建造。"),{788,732},{808,42},16).Color=Color(TEXT("muted"));
    Add(TEXT("building.submit"),TEXT("menuAction"),TEXT("开始放置 ›"),{1290,790},{306,44},21,TEXT("build:")+SelectedBuilding.ToString()).Enabled=Unlocked;
    Add(TEXT("building.back"),TEXT("menuAction"),TEXT("Esc 返回"),{40,880},{220,44},20,TEXT("back"));
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
