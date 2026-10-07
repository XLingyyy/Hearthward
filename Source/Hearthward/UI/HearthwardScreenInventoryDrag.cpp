#include "HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameplayComponent.h"
#include "../Gameplay/HearthwardGameData.h"
#include "../Inventory/HearthwardInventoryComponent.h"
#include "../Inventory/HearthwardStorageSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"

int32 UHearthwardScreenWidget::InventoryTarget(FVector2D Point) const
{
    if(Page!=TEXT("inventory") || LayoutEditing || !ConfirmAction.IsEmpty())return INDEX_NONE;
    for(int32 Index=Elements.Num()-1;Index>=0;--Index)
    {
        const auto& Cell=Elements[Index];
        if(Cell.Hidden || !Cell.Enabled || (Cell.InventoryPosition==INDEX_NONE && Cell.EquipmentSlot.IsNone() && Cell.QuickSlot==INDEX_NONE))continue;
        if(Point.X>=Cell.Position.X && Point.Y>=Cell.Position.Y && Point.X<Cell.Position.X+Cell.Size.X && Point.Y<Cell.Position.Y+Cell.Size.Y)return Index;
    }
    return INDEX_NONE;
}
bool UHearthwardScreenWidget::InventoryDropAllowed(const FHearthwardUIElement& Target) const
{
    if(InventoryDragItem.IsNone())return false;
    if(Target.InventoryPosition!=INDEX_NONE)return UHearthwardGameplayComponent::InventoryTab(InventoryDragItem)==Target.InventoryGroup;
    if(Target.QuickSlot!=INDEX_NONE)return UHearthwardGameplayComponent::FitsQuickSlot(Target.QuickSlot,InventoryDragItem);
    const auto Row=HearthwardData::Find(TEXT("items"),InventoryDragItem.ToString());
    return InventoryDragInstance.IsValid() && !Target.EquipmentSlot.IsNone()
        && Target.EquipmentSlot==FName(*HearthwardData::Text(Row,TEXT("slot")));
}
void UHearthwardScreenWidget::CancelInventoryDrag(bool ReleaseCapture)
{
    InventoryDragItem=InventoryDragEquipment=NAME_None;InventoryDragInstance.Invalidate();InventoryDragEpoch.Invalidate();
    InventoryDragQuick=INDEX_NONE;InventoryDragging=false;InventoryDragAsset.Reset();InventoryDragSource.Reset();
    const auto Widget=GetCachedWidget();
    if(ReleaseCapture && FSlateApplication::IsInitialized() && Widget.IsValid() && Widget->HasMouseCapture())FSlateApplication::Get().ReleaseAllPointerCapture();
}
void UHearthwardScreenWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& Event)
{
    MapDragging=false;CancelInventoryDrag(false);Super::NativeOnMouseCaptureLost(Event);
}
FReply UHearthwardScreenWidget::InventoryMouseDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
    const FVector2D Point=CanvasPoint(Geometry,Event.GetScreenSpacePosition());const int32 Target=InventoryTarget(Point);
    if(!Elements.IsValidIndex(Target) || Elements[Target].InventoryItem.IsNone())return FReply::Unhandled();
    const auto Cell=Elements[Target];
    if(!Inventory() || Inventory()->GetItemCount(Cell.InventoryItem)<=0)return FReply::Handled();
    CancelInventoryDrag();
    InventoryDragItem=Cell.InventoryItem;InventoryDragInstance=Cell.InventoryInstance;InventoryDragEquipment=Cell.EquipmentSlot;
    InventoryDragQuick=Cell.QuickSlot;InventoryDragSource=Cell.LayoutId;InventoryDragAsset=Cell.Asset;
    InventoryDragEpoch=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch();
    InventoryDragStart=InventoryDragCursor=Point;SelectedItem=Cell.InventoryItem;
    InventorySelectionEpoch=InventoryDragEpoch;
    if(Cell.InventoryInstance.IsValid())
    {EquipmentSelection=Cell.InventoryInstance;EquipmentOwner=TEXT("player");EquipmentStack=NAME_None;}
    Refresh();return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
}
FReply UHearthwardScreenWidget::InventoryMouseUp(const FGeometry& Geometry,const FPointerEvent& Event)
{
    InventoryDragCursor=CanvasPoint(Geometry,Event.GetScreenSpacePosition());
    InventoryDragging|=(InventoryDragCursor-InventoryDragStart).SizeSquared()>=25;
    auto* G=Gameplay();const int32 TargetIndex=InventoryTarget(InventoryDragCursor);
    const FHearthwardUIElement Target=Elements.IsValidIndex(TargetIndex)?Elements[TargetIndex]:FHearthwardUIElement();
    if(Page!=TEXT("inventory") || !G || InventoryDragEpoch!=GetWorld()->GetSubsystem<UHearthwardStorageSubsystem>()->GetTimelineEpoch())
        Message=TEXT("存档或界面已变化，请重新拖动");
    else if(!InventoryDragging)
    {
        const FName Item=InventoryDragItem;CancelInventoryDrag(false);ExecuteAction(TEXT("item:")+Item.ToString());
        return FReply::Handled().ReleaseMouseCapture();
    }
    else if(!Inventory() || Inventory()->GetItemCount(InventoryDragItem)<=0
        || (InventoryDragInstance.IsValid() && !Inventory()->FindInstance(InventoryDragInstance))
        || (!InventoryDragEquipment.IsNone() && Inventory()->EquippedInstance(InventoryDragEquipment)!=InventoryDragInstance)
        || (InventoryDragQuick!=INDEX_NONE && G->QuickItem(InventoryDragQuick)!=InventoryDragItem))Message=TEXT("物品已变化，请重新拖动");
    else if(TargetIndex!=INDEX_NONE)
    {
        if(Target.InventoryPosition!=INDEX_NONE && !InventoryDropAllowed(Target))Message=TEXT("无法放置：物品不属于当前背包分类");
        else if(Target.InventoryPosition!=INDEX_NONE)
        {
            if(G->MoveInventoryItem(InventoryDragItem,Target.InventoryPosition,InventoryDragEpoch,InventoryDragEquipment.IsNone()?FGuid():InventoryDragInstance))
                if(InventoryDragQuick!=INDEX_NONE)G->AssignQuickItem(InventoryDragQuick,NAME_None);
            Message=G->Feedback;
        }
        else if(Target.QuickSlot!=INDEX_NONE){G->AssignQuickItem(Target.QuickSlot,InventoryDragItem);Message=G->Feedback;}
        else {G->SetBackpackEquipment(InventoryDragInstance,Target.EquipmentSlot,true,InventoryDragEpoch);Message=G->Feedback;}
    }
    else if(!InventoryDragEquipment.IsNone())
    {
        G->SetBackpackEquipment(InventoryDragInstance,InventoryDragEquipment,false,InventoryDragEpoch);Message=G->Feedback;
    }
    else if(InventoryDragQuick!=INDEX_NONE)
    {G->AssignQuickItem(InventoryDragQuick,NAME_None);Message=G->Feedback;}
    else Message=TEXT("请拖到背包格子或对应栏位");
    MessageUntil=FPlatformTime::Seconds()+2;CancelInventoryDrag(false);Hover=KeyboardFocus=INDEX_NONE;Refresh();
    return FReply::Handled().ReleaseMouseCapture();
}
