#pragma once

class Renderer;

class DemoScene {
 public:
  DemoScene();
  ~DemoScene();

  void Init();
  void Step(float dt);     // advance physics
  void Render(Renderer* r, int view_w, int view_h);
  void SpawnCubeAt(float x, float y, float z);
  // Normalized touch coords: (0,0) = top-left, (1,1) = bottom-right.
  void SpawnFromTouch(float norm_x, float norm_y);

  struct Impl;  // public so the C-style ODE callback can access it

 private:
  Impl* impl_ = nullptr;
};
