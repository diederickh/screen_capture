#include <screencapture/win/ScreenCaptureScaleAndColorTransformDirect3D11.h>

namespace sc {

  /* ----------------------------------------------------------- */

  ScaleAndTransformSettingsD3D11::ScaleAndTransformSettingsD3D11()
    :device(NULL)
    ,context(NULL)
  {
  }
  
  /* ----------------------------------------------------------- */

  ScreenCaptureScaleAndColorTransformDirect3D11::ScreenCaptureScaleAndColorTransformDirect3D11()
    :device(NULL)
    ,context(NULL)
    ,tex(NULL)
    ,target_view(NULL)
    ,sampler(NULL)
  {

  }

  ScreenCaptureScaleAndColorTransformDirect3D11::~ScreenCaptureScaleAndColorTransformDirect3D11() {
    device = NULL;
    context = NULL;
  }

  int ScreenCaptureScaleAndColorTransformDirect3D11::init(ScaleAndTransformSettingsD3D11 cfg) {
    
    if (NULL == cfg.device) {
      printf("Error: the given D3D device is NULL.\n");
      return -1;
    }

    if (NULL == cfg.context) {
      printf("Error: the given D3D context is NULL\n");
      return -2;
    }

    device = cfg.device;
    context = cfg.context;

    return 0;
  }

  int ScreenCaptureScaleAndColorTransformDirect3D11::shutdown() {

    /* @todo release textures, shaders, etc.. */

    if (NULL != tex) {
      tex->Release();
      tex = NULL;
    }

    if (NULL != target_view) {
      target_view->Release();
      target_view = NULL;
    }

    if (NULL != sampler) {
      sampler->Release();
      sampler = NULL;
    }
    
    device = NULL;
    context = NULL;

    return 0;
  }
  
} /* namespace */
