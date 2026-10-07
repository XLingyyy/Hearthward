#include "HearthwardNatureActor.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "HearthwardNatureSubsystem.h"
#include "../Animals/HearthwardAnimalMotionComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Combat/HearthwardCombatTargetComponent.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Combat/HearthwardCombatRules.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../UI/HearthwardHUD.h"
#include "../UI/HearthwardScreenWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "../Time/HearthwardWorldClockSubsystem.h"
#include "../Companion/HearthwardCompanionFixture.h"
using namespace HearthwardData;
AHearthwardNatureActor::AHearthwardNatureActor()
{
    PrimaryActorTick.bCanEverTick=true;
    Shape=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));SetRootComponent(Shape);
    Shape->SetMobility(EComponentMobility::Movable);Shape->SetCollisionProfileName(TEXT("BlockAllDynamic"));Shape->ComponentTags.Add(TEXT("body"));
    Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));Label->SetupAttachment(RootComponent);Label->SetHorizontalAlignment(EHTA_Center);Label->SetWorldSize(20);
    Interaction=CreateDefaultSubobject<UHearthwardNatureInteraction>(TEXT("Interaction"));Interaction->SetupAttachment(RootComponent);Interaction->MaxDistance=300;
    Combat=CreateDefaultSubobject<UHearthwardCombatTargetComponent>(TEXT("Combat"));Combat->NaturalTarget=true;Combat->Protected=true;
    AnimalMotion=CreateDefaultSubobject<UHearthwardAnimalMotionComponent>(TEXT("AnimalMotion"));
}
void AHearthwardNatureActor::Configure(FGuid Entity,FName Type,FName Def)
{
    Id=Entity;Kind=Type;Definition=Def;
    Interaction->MaxDistance=Kind==TEXT("fish")?1500:Kind==TEXT("resource")?240:300;
    UStaticMesh* Mesh=nullptr;FVector Scale(.5);
    if(Kind==TEXT("animal"))
    {
        FString Stem=Def==TEXT("deer")?TEXT("stag_a"):Def==TEXT("boar")?TEXT("pig"):Def.ToString();
        Mesh=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Hearthward/Nature/SM_")+Stem+TEXT(".SM_")+Stem));
        if(Mesh){const FVector Extent=Mesh->GetBounds().BoxExtent;const float Length=Def==TEXT("hare") || Def==TEXT("hen") || Def==TEXT("pheasant")?65:Def==TEXT("black_bear")?220:160;Scale=FVector(Length/FMath::Max(Extent.X*2,Extent.Y*2));}
        else Scale=FVector(1.2,.55,.65);
        Combat->Id=FName(Id.ToString());Combat->Region=NAME_None;Combat->Protected=false;Combat->RewardKind=TEXT("hunt");
        auto* Head=NewObject<UStaticMeshComponent>(this,TEXT("Head"));Head->SetupAttachment(RootComponent);Head->RegisterComponent();
        Head->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));Head->SetWorldScale3D(FVector(.32));Head->SetRelativeLocation(FVector(50,0,55)/Scale);
        Head->SetHiddenInGame(Mesh!=nullptr);Head->SetCollisionProfileName(TEXT("BlockAllDynamic"));Head->ComponentTags.Add(TEXT("head"));Parts.Add(Head);
    }
    else if(Kind==TEXT("pen"))Scale=FVector(4,4,.15);
    else if(Kind==TEXT("crop"))Scale=FVector(2,2,.08);
    else if(Kind==TEXT("fish"))Scale=FVector(.45,.45,1.2);
    else if(Kind==TEXT("treasure"))Scale=FVector(.8,.65,.5);
    else if(Def==TEXT("tree"))Scale=FVector(.6,.6,3);
    else Scale=FVector(.7,.7,.35);
    if(Kind==TEXT("resource"))
    {
        const FString Natural=Def==TEXT("tree")?TEXT("SM_Tree"):(Def==TEXT("stone_outcrop") || Def==TEXT("loose_stones") || Def.ToString().Contains(TEXT("ore_vein")))?TEXT("SM_Rock"):TEXT("SM_Shrub");
        Mesh=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes/")+Natural+TEXT(".")+Natural));
        if(Mesh){const float Length=Def==TEXT("tree")?500:100;Scale=FVector(Length/FMath::Max(1.,Mesh->GetBounds().BoxExtent.GetMax()*2));}
    }
    AdultScale=Scale;
    if(Kind==TEXT("animal"))if(const auto* A=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->State.Animals.FindByPredicate([&](const auto& X){return X.Id==Id;});A && A->Juvenile)Scale*=.55;
    Shape->SetStaticMesh(Mesh?Mesh:LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Shape->SetWorldScale3D(Scale);
    if(Kind==TEXT("pen") || Kind==TEXT("crop"))Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(Kind==TEXT("animal"))
    {
        Shape->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
        for(TActorIterator<AHearthwardNatureActor> It(GetWorld());It;++It)if(*It!=this && It->Kind==TEXT("animal"))
        {Shape->IgnoreActorWhenMoving(*It,true);It->Shape->IgnoreActorWhenMoving(this,true);}
        FVector Floor;if(GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->Ground(GetActorLocation(),Floor))
        {const auto Bounds=Shape->GetStaticMesh()->GetBounds();SetActorLocation(Floor+FVector(0,0,(Bounds.BoxExtent.Z-Bounds.Origin.Z)*Scale.Z+2));}
    }
    if(Kind==TEXT("animal"))for(const auto& Part:Parts){Part->SetWorldScale3D(FVector(.32));Part->SetRelativeLocation(FVector(65,0,55)/Scale);}
    auto Part=[&](FVector Offset,FVector Size,FLinearColor Color)
    {
        auto* P=NewObject<UStaticMeshComponent>(this);P->SetupAttachment(RootComponent);P->RegisterComponent();P->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        P->SetWorldScale3D(Size);P->SetWorldLocation(GetActorLocation()+Offset);P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if(auto* M=P->CreateAndSetMaterialInstanceDynamic(0))M->SetVectorParameterValue(TEXT("Color"),Color);Parts.Add(P);
    };
    if(Kind==TEXT("pen"))
    {
        const FLinearColor Wood(.25f,.12f,.04f);
        for(int32 X:{-180,180})for(int32 Y:{-180,180})Part(FVector(X,Y,50),FVector(.15,.15,1),Wood);
        for(int32 Side:{-1,1})for(int32 Z:{35,75})
        {Part(FVector(Side*180,0,Z),FVector(.1,3.6,.1),Wood);Part(FVector(0,Side*180,Z),FVector(3.6,.1,.1),Wood);}
        Part(FVector(0,150,20),FVector(.9,.4,.3),Wood);
    }
    if(Kind==TEXT("crop"))
    {
        if(auto* M=Shape->CreateAndSetMaterialInstanceDynamic(0))M->SetVectorParameterValue(TEXT("Color"),FLinearColor(.16f,.08f,.025f));
        for(int32 Plant=0;Plant<9;++Plant)
        {
            auto* P=NewObject<UStaticMeshComponent>(this);P->SetupAttachment(RootComponent);
            P->SetCollisionEnabled(ECollisionEnabled::NoCollision);P->SetCanEverAffectNavigation(false);
            P->RegisterComponent();Parts.Add(P);
        }
    }
    if(Kind==TEXT("treasure")){Part(FVector(0,0,30),FVector(.85,.7,.12),FLinearColor(.25f,.13f,.04f));for(int32 X:{-25,25})Part(FVector(X,0,0),FVector(.07,.68,.55),FLinearColor(.45f,.4f,.22f));}
    Label->SetWorldScale3D(FVector(1));Label->SetRelativeLocation(FVector(0,0,Kind==TEXT("animal")?120:160)/Scale);
    Refresh();
    if(Kind==TEXT("animal") && AnimalMotion->Configure(Definition))Shape->SetVisibility(false,false);
}
void AHearthwardNatureActor::Refresh()
{
    auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
    const auto* A=N->State.Animals.FindByPredicate([&](const auto& X){return X.Id==Id;});
    const TCHAR* Table=Kind==TEXT("animal")?(A && A->Domestic?TEXT("domestic"):TEXT("wildlife")):Kind==TEXT("crop")?TEXT("crops"):Kind==TEXT("pen")?TEXT("domestic"):Kind==TEXT("fish")?TEXT("fish"):TEXT("resources");
    FString Name=Text(HearthwardNature::Definition(Table,Definition),TEXT("name"));
    if(Kind==TEXT("pen"))Name+=TEXT("栏舍");if(Kind==TEXT("fish"))Name=TEXT("钓鱼点");if(Kind==TEXT("treasure"))Name=TEXT("藏宝箱");
    if(Kind==TEXT("resource"))
    {
        const auto* Point=N->State.Points.FindByPredicate([&](const auto& P){return P.Id==Id;});
        const auto* Source=Point?GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->Source(Point->Key.ToString()):nullptr;
        if(Source){Name+=FString::Printf(TEXT(" · %d/%d"),Source->Remaining,Source->Capacity);Shape->SetVisibility(Source->Remaining>0 && !Source->Blocked);Shape->SetCollisionEnabled(Source->Remaining>0 && !Source->Blocked?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);}
    }
    if(A)
    {
        const FVector DesiredScale=AdultScale*(A->Juvenile?.55:1.);
        if(!Shape->GetComponentScale().Equals(DesiredScale))
        {
            Shape->SetWorldScale3D(DesiredScale);FVector Floor;
            if(N->Ground(GetActorLocation(),Floor)){const auto Bounds=Shape->GetStaticMesh()->GetBounds();SetActorLocation(Floor+FVector(0,0,(Bounds.BoxExtent.Z-Bounds.Origin.Z)*DesiredScale.Z+2));}
        }
        Combat->MaximumHealth=Number(HearthwardNature::Definition(Table,Definition),TEXT("health"));Combat->Health=A->Health;
        if(A->Health<=0)Name+=A->Loot.IsEmpty()?TEXT("（已处理）"):TEXT("（可拾取）");
        else if(A->Juvenile)Name+=TEXT("幼崽");
        if(A->Health<=0){Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);for(const auto& Part:Parts)Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
    }
    else SetActorLocation(N->Position(Id));
    if(Kind==TEXT("crop"))if(const auto* Crop=N->State.Crops.FindByPredicate([&](const auto& C){return C.Id==Id;}))
    {
        const double Duration=Number(HearthwardNature::Definition(TEXT("crops"),Crop->Definition),TEXT("days"))*1440;
        const double Growth=N->State.Ready(*Crop)?1.:N->State.Calendar-Crop->Planted>=Duration*.5?.65:.3;
        const FString PlantName=Definition==TEXT("greens")?TEXT("Greens"):Definition==TEXT("grain")?TEXT("Grain"):TEXT("Herb");
        const FString Stage=N->State.Ready(*Crop)?TEXT("Mature"):TEXT("Young");
        const FString Stem=PlantName+TEXT("_")+Stage;
        auto* PlantMesh=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Hearthward/Assets/TASK-097/Crops/")+Stem+TEXT("/SM_")+Stem+TEXT(".SM_")+Stem));
        for(int32 I=0;I<Parts.Num();++I)
        {
            if(Parts[I]->GetStaticMesh()!=PlantMesh)Parts[I]->SetStaticMesh(PlantMesh);
            Parts[I]->SetWorldScale3D(FVector(Growth));
            Parts[I]->SetWorldLocation(GetActorLocation()+FVector((I/3-1)*60,(I%3-1)*60,4));
        }
    }
    InteractionText=Name;
    // The world-space engine font has no CJK glyphs; the existing HUD renders the localized prompt.
    Label->SetText(FText::FromString(TEXT("E")));
}
void AHearthwardNatureActor::Tick(float Delta)
{
    Super::Tick(Delta);if(GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->Suspended())return;
    Combat->Memory.HitRemaining=FMath::Max(0.,Combat->Memory.HitRemaining-Delta);
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    if(PC){Label->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation()-Label->GetComponentLocation()).Rotation());Label->SetVisibility(PC->GetPawn() && FVector::Dist2D(PC->GetPawn()->GetActorLocation(),GetActorLocation())<300);}
    if(Kind==TEXT("animal"))MoveAnimal(Delta);
}
void AHearthwardNatureActor::MoveAnimal(float Delta)
{
    const FName PreviousIntent=MotionIntent;
    MotionIntent=TEXT("idle");MotionThreat=NAME_None;
    auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();auto* A=N->State.Animals.FindByPredicate([&](const auto& X){return X.Id==Id;});
    if(!A)return;
    if(A->Health<=0){MotionIntent=TEXT("dead");return;}
    A->AlertRemaining=FMath::Max(0.,A->AlertRemaining-Delta);AttackDelay-=Delta;WanderDelay-=Delta;Age+=Delta;
    if(A->AlertRemaining<=0)A->Threat=NAME_None;
    MotionThreat=A->Threat;
    if(!Combat->CanAct()){MotionIntent=TEXT("hit");return;}
    if(N->WorkingOn(Id)){MotionIntent=TEXT("working");return;}
    auto* Player=UGameplayStatics::GetPlayerPawn(this,0);if(!Player)return;
    const auto D=HearthwardNature::Definition(A->Domestic?TEXT("domestic"):TEXT("wildlife"),Definition);
    FVector Goal=A->Destination;double Speed=100;
    if(A->Domestic && A->Captured)
    {
        MotionIntent=TEXT("captured");MotionThreat=NAME_None;
        if(A->Following)
        {
            const auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();if(!G || G->InCombat() || A->AlertRemaining>0){A->Following=false;A->FollowingBrother=false;return;}
            AActor* Leader=Player;
            if(A->FollowingBrother)
            {
                AHearthwardCompanionFixture* Brother=nullptr;
                for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)if(It->LeadingAnimal(Id)){Brother=*It;break;}
                if(!Brother){A->Following=false;A->FollowingBrother=false;return;}
                Leader=Brother;
                if(FVector::Dist2D(Player->GetActorLocation(),Leader->GetActorLocation())>3000)return;
            }
            const auto* Pen=N->State.Pens.FindByPredicate([&](const auto& P){return P.Id==A->ReservedPen;});
            if(!Pen)return;
            if(FVector::Dist2D(GetActorLocation(),Pen->Position)<190){A->Pen=Pen->Id;A->ReservedPen.Invalidate();A->Following=false;A->FollowingBrother=false;return;}
            if(FVector::Dist2D(Leader->GetActorLocation(),Pen->Position)<600)Goal=Pen->Position;
            else if(FVector::Dist2D(Leader->GetActorLocation(),GetActorLocation())>220)Goal=Leader->GetActorLocation();else return;
            Speed=300;MotionIntent=TEXT("lead");
        }
        else return;
    }
    else
    {
        const FString Behavior=A->Domestic?TEXT("flee"):Text(D,TEXT("behavior"));
        const double DetectRadius=A->Domestic?1200:Number(D,TEXT("detect_m"))*100;
        auto Identity=[&](const AActor* Actor){return Actor==Player?FName(TEXT("player")):FName(TEXT("brother"));};
        auto Living=[](AActor* Actor)
        {
            auto* S=Actor?Actor->FindComponentByClass<UHearthwardSurvivalComponent>():nullptr;
            return S && S->Enabled() && S->Alive() && S->Health()>0;
        };
        auto Visible=[&](AActor* Actor)
        {
            FCollisionQueryParams Query(SCENE_QUERY_STAT(NatureSight),false,this);Query.AddIgnoredActor(Actor);
            return !GetWorld()->LineTraceTestByChannel(GetActorLocation(),Actor->GetActorLocation(),ECC_Visibility,Query);
        };
        TArray<AActor*> Candidates;if(Living(Player))Candidates.Add(Player);
        for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)if(Living(*It))Candidates.Add(*It);
        AActor* Threat=nullptr;AActor* SeenThreat=nullptr;double SeenDistance=DetectRadius;
        Combat->Memory.Seen.Reset();
        for(auto* Candidate:Candidates)
        {
            const FName Who=Identity(Candidate);const double Distance=FVector::Dist2D(Candidate->GetActorLocation(),GetActorLocation());
            if(A->AlertRemaining>0 && A->Threat==Who)Threat=Candidate;
            if(Distance<DetectRadius && Visible(Candidate))
            {
                Combat->Memory.Seen.Add(Who);Combat->Memory.LastKnown.Add(Who,Candidate->GetActorLocation());
                if(Distance<SeenDistance){SeenDistance=Distance;SeenThreat=Candidate;}
            }
        }
        if(!Threat)Threat=SeenThreat;
        const bool Detected=Threat && Combat->Memory.Seen.Contains(Identity(Threat));
        const double Distance=Threat?FVector::Dist2D(Threat->GetActorLocation(),GetActorLocation()):DBL_MAX;
        if(Detected){A->Destination=Threat->GetActorLocation();MotionThreat=Identity(Threat);MotionIntent=TEXT("alert");}
        const bool Attack=Threat && !A->Domestic && (Behavior==TEXT("aggressive")?Detected || A->AlertRemaining>0:Behavior==TEXT("retaliate") && A->AlertRemaining>0 && A->Threat==Identity(Threat));
        const auto* Home=N->State.Slots.FindByPredicate([&](const auto& S){return S.Id==A->Slot;});
        const bool TooFar=Home && FVector::Dist2D(GetActorLocation(),Home->Position)>Number(D,TEXT("flee_m"))*100;
        if(Attack && !TooFar)
        {
            if(Detected){A->AlertRemaining=15;A->Threat=Identity(Threat);}
            Goal=A->Destination;Speed=Number(D,TEXT("run_mps"))*100;MotionIntent=TEXT("chase");MotionThreat=A->Threat;
            if(Detected && Distance<180 && AttackDelay<=0)
            {
                AttackDelay=Number(D,TEXT("attack_interval_seconds"));++AttackSequence;const float Raw=100.f/.95f/7.f;
                if(auto* C=Threat->FindComponentByClass<UHearthwardCombatComponent>())C->Damage(Raw,TEXT("body"),GetActorLocation());
                else if(auto* S=Threat->FindComponentByClass<UHearthwardSurvivalComponent>())
                {
                    auto* Bag=Threat->FindComponentByClass<UHearthwardInventoryComponent>();const auto* I=Bag?Bag->FindInstance(Bag->EquippedInstance(TEXT("chest"))):nullptr;
                    const auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();
                    const bool Armored=I && I->Durability>0;
                    const double Armor=Armored?Number(Find(TEXT("items"),I->Definition.ToString()),TEXT("defense"))/100+(G?G->Effect(TEXT("local_armor_bonus")):0):0;
                    S->ReceiveDamage(HearthwardCombat::ArmorDamage(Raw,Armor,G?G->Effect(TEXT("defense")):0),FGuid::NewGuid(),GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch());
                    if(Armored)Bag->WearInstance(I->Id,1/(1+(G?G->Effect(TEXT("durability")):0)));
                }
            }
        }
        else if(Threat && (Detected || A->AlertRemaining>0) && Behavior==TEXT("flee") && !TooFar)
        {
            if(Detected){A->AlertRemaining=3;A->Threat=Identity(Threat);}
            Goal=GetActorLocation()+(GetActorLocation()-A->Destination).GetSafeNormal2D()*600;
            const auto* G=Player->FindComponentByClass<UHearthwardGameplayComponent>();
            Speed=AnimalMotion->Ready()?AnimalMotion->EscapeSpeed():630*(1+(G?G->Effect(TEXT("sprint")):0));
            MotionIntent=TEXT("flee");MotionThreat=A->Threat;
        }
        else if(TooFar){Goal=Home->Position;Speed=160;A->AlertRemaining=0;A->Threat=NAME_None;MotionIntent=TEXT("return");MotionThreat=NAME_None;}
        else if(WanderDelay<=0)
        {
            const double Hour=GetWorld()->GetSubsystem<UHearthwardWorldClockSubsystem>()->GetSnapshot().MinuteOfDay/60;const FString Active=Text(D,TEXT("active"));
            const bool ActiveNow=Active==TEXT("night")?(Hour<6 || Hour>=18):Active==TEXT("dawn_dusk")?((Hour>=5 && Hour<8) || (Hour>=17 && Hour<20)):(Hour>=6 && Hour<18);
            WanderDelay=(4+FMath::FRand()*4)*(ActiveNow?1:3);const FVector Center=Home?Home->Position:A->Position;Goal=Center+FVector(FMath::FRandRange(-500.f,500.f),FMath::FRandRange(-500.f,500.f),0);A->Destination=Goal;
        }
    }
    if(MotionIntent!=PreviousIntent)PathRemaining=0;
    if(FVector::Dist2D(GetActorLocation(),Goal)<70)return;
    FVector Direction=(Goal-GetActorLocation()).GetSafeNormal2D();
    PathRemaining-=Delta;
    if(PathRemaining<=0)
    {
        PathRemaining=.5;PathNext=Goal;
        if(auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),GetActorLocation(),Goal);Path && Path->PathPoints.Num()>1)PathNext=Path->PathPoints[1];
    }
    if(FVector::Dist2D(GetActorLocation(),PathNext)>50)Direction=(PathNext-GetActorLocation()).GetSafeNormal2D();
    FVector Next=GetActorLocation()+Direction*FMath::Min(double(Delta)*Speed,FVector::Dist2D(GetActorLocation(),Goal));
    FVector Floor;if(!N->Ground(Next,Floor))return;
    const auto Bounds=Shape->GetStaticMesh()->GetBounds();
    Next.Z=Floor.Z+(Bounds.BoxExtent.Z-Bounds.Origin.Z)*Shape->GetComponentScale().Z+2;
    SetActorRotation(Direction.Rotation());SetActorLocation(Next,true);A->Position=GetActorLocation();
}
FString UHearthwardNatureInteraction::GetInteractionPrompt(AActor* Interactor) const
{const auto* A=Cast<AHearthwardNatureActor>(GetOwner());return A?A->InteractionText+(A->Kind==TEXT("fish")?TEXT(" · E 开始钓鱼"):TEXT(" · E 操作")):FString();}
FString UHearthwardNatureInteraction::CompleteInteraction(AActor* Interactor)
{
    auto* A=Cast<AHearthwardNatureActor>(GetOwner());auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
    if(!A || N->Busy())return N->Feedback;
    if(A->Kind==TEXT("fish")){N->Act(TEXT("fish"),A->Id,NAME_None,GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch());return N->Feedback;}
    if(A->Kind==TEXT("resource")){N->Act(TEXT("harvest"),A->Id,NAME_None,GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch());return N->Feedback;}
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0))if(auto* HUD=Cast<AHearthwardHUD>(PC->GetHUD());HUD && HUD->Screen)HUD->Screen->OpenNature(A->Id);
    return N->Feedback;
}
