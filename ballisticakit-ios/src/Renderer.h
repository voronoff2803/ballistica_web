#pragma once

// Minimal GL ES 3 cube renderer. All state lives inside the object.
// Usage:
//   Renderer r;
//   r.Init();                                 // after GL context is current
//   r.BeginFrame(viewW, viewH);
//   r.DrawCube(modelMatrix, r, g, b);         // once per cube
//   r.EndFrame();

class Renderer {
 public:
  Renderer();
  ~Renderer();

  bool Init();
  void Shutdown();

  void BeginFrame(int view_w, int view_h);
  // Get a copy of the current view-projection matrix (after BeginFrame).
  void GetViewProj(float* out16) const;
  // Model is a column-major 4x4 matrix (16 floats).
  void DrawCube(const float* model, float r, float g, float b);
  void EndFrame();

 private:
  unsigned int program_ = 0;
  unsigned int vao_ = 0;
  unsigned int vbo_ = 0;
  int u_mvp_ = -1;
  int u_model_ = -1;
  int u_color_ = -1;
  int u_light_dir_ = -1;
  float view_proj_[16];
};

// Public matrix helpers (column-major, OpenGL convention).
namespace mat {
void Identity(float* out);
void Multiply(const float* a, const float* b, float* out);
void Translation(float x, float y, float z, float* out);
void Scale(float s, float* out);
void RotationX(float rad, float* out);
void RotationY(float rad, float* out);
void Perspective(float fov_y_rad, float aspect, float zn, float zf, float* out);
void LookAt(float ex, float ey, float ez, float tx, float ty, float tz,
            float* out);
// ODE rotation is dReal[12] (3x4 row-major). dReal is float in dSINGLE mode.
void FromOdeRotation(const float* ode_rot, float* out_mat4);
bool Invert4x4(const float* m, float* out);
}  // namespace mat
