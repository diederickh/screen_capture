cmake_minimum_required(VERSION 2.8)
project(screencapture)

message(STATUS "EXTERN_LIB_DIR: ${EXTERN_LIB_DIR}")
message(STATUS "EXTERN_INC_DIR: ${EXTERN_INC_DIR}")
message(STATUS "EXTERN_SRC_DIR: ${EXTERN_SRC_DIR}")
message(STATUS "TINYLIB_DIR: ${TINYLIB_DIR}")

set(bd ${CMAKE_CURRENT_LIST_DIR}/../)
set(sd ${bd}/src)
set(id ${bd}/include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(debug_flag "_debug")
endif()

include_directories(
  ${id}
  ${EXTERN_INC_DIR}
  ${TINYLIB_DIR}/src
  )

set(screencapture_lib_sources
  ${sd}/ScreenCapture.cpp
  ${sd}/Base.cpp
  ${sd}/Types.cpp
  )


if (APPLE)

  find_library(fr_corefoundation CoreFoundation)
  find_library(fr_iokit IOKit)
  find_library(fr_opengl OpenGL)
  find_library(fr_corevideo CoreVideo)
  find_library(fr_coregraphics CoreGraphics)
  find_library(fr_cocoa Cocoa)
  find_library(fr_iosurface IOSurface)

  list(APPEND screencapture_lib_sources
    ${sd}/mac/ScreenCaptureDisplayStream.mm
    )
  
  set(app_libs
    ${fr_iokit}
    ${fr_opengl}
    ${fr_corevideo}
    ${fr_coregraphics}
    ${fr_cocoa}
    ${fr_iosurface}
    ${EXTERN_LIB_DIR}/libpng.a
    ${EXTERN_LIB_DIR}/libjpeg.a
    ${EXTERN_LIB_DIR}/libz.a
    ${EXTERN_LIB_DIR}/libglfw3.a
    )
  
endif()

add_library(screencapture${debug_flag} ${screencapture_lib_sources})
install(TARGETS screencapture${debug_flag} ARCHIVE DESTINATION lib)

macro(create_test name fname)
  set(test_name "test_${name}${debug_flag}")
  add_executable(${test_name}  ${sd}/test/test_${fname})
  target_link_libraries(${test_name} screencapture${debug_flag} ${app_libs})
  install(TARGETS ${test_name} DESTINATION bin/)
  add_dependencies(${test_name} screencapture${debug_flag})
endmacro()

create_test(mac_api_research "mac_api_research.m")
create_test(mac_screencapture_console "mac_screencapture_console.cpp")
create_test(opengl "opengl.cpp;${EXTERN_SRC_DIR}/glad.c")
create_test(math "math.cpp")