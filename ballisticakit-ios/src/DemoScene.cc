#include "DemoScene.h"

#include <ode/ode.h>
#include <stdio.h>

#include <vector>

#include "Renderer.h"

namespace {

struct Cube {
  dBodyID body;
  dGeomID geom;
  float color[3];
};

}  // namespace

struct DemoScene::Impl {
  dWorldID world = nullptr;
  dSpaceID space = nullptr;
  dJointGroupID contact_group = nullptr;
  dGeomID ground_geom = nullptr;
  std::vector<Cube> cubes;
};

static void NearCallback(void* data, dGeomID o1, dGeomID o2) {
  auto* impl = static_cast<DemoScene::Impl*>(data);
  dBodyID b1 = dGeomGetBody(o1);
  dBodyID b2 = dGeomGetBody(o2);
  if (b1 && b2 && dAreConnectedExcluding(b1, b2, dJointTypeContact)) return;

  const int kMaxContacts = 4;
  dContact contact[kMaxContacts];
  int n = dCollide(o1, o2, kMaxContacts, &contact[0].geom, sizeof(dContact));
  for (int i = 0; i < n; ++i) {
    contact[i].surface.mode = dContactBounce | dContactSoftCFM;
    contact[i].surface.mu = 2.0;     // high friction to stop sliding
    contact[i].surface.bounce = 0.3;
    contact[i].surface.bounce_vel = 0.1;
    contact[i].surface.soft_cfm = 0.001;
    dJointID c = dJointCreateContact(impl->world, impl->contact_group,
                                     &contact[i]);
    dJointAttach(c, b1, b2);
  }
}

DemoScene::DemoScene() {}
DemoScene::~DemoScene() {
  if (!impl_) return;
  if (impl_->contact_group) dJointGroupDestroy(impl_->contact_group);
  if (impl_->space) dSpaceDestroy(impl_->space);
  if (impl_->world) dWorldDestroy(impl_->world);
  delete impl_;
}

void DemoScene::Init() {
  impl_ = new Impl();
  impl_->world = dWorldCreate();
  dWorldSetGravity(impl_->world, 0, -9.81, 0);
  dWorldSetCFM(impl_->world, 1e-5);
  dWorldSetERP(impl_->world, 0.8);

  impl_->space = dHashSpaceCreate(nullptr);
  impl_->contact_group = dJointGroupCreate(0);

  // Ground plane at y=0.
  impl_->ground_geom = dCreatePlane(impl_->space, 0, 1, 0, 0);

  // 10 falling cubes.
  const float colors[][3] = {
      {1.00f, 0.30f, 0.30f}, {0.30f, 0.80f, 0.40f}, {0.30f, 0.50f, 1.00f},
      {1.00f, 0.80f, 0.20f}, {0.80f, 0.30f, 0.80f}, {0.20f, 0.80f, 0.80f},
      {1.00f, 0.50f, 0.20f}, {0.60f, 0.80f, 0.20f}, {0.40f, 0.40f, 0.90f},
      {0.90f, 0.40f, 0.60f},
  };
  for (int i = 0; i < 10; ++i) {
    // Tight cluster so they stack visibly in front of the camera.
    float x = ((i % 3) - 1) * 0.6f;
    float y = 2.f + i * 1.3f;
    float z = (((i / 3) % 3) - 1) * 0.6f;
    SpawnCubeAt(x, y, z);
    auto& c = impl_->cubes.back();
    c.color[0] = colors[i][0];
    c.color[1] = colors[i][1];
    c.color[2] = colors[i][2];
  }
}

void DemoScene::SpawnCubeAt(float x, float y, float z) {
  if (!impl_) return;
  Cube c;
  c.body = dBodyCreate(impl_->world);
  dBodySetPosition(c.body, x, y, z);
  // Random-ish starting rotation.
  dMatrix3 R;
  dRFromAxisAndAngle(R, 0.3, 0.7, 0.2, y * 0.2);
  dBodySetRotation(c.body, R);
  dMass m;
  dMassSetBox(&m, 1.0, 1.0, 1.0, 1.0);
  dBodySetMass(c.body, &m);
  c.geom = dCreateBox(impl_->space, 1.0, 1.0, 1.0);
  dGeomSetBody(c.geom, c.body);
  c.color[0] = 0.8f;
  c.color[1] = 0.8f;
  c.color[2] = 0.8f;
  impl_->cubes.push_back(c);
}

void DemoScene::SpawnFromTouch(float nx, float ny) {
  // Map screen to world XZ: x in [-5, 5], z in [6, -6] (front=bottom of screen).
  float wx = (nx - 0.5f) * 10.f;
  float wz = (ny - 0.5f) * -12.f;
  SpawnCubeAt(wx, 10.f, wz);
  // Random-ish color.
  auto& c = impl_->cubes.back();
  float h = (float)impl_->cubes.size() * 0.618f;
  h = h - (int)h;
  c.color[0] = 0.5f + 0.5f * sinf(h * 6.28f);
  c.color[1] = 0.5f + 0.5f * sinf((h + 0.33f) * 6.28f);
  c.color[2] = 0.5f + 0.5f * sinf((h + 0.66f) * 6.28f);
}

void DemoScene::Step(float dt) {
  if (!impl_) return;
  dSpaceCollide(impl_->space, impl_, &NearCallback);
  dWorldStep(impl_->world, dt);
  dJointGroupEmpty(impl_->contact_group);
}

void DemoScene::Render(Renderer* r, int w, int h) {
  if (!impl_) return;

  r->BeginFrame(w, h);

  // Ground: huge flat box at y=-0.25. Column-major 4x4.
  float ground[16] = {
    60.f,   0.f,   0.f, 0.f,
     0.f,   0.5f,  0.f, 0.f,
     0.f,   0.f,  60.f, 0.f,
     0.f,  -0.25f, 0.f, 1.f,
  };
  r->DrawCube(ground, 0.30f, 0.32f, 0.36f);

  for (const auto& c : impl_->cubes) {
    const dReal* p = dBodyGetPosition(c.body);
    const dReal* R = dBodyGetRotation(c.body);
    float rot[16], trans[16], model[16];
    mat::FromOdeRotation(R, rot);
    mat::Translation((float)p[0], (float)p[1], (float)p[2], trans);
    mat::Multiply(trans, rot, model);
    r->DrawCube(model, c.color[0], c.color[1], c.color[2]);
  }

  r->EndFrame();
}
