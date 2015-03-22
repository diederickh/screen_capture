#include <sstream> 
#include <screencapture/win/ScreenCaptureDuplicateOutputDirect3D11.h>
#include <screencapture/win/ScreenCaptureUtilsDirect3D11.h>

namespace sc {

  /* ----------------------------------------------------------- */
  
  void on_scaled_pixels(uint8_t* pixels, int stride, int width, int height, void* user);
  
  /* ----------------------------------------------------------- */
  
  ScreenCaptureDuplicateOutputDirect3D11::ScreenCaptureDuplicateOutputDirect3D11()
    :Base()
    ,factory(NULL)
    ,device(NULL)
    ,context(NULL)
    ,output(NULL)
    ,duplication(NULL)
    ,frame(NULL)
  {
    ZeroMemory(&output_desc, sizeof(output_desc));
    ZeroMemory(&frame_info, sizeof(frame_info));
  }
  
  /* @todo - fix return values. */
  
  int ScreenCaptureDuplicateOutputDirect3D11::init() {

    HRESULT hr = E_FAIL;

    /* Validate state */
    {
      if (NULL != factory) {
        printf("Error: we're already initialized, first call shutdown().\n");
        return -1;
      }

      if (NULL != context) {
        printf("Error: the d3d context is not NULL, first call shutdown().\n");
        return -2;
      }

      if (NULL != device) {
        printf("Error: the d3d device is not NULL, first call shutdown().\n");
        return -3;
      }
      
      if (NULL != output) {
        printf("Error: it seems we've already a selected output, first call shutdown().\n");
        return -4;
      }

      if (NULL != duplication) {
        printf("Error: we already have a duplication object; call shutdown() first.\n");
        return -5;
      }

      if (0 != adapters.size()) {
        printf("Error: our internal adapters vector contains some elements. Not supposed to happen.\n");
        return -6;
      }

      if (0 != outputs.size()) {
        printf("Error: our outputs vector contains some elements. Not supposed to happen.\n");
        return -7;
      }
    }

    /* Retrieve the IDXGIFactory that can enumerate the adapters.*/
    hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&factory));
    if (S_OK != hr) {
      printf("Error: failed to retrieve a IDXGIFactory1.\n");
      shutdown();
      return -8;
    }

    UINT i = 0;
    IDXGIAdapter1* adapter = NULL;
    while (DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter)) {
      adapters.push_back(adapter);
      ++i;
    }

    if (0 == adapters.size()) {
      printf("Error: no adapter founds, so we cannot enumerate displays.\n");
      shutdown();
      return -9;
    }

    /* Check what devices/monitors are attached to the adapters. */
    UINT dx = 0;
    IDXGIOutput* output = NULL;
    
    for (size_t i = 0; i < adapters.size(); ++i) {

      dx = 0;
      adapter = adapters[i];
    
      while (DXGI_ERROR_NOT_FOUND != adapter->EnumOutputs(dx, &output)) {
        outputs.push_back(output);
        ++dx;
      }
    }

    if (0 == outputs.size()) {
      printf("Error: no outputs found.\n");
      shutdown();
      return -10;
    }

    /* Get information about the monitors. */
    char name_buf[256] = { 0 } ;
    size_t nbytes_needed = 0;
    
    for (size_t i = 0; i < outputs.size(); ++i) {
      
      DXGI_OUTPUT_DESC desc;
      ZeroMemory(&desc, sizeof(desc));
      
      output = outputs[i];
      hr = output->GetDesc(&desc);
      
      if (S_OK != hr) {
        printf("Error: failed to retrieve the output description for monitor: %lu\n", i);
        shutdown();
        return -11;
      }
      
      sc::Display* display = NULL;
      nbytes_needed = WideCharToMultiByte(CP_UTF8, 0, desc.DeviceName, -1, NULL, 0, NULL, NULL);
      
      if (nbytes_needed > sizeof(name_buf)) {
        printf("Warning: cannot convert the name of the display to UTF8, buffer is too small. We will use a standard name.\n");

        std::stringstream ss;
        ss << "Monitor " << i;
        
        display = new sc::Display();
        display->name = ss.str();
      }
      else {
        nbytes_needed = WideCharToMultiByte(CP_UTF8, 0, desc.DeviceName, -1, name_buf, sizeof(name_buf), NULL, NULL);
        display = new sc::Display();
        display->name.assign(name_buf, nbytes_needed);
      }

      if (NULL == display) {
        printf("Error: display is NULL. Not supposed to happen.\n");
        shutdown();
        return -12;
      }

      display->info = (void*)output;
      displays.push_back(display);
    }
 
    /* Create the D3D11 Device and Device Context */
    D3D_FEATURE_LEVEL feature_level; 

    hr = D3D11CreateDevice(NULL,                     /* Adapter: The adapter (video card) we want to use. We may use NULL to pick the default adapter. */  
                           D3D_DRIVER_TYPE_HARDWARE, /* DriverType: We use the GPU as backing device. */
                           NULL,                     /* Software: we're using a D3D_DRIVER_TYPE_HARDWARE so it's not applicaple. */
                           0,                        /* D3D11_CREATE_DEVICE_FLAG */  
                           NULL,                     /* Feature Levels (ptr to array):  what version to use. */
                           0,                        /* Number of feature levels. */
                           D3D11_SDK_VERSION,        /* The SDK version, use D3D11_SDK_VERSION */
                           &device,                  /* OUT: the ID3D11Device object. */
                           &feature_level,           /* OUT: the selected feature level. */
                           &context);                /* OUT: the ID3D11DeviceContext that represents the above features. */

    if (S_OK != hr) {
      printf("Error: failed to create the D3D11 Device and Context.\n");
      shutdown();
      return -13;
    }
       
    return 0;
    
  } /* init */

  int ScreenCaptureDuplicateOutputDirect3D11::shutdown() {

    int r = 0;

    has_frame = false;
    
    if (0 != transform.shutdown()) {
      r -= 1;
    }

    if (NULL != factory) {
      factory->Release();
      factory = NULL;
    }

    for (size_t i = 0; i < adapters.size(); ++i) {
      adapters[i]->Release();
      adapters[i] = NULL;
    }
    adapters.clear();

    for (size_t i = 0; i < outputs.size(); ++i) {
      outputs[i]->Release();
      outputs[i] = NULL;
    }
    outputs.clear();

    for (size_t i = 0; i < displays.size(); ++i) {
      displays[i]->info = NULL;
      delete displays[i];
    }
    displays.clear();

    if (NULL != duplication) {
      /* @todo do we need to call stop first? */
      duplication->Release();
      duplication = NULL;
    }

    if (NULL != output) {
      output->Release();
      output = NULL;
    }

    if (NULL != device) {
      device->Release();
      device = NULL;
    }

    if (NULL != context) {
      context->Release();
      context = NULL;
    }

    if (NULL != frame) {
      frame->Release();
      frame = NULL;
    }
    
    return r;
  }

  int ScreenCaptureDuplicateOutputDirect3D11::configure(Settings cfg) {

    HRESULT hr = E_FAIL;
    
    /* Validate input. */
    if (cfg.display >= displays.size()) {
      printf("Error: given display index is invalid; out of bounds.\n");
      return -1;
    }

    if (NULL == device) {
      printf("Error: the D3D11 device is NULL. Did you call init?\n");
      return -2;
    }

    if (SC_BGRA != cfg.pixel_format) {
      printf("Trying to configure the Screen Capture, but we received an unsupported pixel format. %d\n", cfg.pixel_format);
      return -3;
    }
       
    /* Check state */
    if (NULL != output) {
      output->Release();
      output = NULL;
    }

    if (NULL != duplication) {
      duplication->Release();
      duplication = NULL;
    }

    if (0 != pixel_buffer.init(cfg.output_width, cfg.output_height, cfg.pixel_format)) {
      printf("Error: failed to initialize the pixel buffer.");
      return -4;
    }
    
    /* @todo > WE DON'T WANT TO MAKE THIS THE RESPONSIBILITY OF AN IMPLEMENTATION! */
    pixel_buffer.user = user;
    
    /*
      When the transform is already initialized, we first deallocate/release
       all created objects that are used to perform the scaling. 
    */
    if (0 == transform.isInit()) {
      transform.shutdown();
    }

    /* We can only initialize the transform, after we know the output width/height. */
    ScaleColorTransformSettingsD3D11 trans_cfg;
    trans_cfg.device = device;
    trans_cfg.context = context;
    trans_cfg.output_width = cfg.output_width;
    trans_cfg.output_height = cfg.output_height;
    trans_cfg.cb_scaled = on_scaled_pixels;
    trans_cfg.cb_user = this;
    
    if (0 != transform.init(trans_cfg)) {
      printf("Error: failed to initialize the tansform for the screen capture.\n");
      return -5;
    }

    Display* display = displays[cfg.display];
    if (NULL == display->info) {
      printf("Error: The display doesn't a valid info member. Not supposed to happen.\n");
      return -6;
    }
    
    IDXGIOutput* monitor = static_cast<IDXGIOutput*>(display->info);
    if (NULL == monitor) {
      printf("Error: failed to cast the info member of the display to IDXGIOutput*.\n");
      return -7;
    }
    
    hr = monitor->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output);
    if (S_OK != hr) {
      printf("Error: failed to query the IDXGIOutput1 which exposes the Output Duplication interface.\n");
      return -8;
    }

    hr = output->DuplicateOutput(device, &duplication);
    if (S_OK != hr) {
      printf("Error: failed to duplicate the output.\n");
      output->Release();
      output = NULL;
      return -9;
    }

    /* Get info about the output (monitor). */
    ZeroMemory(&output_desc, sizeof(output_desc));
    hr = output->GetDesc(&output_desc);
    
    if (S_OK != hr) {
      printf("Error: failed to retrieve output information.\n");
      output->Release();
      duplication->Release();
      output = NULL;
      duplication = NULL;
      return -10;
    }

#if !defined(NDEBUG)    
    printf("The monitor has the following dimensions: left: %d, right: %d, top: %d, bottom: %d.\n"
           ,(int)output_desc.DesktopCoordinates.left
           ,(int)output_desc.DesktopCoordinates.right
           ,(int)output_desc.DesktopCoordinates.top
           ,(int)output_desc.DesktopCoordinates.bottom
           );
#endif    

    /* Currently this class assumes the captured desktop image is in GPU
       mem; and we optimised for this. If the memory is already in CPU we
       need to change some stuff. Here I check if the mem is on GPU. */
    
    DXGI_OUTDUPL_DESC duplication_desc;
    duplication->GetDesc(&duplication_desc);

    if (TRUE == duplication_desc.DesktopImageInSystemMemory) {
      printf("Error: the desktop image is already in system memory; this is something we still need to implement.\n");
      output->Release();
      duplication->Release();
      output = NULL;
      duplication = NULL;
      return -11;
    }

    return 0;
  }

  int ScreenCaptureDuplicateOutputDirect3D11::start() {
    return 0;
  }

  void ScreenCaptureDuplicateOutputDirect3D11::update() {

    HRESULT hr = E_FAIL;
    ID3D11Texture2D* frame_tex = NULL;
    
#if !defined(NDEBUG)
    if (NULL == duplication) {
      printf("Error: duplication is NULL in ScreenCaptureDuplicateOutputDirect3D11().\n");
      return;
    }
#endif

   /*
      According to the remarks here: https://msdn.microsoft.com/en-us/library/windows/desktop/hh404623(v=vs.85).aspx
      we should release the frame just before calling AcquireNextFrame(). When we keep access of the 
      frame (by NOT calling ReleaseFrame as long as possible), the OS won't copy all updated regions
      until we call AcquireNextFrame again.
      
      @todo we need to check if this really improves performance :) 

    */
    if (true == has_frame) {
      
      has_frame = false;
      hr = duplication->ReleaseFrame();

      if (S_OK != hr) {
        printf("Error: failed to release the frame right before acquiring the next one.\n");
      }
    }

    /* Acquire a new frame. */
    hr = duplication->AcquireNextFrame(100, &frame_info, &frame);

    if (S_OK == hr) {

      has_frame = true;
      hr = frame->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&frame_tex);
      
      if (S_OK == hr) {
        updateMouse(&frame_info);
        transform.scale(frame_tex);
      }
    }
#if !defined(NDEBUG)    
    else if (DXGI_ERROR_WAIT_TIMEOUT == hr) {
      printf("Info: tried to acquire a desktop frame, but we timed out.\n");
    }
    else if (DXGI_ERROR_ACCESS_LOST == hr) {
      printf("Error: Access to the desktop duplication was lost. We need to handle this situation\n");
      /* See: https://msdn.microsoft.com/en-us/library/windows/desktop/hh404615(v=vs.85).aspx */
    }
    else if (DXGI_ERROR_INVALID_CALL == hr) {
      printf("Error: not supposed to happen but the call for AcquireNextFrame() failed.\n");
    }
    else {
      printf("Warning: unhandled return of AcquireNextFrame().\n");
    }
#endif

    if (NULL != frame) {
      frame->Release();
      frame = NULL;
    }

    if (NULL != frame_tex) {
      frame_tex->Release();
      frame_tex = NULL;
    }
  }

  int ScreenCaptureDuplicateOutputDirect3D11::updateMouse(DGXI_OUTDUPL_FRAME_INFO* info) {

    if (NULL == info) {
      printf("Error: requested to update the mouse info but the given info pointer is NULL.\n");
      return -1;
    }

    printf("info.LastMouseUpdateTime: %llu\n", info->LastMouseUpdateTime);
    
    return 0;
  }

  int ScreenCaptureDuplicateOutputDirect3D11::stop() {
    return 0;
  }

  int ScreenCaptureDuplicateOutputDirect3D11::getDisplays(std::vector<Display*>& result) {
    return 0;
  }

  int ScreenCaptureDuplicateOutputDirect3D11::getPixelFormats(std::vector<int>& formats) {
    
    formats.clear();
    formats.push_back(SC_BGRA); /* @todo set correct one. */
    
    return 0;
  }

  /* ----------------------------------------------------------- */
  
  void on_scaled_pixels(uint8_t* pixels, int stride, int width, int height, void* user) {

    ScreenCaptureDuplicateOutputDirect3D11* cap = static_cast<ScreenCaptureDuplicateOutputDirect3D11*>(user);
    if (NULL == cap) {
      printf("Error: failed to cast the user ptr. to the capturer. Not supposed to happen.\n");
      return;
    }

    if (NULL == cap->callback) {
      printf("Error: callback is not set.\n");
      return;
    }

    if (SC_BGRA == cap->pixel_buffer.pixel_format) {
      cap->pixel_buffer.plane[0] = pixels;
      cap->pixel_buffer.stride[0] = stride;
      cap->pixel_buffer.nbytes[0] = stride * height;
      cap->callback(cap->pixel_buffer);
    }
    else {
      printf("Error: we received a pixel buffer, but we're only supprt BGRA atm.\n");
    }
  }
} /* namespace sc */
