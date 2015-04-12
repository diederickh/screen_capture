/* -*-c++-*-

   Experimenting with the ScreenCapture using a Win8 Window. 
   ---------------------------------------------------------

   This file contains a test where we use the 'ScreenCapture' API
   to enable screencapturing and create a Windows SDK based window.
   This test was created so we can inspect the D3D11 context using 
   e.g. RenderDoc. It seems that the test in 'test_api.cpp' makes
   RenderDoc crash; propably because it's a pure console app. 
  

*/

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>
#include <screencapture/ScreenCapture.h>

/* ------------------------------------------------------------------------------------------------ */
static void frame_callback(sc::PixelBuffer& buf);
LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
/* ------------------------------------------------------------------------------------------------ */

int WINAPI WinMain(HINSTANCE hInstance,
                HINSTANCE hPrevInstance,
                LPSTR lpCmdLine,
                int nCmdShow)  
{

  HWND wnd;
  WNDCLASSEX wc;

  /* Define our window props. */
  ZeroMemory(&wc, sizeof(wc));

  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
  wc.lpszClassName = "WindowClass1";

  RegisterClassEx(&wc);

  /* 
     We use 'AdjustWindowRect' to make sure that the 
     client size will be what we requested. The windows (including
     the border may be a bit bigger).
  */
  RECT wr = { 0, 0, 800, 600 };
  AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
  
  /* Create the window. */
  wnd = CreateWindowEx(NULL,
                       "WindowClass1",                            /* Name of the window class. */
                       "Windows 8.1 based Screen Capure",         /* Title. */
                       WS_OVERLAPPEDWINDOW,                       /* Window style. */
                       10,                                        /* X position. */
                       10,                                        /* Y position. */
                       wr.right - wr.left,                        /* Width. */
                       wr.bottom - wr.top,                        /* Height. */
                       NULL,                                      /* Parent window. */
                       NULL,                                      /* No menus. */
                       hInstance,                                 /* Application handle. */
                       NULL);                                     /* Multiple windows. */

  /* And show it! */
  ShowWindow(wnd, nCmdShow);

  /* Init the screen capture. */
  sc::ScreenCapture capture(frame_callback);
  sc::Settings settings;

  settings.pixel_format = SC_BGRA;
  settings.display = 0;
  settings.output_width = 1280;
  settings.output_height = 720;
  
  if (0 != capture.init()) {
    exit(EXIT_FAILURE);
  }

  if (0 != capture.configure(settings)) {
    exit(EXIT_FAILURE);
  }

  if (0 != capture.start()) {
    exit(EXIT_FAILURE);
  }
  
  /* Main loop. */
  MSG msg;
  while (true) { 
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg); /* Translates keystroke messages into the right format */
      DispatchMessage(&msg);  /* Sends the messages to the WindProc function. */
      if (WM_QUIT == msg.message) {
        break;
      }
    }
    capture.update();    
    //    renderFrame();
  }
  
  capture.shutdown();
  
  return 0;
}

LRESULT CALLBACK WindowProc(HWND wnd,
                            UINT message, 
                            WPARAM wparam,
                            LPARAM lparam)
{
  switch (message) {
    case WM_DESTROY: {
      PostQuitMessage(0);
      return 0;
    }
    default: {
      printf("Warning: unhandled message %u\n", message);
      break;
    }
  }

  /* Pass on messages we didn't handle. */
  return DefWindowProc(wnd, message, wparam, lparam);
}

static void frame_callback(sc::PixelBuffer& buf) {

  printf("Got frame!\n");
}
