Texture2D tex_source: register(t0);
SamplerState sam_linear: register(s0);

struct VertexInput {
  float4 pos: POSITION;
  float2 tex: TEXCOORD0;
};

struct PixelInput {
  float4 pos: SV_POSITION;
  float2 tex: TEXCOORD0;
};

PixelInput vertex_shader(VertexInput vertex) {
  PixelInput output = (PixelInput) 0;
  output.pos = vertex.pos;
  output.tex = vertex.tex;
  return output;
}

float4 pixel_shader(PixelInput input) : SV_Target {
  //return float4(input.tex.x, input.tex.y, 0.0f, 1.0f);
  return tex_source.Sample(sam_linear, input.tex);
}