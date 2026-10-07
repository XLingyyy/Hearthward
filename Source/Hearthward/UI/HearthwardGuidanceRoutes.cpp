#include "HearthwardGuidanceRoutes.h"
#include "../Building/HearthwardHometownFortress.h"
#include "../Campaign/HearthwardCampaignSubsystem.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

FHearthwardRouteStep HearthwardGuidanceRoutes::Resolve(UWorld* World,FName Quest,FName Destination,FVector PlayerPosition)
{
    FHearthwardRouteStep Step;if(!World)return Step;
    static const TSharedPtr<FJsonObject> Config=[]
    {
        FString Source;TSharedPtr<FJsonObject> Result;
        if(!FFileHelper::LoadFileToString(Source,*(FPaths::ProjectDir()/TEXT("Resources/Data/quest_guidance.json")))
            || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),Result))return TSharedPtr<FJsonObject>();
        return Result;
    }();
    if(!Config)return Step;
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();
    if(!Campaign || !Campaign->Active())return Step;
    const auto Show=[&](FName Id,FVector At)
    {
        const TSharedPtr<FJsonObject>* Labels=nullptr;
        if(!Config->TryGetObjectField(TEXT("labels"),Labels) || !Labels)return;
        const FString Label=HearthwardData::Text(*Labels,Id.ToString());if(Label.IsEmpty())return;
        Step.Visible=true;Step.Id=Id;Step.Label=Label;Step.World=At;
    };
    if(Quest==TEXT("main_01") && Destination==TEXT("prologue_exit") && Campaign->State.Phase==TEXT("prologue"))
    {
        AHearthwardHometownFortress* Home=nullptr;
        for(TActorIterator<AHearthwardHometownFortress> It(World);It;++It)if(IsValid(*It)){Home=*It;break;}
        if(!Home)return Step;
        const FVector Origin=Home->GetActorLocation(),Local=PlayerPosition-Origin;
        const float Floor=Home->BedroomLanding().Z-Origin.Z-100;
        if(FMath::Abs(Local.Z-(Floor+100))<180)
        {
            if(FMath::Abs(Local.X)<700 && Local.Y>-650 && Local.Y<600)
                Show(TEXT("bedroom_door"),Origin+FVector(0,650,Floor+100));
            else if(Local.X>-700 && Local.X<1900 && Local.Y>=600 && Local.Y<1100
                && !(FMath::Abs(Local.X-1500)<300 && Local.Y>=1000))
                Show(TEXT("escape_stair_top"),Origin+FVector(1500,1050,Floor+100));
        }
        if(Step.Visible)return Step;
        TArray<UStaticMeshComponent*> Parts;Home->GetComponents(Parts);UStaticMeshComponent* Bottom=nullptr;
        for(auto* Part:Parts)if(Part->ComponentHasTag(TEXT("EscapeStair"))
            && (!Bottom || Part->Bounds.Origin.Y>Bottom->Bounds.Origin.Y))Bottom=Part;
        if(!Bottom)return Step;
        const FVector Exit=Bottom->Bounds.Origin+FVector(0,Bottom->Bounds.BoxExtent.Y+100,Bottom->Bounds.BoxExtent.Z+100);
        if(FMath::Abs(Local.X-1500)<300 && Local.Y>=1000 && PlayerPosition.Y<Exit.Y-150)
            Show(TEXT("escape_stair_bottom"),Exit);
        else if(Local.X>-2200 && Local.X<2500 && Local.Y>=1100 && Local.Y<6500)
        {
            FVector Ground;
            if(Campaign->Ground(Origin+FVector(-2200,2800,0),Ground)
                && FVector::Dist(PlayerPosition,Ground+FVector(0,0,100))>50)
                Show(TEXT("courtyard_side_gate"),Ground+FVector(0,0,100));
        }
    }
    else if(Quest==TEXT("main_03") && Destination==TEXT("slice_rescue") && Campaign->State.Phase==TEXT("occupied"))
    {
        // Only a known, existing junction is offered, and only before passing it along the first outbound segment.
        const auto* Gameplay=World->GetFirstPlayerController() && World->GetFirstPlayerController()->GetPawn()
            ?World->GetFirstPlayerController()->GetPawn()->FindComponentByClass<UHearthwardGameplayComponent>():nullptr;
        if(!Gameplay || !Gameplay->Discovered.Contains(TEXT("route_fork")))return Step;
        const FVector Camp=Campaign->Position(TEXT("camp")),Fork=Campaign->Position(TEXT("route_fork"));
        const FVector2D Leg(Fork.X-Camp.X,Fork.Y-Camp.Y),Offset(PlayerPosition.X-Camp.X,PlayerPosition.Y-Camp.Y);
        if(Leg.SizeSquared()>0)
        {
            const double T=FVector2D::DotProduct(Offset,Leg)/Leg.SizeSquared();
            const double Lateral=(Offset-Leg*T).Size();
            if(T>=0 && T<.92 && Lateral<5000 && FVector::Dist2D(PlayerPosition,Fork)>1000)
                Show(TEXT("route_fork"),Fork);
        }
    }
    return Step;
}
