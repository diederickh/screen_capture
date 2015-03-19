/*-*-c++-*-*/
/*

  Screen Capture Scale and Color Transform for Direct 3D 11.
  ----------------------------------------------------------

  The 'ScreenCaptureDuplicateOutputDirect3D11' class receives frames with a 
  reference to the 2D texture. This class is designed about the idea that we
  take this texture and scale it to a certain destination size and perform 
  color transforms (if necessary). 


 */

#ifndef SCREEN_CAPTURE_SCALE_TRANSFORM_DIRECT3D11_H
#define SCREEN_CAPTURE_SCALE_TRANSFORM_DIRECT3D11_H

#include <stdio.h>
#include <stdlib.h>
#include <DXGI.h>
#include <DXGI1_2.h> /* For IDXGIOutput1 */
#include <D3D11.h>   /* For the D3D11* interfaces. */

namespace sc {

  /* ----------------------------------------------------------- */
  
  class ScaleAndTransformSettingsD3D11 {
  public:
    ScaleAndTransformSettingsD3D11();
    
  public:
    ID3D11Device* device;
    ID3D11DeviceContext* context;
  };

  /* ----------------------------------------------------------- */

  class ScreenCaptureScaleAndColorTransformDirect3D11 {
  public:
    ScreenCaptureScaleAndColorTransformDirect3D11();
    ~ScreenCaptureScaleAndColorTransformDirect3D11();
    int init(ScaleAndTransformSettingsD3D11 cfg);                        /* Initializes the scale/transform features; creates textures etc.. */
    int shutdown();                                                      /* Cleans up; sets the device and context members to NULL. */
    
  public:
    ID3D11Device* device;                                                /* Not owned by this class. The device that we use to create graphics objects. */
    ID3D11DeviceContext* context;                                        /* Not owned by this class. The context that we use to render. */
    ID3D11Texture2D* tex;                                                /* The texture into which the transformed result will be written. */
    ID3D11RenderTargetView* target_view;                                 /* The output render target. */
    ID3D11SamplerState* sampler;                                         /* The sampler for sampling the source texture and scaling it into the dest texture. */
  }; 

  /* ----------------------------------------------------------- */
  
} /* namespace sc */

#endif
