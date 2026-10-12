// Development-only verification in an explicitly selected disposable save pool.
#if !UE_BUILD_SHIPPING
#include "../HearthwardCharacter.h"
#include "../Animation/HearthwardHeroAnimInstance.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
namespace
{
bool VerifyNavigation111(float)
{
    static FString Output;
    static auto Checks=MakeShared<FJsonObject>();
    static auto State=MakeShared<FJsonObject>();
    static bool Passed=true,Moving=false;
    static int32 Step=0,Frames=0;
    static double Started=FPlatformTime::Seconds(),Wait=0;
    static FVector Start,FirstLeg,FirstHand;
    static float LegMotion=0,HandMotion=0,AttackDistance=0,AttackSpeed=0;
    if(Output.IsEmpty())
    {
        FString Pool;FGuid Id;
        if(!FParse::Value(FCommandLine::Get(),TEXT("HearthwardNavigationVerify="),Output)
            || !FParse::Value(FCommandLine::Get(),TEXT("HearthwardSaveTestPool="),Pool) || !FGuid::Parse(Pool,Id) || !Id.IsValid())return false;
    }
    auto Check=[&](const TCHAR* Name,bool Value){Checks->SetBoolField(Name,Value);Passed&=Value;};
    auto Finish=[&]()
    {
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("passed"),Passed);J->SetObjectField(TEXT("checks"),Checks);
        J->SetObjectField(TEXT("state"),State);
        J->SetNumberField(TEXT("attack_distance_cm"),AttackDistance);J->SetNumberField(TEXT("leg_motion_cm"),LegMotion);J->SetNumberField(TEXT("hand_motion_cm"),HandMotion);
        J->SetNumberField(TEXT("attack_speed_cm_s"),AttackSpeed);
        FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));
        FFileHelper::SaveStringToFile(Text,*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);return false;
    };
    const double Now=FPlatformTime::Seconds();
    if(Now-Started>240){Check(TEXT("completion"),false);return Finish();}
    UWorld* World=nullptr;
    if(GEngine)for(const auto& C:GEngine->GetWorldContexts())if(C.WorldType==EWorldType::Game){World=C.World();break;}
    auto* PC=World?World->GetFirstPlayerController():nullptr;
    auto* Hero=PC?Cast<AHearthwardCharacter>(PC->GetPawn()):nullptr;
    auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;
    auto* UI=HUD?HUD->Screen.Get():nullptr;
    if(!UI || !Hero || !World->GetSubsystem<UHearthwardCampaignSubsystem>()->State.Facts.Contains(TEXT("prologue_intro"))
        || World->GetGameInstance()->GetSubsystem<UHearthwardLoadingSubsystem>()->IsLoading())return true;
    auto* Combat=Hero->FindComponentByClass<UHearthwardCombatComponent>();
    auto* Gameplay=Hero->FindComponentByClass<UHearthwardGameplayComponent>();
    auto* Mesh=Hero->GetMesh();
    if(Moving)
    {
        if(Combat->Action==TEXT("attack"))
        {
            ++Frames;AttackDistance=FVector::DistXY(Start,Hero->GetActorLocation());
            AttackSpeed=FMath::Max(AttackSpeed,float(Hero->GetVelocity().Size2D()));
            const FVector Leg=Mesh->GetBoneTransform(Mesh->GetBoneIndex(TEXT("foot_l")),FTransform::Identity).GetLocation();
            const FVector Hand=Mesh->GetBoneTransform(Mesh->GetBoneIndex(TEXT("hand_r")),FTransform::Identity).GetLocation();
            if(Frames==1){FirstLeg=Leg;FirstHand=Hand;}
            LegMotion=FMath::Max(LegMotion,float(FVector::Distance(Leg,FirstLeg)));
            HandMotion=FMath::Max(HandMotion,float(FVector::Distance(Hand,FirstHand)));
        }
    }
    if(Now<Wait)return true;
    auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::GetPath(Output)/(FString(Name)+TEXT(".png")),true,false);};
    auto Snapshot=[&](){TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UI->DescribeHUDPreview()),J);return J;};
    auto MoveKey=[&](bool Down)
    {
        auto* Viewport=World->GetGameInstance()->GetGameViewportClient();
        Viewport->InputKey(FInputKeyEventArgs(Viewport->Viewport,FInputDeviceId::CreateFromInternalId(0),EKeys::W,
            Down?IE_Pressed:IE_Released,Down?1.f:0.f,false,FPlatformTime::Cycles64()));
    };
    switch(Step++)
    {
    case 0:
        UI->OpenPage(TEXT("map"));Wait=Now+2;break;
    case 1:
    {
        const auto J=Snapshot();
        Check(TEXT("scene_capture_bound"),J->GetObjectField(TEXT("mapView"))->GetBoolField(TEXT("sceneCapture")));
        for(const TCHAR* Id:{TEXT("map.player"),TEXT("map.brother")})
        {
            bool Found=false;
            for(const auto& V:J->GetArrayField(TEXT("elements")))
                if(V->AsObject()->GetStringField(TEXT("id"))==Id)Found=V->AsObject()->GetBoolField(TEXT("visible")) && !V->AsObject()->GetStringField(TEXT("text")).IsEmpty();
            Check(Id,Found);
        }
        Shot(TEXT("map"));Wait=Now+.4;break;
    }
    case 2:
        UI->OpenPage(TEXT("hud"));PC->SetControlRotation(FRotator(-25,-90,0));Wait=Now+.4;break;
    case 3:Shot(TEXT("compass-north"));Wait=Now+.4;break;
    case 4:PC->SetControlRotation(FRotator(-25,0,0));Wait=Now+.4;break;
    case 5:Shot(TEXT("compass-east"));Wait=Now+.4;break;
    case 6:
    {
        FVector At=Hero->GetActorLocation()+FVector(10000,10000,0);FHitResult Ground;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(Navigation111),false,Hero);
        if(World->LineTraceSingleByChannel(Ground,At+FVector(0,0,10000),At-FVector(0,0,10000),ECC_Visibility,Q))At=Ground.ImpactPoint+FVector(0,0,100);
        Hero->SetActorLocation(At);Gameplay->Stamina=Gameplay->MaxStamina();
        auto* Bag=Hero->FindComponentByClass<UHearthwardInventoryComponent>();
        if(!Bag->FirstInstance(TEXT("axe")).IsValid())Bag->TryAdd(TEXT("axe"),1);
        if(Bag->EquippedInstance(TEXT("weapon"))!=Bag->FirstInstance(TEXT("axe")))Gameplay->CommitEquipment(TEXT("axe"));
        Gameplay->InventoryChanged();
        Check(TEXT("equip_test_axe"),Gameplay->Equipment.FindRef(TEXT("weapon"))==TEXT("axe"));
        World->GetSubsystem<UHearthwardWorldClockSubsystem>()->AdvanceCalendar(960);
        PC->SetControlRotation(FRotator(-18,145,0));Moving=true;MoveKey(true);Wait=Now+2;break;
    }
    case 7:
        State->SetStringField(TEXT("weapon"),Gameplay->Equipment.FindRef(TEXT("weapon")).ToString());
        State->SetNumberField(TEXT("durability"),Gameplay->EquippedDurability(TEXT("weapon")));
        State->SetNumberField(TEXT("attack_power"),Gameplay->AttackPower());
        State->SetNumberField(TEXT("speed"),Hero->GetVelocity().Size2D());
        State->SetNumberField(TEXT("movement_mode"),Hero->GetCharacterMovement()->MovementMode);
        State->SetBoolField(TEXT("input_ignored"),Hero->IsMoveInputIgnored());
        State->SetBoolField(TEXT("paused"),World->IsPaused());
        Gameplay->SetSprinting(true);
        Start=Hero->GetActorLocation();Check(TEXT("moving_attack_started"),Combat->Attack(false));
        Check(TEXT("sprint_preserved"),Gameplay->IsRunning());Wait=Now+.18;break;
    case 8:Shot(TEXT("attack-windup"));Wait=Now+.22;break;
    case 9:Shot(TEXT("attack-contact"));Wait=Now+.3;break;
    case 10:Shot(TEXT("attack-recovery"));Wait=Now+.7;break;
    default:
        Moving=false;MoveKey(false);Gameplay->SetSprinting(false);
        Check(TEXT("attack_moves_over_one_metre"),AttackDistance>100);
        Check(TEXT("legs_animate_during_attack"),Frames>3 && LegMotion>3);
        Check(TEXT("hands_animate_during_attack"),Frames>3 && HandMotion>8);
        Check(TEXT("sprint_speed_during_attack"),AttackSpeed>400);
        Check(TEXT("attack_finished"),!Combat->Busy());
        return Finish();
    }
    return true;
}
const FTSTicker::FDelegateHandle Navigation111Ticker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&VerifyNavigation111),.02f);
}
#endif
