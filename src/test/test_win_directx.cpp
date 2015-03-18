/*
  
  Experimenting with Direct X
  ---------------------------

  For the Windows DXGI Desktop Duplication screen capture solution
  we need a way to quickly rescale the desktop pixels on the gpu. 
  Later we also need color conversion. This test is used to dive
  into the DirectX and Windows API.

  CMake info for Win32
  --------------------
  When you want to create Win32 GUI application instead of a console 
  application you need to set the WIN32 flag in 'add_executable(name WIN32 files)'
  this will change the applications entry point to 'WinMain'. 
  See: http://www.cmake.org/cmake/help/v3.0/prop_tgt/WIN32_EXECUTABLE.html#prop_tgt:WIN32_EXECUTABLE

  Vertex Buffer
  -------------
  - InputSlot: In D3D11 multiple vertex buffers can be fed to the GPU simultaneously.
               16 in total. Each vertex buffer is bound to an input slot number ranging from 
               0 to 15. The input slot field tells the GPU which vertex buffer it should fetch
               for this element (Part of D3D11_INPUT_ELEMENT_DESC)

  Shaders
  -------
  - HLSL has a concept called "semantics". A semantic is a name which is associated
    to a shader input or output variable. Semantics are required on all variables that are
    passed between shader stages. HLSL also defines System-Value semantics which have a 
    special meaning. All System-Value semantics begin with a SV_ prefix, such as 
    SV_Position.  The SV_Position is associated with a float4 from the vertex shader
    with a homogeneous clip-space position.
    

  References:
  -----------
  - I started with this one, but it's outdated: http://www.directxtutorial.com/Lesson.aspx?lessonid=11-4-5 
  - Updated version for Win8: https://msdn.microsoft.com/en-us/library/windows/apps/ff729719.aspx
  - Another great article: http://3dgep.com/introduction-to-directx-11/

 */
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>
#include <D3D11.h>
#include <D3Dcompiler.h>
//#include <D3DX11asynch.h>
//#include <D3DX11async.h>
//#include <D3dx9math.h>


/* ------------------------------------------------------------------------------------------------ */
IDXGISwapChain* d3d_chain = NULL;
ID3D11Device* d3d_device = NULL;
ID3D11DeviceContext* d3d_context = NULL;
ID3D11RenderTargetView* d3d_backbuf = NULL; /* The location in video memory where we render into. */
ID3DBlob* d3d_vs_blob = NULL; /* Compiled Vertex shader. */
ID3DBlob* d3d_ps_blob = NULL; /* Compiled Pixel shader. */
ID3D11VertexShader* d3d_vs = NULL; /* The vertex shader. */
ID3D11PixelShader* d3d_ps = NULL;  /* The pixel shader. */
ID3D11InputLayout* d3d_vertex_layout = NULL; /* Vertex buffer layout. */
ID3D11Buffer* d3d_buffer = NULL; /* Vertex Buffer. */
/* ------------------------------------------------------------------------------------------------ */
std::ofstream logfile;
void log(const std::string msg);
/* ------------------------------------------------------------------------------------------------ */

LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
HRESULT initDirect3D(HWND wnd);
HRESULT initPipeline();
void renderFrame();
HRESULT shutdownDirect3D();
std::string hresult_to_string(HRESULT hr);

/* ------------------------------------------------------------------------------------------------ */

void log(const std::string msg) {
  
  if (!logfile.is_open()) {
    return;
  }

  logfile.write(msg.c_str(), msg.size());
  logfile.write("\n", 1);
  
  logfile.flush();
}
/* ------------------------------------------------------------------------------------------------ */

int WINAPI WinMain(HINSTANCE hInstance,
                HINSTANCE hPrevInstance,
                LPSTR lpCmdLine,
                int nCmdShow)            /* nCmdShow, Indicates how the window is to appear when created. */
{
  printf("\n\ntest_win_directx\n\n");

  /* Open the log file ... */
  logfile.open("log.txt", std::ios::out | std::ios::app);
  if (!logfile.is_open()) {
    exit(EXIT_FAILURE);
  }

  HWND wnd;
  WNDCLASSEX wc;

  /* Define our window props. */
  ZeroMemory(&wc, sizeof(wc));

  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
  wc.lpszClassName = "WindowClass1";

  RegisterClassEx(&wc);

  /* 
     We use 'AdjustWindowRect' to make sure that the 
     client size will be what we requested. The windows (including
     the border may be a bit bigger).
  */
  RECT wr = { 0, 0, 500, 400 };
  AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
  
  /* Create the window. */
  wnd = CreateWindowEx(NULL,
                       "WindowClass1",        /* Name of the window class. */
                       "Direct X Test",       /* Title. */
                       WS_OVERLAPPEDWINDOW,   /* Window style. */
                       10,                    /* X position. */
                       10,                    /* Y position. */
                       wr.right - wr.left,    /* Width. */
                       wr.bottom - wr.top,    /* Height. */
                       NULL,                  /* Parent window. */
                       NULL,                  /* No menus. */
                       hInstance,             /* Application handle. */
                       NULL);                 /* Multiple windows. */

  /* And show it! */
  ShowWindow(wnd, nCmdShow);

  initDirect3D(wnd);

  printf("Lets init the pipeline.\n");

  if (S_OK != initPipeline()) {
    printf("Error: failed to init the pipeline.\n");
    exit(EXIT_FAILURE);
  }

  /* Main loop. */
  MSG msg;
  while (true) { 
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg); /* Translates keystroke messages into the right format */
      DispatchMessage(&msg);  /* Sends the messages to the WindProc function. */
      if (WM_QUIT == msg.message) {
        break;
      }
    }
    
    renderFrame();
  }

  shutdownDirect3D();
  
  return msg.wParam;
  
#if 0
  MessageBox(NULL,
             "Hello World",
             "Just another app.",
             MB_ICONEXCLAMATION | MB_OK);
#endif  
}

LRESULT CALLBACK WindowProc(HWND wnd,
                            UINT message, 
                            WPARAM wparam,
                            LPARAM lparam)
{
  switch (message) {
    case WM_DESTROY: {
      PostQuitMessage(0);
      return 0;
    }
    default: {
      printf("Warning: unhandled message %u\n", message);
      break;
    }
  }

  /* Pass on messages we didn't handle. */
  return DefWindowProc(wnd, message, wparam, lparam);
}

HRESULT initDirect3D(HWND wnd) {

  HRESULT hr;
  DXGI_SWAP_CHAIN_DESC swap_desc;
  
  ZeroMemory(&swap_desc, sizeof(swap_desc));

  swap_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  swap_desc.SampleDesc.Count = 4;
  swap_desc.SampleDesc.Quality = 0;
  swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_desc.BufferCount = 1;
  swap_desc.OutputWindow = wnd;
  swap_desc.Windowed = TRUE;
  swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; /* Describes options for handling the contents of the presentation buffer after presenting a surface. Only makes sense when the swap chain has more then one buffer.*/
  //swap_desc.Flags =  /* Describes optoins for swap-chain behavior */

  hr = D3D11CreateDeviceAndSwapChain(NULL,                      /* Adapter, NULL = use default adapter. */
                                     D3D_DRIVER_TYPE_HARDWARE,  /* Driver type, when you passed a adapter this should be the unknown type. */
                                     NULL,                      /* Handle to dll. */
                                     NULL,                      /* What runtime layers to enable. */
                                     NULL,                      /* Feature levels, NULL let the function determine. */
                                     0,                         /* Number of feature levels. */
                                     D3D11_SDK_VERSION,         /* SDK version. */
                                     &swap_desc,                /* Swap chain description. */
                                     &d3d_chain,                /* IDXGISwapChain (out). */
                                     &d3d_device,               /* D3D11Device (out). */
                                     NULL,                      /* Selected feature level (out). */
                                     &d3d_context               /* ID3DDeviceContext (out). */ 
                                     );

  if (S_OK != hr) {
    printf("Error: failed to create the D3D Device and Swap Chain.\n");
    return hr;
  }

  /* Set the render target */
  {
    
    /* Get the address of the back buffer from the swap chain. */
    ID3D11Texture2D* bb = NULL;
    d3d_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&bb);

    /* Use the back buffer address to create the render target. */
    d3d_device->CreateRenderTargetView(bb, NULL, &d3d_backbuf);

    if (NULL != bb) {
      bb->Release();
      bb = NULL;
    }

    /* Set the render target as the back buffer. */
    d3d_context->OMSetRenderTargets(1, &d3d_backbuf, NULL);
  }

  /* Set the view port */
  {
    D3D11_VIEWPORT vp;
    ZeroMemory(&vp, sizeof(vp));

    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = 500;
    vp.Height = 400;

    d3d_context->RSSetViewports(1, &vp);
  }

  return S_OK;
}

HRESULT initPipeline() {
  
  HRESULT hr = S_OK;
  ID3DBlob* vs_err = NULL;  /* Vertex shader errors. */
  ID3DBlob* ps_err = NULL;  /* Pixel shader errors.  */
  UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
  
  hr = D3DCompileFromFile(L"test_win_directx_shader.hlsl",  /* pFilename, Filename of shader. */
                          0,                         /* pDefines, Shader defines. */
                          0,                         /* pInclude, handler for include files. */
                          "vertex_shader",           /* pEntryPoint. */
                          "vs_4_0",                  /* pTarget, specifies the shader target set. */
                          flags,                     /* Flags1, compiler options */
                          0,                         /* Flags2, compiler options. */
                          &d3d_vs_blob,              /* ppCode, */  
                          &vs_err);                  /* ppErrorMsgs */ 

  if (S_OK != hr) {
    log("Error: failed to compile the vertex shader.");
    log(hresult_to_string(hr));
    if (NULL != vs_err) {
      log("But we have erorrs..");
      vs_err->Release();
      vs_err = NULL;
    }
    return hr;
  }

  hr = d3d_device->CreateVertexShader(d3d_vs_blob->GetBufferPointer(), d3d_vs_blob->GetBufferSize(), NULL, &d3d_vs);
  if (S_OK != hr) {
    log("Failed to create the vertex shader.\n");
    d3d_vs_blob->Release();
    d3d_vs_blob = NULL;
    return hr;
  }
  
  hr = D3DCompileFromFile(L"test_win_directx_shader.hlsl", 0, 0, "pixel_shader", "ps_4_0", 0, 0, &d3d_ps_blob, &ps_err);
  if (S_OK != hr) {
    log("Error: failed to compile the pixel shader.");
    ps_err->Release();
    ps_err = NULL;
  }

  hr = d3d_device->CreatePixelShader(d3d_ps_blob->GetBufferPointer(), d3d_ps_blob->GetBufferSize(), NULL, &d3d_ps);
  if (S_OK != hr) {
    log("Error: failed to create the pixel shader");
    /* @todo we could Release() the vs/ps here, but shutdownDirect3D does it too. */
    return hr;
  }

  /* Define the layout. */
  D3D11_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
  };
  
  UINT num_els = ARRAYSIZE(layout);
  hr = d3d_device->CreateInputLayout(layout, /* pInputElementDescs, the input info. */
                                     num_els, /* NumElements, number of elements in the 'layout' var. */
                                     d3d_vs_blob->GetBufferPointer(), /* Shader byte code. */
                                     d3d_vs_blob->GetBufferSize(), /* Shader byte code size. */
                                     &d3d_vertex_layout);
  if (S_OK != hr) {
    log("Error: failed to create the input layout for the vertex shader.");
    return hr;
  }

  d3d_context->IASetInputLayout(d3d_vertex_layout);

  /* Create the vertex buffer. */
  float vertices[] = {
    0.0f, 0.5f, 0.5f,
    0.5f, -0.5f, 0.5f,
    -0.5f, -0.5f, 0.5f
  };

  D3D11_BUFFER_DESC bd;
  ZeroMemory(&bd, sizeof(bd));
  bd.ByteWidth = sizeof(float) * 9;
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

  hr = d3d_device->CreateBuffer(&bd, &init_data, &d3d_buffer);
  if (S_OK != hr) {
    log("Failed to create the vertex buffer.\n");
    return hr;
  }

  UINT stride = sizeof(float) * 3;
  UINT offset = 0;
  d3d_context->IASetVertexBuffers(0, 1, &d3d_buffer, &stride, &offset);
  d3d_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  d3d_context->VSSetShader(d3d_vs, NULL, 0); /* Set vertex shader. */
  d3d_context->PSSetShader(d3d_ps, NULL, 0);

  log("Pipeline initialized.\n");

  return hr;
}

void renderFrame() {
  
  if (NULL == d3d_chain) {
    printf("Error: trying to render a frame but the d3d chain is NULL.\n");
    return;
  }

  FLOAT col[4] = { 0.0f, 0.2f, 0.4f, 1.0f } ;
  d3d_context->ClearRenderTargetView(d3d_backbuf, col);

  /* Render using the vertex buffer we've bound in initPipeline(). */
  d3d_context->Draw(3, 0);

  /* Switch back buffer and frond buffer. */
  d3d_chain->Present(0,0);
}

HRESULT shutdownDirect3D() {
  
  HRESULT hr = S_OK;

  if (NULL != d3d_buffer) {
    d3d_buffer->Release();
    d3d_buffer = NULL;
  }
  
  if (NULL != d3d_vertex_layout) {
    d3d_vertex_layout->Release();
    d3d_vertex_layout = NULL;
  }

  if (NULL != d3d_vs_blob) {
    d3d_vs_blob->Release();
    d3d_vs_blob = NULL;
  }

  if (NULL != d3d_ps_blob) {
    d3d_ps_blob->Release();
    d3d_ps_blob = NULL;
  }

  if (NULL != d3d_vs) {
    d3d_vs->Release();
    d3d_vs = NULL;
  }

  if (NULL != d3d_ps) {
    d3d_ps->Release();
    d3d_ps = NULL;
  }

  if (NULL != d3d_chain) {
    d3d_chain->Release();
    d3d_chain = NULL;
  }
  else {
    hr = E_FAIL;
  }

  if (NULL != d3d_backbuf) {
    d3d_backbuf->Release();
    d3d_backbuf = NULL;
  }

  if (NULL != d3d_device) {
    d3d_device->Release();
    d3d_device = NULL;
  }
  else {
    hr = E_FAIL;
  }

  if (NULL != d3d_context) {
    d3d_context->Release();
    d3d_context = NULL;
  }
  else {
    hr = E_FAIL;
  }
  
  return hr;
}

std::string hresult_to_string(HRESULT hr) {
  switch (hr) {
    case D3D11_ERROR_FILE_NOT_FOUND: return "D3D11_ERROR_FILE_NOT_FOUND";
    case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
    case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";
    case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD: return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";
    case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
    case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING";
    case E_FAIL: return "E_FAIL";
    case E_INVALIDARG: return "E_INVALIDARG";
    case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
    case E_NOTIMPL: return "E_NOTIMPL";
    case S_FALSE: return "S_FALSE";
    case S_OK: return "S_OK";
    default: {
      std::stringstream ss;
      ss << std::hex << (unsigned int)hr;
      return "UNKNOWN HRESULT: " +ss.str();
    }
  }
}
