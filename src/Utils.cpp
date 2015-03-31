#include <stdio.h>
#include <screencapture/Utils.h>

namespace sc {

  void create_ortho_matrix(float l, float r, float b, float t, float n, float f, float* m) {

    m[1]  = 0.0f;
    m[2]  = 0.0f;
    m[3]  = 0.0f;
    m[4]  = 0.0f;
    m[6]  = 0.0f;
    m[7]  = 0.0f;
    m[8]  = 0.0f;
    m[9]  = 0.0f;
    m[11] = 0.0f;
    m[15] = 1.0f;

    float invrl = (r != l) ? 1.0f / (r - l) : 0.0f;
    float invtb = (t != b) ? 1.0f / (t - b) : 0.0f;
    float invfn = (f != n) ? 1.0f / (f - n) : 0.0f;

    m[0] = 2.0f * invrl;
    m[5] = 2.0f * invtb;
    m[10] = -2.0 * invfn;

    m[12] = -(r + l) * invrl;
    m[13] = -(t + b) * invtb;
    m[14] = -(f + n) * invfn;
    m[15] = 1.0f;
  }

  void create_translation_matrix(float x, float y, float z, float* m) {
    m[0] = 1.0f;     m[4] = 0.0f;     m[8]  = 0.0f;    m[12] = x;
    m[1] = 0.0f;     m[5] = 1.0f;     m[9]  = 0.0f;    m[13] = y;
    m[2] = 0.0f;     m[6] = 0.0f;     m[10] = 1.0f;    m[14] = z;
    m[3] = 0.0f;     m[7] = 0.0f;     m[11] = 0.0f;    m[15] = 1.0f;
  }

  void create_identity_matrix(float* m) {
    m[0] = 1.0f;     m[4] = 0.0f;     m[8]  = 0.0f;    m[12] = 0.0f;
    m[1] = 0.0f;     m[5] = 1.0f;     m[9]  = 0.0f;    m[13] = 0.0f;
    m[2] = 0.0f;     m[6] = 0.0f;     m[10] = 1.0f;    m[14] = 0.0f;
    m[3] = 0.0f;     m[7] = 0.0f;     m[11] = 0.0f;    m[15] = 1.0f;
  }

  void print_matrix(float* m) {
    printf("%2.02f, %2.02f, %2.02f, %2.02f\n", m[0], m[4], m[8], m[12]);
    printf("%2.02f, %2.02f, %2.02f, %2.02f\n", m[1], m[5], m[9], m[13]);
    printf("%2.02f, %2.02f, %2.02f, %2.02f\n", m[2], m[6], m[10], m[14]);
    printf("%2.02f, %2.02f, %2.02f, %2.02f\n", m[3], m[7], m[11], m[15]);
    printf("-\n");
  }
  
} /* namespace sc */
