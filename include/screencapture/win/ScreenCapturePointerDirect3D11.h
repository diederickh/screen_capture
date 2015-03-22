/*-*-c++-*-*/
/*

  Screen Capture Pointer Drawer
  ------------------------------

  The Mac 'CGDisplayStream' API has a flag to embed the pointer into the
  captured framebuffers; Windows doesn't seem to have this feature so we
  need to draw it manually which is done with this class.

  Some remarks on D3D11 and Matrices. 
  -----------------------------------

  We're using the 'DirectXMath' library that is part of the Win8 SDK.
  The XMMatrix uses row-major order, see the note on ordering in this 
  article https://msdn.microsoft.com/en-us/library/windows/desktop/ff729728(v=vs.85).aspx 
  (see for "Matrix Ordering"). See this post that describes a bit of the 
  confusing about matrix ordering and Direct3D: http://www.catalinzima.com/2012/12/a-word-on-matrices/

  Because DirectXMath is row-order we can either transpose it before
  sending it to the GPU, or use the "row_major" identifier in the shader.
  We're using "row_major" in the shader so we don't need to transpose. 

 */
#ifndef SCREEN_CAPTURE_POINTER_DIRECT3D11_H
#define SCREEN_CAPTURE_POINTER_DIRECT3D11_H

#include <DXGI.h>
#include <D3D11.h>       /* For the D3D11* interfaces. */
#include <D3DCompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;

namespace sc {

  class ScreenCapturePointerConstantBuffer {
  public:
    ScreenCapturePointerConstantBuffer();
  public:
    XMMATRIX pm;                                                     /* The ortho graphic projection matrix. */
    XMMATRIX mm;                                                     /* The model matrix. */
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
  };
  
} /* namespace sc */

#endif
