// PROTOTYPE_ONLY: explicit Development verification in a disposable GUID pool.
#if !UE_BUILD_SHIPPING
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Campaign/HearthwardCampaignActor.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
namespace
{
struct FMapTravelVerification078
{
    FString Output;int32 Step=0;bool Passed=true;
    double Started=FPlatformTime::Seconds(),WaitUntil=0;
    FVector Before,BrotherBefore;double ActiveBefore=0,CalendarBefore=0;
    TWeakObjectPtr<AActor> FordSource;
    TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>(),States=MakeShared<FJsonObject>();
    void Check(const FString& Name,bool Value)
    {Checks->SetBoolField(Name,Value);Passed&=Value;UE_LOG(LogTemp,Log,TEXT("Map travel %s: %s"),*Name,Value?TEXT("PASS"):TEXT("FAIL"));}
    bool Finish()
    {
        if(FordSource.IsValid())FordSource->Destroy();
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("passed"),Passed);J->SetObjectField(TEXT("checks"),Checks);J->SetObjectField(TEXT("states"),States);
        J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        J->SetStringField(TEXT("scope"),TEXT("Real new-game widget, actual prologue interactions and escape, real ground/streaming/navigation, public map travel in both directions and invalid-ground rejection. Disposable GUID fixture seeds discovered locations, preloads the ford, and repositions actors for interaction; no Windows physical-input or player-save claim."));
        FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));
        FFileHelper::SaveStringToFile(Text,*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);return false;
    }
};
bool VerifyMapTravel078(float)
{
    static FMapTravelVerification078 Run;
    if(Run.Output.IsEmpty())
    {
        FString Pool;FGuid Id;
        if(!FParse::Value(FCommandLine::Get(),TEXT("HearthwardMapTravelVerify="),Run.Output)
            || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id) || !Id.IsValid())return false;
    }
    const double Now=FPlatformTime::Seconds();
    if(Now-Run.Started>220){Run.Check(TEXT("completion_within_timeout"),false);return Run.Finish();}
    if(Now<Run.WaitUntil || !GEngine)return true;
    UWorld* World=nullptr;for(const auto& Context:GEngine->GetWorldContexts())if(Context.WorldType==EWorldType::Game && Context.World()){World=Context.World();break;}
    auto* PC=World?World->GetFirstPlayerController():nullptr;auto* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;auto* UI=HUD?HUD->Screen.Get():nullptr;
    if(!UI || !Pawn || !World->HasBegunPlay())return true;
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();auto* G=Pawn->FindComponentByClass<UHearthwardGameplayComponent>();
    auto* Loading=World->GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>();auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
    if(!Campaign->Active() || !Campaign->State.Facts.Contains(TEXT("prologue_intro")))return true;
    AHearthwardCompanionFixture* Brother=nullptr;for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It){Brother=*It;break;}
    const FString Folder=FPaths::GetPath(Run.Output);
    auto Snapshot=[&](){TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),J);return J;};
    auto Find=[&](const FString& Id){const auto J=Snapshot();for(const auto& Value:J->GetArrayField(TEXT("elements")))if(Value->AsObject()->GetStringField(TEXT("id"))==Id)return Value->AsObject();return TSharedPtr<FJsonObject>();};
    auto Trace=[&](const TCHAR* Name)
    {
        auto J=Snapshot();J->SetStringField(TEXT("campaignFeedback"),Campaign->Feedback);J->SetStringField(TEXT("gameplayFeedback"),G->Feedback);
        J->SetBoolField(TEXT("paused"),World->IsPaused());J->SetBoolField(TEXT("traveling"),Campaign->IsTraveling());J->SetBoolField(TEXT("loading"),Loading->IsLoading());
        J->SetArrayField(TEXT("playerCm"),{MakeShared<FJsonValueNumber>(Pawn->GetActorLocation().X),MakeShared<FJsonValueNumber>(Pawn->GetActorLocation().Y),MakeShared<FJsonValueNumber>(Pawn->GetActorLocation().Z)});
        Run.States->SetObjectField(Name,J);
    };
    auto Capture=[&](const TCHAR* Name,int32 Width=1600,int32 Height=1000)
    {
        IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Task020")),true);
        const FString Id=FPaths::GetBaseFilename(Folder)+TEXT("-")+Name;
        Run.Check(FString(TEXT("capture_"))+Name,UI->CaptureUI(Id,Width,Height)
            && IFileManager::Get().Copy(*(Folder/(FString(Name)+TEXT(".png"))),*(FPaths::ProjectSavedDir()/TEXT("Task020")/(Id+TEXT(".png"))))==COPY_OK);
    };
    if(Loading->IsLoading())
    {
        if(Run.Step==5 || Run.Step==7)
        {
            const auto Time=Clock->GetSnapshot();
            Run.Check(FString::Printf(TEXT("travel_%d_loading_freezes_clock"),Run.Step),FMath::Abs(Time.ActivePlaySeconds-Run.ActiveBefore)<.01 && FMath::Abs(Time.ElapsedCalendarMinutes-Run.CalendarBefore)<.01);
        }
        return true;
    }
    auto StartTrip=[&](FName Destination,int32 Next)
    {
        UI->OpenPage(TEXT("map"));UI->ExecuteAction(TEXT("map.world"));UI->ExecuteAction(TEXT("location:")+Destination.ToString());
        Run.Check(FString::Printf(TEXT("trip_%d_map_owns_pause"),Next),World->IsPaused());
        const auto Time=Clock->GetSnapshot();Run.ActiveBefore=Time.ActivePlaySeconds;Run.CalendarBefore=Time.ElapsedCalendarMinutes;
        Run.Before=Pawn->GetActorLocation();Run.BrotherBefore=Brother?Brother->GetActorLocation():FVector::ZeroVector;
        Run.Check(FString::Printf(TEXT("trip_%d_public_map_accepts_travel"),Next),UI->ExecuteAction(TEXT("travel")));
        Run.Check(FString::Printf(TEXT("trip_%d_resumes_world_and_closes_map"),Next),UI->GetPage()==TEXT("hud") && !World->IsPaused());
        Run.Check(FString::Printf(TEXT("trip_%d_actual_campaign_loading_started"),Next),Campaign->IsTraveling() && Loading->IsLoading());
        Run.Step=Next;Run.WaitUntil=Now+.05;
    };
    switch(Run.Step)
    {
    case 0:
    {
        for(const FName Id:{FName(TEXT("prologue_relic")),FName(TEXT("loot_dwellings")),FName(TEXT("camp_hunter")),FName(TEXT("camp_records")),FName(TEXT("loc_dwellings"))})G->Discovered.Add(Id);
        UI->OpenPage(TEXT("map"));UI->ExecuteAction(TEXT("map.world"));UI->ExecuteAction(TEXT("location:loc_dwellings"));
        Run.Check(TEXT("discovered_flag_has_actual_map_marker"),Find(TEXT("map.location.marker.loc_dwellings")).IsValid());
        const auto Flag=Find(TEXT("map.location.detail"));Run.Check(TEXT("flag_type_and_capture_guidance"),Flag && Flag->GetStringField(TEXT("text")).Contains(TEXT("占领旗帜")) && Flag->GetStringField(TEXT("text")).Contains(TEXT("5秒")));
        const auto S=Snapshot();bool Clean=true;
        for(const auto& V:S->GetArrayField(TEXT("elements"))){const auto E=V->AsObject();const FString Action=E->GetStringField(TEXT("action"));Clean&=Action!=TEXT("location:prologue_relic") && Action!=TEXT("location:loot_dwellings") && Action!=TEXT("location:camp_hunter") && Action!=TEXT("location:camp_records");}
        Run.Check(TEXT("task_objects_and_people_absent_from_place_list"),Clean);Trace(TEXT("flag"));Capture(TEXT("flag-guide"));
        auto* Settings=World->GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();Settings->Comfort.TextScale=150;UI->Refresh();Capture(TEXT("flag-guide-150"));Settings->Comfort.TextScale=100;
        UI->ExecuteAction(TEXT("location:camp"));Run.Before=Pawn->GetActorLocation();Run.Check(TEXT("prologue_travel_rejected"),!UI->ExecuteAction(TEXT("travel")));
        Run.Check(TEXT("prologue_rejection_has_current_reason"),G->Feedback.Contains(TEXT("序章")) && G->Feedback.Contains(TEXT("护符")) && !G->Feedback.Contains(TEXT("正在准备目的地")));
        Run.Check(TEXT("rejected_travel_preserves_map_pause_and_position"),UI->GetPage()==TEXT("map") && World->IsPaused() && Pawn->GetActorLocation().Equals(Run.Before,.01));
        Trace(TEXT("prologue_rejection"));Capture(TEXT("travel-prologue-reason"));UI->OpenPage(TEXT("hud"));
        Pawn->SetActorLocation(Campaign->Position(TEXT("prologue_relic")));Run.WaitUntil=Now+.1;Run.Step=1;break;
    }
    case 1:
        Run.Check(TEXT("real_relic_interaction_still_works"),Campaign->Interact() && Campaign->State.Facts.Contains(TEXT("relic")));
        Run.Check(TEXT("real_brother_follow_order_still_works"),G->OrderCompanion(TEXT("follow")) && Campaign->State.Facts.Contains(TEXT("prologue_order")));
        Pawn->SetActorLocation(Campaign->Position(TEXT("prologue_exit")));if(Brother)Brother->SetActorLocation(Pawn->GetActorLocation()+FVector(180,0,0));
        Run.WaitUntil=Now+.1;Run.Step=2;break;
    case 2:
        Run.Check(TEXT("actual_back_alley_escape_starts"),Campaign->Interact() && Campaign->IsTraveling());Run.Step=3;break;
    case 3:
    {
        Run.Check(TEXT("actual_escape_arrives_at_camp"),Campaign->State.Phase==TEXT("occupied") && FVector::Dist2D(Pawn->GetActorLocation(),Campaign->Position(TEXT("camp")))<300);
        G->OrderCompanion(TEXT("wait"));G->Discovered.Add(TEXT("route_ford"));G->Activated.Remove(TEXT("route_ford"));
        UI->OpenPage(TEXT("map"));UI->ExecuteAction(TEXT("map.world"));UI->ExecuteAction(TEXT("location:route_ford"));Run.Before=Pawn->GetActorLocation();
        Run.Check(TEXT("inactive_ford_rejected_with_activation_instructions"),!UI->ExecuteAction(TEXT("travel")) && G->Feedback.Contains(TEXT("渡口")) && G->Feedback.Contains(TEXT("按 E")));
        Run.Check(TEXT("inactive_destination_keeps_world_and_map"),World->IsPaused() && !Campaign->IsTraveling() && Pawn->GetActorLocation().Equals(Run.Before,.01));
        Trace(TEXT("inactive_ford"));Capture(TEXT("travel-activation-reason"));UI->OpenPage(TEXT("hud"));
        auto* Source=World->SpawnActor<AActor>();Run.FordSource=Source;
        auto* Root=NewObject<USceneComponent>(Source);Source->AddInstanceComponent(Root);Source->SetRootComponent(Root);Root->RegisterComponent();Source->SetActorLocation(Campaign->Position(TEXT("route_ford")));
        auto* Stream=NewObject<UWorldPartitionStreamingSourceComponent>(Source);Source->AddInstanceComponent(Stream);Stream->RegisterComponent();Stream->EnableStreamingSource();Run.Step=4;Run.WaitUntil=Now+.5;break;
    }
    case 4:
    {
        FVector Floor;auto* Stream=Run.FordSource->FindComponentByClass<UWorldPartitionStreamingSourceComponent>();
        if(!Stream->IsStreamingCompleted() || !Campaign->Ground(Campaign->Position(TEXT("route_ford")),Floor))return true;
        Pawn->SetActorLocation(Floor+FVector(0,0,100));Campaign->State.Positions.Add(TEXT("route_ford"),Pawn->GetActorLocation());
        Run.Check(TEXT("ford_activation_uses_real_public_interaction"),Campaign->Interact() && G->Activated.Contains(TEXT("route_ford")));
        StartTrip(TEXT("camp"),5);break;
    }
    case 5:
        Run.Check(TEXT("ford_to_camp_really_arrived"),!Campaign->IsTraveling() && FVector::Dist2D(Pawn->GetActorLocation(),Campaign->Position(TEXT("camp")))<300 && G->Feedback.Contains(TEXT("已抵达")));
        Run.Check(TEXT("independent_brother_not_teleported"),Brother && FVector::Dist2D(Brother->GetActorLocation(),Run.BrotherBefore)<20);Trace(TEXT("arrived_camp"));
        UI->OpenPage(TEXT("map"));UI->ExecuteAction(TEXT("map.world"));UI->ExecuteAction(TEXT("location:camp"));
        Run.Check(TEXT("already_at_station_reason"),!UI->ExecuteAction(TEXT("travel")) && G->Feedback.Contains(TEXT("已在")));
        Trace(TEXT("same_station"));Capture(TEXT("travel-same-station"));UI->OpenPage(TEXT("hud"));Run.Step=6;Run.WaitUntil=Now+.1;break;
    case 6:StartTrip(TEXT("route_ford"),7);break;
    case 7:
        Run.Check(TEXT("camp_to_ford_really_arrived"),!Campaign->IsTraveling() && FVector::Dist2D(Pawn->GetActorLocation(),Campaign->Position(TEXT("route_ford")))<300 && G->Feedback.Contains(TEXT("已抵达")));
        Trace(TEXT("arrived_ford"));Run.Before=Pawn->GetActorLocation();
        // Deliberately invalid ground tests the same production landing transaction.
        Campaign->State.Positions.Add(TEXT("camp"),FVector(10000000,10000000,20000));StartTrip(TEXT("camp"),8);break;
    case 8:
        Run.Check(TEXT("invalid_ground_rejected_without_position_commit"),!Campaign->IsTraveling() && Pawn->GetActorLocation().Equals(Run.Before,.01) && G->Feedback.Contains(TEXT("传送已取消")));
        Trace(TEXT("invalid_ground"));return Run.Finish();
    }
    return true;
}
const FTSTicker::FDelegateHandle MapTravelTicker078=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&VerifyMapTravel078),.02f);
}
#endif
