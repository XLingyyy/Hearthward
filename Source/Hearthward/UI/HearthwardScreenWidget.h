#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Dom/JsonObject.h"
#include "Styling/SlateBrush.h"
#include "HearthwardScreenWidget.generated.h"

struct FHearthwardUIElement
{
    FString Type, Text, Asset, Action, Bind, Id, FontRole, Align;
    FVector2D Position=FVector2D::ZeroVector, Size=FVector2D::ZeroVector;
    FLinearColor Color=FLinearColor::White;
    float Font=18, Value=1, TextInset=18;
    int32 Tracking=0;
    bool Enabled=true, Selected=false;
    bool MapClipped=false;
};

UCLASS()
class HEARTHWARD_API UHearthwardScreenWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void OpenPage(FName Name);
    UFUNCTION(BlueprintCallable) bool ExecuteAction(const FString& Action);
    UFUNCTION(BlueprintPure) FName GetPage() const { return Page; }
    UFUNCTION(BlueprintPure) FString GetMessage() const { return Message; }
    UFUNCTION(BlueprintCallable) void Refresh();
    UFUNCTION(BlueprintCallable) bool CaptureUI(const FString& Name,int32 Width=1672,int32 Height=941);
    void InitializeScreen(class AHearthwardHUD* HUD);
    virtual void NativeTick(const FGeometry& Geometry,float Delta) override;
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
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
    void LoadElements(const TArray<TSharedPtr<FJsonValue>>& Rows);
    void ComposeInventory(bool Storage);
    void ComposeSkills();
    void ComposeMap();
    void ComposeJournal();
    void ComposeCodex();
    void ComposeDialogue();
    void ComposeHUD();
    void ComposeSave();
    void Element(FString Type,FString Text,FVector2D Position,FVector2D Size,float Font=18,FString Action=TEXT(""),FString Asset=TEXT(""),bool Selected=false);
    FString Resolve(const FString& Bind) const;
    FLinearColor Color(const FString& Name) const;
    FSlateBrush* Brush(const FString& Name) const;
    FVector2D CanvasPoint(const FGeometry& Geometry,const FVector2D& Screen) const;
    int32 Hit(const FVector2D& Point) const;
    TSharedPtr<FJsonObject> Theme;
    TSharedPtr<const FCompositeFont> Typeface;
    TSharedPtr<const FCompositeFont> DisplayTypeface;
    TMap<FString,FSlateBrush> Brushes;
    TArray<FHearthwardUIElement> Elements;
    FName Page=TEXT("title");
    FName ReturnPage=TEXT("title");
    FName SelectedItem=TEXT("axe"),SelectedSkill=TEXT("strong"),SelectedQuest=TEXT("ember"),SelectedLocation=TEXT("camp");
    FName SelectedCodex;
    FString Category,Message,ConfirmAction;
    int32 Quantity=1,Scroll=0,Hover=INDEX_NONE,KeyboardFocus=INDEX_NONE;
    float RefreshDelay=0,MapZoom=1;
    double MessageUntil=0;
    FVector2D MapPan=FVector2D::ZeroVector;
    bool OwnPause=false,StorageToCamp=true;
    FGuid StorageEpoch;
    FVector2D DesignSize=FVector2D(1672,941);
};
