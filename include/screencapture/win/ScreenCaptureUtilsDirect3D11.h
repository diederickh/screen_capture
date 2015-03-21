#ifndef SCREEN_CAPTURE_UTILS_DIRECT3D_H
#define SCREEN_CAPTURE_UTILS_DIRECT3D_H

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <D3D11.h>

namespace sc {

  std::string hresult_to_string(HRESULT hr);
  std::vector<std::string> get_d3d11_bind_flags(UINT flag);
  void print_d3d11_texture2d_info(ID3D11Texture2D* tex);
  
  
} /* namespace sc */
#endif
