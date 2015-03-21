#ifndef SCREEN_CAPTURE_UTILS_H
#define SCREEN_CAPTURE_UTILS_H

namespace sc {

  void create_identity_matrix(float* m);
  void create_ortho_matrix(float l, float r, float b, float t, float n, float f, float* m);    /* e.g.   create_ortho_matrix(0.0f, width, height, 0.0f, 0.0f, 100.0f, ortho); */
  void create_translation_matrix(float x, float y, float z, float* m);
  void print_matrix(float* m);
  
} /* namespace sc */

#endif
