/*

  Mac Console ScreenCapture
  -------------------------
  
  Although this is not really Mac specific, it's used to 
  test and develop the Mac screen grabber. 

 */
#define ROXLU_USE_PNG
#define ROXLU_USE_JPG
#define ROXLU_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <tinylib.h>
#include <screencapture/ScreenCapture.h>

using namespace sc;

static void on_frame(PixelBuffer& buffer);

int main() {
  
  printf("\n\ntest_mac_screencapture_console\n\n");

  std::vector<Display*> displays;

  ScreenCapture capture(on_frame);

  if (0 != capture.init()) {
    exit(EXIT_FAILURE);
  }
  
  if (0 != capture.listDisplays()) {
    exit(EXIT_FAILURE);
  }

  if (0 != capture.getDisplays(displays)) {
    exit(EXIT_FAILURE);
  }

  if (0 != capture.listPixelFormats()) {
    exit(EXIT_FAILURE);
  }

  if (0 != capture.isPixelFormatSupported(SC_420F)) {
    printf("It seems that the pixel format SC_420F is not supported. This test expects that it's supported.\n");
    exit(EXIT_FAILURE);
  }

  settings.pixel_format = SC_420F;
  settings.pixel_format = SC_BGRA;
  settings.display = 0;
  settings.output_width = 1440; // 1280;
  settings.output_height = 810; // 720;

  if (0 != capture.configure(settings)) {
    exit(EXIT_FAILURE);
  }

  if (0 != capture.start()) {
    exit(EXIT_FAILURE);
  }

  int count = 0;
  while(true) {
    printf("Looping...%d\n", count);
    SLEEP_MILLIS(200);
    if (5 == count) {
      printf("------ Stopping...\n");
      capture.stop();
    }
    else if(10 == count) {
      printf("------ Starting again...\n");
      capture.start();
    }
    else if(65 == count) {
      printf("------- Stopping loop...\n");
      break;
    }
    ++count;
  }

  if (0 != capture.stop()) {
    exit(EXIT_FAILURE);
  }
  
  printf("Stopped.\n");

  if (0 != capture.shutdown()) {
    exit(EXIT_FAILURE);
  }

  printf("\n");
  
  return 0;
}
  
static void on_frame(PixelBuffer& buffer) {

  static int framecount = 0;
  static uint8_t* pix = NULL;
  
  if (NULL == pix) {
    pix = new uint8_t[buffer.width * buffer.height * 4];
    memset(pix, 0xFF, buffer.width * buffer.height * 4);
  }
  
  char filename[512];
  sprintf(filename, "screencapture_%02d.png", framecount);
          
  if (SC_BGRA == buffer.pixel_format) {
    
    if (NULL == buffer.plane[0]) {
      printf("Error: failed to find the pixel data.\n");
      exit(EXIT_FAILURE);
    }
#if 0
#if 1    
    for (int i = 0; i < buffer.width; ++i) {
      for (int j = 0; j < buffer.height; ++j) {
        int dx = j * buffer.width * 4 + i * 4;

        pix[dx + 0] = buffer.plane[0][dx + 2];
        pix[dx + 1] = buffer.plane[0][dx + 1];
        pix[dx + 2] = buffer.plane[0][dx + 0];
        //        pix[dx + 2] = 255;
        pix[dx + 3] =  buffer.plane[0][dx + 3];
      }
    }
#else
    memcpy(pix, buffer.plane[0], buffer.width * buffer.height * 4);
#endif
    
    for (int i = 0; i < 10; ++i) {
      printf("%02X ", buffer.plane[0][i]);
    }
    
#if 1
    if (false == rx_save_png(rx_to_data_path(filename), pix, buffer.width, buffer.height, 4)) {
    //    if (false == rx_save_png(rx_to_data_path(filename), buffer.plane[0], buffer.width, buffer.height, 4)) {
      printf("Error: Failed to write a png.\n");
    }
#endif
#else
    for (int i = 0; i < 10; ++i) {
      printf("%02X ", buffer.plane[0][i]);
    }
    printf("\n");
#endif    

  }
  else if (SC_420F == buffer.pixel_format) {
    /*
    if (false == rx_save_jpg(rx_to_data_path(filename), buffer.plane[0], buffer.width, buffer.height, 3, JCS_YCbCr)) {
      printf("Error: Failed to write a jpg.\n");
    }

    printf("Wrote: %s\n", filename);
    */
  }

  ++framecount;

}
