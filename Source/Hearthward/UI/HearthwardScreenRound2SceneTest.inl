// Explicit disposable-pool fixture; these results do not credit normal-route playtesting.
#if !UE_BUILD_SHIPPING
#include "../Environment/HearthwardShortcutActor.h"
#include "../Environment/HearthwardLampInteraction.h"
#include "../Environment/HearthwardWeatherActor.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "Components/PointLightComponent.h"
#include "../HearthwardCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "../Save/HearthwardSaveSubsystem.h"
namespace
{
bool VerifyRound2Scenes(float)
{
    static FString Output;
    static bool Reload=false;
    static FString Checkpoint;
    static FHearthwardPersistentInteractionState SavedInteractions;
    static auto Checks=MakeShared<FJsonObject>();
    static auto Details=MakeShared<FJsonObject>();
    static int32 Step=0,Site=0;
    static bool Passed=true;
    static double Started=FPlatformTime::Seconds(),Wait=0;
    static double PausedCalendar=0,PausedRain=0;
    static TWeakObjectPtr<AHearthwardShortcutActor> Beam,Winch,Gate,Ladder;
    static TWeakObjectPtr<AHearthwardShortcutActor> RetryDevice;
    static bool RetriedCheckpoint=false;
    if(Output.IsEmpty())
    {
        FString Pool;FGuid Id;
        if(!FParse::Value(FCommandLine::Get(),TEXT("HearthwardRound2SceneVerify="),Output)
            || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id) || !Id.IsValid())return false;
        Reload=FParse::Param(FCommandLine::Get(),TEXT("HearthwardRound2Reload"));
        FParse::Value(FCommandLine::Get(),TEXT("HearthwardRound2Checkpoint="),Checkpoint);
    }
    auto Check=[&](const TCHAR* Name,bool Value){Checks->SetBoolField(FString(Reload?TEXT("reload."):Site?TEXT("cellar."):TEXT("workshops."))+Name,Value);Passed&=Value;};
    auto Finish=[&]()
    {
        auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("passed"),Passed);Report->SetBoolField(TEXT("fixture"),true);
        Report->SetStringField(TEXT("scope"),TEXT("Rendered actual map geometry; fixture relocates participants and pre-clears enemies. No normal-route, combat or human acceptance."));
        Report->SetObjectField(TEXT("checks"),Checks);Report->SetObjectField(TEXT("details"),Details);
        FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
        FFileHelper::SaveStringToFile(Text,*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);return false;
    };
    const double Now=FPlatformTime::Seconds();if(Now-Started>240){Check(TEXT("completed_before_timeout"),false);return Finish();}
    UWorld* World=nullptr;
    if(GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(Context.WorldType==EWorldType::Game){World=Context.World();break;}
    auto* PC=World?World->GetFirstPlayerController():nullptr;
    auto* Hero=PC?Cast<AHearthwardCharacter>(PC->GetPawn()):nullptr;
    auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr;
    if(!Hero || !UI || World->GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>()->IsLoading())return true;
    auto* Campaign=World->GetSubsystem<UHearthwardCampaignSubsystem>();
    if(Reload)
    {
        if(Step==0){Check(TEXT("continue_action"),UI->ExecuteAction(Checkpoint.IsEmpty()?FString(TEXT("continue")):TEXT("load:")+Checkpoint));Step=1;Wait=Now+3;return true;}
        if(!Campaign->Active() || Now<Wait)return true;
        if(Step==1)
        {
            auto* Save=World->GetSubsystem<UHearthwardSaveSubsystem>();
            const auto Points=Save->GetPoints();
            Check(TEXT("weather_saved_point_exists"),!Points.IsEmpty());
            if(!Points.IsEmpty())
            {
                const auto* Latest=&Points[0];for(const auto& Point:Points)if(Point.Created>Latest->Created)Latest=&Point;
                if(!Checkpoint.IsEmpty())
                {
                    const auto* Selected=Points.FindByPredicate([&](const auto& Point){return Point.SaveId.ToString()==Checkpoint;});
                    Check(TEXT("requested_checkpoint_exists"),Selected!=nullptr);if(!Selected)return Finish();Latest=Selected;
                }
                SavedInteractions=Latest->World.Interactions;
                Details->SetStringField(TEXT("checkpoint_saved_player"),Latest->World.Player.GetLocation().ToString());
                Details->SetStringField(TEXT("checkpoint_restored_player"),Hero->GetActorLocation().ToString());
                const auto* Saved=Latest->World.Interactions.Weather.FindByPredicate([](const auto& V){return V.Id==TEXT("world_light_rain");});
                const auto* Current=Save->InteractionState().Weather.FindByPredicate([](const auto& V){return V.Id==TEXT("world_light_rain");});
                Check(TEXT("weather_seed_restored"),Saved && Current && Saved->Seed!=0 && Current->Seed==Saved->Seed);
                const double Calendar=World->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().ElapsedCalendarMinutes;
                Check(TEXT("weather_calendar_restored"),Calendar>=Latest->World.CalendarMinutes && Calendar-Latest->World.CalendarMinutes<10);
                Details->SetNumberField(TEXT("saved_calendar"),Latest->World.CalendarMinutes);
                Details->SetNumberField(TEXT("restored_calendar"),Calendar);
                Details->SetNumberField(TEXT("saved_trail_count"),Latest->World.Interactions.Trails.Num());
                Check(TEXT("saved_trails_are_present"),!Latest->World.Interactions.Trails.IsEmpty());
                for(int32 Pass=0;Pass<2;++Pass)
                {
                    if(Pass)Check(TEXT("repeat_actual_load"),Save->LoadPoint(Latest->SaveId));
                    const auto& CurrentTrails=Save->InteractionState().Trails;
                    bool Same=CurrentTrails.Num()==Latest->World.Interactions.Trails.Num();
                    for(const auto& Trail:Latest->World.Interactions.Trails)
                    {
                        const auto* Actual=CurrentTrails.FindByPredicate([&](const auto& V){return V.Id==Trail.Id;});
                        Same&=Actual && FHearthwardTrailState::StaticStruct()->CompareScriptStruct(&Trail,Actual,0);
                    }
                    Check(Pass?TEXT("trails_repeat_load_exact"):TEXT("trails_independent_restart_exact"),Same);
                    const auto ActualInventory=Hero->FindComponentByClass<UHearthwardInventoryComponent>()->Snapshot();
                    Check(Pass?TEXT("inventory_repeat_load_exact"):TEXT("inventory_independent_restart_exact"),
                        FHearthwardInventorySnapshot::StaticStruct()->CompareScriptStruct(&Latest->World.PlayerItems,&ActualInventory,0));
                }
            }
            Step=10;Wait=Now+3;return true;
        }
        if(Step==10)
        {
            if(!Checkpoint.IsEmpty() && !RetriedCheckpoint)
            {
                double Nearest=250;
                for(TActorIterator<AHearthwardShortcutActor> It(World);It;++It)
                    if(It->Kind==EHearthwardDeviceKind::Beam || It->Kind==EHearthwardDeviceKind::Winch)
                    {
                        const double Distance=FVector::Dist(Hero->GetActorLocation(),It->GetActorLocation());
                        if(Distance<Nearest){Nearest=Distance;RetryDevice=*It;}
                    }
                Check(TEXT("checkpoint_retry_device_found"),RetryDevice.IsValid());
                if(!RetryDevice.IsValid())return Finish();
                const double Distance=FVector::Dist(Hero->GetActorLocation(),RetryDevice->Interaction->GetComponentLocation());
                Details->SetNumberField(TEXT("checkpoint_retry_distance_before"),Distance);
                Details->SetStringField(TEXT("checkpoint_retry_player"),Hero->GetActorLocation().ToString());
                Details->SetStringField(TEXT("checkpoint_retry_interaction"),RetryDevice->Interaction->GetComponentLocation().ToString());
                if(Distance>RetryDevice->Interaction->MaxDistance)
                {
                    FVector Floor;
                    Check(TEXT("checkpoint_retry_approach_ground"),Campaign->Ground(RetryDevice->GetActorTransform().TransformPosition(FVector(-65,-115,0)),Floor));
                    Details->SetStringField(TEXT("checkpoint_retry_approach_goal"),Floor.ToString());
                    UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC,Floor);
                    Step=12;Wait=Now+3;return true;
                }
                Details->SetStringField(TEXT("checkpoint_retry_action"),RetryDevice->Use(Hero));
                Step=11;Wait=Now+.6;return true;
            }
            // Restore rebuilds nearby content on subsequent ticks; visit workshops after cellar initialization.
            const FVector Workshops=Campaign->Position(TEXT("loot_workshops")),Cellar=Campaign->Position(TEXT("poi_cellar_01"));
            const FVector At=FVector::Dist2D(Hero->GetActorLocation(),Workshops)<FVector::Dist2D(Hero->GetActorLocation(),Cellar)?Cellar:Workshops;
            Hero->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Hero->SetActorLocation(FVector(At.X,At.Y,Hero->GetActorLocation().Z));
            Step=2;Wait=Now+3;return true;
        }
        if(Step==11)
        {
            Check(TEXT("checkpoint_retry_begins"),RetryDevice.IsValid() && RetryDevice->Active());
            if(RetryDevice.IsValid())
            {
                RetryDevice->Cancel();
                Check(TEXT("checkpoint_retry_cancel_restores_idle"),!RetryDevice->Active());
                Check(TEXT("checkpoint_retry_cancel_does_not_commit"),!RetryDevice->IsOpen());
            }
            RetriedCheckpoint=true;Step=10;return true;
        }
        if(Step==12)
        {
            PC->StopMovement();
            Details->SetNumberField(TEXT("checkpoint_retry_distance_after_walk"),FVector::Dist(Hero->GetActorLocation(),RetryDevice->Interaction->GetComponentLocation()));
            Details->SetStringField(TEXT("checkpoint_retry_action"),RetryDevice->Use(Hero));
            Step=11;Wait=Now+.6;return true;
        }
        if(Step==2)
        {
            FVector Floor;if(!Campaign->Ground(Hero->GetActorLocation(),Floor))return true;
            Hero->SetActorLocation(Floor+FVector(0,0,Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2));
            Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            Step=3;Wait=Now+3;return true;
        }
        auto* Clock=World->GetSubsystem<UHearthwardWorldClockSubsystem>();
        if(Step==3)
        {
            Check(TEXT("weather_pause_action"),UI->ExecuteAction(TEXT("page:pause")));
            PausedCalendar=Clock->GetSnapshot().ElapsedCalendarMinutes;PausedRain=AHearthwardWeatherActor::Rain(World);
            Details->SetNumberField(TEXT("paused_rain"),PausedRain);
            Step=4;Wait=Now+2;return true;
        }
        if(Step==4)
        {
            Check(TEXT("weather_pause_holds_clock"),Clock->Suspended() && Clock->GetSnapshot().ElapsedCalendarMinutes==PausedCalendar);
            Check(TEXT("weather_pause_holds_rain"),AHearthwardWeatherActor::Rain(World)==PausedRain);
            Check(TEXT("weather_resume_action"),UI->ExecuteAction(TEXT("page:hud")));
            Step=5;Wait=Now+2;return true;
        }
        Check(TEXT("weather_resume_advances_clock"),!Clock->Suspended() && Clock->GetSnapshot().ElapsedCalendarMinutes>PausedCalendar);
        const auto& State=World->GetSubsystem<UHearthwardSaveSubsystem>()->InteractionState();
        for(const FName Id:{FName(TEXT("workshops_inner_latch")),FName(TEXT("cellar_inner_latch")),FName(TEXT("cellar_drop_ladder"))})
        {
            const bool Expected=Checkpoint.IsEmpty() || SavedInteractions.Shortcuts.ContainsByPredicate([&](const auto& V){return V.Id==Id && V.Open;});
            Check(*Id.ToString(),State.Shortcuts.ContainsByPredicate([&](const auto& V){return V.Id==Id && V.Open;})==Expected);
        }
        for(const FName Id:{FName(TEXT("workshops_beam")),FName(TEXT("cellar_beam")),FName(TEXT("workshops_winch")),FName(TEXT("cellar_winch"))})
        {
            const bool Expected=Checkpoint.IsEmpty() || SavedInteractions.Cooperation.ContainsByPredicate([&](const auto& V){return V.Id==Id && V.Result!=EHearthwardCooperationResult::Unchanged;});
            Check(*Id.ToString(),State.Cooperation.ContainsByPredicate([&](const auto& V){return V.Id==Id && V.Result!=EHearthwardCooperationResult::Unchanged;})==Expected);
        }
        Check(TEXT("no_active_device_after_restart"),!World->IsPaused());
        for(TActorIterator<AHearthwardShortcutActor> It(World);It;++It)if(It->Active())Check(TEXT("no_active_device_after_restart"),false);
        int32 LampCount=0;
        for(TActorIterator<AActor> It(World);It;++It)
            if(auto* Lamp=It->FindComponentByClass<UHearthwardLampInteraction>())
            {
                ++LampCount;
                const auto* SavedLamp=SavedInteractions.Lamps.FindByPredicate([&](const auto& V){return V.Id==Lamp->Identity;});
                const bool Expected=SavedLamp?SavedLamp->Lit:Lamp->InitiallyLit;
                Check(*(Lamp->Identity.ToString()+TEXT(".state_and_light")),Lamp->IsLit()==Expected && Lamp->Light && Lamp->Light->IsVisible()==Expected);
                TArray<UPointLightComponent*> Lights;It->GetComponents(Lights);
                Check(*(Lamp->Identity.ToString()+TEXT(".one_light_component")),Lights.Num()==1);
                if(!Expected)Check(*(Lamp->Identity.ToString()+TEXT(".detection_dark")),
                    UHearthwardLampInteraction::Illumination(World,Lamp->GetComponentLocation(),.15,Hero)<=.15001);
            }
        Check(TEXT("four_unique_lamp_instances"),LampCount==4);
        return Finish();
    }
    if(!Campaign->Active() || (Step==0 && !Campaign->State.Facts.Contains(TEXT("prologue_intro"))))return true;
    AHearthwardCompanionFixture* Brother=nullptr;for(TActorIterator<AHearthwardCompanionFixture> It(World);It;++It){Brother=*It;break;}
    if(!Brother || Now<Wait)return true;
    auto* Gameplay=Hero->FindComponentByClass<UHearthwardGameplayComponent>();
    auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::GetPath(Output)/(FString(Site?TEXT("cellar-"):TEXT("workshops-"))+Name+TEXT(".png")),true,false);};
    auto SaveMidpoint=[&](const TCHAR* Name,bool MustReject=false)
    {
        auto* Save=World->GetSubsystem<UHearthwardSaveSubsystem>();TSet<FGuid> Before;for(const auto& P:Save->GetPoints())Before.Add(P.SaveId);
        const bool Saved=Save->SavePoint(true);
        Check(Name,MustReject?(!Saved && Save->GetStatus().Contains(TEXT("机关操作或协作尚未结束")) && Save->GetPoints().Num()==Before.Num()):Saved);
        Details->SetStringField(FString(Site?TEXT("cellar."):TEXT("workshops."))+Name+TEXT(".status"),Save->GetStatus());
        if(Saved)for(const auto& P:Save->GetPoints())if(!Before.Contains(P.SaveId))Details->SetStringField(FString(Site?TEXT("cellar."):TEXT("workshops."))+Name,P.SaveId.ToString());
    };
    auto Place=[&](ACharacter* Actor,AHearthwardShortcutActor* Device,FVector Local)
    {
        Actor->GetCharacterMovement()->StopMovementImmediately();
        Actor->SetActorLocation(Device->GetActorTransform().TransformPosition(Local));Actor->SetActorRotation(Device->GetActorRotation());
        Actor->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    };
    auto Find=[&](const TCHAR* Suffix)
    {
        const FName Id(*(FString(Site?TEXT("cellar_"):TEXT("workshops_"))+Suffix));
        for(TActorIterator<AHearthwardShortcutActor> It(World);It;++It)if(It->Identity==Id)return *It;
        return static_cast<AHearthwardShortcutActor*>(nullptr);
    };
    auto Diagnose=[&](AHearthwardShortcutActor* Device,const TCHAR* Name)
    {
        auto Result=MakeShared<FJsonObject>();
        for(ACharacter* Person:{static_cast<ACharacter*>(Hero),static_cast<ACharacter*>(Brother)})
        {
            const bool IsHero=Person==Hero;const float Side=Device->Kind==EHearthwardDeviceKind::Beam?(IsHero?-115:115):0;
            const float Forward=Device->Kind==EHearthwardDeviceKind::Latch?-90:Device->Kind==EHearthwardDeviceKind::Ladder?-160:-65;
            const float Half=Person->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
            FVector Point=Device->GetActorTransform().TransformPosition(FVector(Forward,Side,Half+2+(Device->Kind==EHearthwardDeviceKind::Ladder?220:0)));
            FCollisionQueryParams Query(SCENE_QUERY_STAT(SceneContactDiagnostic),false,Person);FHitResult Hit;
            Query.AddIgnoredActor(Device);
            if(World->LineTraceSingleByChannel(Hit,Point-FVector(0,0,Half-60),Point-FVector(0,0,Half+60),ECC_Visibility,Query))Point.Z=Hit.ImpactPoint.Z+Half+2;
            Query.ClearIgnoredSourceObjects();Query.AddIgnoredActor(Person);
            const bool Ground=World->LineTraceSingleByChannel(Hit,Point-FVector(0,0,Half-12),Point-FVector(0,0,Half+20),ECC_Visibility,Query);
            const FString Prefix=IsHero?TEXT("hero."):TEXT("brother.");
            Result->SetStringField(Prefix+TEXT("position"),Person->GetActorLocation().ToString());Result->SetStringField(Prefix+TEXT("station"),Point.ToString());
            Result->SetBoolField(Prefix+TEXT("walking"),Person->GetCharacterMovement()->IsMovingOnGround());
            Result->SetBoolField(Prefix+TEXT("ground"),Ground);Result->SetNumberField(Prefix+TEXT("normal_z"),Hit.ImpactNormal.Z);
            const bool Blocked=World->SweepSingleByChannel(Hit,Person->GetActorLocation(),Point,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Person->GetCapsuleComponent()->GetScaledCapsuleRadius(),Half-2),Query);
            Result->SetBoolField(Prefix+TEXT("sweep_blocked"),Blocked);if(Blocked)
            {
                Result->SetStringField(Prefix+TEXT("blocker"),GetNameSafe(Hit.GetActor()));
                Result->SetStringField(Prefix+TEXT("component"),GetNameSafe(Hit.GetComponent()));
                if(Hit.GetComponent())Result->SetStringField(Prefix+TEXT("bounds"),Hit.GetComponent()->Bounds.GetBox().ToString());
            }
        }
        if(Device->Kind==EHearthwardDeviceKind::Beam)
        {
            FCollisionQueryParams Query(SCENE_QUERY_STAT(SceneBeamDiagnostic),false,Device);FHitResult Hit;
            const FVector Points[]={FVector(0,0,118),FVector(0,0,148),FVector(0,340,148),FVector(0,340,18)};
            for(int32 I=1;I<4;++I)if(World->SweepSingleByChannel(Hit,Device->GetActorTransform().TransformPosition(Points[I-1]),Device->GetActorTransform().TransformPosition(Points[I]),Device->GetActorQuat(),ECC_Visibility,FCollisionShape::MakeBox(FVector(16,160,16)),Query))
            {Result->SetStringField(TEXT("legacy_beam_path_blocker"),GetNameSafe(Hit.GetActor()));Result->SetNumberField(TEXT("legacy_beam_path_segment"),I);Result->SetStringField(TEXT("legacy_beam_path_impact"),Hit.ImpactPoint.ToString());break;}
        }
        Details->SetObjectField(FString(Site?TEXT("cellar."):TEXT("workshops."))+Name,Result);
    };
    switch(Step)
    {
    case 0:
    {
        auto Fixture=Campaign->State;Fixture.Phase=TEXT("occupied");
        for(auto& Enemy:Fixture.Enemies){Enemy.Combat.Health=0;Enemy.Combat.bStunned=true;}
        Check(TEXT("fixture_initialized"),Campaign->Restore(Fixture.Snapshot()));
        Gameplay->CompanionRoutineEnabled=false;UI->OpenPage(TEXT("hud"));
        ++Step;Wait=Now+1;break;
    }
    case 1:
    {
        const FVector At=Campaign->Position(Site?TEXT("poi_cellar_01"):TEXT("loot_workshops"));
        Hero->GetCharacterMovement()->SetMovementMode(MOVE_Flying);Hero->SetActorLocation(FVector(At.X,At.Y,Hero->GetActorLocation().Z));
        Brother->StopNavigation();Brother->GetCharacterMovement()->SetMovementMode(MOVE_Flying);Brother->SetActorLocation(Hero->GetActorLocation()+FVector(0,300,0));
        ++Step;Wait=Now+3;break;
    }
    case 2:
    {
        FVector Floor;if(!Campaign->Ground(Hero->GetActorLocation(),Floor)){Wait=Now+1;break;}
        Hero->SetActorLocation(Floor+FVector(-500,-100,92));Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Brother->SetActorLocation(Floor+FVector(-500,150,82));Brother->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        ++Step;Wait=Now+2;break;
    }
    case 3:
        Beam=Find(TEXT("beam"));Winch=Find(TEXT("winch"));Gate=Find(TEXT("inner_latch"));Ladder=Find(TEXT("drop_ladder"));
        if(!Beam.IsValid() || !Winch.IsValid() || !Gate.IsValid() || (Site && !Ladder.IsValid())){Wait=Now+.5;break;}
        {
            const FName LampId=Site?TEXT("cellar_lamp_entry"):TEXT("workshops_lamp_entry");
            UHearthwardLampInteraction* Lamp=nullptr;
            for(TActorIterator<AActor> It(World);It;++It)
                if(auto* Candidate=It->FindComponentByClass<UHearthwardLampInteraction>();Candidate && Candidate->Identity==LampId){Lamp=Candidate;break;}
            Check(TEXT("entry_lamp_exists"),Lamp!=nullptr);
            if(Lamp)
            {
                Hero->GetCharacterMovement()->StopMovementImmediately();
                FVector LampFloor;
                const bool Grounded=Campaign->Ground(Lamp->GetComponentLocation()+FVector(Site?-100:100,0,0),LampFloor);
                Check(TEXT("entry_lamp_approach_ground"),Grounded);
                if(Grounded)Hero->SetActorLocation(LampFloor+FVector(0,0,Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2));
                auto LampDetail=MakeShared<FJsonObject>();
                LampDetail->SetStringField(TEXT("lamp"),Lamp->GetComponentLocation().ToString());
                LampDetail->SetStringField(TEXT("player"),Hero->GetActorLocation().ToString());
                FHitResult LampHit;FCollisionQueryParams LampQuery(SCENE_QUERY_STAT(LampApproachDiagnostic),false,Hero);LampQuery.AddIgnoredActor(Lamp->GetOwner());
                if(World->LineTraceSingleByChannel(LampHit,Hero->GetActorLocation(),Lamp->GetComponentLocation(),ECC_Visibility,LampQuery))
                {
                    LampDetail->SetStringField(TEXT("blocker"),GetNameSafe(LampHit.GetActor()));
                    LampDetail->SetStringField(TEXT("component"),GetNameSafe(LampHit.GetComponent()));
                    LampDetail->SetStringField(TEXT("impact"),LampHit.ImpactPoint.ToString());
                    if(LampHit.GetComponent())LampDetail->SetStringField(TEXT("bounds"),LampHit.GetComponent()->Bounds.GetBox().ToString());
                }
                FHitResult EastHit;
                if(World->LineTraceSingleByChannel(EastHit,Lamp->GetComponentLocation()+FVector(100,0,50),Lamp->GetComponentLocation(),ECC_Visibility,LampQuery))
                {
                    LampDetail->SetStringField(TEXT("east_blocker"),GetNameSafe(EastHit.GetActor()));
                    LampDetail->SetStringField(TEXT("east_component"),GetNameSafe(EastHit.GetComponent()));
                    if(EastHit.GetComponent())LampDetail->SetStringField(TEXT("east_bounds"),EastHit.GetComponent()->Bounds.GetBox().ToString());
                }
                Details->SetObjectField(Site?TEXT("cellar.lamp_approach"):TEXT("workshops.lamp_approach"),LampDetail);
                Check(TEXT("entry_lamp_initially_lit"),Lamp->IsLit());
                Details->SetStringField(Site?TEXT("cellar.lamp_action"):TEXT("workshops.lamp_action"),Lamp->CompleteInteraction(Hero));
                Check(TEXT("entry_lamp_real_toggle"),!Lamp->IsLit() && Lamp->Light && !Lamp->Light->IsVisible());
            }
        }
        if(!Site)
        {
            const auto* Landing=Campaign->State.Positions.Find(TEXT("round2_archer_landing"));
            auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);FNavLocation Reachable;
            Check(TEXT("archer_platform_navigation"),Landing && Nav && Nav->ProjectPointToNavigation(*Landing,Reachable,FVector(100,100,100)));
            FVector CoverFloor;FHitResult CoverHit;FCollisionQueryParams CoverQuery(SCENE_QUERY_STAT(WorkshopCover),false,Hero);CoverQuery.AddIgnoredActor(Brother);
            Check(TEXT("front_cover_blocks_archer"),Landing && Campaign->Ground(Campaign->Position(TEXT("loot_workshops"))+FVector(300,0,0),CoverFloor)
                && World->LineTraceSingleByChannel(CoverHit,*Landing+FVector(0,0,160),CoverFloor+FVector(0,0,90),ECC_Visibility,CoverQuery)
                && CoverHit.GetActor() && CoverHit.GetActor()->ActorHasTag(TEXT("Round2Workshops")));
        }
        Brother->StopNavigation();Place(Hero,Beam.Get(),FVector(-65,-115,92));Place(Brother,Beam.Get(),FVector(-65,115,82));
        PC->SetControlRotation(FRotator(-20,Beam->GetActorRotation().Yaw,0));++Step;Wait=Now+.6;break;
    case 4:
        Diagnose(Beam.Get(),TEXT("beam_contact"));
        SaveMidpoint(TEXT("beam_before_save"));
        Details->SetStringField(Site?TEXT("cellar.beam_start"):TEXT("workshops.beam_start"),Beam->Use(Hero));++Step;Wait=Now+1.6;break;
    case 5:
        Check(TEXT("beam_active_midway"),Beam->Active() && !Beam->IsOpen());Shot(TEXT("beam-midway"));
        SaveMidpoint(TEXT("beam_midpoint_rejected"),true);
        Details->SetStringField(Site?TEXT("cellar.feedback"):TEXT("workshops.feedback"),Gameplay->Feedback);
        Step=50;Wait=Now+.2;break;
    case 50:
        Beam->Cancel();Check(TEXT("beam_cancel_restores_support"),!Beam->IsOpen() && Beam->Mesh->GetRelativeLocation().Equals(FVector(0,0,100),1));
        SaveMidpoint(TEXT("beam_cancel_save"));
        Place(Hero,Beam.Get(),FVector(-65,-115,92));Place(Brother,Beam.Get(),FVector(-65,115,82));
        Step=6;Wait=Now+.6;break;
    case 6:Beam->Use(Hero);++Step;Wait=Now+3.6;break;
    case 7:
        Check(TEXT("beam_commit"),Beam->IsOpen());Shot(TEXT("beam-complete"));
        Step=70;Wait=Now+.2;break;
    case 70:
        Place(Hero,Winch.Get(),FVector(-65,-115,92));Place(Brother,Winch.Get(),FVector(-365,0,82));
        PC->SetControlRotation(FRotator(-20,Winch->GetActorRotation().Yaw,0));Step=8;Wait=Now+.6;break;
    case 8:
        Diagnose(Winch.Get(),TEXT("winch_contact"));SaveMidpoint(TEXT("winch_before_save"));Winch->Use(Hero);
        SaveMidpoint(TEXT("winch_approach_rejected"),true);Step=80;Wait=Now+.05;break;
    case 80:
        if(Winch->Phase==EHearthwardDevicePhase::Ready)
        {SaveMidpoint(TEXT("winch_ready_rejected"),true);Step=81;}
        else if(!Winch->Active()){Check(TEXT("winch_reaches_ready"),false);Step=9;}
        Wait=Now+.05;break;
    case 81:
        if(Winch->Phase==EHearthwardDevicePhase::Hold || !Winch->Active())Step=9;
        Wait=Now+.05;break;
    case 9:
        Check(TEXT("winch_navigates_and_holds"),Winch->Phase==EHearthwardDevicePhase::Hold);Shot(TEXT("winch-hold"));
        SaveMidpoint(TEXT("winch_hold_rejected"),true);
        Step=90;Wait=Now+.2;break;
    case 90:
        Place(Hero,Gate.Get(),FVector(-90,0,92));Step=10;Wait=Now+.6;break;
    case 10:Diagnose(Gate.Get(),TEXT("latch_contact"));Gate->Use(Hero);++Step;Wait=Now+1.6;break;
    case 11:
        Check(TEXT("inner_latch_commits"),Gate->IsOpen());Check(TEXT("winch_releases_after_latch"),!Winch->Active());Shot(TEXT("gate-open"));
        Step=110;Wait=Now+.2;break;
    case 110:
        if(!Site){Site=1;Step=1;Wait=Now+.5;break;}
        Place(Hero,Ladder.Get(),FVector(-160,0,312));PC->SetControlRotation(FRotator(-25,180,0));
        Step=12;Wait=Now+.8;break;
    case 12:Diagnose(Ladder.Get(),TEXT("ladder_contact"));Ladder->Use(Hero);++Step;Wait=Now+1.5;break;
    case 13:
        Check(TEXT("ladder_released"),Ladder->IsOpen());Ladder->Use(Hero);++Step;Wait=Now+3.6;break;
    case 14:
        Check(TEXT("ladder_descent_ends"),!Ladder->Active());
        Check(TEXT("ladder_reaches_outer_end"),FVector::Dist2D(Hero->GetActorLocation(),Ladder->GetActorTransform().TransformPosition(FVector(100,0,0)))<100);
        Shot(TEXT("ladder-exit"));++Step;Wait=Now+.5;break;
    default:
        Check(TEXT("actual_save_point"),World->GetSubsystem<UHearthwardSaveSubsystem>()->SavePoint(true));
        Details->SetStringField(TEXT("save_status"),World->GetSubsystem<UHearthwardSaveSubsystem>()->GetStatus());
        return Finish();
    }
    return true;
}
const FTSTicker::FDelegateHandle Round2SceneTicker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&VerifyRound2Scenes),.05f);
}
#endif
