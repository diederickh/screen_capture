#include <string>
#include <stdio.h>
#include <screencapture/win/ScreenCapturePointerDirect3D11.h>
#include <screencapture/win/ScreenCaptureUtilsDirect3D11.h>
#include <screencapture/Utils.h>

static const std::string D3D11_POINTER_SHADER = ""
  "Texture2D tex_source: register(t0);\n"
  "SamplerState sam_linear: register(s0);\n"
  "\n"
  "cbuffer Projection : register(b0)  {\n"
  "  row_major matrix pm; \n"
  "};\n"
  "cbuffer ModelMatrix : register(b1) {\n"
  "  row_major matrix mm; \n"
  "};\n"
  "\n"
  "struct VertexInput {\n"
  "  float4 pos: POSITION;\n"
  "  float2 tex: TEXCOORD0;\n"
  "};\n"
  "\n"
  "struct PixelInput {\n"
  "  float4 pos: SV_POSITION;\n"
  "  float2 tex: TEXCOORD0;\n"
  "};\n"
  "\n"
  "PixelInput vertex_shader(VertexInput vertex) {\n"
  "  PixelInput output = (PixelInput) 0;\n"
  "  output.pos = mul(vertex.pos, mm);\n"
  "  output.pos = mul(output.pos, pm);\n"
  "  output.tex = vertex.tex;\n"
  "  return output;\n"
  "}\n"
  "\n"
  "float4 pixel_shader(PixelInput input) : SV_Target {\n"
  "  return tex_source.Sample(sam_linear, input.tex);\n"
  "}\n";

namespace sc {

  /* ----------------------------------------------------------- */

  ScreenCapturePointerTextureDirect3D11::ScreenCapturePointerTextureDirect3D11(ID3D11Device* device, ID3D11DeviceContext* context)
    :width(0)
    ,height(0)
    ,device(device)
    ,context(context)
    ,texture(NULL)
    ,view(NULL)
  {
  }
  
  ScreenCapturePointerTextureDirect3D11::~ScreenCapturePointerTextureDirect3D11() {

    COM_RELEASE(view);
    COM_RELEASE(texture);
    
    width = 0;
    height = 0;
    device = NULL;
    context = NULL;
  }

  int ScreenCapturePointerTextureDirect3D11::updatePixels(int w, int h, uint8_t* pixels) {

    HRESULT hr = S_OK;
    
    if (0 >= w) {
      printf("Error: the given width for the pointer texture is 0.\n");
      return -1;
    }

    if (0 >= h) {
      printf("Error: the given height for the pointer texture is 0.\n");
      return -2;
    }

    if (0 != width && w != width) {
      printf("Error: cannot update the pointer texture; the width doesn't match the texture width.\n");
      return -3;
    }

    if (0 != height && h != height) {
      printf("Error: cannot update the pointer texture; the height doesn't match the texture height.\n");
      return -4;
    }

    if (NULL == device) {
      printf("Error: cannot update the pointer texture; the D3D11 Device member is NULL.\n");
      return -5;
    }

    if (NULL == context) {
      printf("Error: cannot update the pointer texture; the D3D11 Device Context member is NULL.\n");
      return -6;
    }

    /* Create the texture when we didn't create it yet. */
    if (NULL == texture) {

      D3D11_SUBRESOURCE_DATA data;
      ZeroMemory(&data, sizeof(data));
      data.pSysMem = pixels;
      data.SysMemPitch = 4 * w;
      data.SysMemSlicePitch = 0;
      
      D3D11_TEXTURE2D_DESC desc;
      ZeroMemory(&desc, sizeof(desc));
      desc.Width = w;
      desc.Height = h;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;                               /* This is the default data when using desktop duplication, see https://msdn.microsoft.com/en-us/library/windows/desktop/hh404611(v=vs.85).aspx */
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_DYNAMIC;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; 
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; 
      desc.MiscFlags = 0;

      hr = device->CreateTexture2D(&desc, &data, &texture);
      if (S_OK != hr) {
        printf("Error: failed to create the pointer texture.\n");
        return -5;
      }

      hr = device->CreateShaderResourceView(texture, NULL, &view);
      if (S_OK != hr) {
        printf("Error: failed to create the pointer shader resource view.\n");
        COM_RELEASE(texture);
        return -6;
      }
    }
    else {
      
      D3D11_MAPPED_SUBRESOURCE map;
      hr = context->Map(texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
      if (S_OK != hr) {
        printf("Error: failed to map the pointer texture.\n");
        return -7;
      }

      memcpy(map.pData, pixels, w * h * 4);
      context->Unmap(texture, 0);
    }

    width = w;
    height = h;

    return 0;
  }

  /* ----------------------------------------------------------- */
  
  ScreenCapturePointerDirect3D11::ScreenCapturePointerDirect3D11()
    :device(NULL)
    ,context(NULL)
    ,pointer_tex(NULL)
    ,pointer_view(NULL)
    ,input_layout(NULL)
    ,vertex_buffer(NULL)
    ,conb_pm(NULL)
    ,conb_mm(NULL)
    ,ps(NULL)
    ,vs(NULL)
    ,sx(1.0f)
    ,sy(1.0f)
    ,vp_width(0.0f)
    ,vp_height(0.0f)
    ,curr_pointer(NULL)
  {
  }

  ScreenCapturePointerDirect3D11::~ScreenCapturePointerDirect3D11() {

    shutdown();
  }

  int ScreenCapturePointerDirect3D11::init(ID3D11Device* dev, ID3D11DeviceContext* ctx) {

    if (NULL != device || NULL != context) {
      printf("Error: trying to initialize the pointer drawer but we're already initialized, call shutdown() first.\n");
      return -1;
    }
    
    if (NULL == dev) {
      printf("Error: failed to initailize the pointer drawer because device is NULL.\n");
      return -2;
    }

    if (NULL == ctx) {
      printf("Error: failed to initialize the pointer drawer because context is NULL.\n");
      return -3;
    }

    device = dev;
    context = ctx;

    if (0 != createShadersAndBuffers()) {
      shutdown();
      return -4;
    }
    
    return 0;
  }
  
  int ScreenCapturePointerDirect3D11::createShadersAndBuffers() {

    HRESULT hr = S_OK;
    ID3DBlob* vs_blob = NULL;
    ID3DBlob* vs_err = NULL;
    ID3DBlob* ps_blob = NULL;
    ID3DBlob* ps_err = NULL;

#if !defined(NDEBUG)    
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION; /* @todo remove the debug flag. */
#else
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3; 
#endif

    D3D11_INPUT_ELEMENT_DESC layout[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    UINT num_els = ARRAYSIZE(layout);
    std::string err_msg;

    if (NULL == device) {
      printf("Error: failed to create the shader/buffers because the device member is NULL.\n");
      return -1;
    }

    if (NULL == context) {
      printf("Error: failed to create the shader/buffers because the context member is NULL.\n");
      return -2;
    }

    if (NULL != ps) {
      printf("Error: requested to create the shaders, but we already created the ps shader.\n");
      return -3;
    }

    if (NULL != vs) {
      printf("Error: requested to create the shaders, but we already created the vs shader.\n");
      return -4;
    }

    /* Create the pointer vertex shader. */
    hr = D3DCompile(D3D11_POINTER_SHADER.c_str(), D3D11_POINTER_SHADER.size(), NULL, NULL, NULL, "vertex_shader", "vs_4_0", flags, 0, &vs_blob, &vs_err);
    if(S_OK != hr) {
      printf("Error: failed to create the pointer vertex shader.\n");
      if (NULL != vs_err) {
        err_msg.assign((char*)vs_err->GetBufferPointer(), vs_err->GetBufferSize());
        printf("Error: %s\n", err_msg.c_str());
        goto error;
      }
    }

    hr = device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &vs);
    if (S_OK != hr) {
      printf("Error: failed to create the vertex shader for the cursor pointer, after we compiled it.\n");
      goto error;
    }

    /* Create the pointer pixel shader. */
    hr = D3DCompile(D3D11_POINTER_SHADER.c_str(), D3D11_POINTER_SHADER.size(), NULL, NULL, NULL, "pixel_shader", "ps_4_0", flags, 0, &ps_blob, &ps_err);
    if(S_OK != hr) {
      printf("Error: failed to create the pointer pixel shader.\n");
      if (NULL != ps_err) {
        err_msg.assign((char*)ps_err->GetBufferPointer(), ps_err->GetBufferSize());
        printf("Error: %s\n", err_msg.c_str());
        goto error; 
      }
    }

    hr = device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &ps);
    if (S_OK != hr) {
      printf("Error: failed to create the pixel shader for the cursor pointer, after we compiled it.\n");
      goto error;
    }

    /* Create the input layout. */
    hr = device->CreateInputLayout(layout, num_els, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &input_layout);
    if (S_OK != hr) {
      printf("Error: failed to create the input layout for the pointer drawer.\n");
      goto error;
    }

    /* Create vertex buffer. */
    if (0 != createVertexBuffer()) {
      printf("Error: failed to create the vertex buffer.\n");
      goto error;
    }

    if (0 != createConstantBuffer()) {
      printf("Error: failed to create the constant buffer.\n");
      goto error;
    }
    
    COM_RELEASE(ps_blob);
    COM_RELEASE(vs_blob);
    
    return 0;

  error:
    COM_RELEASE(ps_blob);
    COM_RELEASE(ps_err);
    COM_RELEASE(vs_blob);
    COM_RELEASE(vs_err);
    COM_RELEASE(ps);
    COM_RELEASE(vs);
    COM_RELEASE(input_layout);
    return -666;
  }

  int ScreenCapturePointerDirect3D11::createVertexBuffer() {

    HRESULT hr = S_OK;

    if (NULL == device) {
      printf("Error: trying to create the vertex buffer but the D3D11 Device is NULL.\n");
      return -1;
    }

    if (NULL != vertex_buffer) {
      printf("Error: requested to create the vertex buffer but its already created. Call shutdown() first.\n");
      return -2;
    }
    
    /* Create the vertex buffer. */
    float vertices[] = {
      0.0f, 0.0f, 0.0f,  0.0f, 1.0f,    /* bottom left */
      0.0f, 1.0f, 0.0f,  0.0f, 0.0f,    /* top left */
      1.0f, 0.0f, 0.0f,  1.0f, 1.0f,    /* bottom right */
      1.0f, 1.0f, 0.0f,  1.0f, 0.0f     /* top right */
    };

    int num_floats = 20;

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.ByteWidth = sizeof(float) * num_floats;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA init_data;
    ZeroMemory(&init_data, sizeof(init_data));
    init_data.pSysMem = vertices;
    init_data.SysMemPitch = 0;
    init_data.SysMemSlicePitch = 0;

    hr = device->CreateBuffer(&bd, &init_data, &vertex_buffer);
    if (S_OK != hr) {
      printf("Error: Failed to create the vertex buffer.\n");
      return -2;
    }

    return 0;
  }

  int ScreenCapturePointerDirect3D11::createConstantBuffer() {
    
    if (NULL == device) {
      printf("Error: trying to create the constant buffer for the pointer drawer but the device is NULL.\n");
      return -1;
    }

    if (NULL != conb_pm) {
      printf("Error: requested to create the constant buffer for the pm, but it's already created.\n");
      return -2;
    }
    
    if (NULL != conb_mm) {
      printf("Error: requested to create the constant buffer for the mm, but it's already created.\n");
      return -3;
    }

    /* Create constant buffer for projection matrix. */
    {
      D3D11_BUFFER_DESC desc;
      ZeroMemory(&desc, sizeof(desc));

      desc.ByteWidth = sizeof(XMMATRIX);
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags = 0;
      desc.MiscFlags = 0;
      desc.StructureByteStride = 0;

      HRESULT hr;
      hr = device->CreateBuffer(&desc, NULL, &conb_pm);
      if (S_OK != hr) {
        printf("Error: failed to create the constant buffer for the projection matrix.\n");
        return -3;
      }
    }

    /* Create constant buffer for model matrix. */
    {
      D3D11_BUFFER_DESC desc;
      ZeroMemory(&desc, sizeof(desc));

      desc.ByteWidth = sizeof(XMMATRIX);
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags = 0;
      desc.MiscFlags = 0;
      desc.StructureByteStride = 0;

      HRESULT hr;
      hr = device->CreateBuffer(&desc, NULL, &conb_mm);
      if (S_OK != hr) {
        printf("Error: failed to create the constant buffer for the projection matrix.\n");
        return -3;
      }
    }
    
    return 0;
  }

  void ScreenCapturePointerDirect3D11::setViewport(D3D11_VIEWPORT vp) {

    if (NULL == context) {
      printf("Error: cannot set the viewport and update the projection matrix because the context is NULL.\n");
      exit(EXIT_FAILURE);
    }

    if (NULL == conb_pm) {
      printf("Error: cannot set the projection matrix because the constant buffer for it is NULL.\n");
      exit(EXIT_FAILURE);
    }

    if (NULL == conb_mm) {
      printf("Error: cannot set the model matrix because the constant buffer for it is NULL.\n");
      exit(EXIT_FAILURE);
    }

    vp_width = vp.Width;
    vp_height = vp.Height;

    /* 
       Placing the matrix on the stack so it's 16bit aligned which is necessary: 
       https://msdn.microsoft.com/en-us/library/windows/desktop/ee418725(v=vs.85).aspx 
    */
    XMMATRIX m = XMMatrixOrthographicOffCenterLH(0.0f, (float)vp.Width, 0.0f, (float)vp.Height, 0.0f, 100.0f);
    context->UpdateSubresource(conb_pm, 0, NULL, &m, 0, 0);
    
    m = XMMatrixScaling(32.0f * sx , 32.0f * sy, 1.0f) * XMMatrixTranslation(100.0f, 50.0, 0.0) ;
    context->UpdateSubresource(conb_mm, 0, NULL, &m, 0, 0);
  }

  void ScreenCapturePointerDirect3D11::updatePointerPosition(float x, float y) {

#if !defined(NDEBUG)
    if (NULL == conb_mm) {
      printf("Error: failed to update the pointer position; the constant buffer is NULL.\n");
      return;
    }
    
    if (NULL == context) {
      printf("Error: failed to update the pointer position; the context is NULL.\n");
      return;
    }
#endif
    
    if (NULL == curr_pointer) {
      return;
    }
    
    /* 
       Placing the matrix on the stack so it's 16bit aligned which is necessary: 
       https://msdn.microsoft.com/en-us/library/windows/desktop/ee418725(v=vs.85).aspx 
    */
    XMMATRIX m = XMMatrixScaling(curr_pointer->width * sx, curr_pointer->height * sy, 1.0f) * XMMatrixTranslation((int)(sx * x), (int)(vp_height -(y * sy) -(sy * curr_pointer->height)) , 0.0) ;
    context->UpdateSubresource(conb_mm, 0, NULL, &m, 0, 0);
  }

  void ScreenCapturePointerDirect3D11::draw() {

#if !defined(NDEBUG)
      
    if (NULL == conb_pm) {
      printf("Error: conb_mm is NULL.\n");
      return;
    }
    
    if (NULL == conb_mm) {
      printf("Error: conb_mm is NULL.\n");
      return;
    }

    if (NULL == vertex_buffer) {
      printf("Error: vertex buffer is NULL.\n");
      return;
    }
    
    if (NULL == input_layout) {
      printf("Error: input layout is NULL.\n");
      return;
    }
    
    if (NULL == ps) {
      printf("Error: ps is NULL.\n");
      return;
    }
    
    if (NULL == vs) {
      printf("Error: vs is NULL.\n");
      return;
    }
#endif
    
    if (NULL != curr_pointer) {
      context->PSSetShaderResources(0, 1, &curr_pointer->view);
    }
     
    /* Set vertex layout and buffer. */
    UINT stride = sizeof(float) * 5; 
    UINT offset = 0;

    context->IASetInputLayout(input_layout);
    context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
    context->VSSetConstantBuffers(0, 1, &conb_pm);
    context->VSSetConstantBuffers(1, 1, &conb_mm);
    context->VSSetShader(vs, NULL, 0);
    context->PSSetShader(ps, NULL, 0);
    context->Draw(4, 0);
  }

  int ScreenCapturePointerDirect3D11::updatePointerPixels(int w, int h, uint8_t* pixels) {
    
    if (0 >= w) {
      printf("Error: cannot update the pointer pixels because the width is invalid: %d\n", w);
      return -1;
    }

    if (0 >= h) {
      printf("Error: cannot update the pointer pixels because the height is invalid: %d\n", h);
      return -2;
    }

    if (NULL == pixels) {
      printf("Error: cannot update the pointer pixels because the given pixels are NULL.\n");
      return -3;
    }

    /* Find the texture. */
    ScreenCapturePointerTextureDirect3D11* tex = NULL;
    for (size_t i = 0; i < pointers.size(); ++i) {
      tex = pointers[i];
      if (tex->width == w && tex->height == h) {
        break;
      }
      tex = NULL;
    }

    if (NULL == tex) {
      printf("Info: creating a new pointer texture.\n");
      tex = new ScreenCapturePointerTextureDirect3D11(device, context);
      pointers.push_back(tex);
    }

    if (0 != tex->updatePixels(w, h, pixels)) {
      printf("Error: failed to update the pointer pixels.\n");
      return -4;
    }

    curr_pointer = tex;
    
    return 0;
  }

  int ScreenCapturePointerDirect3D11::shutdown() {

    device = NULL;
    context = NULL;
    curr_pointer = NULL;

    for (size_t i = 0; i < pointers.size(); ++i) {
      delete pointers[i];
      pointers[i] = NULL;
    }
    pointers.clear();

    COM_RELEASE(pointer_tex);
    COM_RELEASE(pointer_view);
    COM_RELEASE(input_layout);
    COM_RELEASE(vertex_buffer);
    COM_RELEASE(conb_pm);
    COM_RELEASE(conb_mm);
    COM_RELEASE(ps);
    COM_RELEASE(vs);
    return 0;
  }
} /* namespace sc */
