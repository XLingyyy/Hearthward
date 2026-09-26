#include "HearthwardBuildingComponent.h"
#include "../Camp/HearthwardCampSubsystem.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Interaction/HearthwardFurnitureInteractionComponent.h"
#include "../Interaction/HearthwardResourceInteractionComponent.h"
#include "../Inventory/HearthwardStorageAccessComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Actions/HearthwardTimedActionComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

using namespace HearthwardData;
namespace
{
FVector Extent(const TSharedPtr<FJsonObject>& R)
{
    const auto& A=R->GetArrayField(TEXT("extent"));
    return FVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber());
}
double Tuning(const TCHAR* Key) { return Number(Catalog()->GetObjectField(TEXT("construction")),Key); }
}
UHearthwardBuildingComponent::UHearthwardBuildingComponent() { PrimaryComponentTick.bCanEverTick=true; }
void UHearthwardBuildingComponent::BeginPlay()
{
    Super::BeginPlay();
    auto* Timer=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>();
    Timer->OnTimerCompleted.AddDynamic(this,&UHearthwardBuildingComponent::Complete);
    Timer->OnInterrupted.AddDynamic(this,&UHearthwardBuildingComponent::Interrupted);
}
void UHearthwardBuildingComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseMaterials();ClearPreview();
    for(auto& B:Built) if(B.Actor.IsValid()) B.Actor->Destroy();
    Super::EndPlay(Reason);
}
AActor* UHearthwardBuildingComponent::SpawnBuilding(FName Id,FVector Position,float Rotation,bool PreviewOnly)
{
    const auto R=Find(TEXT("buildings"),Id.ToString());
    auto* A=GetWorld()->SpawnActor<AActor>();
    if(!A) return nullptr;
    const FVector Half=Extent(R);
    auto* Root=NewObject<UBoxComponent>(A); A->AddInstanceComponent(Root); A->SetRootComponent(Root);
    Root->SetBoxExtent(Half); Root->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Root->SetCollisionEnabled(PreviewOnly?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);
    Root->SetCanEverAffectNavigation(!PreviewOnly); Root->RegisterComponent();
    A->SetActorLocation(Position+FVector(0,0,Half.Z)); A->SetActorRotation(FRotator(0,Rotation,0));
    A->Tags.Add(PreviewOnly?TEXT("Hearthward.Building.Preview"):TEXT("Hearthward.Building.Completed"));
    for(const auto& V:R->GetArrayField(TEXT("parts")))
    {
        const auto Part=V->AsObject();
        auto* Mesh=NewObject<UStaticMeshComponent>(A); A->AddInstanceComponent(Mesh); Mesh->SetupAttachment(Root);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Text(Part,TEXT("mesh"))));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCanEverAffectNavigation(false);
        Mesh->SetRelativeLocation(HearthwardData::Position(Part));
        const auto& Scale=Part->GetArrayField(TEXT("scale"));
        Mesh->SetRelativeScale3D(FVector(Scale[0]->AsNumber(),Scale[1]->AsNumber(),Scale[2]->AsNumber()));
        Mesh->RegisterComponent();
    }
    if(!PreviewOnly && (Id==TEXT("bed") || Id==TEXT("campfire") || Id==TEXT("medical_area")))
    {
        auto* Interaction=NewObject<UHearthwardFurnitureInteractionComponent>(A);
        A->AddInstanceComponent(Interaction); Interaction->SetupAttachment(Root);
        Interaction->Kind=Id; Interaction->MaxDistance=220; Interaction->RegisterComponent();
    }
    if(!PreviewOnly && Id==TEXT("warehouse_access"))
    {
        auto* Access=NewObject<UHearthwardStorageAccessComponent>(A);A->AddInstanceComponent(Access);Access->RegisterComponent();
        auto* Interaction=NewObject<UHearthwardResourceInteractionComponent>(A);A->AddInstanceComponent(Interaction);Interaction->SetupAttachment(Root);Interaction->RegisterComponent();Interaction->InitializeResource(true);
    }
    return A;
}
TMap<FName,int32> UHearthwardBuildingComponent::Materials() const
{
    if(Editing.IsValid() && !Upgrading)return {};
    int32 Level=1;
    if(Upgrading) if(const auto* B=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Facilities.FindByPredicate([&](const auto& F){return F.Id==Editing;}))Level=B->Level+1;
    return HearthwardCamp::BuildCost(Selected,Level);
}
bool UHearthwardBuildingComponent::HasMaterials() const
{
    const auto* Bag=GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>();
    if(MaterialTicket.IsValid())return true;
    const auto* Store=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>();
    for(const auto& M:Materials()) if(Bag->Available(M.Key)+Store->Available(M.Key)<M.Value) return false;
    return true;
}
bool UHearthwardBuildingComponent::SelectBuilding(FName Id)
{
    const auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    if(!G->Enabled || G->Health<=0 || !Find(TEXT("buildings"),Id.ToString()) || IsBuilding()
        || HearthwardCamp::RequiredTier(Id,1)>GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Tier) return false;
    ClearPreview(); Selected=Id; Yaw=Cast<APawn>(GetOwner())->GetControlRotation().Yaw;
    Feedback=TEXT("移动和转动视角调整位置，Q旋转，左键建造，右键取消");
    return true;
}
void UHearthwardBuildingComponent::ClearPreview()
{
    if(Editing.IsValid()) if(auto* B=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.Facilities.FindByPredicate([&](const auto& F){return F.Id==Editing;}))B->Paused=false;
    if(Preview.IsValid()) Preview->Destroy();
    Preview.Reset(); Selected=NAME_None; ValidPlacement=false;Editing.Invalidate();Upgrading=false;
}
void UHearthwardBuildingComponent::CancelPlacement()
{
    if(Pending) GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>()->InterruptAction();
    Pending=false; ReleaseMaterials();ClearPreview(); Feedback=TEXT("已取消建造，材料未消耗");
}
void UHearthwardBuildingComponent::Interrupted()
{
    if(!Pending) return;
    Pending=false; ReleaseMaterials();ClearPreview(); Feedback=TEXT("建造已中断，材料未消耗");
}
void UHearthwardBuildingComponent::RotatePreview()
{ if(IsPlacing() && !Pending) Yaw=FMath::Fmod(Yaw+Tuning(TEXT("rotationStep")),360.0); }
bool UHearthwardBuildingComponent::CheckPlacement(FString& Reason) const
{
    const auto R=Find(TEXT("buildings"),Selected.ToString()); if(!R) return false;
    const auto* G=GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>();
    const auto* Player=Cast<ACharacter>(GetOwner());
    if(!G->Enabled || G->Health<=0 || G->InCombat() || !Player->GetCharacterMovement()->IsMovingOnGround())
    { Reason=TEXT("请在地面安全处建造"); return false; }
    if(!R->GetBoolField(TEXT("wilderness")) && GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->State.CampAt(Placement).IsNone())
    { Reason=TEXT("该设施只能建在营地范围内"); return false; }
    if(FVector::Dist2D(Placement,GetOwner()->GetActorLocation())>Tuning(TEXT("reach")))
    { Reason=TEXT("建造位置过远"); return false; }
    const FVector Half=Extent(R); const FQuat Rotation(FRotator(0,Yaw,0));
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HearthwardBuild),false);
    Query.AddIgnoredActor(Preview.Get());
    Query.AddIgnoredActor(ResolveFacility(Editing));
    // Sample the centre and all footprint corners: a single ray would allow a bench over a ledge.
    for(const FVector Offset:{FVector::ZeroVector,FVector(Half.X,Half.Y,0),FVector(-Half.X,Half.Y,0),FVector(Half.X,-Half.Y,0),FVector(-Half.X,-Half.Y,0)})
    {
        const FVector P=Placement+Rotation.RotateVector(Offset); FHitResult Hit;
        if(!GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,30),P-FVector(0,0,30),ECC_Visibility,Query)
            || Cast<APawn>(Hit.GetActor()) || Hit.GetActor()->Tags.Contains(TEXT("Hearthward.Building.Completed"))
            || Hit.ImpactNormal.Z<Tuning(TEXT("minNormalZ")) || FMath::Abs(Hit.ImpactPoint.Z-Placement.Z)>Tuning(TEXT("supportTolerance")))
        { Reason=TEXT("需要平整地面，边缘不能悬空或叠在建筑上"); return false; }
    }
    FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic); Objects.AddObjectTypesToQuery(ECC_Pawn);
    if(GetWorld()->OverlapAnyTestByObjectType(Placement+FVector(0,0,Half.Z+2),Rotation,Objects,FCollisionShape::MakeBox(Half-FVector(1,1,1)),Query))
    { Reason=TEXT("该位置被建筑、障碍或角色占用"); return false; }
    if(!HasMaterials()) { Reason=TEXT("背包与共享仓储材料不足，或材料已被其他操作预留"); return false; }
    Reason=TEXT("可以建造：左键确认，Q旋转，右键取消"); return true;
}
void UHearthwardBuildingComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    if(!IsPlacing() || GetWorld()->IsPaused()) return;
    if(Pending)
    {
        FString Reason;
        if(FVector::DistSquared(StartedAt,GetOwner()->GetActorLocation())>25 || !CheckPlacement(Reason))
            GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>()->InterruptAction();
        return;
    }
    const auto* Pawn=Cast<APawn>(GetOwner());
    const FVector Forward=FRotator(0,Pawn->GetControlRotation().Yaw,0).Vector();
    const FVector Aim=GetOwner()->GetActorLocation()+Forward*Tuning(TEXT("previewDistance"));
    FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(HearthwardBuildAim),false,GetOwner()); Query.AddIgnoredActor(Preview.Get());
    const bool Ground=GetWorld()->LineTraceSingleByChannel(Hit,Aim+FVector(0,0,150),Aim-FVector(0,0,400),ECC_Visibility,Query);
    Placement=Ground?Hit.ImpactPoint:Aim;
    ValidPlacement=Ground && CheckPlacement(Feedback);
    if(!Ground) Feedback=TEXT("没有可支撑建筑的地面");
    const FVector Half=Extent(Find(TEXT("buildings"),Selected.ToString()));
    if(!Preview.IsValid()) Preview=SpawnBuilding(Selected,Placement,Yaw,true);
    if(Preview.IsValid()) { Preview->SetActorLocation(Placement+FVector(0,0,Half.Z)); Preview->SetActorRotation(FRotator(0,Yaw,0)); }
    DrawDebugBox(GetWorld(),Placement+FVector(0,0,Half.Z),Half,FQuat(FRotator(0,Yaw,0)),ValidPlacement?FColor::Green:FColor::Red,false,0,0,2);
}
bool UHearthwardBuildingComponent::ConfirmPlacement()
{
    if(!IsPlacing() || Pending || GetWorld()->IsPaused() || !CheckPlacement(Feedback)) return false;
    auto* Timer=GetOwner()->FindComponentByClass<UHearthwardTimedActionComponent>();
    if(!ReserveMaterials()) {Feedback=TEXT("材料预留失败");return false;}
    if(!Timer->StartAction()) { ReleaseMaterials();Feedback=TEXT("请先完成当前动作"); return false; }
    Pending=true; StartedAt=GetOwner()->GetActorLocation(); Feedback=TEXT("建造中，移动或受伤将中断"); return true;
}
void UHearthwardBuildingComponent::Complete()
{
    if(!Pending) return;
    Pending=false;
    if(!CheckPlacement(Feedback)) { ReleaseMaterials();ClearPreview(); return; }
    TGuardValue<bool> Guard(Settling,true);
    auto* Economy=GetWorld()->GetSubsystem<UHearthwardCampSubsystem>();
    TGuardValue<bool> EconomyGuard(Economy->Settling,true);
    if(Editing.IsValid())
    {
        auto* Existing=Built.FindByPredicate([&](const auto& B){return B.Id==Editing;});
        if(!Existing || !Existing->Actor.IsValid()){ReleaseMaterials();ClearPreview();return;}
        const auto Paid=Materials();
        if(!CommitMaterials()){Feedback=TEXT("材料结算失败");ReleaseMaterials();ClearPreview();return;}
        if(Upgrading) Economy->CompleteFacilityUpgrade(Editing,Paid);
        else
        {
            Existing->Position=Placement;Existing->Rotation=Yaw;
            Existing->Actor->SetActorLocation(Placement+FVector(0,0,Extent(Find(TEXT("buildings"),Selected.ToString())).Z));
            Existing->Actor->SetActorRotation(FRotator(0,Yaw,0));
            if(auto* F=Economy->State.Facilities.FindByPredicate([&](const auto& B){return B.Id==Editing;}))
            {
                const FName NewCamp=Economy->State.CampAt(Placement);
                if(F->Camp!=NewCamp) for(auto& Region:Economy->State.Regions)if(Region.Facility==Editing)Region.Camp=NewCamp;
                F->Camp=NewCamp;
            }
        }
        GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>()->OnInventoryChanged.Broadcast();Feedback=Upgrading?TEXT("设施升级完成"):TEXT("设施移动完成");ClearPreview();return;
    }
    auto* Actor=SpawnBuilding(Selected,Placement,Yaw,false);
    if(!Actor) { Feedback=TEXT("建造失败，材料未消耗"); ReleaseMaterials();ClearPreview(); return; }
    const auto Paid=Materials();
    if(!CommitMaterials()){Actor->Destroy();Feedback=TEXT("材料结算失败");ReleaseMaterials();ClearPreview();return;}
    // Publish both the constructed object and all material changes before inventory observers run.
    Built.Add({FGuid::NewGuid(),Selected,Placement,Yaw,Actor});
    Economy->RegisterFacility(Built.Last().Id,Selected,Placement,Paid);
    GetOwner()->FindComponentByClass<UHearthwardInventoryComponent>()->OnInventoryChanged.Broadcast();
    GetOwner()->FindComponentByClass<UHearthwardGameplayComponent>()->Record(TEXT("build"),Selected);
    Feedback=TEXT("建造完成：")+Text(Find(TEXT("buildings"),Selected.ToString()),TEXT("name")); ClearPreview();
}
TArray<AActor*> UHearthwardBuildingComponent::GetBuildings() const
{ TArray<AActor*> Out; for(const auto& B:Built) if(B.Actor.IsValid()) Out.Add(B.Actor.Get()); return Out; }
TArray<TSharedPtr<FJsonValue>> UHearthwardBuildingComponent::Snapshot() const
{
    TArray<TSharedPtr<FJsonValue>> Out;
    for(const auto& B:Built)
    {
        auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"),B.Id.ToString()); R->SetStringField(TEXT("recipe"),B.Recipe.ToString());
        R->SetStringField(TEXT("position"),B.Position.ToString()); R->SetNumberField(TEXT("yaw"),B.Rotation); Out.Add(MakeShared<FJsonValueObject>(R));
    }
    return Out;
}
bool UHearthwardBuildingComponent::Validate(const TArray<TSharedPtr<FJsonValue>>& Entries)
{
    TSet<FGuid> Seen;
    for(const auto& V:Entries)
    {
        if(V->Type!=EJson::Object) return false;
        const auto R=V->AsObject(); FGuid Id; FVector P; double Rotation;
        if(!FGuid::Parse(Text(R,TEXT("id")),Id) || !Id.IsValid() || Seen.Contains(Id) || !Find(TEXT("buildings"),Text(R,TEXT("recipe")))
            || !P.InitFromString(Text(R,TEXT("position"))) || P.ContainsNaN() || !R->TryGetNumberField(TEXT("yaw"),Rotation) || !FMath::IsFinite(Rotation)) return false;
        Seen.Add(Id);
    }
    return true;
}
void UHearthwardBuildingComponent::Restore(const TArray<TSharedPtr<FJsonValue>>& Entries)
{
    Pending=false;ReleaseMaterials(); ClearPreview(); Feedback.Reset();
    for(auto& B:Built) if(B.Actor.IsValid()) B.Actor->Destroy();
    Built.Reset();
    for(const auto& V:Entries)
    {
        const auto R=V->AsObject(); FBuilt B; FGuid::Parse(Text(R,TEXT("id")),B.Id); B.Recipe=FName(*Text(R,TEXT("recipe")));
        B.Position.InitFromString(Text(R,TEXT("position"))); B.Rotation=Number(R,TEXT("yaw"));
        B.Actor=SpawnBuilding(B.Recipe,B.Position,B.Rotation,false); Built.Add(B);
        GetWorld()->GetSubsystem<UHearthwardCampSubsystem>()->RegisterFacility(B.Id,B.Recipe,B.Position,{});
    }
}
