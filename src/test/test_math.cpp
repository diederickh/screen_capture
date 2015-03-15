#include <stdio.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------------*/
/* Embeddable Math           .                                                */
/* For latest version see: https://gist.github.com/roxlu/ed69c74b819f4fc8cf29 */
/* ---------------------------------------------------------------------------*/

static void create_ortho_matrix(float l, float r, float b, float t, float n, float f, float* m);
static void create_identity_matrix(float* m);
static void create_translation_matrix(float x, float y, float z, float* m);

int main() {

  /* Create orthographic projection matrix. */
  float ortho[16];
  float width = 1280.0f;
  float height = 720.0f;
  create_ortho_matrix(0.0f, width, height, 0.0f, 0.0f, 100.0f, ortho);

  float ident[16];
  create_identity_matrix(ident);

  float translation[16];
  create_translation_matrix(10.0f, 10.0f, 0.0f, translation);
  
  return 0;
}

static void create_ortho_matrix(float l, float r, float b, float t, float n, float f, float* m) {

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
    
  float rml = r - l;
  float fmn = f - n;
  float tmb = t - b;
    
  m[0]  = 2.0f / rml;
  m[5]  = 2.0f / tmb;
  m[10] = -2.0f / fmn;
  m[12] = -(r + l) / rml;
  m[13] = -(t + b) / tmb;
  m[14] = -(f + n) / fmn;
}

static void create_translation_matrix(float x, float y, float z, float* m) {
  m[0] = 1.0f;     m[4] = 0.0f;     m[8]  = 0.0f;    m[12] = x;
  m[1] = 0.0f;     m[5] = 1.0f;     m[9]  = 0.0f;    m[13] = y;
  m[2] = 0.0f;     m[6] = 0.0f;     m[10] = 1.0f;    m[14] = z;
  m[3] = 0.0f;     m[7] = 0.0f;     m[11] = 0.0f;    m[15] = 1.0f;
}

static void create_identity_matrix(float* m) {
  m[0] = 1.0f;     m[4] = 0.0f;     m[8]  = 0.0f;    m[12] = 0.0f;
  m[1] = 0.0f;     m[5] = 1.0f;     m[9]  = 0.0f;    m[13] = 0.0f;
  m[2] = 0.0f;     m[6] = 0.0f;     m[10] = 1.0f;    m[14] = 0.0f;
  m[3] = 0.0f;     m[7] = 0.0f;     m[11] = 0.0f;    m[15] = 1.0f;
}

