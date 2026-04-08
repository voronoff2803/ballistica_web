// Build configuration for Emscripten/WASM web engine build.
// Force-included before all engine sources via -include flag.

#ifndef BUILDCONFIG_WEB_H_
#define BUILDCONFIG_WEB_H_

// Platform identification.
#define BA_PLATFORM "web"
#define BA_PLATFORM_WEB 1

// Architecture.
#define BA_ARCH "wasm32"

// Variant.
#define BA_VARIANT "generic"
#define BA_VARIANT_GENERIC 1

// Build type.
#define BA_MONOLITHIC_BUILD 1
#define BA_CONTAINS_PYTHON_DIST 1

// Graphics: WebGL2 = OpenGL ES 3.0.
#define BA_ENABLE_OPENGL 1
#define BA_OPENGL_IS_ES 1

// Audio via OpenAL → Web Audio API (Emscripten).
#define BA_ENABLE_AUDIO 1

// Disable things not available in WASM.
#define BA_ENABLE_DISCORD 0
#define BA_HEADLESS_BUILD 0
#define BA_SDL_BUILD 0
#define BA_MINSDL_BUILD 1
#define BA_DEBUG_BUILD 0
#define BA_VR_BUILD 0
#define BA_GEARVR_BUILD 0
#define BA_RIFT_BUILD 0
#define BA_XCODE_BUILD 0
#define BA_ENABLE_SDL_JOYSTICKS 0
#define BA_ENABLE_STDIO_CONSOLE 0
#define BA_DEFINE_MAIN 0
#define BA_ENABLE_EXECINFO_BACKTRACES 0
#define BA_ENABLE_OS_FONT_RENDERING 0

// ODE physics.
#define dNODEBUG 1
#define dTRIMESH_ENABLED 1

// Release mode.
#ifndef NDEBUG
#define NDEBUG
#endif

// Suppress GL deprecation warnings.
#define GL_SILENCE_DEPRECATION 1

// This must always be last.
#include "ballistica/shared/buildconfig/buildconfig_common.h"

#endif  // BUILDCONFIG_WEB_H_
