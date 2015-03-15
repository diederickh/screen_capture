
# Screen Capture

This repository contains a cross platform Screen Capture Library currently
in development. The API is likely to change in the near future when more
drivers (Windows / Linux) will be added. This is experimental code.

## Roadmap

Currently I'm working on driver for Windows and Linux. The Mac
driver has been tested on just two Macs, a 10.9.5 and 10.10.2. Please let
me know when you've compiled and run this library successfully.

## Dependencies

 - Install CMake with command line support. On Mac, after installing CMake, make sure
   that you can execute CMake from the terminal. To enable this feature you need to
   open a terminal and then execute `sudo /Applications/CMake.app/Contents/bin/cmake-gui`,
   then from the menu choose `Tools > Install for command line use`.

 - A working C/C++ compiler :) 


## Compiling on Mac

Execute the following in a terminal which will start the opengl 
test application that captures from the first display:

````sh
cd build
./release 64
````


  
      