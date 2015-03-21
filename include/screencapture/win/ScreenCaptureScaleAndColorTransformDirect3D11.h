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
#include <string>
#include <stdint.h>
#include <DXGI.h>
#include <DXGI1_2.h>     /* For IDXGIOutput1 */
#include <D3D11.h>       /* For the D3D11* interfaces. */
#include <D3DCompiler.h>
#include <screencapture/win/ScreenCaptureUtilsDirect3D11.h>

namespace sc {

  typedef void(*scale_color_callback)(uint8_t* pixels, int width, int height, void* user);

  /* ----------------------------------------------------------- */
  
  class ScaleColorTransformSettingsD3D11 {
  public:
    ScaleColorTransformSettingsD3D11();
    
  public:
    ID3D11Device* device;                                                /* Pointer to the D3D11 Device. */
    ID3D11DeviceContext* context;                                        /* Pointer to the D3D11 Device Context. */
    int output_width;                                                    /* The width of the resulting output texture when calling scale(). */
    int output_height;                                                   /* The height of the resulting output texture when calling scale(). */
    scale_color_callback cb_scaled;                                      /* Gets called when we've scaled a frame. */
    void* cb_user;                                                       /* Gets passed into cb_scaled. */
  };

  /* ----------------------------------------------------------- */

  class ScreenCaptureScaleAndColorTransformDirect3D11 {
  public:
    ScreenCaptureScaleAndColorTransformDirect3D11();
    ~ScreenCaptureScaleAndColorTransformDirect3D11();
    int init(ScaleColorTransformSettingsD3D11 cfg);                      /* Initializes the scale/transform features; creates textures etc.. */
    int shutdown();                                                      /* Cleans up; sets the device and context members to NULL. */
    int scale(ID3D11Texture2D* tex);                                     /* Scale the given texture to the output_width and output_height of the settings passed into init(). */
    int isInit();                                                        /* Returns 0 when we're initialized. */
    
  private:
    HRESULT createVertexBuffer();                                        /* Creates the full screen vertex buffer to render the texture. */
    HRESULT createDestinationObjects();                                  /* Creates the necessary D3D11 objects so we can render a source texture into a offscreen destination target.  */
    
  public:
    int is_init;                                                         /* Used to check if we're initialised. When initialized it's set to 0, otherwise < 0. */ 
    ID3D11Device* device;                                                /* Not owned by this class. The device that we use to create graphics objects. */
    ID3D11DeviceContext* context;                                        /* Not owned by this class. The context that we use to render. */
    ID3D11Texture2D* dest_tex;                                           /* The texture into which the transformed result will be written. */
    ID3D11Texture2D* staging_tex;                                        /* It seems that we can't do GPU > CPU transfers for texture which are a RENDER_TARGET. We need a STAGING one. @todo check what's the fastest solution to download texture data from gpu > cpu */
    ID3D11ShaderResourceView* src_tex_view;                              /* The shader resource view for the destination texture. This represents the texture of the desktop.*/
    ID3D11RenderTargetView* dest_target_view;                            /* The output render target. */
    ID3D11SamplerState* sampler;                                         /* The sampler for sampling the source texture and scaling it into the dest texture. */
    ID3D11PixelShader* ps_scale;                                         /* Mostly a pass though shader used to scale. */ 
    ID3D11VertexShader* vs_scale;                                        /* Vertex shader, used to scale. */
    ID3D11InputLayout* input_layout;                                     /* The input layout that defines our vertex buffer. */
    ID3D11Buffer* vertex_buffer;                                         /* The Position + Texcoord vertex buffer. */
    ScaleColorTransformSettingsD3D11 settings;                           /* We copy the settings that are passed into init(). */   
  }; 

  /* ----------------------------------------------------------- */

  inline int ScreenCaptureScaleAndColorTransformDirect3D11::isInit() {
    return is_init;
  }
  
} /* namespace sc */

#endif
