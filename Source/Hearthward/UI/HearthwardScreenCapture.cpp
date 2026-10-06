#include "HearthwardScreenWidget.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool UHearthwardScreenWidget::CaptureUI(const FString& Name,int32 Width,int32 Height,bool PreserveFocus)
{
#if UE_BUILD_SHIPPING
    return false;
#else
    const bool Captured=[&]()
    {
    const double ScreenScale=FMath::Min(double(Width)/DesignSize.X,double(Height)/DesignSize.Y);
    const TGuardValue<FVector2D> PreserveView(MapViewDesignSize,FVector2D(Width,Height)/ScreenScale);
    const TGuardValue<FVector2D> PreservePan(MapPan,MapPan);
    const TGuardValue<float> PreserveZoom(MapZoom,MapZoom);
    const TGuardValue<int32> NeutralHover(Hover,PreserveFocus?Hover:INDEX_NONE),NeutralKeyboardFocus(KeyboardFocus,PreserveFocus?KeyboardFocus:INDEX_NONE);
    const TGuardValue<bool> NeutralInput(KeyboardNavigationActive,PreserveFocus && KeyboardNavigationActive);
    Refresh();
    if(Page==TEXT("map"))FFileHelper::SaveStringToFile(DescribeHUDPreview(),*(FPaths::ProjectSavedDir()/TEXT("Task020")/(FPaths::MakeValidFileName(Name)+TEXT(".json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
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
    }();
    Refresh();return Captured;
#endif
}

#if !UE_BUILD_SHIPPING
bool UHearthwardScreenWidget::CaptureMapVisibilityPair(const FString& Name,int32 Width,int32 Height)
{
    const TGuardValue<bool> RestoreProbe(MapTerrainProbe,false);
    const bool Base=CaptureUI(Name+TEXT("-base"),Width,Height);
    MapTerrainProbe=true;
    const bool Probe=CaptureUI(Name+TEXT("-probe"),Width,Height);
    MapTerrainProbe=false;Refresh();
    return Base && Probe;
}
#endif

#include "HearthwardScreenFocusTest.inl"
#include "HearthwardScreenHUDTest.inl"
#include "HearthwardScreenInventoryTest.inl"
#include "HearthwardScreenSkillsTest.inl"
#include "HearthwardScreenJournalTest.inl"
#include "HearthwardScreenInputTest.inl"
#include "HearthwardScreenFeedbackTest.inl"

#include "HearthwardScreenQuickTest.inl"
#include "HearthwardScreenDragTest.inl"
#include "HearthwardScreenMapTest.inl"
#include "HearthwardScreenTravelTest.inl"

#include "HearthwardScreenStorageTest.inl"
