#include "HearthwardQuestGuidance.h"
#include "HearthwardScreenWidget.h"
#include "HearthwardGuidanceRoutes.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"

FHearthwardQuestMarker HearthwardQuestGuidance::Resolve(APlayerController* Player,FName QuestId)
{
    FHearthwardQuestMarker M;
    APawn* Pawn=Player?Player->GetPawn():nullptr;
    const auto* G=Pawn?Pawn->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
    if(!G)return M;
    if(QuestId.IsNone())QuestId=G->TrackedQuest;
    if(QuestId.IsNone() || G->Claimed.Contains(QuestId) || !G->QuestAvailable(QuestId))return M;
    const auto* Campaign=Pawn->GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>();
    const auto Quest=HearthwardData::Find(TEXT("quests"),QuestId.ToString());
    if(!Quest)return M;
    M.Location=Campaign->Active()?Campaign->QuestLocation(QuestId):FName(*HearthwardData::Text(Quest,TEXT("location")));
    const auto Location=HearthwardData::Find(TEXT("locations"),M.Location.ToString());
    if(M.Location.IsNone() || (!Location && !Campaign->HasLocation(M.Location)))return M;
    if(HearthwardData::Text(Quest,TEXT("kind"))==TEXT("side") && !G->Discovered.Contains(M.Location))return M;
    // Once every step is complete, the next action is claiming in the journal, not revisiting the old destination.
    if(G->QuestProgress(QuestId)>=HearthwardData::Number(Quest,TEXT("required")))return M;
    M.World=G->LocationPosition(M.Location);
    M.Label=HearthwardData::Text(Location,TEXT("name"));
    if(M.Label.IsEmpty())M.Label=TEXT("任务地点");
    bool WaitingRescue=false;
    if(Campaign->Active() && QuestId==TEXT("main_03"))
    {
        const auto* Person=Campaign->State.People.FindByPredicate([](const auto& P){return P.Id==TEXT("rescued_01");});
        if(Person && Person->Stage==TEXT("waiting"))
        {
            const auto* Actor=Campaign->Actor(Person->Id);
            if(!IsValid(Actor) && !Person->Located)return M;
            M.World=IsValid(Actor)?Actor->GetActorLocation():Person->Position;
            M.Label=TEXT("等待中的获救者");WaitingRescue=true;
        }
    }
    M.Distance=FVector::Dist(Pawn->GetActorLocation(),M.World)/100.;
    M.Visible=true;
    if(WaitingRescue)return M;
    const auto Step=HearthwardGuidanceRoutes::Resolve(Pawn->GetWorld(),QuestId,M.Location,Pawn->GetActorLocation());
    M.HasRoute=Step.Visible;M.RouteId=Step.Id;M.RouteLabel=Step.Label;M.RouteWorld=Step.World;
    return M;
}

void HearthwardQuestGuidance::Place(FHearthwardQuestMarker& M,FVector2D Size,FVector2D Projected,bool InFront,FVector2D CameraDirection)
{
    const double Scale=FMath::Min(Size.X/1672.,Size.Y/941.);
    const FVector2D Center=Size*.5,Margin(140*Scale,125*Scale),Half=Center-Margin;
    M.Offscreen=!InFront || Projected.X<Margin.X || Projected.X>Size.X-Margin.X
        || Projected.Y<Margin.Y || Projected.Y>Size.Y-Margin.Y;
    if(!M.Offscreen){M.Position=Projected;M.Direction=FVector2D::ZeroVector;return;}
    FVector2D D=InFront?Projected-Center:CameraDirection;
    if(D.IsNearlyZero())D=FVector2D(0,1);
    M.Direction=D.GetSafeNormal();
    const double X=FMath::Abs(D.X)>UE_SMALL_NUMBER?Half.X/FMath::Abs(D.X):UE_BIG_NUMBER;
    const double Y=FMath::Abs(D.Y)>UE_SMALL_NUMBER?Half.Y/FMath::Abs(D.Y):UE_BIG_NUMBER;
    M.Position=Center+D*FMath::Min(X,Y);
}

void UHearthwardScreenWidget::PaintQuestGuidance(const FGeometry& G,FSlateWindowElementList& Out,int32 Layer) const
{
    if(Page!=TEXT("hud"))return;
    auto* Player=GetOwningPlayer();auto M=HearthwardQuestGuidance::Resolve(Player);
    if(!M.Visible)return;
    int32 Width=0,Height=0;Player->GetViewportSize(Width,Height);
    if(Width<=0 || Height<=0)return;
    FVector Eye;FRotator View;Player->GetPlayerViewPoint(Eye,View);
    TArray<FHearthwardQuestMarker> Markers={M};
    if(M.HasRoute)
    {
        auto Route=M;Route.World=M.RouteWorld;Route.Label=M.RouteLabel;
        Route.Distance=FVector::Dist(GetOwningPlayerPawn()->GetActorLocation(),Route.World)/100.;Markers.Add(Route);
    }
    for(int32 MarkerIndex=0;MarkerIndex<Markers.Num();++MarkerIndex)
    {
    M=Markers[MarkerIndex];
    const FVector Target=M.World+FVector(0,0,160),Delta=Target-Eye;
    FVector2D Screen=FVector2D::ZeroVector;
    const bool Front=Player->ProjectWorldLocationToScreen(Target,Screen,false);
    const FVector2D Size=G.GetLocalSize();
    Screen=Screen*FVector2D(Size.X/Width,Size.Y/Height);
    HearthwardQuestGuidance::Place(M,Size,Screen,Front,
        FVector2D(FVector::DotProduct(Delta,FRotationMatrix(View).GetUnitAxis(EAxis::Y)),-FVector::DotProduct(Delta,FRotationMatrix(View).GetUnitAxis(EAxis::Z))));
    const float Scale=FMath::Min(Size.X/1672.,Size.Y/941.);
    const auto Geometry=G.ToPaintGeometry(FVector2D(1,1),FSlateLayoutTransform(Scale,M.Position));
    const FLinearColor Gold=MarkerIndex==0?FLinearColor(1.f,.77f,.28f,1.f):FLinearColor(.3f,.88f,.92f,1.f);
    const FLinearColor Shadow(.025f,.018f,.008f,.9f);
    const TArray<FVector2D> Diamond={{0,-17},{12,0},{0,17},{-12,0},{0,-17}};
    FSlateDrawElement::MakeLines(Out,Layer,Geometry,Diamond,ESlateDrawEffect::None,Shadow,true,6);
    FSlateDrawElement::MakeLines(Out,Layer+1,Geometry,Diamond,ESlateDrawEffect::None,Gold,true,2.5f);
    const TArray<FVector2D> Core={{0,-6},{4,0},{0,6},{-4,0},{0,-6}};
    FSlateDrawElement::MakeLines(Out,Layer+1,Geometry,Core,ESlateDrawEffect::None,Gold,true,2);
    if(M.Offscreen)
    {
        const FVector2D D=M.Direction,N(-D.Y,D.X);
        const TArray<FVector2D> Arrow={D*23+N*6,D*31,D*23-N*6};
        FSlateDrawElement::MakeLines(Out,Layer,Geometry,Arrow,ESlateDrawEffect::None,Shadow,true,6);
        FSlateDrawElement::MakeLines(Out,Layer+1,Geometry,Arrow,ESlateDrawEffect::None,Gold,true,2.5f);
    }
    auto Caption=[&](FString Text,float Y,int32 FontSize,FLinearColor Tint)
    {
        FSlateFontInfo Font(Typeface,FontSize);
        const FVector2D Extent=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text,Font);
        const FVector2D P(-Extent.X*.5,Y);
        FSlateDrawElement::MakeText(Out,Layer+2,G.ToPaintGeometry(Extent,FSlateLayoutTransform(Scale,M.Position+(P+FVector2D(1,1))*Scale)),Text,Font,ESlateDrawEffect::None,Shadow);
        FSlateDrawElement::MakeText(Out,Layer+3,G.ToPaintGeometry(Extent,FSlateLayoutTransform(Scale,M.Position+P*Scale)),Text,Font,ESlateDrawEffect::None,Tint);
    };
    const float CaptionY=M.Offscreen && M.Direction.Y>.5?-65.f:23.f;
    Caption((MarkerIndex==0?FString():FString(TEXT("途经 · ")))+FString::Printf(TEXT("直线 %.0f 米"),M.Distance),CaptionY,18,Gold);
    Caption(M.Label,CaptionY+24,16,FLinearColor(1,.96f,.83f,1));
    }
}

FString UHearthwardScreenWidget::DescribeQuestGuidance() const
{
    const auto M=HearthwardQuestGuidance::Resolve(GetOwningPlayer());
    auto O=MakeShared<FJsonObject>();O->SetBoolField(TEXT("visible"),Page==TEXT("hud") && M.Visible);
    O->SetStringField(TEXT("location"),M.Location.ToString());O->SetStringField(TEXT("label"),M.Label);
    O->SetNumberField(TEXT("distance_m"),M.Distance);
    O->SetBoolField(TEXT("route_visible"),Page==TEXT("hud") && M.Visible && M.HasRoute);
    O->SetStringField(TEXT("route_id"),M.RouteId.ToString());O->SetStringField(TEXT("route_label"),M.RouteLabel);
    O->SetArrayField(TEXT("route_world"),{MakeShared<FJsonValueNumber>(M.RouteWorld.X),MakeShared<FJsonValueNumber>(M.RouteWorld.Y),MakeShared<FJsonValueNumber>(M.RouteWorld.Z)});
    O->SetArrayField(TEXT("world"),{MakeShared<FJsonValueNumber>(M.World.X),MakeShared<FJsonValueNumber>(M.World.Y),MakeShared<FJsonValueNumber>(M.World.Z)});
    FString Json;FJsonSerializer::Serialize(O,TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json));return Json;
}
