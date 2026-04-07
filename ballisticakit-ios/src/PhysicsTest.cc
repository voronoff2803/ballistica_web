#include "PhysicsTest.h"

#include <ode/ode.h>
#include <stdio.h>

int RunPhysicsTest(void) {
  // This ODE fork doesn't need dInitODE/dCloseODE.
  dWorldID world = dWorldCreate();
  dWorldSetGravity(world, 0, -9.81, 0);

  dBodyID body = dBodyCreate(world);
  dMass m;
  dMassSetBox(&m, 1.0, 1.0, 1.0, 1.0);
  dBodySetMass(body, &m);
  dBodySetPosition(body, 0, 10.0, 0);

  const dReal dt = 1.0 / 60.0;
  for (int i = 0; i < 60; ++i) {
    dWorldStep(world, dt);
  }
  const dReal* pos = dBodyGetPosition(body);
  printf("ODE test: after 60 steps, y = %.4f (expected ~5.1)\n",
         (double)pos[1]);

  dWorldDestroy(world);
  return 0;
}
