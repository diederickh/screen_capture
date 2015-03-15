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

  Base
  =====

  Each `ScreenCapture` driver needs to implement this class.

 */
#ifndef SCREEN_CAPTURE_BASE_H
#define SCREEN_CAPTURE_BASE_H

#include <stdio.h>
#include <vector>
#include <screencapture/Types.h>

namespace sc {

  /* ----------------------------------------------------------- */
  
  class Base {
  public:
    Base();
    virtual ~Base();

    /* Control */
    virtual int init() = 0;                                      /* Initialize the screencapturer; should be used as a constructor. */
    virtual int shutdown() = 0;                                  /* Shutdown the screencapturer; release all allocated memory and reset state as it was before init. Should be used as a destructor*/
    virtual int configure(Settings settings) = 0;                /* Configure the capturer using the provided settings. */
    virtual int start() = 0;                                     /* Should start capturing. Can be called multiple times when init() has been called. Note that before calling start, the user needs to call stop(). */
    virtual int stop() = 0;                                      /* Should stop capturing. "" "" "" */

    /* Features */
    virtual int getDisplays(std::vector<Display*>& result) = 0;  /* Get the available / active displays. The implementation is responsible for freeing the allocated displays in `shutdown()`. The info member of the `Display` struct can be set to anything implementation related. */
    virtual int getPixelFormats(std::vector<int>& formats) = 0;  /* Get the supported output pixel formats that the capturer can use. */                             
    virtual int canChangeCursorVisibility();                     /* Should returns 0 when we can hide/show the cursor. */

    /* State  */
    int isConfigured();                                          /* Returns 0 when the capturer is configured, otherwise -1. */
    int isInit();                                                /* Returns 0 when initialised, otherwise -1 */
    int isStarted();                                             /* Returns 0 when capture has started, otherwise -1. */
    int isStopped();                                             /* Returns 0 when we're stopped, otherwise -1. */
    int isShutdown();                                            /* Returns 0 when shutdown otherwise -1. */

    /* Utils. */
    int setCallback(screencapture_callback callback, void* user);

  public:
    unsigned int state;                                          /* Are we initialized, started, stopped, shutdown? */
    screencapture_callback callback;                             /* The screencapture callback which should be called whenever a new frame is received. */
    void* user;                                                  /* A user pointer which must be set on the PixelBuffer you pass into the callback. */
  };

  /* ----------------------------------------------------------- */

  inline int Base::setCallback(screencapture_callback cb, void* u) {
    
    if (NULL == cb) {
      printf("Error: cannot set screen capture callback because it's NULL.\n");
      return -1;
    }

    callback = cb;
    user = u;

    return 0;
  }

  inline int Base::isInit() {
    return (state & SC_STATE_INIT)? 0 : - 1;
  }

  inline int Base::isShutdown() {
    return (state & SC_STATE_SHUTDOWN) ? 0 : - 1;
  }

  inline int Base::isConfigured() {
    return (state & SC_STATE_CONFIGURED) ? 0 : -1;
  }

  inline int Base::isStarted() {
    return (state & SC_STATE_STARTED) ? 0 : - 1;
  }
  
  inline int Base::isStopped() {
    return (state & SC_STATE_STOPPED) ? 0 : - 1;
  }

  inline int Base::canChangeCursorVisibility() {
    return -1;
  }
    


  /* ----------------------------------------------------------- */
  
} /* namespace sc */

#endif
