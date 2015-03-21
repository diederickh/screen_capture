#include <stdlib.h>
#include <screencapture/ScreenCapture.h>

namespace sc {

  ScreenCapture::ScreenCapture(screencapture_callback callback, void* user, int driver)
    :impl(NULL)
  {
    
#if defined(__APPLE__)
    if (NULL == impl && SC_DISPLAY_STREAM == driver) {
      impl = new ScreenCaptureDisplayStream();
    }
#elif defined(_WIN32)
    if (NULL == impl && SC_DUPLICATE_OUTPUT_DIRECT3D11 == driver) {
      impl = new ScreenCaptureDuplicateOutputDirect3D11();
    }
#endif

    if (NULL == impl) {
      printf("Error: we didn't find a screencapture driver.\n");
      exit(EXIT_FAILURE);
    }

    if (0 != impl->setCallback(callback, user)) {
      printf("Error: failed to set the callback on the screencapture driver. Not supposed to happen..\n");
      exit(EXIT_FAILURE);
    }
  }

  ScreenCapture::~ScreenCapture() {

    if (0 == isInit()) {
      shutdown();
    }

    delete impl;
    impl = NULL;
  }

  int ScreenCapture::init() {
    
    if (0 == isInit()) {
      printf("Error: already initialised the screen capture, already initialised.\n");
      return -1;
    }

    if (0 != impl->init()) {
      printf("Error: failed to initialise the screen capture driver. See log above.\n");
      return -2;
    }

    impl->state |= SC_STATE_INIT;

    return 0;
  }

  int ScreenCapture::configure(Settings settings) {

    if (0 != isInit()) {
      printf("Error: cannot configure because we're not yet initialized. Call init() first.\n");
      return -1;
    }

    if (0 > settings.display) {
      printf("Error: invalid display given for the ScreenCapture (%d).\n", settings.display);
      return -2;
    }

    if (0 > settings.pixel_format) {
      printf("Error: invalid pixel format given for ScreenCapture (%d).\n", settings.pixel_format);
      return -3;
    }

    if (0 > settings.output_width) {
      printf("Error: invalid output width set for ScreenCapture (%d).\n", settings.output_width);
      return -4;
    }

    if (0 > settings.output_height) {
      printf("Error: invalid output height set for ScreenCapture (%d).\n", settings.output_height);
      return -5;
    }

    if (NULL == impl->callback) {
      printf("Error: cannot configure screencapture, because the frame callback is NULL.\n");
      return -6;
    }

    if (0 != impl->configure(settings)) {
      printf("Error: failed to setup the screencapture.\n");
      return -7;
    }

    impl->state |= SC_STATE_CONFIGURED;

    return 0;
  }

  int ScreenCapture::listDisplays() {

    if (0 != isInit()) {
      printf("Error: cannot list displays because we're not initialized.\n");
      return -1;
    }

    std::vector<Display*> displays;
    if (0 != getDisplays(displays)) {
      printf("Error: failed to retrieve displays in the screencapturer; cannot list them.\n");
      return -1;
    }

    for (size_t i = 0; i < displays.size(); ++i) {
      printf("[%02lu]: %s\n", i, displays[i]->name.c_str());
    }

    return 0;
  }

  int ScreenCapture::listPixelFormats() {

    if (0 != isInit()) {
      printf("Error: cannot list pixel formats; we're not initalized in ScreenCapture.\n");
      return -1;
    }
    
    std::vector<int> formats;
    if (0 != getPixelFormats(formats)) {
      printf("Error: failed to retrieve the supported pixel formats in the screen capturer.\n");
      return -2;
    }

    for (size_t i = 0; i < formats.size(); ++i) {
      printf("- Supported pixel format: %s\n", screencapture_pixelformat_to_string(formats[i]).c_str());
    }
    
    return 0;
  }

  int ScreenCapture::isPixelFormatSupported(int fmt) {

    if (0 != isInit()) {
      printf("Error: cannot check if the given pixel format is supported because you didn't init the screen capture.\n");
      return -1;
    }

    std::vector<int> formats;
    if (0 != getPixelFormats(formats)) {
      printf("Error: cannto check if the given pixel format is supported. Failed to retrieve list of formats.\n");
      return -2;
    }

    for (size_t i = 0; i < formats.size(); ++i) {
      if (formats[i] == fmt) {
        return 0;
      }
    }

    return -3;
  }

  int ScreenCapture::shutdown() {

    int r = 0;

    if (0 == isShutdown()) {
      printf("Warning: shutting down screen capture but we're already shutdown.\n");
      return 0;
    }
    
    if (0 == isStarted()) {
      if (0 != stop()) {
        r -= 1;
      }
    }

    if (0 == isInit()) {
      if (0 != impl->shutdown()) {
        printf("Error: when trying to shutdown the screen capture an error occured.\n");
        r -= 2;
      }
    }

    impl->state &= ~SC_STATE_INIT;
    impl->state |= SC_STATE_SHUTDOWN;

    return r;
  }

  int ScreenCapture::start() {

    /* Only start when init. */
    if (0 != isInit()) {
      printf("Error: cannot start screencapture because we're not initialized.\n");
      return -1;
    }

    /* Already started? */
    if (0 == isStarted()) {
      printf("Warning: you're trying to start screencapture but we're already capturing.\n");
      return -2;
    }
    
    if (0 != impl->start()) {
      printf("Error: failed to start screencapture, see log.\n");
      return -3;
    }

    impl->state |= SC_STATE_STARTED;
    impl->state &= ~SC_STATE_STOPPED;
    
    return 0;
  }

  void ScreenCapture::update() {
    
#if !defined(NDEBUG)
    if (NULL == impl) {
      printf("Error: trying to update the screencapturer, but we're not initialized; no implementation was allocated.\n");
      exit(EXIT_FAILURE);
    }
#endif
    
    impl->update();
  }

  int ScreenCapture::stop() {

    int r = 0;

    /* Alrady stoped? */
    if (0 == isStopped()) {
      printf("Warning: stopping the screen capture but we're already stopped.\n");
      return 0;
    }

    /* When started, try to stop. */
    if (0 == isStarted()) {
      if (0 != impl->stop()) {
        printf("Error: when trying to shutdown and stopping the screen capture an error occured.\n");
        r -= 1;
      }    
    }

    impl->state &= ~SC_STATE_STARTED;
    impl->state |= SC_STATE_STOPPED;

    return r;
  }
  
} /* namespace sc */
