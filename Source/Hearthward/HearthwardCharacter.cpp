#include "HearthwardCharacter.h"
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

AHearthwardCharacter::AHearthwardCharacter()
{
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

    auto* Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    Boom->SetupAttachment(GetRootComponent());
    Boom->TargetArmLength = 400.0f;
    Boom->TargetOffset = FVector(0.0f, 0.0f, 60.0f);
    Boom->bUsePawnControlRotation = true;
    auto* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;

    // Engine primitives are a greybox silhouette, not final character art.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GreyboxBody"));
    Body->SetupAttachment(GetRootComponent());
    Body->SetStaticMesh(Cylinder.Object);
    Body->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
    Body->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.4f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GreyboxHead"));
    Head->SetupAttachment(GetRootComponent());
    Head->SetStaticMesh(Sphere.Object);
    Head->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
    Head->SetRelativeScale3D(FVector(0.5f));
    Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Facing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
    Facing->SetupAttachment(GetRootComponent());
    Facing->SetStaticMesh(Cube.Object);
    Facing->SetRelativeLocation(FVector(27.0f, 0.0f, 65.0f));
    Facing->SetRelativeScale3D(FVector(0.18f, 0.12f, 0.12f));
    Facing->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // TASK-028: a provisional greybox hand position until TASK-027 publishes its final socket.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Axe(TEXT("/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.SM_stone_bone_axe"));
    HeldAxe=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldAxe"));
    HeldAxe->SetupAttachment(GetRootComponent());
    HeldAxe->SetStaticMesh(Axe.Object);
    HeldAxe->SetRelativeLocation(FVector(30,35,25));
    HeldAxe->SetRelativeScale3D(FVector(.7));
    HeldAxe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeldAxe->SetVisibility(false);
}

void AHearthwardCharacter::BeginPlay()
{
    Super::BeginPlay();
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
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    auto* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    auto* Player = CastChecked<APlayerController>(GetController());
    auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer());

    MoveAction = NewObject<UInputAction>(this);
    MoveAction->ValueType = EInputActionValueType::Axis2D;
    MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
    LookAction = NewObject<UInputAction>(this);
    LookAction->ValueType = EInputActionValueType::Axis2D;
    InputMapping = NewObject<UInputMappingContext>(this);
    InventoryAction = NewObject<UInputAction>(this, TEXT("InventoryToggleAction"));
    InventoryAction->ValueType = EInputActionValueType::Boolean;
    InventoryAction->bTriggerWhenPaused = true;
    // Temporary greybox binding; GDD R23 does not define the final inventory key.
    InputMapping->MapKey(InventoryAction, EKeys::Tab);
    // Temporary greybox binding; final interaction keys remain GDD R23.
    InteractAction = NewObject<UInputAction>(this, TEXT("InteractAction"));
    InteractAction->ValueType = EInputActionValueType::Boolean;
    InputMapping->MapKey(InteractAction, EKeys::E);
    JumpAction = NewObject<UInputAction>(this, TEXT("JumpAction"));
    JumpAction->ValueType = EInputActionValueType::Boolean;
    InputMapping->MapKey(JumpAction, EKeys::SpaceBar);

    auto* Negate = NewObject<UInputModifierNegate>(InputMapping);
    auto* Swizzle = NewObject<UInputModifierSwizzleAxis>(InputMapping);
    Swizzle->Order = EInputAxisSwizzle::YXZ;
    InputMapping->MapKey(MoveAction, EKeys::D);
    InputMapping->MapKey(MoveAction, EKeys::A).Modifiers.Add(Negate);
    InputMapping->MapKey(MoveAction, EKeys::W).Modifiers.Add(Swizzle);
    auto& Backward = InputMapping->MapKey(MoveAction, EKeys::S);
    Backward.Modifiers.Add(Negate);
    Backward.Modifiers.Add(Swizzle);
    auto* InvertPitch = NewObject<UInputModifierNegate>(InputMapping);
    InvertPitch->bX = false;
    InvertPitch->bY = true;
    InvertPitch->bZ = false;
    InputMapping->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(InvertPitch);

    Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHearthwardCharacter::Move);
    Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHearthwardCharacter::Look);
    Input->BindAction(InventoryAction, ETriggerEvent::Started, this, &AHearthwardCharacter::ToggleInventory);
    Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AHearthwardCharacter::Interact);
    Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AHearthwardCharacter::StartJump);
    Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
    Input->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ACharacter::StopJumping);
    Subsystem->AddMappingContext(InputMapping, 0);
    Player->SetInputMode(FInputModeGameOnly());
    Player->bShowMouseCursor = false;
}

void AHearthwardCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
    const FVector2D Axis = Value.Get<FVector2D>();
    if (!Axis.IsNearlyZero()) TimedAction->InterruptAction();
    const FRotator Yaw(0.0f, GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void AHearthwardCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
}

void AHearthwardCharacter::StartJump()
{
    if ((Gameplay->Enabled && Gameplay->Health <= 0) || !CanJump()) return;
    TimedAction->InterruptAction();
    Jump();
}

void AHearthwardCharacter::ToggleInventory()
{
    if (auto* Player = Cast<APlayerController>(GetController()))
    {
        if (auto* HUD = Cast<AHearthwardHUD>(Player->GetHUD())) HUD->ToggleInventory();
    }
}

void AHearthwardCharacter::Interact()
{
    if(FindComponentByClass<UHearthwardBuildingComponent>()->IsPlacing()) return;
    if(FindComponentByClass<UHearthwardBuildingComponent>()->NearbyWorkbench().IsValid())
    {
        if(auto* PC=Cast<APlayerController>(GetController()))
            if(auto* HUD=Cast<AHearthwardHUD>(PC->GetHUD());HUD && HUD->Screen) HUD->Screen->ExecuteAction(TEXT("page:crafting"));
        return;
    }
    if (Gameplay->Enabled && Gameplay->ActivateNearby()) return;
    Interaction->InteractNearest();
}
