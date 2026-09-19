#include "HearthwardScreenWidget.h"
#include "../Gameplay/HearthwardGameData.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

using namespace HearthwardData;
namespace
{
FVector4 ReadLayoutRect(const TSharedPtr<FJsonObject>& O,const TCHAR* Field)
{
    const auto& R=O->GetArrayField(Field);
    return FVector4(R[0]->AsNumber(),R[1]->AsNumber(),R[2]->AsNumber(),R[3]->AsNumber());
}
void WriteRect(const TSharedPtr<FJsonObject>& O,FVector2D P,FVector2D S)
{
    TArray<TSharedPtr<FJsonValue>> R;
    for(double V:{P.X,P.Y,S.X,S.Y}) R.Add(MakeShared<FJsonValueNumber>(V));
    O->SetArrayField(TEXT("rect"),R);
}
bool Contains(FVector2D P,FVector2D A,FVector2D S)
{ return P.X>=A.X && P.Y>=A.Y && P.X<=A.X+S.X && P.Y<=A.Y+S.Y; }
FString LayoutFile() { return FPaths::ProjectDir()/TEXT("Resources/UI/layout.json"); }
}
TSharedPtr<FJsonObject> UHearthwardScreenWidget::PageLayout() const
{ return LayoutConfig->GetObjectField(TEXT("pages"))->GetObjectField(Page.ToString()); }
bool UHearthwardScreenWidget::ReloadLayout()
{
    FString Json; TSharedPtr<FJsonObject> Parsed;
    if(!FFileHelper::LoadFileToString(Json,*LayoutFile()) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Parsed)) return false;
    LayoutConfig=Parsed; LayoutSelection.Reset(); LayoutUndo.Reset(); Refresh(); return true;
}
bool UHearthwardScreenWidget::SaveLayout()
{
#if !UE_BUILD_SHIPPING
    FString Json; FJsonSerializer::Serialize(LayoutConfig.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
    const bool Saved=FFileHelper::SaveStringToFile(Json,*LayoutFile(),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    LayoutStatus=Saved?TEXT("布局已保存至 Resources/UI/layout.json"):TEXT("保存失败，请检查文件写入权限");
    return Saved;
#else
    return false;
#endif
}
void UHearthwardScreenWidget::RememberLayout()
{
    FString Json; FJsonSerializer::Serialize(LayoutConfig.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
    LayoutUndo.Add(Json); if(LayoutUndo.Num()>32) LayoutUndo.RemoveAt(0);
}
void UHearthwardScreenWidget::LoadComponents()
{
    for(const auto& V:PageLayout()->GetArrayField(TEXT("components")))
    {
        const auto C=V->AsObject(); const FString Asset=Text(C,TEXT("asset"));
        if(Asset.IsEmpty()) continue;
        const auto R=ReadLayoutRect(C,TEXT("base"));
        Element(TEXT("image"),TEXT(""),FVector2D(R.X,R.Y),FVector2D(R.Z,R.W),18,TEXT(""),Asset);
        Elements.Last().Component=Text(C,TEXT("id"));
        Elements.Last().LayoutId=TEXT("surface:")+Text(C,TEXT("id"));
    }
}
FVector2D UHearthwardScreenWidget::ComponentPoint(const FString& Id,FVector2D P,bool Inverse) const
{
    for(const auto& V:PageLayout()->GetArrayField(TEXT("components")))
    {
        const auto C=V->AsObject(); if(Text(C,TEXT("id"))!=Id) continue;
        const auto B=ReadLayoutRect(C,TEXT("base")),R=ReadLayoutRect(C,TEXT("rect"));
        const FVector2D BP(B.X,B.Y),BS(B.Z,B.W),RP(R.X,R.Y),RS(R.Z,R.W);
        return Inverse?BP+(P-RP)*BS/RS:RP+(P-BP)*RS/BS;
    }
    return P;
}
void UHearthwardScreenWidget::ApplyLayout()
{
    LayoutBounds.Reset(); LayoutOrder.Reset();
    const auto P=PageLayout(); const auto& Components=P->GetArrayField(TEXT("components"));
    const auto Overrides=P->GetObjectField(TEXT("overrides"));
    for(const auto& V:Components)
    {
        const auto C=V->AsObject(); const auto R=ReadLayoutRect(C,TEXT("rect"));
        FLayoutBounds B; B.Position=FVector2D(R.X,R.Y); B.Size=FVector2D(R.Z,R.W); B.Hidden=!C->GetBoolField(TEXT("visible"));
        const FString Id=Text(C,TEXT("id")); LayoutBounds.Add(Id,B); LayoutOrder.Add(Id);
    }
    TMap<FString,int32> Occurrences;
    for(auto& E:Elements)
    {
        if(E.LayoutId==TEXT("background")) continue;
        if(E.LayoutId.IsEmpty())
        {
            // Address a template slot by geometry/type, independent of changing item names, counts and actions.
            const FString Key=FString::Printf(TEXT("dynamic:%s:%.1f:%.1f"),*E.Type,E.Position.X,E.Position.Y);
            E.LayoutId=Key+FString::Printf(TEXT(":%d"),Occurrences.FindOrAdd(Key)++);
        }
        if(E.Component.IsEmpty())
        {
            const FVector2D Center=E.Position+E.Size*.5;
            for(const auto& V:Components)
            {
                const auto C=V->AsObject(); const TArray<TSharedPtr<FJsonValue>>* Capture;
                if(!C->TryGetArrayField(TEXT("capture"),Capture)) continue;
                const auto R=ReadLayoutRect(C,TEXT("capture"));
                if(Contains(Center,FVector2D(R.X,R.Y),FVector2D(R.Z,R.W))) E.Component=Text(C,TEXT("id"));
            }
        }
        if(E.MapClipped) E.Component=TEXT("map.canvas");
        const FVector2D OriginalSize=E.Size;
        const TSharedPtr<FJsonObject>* Override;
        if(Overrides->TryGetObjectField(E.LayoutId,Override))
        {
            const TArray<TSharedPtr<FJsonValue>>* Values;
            if((*Override)->TryGetArrayField(TEXT("rect"),Values))
            { const auto R=ReadLayoutRect(*Override,TEXT("rect")); E.Position=FVector2D(R.X,R.Y); E.Size=FVector2D(R.Z,R.W); }
            bool Visible; if((*Override)->TryGetBoolField(TEXT("visible"),Visible)) E.Hidden=!Visible;
        }
        const FVector2D End=ComponentPoint(E.Component,E.Position+E.Size);
        E.Position=ComponentPoint(E.Component,E.Position); E.Size=End-E.Position;
        if(const auto* Parent=LayoutBounds.Find(E.Component)) E.Hidden|=Parent->Hidden;
        const float Factor=FMath::Min(FMath::Abs(E.Size.X/FMath::Max(1.,OriginalSize.X)),FMath::Abs(E.Size.Y/FMath::Max(1.,OriginalSize.Y)));
        E.Font*=Factor; E.TextInset*=Factor;
        FLayoutBounds B; B.Position=E.Position; B.Size=E.Size; B.Parent=E.Component; B.Hidden=E.Hidden;
        LayoutBounds.Add(E.LayoutId,B); LayoutOrder.Add(E.LayoutId);
    }
    if(Draft && Page==TEXT("dialogue"))
    {
        const auto* B=LayoutBounds.Find(TEXT("dialogue.input"));
        if(B)
        {
            auto* CanvasSlot=CastChecked<UCanvasPanelSlot>(Draft->Slot); CanvasSlot->SetPosition(B->Position); CanvasSlot->SetSize(B->Size);
            Draft->SetVisibility(B->Hidden || LayoutEditing?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
        }
    }
}
bool UHearthwardScreenWidget::SetComponentRect(const FString& Id,FVector2D Position,FVector2D Size)
{
#if !UE_BUILD_SHIPPING
    if(Size.X<1 || Size.Y<1 || !LayoutBounds.Contains(Id)) return false;
    for(const auto& V:PageLayout()->GetArrayField(TEXT("components")))
    {
        const auto C=V->AsObject(); if(Text(C,TEXT("id"))==Id)
        { WriteRect(C,Position,Size); Refresh(); return true; }
    }
    const auto B=LayoutBounds[Id];
    const FVector2D End=ComponentPoint(B.Parent,Position+Size,true);
    Position=ComponentPoint(B.Parent,Position,true); Size=End-Position;
    const auto Overrides=PageLayout()->GetObjectField(TEXT("overrides"));
    const TSharedPtr<FJsonObject>* Existing;
    auto O=Overrides->TryGetObjectField(Id,Existing)?*Existing:MakeShared<FJsonObject>();
    WriteRect(O,Position,Size); Overrides->SetObjectField(Id,O); Refresh(); return true;
#else
    return false;
#endif
}
bool UHearthwardScreenWidget::SetComponentVisible(const FString& Id,bool Visible)
{
#if !UE_BUILD_SHIPPING
    if(!LayoutBounds.Contains(Id)) return false;
    for(const auto& V:PageLayout()->GetArrayField(TEXT("components")))
    {
        const auto C=V->AsObject(); if(Text(C,TEXT("id"))==Id)
        { C->SetBoolField(TEXT("visible"),Visible); Refresh(); return true; }
    }
    const auto Overrides=PageLayout()->GetObjectField(TEXT("overrides"));
    const TSharedPtr<FJsonObject>* Existing;
    auto O=Overrides->TryGetObjectField(Id,Existing)?*Existing:MakeShared<FJsonObject>();
    O->SetBoolField(TEXT("visible"),Visible); Overrides->SetObjectField(Id,O); Refresh(); return true;
#else
    return false;
#endif
}
FString UHearthwardScreenWidget::DescribeLayout() const
{
    auto Result=MakeShared<FJsonObject>(); Result->SetStringField(TEXT("page"),Page.ToString());
    TArray<TSharedPtr<FJsonValue>> Rows;
    for(const auto& Id:LayoutOrder)
    {
        const auto& B=LayoutBounds[Id]; auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"),Id);
        R->SetStringField(TEXT("parent"),B.Parent); R->SetBoolField(TEXT("visible"),!B.Hidden); WriteRect(R,B.Position,B.Size);
        if(const auto* E=Elements.FindByPredicate([&](const auto& Row){return Row.LayoutId==Id;}))
        { R->SetStringField(TEXT("action"),E->Action); R->SetStringField(TEXT("text"),E->Text); R->SetStringField(TEXT("asset"),E->Asset); }
        Rows.Add(MakeShared<FJsonValueObject>(R));
    }
    Result->SetArrayField(TEXT("components"),Rows);
    FString Json; FJsonSerializer::Serialize(Result,TJsonWriterFactory<>::Create(&Json)); return Json;
}
FString UHearthwardScreenWidget::ActionAt(FVector2D Point) const
{ const int32 Index=Hit(Point); return Elements.IsValidIndex(Index)?Elements[Index].Action:FString(); }
void UHearthwardScreenWidget::SetLayoutEditing(bool Editing)
{
#if !UE_BUILD_SHIPPING
    LayoutEditing=Editing; LayoutDragging=false; LayoutSelection.Reset(); Hover=KeyboardFocus=INDEX_NONE;
    OpenPage(Page); // Reapply the page's input mode; editor makes even the HUD interactive.
#endif
}
FString UHearthwardScreenWidget::LayoutHit(FVector2D Point,bool Leaf) const
{
    for(int32 I=LayoutOrder.Num()-1;I>=0;--I)
    {
        const auto& Id=LayoutOrder[I]; const auto& B=LayoutBounds[Id];
        if(B.Hidden || !Contains(Point,B.Position,B.Size)) continue;
        if(!Leaf && !B.Parent.IsEmpty()) return B.Parent;
        return Id;
    }
    return FString();
}
FReply UHearthwardScreenWidget::LayoutMouseDown(const FGeometry& G,const FPointerEvent& E)
{
    if(E.GetEffectingButton()!=EKeys::LeftMouseButton) return FReply::Handled();
    const auto Point=CanvasPoint(G,E.GetScreenSpacePosition());
    const auto* Selected=LayoutBounds.Find(LayoutSelection);
    LayoutResizing=Selected && Contains(Point,Selected->Position+Selected->Size-FVector2D(14,14),FVector2D(22,22));
    if(!LayoutResizing) LayoutSelection=LayoutHit(Point,E.IsAltDown());
    if(const auto* B=LayoutBounds.Find(LayoutSelection))
    {
        RememberLayout(); DragStart=Point; DragPosition=B->Position; DragSize=B->Size; LayoutDragging=true;
        return FReply::Handled().CaptureMouse(TakeWidget());
    }
    return FReply::Handled();
}
FReply UHearthwardScreenWidget::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)
{
    if(LayoutDragging) { LayoutDragging=false; return FReply::Handled().ReleaseMouseCapture(); }
    return Super::NativeOnMouseButtonUp(G,E);
}
bool UHearthwardScreenWidget::LayoutKey(const FKeyEvent& E)
{
#if !UE_BUILD_SHIPPING
    const FKey Key=E.GetKey();
    if(Key==EKeys::F10) { SetLayoutEditing(!LayoutEditing); return true; }
    if(!LayoutEditing) return false;
    if(Key==EKeys::Escape) { SetLayoutEditing(false); return true; }
    if(E.IsControlDown() && Key==EKeys::S) { SaveLayout(); return true; }
    if(E.IsControlDown() && Key==EKeys::Z && !LayoutUndo.IsEmpty())
    {
        const FString Previous=LayoutUndo.Pop();
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Previous),LayoutConfig); Refresh(); return true;
    }
    if(Key==EKeys::PageDown || Key==EKeys::PageUp)
    {
        TArray<FString> Pages; for(const auto& Entry:Theme->GetObjectField(TEXT("pages"))->Values) Pages.Add(FString(*Entry.Key)); Pages.Sort();
        const int32 Index=Pages.IndexOfByKey(Page.ToString());
        OpenPage(FName(*Pages[(Index+(Key==EKeys::PageDown?1:Pages.Num()-1))%Pages.Num()])); return true;
    }
    if(Key==EKeys::Tab)
    {
        const auto& Components=PageLayout()->GetArrayField(TEXT("components"));
        const int32 Index=Components.IndexOfByPredicate([&](const auto& V){return Text(V->AsObject(),TEXT("id"))==LayoutSelection;});
        if(!Components.IsEmpty()) LayoutSelection=Text(Components[(Index+1)%Components.Num()]->AsObject(),TEXT("id"));
        return true;
    }
    if(const auto* Found=LayoutBounds.Find(LayoutSelection))
    {
        const auto B=*Found;
        if(Key==EKeys::Delete) { RememberLayout(); SetComponentVisible(LayoutSelection,B.Hidden); return true; }
        if(Key==EKeys::Left || Key==EKeys::Right || Key==EKeys::Up || Key==EKeys::Down)
        {
            RememberLayout(); const float Step=E.IsShiftDown()?10:1;
            SetComponentRect(LayoutSelection,B.Position+FVector2D(Key==EKeys::Left?-Step:Key==EKeys::Right?Step:0,Key==EKeys::Up?-Step:Key==EKeys::Down?Step:0),B.Size);
        }
    }
    if(Key==EKeys::Home)
    {
        RememberLayout();
        for(const auto& V:PageLayout()->GetArrayField(TEXT("components")))
        { auto C=V->AsObject(); C->SetArrayField(TEXT("rect"),C->GetArrayField(TEXT("base"))); C->SetBoolField(TEXT("visible"),true); }
        PageLayout()->SetObjectField(TEXT("overrides"),MakeShared<FJsonObject>()); Refresh();
    }
    return true;
#else
    return false;
#endif
}
