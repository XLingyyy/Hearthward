#include "HearthwardScreenWidget.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool UHearthwardScreenWidget::CaptureUI(const FString& Name,int32 Width,int32 Height)
{
#if UE_BUILD_SHIPPING
    return false;
#else
    const TGuardValue<int32> NeutralHover(Hover,INDEX_NONE),NeutralKeyboardFocus(KeyboardFocus,INDEX_NONE);
    Refresh();
    FWidgetRenderer Renderer(false,true);
    UTextureRenderTarget2D* Target=NewObject<UTextureRenderTarget2D>();
    Target->ClearColor=FLinearColor::Transparent;
    Target->InitCustomFormat(Width,Height,PF_FloatRGBA,true);
    Target->UpdateResourceImmediate(true);
    Renderer.DrawWidget(Target,TakeWidget(),FVector2D(Width,Height),0,false);
    TArray<FColor> Pixels;
    TArray<FLinearColor> LinearPixels;
    FReadSurfaceDataFlags Flags(RCM_MinMax);
    Flags.SetLinearToGamma(false);
    const bool Read=Target->GameThread_GetRenderTargetResource()->ReadLinearColorPixels(LinearPixels,Flags);
    if(!Read) return false;
    // The offscreen renderer writes linear values. PNG viewers expect sRGB.
    Pixels.Reserve(LinearPixels.Num());
    for(const FLinearColor& Pixel:LinearPixels) Pixels.Add(Pixel.ToFColorSRGB());
    TArray64<uint8> PNG;
    FImageUtils::PNGCompressImageArray(Width,Height,Pixels,PNG);
    return FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()/TEXT("Task020")/(FPaths::MakeValidFileName(Name)+TEXT(".png"))));
#endif
}
