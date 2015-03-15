/*

What this code is about:

      This code example explores the screen capture API on Mac. It seems
      that we can either use AVFoundation or CGDsiplayStream* . The CGDisplayStream*
      is promoted by the `WWDC2012 506` video and seems to be -the- solution for 
      high performance screenccapture where color conversion, scaling is done on the gpu. 

Loose remarks:

      - it looks like We need to create a dispatch_queue which is serial (instead of concurrent)
      - there are a couple of different output formats (we want 420 full video range) see docs of `CGDisplayStreamCreate`

Frame callback:

        typedef void (^CGDisplayStreamFrameAvailableHandler)(
           CGDisplayStreamFrameStatus status,
           uint64_t displayTime,
           IOSurfaceRef frameSurface,
           CGDisplayStreamUpdateRef updateRef
        );

IOSurface info in your callback:

        This contains the pixels from the display. If you want to keep it for a longer period 
        then the callback you need to:

          CFRetain()
          IOSurfaceIncrementUseCount()
          ..
          when ready ..
          ...
          CFRelease()
          IOSurfaceDecrementUseCount()
        

CGDisplayStreamCreate* properties:

        const CFStringRef kCGDisplayStreamSourceRect;
        const CFStringRef kCGDisplayStreamDestinationRect;
        const CFStringRef kCGDisplayStreamPreserveAspectRatio;
        const CFStringRef kCGDisplayStreamColorSpace;
        const CFStringRef kCGDisplayStreamMinimumFrameTime;
        const CFStringRef kCGDisplayStreamShowCursor;
        const CFStringRef kCGDisplayStreamQueueDepth;
        const CFStringRef kCGDisplayStreamYCbCrMatrix;


 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CoreGraphics/CGDisplayStream.h>

int main() {
  printf("\n\ntest_api_research\n\n");

  size_t output_width = 1280;
  size_t output_height = 720;
  uint32_t pixel_format = '420f';
  pixel_format = 'BGRA';
  dispatch_queue_t dq = dispatch_queue_create("com.domain.screengrabber", DISPATCH_QUEUE_SERIAL);

  /* List displays. */
  CGDirectDisplayID display_ids[5]; /* just a typedef'd uint32_t */
  uint32_t found_displays = 0;
  CGError err = CGGetActiveDisplayList(5, display_ids, &found_displays); /* more values at: https://developer.apple.com/library/mac/documentation/CoreGraphics/Reference/CoreGraphicsConstantsRef/index.html#//apple_ref/c/tdef/CGError */

  if (kCGErrorSuccess != err) {
    printf("Error: failed to retrieve displays.\n");
    exit(EXIT_FAILURE);
  }

  if (0 == found_displays) {
    printf("Error: no active displays found.\n");
    exit(EXIT_FAILURE);
  }
  
  for (uint32_t i = 0; i < found_displays; ++i) {
    printf("Display: #%u, id: %u\n", i, display_ids[i]);
  }
  
  CGDisplayStreamRef sref;
  __block uint64_t prev_time = 0; /* because this variable is used in a dispatch_queue we need to use __block because we change it's value inside the block. */

  sref = CGDisplayStreamCreateWithDispatchQueue(display_ids[0],
                                                output_width,
                                                output_height,
                                                pixel_format,
                                                NULL,
                                                dq,
                                                ^(CGDisplayStreamFrameStatus status,    /* kCGDisplayStreamFrameComplete, *FrameIdle, *FrameBlank, *Stopped */
                                                  uint64_t time,                        /* Mach absolute time when the event occurred. */
                                                  IOSurfaceRef frame,                   /* opaque pixel buffer, can be backed by GL, CL, etc.. This may be NULL in some cases. See the docs if you want to keep access to this. */
                                                  CGDisplayStreamUpdateRef ref)         
                                                {
                                                  printf("Got frame: %llu, ", time);
                                                  
                                                  if (0 != prev_time) {
                                                    uint64_t d = time - prev_time;
                                                    printf("took: %llu ms.", (d / 1000000ull));
                                                  }
                                                  
                                                  if (kCGDisplayStreamFrameStatusFrameComplete == status
                                                      && NULL != frame)
                                                    {

                                                      /*
                                                        IMPORTANT: w/o locking the surface the pixel buffer
                                                         seems to be overwritten by the OS, therefore we lock 
                                                         it.
                                                      */
                                                      IOSurfaceLock(frame, kIOSurfaceLockReadOnly, NULL);
                                                      
                                                      uint8_t* pix = (uint8_t*)IOSurfaceGetBaseAddress(frame);
                                                      if (NULL != pix) {
                                                        /* Now you can use the pixels. */
                                                      }
                                                      
                                                      IOSurfaceUnlock(frame, kIOSurfaceLockReadOnly, NULL);
                                                    }
                                                  printf("\n");
                                                  prev_time = time;
                                                }
                                                );


  err = CGDisplayStreamStart(sref);
  
  if (kCGErrorSuccess != err) {
    printf("Error: failed to start streaming the display.\n");
    exit(EXIT_FAILURE);
  }
  
  while (true) {
    usleep(1e5);
  }

  err = CGDisplayStreamStop(sref);
  if (kCGErrorSuccess != err) {
    printf("Error: failed to stop streaming the display.\n");
    exit(EXIT_FAILURE);
  }
  
  printf("\n\n");
  return 0;
}



