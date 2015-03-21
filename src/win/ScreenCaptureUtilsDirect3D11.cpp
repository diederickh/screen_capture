#include <screencapture/win/ScreenCaptureUtilsDirect3D11.h>

namespace sc {

  std::string hresult_to_string(HRESULT hr) {
    
    switch (hr) {
      
      case D3D11_ERROR_FILE_NOT_FOUND:                                { return "D3D11_ERROR_FILE_NOT_FOUND";                                        }
      case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:                 { return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";                         }
      case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:                  { return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";                          }       
      case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:  { return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";          }
      case DXGI_ERROR_INVALID_CALL:                                   { return "DXGI_ERROR_INVALID_CALL";                                           }
      case DXGI_ERROR_WAS_STILL_DRAWING:                              { return "DXGI_ERROR_WAS_STILL_DRAWING";                                      }
      case E_FAIL:                                                    { return "E_FAIL";                                                            }
      case E_INVALIDARG:                                              { return "E_INVALIDARG";                                                      }
      case E_OUTOFMEMORY:                                             { return "E_OUTOFMEMORY";                                                     }
      case E_NOTIMPL:                                                 { return "E_NOTIMPL";                                                         }
      case S_FALSE:                                                   { return "S_FALSE";                                                           }
      case S_OK:                                                      { return "S_OK";                                                              }  

      default: {
        std::stringstream ss;
        ss << std::hex << (unsigned int)hr;
        return "UNKNOWN HRESULT: " +ss.str();
      }
    }
  }

  std::vector<std::string> get_d3d11_bind_flags(UINT flag) {
    std::vector<std::string> result;

    if (flag & D3D11_BIND_VERTEX_BUFFER) {
      result.push_back("D3D11_BIND_VERTEX_BUFFER");
    }
    
    if (flag & D3D11_BIND_INDEX_BUFFER) {
      result.push_back("D3D11_BIND_INDEX_BUFFER");
    }
    
    if (flag & D3D11_BIND_CONSTANT_BUFFER) {
      result.push_back("D3D11_BIND_CONSTANT_BUFFER");
    }
    
    if (flag & D3D11_BIND_SHADER_RESOURCE) {
      result.push_back("D3D11_BIND_SHADER_RESOURCE");
    }
    
    if (flag & D3D11_BIND_STREAM_OUTPUT) {
      result.push_back("D3D11_BIND_STREAM_OUTPUT");
    }
    
    if (flag & D3D11_BIND_RENDER_TARGET) {
      result.push_back("D3D11_BIND_RENDER_TARGET");
    }
    
    if (flag & D3D11_BIND_DEPTH_STENCIL) {
      result.push_back("D3D11_BIND_DEPTH_STENCIL");
    }
    
    if (flag & D3D11_BIND_UNORDERED_ACCESS) {
      result.push_back("D3D11_BIND_UNORDERED_ACCESS");
    }
    
    if (flag & D3D11_BIND_DECODER) {
      result.push_back("D3D11_BIND_DECODER");
    }
    
    if (flag & D3D11_BIND_VIDEO_ENCODER) {
      result.push_back("D3D11_BIND_VIDEO_ENCODER");
    }
    
    return result;
  }

  void print_d3d11_texture2d_info(ID3D11Texture2D* tex) {
    
    if (NULL == tex) {
      printf("Error: requested ot print texture info but you gave us a NULL ptr.\n");
      return;
    }

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);

    printf("texture.Width: %u\n", desc.Width);
    printf("texture.Height: %u\n", desc.Height);
    printf("texture.MipLevels: %u\n", desc.MipLevels);
    printf("texture.ArraySize: %u\n", desc.ArraySize);
    printf("texture.Format: %u\n", desc.Format);

    printf("texture.Usage: ");
    if (D3D11_USAGE_DEFAULT == desc.Usage) {
      printf("D3D11_USAGE_DEFAULT");
    }
    else if (D3D11_USAGE_IMMUTABLE == desc.Usage) {
      printf("D3D11_USAGE_IMMUTABLE");
    }
    else if (D3D11_USAGE_DYNAMIC == desc.Usage) {
      printf("D3D11_USAGE_DYNAMIC");
    }
    else if (D3D11_USAGE_STAGING == desc.Usage) {
      printf("D3D11_USAGE_STAGING");
    }
    
    printf("\n");

    printf("texture.BindFlags: \n");    
    std::vector<std::string> binds = get_d3d11_bind_flags(desc.BindFlags);
    for (size_t i = 0; i < binds.size(); ++i) {
      printf("\t- %s\n", binds[i].c_str());
    }

    printf("texture.CPUAccessFlags: %08X", desc.CPUAccessFlags);
    if (desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) {
      printf("D3D11_CPU_ACCESS_READ ");
    }
    if (desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) {
      printf("D3D11_CPU_ACCESS_WRITE ");
    }
    printf("\n");
   
    printf("-\n");
  }
  
} /* namespace sc */
