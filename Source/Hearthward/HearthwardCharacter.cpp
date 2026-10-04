#include "HearthwardCharacter.h"
#include "Experience/HearthwardPlayerSettings.h"
#include "Experience/HearthwardPresentationComponent.h"
#include "Experience/HearthwardTraversalComponent.h"
#include "Nature/HearthwardNatureSubsystem.h"
#include "Engine/GameInstance.h"
#include "InputTriggers.h"
#include "Misc/CoreDelegates.h"
#include "Campaign/HearthwardCampaignSubsystem.h"
#include "Interaction/HearthwardResourceInteractionComponent.h"
#include "Combat/HearthwardCombatComponent.h"
#include "Combat/HearthwardProjectile.h"
#include "Survival/HearthwardSurvivalComponent.h"
#include "Companion/HearthwardCompanionFixture.h"
#include "Animation/HearthwardHeroAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Building/HearthwardBuildingComponent.h"
#include "Building/HearthwardTask028CampHouse.h"
#include "Gameplay/HearthwardGameplayComponent.h"
#include "Actions/HearthwardTimedActionComponent.h"
#include "Inventory/HearthwardInventoryComponent.h"
#include "UI/HearthwardHUD.h"
#include "UI/HearthwardScreenWidget.h"
#include "Interaction/HearthwardInteractionComponent.h"
#include "Save/HearthwardSaveSubsystem.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/ConfigCacheIni.h"

AHearthwardCharacter::AHearthwardCharacter(const FObjectInitializer& Initializer)
    : Super(Initializer.SetDefaultSubobjectClass<UHearthwardMovementComponent>(ACharacter::CharacterMovementComponentName))
{
    PrimaryActorTick.bCanEverTick=true;
    CreateDefaultSubobject<UHearthwardTraversalComponent>(TEXT("Traversal"));
    CreateDefaultSubobject<UHearthwardPresentationComponent>(TEXT("Presentation"));
    CreateDefaultSubobject<UHearthwardCombatComponent>(TEXT("Combat"));
    CreateDefaultSubobject<UHearthwardSurvivalComponent>(TEXT("Survival"));
    Gameplay = CreateDefaultSubobject<UHearthwardGameplayComponent>(TEXT("Gameplay"));
    CreateDefaultSubobject<UHearthwardBuildingComponent>(TEXT("Building"));
    TimedAction = CreateDefaultSubobject<UHearthwardTimedActionComponent>(TEXT("TimedAction"));
    Inventory = CreateDefaultSubobject<UHearthwardInventoryComponent>(TEXT("Inventory"));
    Interaction = CreateDefaultSubobject<UHearthwardInteractionComponent>(TEXT("Interaction"));
    GetCapsuleComponent()->InitCapsuleSize(34.0f, 90.0f);
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    // GDD v0.3 Q177: accepted initial tuning, 3.5 m/s without load.
    GetCharacterMovement()->MaxWalkSpeed = 350.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
    GetCharacterMovement()->MaxStepHeight=45;
    GetCharacterMovement()->SetWalkableFloorAngle(45);
    GetCharacterMovement()->MaxSwimSpeed=300;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanSwim=true;

    auto* Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    Boom->SetupAttachment(GetRootComponent());
    Boom->TargetArmLength = 400.0f;
    Boom->TargetOffset = FVector(0.0f, 0.0f, 60.0f);
    Boom->bUsePawnControlRotation = true;
    auto* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> HeroMesh(TEXT("/Game/Characters/Hero/UE5/SK_Hero.SK_Hero"));
    GetMesh()->SetSkeletalMesh(HeroMesh.Object);
    // New Tripo UE skeleton export: display at 180 cm, rotate +Y forward into character +X.
    GetMesh()->SetRelativeScale3D(FVector(180.f / 97.869893f));
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetAnimInstanceClass(UHearthwardHeroAnimInstance::StaticClass());

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Axe(TEXT("/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.SM_stone_bone_axe"));
    HeldAxe=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldAxe"));
    HeldAxe->SetupAttachment(GetMesh(),TEXT("hand_r"));
    HeldAxe->SetStaticMesh(Axe.Object);
    HeldAxe->SetRelativeRotation(FRotator(0,0,-90));
    HeldAxe->SetAbsolute(false,false,true);
    HeldAxe->SetRelativeScale3D(FVector(.7));
    HeldAxe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeldAxe->SetVisibility(false);
}

void AHearthwardCharacter::BeginPlay()
{
    Super::BeginPlay();
    FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this,&AHearthwardCharacter::ResetHeldInput);
    GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.AddDynamic(this,&AHearthwardCharacter::ResetHeldInput);
    int32 Sensitivity=5;
    bool InvertY=false;
    GConfig->GetInt(TEXT("Hearthward.Controls"),TEXT("LookSensitivity"),Sensitivity,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("Hearthward.Controls"),TEXT("InvertLookY"),InvertY,GGameUserSettingsIni);
    SetLookSettings(Sensitivity,InvertY);
    Inventory->OnInventoryChanged.AddDynamic(this, &AHearthwardCharacter::UpdateCarrySpeed);
    Inventory->OnInventoryChanged.AddDynamic(this, &AHearthwardCharacter::RefreshHeldTool);
    Gameplay->OnChanged.AddDynamic(this, &AHearthwardCharacter::RefreshHeldTool);
    GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.AddDynamic(this, &AHearthwardCharacter::RefreshHeldTool);
    UpdateCarrySpeed();
    RefreshHeldTool();
    if(HasAuthority() && UGameplayStatics::GetCurrentLevelName(GetWorld(),true)==TEXT("L_HearthwardWilds"))
    {
        // This level can run under its own map GameMode in PIE, or HearthwardGameMode from the menu.
        // Attach the task-owned camp structure to the player bootstrap in both cases.
        bool AlreadySpawned=false;
        for(TActorIterator<AHearthwardTask028CampHouse> It(GetWorld());It;++It) { AlreadySpawned=true; break; }
        if(!AlreadySpawned)
        {
            const FVector CampHouseXY(-98600,-75500,0);
            FHitResult Hit;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(Task028CampHouse),false);
            if(GetWorld()->LineTraceSingleByChannel(Hit,CampHouseXY+FVector(0,0,30000),
                                                    CampHouseXY-FVector(0,0,30000),ECC_Visibility,Query))
                GetWorld()->SpawnActor<AHearthwardTask028CampHouse>(Hit.ImpactPoint,FRotator::ZeroRotator);
            else UE_LOG(LogTemp,Error,TEXT("TASK-028 camp house ground trace failed"));
        }
    }
}

void AHearthwardCharacter::UpdateCarrySpeed()
{
    GetCharacterMovement()->MaxWalkSpeed = 350.0f * Inventory->GetMoveSpeedMultiplier();
}

void AHearthwardCharacter::RefreshHeldTool()
{
    // The existing equipment and inventory state remain the sole source of truth.
    const bool HoldingAxe=Gameplay->Equipment.FindRef(TEXT("weapon"))==TEXT("axe")
        && Inventory->GetItemCount(TEXT("axe"))>0;
    HeldAxe->SetVisibility(HoldingAxe);
}

void AHearthwardCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);RebuildInputBindings();
    auto* Player=CastChecked<APlayerController>(GetController());
    if(auto* HUD=Cast<AHearthwardHUD>(Player->GetHUD());HUD && HUD->Screen)HUD->Screen->ApplyInputMode();
    else {Player->SetInputMode(FInputModeGameOnly());Player->bShowMouseCursor=false;}
}
void AHearthwardCharacter::RebuildInputBindings()
{
    if(!InputComponent || !GetController()) return;
    ResetHeldInput();
    auto* Input=CastChecked<UEnhancedInputComponent>(InputComponent);
    auto* Player=CastChecked<APlayerController>(GetController());
    auto* Subsystem=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer());
    if(InputMapping) Subsystem->RemoveMappingContext(InputMapping);
    Input->ClearBindingsForObject(this);
    InputMapping=NewObject<UInputMappingContext>(this);
    const auto& Bindings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings;
    TMap<FKey,UInputAction*> Physical;
    auto PhysicalAction=[&](FKey Key)
    {
        if(auto* Existing=Physical.FindRef(Key)) return Existing;
        auto* A=NewObject<UInputAction>(this);A->ValueType=EInputActionValueType::Boolean;A->bConsumeInput=false;
        InputMapping->MapKey(A,Key);Physical.Add(Key,A);
        Input->BindAction(A,ETriggerEvent::Started,this,&AHearthwardCharacter::KeyPressed,Key);
        Input->BindAction(A,ETriggerEvent::Completed,this,&AHearthwardCharacter::KeyReleased,Key);
        Input->BindAction(A,ETriggerEvent::Canceled,this,&AHearthwardCharacter::KeyReleased,Key);
        return A;
    };
    for(const auto& Pair:Bindings) for(const auto& B:Pair.Value)
    { if(B.Key.IsValid()) PhysicalAction(B.Key);if(B.Modifier.IsValid()) PhysicalAction(B.Modifier); }
    MoveAction=NewObject<UInputAction>(this);MoveAction->ValueType=EInputActionValueType::Axis2D;
    MoveAction->bConsumeInput=false;MoveAction->AccumulationBehavior=EInputActionAccumulationBehavior::Cumulative;
    auto MapMove=[&](const TCHAR* Id,bool Negative,bool Y)
    {
        for(const auto& B:Bindings.FindChecked(FName(Id))) if(B.Key.IsValid())
        {
            auto& Mapping=InputMapping->MapKey(MoveAction,B.Key);
            if(Negative) Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(InputMapping));
            if(Y) {auto* Swizzle=NewObject<UInputModifierSwizzleAxis>(InputMapping);Swizzle->Order=EInputAxisSwizzle::YXZ;Mapping.Modifiers.Add(Swizzle);}
            if(B.Modifier.IsValid()) {auto* Chord=NewObject<UInputTriggerChordAction>(InputMapping);Chord->ChordAction=Physical.FindChecked(B.Modifier);Mapping.Triggers.Add(Chord);}
        }
    };
    MapMove(TEXT("move.right"),false,false);MapMove(TEXT("move.left"),true,false);
    MapMove(TEXT("move.forward"),false,true);MapMove(TEXT("move.back"),true,true);
    LookAction=NewObject<UInputAction>(this);LookAction->ValueType=EInputActionValueType::Axis2D;
    auto* Invert=NewObject<UInputModifierNegate>(InputMapping);Invert->bX=false;Invert->bY=true;Invert->bZ=false;
    InputMapping->MapKey(LookAction,EKeys::Mouse2D).Modifiers.Add(Invert);
    Input->BindAction(MoveAction,ETriggerEvent::Triggered,this,&AHearthwardCharacter::Move);
    Input->BindAction(LookAction,ETriggerEvent::Triggered,this,&AHearthwardCharacter::Look);
    FModifyContextOptions Options;Options.bIgnoreAllPressedKeysUntilRelease=true;
    Subsystem->AddMappingContext(InputMapping,0,Options);
}
void AHearthwardCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->ApplyTo(this);
}
bool AHearthwardCharacter::SemanticHeld(FName Id) const
{ return HearthwardInput::Held(GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Bindings,Id,Cast<APlayerController>(GetController())); }
void AHearthwardCharacter::ResetHeldInput()
{
    ActiveKeys.Reset();SprintLatched=false;Gameplay->SetSprinting(false);StopJumping();
    if(auto* C=FindComponentByClass<UHearthwardCombatComponent>()) {C->Aim(false);C->SetGuard(false);}
    GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->HoldLine(false);
    if(auto* Player=Cast<APlayerController>(GetController())) Player->FlushPressedKeys();
}
void AHearthwardCharacter::KeyPressed(FKey Key)
{
    auto* Player=CastChecked<APlayerController>(GetController());auto* HUD=Cast<AHearthwardHUD>(Player->GetHUD());
    if(HUD && HUD->Screen && HUD->Screen->GetPage()!=TEXT("hud")) return;
    ActiveKeys.Add(Key);
    const auto* Settings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
    const bool Shift=Player->IsInputKeyDown(EKeys::LeftShift)||Player->IsInputKeyDown(EKeys::RightShift);
    const bool Control=Player->IsInputKeyDown(EKeys::LeftControl)||Player->IsInputKeyDown(EKeys::RightControl);
    const bool Alt=Player->IsInputKeyDown(EKeys::LeftAlt)||Player->IsInputKeyDown(EKeys::RightAlt);
    int32 Specificity=0;
    for(const auto& D:HearthwardInput::Definitions()) if(D.Contexts.Contains(TEXT("hud")))
        for(const auto& B:Settings->Bindings.FindChecked(D.Id)) if(B.Modifier.IsValid() && B.Matches(Key,Shift,Control,Alt)) Specificity=1;
    auto Is=[&](const TCHAR* Id)
    {return Settings->Bindings.FindChecked(FName(Id)).ContainsByPredicate([&](const auto& B){return B.Matches(Key,Shift,Control,Alt) && (B.Modifier.IsValid()?1:0)==Specificity;});};
    auto Screen=[&](const TCHAR* Action){if(HUD && HUD->Screen) HUD->Screen->ExecuteAction(Action);};
    if(Is(TEXT("ui.pause"))) {if(HUD) HUD->OpenPause();return;}
    if(Is(TEXT("ui.save"))) {if(HUD) HUD->ToggleSaveMenu();return;}
    if(Is(TEXT("ui.inventory"))) {ToggleInventory();return;}
    if(Is(TEXT("ui.map"))) {Screen(TEXT("page:map"));return;}
    if(Is(TEXT("ui.skills"))) {Screen(TEXT("page:skills"));return;}
    if(Is(TEXT("ui.journal"))) {Screen(TEXT("page:journal"));return;}
    if(Is(TEXT("companion.dialogue"))) {if(HUD) HUD->ToggleDialogue();return;}
    if(Gameplay->Enabled && !FindComponentByClass<UHearthwardSurvivalComponent>()->Alive()) return;
    auto* N=GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>();
    if(N->Busy()) {if(Is(TEXT("combat.attack"))) N->HoldLine(true);return;}
    if(Is(TEXT("building.catalogue"))) {Screen(TEXT("page:building"));return;}
    if(Is(TEXT("building.rotate"))) {if(auto* B=FindComponentByClass<UHearthwardBuildingComponent>();B->IsPlacing()) B->RotatePreview();return;}
    if(Is(TEXT("combat.attack"))) {PreviewAttack();return;}
    if(Is(TEXT("combat.guardAim"))) {GuardStart();return;}
    if(Is(TEXT("sprint"))) {StartSprint();return;}
    if(Is(TEXT("jump"))) {StartJump();return;}
    if(Is(TEXT("interact"))) {Interact(Is(TEXT("traversal.vault")));return;}
    if(Is(TEXT("traversal.vault"))) {FindComponentByClass<UHearthwardTraversalComponent>()->BeginVault();return;}
    if(Is(TEXT("combat.throw"))) {CombatThrow();return;}
    if(Is(TEXT("survival.medicine"))) {Screen(TEXT("quick:0"));return;}
    if(Is(TEXT("survival.food"))) {Screen(TEXT("quick:1"));return;}
    if(Is(TEXT("combat.ammunition"))) {Screen(TEXT("quick:2"));return;}
    auto* C=FindComponentByClass<UHearthwardCombatComponent>();
    if(FindComponentByClass<UHearthwardTraversalComponent>()->IsInWater()) return;
    if(Is(TEXT("combat.execute"))) {Execution();return;}
    if(Is(TEXT("combat.stun")) && C->CanExecute()) {C->Stun();return;}
    if(Is(TEXT("storage.open")))
        for(TActorIterator<AActor> It(GetWorld());It;++It)
            if(const auto* R=It->FindComponentByClass<UHearthwardResourceInteractionComponent>();R && R->CanAccessStorage(this)) {Screen(TEXT("page:storage"));return;}
    if(Is(TEXT("combat.reload"))) {C->Reload();return;}
    if(Is(TEXT("combat.dodge"))) {CombatDodge();return;}
    if(Is(TEXT("combat.lock"))) {CombatLock();return;}
    if(Is(TEXT("combat.sense"))) {CombatSense();return;}
    if(Is(TEXT("companion.wait"))) {Gameplay->OrderCompanion(TEXT("wait"));return;}
    if(Is(TEXT("companion.follow"))) {Gameplay->OrderCompanion(TEXT("follow"));return;}
    if(Is(TEXT("companion.attack"))) {Gameplay->OrderCompanion(TEXT("attack"));return;}
}
void AHearthwardCharacter::KeyReleased(FKey Key)
{
    if(!ActiveKeys.Remove(Key)) return;
    const auto* Settings=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>();
    auto Uses=[&](const TCHAR* Id){return Settings->Bindings.FindChecked(FName(Id)).ContainsByPredicate([&](const auto& B){return B.Key==Key;});};
    if(Uses(TEXT("sprint")) && !Settings->Comfort.SprintToggle) StopSprint();
    if(Uses(TEXT("jump"))) StopJumping();
    if(Uses(TEXT("combat.attack"))) {GetWorld()->GetSubsystem<UHearthwardNatureSubsystem>()->HoldLine(false);ReleaseAttack();}
    if(Uses(TEXT("combat.guardAim"))) GuardEnd();
}

void AHearthwardCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveAll(this);
    GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.RemoveDynamic(this,&AHearthwardCharacter::ResetHeldInput);
    Inventory->OnInventoryChanged.RemoveDynamic(this, &AHearthwardCharacter::UpdateCarrySpeed);
    Inventory->OnInventoryChanged.RemoveDynamic(this, &AHearthwardCharacter::RefreshHeldTool);
    Gameplay->OnChanged.RemoveDynamic(this, &AHearthwardCharacter::RefreshHeldTool);
    GetWorld()->GetSubsystem<UHearthwardSaveSubsystem>()->OnSnapshotRestored.RemoveDynamic(this, &AHearthwardCharacter::RefreshHeldTool);
    if (auto* Player = Cast<APlayerController>(GetController()))
    {
        if (auto* LocalPlayer = Player->GetLocalPlayer())
        {
            LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->RemoveMappingContext(InputMapping);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AHearthwardCharacter::Move(const FInputActionValue& Value)
{
    if(Gameplay->Enabled && Gameplay->Health<=0) return;
    if(auto* T=FindComponentByClass<UHearthwardTraversalComponent>();T->IsVaulting()) {T->CancelVault();return;}
    auto* Combat=FindComponentByClass<UHearthwardCombatComponent>();
    const FVector2D Axis = Value.Get<FVector2D>();
    if(!Axis.IsNearlyZero() && Combat->Executing()) Combat->Cancel();
    if(Combat->MovementLocked()) return;
    if (!Axis.IsNearlyZero()) { TimedAction->InterruptAction(); FindComponentByClass<UHearthwardSurvivalComponent>()->CancelAction(); }
    const FRotator Yaw(0.0f, GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void AHearthwardCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    AddControllerYawInput(Axis.X*LookSensitivity);
    AddControllerPitchInput(Axis.Y*LookSensitivity*(bInvertLookY?-1.f:1.f));
}

void AHearthwardCharacter::SetLookSettings(int32 Sensitivity,bool InvertY)
{
    LookSensitivity=FMath::Clamp(Sensitivity,1,10)/5.f;
    bInvertLookY=InvertY;
}

void AHearthwardCharacter::StartJump()
{
    if ((Gameplay->Enabled && Gameplay->Health <= 0) || !CanJump()) return;
    auto* Combat=FindComponentByClass<UHearthwardCombatComponent>();
    if(Combat->Executing()) Combat->Cancel();
    if(Combat->Busy() || Combat->MovementMultiplier()<1) return;
    if(Gameplay->Enabled && !Gameplay->SpendStamina(5)) return;
    TimedAction->InterruptAction();
    FindComponentByClass<UHearthwardSurvivalComponent>()->CancelAction();
    Jump();
}

void AHearthwardCharacter::StartSprint() {
    const bool Toggle=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.SprintToggle;
    SprintLatched=Toggle?!SprintLatched:true;Gameplay->SetSprinting(SprintLatched);
}
void AHearthwardCharacter::StopSprint() { Gameplay->SetSprinting(false); }

void AHearthwardCharacter::PreviewAttack()
{
    if(FindComponentByClass<UHearthwardTraversalComponent>()->IsInWater()) return;
    if(auto* B=FindComponentByClass<UHearthwardBuildingComponent>();B && B->IsPlacing()) { B->ConfirmPlacement(); return; }
    if (!Gameplay->Enabled)
    {
        if (auto* Animation = Cast<UHearthwardHeroAnimInstance>(GetMesh()->GetAnimInstance()))
            Animation->PlayAttack();
        return;
    }
    auto* Combat=FindComponentByClass<UHearthwardCombatComponent>();
    if(Combat->Executing()) { Combat->Cancel(); return; }
    auto* PC=Cast<APlayerController>(GetController());
    const auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;
    const int32 Slot=HUD && HUD->Screen?HUD->Screen->GetHUDQuickSelection():0;
    if(Slot==3) { Gameplay->UseQuickItem(3); return; }
    if(Slot==2 && Combat->SupportsAmmo(Gameplay->QuickItem(2)))
    {
        Combat->Aim(true); Combat->Shoot(false); return;
    }
    Combat->Aim(false);
    Combat->Attack(SemanticHeld(TEXT("combat.heavyModifier")));
}

void AHearthwardCharacter::ToggleInventory()
{
    if (auto* Player = Cast<APlayerController>(GetController()))
    {
        if (auto* HUD = Cast<AHearthwardHUD>(Player->GetHUD())) HUD->ToggleInventory();
    }
}

void AHearthwardCharacter::Interact(bool AllowVault)
{
    auto* Survival=FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!Survival->Alive()) return;
    for(TActorIterator<AHearthwardCompanionFixture> It(GetWorld());It;++It)
        if(Survival->BeginRescue(It->FindComponentByClass<UHearthwardSurvivalComponent>())) return;
    auto* Combat=FindComponentByClass<UHearthwardCombatComponent>();
    if(Combat->Busy()) return;
    if(Combat->Carry(SemanticHeld(TEXT("combat.heavyModifier")))) return;
    TArray<AHearthwardProjectile*> Arrows;
    for(TActorIterator<AHearthwardProjectile> It(GetWorld());It;++It)
        if(It->Landed && !It->HitTarget && !It->Item.IsNone() && FVector::DistSquared(It->GetActorLocation(),GetActorLocation())<=FMath::Square(150.f)) Arrows.Add(*It);
    Arrows.Sort([&](const auto& A,const auto& B)
    {const double DA=FVector::DistSquared(A.GetActorLocation(),GetActorLocation()),DB=FVector::DistSquared(B.GetActorLocation(),GetActorLocation());return DA!=DB?DA<DB:A.GetPathName()<B.GetPathName();});
    if(!Arrows.IsEmpty())
    {if(!Arrows[0]->Recover(this)) {Gameplay->SetFeedback(TEXT("背包容量不足，无法回收箭"));Gameplay->OnChanged.Broadcast();}return;}
    Survival->CancelAction();
    if(FindComponentByClass<UHearthwardBuildingComponent>()->IsPlacing()) return;
    if(FindComponentByClass<UHearthwardBuildingComponent>()->NearbyWorkbench().IsValid())
    {
        if(auto* PC=Cast<APlayerController>(GetController()))
            if(auto* HUD=Cast<AHearthwardHUD>(PC->GetHUD());HUD && HUD->Screen) HUD->Screen->ExecuteAction(TEXT("page:crafting"));
        return;
    }
    if(GetWorld()->GetSubsystem<UHearthwardCampaignSubsystem>()->Interact())return;
    if (Gameplay->Enabled && Gameplay->ActivateNearby()) return;
    if(Interaction->GetNearestTarget()) {Interaction->InteractNearest();return;}
    if(AllowVault) FindComponentByClass<UHearthwardTraversalComponent>()->BeginVault();
}

void AHearthwardCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
    auto* S=FindComponentByClass<UHearthwardSurvivalComponent>();
    if(!S || !S->Enabled()) { Super::FellOutOfWorld(DamageType); return; }
    S->FatalEnvironment();
}
void AHearthwardCharacter::Landed(const FHitResult& Hit)
{
    const float Speed=FMath::Max(0.f,-GetVelocity().Z);
    Super::Landed(Hit);
    if(auto* S=FindComponentByClass<UHearthwardSurvivalComponent>();S && !FindComponentByClass<UHearthwardTraversalComponent>()->IsInWater()) S->FallImpact(Speed);
}

void AHearthwardCharacter::Execution() { FindComponentByClass<UHearthwardCombatComponent>()->Execute(); }
void AHearthwardCharacter::ContextR()
{
    auto* C=FindComponentByClass<UHearthwardCombatComponent>();
    if(C->CanExecute()) {C->Stun();return;}
    for(TActorIterator<AActor> It(GetWorld());It;++It)
        if(const auto* Resource=It->FindComponentByClass<UHearthwardResourceInteractionComponent>();Resource && Resource->CanAccessStorage(this))
        {
            if(auto* PC=Cast<APlayerController>(GetController())) if(auto* HUD=Cast<AHearthwardHUD>(PC->GetHUD());HUD && HUD->Screen) HUD->Screen->ExecuteAction(TEXT("page:storage"));
            return;
        }
    C->Reload();
}
void AHearthwardCharacter::GuardStart()
{
    if(FindComponentByClass<UHearthwardTraversalComponent>()->IsInWater()) return;
    if(auto* B=FindComponentByClass<UHearthwardBuildingComponent>();B && B->IsPlacing()) { B->CancelPlacement(); return; }
    auto* C=FindComponentByClass<UHearthwardCombatComponent>();
    const auto& Comfort=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort;
    const auto* PC=Cast<APlayerController>(GetController()); const auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;
    if(HUD && HUD->Screen && HUD->Screen->GetHUDQuickSelection()==2 && C->SupportsAmmo(Gameplay->QuickItem(2)))
        C->Aim(Comfort.AimToggle?!C->Aiming:true);
    else {C->Aim(false);C->SetGuard(Comfort.GuardToggle?!C->Guarding():true);}
}
void AHearthwardCharacter::GuardEnd() {
    auto* C=FindComponentByClass<UHearthwardCombatComponent>();
    const auto& Comfort=GetGameInstance()->GetSubsystem<UHearthwardPlayerSettings>()->Comfort;
    if(!Comfort.AimToggle) C->Aim(false);if(!Comfort.GuardToggle) C->SetGuard(false);
}
void AHearthwardCharacter::ReleaseAttack()
{
    auto* C=FindComponentByClass<UHearthwardCombatComponent>();
    const auto* PC=Cast<APlayerController>(GetController()); const auto* HUD=PC?Cast<AHearthwardHUD>(PC->GetHUD()):nullptr;
    if(HUD && HUD->Screen && HUD->Screen->GetHUDQuickSelection()==2 && C->Aiming && C->SupportsAmmo(Gameplay->QuickItem(2)) && !FindComponentByClass<UHearthwardTraversalComponent>()->IsInWater()) C->Shoot(true);
}
void AHearthwardCharacter::CombatDodge() { FindComponentByClass<UHearthwardCombatComponent>()->Dodge(GetLastMovementInputVector()); }
void AHearthwardCharacter::CombatLock() { FindComponentByClass<UHearthwardCombatComponent>()->ToggleLock(); }
void AHearthwardCharacter::CombatSense() { FindComponentByClass<UHearthwardCombatComponent>()->Sense(); }
void AHearthwardCharacter::CombatThrow() {
    if(auto* PC=Cast<APlayerController>(GetController())) if(auto* HUD=Cast<AHearthwardHUD>(PC->GetHUD());HUD && HUD->Screen) HUD->Screen->ExecuteAction(TEXT("quick:3"));
}
