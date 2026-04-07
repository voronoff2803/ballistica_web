#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Runs a small ODE sanity test: drops a body under gravity for 60 steps
// and logs its Y position. Returns 0 on success.
int RunPhysicsTest(void);

#ifdef __cplusplus
}
#endif
