#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Dom/JsonObject.h"
#include "Styling/SlateBrush.h"
#include "../Experience/HearthwardPlayerSettings.h"
#include "HearthwardScreenWidget.generated.h"

struct FHearthwardUIElement
{
    FString Type, Text, Asset, Action, Bind, Id, FontRole, Align, LayoutId, Component;
    FVector2D Position=FVector2D::ZeroVector, Size=FVector2D::ZeroVector;
    FLinearColor Color=FLinearColor::White;
    float Font=18, Value=1, TextInset=18;
    int32 Tracking=0;
    bool Enabled=true, Selected=false, Hidden=false;
    bool MapClipped=false;
    bool TextScrollClipped=false;
};

struct FHearthwardSavePoint;

UCLASS()
class HEARTHWARD_API UHearthwardScreenWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void OpenPage(FName Name);
    UFUNCTION(BlueprintCallable) void OpenNature(FGuid Target);
    UFUNCTION(BlueprintCallable) bool ExecuteAction(const FString& Action);
    UFUNCTION(BlueprintPure) FName GetPage() const { return Page; }
    UFUNCTION(BlueprintPure) FString GetCategory() const { return Category; }
    UFUNCTION(BlueprintPure) FString GetMessage() const { return Message; }
    UFUNCTION(BlueprintCallable) void Refresh();
    UFUNCTION(BlueprintCallable) void SetLayoutEditing(bool Editing);
    UFUNCTION(BlueprintCallable) bool SetComponentRect(const FString& Id,FVector2D Position,FVector2D Size);
    UFUNCTION(BlueprintCallable) bool SetComponentVisible(const FString& Id,bool Visible);
    UFUNCTION(BlueprintCallable) bool SaveLayout();
    UFUNCTION(BlueprintCallable) bool ReloadLayout();
    UFUNCTION(BlueprintPure) FString DescribeLayout() const;
    UFUNCTION(BlueprintPure) FString ActionAt(FVector2D Point) const;
    UFUNCTION(BlueprintCallable) bool CaptureUI(const FString& Name,int32 Width=1672,int32 Height=941);
    void InitializeScreen(class AHearthwardHUD* HUD);
    virtual void NativeTick(const FGeometry& Geometry,float Delta) override;
    virtual void NativeDestruct() override;
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual FReply NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    UPROPERTY() TObjectPtr<class AHearthwardHUD> OwnerHUD;
    UPROPERTY() TArray<TObjectPtr<class UTexture2D>> Textures;
    UPROPERTY() TObjectPtr<class UEditableTextBox> Draft;
    UFUNCTION() void DraftCommitted(const FText& Text,ETextCommit::Type Method);
    class UHearthwardGameplayComponent* Gameplay() const;
    class UHearthwardInventoryComponent* Inventory() const;
    void LoadTheme();
    bool PrepareSession();
    bool OpenSavePoint(const FHearthwardSavePoint& Point);
    void LoadElements(const TArray<TSharedPtr<FJsonValue>>& Rows);
    void ComposeInventory(bool Storage);
    void ComposeEquipment();
    void ComposeNature();
    bool ExecuteNatureAction(const FString& Action);
    FGuid NatureSelection,NatureEpoch;
    bool ExecuteEquipmentAction(const FString& Action);
    FName EquipmentOwner=TEXT("player");
    FGuid EquipmentSelection,EquipmentEpoch;
    FName EquipmentStack;
    void ComposeSkills();
    void ComposeMap();
    void ComposeJournal();
    void ComposeCodex();
    void ComposeDialogue();
    void ComposeMemory();
    void ComposeHUD();
    void ComposeBuilding();
    void ComposeCamp();
    bool ExecuteCampAction(const FString& Action);
    void ComposeCrafting();
    void ComposeRepair();
    void ComposeSave();
    void ComposeSettings();
    bool ExecuteSettingsAction(const FString& Action);
    void LoadSettingsDraft();
    void ApplyMasterVolume();
    void ComposeCompatibility();
    void ComposeUpdateNotice();
    int32 CompatibilityScroll=0;
    void Element(FString Type,FString Text,FVector2D Position,FVector2D Size,float Font=18,FString Action=TEXT(""),FString Asset=TEXT(""),bool Selected=false);
    FString Resolve(const FString& Bind) const;
    FLinearColor Color(const FString& Name) const;
    FSlateBrush* Brush(const FString& Name) const;
    FVector2D CanvasPoint(const FGeometry& Geometry,const FVector2D& Screen) const;
    int32 Hit(const FVector2D& Point) const;
    void ApplyLayout();
    void ApplyReadableLayout();
    void ApplyReadableHUD();
    bool ReadableLayout() const;
    float TextScroll=0,TextScrollMaximum=0;
    void LoadComponents();
    bool LayoutKey(const FKeyEvent& Event);
    FReply LayoutMouseDown(const FGeometry& Geometry,const FPointerEvent& Event);
    FString LayoutHit(FVector2D Point,bool Leaf) const;
    TSharedPtr<FJsonObject> PageLayout() const;
    FVector2D ComponentPoint(const FString& Id,FVector2D Point,bool Inverse=false) const;
    void RememberLayout();
    TSharedPtr<FJsonObject> LayoutConfig;
    struct FLayoutBounds { FVector2D Position,Size; FString Parent; bool Hidden=false; };
    TMap<FString,FLayoutBounds> LayoutBounds;
    TArray<FString> LayoutOrder,LayoutUndo;
    FString LayoutSelection,LayoutStatus;
    FVector2D DragStart,DragPosition,DragSize;
    bool LayoutEditing=false,LayoutDragging=false,LayoutResizing=false;
    TSharedPtr<FJsonObject> Theme;
    TSharedPtr<const FCompositeFont> Typeface;
    TSharedPtr<const FCompositeFont> DisplayTypeface;
    TMap<FString,FSlateBrush> Brushes;
    TArray<FHearthwardUIElement> Elements;
    FName Page=TEXT("title");
    TArray<FName> ReturnPages;
    TArray<FString> ReturnCategories;
    FName SelectedItem=TEXT("axe"),SelectedSkill=TEXT("strong"),SelectedQuest=TEXT("ember"),SelectedLocation=TEXT("camp");
    FName SelectedCodex;
    FString Category,Message,ConfirmAction,ConfirmMessage;
    TArray<FString> ResetItems;
    FGuid ResetEpoch,ResetCommand,ResetCandidate;
    int64 ResetRevision=0;
    int32 ResetScroll=0;
    bool bResetActive=false;
    int32 Quantity=1,Scroll=0,Hover=INDEX_NONE,KeyboardFocus=INDEX_NONE;
    float RefreshDelay=0,MapZoom=1;
    double MessageUntil=0;
    FVector2D MapPan=FVector2D::ZeroVector;
    bool OwnPause=false,StorageToCamp=true;
    bool MenuPause=true;
    UPROPERTY() TObjectPtr<class USoundMix> SettingsSoundMix;
    FString SettingsSelection;
    int32 SettingsAutoMinutes=10,SettingsFrameLimit=0,SettingsQuality=2,SettingsShadow=2;
    int32 SettingsTexture=2,SettingsViewDistance=2,SettingsEffects=2,SettingsVolume=100,SettingsSensitivity=5;
    bool SettingsMenuPause=true,SettingsFullscreen=true,SettingsVSync=false,SettingsInvertY=false,SettingsDirty=false;
    FGuid StorageEpoch;
    FGuid MemoryEpoch, SelectedMemory;
    int64 MemoryRevision = 0;
    int32 AgentCapabilityIndex=0,AgentItemIndex=0,AgentInstanceIndex=0,AgentSourceIndex=0;
    FName MemoryKind=TEXT("claim");
    FName MemoryBlockedItem=TEXT("wood");
    FGuid CraftingEpoch,Workbench;
    FName SelectedRecipe;
    FName SelectedRepair;
    int32 CraftingBatches=1;
    FGuid CampEpoch,CampFacility;
    FName CampRegion,CampRecipe;
    int32 CampPerson=0,CampFood=0;
    FVector2D DesignSize=FVector2D(1672,941);
};
