#include <screencapture/Types.h>

namespace sc {

  PixelBuffer::PixelBuffer()
    :pixel_format(SC_NONE)
    ,width(0)
    ,height(0)
    ,user(NULL)
  {
    plane[0] = NULL;
    plane[1] = NULL;
    plane[2] = NULL;
    stride[0] = 0;
    stride[1] = 0;
    stride[2] = 0;
    nbytes[0] = 0;
    nbytes[1] = 0;
    nbytes[2] = 0;
  }

  PixelBuffer::~PixelBuffer() {
    pixel_format = SC_NONE;
    plane[0] = NULL;
    plane[1] = NULL;
    plane[2] = NULL;
    stride[0] = 0;
    stride[1] = 0;
    stride[2] = 0;
    nbytes[0] = 0;
    nbytes[1] = 0;
    nbytes[2] = 0;
    user = NULL;
  }

  int PixelBuffer::init(int w, int h, int fmt) {

    if (w <= 0) {
      printf("Error: initializing a PixelBuffer with a width which is < 0. %d\n", w);
      return -1;
    }

    if (h <= 0) {
      printf("Error: initializing a PixelBuffer with a height which is < 0. %d\n", h);
      return -2;
    }

    if (SC_BGRA == fmt) {
      /* This may be overwritten by the capture driver. */
      nbytes[0] = w * h * 4;
    }
    else {
      printf("Error: pixel buffer has no initialisation for the given format: %s\n", screencapture_pixelformat_to_string(fmt).c_str());
      return -3;
    }
    
    width = w;
    height = h;
    pixel_format = fmt;

    return 0;
  }
  
  Settings::Settings()
    :display(-1)
    ,pixel_format(-1)
    ,output_width(-1)
    ,output_height(-1)
  {
  }

  /* ----------------------------------------------------------- */
  
  std::string screencapture_pixelformat_to_string(int format) {

    switch (format) {
      case SC_420V: { return "SC_420V"; }
      case SC_420F: { return "SC_420F"; }
      case SC_BGRA: { return "SC_BGRA"; }
      case SC_L10R: { return "SC_L10R"; }
      default: { return "UNKNOWN PIXEL FORMAT"; } 
    }
  }

  /* ----------------------------------------------------------- */
    
} /* namespace sc */
