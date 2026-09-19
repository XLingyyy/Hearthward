#include "HearthwardScreenWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Blueprint/WidgetTree.h"
#include "Fonts/FontMeasure.h"

int32 UHearthwardScreenWidget::NativePaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool ParentEnabled) const
{
    if(!Theme) return Super::NativePaint(Args,G,Clip,Out,Layer,Style,ParentEnabled);
    const float Scale=FMath::Min(G.GetLocalSize().X/DesignSize.X,G.GetLocalSize().Y/DesignSize.Y);
    const FVector2D Offset=(G.GetLocalSize()-DesignSize*Scale)*.5;
    const FSlateBrush* White=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    auto Geometry=[&](FVector2D P,FVector2D S){ return G.ToPaintGeometry(S,FSlateLayoutTransform(Scale,Offset+P*Scale)); };
    auto Box=[&](FVector2D P,FVector2D S,FLinearColor C,int32 L){ FSlateDrawElement::MakeBox(Out,L,Geometry(P,S),White,ESlateDrawEffect::None,C); };
    auto Frame=[&](FVector2D P,FVector2D S,FLinearColor C,int32 L)
    { Box(P,FVector2D(S.X,1),C,L); Box(P+FVector2D(0,S.Y-1),FVector2D(S.X,1),C,L); Box(P,FVector2D(1,S.Y),C,L); Box(P+FVector2D(S.X-1,0),FVector2D(1,S.Y),C,L); };
    if(Page!=TEXT("hud")) Box(FVector2D::ZeroVector,DesignSize,FLinearColor::Black,Layer);
    for(int32 I=0;I<Elements.Num();++I)
    {
        const auto& E=Elements[I]; const bool Focus=I==Hover || I==KeyboardFocus || E.Selected;
        if(E.Hidden) continue;
        if(E.MapClipped)
        {
            const FVector2D A=G.LocalToAbsolute(Offset+ComponentPoint(TEXT("map.canvas"),FVector2D(407,95))*Scale);
            const FVector2D B=G.LocalToAbsolute(Offset+ComponentPoint(TEXT("map.canvas"),FVector2D(1517,855))*Scale);
            Out.PushClip(FSlateClippingZone(FSlateRect(A.X,A.Y,B.X,B.Y)));
        }
        const int32 L=Layer+I*4+1;
        const FLinearColor Ink=E.Enabled ? (Focus?Color(TEXT("gold")):E.Color) : Color(TEXT("muted"));
        if(E.Type==TEXT("image") || (!E.Asset.IsEmpty() && E.Type!=TEXT("minimap") && E.Type!=TEXT("portrait")))
            if(auto* B=Brush(E.Asset))
            {
                if(B->DrawAs==ESlateBrushDrawType::Box)
                {
                    // Runtime textures retain their full resolution. Slice explicitly so border
                    // thickness follows the authored brush size, not the texture's pixel dimensions.
                    const FBox2f UV=B->GetUVRegion(); const auto M=B->Margin;
                    const float X[]={0,FMath::Min(float(E.Size.X*.5),M.Left*B->ImageSize.X),float(E.Size.X)-FMath::Min(float(E.Size.X*.5),M.Right*B->ImageSize.X),float(E.Size.X)};
                    const float Y[]={0,FMath::Min(float(E.Size.Y*.5),M.Top*B->ImageSize.Y),float(E.Size.Y)-FMath::Min(float(E.Size.Y*.5),M.Bottom*B->ImageSize.Y),float(E.Size.Y)};
                    const float U[]={UV.Min.X,UV.Min.X+UV.GetSize().X*M.Left,UV.Max.X-UV.GetSize().X*M.Right,UV.Max.X};
                    const float V[]={UV.Min.Y,UV.Min.Y+UV.GetSize().Y*M.Top,UV.Max.Y-UV.GetSize().Y*M.Bottom,UV.Max.Y};
                    for(int32 Row=0;Row<3;++Row) for(int32 Column=0;Column<3;++Column)
                    {
                        FSlateBrush Slice=*B; Slice.DrawAs=ESlateBrushDrawType::Image;
                        Slice.SetUVRegion(FBox2f(FVector2f(U[Column],V[Row]),FVector2f(U[Column+1],V[Row+1])));
                        FSlateDrawElement::MakeBox(Out,L,Geometry(E.Position+FVector2D(X[Column],Y[Row]),FVector2D(X[Column+1]-X[Column],Y[Row+1]-Y[Row])),&Slice,ESlateDrawEffect::None,FLinearColor::White);
                    }
                }
                else FSlateDrawElement::MakeBox(Out,L,Geometry(E.Position,E.Size),B,ESlateDrawEffect::None,FLinearColor::White);
            }
        if(E.Type==TEXT("panel") || E.Type==TEXT("notice"))
        {
            Box(E.Position,E.Size,Color(TEXT("panel")),L); Frame(E.Position,E.Size,Color(TEXT("bronze")),L+1);
        }
        if(E.Type==TEXT("button") || E.Type==TEXT("choice") || E.Type==TEXT("tab") || E.Type==TEXT("slot") || E.Type==TEXT("node"))
        {
            if(E.Type==TEXT("tab"))
            {
                if(Focus) { Box(E.Position+FVector2D(10,E.Size.Y-3),FVector2D(E.Size.X-20,2),Color(TEXT("gold")),L+1); }
            }
            else if(E.Type==TEXT("node"))
            {
                if(Focus || E.Value>0)
                {
                    TArray<FVector2D> Circle;
                    for(int32 N=0;N<=48;++N) { const float A=2*PI*N/48; Circle.Add(E.Size*.5+E.Size*.55*FVector2D(FMath::Cos(A),FMath::Sin(A))); }
                    FSlateDrawElement::MakeLines(Out,L+2,Geometry(E.Position,E.Size),Circle,ESlateDrawEffect::None,Color(Focus?TEXT("gold"):TEXT("teal")),true,Focus?2.4f:1.f);
                }
            }
            else if(Focus)
            {
                if(auto* B=Brush(TEXT("selection"))) FSlateDrawElement::MakeBox(Out,L+1,Geometry(E.Position,E.Size),B,ESlateDrawEffect::None,FLinearColor(1,1,1,.72f));
                Frame(E.Position,E.Size,Color(TEXT("gold")),L+2);
                Frame(E.Position-FVector2D(2,2),E.Size+FVector2D(4,4),FLinearColor(.42f,.23f,.06f,.4f),L+1);
            }
            else if(E.Type==TEXT("choice"))
            { Box(E.Position,E.Size,FLinearColor(.02f,.018f,.014f,.58f),L); Frame(E.Position,E.Size,Color(TEXT("bronze")),L+1); }
            else if(E.Type!=TEXT("button")) Frame(E.Position,E.Size,Color(TEXT("bronze"))*.55f,L+1);
        }
        if(E.Type==TEXT("line")) Box(E.Position,E.Size,E.Color,L);
        if(E.Type==TEXT("keycap")) Frame(E.Position,E.Size,Color(TEXT("text")),L);
        if(E.Type==TEXT("minimap") || E.Type==TEXT("portrait"))
        {
            if(const auto* B=Brush(E.Asset))
            {
                const FBox2f UV=B->GetUVRegion();
                const FVector2f Center(E.Size*.5),Radius(E.Size*.5);
                const FSlateRenderTransform Transform=Geometry(E.Position,E.Size).GetAccumulatedRenderTransform();
                TArray<FSlateVertex> Vertices; TArray<SlateIndex> Indices; TArray<FVector2D> Outline;
                Vertices.Add(FSlateVertex::Make(Transform,Center,UV.GetCenter(),FColor::White));
                for(int32 N=0;N<=64;++N)
                {
                    const float Angle=2*PI*N/64;
                    const FVector2f Unit(FMath::Cos(Angle),FMath::Sin(Angle));
                    Vertices.Add(FSlateVertex::Make(Transform,Center+Unit*Radius,UV.GetCenter()+Unit*UV.GetSize()*.5f,FColor::White));
                    Outline.Add(FVector2D(Center+Unit*Radius));
                    if(N>0) { Indices.Add(0); Indices.Add(N); Indices.Add(N+1); }
                }
                const auto Handle=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*B);
                FSlateDrawElement::MakeCustomVerts(Out,L,Handle,Vertices,Indices,nullptr,0,0);
                FSlateDrawElement::MakeLines(Out,L+1,Geometry(E.Position,E.Size),Outline,ESlateDrawEffect::None,Color(TEXT("text")),true,1.2f);
            }
        }
        if(E.Type==TEXT("arrow"))
        {
            const float Angle=FMath::DegreesToRadians(E.Value);
            TArray<FVector2D> Points;
            for(const FVector2D P:{FVector2D(0,-10),FVector2D(7,9),FVector2D(0,4),FVector2D(-7,9),FVector2D(0,-10)})
                Points.Add(FVector2D(P.X*FMath::Cos(Angle)-P.Y*FMath::Sin(Angle),P.X*FMath::Sin(Angle)+P.Y*FMath::Cos(Angle))*E.Size/20+E.Size*.5);
            FSlateDrawElement::MakeLines(Out,L,Geometry(E.Position,E.Size),Points,ESlateDrawEffect::None,E.Color,true,2);
        }
        if(E.Type==TEXT("fog")) Box(E.Position,E.Size,FLinearColor(.012f,.015f,.015f,.9f),L);
        if(E.Type==TEXT("connection"))
        {
            TArray<FVector2D> Points={FVector2D::ZeroVector,FVector2D(E.Size.X*.5,E.Size.Y*.4),E.Size};
            FSlateDrawElement::MakeLines(Out,L,Geometry(E.Position,DesignSize),Points,ESlateDrawEffect::None,E.Color,true,1.3f);
        }
        if(E.Type==TEXT("bar"))
        {
            Box(E.Position,E.Size,Color(TEXT("panel")),L); Frame(E.Position,E.Size,Color(TEXT("bronze")),L+1);
            Box(E.Position+FVector2D(1,1),FVector2D(FMath::Max(0.f,float(E.Size.X-2)*FMath::Clamp(E.Value,0.f,1.f)),E.Size.Y-2),E.Color,L+2);
        }
        if(!E.Text.IsEmpty())
        {
            const bool Display=E.FontRole==TEXT("display") || (E.FontRole.IsEmpty() && E.Font>=30);
            FSlateFontInfo Font(Display?DisplayTypeface:Typeface,FMath::RoundToInt(E.Font*.75f)); Font.LetterSpacing=E.Tracking;
            TArray<FString> SourceLines,Lines; E.Text.ParseIntoArrayLines(SourceLines,false);
            const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
            for(FString Line:SourceLines)
            {
                while(E.Type==TEXT("text") && Line.Len()>1 && Measure->Measure(Line,Font).X>E.Size.X)
                {
                    const int32 Count=FMath::Clamp(Measure->FindLastWholeCharacterIndexBeforeOffset(FStringView(Line),Font,E.Size.X)+1,1,Line.Len());
                    Lines.Add(Line.Left(Count)); Line=Line.Mid(Count);
                }
                Lines.Add(Line);
            }
            FVector2D P=E.Position;
            if(E.Type==TEXT("button") || E.Type==TEXT("choice") || E.Type==TEXT("tab") || E.Type==TEXT("notice")) P+=FVector2D(E.TextInset,FMath::Max(0.f,float(E.Size.Y-E.Font*1.3f)*.5f));
            if(E.Type==TEXT("slot") || E.Type==TEXT("node")) P+=FVector2D(FMath::Max(4.,E.Size.X-E.Text.Len()*E.Font*.6-6),E.Size.Y-E.Font*1.3f);
            for(const auto& Line:Lines)
            {
                FVector2D TextPosition=P;
                // Slate rounds glyph advances at the final layout scale, including viewport DPI.
                const float FontScale=Scale*G.GetAccumulatedLayoutTransform().GetScale();
                const float TextWidth=Measure->Measure(Line,Font,FontScale).X/FontScale;
                if(E.Align==TEXT("right")) TextPosition.X+=E.Size.X-TextWidth;
                if(E.Align==TEXT("center") || E.Type==TEXT("keycap")) TextPosition.X+=(E.Size.X-TextWidth)*.5;
                if(E.Type==TEXT("keycap")) TextPosition.Y+=(E.Size.Y-E.Font*1.3f)*.5;
                const auto PG=Geometry(TextPosition,E.Size);
                if(Page==TEXT("hud")) FSlateDrawElement::MakeText(Out,L+2,Geometry(TextPosition+FVector2D(1,1),E.Size),Line,Font,ESlateDrawEffect::None,FLinearColor(0,0,0,.85f));
                FSlateDrawElement::MakeText(Out,L+3,PG,Line,Font,ESlateDrawEffect::None,Ink);
                P.Y+=E.Font*1.6f;
            }
        }
        if(E.MapClipped) Out.PopClip();
    }
    const int32 ContentLayer=Layer+Elements.Num()*4+5;
    if(LayoutEditing)
    {
        const FLinearColor Cyan(.15f,.85f,.95f);
        if(const auto* B=LayoutBounds.Find(LayoutSelection))
        {
            Frame(B->Position,B->Size,Cyan,ContentLayer+1);
            Box(B->Position+B->Size-FVector2D(7,7),FVector2D(14,14),Cyan,ContentLayer+2);
        }
        Box(FVector2D(0,0),FVector2D(1672,76),FLinearColor(.008f,.014f,.02f,.96f),ContentLayer+3);
        FString Caption=TEXT("布局编辑 · ")+Page.ToString()+TEXT("  |  拖动组件 · Alt 单独选图层 · 右下角缩放 · Tab 切换组件\nF10/Esc 退出 · Ctrl+S 保存 · Ctrl+Z 撤销 · Delete 隐藏/显示 · Home 重置此页 · PgUp/PgDn 切页");
        if(!LayoutSelection.IsEmpty())
        {
            const auto* B=LayoutBounds.Find(LayoutSelection);
            if(B) Caption+=FString::Printf(TEXT("\n%s  [%.0f, %.0f, %.0f, %.0f]  %s"),*LayoutSelection,B->Position.X,B->Position.Y,B->Size.X,B->Size.Y,B->Hidden?TEXT("已隐藏"):TEXT(""));
        }
        if(!LayoutStatus.IsEmpty()) Caption+=TEXT("  ")+LayoutStatus;
        FSlateFontInfo Font(Typeface,11);
        FSlateDrawElement::MakeText(Out,ContentLayer+4,Geometry(FVector2D(16,7),FVector2D(1640,65)),Caption,Font,ESlateDrawEffect::None,Cyan);
        return ContentLayer+5;
    }
    // SObjectWidget paints children before NativePaint. Keep the editable field above the illustration.
    if((Page==TEXT("dialogue") || Page==TEXT("memory")) && WidgetTree && WidgetTree->RootWidget)
        return WidgetTree->RootWidget->TakeWidget()->Paint(Args,G,Clip,Out,ContentLayer,Style,ParentEnabled);
    return Super::NativePaint(Args,G,Clip,Out,ContentLayer,Style,ParentEnabled);
}
