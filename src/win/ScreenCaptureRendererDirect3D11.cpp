#include <screencapture/win/ScreenCaptureRendererDirect3D11.h>
#include <screencapture/win/ScreenCaptureUtilsDirect3D11.h>
#include <algorithm>

static const std::string D3D11_SCALE_SHADER = ""
  "Texture2D tex_source: register(t0);\n"
  "SamplerState sam_linear: register(s0);\n"
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
  "  output.pos = vertex.pos;\n"
  "  output.tex = vertex.tex;\n"
  "  return output;\n"
  "}\n"
  "\n"
  "float4 pixel_shader(PixelInput input) : SV_Target {\n"
  "  float4 pix = tex_source.Sample(sam_linear, input.tex);\n"
  "  pix.a = 1.0;\n"
  "  return pix;\n;"
  "}\n";

namespace sc {

  /* ----------------------------------------------------------- */

  ScreenCaptureRendererSettingsDirect3D11::ScreenCaptureRendererSettingsDirect3D11()
    :device(NULL)
    ,context(NULL)
    ,output_width(0)
    ,output_height(0)
    ,cb_scaled(NULL)
    ,cb_user(NULL)
  {
  }
  
  /* ----------------------------------------------------------- */

  ScreenCaptureRendererDirect3D11::ScreenCaptureRendererDirect3D11()
    :is_init(-1)
    ,device(NULL)
    ,context(NULL)
    ,src_tex_view(NULL)
    ,dest_tex(NULL)
    ,staging_tex(NULL)
    ,dest_target_view(NULL)
    ,sampler(NULL)
    ,vs_scale(NULL)
    ,ps_scale(NULL)
    ,input_layout(NULL)
    ,vertex_buffer(NULL)
    ,blend_state(NULL)
  {

  }

  ScreenCaptureRendererDirect3D11::~ScreenCaptureRendererDirect3D11() {
    shutdown();
  }

  int ScreenCaptureRendererDirect3D11::init(ScreenCaptureRendererSettingsDirect3D11 cfg) {

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

    /* ----------------------------------------------------------- */
    
    if (NULL == cfg.device) {
      printf("Error: the given D3D device is NULL.\n");
      return -1;
    }

    if (NULL == cfg.context) {
      printf("Error: the given D3D context is NULL\n");
      return -2;
    }

    if (0 >= cfg.output_width) {
      printf("Error: the given output width is: %d.\n", cfg.output_width);
      return -3;
    }

    if (0 >= cfg.output_height) {
      printf("Error: the given output height is: %d.\n", cfg.output_height);
      return -4;
    }

    if (NULL == cfg.cb_scaled) {
      printf("Error: no cb_scaled callback set.\n");
      return -5;
    }

    if (0 != pointer.init(cfg.device, cfg.context)) {
      printf("Error: failed to initialize the pointer drawer.\n");
      return -6;
    }

    settings = cfg;
    device = cfg.device;
    context = cfg.context;

    /* Create the scale vertex shader */
    {
      hr = D3DCompile(D3D11_SCALE_SHADER.c_str(),
                      D3D11_SCALE_SHADER.size(),
                      NULL,
                      NULL,
                      NULL,
                      "vertex_shader",
                      "vs_4_0",
                      flags,
                      0,
                      &vs_blob,
                      &vs_err);

      if (S_OK != hr) {
        printf("Error: failed to create the vertex shader.\n");
        if (NULL != vs_err) {
          err_msg.assign((char*)vs_err->GetBufferPointer(), vs_err->GetBufferSize());
          printf("Error: %s", err_msg.c_str());
          goto error;
        }
      }

      hr = device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &vs_scale);
      if (S_OK != hr) {
        printf("Error: failed to create the vertex shader after we compiled it..\n");
        goto error;
      }
    } 

    /* Create the scale (pass-through) pixel shader */
    {
      hr = D3DCompile(D3D11_SCALE_SHADER.c_str(),
                      D3D11_SCALE_SHADER.size(),
                      NULL,
                      NULL,
                      NULL,
                      "pixel_shader",
                      "ps_4_0",
                      flags,
                      0,
                      &ps_blob,
                      &ps_err);

      if (S_OK != hr) {
        printf("Error: failed to create the pixel shader.\n");
        if (NULL != ps_err) {
          err_msg.assign((char*)ps_err->GetBufferPointer(), ps_err->GetBufferSize());
          printf("Error: %s", err_msg.c_str());
        }
        goto error;
      }

      hr = device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &ps_scale);
      if (S_OK != hr) {
        printf("Error: failed to create the pixel shader after we compiled it.\n");
        goto error;
      }
    }

    /* Create the vertex input layout. */
    {
      hr = device->CreateInputLayout(layout,                              /* pInputElementDescs, the input info. */
                                     num_els,                             /* NumElements, number of elements in the 'layout' var. */
                                     vs_blob->GetBufferPointer(),         /* Shader byte code. */
                                     vs_blob->GetBufferSize(),            /* Shader byte code size. */
                                     &input_layout);
      if (S_OK != hr) {
        printf("Error: failed to create the input layout for the vertex shader.\n");
        goto error;
      }
    } /* End: vertex input layout. */

    /* Create the blend state. */
    {
      D3D11_BLEND_DESC desc;
      ZeroMemory(&desc, sizeof(desc));

      desc.RenderTarget[0].BlendEnable = TRUE;
      desc.RenderTarget[0].SrcBlend	= D3D11_BLEND_SRC_ALPHA;
      desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
      desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
      desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
      desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
      desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
      desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

      hr = device->CreateBlendState(&desc, &blend_state);
      if (S_OK != hr) {
        printf("Error: failed to create the blend state.\n");
        goto error;
      }
    }

    /* Create the vertex buffer. */
    hr = createVertexBuffer();
    if (S_OK != hr) {
      goto error;
    }

    /* Create objects that will hold the output. */
    hr = createDestinationObjects();
    if (S_OK != hr) {
      goto error;
    }

    ZeroMemory(&scale_viewport, sizeof(scale_viewport));

    COM_RELEASE(ps_blob);
    COM_RELEASE(vs_blob);

    return S_OK;
    
  error:
    COM_RELEASE(ps_blob);
    COM_RELEASE(ps_err);
    COM_RELEASE(ps_scale);
    COM_RELEASE(vs_blob);
    COM_RELEASE(vs_err);
    COM_RELEASE(vs_scale);
    COM_RELEASE(input_layout);
    COM_RELEASE(blend_state);
    return hr;
  }

  HRESULT ScreenCaptureRendererDirect3D11::createVertexBuffer() {
    
    HRESULT hr = S_OK;

    if (NULL == device) {
      printf("Error: trying to create the vertex buffer but the D3D11 Device is NULL.\n");
      return E_FAIL;
    }
    
    /* Create the vertex buffer. */
    float vertices[] = {
      -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,   /* bottom left */
      -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,   /* top left */
      1.0f, -1.0f, 0.0f,  1.0f, 1.0f,    /* bottom right */
      1.0f,  1.0f, 0.0f,  1.0f, 0.0f     /* top right */
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
      return hr;
    }

    return hr;
  }

  HRESULT ScreenCaptureRendererDirect3D11::createDestinationObjects() {
    
    HRESULT hr = S_OK;

    if (NULL == device) {
      printf("Error: cannot create the destination objects; device is NULL. Not supposed to happen.\n");
      return E_FAIL;
    }

    if (0 >= settings.output_width) {
      printf("Error: cannot create the destination objects; output_width is <= 0.\n");
      return E_FAIL;
    }

    if (0 >= settings.output_height) {
      printf("Error: cannot create the destination objects; output_height is <= 0.\n");
      return E_FAIL;
    }

    if (NULL != dest_tex) {
      printf("Error: the destination texture is already created. First call shutdown().\n");
      return E_FAIL;
    }

    /* Create the destination texture into which we render the scaled version of the desktop image. */
    {
      D3D11_TEXTURE2D_DESC desc;
      ZeroMemory(&desc, sizeof(desc));

      desc.Width = settings.output_width;
      desc.Height = settings.output_height;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;                               /* This is the default data when using desktop duplication, see https://msdn.microsoft.com/en-us/library/windows/desktop/hh404611(v=vs.85).aspx */
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; 
      desc.CPUAccessFlags = 0; 
      desc.MiscFlags = 0;

      hr = device->CreateTexture2D(&desc, NULL, &dest_tex);
      if (S_OK != hr) {
        printf("Error: failed to create the render target texture.\n");
        return hr;
      }
    }

    /* 
       Create the staging texture which we seem to need if we want to transfer a texture from GPU to CPU. 

           @todo it feels unnessary to create an extra texture just for GPU > CPU transfer, but then 
           maybe D3D11 does some optimisation internally when the STAGING flag is set. 
           Now, we do this:
           > Receive desktop frame  
           > Render a scaled version into 'dest_tex' 
           > Copy 'dest_tex' into 'staging_tex'
           > Map the staging tex.

           For a bit more info: http://stackoverflow.com/questions/13479259/read-pixel-data-from-render-target-in-d3d11
    */
    {
      D3D11_TEXTURE2D_DESC desc;
      ZeroMemory(&desc, sizeof(desc));
      
      desc.Width = settings.output_width;
      desc.Height = settings.output_height;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; /* This is the default data when using desktop duplication, see https://msdn.microsoft.com/en-us/library/windows/desktop/hh404611(v=vs.85).aspx */
      desc.SampleDesc.Count = 1;                /* MultiSampling, we can use 1 as we're just downloading an existing one. */
      desc.SampleDesc.Quality = 0;              /* "" */
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      desc.MiscFlags = 0;

      hr = device->CreateTexture2D(&desc, NULL, &staging_tex);
      if (S_OK != hr) {
        printf("Error: failed to create the staging texture.\n");
        COM_RELEASE(dest_tex);
        return hr;
      }
    }

    /* Create the render target. */
    {
      D3D11_RENDER_TARGET_VIEW_DESC desc;
      ZeroMemory(&desc, sizeof(desc));

      desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; 
      desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
      desc.Texture2D.MipSlice = 0;
      
      hr = device->CreateRenderTargetView(dest_tex, &desc, &dest_target_view);
      if (S_OK != hr) {
        printf("Error: failed to create the render target view for the RTT pass.\n");
        COM_RELEASE(dest_tex);
        COM_RELEASE(staging_tex);
        return hr;
      }
    }

    /* Create the sampler. */
    {
      D3D11_SAMPLER_DESC sam_desc;
      ZeroMemory(&sam_desc, sizeof(sam_desc));
      
      sam_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      sam_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
      sam_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
      sam_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
      sam_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
      sam_desc.MinLOD = 0;
      sam_desc.MaxLOD = D3D11_FLOAT32_MAX;

      hr = device->CreateSamplerState(&sam_desc, &sampler);
      if (S_OK != hr) {
        printf("Error: failed to create the linear sampler.\n");
        COM_RELEASE(dest_tex);
        COM_RELEASE(staging_tex);
        COM_RELEASE(dest_target_view);
        return hr;
      }
    }

    return S_OK;
  }

  /*
    @todo we're setting some state which may be cached and therefore
     no necessarily needs to be set everytime. Though setting it here makes
     sure that when we add other state changes in the future, this will still
     work.
  */
  int ScreenCaptureRendererDirect3D11::scale(ID3D11Texture2D* tex) {

    if (NULL == tex) {
      printf("Error: cannot scale because the given tex pointer is NULL.\n");
      return -1;
    }
    
#if !defined(NDEBUG)    
    if (NULL == vertex_buffer) {
      printf("Error: the vertex buffer is NULL.\n");
      return -2;
    }

    if (NULL == sampler) {
      printf("Error: the sampler is NULL.\n");
      return -3;
    }

    if (NULL == staging_tex) {
      printf("Error: the staging tex is NULL.\n");
      return -4;
    }

    if (NULL == input_layout) {
      printf("Error: input layout is NULL.\n");
      return -5;
    }

    if (NULL == dest_target_view) {
      printf("Error: dest target view is NULL.\n");
      return -6;
    }
      
    if (0 >= settings.output_width) {
      printf("Error: failed to scale, because the output_width is invalid.\n");
      return -2;
    }

    if (0 >= settings.output_height) {
      printf("Error: failed to scale, because the output_height is invalid.\n");
      return -3;
    }
    
    if (NULL == settings.cb_scaled) {
      printf("Error: we're not scaling because no callback is set.\n");
      return -7;
    }
#endif
    
    /* Create the Shader Resource View */
    if (NULL == src_tex_view) {

      /* Create the viewport */
      {
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        
        tex->GetDesc(&desc);
        float width_ratio = float(settings.output_width) / desc.Width;
        float height_ratio = float(settings.output_height) / desc.Height;
        float scale_ratio = std::min<float>(width_ratio, height_ratio);
        float new_width = scale_ratio * desc.Width;
        float new_height = scale_ratio * desc.Height;

        scale_viewport.TopLeftX = (settings.output_width - new_width) * 0.5;
        scale_viewport.TopLeftY = (settings.output_height - new_height) * 0.5;
        scale_viewport.MinDepth = 0.0f;
        scale_viewport.MaxDepth = 1.0f;
        scale_viewport.Width = scale_ratio * desc.Width;
        scale_viewport.Height = scale_ratio * desc.Height;

        pointer.setViewport(scale_viewport);
        pointer.setScale(scale_viewport.Width / desc.Width,scale_viewport.Height / desc.Height);
      }

      D3D11_SHADER_RESOURCE_VIEW_DESC desc;
      ZeroMemory(&desc, sizeof(desc));
      
      desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; 
      desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      desc.Texture2D.MostDetailedMip = 0;
      desc.Texture2D.MipLevels = 1;

      HRESULT hr = device->CreateShaderResourceView(tex, &desc, &src_tex_view);
      if (S_OK != hr) {
        printf("Error: failed to create the shader resource view for the RTT pass: %s\n", hresult_to_string(hr).c_str());
        return -4;
      }
    }

    /* Set vertex layout and buffer. */
    UINT stride = sizeof(float) * 5; /* @todo I guess it's better to make this a member so we can set it where we create the vertex buffer. */
    UINT offset = 0;
    //float blend_factor[] = {0.5f, 0.5f, 0.5f, 1.0f};
    float blend_factor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float col[] = {0.0f, 0.0f, 0.0f, 1.0f};
    //context->ClearRenderTargetView(dest_target_view, col);

    context->IASetInputLayout(input_layout);
    context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->VSSetShader(vs_scale, NULL, 0);
    context->PSSetShader(ps_scale, NULL, 0);
    context->PSSetSamplers(0, 1, &sampler);
    context->PSSetShaderResources(0, 1, &src_tex_view);
    context->RSSetViewports(1, &scale_viewport);
    context->OMSetRenderTargets(1, &dest_target_view, NULL);
    context->OMSetBlendState(blend_state, blend_factor, 0xFFFFFF);
    context->Draw(4, 0);

    pointer.draw();

    /* Copy the scaled version into our staging tex so we can download it. */
    context->CopyResource(staging_tex, dest_tex);

    /* Map the surface and pass it into the callback. */
    D3D11_MAPPED_SUBRESOURCE map;
    HRESULT hr = S_OK;

    hr = context->Map(staging_tex, 0, D3D11_MAP_READ,  0, &map);
    
    if (S_OK == hr) {
      /* @todo pass the rowPitch too! */
      // printf("NBYTES: %u\n", map.DepthPitch);
      settings.cb_scaled((uint8_t*)map.pData, (int)map.RowPitch, settings.output_width, settings.output_height, settings.cb_user);
      context->Unmap(staging_tex, 0);
    }
    else if (E_INVALIDARG == hr) {
      printf("Error: received E_INVALIDARG error when trying to map the staging tex..\n");
    }
    else {
      printf("Error: failed to map: %08X in ::scale().\n", hr);
    }
    
    return 0;
  }

  int ScreenCaptureRendererDirect3D11::shutdown() {

    int r = 0;
    
    is_init = -1;
    device = NULL;
    context = NULL;
    ZeroMemory(&scale_viewport, sizeof(scale_viewport));

    COM_RELEASE(dest_tex);
    COM_RELEASE(staging_tex);
    COM_RELEASE(src_tex_view);
    COM_RELEASE(dest_target_view);
    COM_RELEASE(sampler);
    COM_RELEASE(ps_scale);
    COM_RELEASE(vs_scale);
    COM_RELEASE(input_layout);
    COM_RELEASE(vertex_buffer);
    COM_RELEASE(blend_state);

    if (0 != pointer.shutdown()) {
      r -= 1;
    }

    return r;
  }
  
} /* namespace */
