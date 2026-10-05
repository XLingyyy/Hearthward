#include "../Input/HearthwardInputBindings.h"
#include "../Experience/HearthwardTraversalComponent.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Survival/HearthwardSurvivalComponent.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "../Combat/HearthwardCombatComponent.h"
#include "../Building/HearthwardBuildingComponent.h"
#include "../UI/HearthwardScreenWidget.h"
#include "../UI/HearthwardLoadingSubsystem.h"
#include "../HearthwardCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Widgets/SViewport.h"
#include "../Gameplay/HearthwardGameData.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExperienceBindingsTest,"Hearthward.Experience.BindingConflictsAndContexts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExperienceBindingsTest::RunTest(const FString&)
{
    auto B=HearthwardInput::Defaults();
    TestTrue(TEXT("Defaults preserve shared R and E priorities"),HearthwardInput::Validate(B).IsEmpty());
    B[TEXT("storage.open")][0]={EKeys::Y,FKey()};TestTrue(TEXT("R actions can split"),HearthwardInput::Validate(B).IsEmpty());
    TestTrue(TEXT("Changed binding affects semantic lookup"),HearthwardInput::Matches(B,TEXT("storage.open"),EKeys::Y,false,false,false));
    TestFalse(TEXT("Old physical key is no longer storage"),HearthwardInput::Matches(B,TEXT("storage.open"),EKeys::R,false,false,false));
    B[TEXT("interact")][0]={EKeys::W,FKey()};TestFalse(TEXT("Simultaneous world conflict is rejected"),HearthwardInput::Validate(B).IsEmpty());
    B=HearthwardInput::Defaults();B[TEXT("ui.inventory")][0]={EKeys::F,FKey()};TestFalse(TEXT("UI global shortcut cannot mask inventory use"),HearthwardInput::Validate(B).IsEmpty());
    B=HearthwardInput::Defaults();B[TEXT("move.forward")][0]={};TestFalse(TEXT("Movement entry must survive"),HearthwardInput::Validate(B).IsEmpty());
    B=HearthwardInput::Defaults();B[TEXT("combat.throw")][0]={EKeys::Enter,FKey()};TestFalse(TEXT("Text submit key reserved"),HearthwardInput::Validate(B).IsEmpty());
    B[TEXT("combat.throw")][0]={EKeys::Y,EKeys::LeftControl};TestTrue(TEXT("A one-modifier chord is valid"),HearthwardInput::Validate(B).IsEmpty());
    TestFalse(TEXT("Chord cannot trigger from bare key"),HearthwardInput::Matches(B,TEXT("combat.throw"),EKeys::Y,false,false,false));
    TestTrue(TEXT("Chord triggers from actual modifier"),HearthwardInput::Matches(B,TEXT("combat.throw"),EKeys::Y,false,true,false));
    auto Conflict=HearthwardInput::Defaults();Conflict[TEXT("combat.throw")][0]={EKeys::W,EKeys::LeftControl};
    TestFalse(TEXT("Chord cannot also move through its bare key"),HearthwardInput::Validate(Conflict).IsEmpty());
    const auto Encoded=FHearthwardKeyBinding::Decode(TEXT("LeftControl+Y"));TestTrue(TEXT("Binding round trip retains chord"),Encoded==B[TEXT("combat.throw")][0]);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExperienceFallTest,"Hearthward.Experience.FallHeightAndMaximumHealth",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExperienceFallTest::RunTest(const FString&)
{
    auto Damage=[](float Height,float Gravity,float Health){return UHearthwardTraversalComponent::FallDamage(FMath::Sqrt(2*FMath::Abs(Gravity)*Height*100),Gravity,Health);};
    TestEqual(TEXT("Stationary landing costs nothing"),UHearthwardTraversalComponent::FallDamage(0,-980,100),0.f);
    TestTrue(TEXT("Three metres remains harmless"),Damage(3,-980,100)<.0001);
    TestTrue(TEXT("7.5 metres costs half maximum health"),FMath::IsNearlyEqual(Damage(7.5,-980,180),90.f,.001f));
    TestTrue(TEXT("Uses actual gravity"),FMath::IsNearlyEqual(Damage(7.5,-490,180),90.f,.001f));
    TestTrue(TEXT("Twelve metres caps at maximum health"),FMath::IsNearlyEqual(Damage(12,-980,180),180.f,.001f));
    TestEqual(TEXT("Longer falls still cap"),Damage(30,-980,180),180.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExperienceSwimTest,"Hearthward.Experience.SwimmingFloatExhaustionAndGroundFall",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExperienceSwimTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* C=World->SpawnActor<ACharacter>();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(C);C->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* G=NewObject<UHearthwardGameplayComponent>(C);C->AddInstanceComponent(G);G->RegisterComponent();G->Enabled=true;
    auto* S=NewObject<UHearthwardSurvivalComponent>(C);C->AddInstanceComponent(S);S->RegisterComponent();
    C->GetCharacterMovement()->SetMovementMode(MOVE_Swimming);G->Stamina=G->MaxStamina();
    S->AdvanceContinuous(3,0,0);TestEqual(TEXT("Positive floating neither costs nor regenerates"),G->Stamina,G->MaxStamina());
    C->GetCharacterMovement()->Velocity={300,0,0};S->AdvanceContinuous(25,0,0);
    TestEqual(TEXT("25 active seconds exhaust unloaded swimmer"),G->Stamina,0.f);
    TestTrue(TEXT("Drowning starts at exhaustion boundary"),FMath::IsNearlyEqual(S->State.DrowningRemaining,10.));
    C->GetCharacterMovement()->SetMovementMode(MOVE_Falling);S->AdvanceContinuous(9,0,0);
    TestTrue(TEXT("Brief surfacing does not reset drowning"),S->Alive() && FMath::IsNearlyEqual(S->State.DrowningRemaining,1.));
    S->AdvanceContinuous(1,0,0);TestTrue(TEXT("Drowning at ten seconds is true death"),S->State.Life==EHearthwardLife::Dead);
    S->State={};G->Health=G->MaxHealth();C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    S->FallImpact(FMath::Sqrt(2.f*980.f*1200.f));TestTrue(TEXT("Ground fall to zero remains rescuable"),S->State.Life==EHearthwardLife::Downed);
    World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDangerousConfirmation069Test,
    "Hearthward.UI069.DangerousConfirmationNeedsExplicitFocus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDangerousConfirmation069Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();
    auto* World=Instance->GetWorld();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);
    const TSharedRef<SViewport> SlateViewport=SNew(SViewport);
    TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SlateViewport);
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);
    Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);
    World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);
    Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);
    Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);
    Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Combat=NewObject<UHearthwardCombatComponent>(Player);
    Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
    auto* Building=NewObject<UHearthwardBuildingComponent>(Player);
    Player->AddInstanceComponent(Building);Building->RegisterComponent();
    auto* Screen=CreateWidget<UHearthwardScreenWidget>(Controller);
    const auto Cleanup=[&]()
    {
        if(Screen)Screen->RemoveFromParent();
        SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    };
    if(!TestNotNull(TEXT("Real confirmation widget is created"),Screen)) {Cleanup();return false;}
    Screen->SetIsFocusable(true);Screen->TakeWidget();Screen->AddToViewport();
    const FGuid Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    if(!TestTrue(TEXT("Actual damage enters rescuable downed state"),
        Survival->ReceiveDamage(Survival->MaxHealth(),FGuid::NewGuid(),Epoch)
        && Survival->State.Life==EHearthwardLife::Downed)) {Cleanup();return false;}
    Screen->OpenPage(TEXT("pause"));
    if(!TestTrue(TEXT("Actual rescue abandonment opens confirmation"),Screen->ExecuteAction(TEXT("giveUp"))))
    {Cleanup();return false;}
    TestEqual(TEXT("Confirmation has an actual affirmative hit target"),Screen->ActionAt({650,537}),FString(TEXT("confirm")));
    const double Remaining=Survival->State.DownRemaining;
    const FKeyEvent Enter(EKeys::Enter,FModifierKeysState(),0,false,0,0);
    Screen->NativeOnPreviewKeyDown(FGeometry(),Enter);
    const bool StillRescuable=TestTrue(TEXT("Enter without explicit focus cannot abandon rescue"),Survival->State.Life==EHearthwardLife::Downed);
    TestEqual(TEXT("Unfocused Enter preserves the rescue deadline"),Survival->State.DownRemaining,Remaining);
    TestEqual(TEXT("Unfocused Enter leaves the dangerous confirmation pending"),Screen->ActionAt({650,537}),FString(TEXT("confirm")));
    if(!StillRescuable) {Cleanup();return false;}

    Screen->NativeOnPreviewKeyDown(FGeometry(),FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
    TestTrue(TEXT("Escape cancels confirmation without abandoning rescue"),Survival->State.Life==EHearthwardLife::Downed);
    TestNotEqual(TEXT("Escape removes the affirmative confirmation target"),Screen->ActionAt({650,537}),FString(TEXT("confirm")));
    if(!TestTrue(TEXT("Confirmation can be reopened"),Screen->ExecuteAction(TEXT("giveUp")))) {Cleanup();return false;}
    Screen->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::Down,FModifierKeysState(),0,false,0,0));
    Screen->NativeOnPreviewKeyDown(FGeometry(),Enter);
    TestTrue(TEXT("Explicit keyboard selection allows rescue abandonment"),Survival->State.Life==EHearthwardLife::Dead);
    TestEqual(TEXT("Accepted abandonment reaches the actual failure page"),Screen->GetPage(),FName(TEXT("save")));
    Cleanup();return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModifierSides069Test,
    "Hearthward.UI069.ModifierSidesAgreeForHeldAndMovementInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModifierSides069Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();
    auto* World=Instance->GetWorld();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);
    const TSharedRef<SViewport> SlateViewport=SNew(SViewport);
    TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SlateViewport);
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);
    Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);
    World->AddController(Controller);Controller->InitInputSystem();
    auto* Player=World->SpawnActor<AHearthwardCharacter>();Controller->Possess(Player);
    const auto Cleanup=[&]()
    {
        Player->ResetHeldInput();
        SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    };
    auto* Input=Cast<UEnhancedPlayerInput>(Controller->PlayerInput);
    auto* Subsystem=LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if(!TestNotNull(TEXT("Configured player input is Enhanced Input"),Input)
        || !TestNotNull(TEXT("Actual local Enhanced Input subsystem exists"),Subsystem)) {Cleanup();return false;}
    Player->Gameplay->Enabled=true;
    auto* Component=NewObject<UEnhancedInputComponent>(Player);
    Player->InputComponent=Component;Player->AddInstanceComponent(Component);Component->RegisterComponent();
    auto* Settings=Instance->GetSubsystem<UHearthwardPlayerSettings>();
    Settings->Bindings=HearthwardInput::Defaults();
    const FInputDeviceId Device=FInputDeviceId::CreateFromInternalId(0);
    const auto Send=[&](FKey Key,EInputEvent Event)
    {Controller->InputKey(FInputKeyEventArgs(SceneViewport.Get(),Device,Key,Event,Event==IE_Released?0.f:1.f,false,FPlatformTime::Cycles64()));};
    const TArray<UInputComponent*> InputStack{Component};
    const auto Process=[&]() {Input->Tick(1.f/60);Input->ProcessInputStack(InputStack,1.f/60,false);};
    struct FModifierPair {const TCHAR* Label;FKey Left,Right;};
    const FModifierPair Modifiers[]={{TEXT("Ctrl"),EKeys::LeftControl,EKeys::RightControl},{TEXT("Shift"),EKeys::LeftShift,EKeys::RightShift}};
    for(const auto& Modifier:Modifiers)
    {
        Settings->Bindings[TEXT("move.forward")][0]={EKeys::I,Modifier.Left};
        TestTrue(FString::Printf(TEXT("%s+I is a valid canonical binding"),Modifier.Label),HearthwardInput::Validate(Settings->Bindings).IsEmpty());
        Player->SetupPlayerInputComponent(Component);
        FModifyContextOptions Options;Options.bForceImmediately=true;
        Subsystem->RequestRebuildControlMappings(Options);
        Process();Player->ConsumeMovementInputVector();
        Send(EKeys::I,IE_Pressed);Process();
        TestTrue(FString::Printf(TEXT("Bare I cannot request %s-chord movement"),Modifier.Label),Player->ConsumeMovementInputVector().IsNearlyZero());
        TestFalse(TEXT("Bare I cannot satisfy the production Held lookup"),Player->SemanticHeld(TEXT("move.forward")));
        Send(EKeys::I,IE_Released);Process();Player->ConsumeMovementInputVector();

        FVector LeftMovement=FVector::ZeroVector;
        for(const FKey Side:{Modifier.Left,Modifier.Right})
        {
            const FString Label=Side.GetFName().ToString();
            Send(Side,IE_Pressed);Process();Player->ConsumeMovementInputVector();
            Send(EKeys::I,IE_Pressed);Process();
            TestTrue(Label+TEXT(" reaches actual controller key state"),Controller->IsInputKeyDown(Side) && Controller->IsInputKeyDown(EKeys::I));
            TestTrue(Label+TEXT(" satisfies production Matches"),HearthwardInput::Matches(Settings->Bindings,TEXT("move.forward"),EKeys::I,Input->IsShiftPressed(),Input->IsCtrlPressed(),Input->IsAltPressed()));
            TestTrue(Label+TEXT(" satisfies production Held"),HearthwardInput::Held(Settings->Bindings,TEXT("move.forward"),Controller));
            TestTrue(Label+TEXT(" satisfies Character::SemanticHeld"),Player->SemanticHeld(TEXT("move.forward")));
            const FVector Movement=Player->ConsumeMovementInputVector();
            TestTrue(Label+TEXT(" is consumed by the real Enhanced movement mapping"),Movement.X>.99 && FMath::Abs(Movement.Y)<.001 && FMath::Abs(Movement.Z)<.001);
            if(Side==Modifier.Left)LeftMovement=Movement;
            else TestTrue(Label+TEXT(" requests the same forward input as its left side"),Movement.Equals(LeftMovement,.001));
            Send(EKeys::I,IE_Released);Send(Side,IE_Released);Process();Player->ConsumeMovementInputVector();
            TestFalse(Label+TEXT(" release clears the production Held lookup"),Player->SemanticHeld(TEXT("move.forward")));
        }
        Send(Modifier.Left,IE_Pressed);Send(Modifier.Right,IE_Pressed);Process();Player->ConsumeMovementInputVector();
        Send(EKeys::I,IE_Pressed);Process();
        TestTrue(FString::Printf(TEXT("Both %s sides satisfy the production Held lookup"),Modifier.Label),Player->SemanticHeld(TEXT("move.forward")));
        TestTrue(FString::Printf(TEXT("Both %s sides preserve one forward movement request"),Modifier.Label),Player->ConsumeMovementInputVector().Equals(LeftMovement,.001));
        Send(Modifier.Left,IE_Released);Process();
        TestTrue(FString::Printf(TEXT("Releasing left %s preserves the held right-side chord"),Modifier.Label),Player->SemanticHeld(TEXT("move.forward")));
        TestTrue(FString::Printf(TEXT("Releasing left %s preserves Enhanced movement"),Modifier.Label),Player->ConsumeMovementInputVector().Equals(LeftMovement,.001));
        Send(Modifier.Right,IE_Released);Process();
        TestFalse(FString::Printf(TEXT("Releasing both %s sides clears the chord"),Modifier.Label),Player->SemanticHeld(TEXT("move.forward")));
        TestTrue(FString::Printf(TEXT("Releasing both %s sides stops Enhanced movement"),Modifier.Label),Player->ConsumeMovementInputVector().IsNearlyZero());
        Send(EKeys::I,IE_Released);Process();Player->ConsumeMovementInputVector();
    }
    Cleanup();return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPauseObjectiveMetrics069Test,
    "Hearthward.UI069.PauseAllObjectivesUseReadableNonOverlappingText",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPauseObjectiveMetrics069Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();
    auto* World=Instance->GetWorld();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);
    const TSharedRef<SViewport> SlateViewport=SNew(SViewport);
    TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SlateViewport);
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);
    Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);
    World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);
    Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);
    Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);
    Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Combat=NewObject<UHearthwardCombatComponent>(Player);
    Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
    auto* Building=NewObject<UHearthwardBuildingComponent>(Player);
    Player->AddInstanceComponent(Building);Building->RegisterComponent();
    Instance->GetSubsystem<UHearthwardPlayerSettings>()->Comfort.TextScale=100;
    auto* Screen=CreateWidget<UHearthwardScreenWidget>(Controller);
    const auto Cleanup=[&]()
    {
        if(Screen)Screen->RemoveFromParent();
        SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    };
    if(!TestNotNull(TEXT("Actual Pause widget exists"),Screen)) {Cleanup();return false;}
    Screen->SetIsFocusable(true);Screen->TakeWidget();Screen->AddToViewport();

    FString ThemeJson;TSharedPtr<FJsonObject> Theme;
    const FString UIRoot=FPaths::ProjectDir()/TEXT("Resources/UI");
    if(!TestTrue(TEXT("Actual theme typography loads"),FFileHelper::LoadFileToString(ThemeJson,*(UIRoot/TEXT("interface.json")))
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ThemeJson),Theme))) {Cleanup();return false;}
    const auto Typography=Theme->GetObjectField(TEXT("typography"));
    const auto BodyFont=MakeShared<FCompositeFont>(NAME_None,UIRoot/Typography->GetStringField(TEXT("body")),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
    const auto DisplayFont=MakeShared<FCompositeFont>(NAME_None,UIRoot/Typography->GetStringField(TEXT("display")),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
    const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    const auto ReadRect=[](const TSharedPtr<FJsonObject>& Row)
    {
        const auto& Values=Row->GetArrayField(TEXT("rect"));
        return FVector4(Values[0]->AsNumber(),Values[1]->AsNumber(),Values[2]->AsNumber(),Values[3]->AsNumber());
    };
    const auto& QuestRows=HearthwardData::Rows(TEXT("quests"));
    if(!TestTrue(TEXT("Actual quest catalog is nonempty"),!QuestRows.IsEmpty())) {Cleanup();return false;}
    int32 MaximumLines=0;
    for(int32 LifeCase=0;LifeCase<2;++LifeCase)
    {
        if(LifeCase==1)
        {
            const FGuid Epoch=World->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
            if(!TestTrue(TEXT("Actual lethal damage supplies the Downed Pause button"),
                Survival->ReceiveDamage(Survival->MaxHealth(),FGuid::NewGuid(),Epoch)
                && Survival->State.Life==EHearthwardLife::Downed)) {Cleanup();return false;}
        }
        for(const auto& QuestValue:QuestRows)
        {
            const auto Quest=QuestValue->AsObject();
            const FString QuestId=Quest->GetStringField(TEXT("id"));
            const FString Case=FString(LifeCase?TEXT("Downed "):TEXT("Alive "))+QuestId;
            Gameplay->TrackedQuest=FName(*QuestId);Screen->OpenPage(TEXT("pause"));
            TSharedPtr<FJsonObject> Description;
            if(!TestTrue(Case+TEXT(" actual layout parses"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Screen->DescribeLayout()),Description)))
            {Cleanup();return false;}
            const auto& Components=Description->GetArrayField(TEXT("components"));
            TSharedPtr<FJsonObject> Objective;
            for(const auto& Value:Components)
                if(HearthwardData::Text(Value->AsObject(),TEXT("bind"))==TEXT("objective")) {Objective=Value->AsObject();break;}
            if(!TestNotNull(Case+TEXT(" actual objective is present"),Objective.Get())) {Cleanup();return false;}
            TestTrue(Case+TEXT(" objective remains visible"),Objective->GetBoolField(TEXT("visible")));
            TestEqual(Case+TEXT(" objective retains the complete catalog text"),Objective->GetStringField(TEXT("text")),Quest->GetStringField(TEXT("objective")));
            const float FontSize=float(Objective->GetNumberField(TEXT("font")));
            TestTrue(Case+TEXT(" actual objective font meets 24px floor"),FontSize>=24.f);
            const FVector4 Bounds=ReadRect(Objective);
            const FString FontRole=Objective->GetStringField(TEXT("fontRole"));
            const bool Display=FontRole==TEXT("display") || (FontRole.IsEmpty() && FontSize>=30);
            FSlateFontInfo Font(Display?DisplayFont:BodyFont,FMath::RoundToInt(FontSize*.75f));
            Font.LetterSpacing=int32(Objective->GetNumberField(TEXT("tracking")));
            TArray<FString> SourceLines,Lines;Objective->GetStringField(TEXT("text")).ParseIntoArrayLines(SourceLines,false);
            // Match NativePaint's final FontInfo, letter spacing, and whole-character wrapping.
            for(FString Line:SourceLines)
            {
                while(Line.Len()>1 && Measure->Measure(Line,Font).X>Bounds.Z)
                {
                    const int32 Count=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,Bounds.Z)+1,1,Line.Len());
                    Lines.Add(Line.Left(Count));Line=Line.Mid(Count);
                }
                Lines.Add(Line);
            }
            MaximumLines=FMath::Max(MaximumLines,Lines.Num());
            float Width=0,Height=0;
            for(int32 LineIndex=0;LineIndex<Lines.Num();++LineIndex)
            {
                const FVector2D Extent=Measure->Measure(Lines[LineIndex],Font);
                Width=FMath::Max(Width,float(Extent.X));
                Height=FMath::Max(Height,float(LineIndex*FontSize*1.6f+Extent.Y));
            }
            TestTrue(Case+TEXT(" declared objective height contains every rendered line"),Height<=Bounds.W);
            const FBox2D TextBounds(FVector2D(Bounds.X,Bounds.Y),FVector2D(Bounds.X+Width,Bounds.Y+Height));
            int32 InformationRows=0;bool FoundAbandon=false;
            for(const auto& Value:Components)
            {
                const auto Other=Value->AsObject();
                const FString ElementId=Other->GetStringField(TEXT("id"));
                const FString Bind=HearthwardData::Text(Other,TEXT("bind"));
                const FString Action=HearthwardData::Text(Other,TEXT("action"));
                const bool Information=Bind==TEXT("time") || Bind==TEXT("location")
                    || ElementId==TEXT("pause.element.016") || ElementId==TEXT("pause.element.019");
                const bool SurvivalButton=Action==TEXT("giveUp") || Action==TEXT("cancelSurvival") || Action==TEXT("menuPause");
                if(!Other->GetBoolField(TEXT("visible")) || (!Information && !SurvivalButton))continue;
                if(Information)++InformationRows;
                if(Action==TEXT("giveUp"))FoundAbandon=true;
                const FVector4 OtherBounds=ReadRect(Other);
                const FBox2D Occupied(FVector2D(OtherBounds.X,OtherBounds.Y),FVector2D(OtherBounds.X+OtherBounds.Z,OtherBounds.Y+OtherBounds.W));
                TestFalse(Case+TEXT(" drawn objective avoids ")+ElementId,TextBounds.Intersect(Occupied));
                if(Action==TEXT("giveUp"))TestEqual(Case+TEXT(" actual abandonment hit target remains available"),
                    Screen->ActionAt(FVector2D(OtherBounds.X+OtherBounds.Z*.5,OtherBounds.Y+OtherBounds.W*.5)),Action);
            }
            TestEqual(Case+TEXT(" both titles and values are present"),InformationRows,4);
            if(LifeCase==1)TestTrue(Case+TEXT(" actual Downed abandonment button is present"),FoundAbandon);
        }
    }
    AddInfo(FString::Printf(TEXT("Measured %d real catalog objectives in Alive and Downed Pause; maximum wrapped lines=%d"),QuestRows.Num(),MaximumLines));
    Cleanup();return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadingPageInput069Test,
    "Hearthward.UI069.LoadingRestoresLatestPageInputMode",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoadingPageInput069Test::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>(GEngine);Instance->InitializeStandalone();
    auto* World=Instance->GetWorld();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Instance->GetWorldContext()->GameViewport=Viewport;
    Viewport->Init(*Instance->GetWorldContext(),Instance,false);
    const TSharedRef<SViewport> SlateViewport=SNew(SViewport);
    TSharedPtr<FSceneViewport> SceneViewport=Viewport->CreateViewport(SlateViewport);
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);
    Instance->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));
    auto* Controller=World->SpawnActor<APlayerController>();Controller->SetPlayer(LocalPlayer);
    World->AddController(Controller);
    auto* Player=World->SpawnActor<ACharacter>();Controller->Possess(Player);
    auto* Traversal=NewObject<UHearthwardTraversalComponent>(Player);
    Player->AddInstanceComponent(Traversal);Traversal->RegisterComponent();
    auto* Bag=NewObject<UHearthwardInventoryComponent>(Player);
    Player->AddInstanceComponent(Bag);Bag->RegisterComponent();
    auto* Gameplay=NewObject<UHearthwardGameplayComponent>(Player);
    Player->AddInstanceComponent(Gameplay);Gameplay->RegisterComponent();Gameplay->Enabled=true;
    auto* Survival=NewObject<UHearthwardSurvivalComponent>(Player);
    Player->AddInstanceComponent(Survival);Survival->RegisterComponent();
    auto* Combat=NewObject<UHearthwardCombatComponent>(Player);
    Player->AddInstanceComponent(Combat);Combat->RegisterComponent();
    auto* Building=NewObject<UHearthwardBuildingComponent>(Player);
    Player->AddInstanceComponent(Building);Building->RegisterComponent();
    auto* Screen=CreateWidget<UHearthwardScreenWidget>(Controller);
    auto* Loading=Instance->GetSubsystem<UHearthwardLoadingSubsystem>();
    const auto Cleanup=[&]()
    {
        if(Loading)Loading->FinishSession(false);
        if(Screen)Screen->RemoveFromParent();
        SceneViewport->SetViewportClient(nullptr);SceneViewport.Reset();
        Instance->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    };
    if(!TestNotNull(TEXT("Actual loading subsystem exists"),Loading)
        || !TestNotNull(TEXT("Actual page widget exists"),Screen)
        || !TestTrue(TEXT("Input modes have an actual Slate viewport"),Viewport->GetGameViewportWidget().IsValid()))
    {Cleanup();return false;}
    Screen->SetIsFocusable(true);Screen->TakeWidget();Screen->AddToViewport();
    Screen->OpenPage(TEXT("title"));
    if(!TestTrue(TEXT("Actual Title UIOnly starts with ignored game input"),Viewport->IgnoreInput()))
    {Cleanup();return false;}

    Loading->BeginLoading();
    TestTrue(TEXT("Loading owns a blocking overlay"),Loading->IsLoading() && Viewport->IgnoreInput());
    Screen->OpenPage(TEXT("hud"));
    TestTrue(TEXT("Actual HUD GameOnly cannot release input while loading remains visible"),Viewport->IgnoreInput());
    Loading->FinishSession(false);
    TestFalse(TEXT("Public loading completion removes the overlay"),Loading->IsLoading());
    TestFalse(TEXT("Completion restores the latest HUD GameOnly input instead of the old Title mode"),Viewport->IgnoreInput());

    Screen->OpenPage(TEXT("title"));
    Loading->BeginLoading();
    Screen->OpenPage(TEXT("hud"));
    TestTrue(TEXT("Intermediate HUD mode remains blocked by loading"),Viewport->IgnoreInput());
    Screen->OpenPage(TEXT("pause"));
    TestTrue(TEXT("Switching back to UI keeps input blocked during loading"),Viewport->IgnoreInput());
    Loading->FinishSession(false);
    TestTrue(TEXT("Completion restores the latest UIOnly mode"),Viewport->IgnoreInput());

    Screen->OpenPage(TEXT("hud"));
    TestFalse(TEXT("Actual GameOnly supplies the no-page-change travel starting state"),Viewport->IgnoreInput());
    Loading->BeginLoading();
    TestTrue(TEXT("Travel without a page change still blocks input while loading"),Viewport->IgnoreInput());
    Loading->FinishSession(false);
    TestFalse(TEXT("Travel without a page change restores its original GameOnly input"),Viewport->IgnoreInput());
    Cleanup();return true;
}
#endif
