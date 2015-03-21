/*
  Win API tests for Screen Capture
  --------------------------------

  This code was written while I was exploring the Windows API. 
  I'm not a Windows SDK developer and I'm sure that there are 
  other, better ways to capture screen using the output duplication 
  API then below. Though if you're like me and diving into the 
  Windows API for screen capturing using the output duplication 
  method this code may show you some usefull info.

  Enumerating Adapters (Graphics Cards) and Outputs (Monitors)
  ------------------------------------------------------------
  - Use IDXGIFactory, EnumAdapters.
  - For each enumerated IDXGIAdapter, use EnumOutputs to get the IDXGIOutput (Monitors)

  Duplicate Output
  ----------------
  - From a IDGIOutput which you can retrieve from an IDXGIAdapter you 
    need to query the IDXGIOutput1 interface which exposes the 
    IDXGIOutputDuplication api.
  - You call IDXGIOutput1::DuplicateOutput() and pass a Direct3D 
    device for the first parameter. 
 
  Receiving desktop images
  ------------------------
  - Once you have a IDXGIOutputDuplication handle you can start 
    requesting (acquiring) frames. You use the AcquireNextFrame(), 
    on the duplication handle. When you receive a S_OK it means you
    received a new frame. 
  - The received frame is stored in a IDXGIResource, which you need
    to release (isn't documented, but found this inthe examples).
  - You can get access to the texture that the IDXGIResource wraps
    around by querying the ID3D11Texture2D interface on this object.
 
  
  References:
  -----------
  - Code Project, 3 solutions: http://www.codeproject.com/Articles/5051/Various-methods-for-capturing-the-screen
  - SO, see comment of Herman: http://stackoverflow.com/questions/5069104/fastest-method-of-screen-capturing
  - A blog post on DirectX, from above SO: http://blog.nektra.com/main/2013/07/23/instrumenting-direct3d-applications-to-capture-video-and-calculate-frames-per-second/
  - Mirror Driver: https://msdn.microsoft.com/en-us/library/windows/hardware/ff568315(v=vs.85).aspx
    ^
    |
    +--- This supposed to be the best solution according to SO and the author of the Code Project article.
  - Desktop Duplication API (win8): http://blogs.msdn.com/b/dsui_team/archive/2013/03/25/ways-to-capture-the-screen.aspx
    + Doesn't work for all full screen apps. 
  - Some blog with a couple of methods: http://blogs.msdn.com/b/dsui_team/archive/2013/03/25/ways-to-capture-the-screen.aspx
  - A nice clean, simple example of reading frames using the duplication api: https://github.com/lwnexgen/Dx11Streamer/blob/master/Dx11Streamer/Main.cpp
    Backup: https://gist.github.com/roxlu/b5689d9ad2eda095b793
  - How to map the pixels you receive from the duplication api: : http://www.getcodesamples.com/src/2D4BC0AA/BFBE14E5

 */
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <DXGI.h>
#include <DXGI1_2.h> /* For IDXGIOutput1 */
#include <D3D11.h> /* For the D3D11* interfaces. */
#include <png.h>

/*
    save_png
    -----------------------------------------------------

    A simple to save a png with a bit more flexibility.
    
    - filename:   the path where you want to save the png.
    - width:      width of the image
    - height:     height of the image
    - bitdepth:   how many bits per pixel (e.g. 8).
    - colortype:  PNG_COLOR_TYEP_GRAY
                  PNG_COLOR_TYPE_PALETTE
                  PNG_COLOR_TYPE_RGB
                  PNG_COLOR_TYPE_RGB_ALPHA
                  PNG_COLOR_TYPE_GRAY_ALPHA
                  PNG_COLOR_TYPE_RGBA          (alias for _RGB_ALPHA)
                  PNG_COLOR_TYPE_GA            (alias for _GRAY_ALPHA)
    - pitch:      The stride (e.g. '4 * width' for RGBA). 
    - transform:  PNG_TRANSFORM_IDENTITY
                  PNG_TRANSFORM_PACKING
                  PNG_TRANSFORM_PACKSWAP
                  PNG_TRANSFORM_INVERT_MONO
                  PNG_TRANSFORM_SHIFT
                  PNG_TRANSFORM_BGR
                  PNG_TRANSFORM_SWAP_ALPHA
                  PNG_TRANSFORM_SWAP_ENDIAN
                  PNG_TRANSFORM_INVERT_ALPHA
                  PNG_TRANSFORM_STRIP_FILLER
                  PNG_TRANSFORM_STRIP_FILLER_AFTER

 */
static int save_png(std::string filename, int width, int height,
                    int bitdepth, int colortype,
                    unsigned char* data, int pitch,
                    int transform = PNG_TRANSFORM_IDENTITY);

int main() {

  printf("\n\ntest_win_api_directx_research\n\n");

  /* Retrieve a IDXGIFactory that can enumerate the adapters. */
  IDXGIFactory1* factory = NULL;
  HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&factory));

  if (S_OK != hr) {
    printf("Error: failed to retrieve the IDXGIFactory.\n");
    exit(EXIT_FAILURE);
  }

  /* Enumerate the adapters.*/
  UINT i = 0;
  IDXGIAdapter1* adapter = NULL;
  std::vector<IDXGIAdapter1*> adapters; /* Needs to be Released(). */

  while (DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter)) {
    adapters.push_back(adapter);
    ++i;
  }

  /* Get some info about the adapters (GPUs). */
  for (size_t i = 0; i < adapters.size(); ++i) {
    
    DXGI_ADAPTER_DESC1 desc;
    adapter = adapters[i];
    hr = adapter->GetDesc1(&desc);
    
    if (S_OK != hr) {
      printf("Error: failed to get a description for the adapter: %lu\n", i);
      continue;
    }

    wprintf(L"Adapter: %lu, description: %s\n", i, desc.Description);
  }

  /* Check what devices/monitors are attached to the adapters. */
  UINT dx = 0;
  IDXGIOutput* output = NULL;
  std::vector<IDXGIOutput*> outputs; /* Needs to be Released(). */
  
  for (size_t i = 0; i < adapters.size(); ++i) {

    dx = 0;
    adapter = adapters[i];
    
    while (DXGI_ERROR_NOT_FOUND != adapter->EnumOutputs(dx, &output)) {
      printf("Found monitor %d on adapter: %lu\n", dx, i);
      outputs.push_back(output);
      ++dx;
    }
  }

  if (0 >= outputs.size()) {
    printf("Error: no outputs found (%lu).\n", outputs.size());
    exit(EXIT_FAILURE);
  }

  /* Print some info about the monitors. */
  for (size_t i = 0; i < outputs.size(); ++i) {
    
    DXGI_OUTPUT_DESC desc;
    output = outputs[i];
    hr = output->GetDesc(&desc);
    
    if (S_OK != hr) {
      printf("Error: failed to retrieve a DXGI_OUTPUT_DESC for output %lu.\n", i);
      continue;
    }

    wprintf(L"Monitor: %s, attached to desktop: %c\n", desc.DeviceName, (desc.AttachedToDesktop) ? 'y' : 'n');
  }

  /*

    To get access to a OutputDuplication interface we need to have a 
    Direct3D device which handles the actuall rendering and "gpu" 
    stuff. According to a gamedev stackexchange it seems we can create
    one w/o a HWND. 

   */

  ID3D11Device* d3d_device = NULL; /* Needs to be released. */
  ID3D11DeviceContext* d3d_context = NULL; /* Needs to be released. */
  IDXGIAdapter1* d3d_adapter = NULL;
  D3D_FEATURE_LEVEL d3d_feature_level; /* The selected feature level (D3D version), selected from the Feature Levels array, which is NULL here; when it's NULL the default list is used see:  https://msdn.microsoft.com/en-us/library/windows/desktop/ff476082%28v=vs.85%29.aspx ) */
  
  { /* Start creating a D3D11 device */

#if 1
    /* 
       NOTE:  Apparently the D3D11CreateDevice function returns E_INVALIDARG, when
              you pass a pointer to an adapter for the first parameter and use the 
              D3D_DRIVER_TYPE_HARDWARE. When you want to pass a valid pointer for the
              adapter, you need to set the DriverType parameter (2nd) to 
              D3D_DRIVER_TYPE_UNKNOWN.
             
              @todo figure out what would be the best solution; easiest to use is 
              probably using NULL here. 
     */
    int use_adapter = 0;
    if (use_adapter >= adapters.size()) {
      printf("Invalid adapter index: %d, we only have: %lu - 1\n", use_adapter, adapters.size());
      exit(EXIT_FAILURE);
    }

    d3d_adapter = adapters[use_adapter];
    if (NULL == d3d_adapter) {
      printf("Error: the stored adapter is NULL.\n");
      exit(EXIT_FAILURE);
    }
#endif

    hr = D3D11CreateDevice(d3d_adapter,              /* Adapter: The adapter (video card) we want to use. We may use NULL to pick the default adapter. */  
                           D3D_DRIVER_TYPE_UNKNOWN,  /* DriverType: We use the GPU as backing device. */
                           NULL,                     /* Software: we're using a D3D_DRIVER_TYPE_HARDWARE so it's not applicaple. */
                           NULL,                     /* Flags: maybe we need to use D3D11_CREATE_DEVICE_BGRA_SUPPORT because desktop duplication is using this. */
                           NULL,                     /* Feature Levels (ptr to array):  what version to use. */
                           0,                        /* Number of feature levels. */
                           D3D11_SDK_VERSION,        /* The SDK version, use D3D11_SDK_VERSION */
                           &d3d_device,              /* OUT: the ID3D11Device object. */
                           &d3d_feature_level,       /* OUT: the selected feature level. */
                           &d3d_context);            /* OUT: the ID3D11DeviceContext that represents the above features. */

    if (S_OK != hr) {
      printf("Error: failed to create the D3D11 Device.\n");
      if (E_INVALIDARG == hr) {
        printf("Got INVALID arg passed into D3D11CreateDevice. Did you pass a adapter + a driver which is not the UNKNOWN driver?.\n");
      }
      exit(EXIT_FAILURE);
    }
    
  } /* End creating a D3D11 device. */
  
  /* 
     Create a IDXGIOutputDuplication for the first monitor. 
     
     - From a IDXGIOutput which represents an monitor, we query a IDXGIOutput1
       because the IDXGIOutput1 has the DuplicateOutput feature. 
  */

  IDXGIOutput1* output1 = NULL;
  IDXGIOutputDuplication* duplication = NULL;
  
  { /* Start IDGIOutputDuplication init. */
    
    int use_monitor = 0;
    if (use_monitor >= outputs.size()) {
      printf("Invalid monitor index: %d, we only have: %lu - 1\n", use_monitor, outputs.size());
      exit(EXIT_FAILURE);
    }

    output = outputs[use_monitor];
    if (NULL == output) {
      printf("No valid output found. The output is NULL.\n");
      exit(EXIT_FAILURE);
    }
    
    hr = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
    if (S_OK != hr) {
      printf("Error: failed to query the IDXGIOutput1 interface.\n");
      exit(EXIT_FAILURE);
    }

    hr = output1->DuplicateOutput(d3d_device, &duplication);
    if (S_OK != hr) {
      printf("Error: failed to create the duplication output.\n");
      exit(EXIT_FAILURE);
    }
    printf("Queried the IDXGIOutput1.\n");
                                
  } /* End IDGIOutputDuplication init. */

  if (NULL == duplication) {
    printf("Error: okay, we shouldn't arrive here but the duplication var is NULL.\n");
    exit(EXIT_FAILURE);
  }

  /*
    To download the pixel data from the GPU we need a 
    staging texture. Therefore we need to determine the width and 
    height of the buffers that we receive. 
    
    @TODO - We could also retrieve the width/height from the texture we got 
            from through the acquired frame (see the 'tex' variable below).
            That may be a safer solution.
  */
  DXGI_OUTPUT_DESC output_desc;
  {
    hr = output->GetDesc(&output_desc);
    if (S_OK != hr) {
      printf("Error: failed to get the DXGI_OUTPUT_DESC from the output (monitor). We need this to create a staging texture when downloading the pixels from the gpu.\n");
      exit(EXIT_FAILURE);
    }

    printf("The monitor has the following dimensions: left: %d, right: %d, top: %d, bottom: %d.\n"
           ,(int)output_desc.DesktopCoordinates.left
           ,(int)output_desc.DesktopCoordinates.right
           ,(int)output_desc.DesktopCoordinates.top
           ,(int)output_desc.DesktopCoordinates.bottom
           );
  }

  if (0 == output_desc.DesktopCoordinates.right
      || 0 == output_desc.DesktopCoordinates.bottom)
    {
      printf("The output desktop coordinates are invalid.\n");
      exit(EXIT_FAILURE);
    }
    
  /* Create the staging texture that we need to download the pixels from gpu. */
  D3D11_TEXTURE2D_DESC tex_desc;
  tex_desc.Width = output_desc.DesktopCoordinates.right;
  tex_desc.Height = output_desc.DesktopCoordinates.bottom;
  tex_desc.MipLevels = 1;
  tex_desc.ArraySize = 1; /* When using a texture array. */
  tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; /* This is the default data when using desktop duplication, see https://msdn.microsoft.com/en-us/library/windows/desktop/hh404611(v=vs.85).aspx */
  tex_desc.SampleDesc.Count = 1; /* MultiSampling, we can use 1 as we're just downloading an existing one. */
  tex_desc.SampleDesc.Quality = 0; /* "" */
  tex_desc.Usage = D3D11_USAGE_STAGING;
  tex_desc.BindFlags = 0;
  tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  tex_desc.MiscFlags = 0;

  ID3D11Texture2D* staging_tex = NULL;
  hr = d3d_device->CreateTexture2D(&tex_desc, NULL, &staging_tex);
  if (E_INVALIDARG == hr) {
    printf("Error: received E_INVALIDARG when trying to create the texture.\n");
    exit(EXIT_FAILURE);
  }
  else if (S_OK != hr) {
    printf("Error: failed to create the 2D texture, error: %d.\n", hr);
    exit(EXIT_FAILURE);
  }
   
  /* 
     Get some info about the output duplication. 
     When the DesktopImageInSystemMemory is TRUE you can use 
     the MapDesktopSurface/UnMapDesktopSurface directly to retrieve the
     pixel data. If not, then you need to use a surface. 

  */
  DXGI_OUTDUPL_DESC duplication_desc;
  duplication->GetDesc(&duplication_desc);
  printf("duplication desc.DesktopImageInSystemMemory: %c\n", (duplication_desc.DesktopImageInSystemMemory) ? 'y' : 'n');
  
  /* Access a couple of frames. */
  DXGI_OUTDUPL_FRAME_INFO frame_info;
  IDXGIResource* desktop_resource = NULL;
  ID3D11Texture2D* tex = NULL;
  DXGI_MAPPED_RECT mapped_rect;
  
  for (int i = 0; i < 500; ++i) {
    //  printf("%02d - ", i);
    
    hr = duplication->AcquireNextFrame(1000, &frame_info, &desktop_resource);
    if (DXGI_ERROR_ACCESS_LOST == hr) {
      printf("Received a DXGI_ERROR_ACCESS_LOST.\n");
    }
    else if (DXGI_ERROR_WAIT_TIMEOUT == hr) {
      printf("Received a DXGI_ERROR_WAIT_TIMEOUT.\n");
    }
    else if (DXGI_ERROR_INVALID_CALL == hr) {
      printf("Received a DXGI_ERROR_INVALID_CALL.\n");
    }
    else if (S_OK == hr) {
      //printf("Yay we got a frame.\n");

      /* Print some info. */
      //printf("frame_info.TotalMetadataBufferSize: %u\n", frame_info.TotalMetadataBufferSize);
      //printf("frame_info.AccumulatedFrames: %u\n", frame_info.AccumulatedFrames);

      /* Get the texture interface .. */
#if 1      
      hr = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);
      if (S_OK != hr) {
        printf("Error: failed to query the ID3D11Texture2D interface on the IDXGIResource we got.\n");
        exit(EXIT_FAILURE);
      }
#endif      

      /* Map the desktop surface */
      hr = duplication->MapDesktopSurface(&mapped_rect);
      if (S_OK == hr) {
        printf("We got acess to the desktop surface\n");
        hr = duplication->UnMapDesktopSurface();
        if (S_OK != hr) {
          printf("Error: failed to unmap the desktop surface after successfully mapping it.\n");
        }
      }
      else if (DXGI_ERROR_UNSUPPORTED == hr) {
        //printf("MapDesktopSurface returned DXGI_ERROR_UNSUPPORTED.\n");
        /* 
           According to the docs, when we receive this error we need
           to transfer the image to a staging surface and then lock the 
            image by calling IDXGISurface::Map().

           To get the data from GPU to the CPU, we do:

               - copy the frame into our staging texture
               - map the texture 
               - ... do something 
               - unmap.

           @TODO figure out what solution is faster:

           There are multiple solutions to copy a texture. I have 
           to look into what solution is better. 
           -  d3d_context->CopySubresourceRegion();
           -  d3d_context->CopyResource(dest, src)

           @TODO we need to make sure that the width/height are valid. 
 
        */

        d3d_context->CopyResource(staging_tex, tex);

        D3D11_MAPPED_SUBRESOURCE map;
        HRESULT map_result = d3d_context->Map(staging_tex,          /* Resource */
                                              0,                    /* Subresource */ 
                                              D3D11_MAP_READ,       /* Map type. */
                                              0,                    /* Map flags. */
                                              &map);

        if (S_OK == map_result) {
          unsigned char* data = (unsigned char*)map.pData;
          //printf("Mapped the staging tex; we can access the data now.\n");
          printf("RowPitch: %u, DepthPitch: %u, %02X, %02X, %02X\n", map.RowPitch, map.DepthPitch, data[0], data[1], data[2]);
#if 0
          if (i < 25) {
            char fname[512];

            /* We have to make the image opaque. */

            for (int k = 0; k < tex_desc.Width; ++k) {
              for (int l = 0; l < tex_desc.Height; ++l) {
                int dx = l * tex_desc.Width * 4 + k * 4;
                data[dx + 3] = 0xFF;
              }
            }
            sprintf(fname, "capture_%03d.png", i);
            save_png(fname,
                     tex_desc.Width, tex_desc.Height, 8, PNG_COLOR_TYPE_RGBA,
                     (unsigned char*)map.pData, map.RowPitch, PNG_TRANSFORM_BGR);
          }
#endif
        }
        else {
          printf("Error: failed to map the staging tex. Cannot access the pixels.\n");
        }

        d3d_context->Unmap(staging_tex, 0);
      }
      else if (DXGI_ERROR_INVALID_CALL == hr) {
        printf("MapDesktopSurface returned DXGI_ERROR_INVALID_CALL.\n");
      }
      else if (DXGI_ERROR_ACCESS_LOST == hr) {
        printf("MapDesktopSurface returned DXGI_ERROR_ACCESS_LOST.\n");
      }
      else if (E_INVALIDARG == hr) {
        printf("MapDesktopSurface returned E_INVALIDARG.\n");
      }
      else {
        printf("MapDesktopSurface returned an unknown error.\n");
      }
    }

    /* Clean up */
    {

      if (NULL != tex) {
        tex->Release();
        tex = NULL;
      }
      
      if (NULL != desktop_resource) {
        desktop_resource->Release();
        desktop_resource = NULL;
      }

      /* We must release the frame. */
      hr = duplication->ReleaseFrame();
      if (S_OK != hr) {
        printf("Failed to release the duplication frame.\n");
      }
    }
  }
  
  //printf("Monitors connected to adapter: %lu\n", i);

  /* Cleanup */
  {

    if (NULL != staging_tex) {
      staging_tex->Release();
      staging_tex = NULL;
    }
    
    if (NULL != d3d_device) {
      d3d_device->Release();
      d3d_device = NULL;
    }

    if (NULL != d3d_context) {
      d3d_context->Release();
      d3d_context = NULL;
    }

    if (NULL != duplication) {
      duplication->Release();
      duplication = NULL;
    }
    
    for (size_t i = 0; i < adapters.size(); ++i) {
      if (NULL != adapters[i]) {
        adapters[i]->Release();
        adapters[i] = NULL;
      }
    }

    for (size_t i = 0; i < outputs.size(); ++i) {
      if (NULL != outputs[i]) {
        outputs[i]->Release();
        outputs[i] = NULL;
      }
    }

    if (NULL != output1) {
      output1->Release();
      output1 = NULL;
    }

    if (NULL != factory) {
      factory->Release();
      factory = NULL;
    }
  }
  
  return 0;
}


static int save_png(std::string filename, int width, int height,
                    int bitdepth, int colortype,
                    unsigned char* data, int pitch, int transform)
{
  int i = 0;
  int r = 0;
  FILE* fp = NULL;
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  png_bytep* row_pointers = NULL;
  
  if (NULL == data) {
    printf("Error: failed to save the png because the given data is NULL.\n");
    r = -1;
    goto error;
  }

  if (0 == filename.size()) {
    printf("Error: failed to save the png because the given filename length is 0.\n");
    r = -2;
    goto error;
  }

  if (0 == pitch) {
    printf("Error: failed to save the png because the given pitch is 0.\n");
    r = -3;
    goto error;
  }

  fp = fopen(filename.c_str(), "wb");
  if (NULL == fp) {
    printf("Error: failed to open the png file: %s\n", filename.c_str());
    r = -4;
    goto error;
  }

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (NULL == png_ptr) {
    printf("Error: failed to create the png write struct.\n");
    r = -5;
    goto error;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (NULL == info_ptr) {
    printf("Error: failed to create the png info struct.\n");
    r = -6;
    goto error;
  }
  
  png_set_IHDR(png_ptr,
               info_ptr,
               width,
               height,
               bitdepth,                 /* e.g. 8 */
               colortype,                /* PNG_COLOR_TYPE_{GRAY, PALETTE, RGB, RGB_ALPHA, GRAY_ALPHA, RGBA, GA} */
               PNG_INTERLACE_NONE,       /* PNG_INTERLACE_{NONE, ADAM7 } */
               PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);
               
  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  
  for (i = 0; i < height; ++i) {
    row_pointers[i] = data + i * pitch;
  }

  png_init_io(png_ptr, fp);
  png_set_rows(png_ptr, info_ptr, row_pointers);
  png_write_png(png_ptr, info_ptr, transform, NULL);

 error:
  
  if (NULL != fp) {
    fclose(fp);
    fp = NULL;
  }

  if (NULL != png_ptr) {
    
    if (NULL == info_ptr) {
      printf("Error: info ptr is null. not supposed to happen here.\n");
    }
    
    png_destroy_write_struct(&png_ptr, &info_ptr);
    png_ptr = NULL;
    info_ptr = NULL;
  }
  
  if (NULL != row_pointers) {
    free(row_pointers);
    row_pointers = NULL;
  }
  
  printf("And we're all free.\n");
  
  return r;
}
