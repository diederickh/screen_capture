#include <stdlib.h>
#include <stdio.h>
#include <screencapture/ScreenCapture.h>

static void frame_callback(sc::PixelBuffer& buf);

int main() {

  printf("\n\ntest_api\n\n");
  sc::ScreenCapture capture(frame_callback);
  std::vector<sc::Display*> displays;
  sc::Settings settings;

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

  settings.pixel_format = SC_BGRA;
  settings.display = 0;
  settings.output_width = 1280;
  settings.output_height = 720;

  if (0 != capture.configure(settings)) {
    exit(EXIT_FAILURE);
  }

  if (0 != capture.start()) {
    exit(EXIT_FAILURE);
  }

  while (true) {
    capture.update();
  }

  capture.shutdown();
  
  return 0;
}

static void frame_callback(sc::PixelBuffer& buf) {
  printf("- PixelBuffer.\n");
}
