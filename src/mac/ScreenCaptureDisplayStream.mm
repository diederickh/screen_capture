/* -*-c++-*- */
#include <sstream>
#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>
#include <screencapture/mac/ScreenCaptureDisplayStream.h>

namespace sc {

  ScreenCaptureDisplayStream::ScreenCaptureDisplayStream()
    :Base()
    ,stream_ref(NULL)
  {
    dq = dispatch_queue_create("com.roxlu.screengrabber", DISPATCH_QUEUE_SERIAL);
  }

  int ScreenCaptureDisplayStream::init() {

    /* Fill our cached list of displays. */
    CGDirectDisplayID display_ids[10];
    uint32_t found_displays = 0;
    CGError err = CGGetActiveDisplayList(10, display_ids, &found_displays);

    if (kCGErrorSuccess != err) {
      printf("Error: failed to retrieve a list of active displays in `ScreenCaptureDisplayStream`.\n");
      return -1;
    }

    if (0 == found_displays) {
      printf("Error: we didn't find any active display.\n");
      return -2;
    }
    
    for (uint32_t i = 0; i < found_displays; ++i) {
      Display* display = new Display();
      ScreenCaptureDisplayStreamDisplayInfo* info = new ScreenCaptureDisplayStreamDisplayInfo();
      std::stringstream ss;
      ss << "Monitor " << i;
      info->id = display_ids[i];
      display->info = (void*)info;
      display->name = ss.str();
      displays.push_back(display);
    }
    
    return 0;
  }

  int ScreenCaptureDisplayStream::shutdown() {

    /* Release the stream. */
    if (NULL != stream_ref) {
      CFRelease(stream_ref);
      stream_ref = NULL;
    }

    /* Clear our cached list of displays. */
    for (size_t i = 0; i < displays.size(); ++i) {
      
      ScreenCaptureDisplayStreamDisplayInfo* info = static_cast<ScreenCaptureDisplayStreamDisplayInfo*>(displays[i]->info);
      if (NULL == info) {
        printf("Error: failed to cast back to `ScreenCaptureDisplayStreamDisplayInfo` in `ScreenCaptureDisplayStream`. Not supposed to happen. Leaking memory");
      }
      else {
        delete info;
        info = NULL;
      }
      
      delete displays[i];
      displays[i] = NULL;
    }

    displays.clear();
    
    return 0;
  }

  int ScreenCaptureDisplayStream::configure(Settings settings) {

    /* Get the implementation specific display id. */
    ScreenCaptureDisplayStreamDisplayInfo* info = static_cast<ScreenCaptureDisplayStreamDisplayInfo*>(displays[settings.display]->info);
    if (NULL == info) {
      printf("Error: failed to cast the display info of stream screen capture. Failed to setup\n");
      return -1;
    }

    /* Already a stream? Reset. */
    if (NULL != stream_ref) {
      if (0 == isStarted()) {
        if (0 != stop()) {
          printf("We're reconfiguring/setupping the display stream but we were captureing; stopping failed. Not supposed to happen; and you may leak memory here. We continue capturing though.\n");
        }
      }
      
      state &= ~SC_STATE_STARTED;
      state |= SC_STATE_STOPPED;
      
      CFRelease(stream_ref);
      stream_ref = NULL;
    }

    uint32_t pixel_format = 0;
    switch (settings.pixel_format) {
      case SC_420F: {
        pixel_format = '420f';
        break;
      }
      case SC_420V: {
        pixel_format = '420v';
        break;
      }
      case SC_BGRA: {
        pixel_format = 'BGRA';
        break;
      }
      case SC_L10R: {
        pixel_format = 'l10r';
        break;
      }
      default: {
        printf("Error: unsupported pixel format; cannot configure display stream.\n");
        return -2;
      }
    }

    __block PixelBuffer pixel_buffer;
    if (0 != pixel_buffer.init(settings.output_width, settings.output_height, settings.pixel_format)) {
      printf("Error: failed to setup the the pixel format.\n");
      return -3;
    }

    /* @todo > WE DON'T WANT TO MAKE THIS THE RESPONSIBILITY OF AN IMPLEMENTATION! */
    pixel_buffer.user = user;

    /* @todo make some settings available through API. */
    void* keys[2];
    void* values[2];
    CFDictionaryRef opts;
    keys[0] = (void *) kCGDisplayStreamShowCursor;
    values[0] = (void *) kCFBooleanTrue;
    opts = CFDictionaryCreate(kCFAllocatorDefault, (const void **) keys, (const void **) values, 1, NULL, NULL);

    /* 
       UPDATE USING THIS CLEAN CODE: https://gist.github.com/roxlu/60f2f635347863d6384d 
       TEST IF UPDATING THE CHANGED RECTS ONLY IS FASTER
     */
    
    stream_ref = CGDisplayStreamCreateWithDispatchQueue(info->id,
                                                        settings.output_width,
                                                        settings.output_height,
                                                        pixel_format,
                                                        opts,
                                                        dq,
                                                        ^(CGDisplayStreamFrameStatus status,    /* kCGDisplayStreamFrameComplete, *FrameIdle, *FrameBlank, *Stopped */
                                                          uint64_t time,                        /* Mach absolute time when the event occurred. */
                                                          IOSurfaceRef frame,                   /* opaque pixel buffer, can be backed by GL, CL, etc.. This may be NULL in some cases. See the docs if you want to keep access to this. */
                                                          CGDisplayStreamUpdateRef ref)         
                                                        {
                                                          if (kCGDisplayStreamFrameStatusFrameComplete == status
                                                             && NULL != frame)
                                                            {

                                                              /* 
                                                                 n.b. when the user needs to handle the pixel buffer after the call, it needs to make 
                                                                 a copy. CGDisplayStream* supports a feature where you can increment the in-use-count, see
                                                                 the documention: https://developer.apple.com/library/mac/documentation/GraphicsImaging/Reference/Quartz_Services_Ref/index.html#//apple_ref/doc/c_ref/CGDisplayStreamFrameAvailableHandler
                                                              */
                                                              
                                                              IOSurfaceLock(frame, kIOSurfaceLockReadOnly, NULL);

                                                              size_t plane_count = IOSurfaceGetPlaneCount(frame);
                                                              
                                                              if (2 == plane_count) {
                                                                pixel_buffer.plane[0] = (uint8_t*)IOSurfaceGetBaseAddressOfPlane(frame, 0);
                                                                pixel_buffer.stride[0] = IOSurfaceGetBytesPerRowOfPlane(frame, 0);
                                                                pixel_buffer.plane[1] = (uint8_t*)IOSurfaceGetBaseAddressOfPlane(frame, 1);
                                                                pixel_buffer.stride[1] = IOSurfaceGetBytesPerRowOfPlane(frame, 1);
                                                              }
                                                              else if (0 == plane_count) {
                                                                pixel_buffer.plane[0] = (uint8_t*)IOSurfaceGetBaseAddress(frame);
                                                                pixel_buffer.stride[0] = IOSurfaceGetBytesPerRow(frame);
                                                              }
                                                              else {
                                                                printf("Error: unsupported plane count in the displaystream capture. Cannot setup the pixel buffer. Exiting now.\n");
                                                                exit(EXIT_FAILURE);
                                                              }
                                                              
                                                              callback(pixel_buffer);

                                                              IOSurfaceUnlock(frame, kIOSurfaceLockReadOnly, NULL);
                                                            }
                                                        }
                                                        );
    if (NULL == stream_ref) {
      printf("Error: failed to create a display stream that we use to capture the screen.\n");
      return -4;
    }
    
    return 0;
  }

  int ScreenCaptureDisplayStream::start() {

    CGError err = CGDisplayStreamStart(stream_ref);

    if (kCGErrorSuccess != err) {
      printf("Error: failed to start the display stream capturer. CGDisplayStreamStart failed: %d .\n", err);
      return -1;
    }
    
    return 0;
  }

  int ScreenCaptureDisplayStream::stop() {
    
    CGError err = CGDisplayStreamStop(stream_ref);
    
    if (kCGErrorSuccess != err) {
      printf("Error: failed to stop the display stream capturer. CGDisplayStreamStart failed: %d .\n", err);
      return -1;
    }
    
    return 0;
  }

  int ScreenCaptureDisplayStream::getDisplays(std::vector<Display*>& result) {
    result = displays;
    return 0;
  }

  int ScreenCaptureDisplayStream::getPixelFormats(std::vector<int>& formats) {
    
    formats.clear();
    
    formats.push_back(SC_420V);
    formats.push_back(SC_420F);
    formats.push_back(SC_BGRA);
    formats.push_back(SC_L10R);

    return 0;
  }

}; /* namespace sc */



