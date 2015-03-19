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


  Screen Capture Display Stream
  =============================

  Screen Capture driver for Mac using the `CGDisplayStream*` API.
  See `Base.h` for more info on the meaning of the functions. 

 */
#ifndef SCREEN_CAPTURE_DISPLAY_STREAM_H
#define SCREEN_CAPTURE_DISPLAY_STREAM_H

#include <stdint.h>
#include <vector>

#include <CoreGraphics/CGDisplayStream.h>
#include <screencapture/Types.h>
#include <screencapture/Base.h>

namespace sc {

  /* ----------------------------------------------------------- */
  
  struct ScreenCaptureDisplayStreamDisplayInfo {
    CGDirectDisplayID id;
  };
  
  /* ----------------------------------------------------------- */
  
  class ScreenCaptureDisplayStream : public Base {

  public:
    /* Allocation */
    ScreenCaptureDisplayStream();
    int init();
    int shutdown();

    /* Control */
    int configure(Settings settings);
    int start();
    void update() { } 
    int stop();

    /* Features */
    int getDisplays(std::vector<Display*>& result);
    int getPixelFormats(std::vector<int>& formats);
    
  public:
    dispatch_queue_t dq;
    CGDisplayStreamRef stream_ref;
    std::vector<Display*> displays;
  };
  
}; /* namespace sc */

#endif
