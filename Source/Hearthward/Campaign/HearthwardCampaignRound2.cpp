#include "HearthwardCampaignSubsystem.h"
#include "HearthwardPersonalStory.h"
#include "../Environment/HearthwardShortcutActor.h"
#include "../Environment/HearthwardLampInteraction.h"
#include "../Environment/HearthwardNoiseInteraction.h"
#include "../Environment/HearthwardWeatherActor.h"
#include "../Nature/HearthwardAnimalTraces.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Pawn.h"

void UHearthwardCampaignSubsystem::RefreshRound2()
{
    if(State.Phase==TEXT("prologue"))return;
    if(!Scenery.ContainsByPredicate([](const auto& A){return A.IsValid() && A->IsA<AHearthwardWeatherActor>();}))Scenery.Add(GetWorld()->SpawnActor<AHearthwardWeatherActor>());
    if(!AHearthwardPersonalStoryDirector::Find(GetWorld()))Scenery.Add(GetWorld()->SpawnActor<AHearthwardPersonalStoryDirector>());
    if(!Scenery.ContainsByPredicate([](const auto& A){return A.IsValid() && A->IsA<AHearthwardAnimalTraces>();}))Scenery.Add(GetWorld()->SpawnActor<AHearthwardAnimalTraces>());
    for(const FName Camp:{FName(TEXT("camp")),FName(TEXT("hometown"))})
    {
        if(Camp==TEXT("hometown") && !State.Victory)continue;
        if(FVector::Dist2D(Player()->GetActorLocation(),Position(Camp))>15000)continue;
        for(int32 Slot=1;Slot<=3;++Slot)
        {
            const FName Id(*FString::Printf(TEXT("display_%s_%d"),*Camp.ToString(),Slot));
            if(Scenery.ContainsByPredicate([&](const auto& A){return A.IsValid() && A->ActorHasTag(Id);}))continue;
            FVector Floor;if(!Ground(Position(Camp)+FVector(1050+(Slot-1)*160,550,0),Floor))continue;
            auto* A=GetWorld()->SpawnActor<AActor>();Scenery.Add(A);A->Tags.Add(Id);
            auto* Stand=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(Stand);A->SetRootComponent(Stand);
            Stand->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-028/furniture/wood_chest/SM_wood_chest.SM_wood_chest")));
            const FBox Bounds=Stand->GetStaticMesh()->GetBoundingBox();const FVector Scale=FVector(100,60,70)/Bounds.GetSize();
            Stand->SetRelativeScale3D(Scale);Stand->SetCollisionProfileName(TEXT("NoCollision"));Stand->RegisterComponent();A->SetActorLocation(Floor-FVector(Bounds.GetCenter().X*Scale.X,Bounds.GetCenter().Y*Scale.Y,Bounds.Min.Z*Scale.Z));
            auto* Object=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(Object);Object->SetupAttachment(Stand);Object->SetAbsolute(false,false,true);Object->SetCollisionProfileName(TEXT("NoCollision"));Object->RegisterComponent();Object->SetWorldLocation(Floor+FVector(0,0,70));
            auto* Use=NewObject<UHearthwardPersonalInteraction>(A);A->AddInstanceComponent(Use);Use->SetupAttachment(Stand);
            Use->Identity=Id;Use->DisplayCamp=Camp;Use->DisplaySlot=FName(*FString::FromInt(Slot));Use->DisplayMesh=Object;Use->RegisterComponent();Use->SetWorldLocation(Floor+FVector(0,0,80));
        }
    }
    for(const bool Cellar:{false,true})
    {
        const FName Site=Cellar?TEXT("poi_cellar_01"):TEXT("loot_workshops");
        const FName Tag=Cellar?TEXT("Round2Cellar"):TEXT("Round2Workshops");
        if(FVector::Dist2D(Player()->GetActorLocation(),Position(Site))>18000
            || Scenery.ContainsByPredicate([&](const auto& A){return A.IsValid() && A->ActorHasTag(Tag);}))continue;
        FVector Origin;if(!Ground(Position(Site),Origin))continue;
        // Keep the original reward coordinates and facts. The new approaches surround them.
        auto* Set=GetWorld()->SpawnActor<AActor>();Set->Tags.Add(Tag);Scenery.Add(Set);
        auto* Root=NewObject<USceneComponent>(Set);Set->AddInstanceComponent(Root);Set->SetRootComponent(Root);Root->RegisterComponent();Set->SetActorLocation(Origin);
        auto Structure=[&](FVector At,FVector Size,bool Stone)
        {
            FVector Floor;if(!Ground(Origin+At,Floor))return;
            auto* Part=NewObject<UStaticMeshComponent>(Set);Set->AddInstanceComponent(Part);Part->SetupAttachment(Root);
            Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
            Part->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Stone?TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_RoughStone.M_RoughStone"):TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber.M_OldTimber")));
            Part->SetRelativeLocation(Floor-Origin+FVector(0,0,At.Z+Size.Z*.5));Part->SetRelativeScale3D(Size/100.);Part->SetCollisionProfileName(TEXT("BlockAll"));Part->RegisterComponent();
        };
        // A broad unlatched entrance and a separate narrow service return; neither changes flag ownership.
        for(int32 Side:{-1,1})Structure(FVector(0,Side*850,0),FVector(1600,60,Cellar?240:170),Cellar);
        Structure(FVector(-800,-500,0),FVector(60,650,240),Cellar);
        Structure(FVector(-800,500,0),FVector(60,650,240),Cellar);
        Structure(FVector(800,-500,0),FVector(60,650,240),Cellar);
        Structure(FVector(800,500,0),FVector(60,650,240),Cellar);
        if(Cellar)
        {
            double Roof=Origin.Z+300;
            for(int32 X:{-800,0,800})for(int32 Y:{-450,200,850})
            {FVector Floor;if(Ground(Origin+FVector(X,Y,0),Floor))Roof=FMath::Max(Roof,Floor.Z+300);}
            FVector RoofFloor;Ground(Origin+FVector(0,200,0),RoofFloor);
            Structure(FVector(0,200,Roof-RoofFloor.Z),FVector(1600,1300,40),true);
            Structure(FVector(-200,150,0),FVector(80,700,220),true);
            Structure(FVector(420,100,0),FVector(300,110,110),false);
        }
        else
        {
            Structure(FVector(-160,-330,0),FVector(600,120,200),false);
            Structure(FVector(260,280,0),FVector(600,130,140),false);
            // The existing archer keeps his identity, rewards and patrol bounds.
            FVector ArcherFloor;
            if(Ground(Origin+FVector(-1300,-1100,0),ArcherFloor))
            {
                auto Deck=[&](FVector Offset,FVector Size)
                {
                    auto* Part=NewObject<UStaticMeshComponent>(Set);Set->AddInstanceComponent(Part);Part->SetupAttachment(Root);
                    Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
                    Part->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber.M_OldTimber")));
                    Part->SetRelativeLocation(ArcherFloor-Origin+Offset+FVector(0,0,Size.Z*.5));Part->SetRelativeScale3D(Size/100.);Part->SetCollisionProfileName(TEXT("BlockAll"));Part->RegisterComponent();
                };
                Deck(FVector::ZeroVector,FVector(600,500,260));
                for(int32 Step=1;Step<=8;++Step)Deck(FVector(-300-(9-Step)*60+30,0,0),FVector(60,180,Step*32.5));
                State.Positions.Add(TEXT("round2_archer_landing"),ArcherFloor+FVector(0,0,260));
            }
        }
        auto Device=[&](const TCHAR* Suffix,EHearthwardDeviceKind Kind,FVector Offset,float Yaw)
        {
            FVector Floor;if(!Ground(Origin+Offset,Floor))return static_cast<AHearthwardShortcutActor*>(nullptr);
            auto* A=GetWorld()->SpawnActor<AHearthwardShortcutActor>(Floor,FRotator(0,Yaw,0));
            A->Configure(FName(*(FString(Cellar?TEXT("cellar_"):TEXT("workshops_"))+Suffix)),Kind);Scenery.Add(A);return A;
        };
        auto* Latch=Device(TEXT("inner_latch"),EHearthwardDeviceKind::Latch,FVector(800,0,0),0);
        auto* Winch=Device(TEXT("winch"),EHearthwardDeviceKind::Winch,FVector(1020,-400,0),180);
        if(Winch)Winch->LinkedGate=Latch;
        if(Cellar)
        {
            for(int32 Step=1;Step<=5;++Step)Structure(FVector(-320-Step*64,-650,0),FVector(64,180,Step*44),true);
            Device(TEXT("drop_ladder"),EHearthwardDeviceKind::Ladder,FVector(-800,-650,0),180);
        }
        Device(TEXT("beam"),EHearthwardDeviceKind::Beam,FVector(Cellar?-320:-250,0,0),0);
        for(int32 I=0;I<3;++I)
        {
            // The cellar entry lamp must sit beside the solid stair treads.
            FVector Floor;if(!Ground(Origin+FVector(I==2?1050:-450,I==0?(Cellar?-420:-620):I==1?500:550,0),Floor))continue;
            auto* A=GetWorld()->SpawnActor<AActor>();Scenery.Add(A);
            auto* Mesh=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(Mesh);A->SetRootComponent(Mesh);
            const TCHAR* Name=I==2?TEXT("NoisePlate"):TEXT("Lamp");
            const FString Path=FString::Printf(TEXT("/Game/Hearthward/Assets/TASK-118/SM_Round2_%s.SM_Round2_%s"),Name,Name);
            Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));Mesh->SetCollisionProfileName(TEXT("NoCollision"));Mesh->RegisterComponent();A->SetActorLocation(Floor);
            if(I==2)
            {
                auto* Use=NewObject<UHearthwardNoiseInteraction>(A);A->AddInstanceComponent(Use);Use->SetupAttachment(Mesh);Use->SetRelativeLocation(FVector(0,0,100));Use->RegisterComponent();
            }
            else
            {
                auto* Light=NewObject<UPointLightComponent>(A);A->AddInstanceComponent(Light);Light->SetupAttachment(Mesh);Light->SetRelativeLocation(FVector(0,0,60));Light->SetIntensity(1800);Light->SetAttenuationRadius(800);Light->SetLightColor(FLinearColor(1,.57f,.22f));Light->RegisterComponent();
                auto* Use=NewObject<UHearthwardLampInteraction>(A);A->AddInstanceComponent(Use);Use->SetupAttachment(Mesh);Use->SetRelativeLocation(FVector(0,0,60));Use->Light=Light;
                Use->Identity=FName(*(FString(Cellar?TEXT("cellar_lamp_"):TEXT("workshops_lamp_"))+(I==0?TEXT("entry"):Cellar?TEXT("store"):TEXT("yard"))));Use->RegisterComponent();
            }
        }
    }
}
