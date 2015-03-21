#include <string>
#include <stdio.h>
#include <screencapture/win/ScreenCapturePointerDirect3D11.h>
#include <screencapture/win/ScreenCaptureUtilsDirect3D11.h>
#include <screencapture/Utils.h>

static const std::string D3D11_POINTER_SHADER = ""
  "Texture2D tex_source: register(t0);\n"
  "SamplerState sam_linear: register(s0);\n"
  "\n"
  "cbuffer Cam : register(b0)  {\n"
  "  matrix pm; \n"
  "  matrix mm; \n"
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
  "  matrix m = mul(mm, pm);\n"
  "  output.pos = mul(vertex.pos, mm);\n"
  "  output.pos = mul(output.pos, pm);\n"

  "  output.tex = vertex.tex;\n"
  "  return output;\n"
  "}\n"
  "\n"
  "float4 pixel_shader(PixelInput input) : SV_Target {\n"
  "  return float4(input.tex.x, input.tex.y, 0.0, 1.0);\n"
  "  return tex_source.Sample(sam_linear, input.tex);\n"
  "}\n";

namespace sc {

  ScreenCapturePointerConstantBuffer::ScreenCapturePointerConstantBuffer() {
    memset(pm, 0x00, sizeof(pm));
    memset(mm, 0x00, sizeof(mm));
  }
  
  ScreenCapturePointerDirect3D11::ScreenCapturePointerDirect3D11()
    :device(NULL)
    ,context(NULL)
    ,pointer_tex(NULL)
    ,pointer_view(NULL)
    ,input_layout(NULL)
    ,vertex_buffer(NULL)
    ,constant_buffer(NULL)
    ,ps(NULL)
    ,vs(NULL)
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
#if 0    
    float vertices[] = {
      -0.5f, -0.5f, 0.0f,  0.0f, 1.0f,   /* bottom left */
      -0.5f,  0.5f, 0.0f,  0.0f, 0.0f,   /* top left */
      0.5f, -0.5f, 0.0f,  1.0f, 1.0f,    /* bottom right */
      0.5f,  0.5f, 0.0f,  1.0f, 0.0f     /* top right */
    };
#else
    float vertices[] = {
      0.0f, 0.0f, 0.0f,  0.0f, 1.0f,   /* bottom left */
      0.0f, 1.0f, 0.0f,  0.0f, 0.0f,   /* top left */
      1.0f, 0.0f, 0.0f,  1.0f, 1.0f,    /* bottom right */
      1.0f, 1.0f, 0.0f,  1.0f, 0.0f     /* top right */
    };

#endif

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

    if (NULL != constant_buffer) {
      printf("Error: requested to create the constant buffer but it's already created; call shutdown() first.\n");
      return -2;
    }

    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.ByteWidth = sizeof(cbuffer);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA init_data;
    ZeroMemory(&init_data, sizeof(init_data));
    
    init_data.pSysMem = &cbuffer;
    init_data.SysMemPitch = 0;
    init_data.SysMemSlicePitch = 0;

    HRESULT hr;
    hr = device->CreateBuffer(&desc, &init_data, &constant_buffer);
    if (S_OK != hr) {
      printf("Error: failed to create the constant buffer for the pointer drawer.\n");
      return -3;
    }
    
    return 0;
  }

  void ScreenCapturePointerDirect3D11::setViewport(D3D11_VIEWPORT vp) {
    
    if (NULL == constant_buffer) {
      printf("Error: cannot set the viewport and update the projection matrix because the constant buffer is NULL in the pointer drawer.\n");
      exit(EXIT_FAILURE);
    }
    if (NULL == context) {
      printf("Error: cannot set the viewport and update the projection matrix because the context is NULL.\n");
      exit(EXIT_FAILURE);
    }
    
    D3D11_MAPPED_SUBRESOURCE map;
    HRESULT hr;

    hr = context->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    if (S_OK != hr) {
      printf("Error: failed to map the constant buffer that we use to draw the pointer.\n");
      exit(EXIT_FAILURE);
    }

    ScreenCapturePointerConstantBuffer* buffer_data = static_cast<ScreenCapturePointerConstantBuffer*>(map.pData);
    if (NULL == buffer_data) {
      printf("Error: failed to cast the mapped constant buffer to our internal representation. Not supposed to happen.\n");
      context->Unmap(constant_buffer, 0);
      return;
    }

    printf("============= CURRENTLY ========================\n");
    print_matrix(buffer_data->pm);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

    create_identity_matrix(buffer_data->mm);
    buffer_data->mm[0] = 100.25;
    buffer_data->mm[5] = 100.5;
    //    buffer_data->mm[10] = 1.0f;
    //    buffer_data->mm[14] = 1.0f;

    create_ortho_matrix(0, vp.Width, vp.Height, 0.0f,-0.1f, 100.0f, buffer_data->pm);

    float* p = buffer_data->pm;
    p[2] = -p[2];
    p[6] = -p[6];
    p[10] = -p[10];
    p[14] = -p[14];

    p = buffer_data->mm;
    p[2] = -p[2];
    p[6] = -p[6];
    p[8] = -p[8];
    p[9] = -p[9];
    p[11] = -p[11];

    
    printf("Model Matrix.\n");
    print_matrix(buffer_data->mm);
    printf("Projection Matrix (ortho).\n");
    print_matrix(buffer_data->pm);

    printf("Updated: %f, %f\n", vp.Width, vp.Height);
    context->Unmap(constant_buffer, 0);
  }

  void ScreenCapturePointerDirect3D11::draw() {

    if (NULL == constant_buffer) {
      printf("Error: constant buffer is NULL.\n");
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
     
    /* Set vertex layout and buffer. */
    UINT stride = sizeof(float) * 5; 
    UINT offset = 0;
    
    context->IASetInputLayout(input_layout);
    context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
    context->VSSetConstantBuffers(0, 1, &constant_buffer);
    context->VSSetShader(vs, NULL, 0);
    context->PSSetShader(ps, NULL, 0);
    context->Draw(4, 0);
  }
  
  
  int ScreenCapturePointerDirect3D11::createPointerTexture() {
    return 0;
  }

  int ScreenCapturePointerDirect3D11::shutdown() {

    device = NULL;
    context = NULL;

    COM_RELEASE(pointer_tex);
    COM_RELEASE(pointer_view);
    COM_RELEASE(input_layout);
    COM_RELEASE(vertex_buffer);
    COM_RELEASE(constant_buffer);
    COM_RELEASE(ps);
    COM_RELEASE(vs);

    //memset(pm, 0x00, sizeof(pm));
    //memset(mm, 0x00, sizeof(mm));
    
    return 0;
  }

  
} /* namespace sc */
