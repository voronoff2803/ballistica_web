#include "Renderer.h"

#include <OpenGLES/ES3/gl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace mat {

void Identity(float* o) {
  memset(o, 0, 16 * sizeof(float));
  o[0] = o[5] = o[10] = o[15] = 1.f;
}

void Multiply(const float* a, const float* b, float* o) {
  float r[16];
  for (int c = 0; c < 4; ++c) {
    for (int rr = 0; rr < 4; ++rr) {
      r[c * 4 + rr] = a[0 * 4 + rr] * b[c * 4 + 0] +
                      a[1 * 4 + rr] * b[c * 4 + 1] +
                      a[2 * 4 + rr] * b[c * 4 + 2] +
                      a[3 * 4 + rr] * b[c * 4 + 3];
    }
  }
  memcpy(o, r, sizeof(r));
}

void Translation(float x, float y, float z, float* o) {
  Identity(o);
  o[12] = x;
  o[13] = y;
  o[14] = z;
}

void Scale(float s, float* o) {
  Identity(o);
  o[0] = o[5] = o[10] = s;
}

void RotationX(float a, float* o) {
  Identity(o);
  float c = cosf(a), s = sinf(a);
  o[5] = c; o[6] = s; o[9] = -s; o[10] = c;
}

void RotationY(float a, float* o) {
  Identity(o);
  float c = cosf(a), s = sinf(a);
  o[0] = c; o[2] = -s; o[8] = s; o[10] = c;
}

void Perspective(float fov, float asp, float zn, float zf, float* o) {
  memset(o, 0, 16 * sizeof(float));
  float f = 1.f / tanf(fov * 0.5f);
  o[0] = f / asp;
  o[5] = f;
  o[10] = (zf + zn) / (zn - zf);
  o[11] = -1.f;
  o[14] = (2.f * zf * zn) / (zn - zf);
}

static void Cross(const float* a, const float* b, float* o) {
  o[0] = a[1] * b[2] - a[2] * b[1];
  o[1] = a[2] * b[0] - a[0] * b[2];
  o[2] = a[0] * b[1] - a[1] * b[0];
}
static void Norm3(float* v) {
  float L = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (L > 0) { v[0] /= L; v[1] /= L; v[2] /= L; }
}

void LookAt(float ex, float ey, float ez, float tx, float ty, float tz,
            float* o) {
  float f[3] = {tx - ex, ty - ey, tz - ez};
  Norm3(f);
  float up[3] = {0, 1, 0};
  float s[3]; Cross(f, up, s); Norm3(s);
  float u[3]; Cross(s, f, u);
  Identity(o);
  o[0] = s[0]; o[4] = s[1]; o[8]  = s[2];
  o[1] = u[0]; o[5] = u[1]; o[9]  = u[2];
  o[2] = -f[0]; o[6] = -f[1]; o[10] = -f[2];
  o[12] = -(s[0] * ex + s[1] * ey + s[2] * ez);
  o[13] = -(u[0] * ex + u[1] * ey + u[2] * ez);
  o[14] =  (f[0] * ex + f[1] * ey + f[2] * ez);
}

// ODE rotation matrix is row-major 3x4 (rotation is 3x3 in leftmost cols).
void FromOdeRotation(const float* R, float* o) {
  Identity(o);
  o[0]  = R[0]; o[4]  = R[1]; o[8]  = R[2];
  o[1]  = R[4]; o[5]  = R[5]; o[9]  = R[6];
  o[2]  = R[8]; o[6]  = R[9]; o[10] = R[10];
}

}  // namespace mat

// ----- Cube mesh: 36 verts, pos (3f) + normal (3f). CCW outward. -----
static const float kCubeVerts[] = {
    // +X (right)
     0.5f,-0.5f,-0.5f,  1, 0, 0,   0.5f,-0.5f, 0.5f,  1, 0, 0,   0.5f, 0.5f, 0.5f,  1, 0, 0,
     0.5f,-0.5f,-0.5f,  1, 0, 0,   0.5f, 0.5f, 0.5f,  1, 0, 0,   0.5f, 0.5f,-0.5f,  1, 0, 0,
    // -X (left)
    -0.5f,-0.5f,-0.5f, -1, 0, 0,  -0.5f, 0.5f,-0.5f, -1, 0, 0,  -0.5f, 0.5f, 0.5f, -1, 0, 0,
    -0.5f,-0.5f,-0.5f, -1, 0, 0,  -0.5f, 0.5f, 0.5f, -1, 0, 0,  -0.5f,-0.5f, 0.5f, -1, 0, 0,
    // +Y (top)
    -0.5f, 0.5f,-0.5f,  0, 1, 0,   0.5f, 0.5f,-0.5f,  0, 1, 0,   0.5f, 0.5f, 0.5f,  0, 1, 0,
    -0.5f, 0.5f,-0.5f,  0, 1, 0,   0.5f, 0.5f, 0.5f,  0, 1, 0,  -0.5f, 0.5f, 0.5f,  0, 1, 0,
    // -Y (bottom)
    -0.5f,-0.5f,-0.5f,  0,-1, 0,   0.5f,-0.5f,-0.5f,  0,-1, 0,   0.5f,-0.5f, 0.5f,  0,-1, 0,
    -0.5f,-0.5f,-0.5f,  0,-1, 0,   0.5f,-0.5f, 0.5f,  0,-1, 0,  -0.5f,-0.5f, 0.5f,  0,-1, 0,
    // +Z (front)
    -0.5f,-0.5f, 0.5f,  0, 0, 1,   0.5f,-0.5f, 0.5f,  0, 0, 1,   0.5f, 0.5f, 0.5f,  0, 0, 1,
    -0.5f,-0.5f, 0.5f,  0, 0, 1,   0.5f, 0.5f, 0.5f,  0, 0, 1,  -0.5f, 0.5f, 0.5f,  0, 0, 1,
    // -Z (back)
     0.5f,-0.5f,-0.5f,  0, 0,-1,  -0.5f,-0.5f,-0.5f,  0, 0,-1,  -0.5f, 0.5f,-0.5f,  0, 0,-1,
     0.5f,-0.5f,-0.5f,  0, 0,-1,  -0.5f, 0.5f,-0.5f,  0, 0,-1,   0.5f, 0.5f,-0.5f,  0, 0,-1,
};

static const char* kVS = R"(#version 300 es
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_nrm;
uniform mat4 u_mvp;
uniform mat4 u_model;
out vec3 v_nrm;
void main() {
  v_nrm = mat3(u_model) * a_nrm;
  gl_Position = u_mvp * vec4(a_pos, 1.0);
}
)";

static const char* kFS = R"(#version 300 es
precision mediump float;
in vec3 v_nrm;
uniform vec3 u_color;
uniform vec3 u_light_dir;
out vec4 frag;
void main() {
  vec3 n = normalize(v_nrm);
  float ndl = max(dot(n, normalize(u_light_dir)), 0.0);
  vec3 col = u_color * (0.25 + 0.75 * ndl);
  frag = vec4(col, 1.0);
}
)";

static GLuint Compile(GLenum type, const char* src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetShaderInfoLog(s, sizeof(log), nullptr, log);
    fprintf(stderr, "Shader compile error: %s\n", log);
    glDeleteShader(s);
    return 0;
  }
  return s;
}

Renderer::Renderer() { mat::Identity(view_proj_); }
Renderer::~Renderer() { Shutdown(); }

bool Renderer::Init() {
  GLuint vs = Compile(GL_VERTEX_SHADER, kVS);
  GLuint fs = Compile(GL_FRAGMENT_SHADER, kFS);
  if (!vs || !fs) return false;
  program_ = glCreateProgram();
  glAttachShader(program_, vs);
  glAttachShader(program_, fs);
  glLinkProgram(program_);
  GLint ok = 0;
  glGetProgramiv(program_, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
    fprintf(stderr, "Link error: %s\n", log);
    return false;
  }
  glDeleteShader(vs);
  glDeleteShader(fs);
  u_mvp_ = glGetUniformLocation(program_, "u_mvp");
  u_model_ = glGetUniformLocation(program_, "u_model");
  u_color_ = glGetUniformLocation(program_, "u_color");
  u_light_dir_ = glGetUniformLocation(program_, "u_light_dir");

  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);
  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glBindVertexArray(0);
  return true;
}

void Renderer::Shutdown() {
  if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
  if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
  if (program_) { glDeleteProgram(program_); program_ = 0; }
}

void Renderer::BeginFrame(int w, int h) {
  glViewport(0, 0, w, h);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glClearColor(0.15f, 0.18f, 0.24f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  float proj[16], view[16];
  float aspect = (float)w / (float)(h > 0 ? h : 1);
  mat::Perspective(60.f * 3.14159265f / 180.f, aspect, 0.1f, 200.f, proj);
  mat::LookAt(0.f, 12.f, 12.f,  0.f, 0.f, 0.f, view);
  mat::Multiply(proj, view, view_proj_);

  glUseProgram(program_);
  glBindVertexArray(vao_);
  float ld[3] = {0.4f, 1.0f, 0.6f};
  glUniform3fv(u_light_dir_, 1, ld);
}

void Renderer::DrawCube(const float* model, float r, float g, float b) {
  float mvp[16];
  mat::Multiply(view_proj_, model, mvp);
  glUniformMatrix4fv(u_mvp_, 1, GL_FALSE, mvp);
  glUniformMatrix4fv(u_model_, 1, GL_FALSE, model);
  glUniform3f(u_color_, r, g, b);
  glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::GetViewProj(float* out) const {
  memcpy(out, view_proj_, sizeof(view_proj_));
}

void Renderer::EndFrame() {
  glBindVertexArray(0);
  glUseProgram(0);
}
