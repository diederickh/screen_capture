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
  ${sd}/Utils.cpp
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
elseif(WIN32)

  list(APPEND screencapture_lib_sources
    ${sd}/win/ScreenCaptureDuplicateOutputDirect3D11.cpp
    ${sd}/win/ScreenCaptureRendererDirect3D11.cpp
    ${sd}/win/ScreenCapturePointerDirect3D11.cpp
    ${sd}/win/ScreenCaptureUtilsDirect3D11.cpp
    )
  
  find_library(lib_dxgi dxgi.lib)
  find_path(d3d Include/d3d10.h)

  set(app_libs
    dxgi.lib
    d3d11.lib
    d3dcompiler.lib
    opengl32.lib
    ${EXTERN_LIB_DIR}/glfw3.lib
    ${EXTERN_LIB_DIR}/libpng16_static.lib
    ${EXTERN_LIB_DIR}/zlibstatic.lib
    )
endif()

add_library(screencapture${debug_flag} ${screencapture_lib_sources})
install(TARGETS screencapture${debug_flag} ARCHIVE DESTINATION lib)

macro(create_test name fname params)
  set(test_name "test_${name}${debug_flag}")
  add_executable(${test_name} ${params} ${sd}/test/test_${fname})
if (APPLE)
  target_link_libraries(${test_name} screencapture${debug_flag} ${app_libs})
  add_dependencies(${test_name} screencapture${debug_flag})
else()
  target_link_libraries(${test_name} ${app_libs} screencapture${debug_flag})
endif()
  install(TARGETS ${test_name} DESTINATION bin/)
endmacro()

#create_test(mac_api_research "mac_api_research.m")
#create_test(mac_screencapture_console "mac_screencapture_console.cpp")
create_test(opengl "opengl.cpp;${EXTERN_SRC_DIR}/glad.c" "")
#create_test(math "math.cpp")
#create_test(win_api_directx_research "win_api_directx_research.cpp" "")
#create_test(win_directx "win_directx.cpp" WIN32)
#create_test(api "api.cpp" "")
#create_test(win_api "win_api" WIN32)
#install(FILES ${sd}/test/test_win_directx_shader.hlsl DESTINATION bin)install(FILES ${sd}/test/test_win_directx_shader.hlsl DESTINATION bin)
