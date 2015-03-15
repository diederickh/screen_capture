/*
  -------------------------------------------------------------------------

  Copyright (C) 2015 roxlu <info#AT#roxlu.com> All Rights Reserved.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  
  -------------------------------------------------------------------------

 */

#ifndef SCREEN_CAPTURE_TYPES_H
#define SCREEN_CAPTURE_TYPES_H

#include <stdint.h>
#include <string>

/* Screen Capture Drivers */
#define SC_DISPLAY_STREAM 1

#if defined (__APPLE__)
#  define SC_DEFAULT_DRIVER SC_DISPLAY_STREAM
#endif

/* General "Unset" value */
#define SC_NONE 0

/* Pixel Formats */
#define SC_420V 1                                                /* 2-plane "video" range YCbCr 4:2:0 */
#define SC_420F 2                                                /* 2-plane "full" range YCbCr 4:2:0 */
#define SC_BGRA 3                                                /* Packed Little Endian ARGB8888 */
#define SC_L10R 4                                                /* Packet Little Endian ARGB2101010 */
                                                                         
/* Capture state. */                                                     
#define SC_STATE_INIT          (1 << 0)                          /* Initialised, init() called, memory allocated.  */
#define SC_STATE_CONFIGURED    (1 << 1)                          /* Configured, configure() called. */
#define SC_STATE_STARTED       (1 << 2)                          /* Started, start() called. */
#define SC_STATE_STOPPED       (1 << 3)                          /* Stopped, stop() called. */
#define SC_STATE_SHUTDOWN      (1 << 4)                          /* Shutdown, shutdown() called, memory deallocated. */

namespace sc {

  /* ----------------------------------------------------------- */

  std::string screencapture_pixelformat_to_string(int format);

  /* ----------------------------------------------------------- */
  
  class PixelBuffer;
  
  typedef void(*screencapture_callback)(PixelBuffer& buffer);

  /* ----------------------------------------------------------- */

  class PixelBuffer {
  public:
    PixelBuffer();                                               /* Initializes; resets all members. */
    ~PixelBuffer();                                              /* Cleans up, resets all members. */
    int init(int w, int h, int fmt);                             /* Sets the given width, height and pixel format members. */
    
  public:
    int pixel_format;                                            /* The pixel format; should be the same as the requested pixel format you pass to the `configure()` function of the screen capture instance. */
    uint8_t* plane[3];                                           /* Pointers to the base addresses where you can find the pixel data. When we have non-planar data, plane[0] will point to the pixels. */
    size_t stride[3];                                            /* Strides per plane. */
    size_t nbytes[3];                                            /* Bytes per plane. */
    size_t width;                                                /* Width of the captured frame. */
    size_t height;                                               /* Height of the captured frame. */
    void* user;                                                  /* User data; set to the user pointer you pass into the capturer. */ 
  };

  /* ----------------------------------------------------------- */
  class Settings {
  public:
    Settings();
    
  public:
    int display;                                                 /* The display number which is shown when you call `listDisplays()`. This is an index into the displays vector you get from `getDisplays()`. */
    int pixel_format;                                            /* The pixel format that you want to use when capturing. */
    int output_width;                                            /* The width for the buffer you'll receive. */
    int output_height;                                           /* The height fr the buffer you'll receive. */
  };

  /* ----------------------------------------------------------- */
  
  struct Display {
    std::string name;                                            /* Human readable name of the display. Set by the driver. */
    void* info;                                                  /* Opaque platform, iplementation specifc info. */
  };

} /* namespace sc */

#endif
