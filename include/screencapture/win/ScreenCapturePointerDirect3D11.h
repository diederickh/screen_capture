#ifndef SCREEN_CAPTURE_POINTER_DIRECT3D11_H
#define SCREEN_CAPTURE_POINTER_DIRECT3D11_H

#include <DXGI.h>
//#include <DXGI1_2.h>     /* For IDXGIOutput1 */
#include <D3D11.h>       /* For the D3D11* interfaces. */
#include <D3DCompiler.h>

namespace sc {

  class ScreenCapturePointerConstantBuffer {
  public:
    ScreenCapturePointerConstantBuffer();
  public:
    float pm[16];
    float mm[16];
  };

  class ScreenCapturePointerDirect3D11 {
  public:
    ScreenCapturePointerDirect3D11();
    ~ScreenCapturePointerDirect3D11(); 
    int init(ID3D11Device* device, ID3D11DeviceContext* context);    /* Creates the necessary D3D11 objects that we need to render a pointer. */
    int shutdown();
    void draw();
    void setViewport(D3D11_VIEWPORT vp);                             /* Must be called whenever the viewport changes so we can update the orthographic matrix. */
    
  private:
    int createShadersAndBuffers();                                   /* Creates the VS and PS shader + vertex buffer, layout. */
    int createVertexBuffer();                                        /* Creates the vertex buffer. */
    int createPointerTexture();                                      /* Creates the texture for the pointer. */
    int createConstantBuffer();

  public:
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11Texture2D* pointer_tex;
    ID3D11ShaderResourceView* pointer_view;
    ID3D11InputLayout* input_layout;
    ID3D11Buffer* vertex_buffer;
    ID3D11Buffer* constant_buffer;
    ID3D11PixelShader* ps;
    ID3D11VertexShader* vs;
    ScreenCapturePointerConstantBuffer cbuffer;
    //    float pm[16];                                                   /* The orthographic projection matrix. */
    //    float mm[16];                                                   /* The model matrix. */
  };
  
} /* namespace sc */

#endif
