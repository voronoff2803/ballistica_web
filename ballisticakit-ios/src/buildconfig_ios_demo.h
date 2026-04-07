// Build configuration for iOS demo app.
// This is force-included before all engine sources via -include flag.

#ifndef BUILDCONFIG_IOS_DEMO_H_
#define BUILDCONFIG_IOS_DEMO_H_

// Platform identification.
#define BA_PLATFORM "ios"
#define BA_PLATFORM_IOS 1
#define BA_PLATFORM_IOS_TVOS 1

// Architecture.
#if __aarch64__
#define BA_ARCH "arm64"
#elif __x86_64__
#define BA_ARCH "x86_64"
#else
#error "Unknown architecture"
#endif

// Variant.
#define BA_VARIANT "generic"
#define BA_VARIANT_GENERIC 1

// Build type.
#define BA_XCODE_BUILD 1
#define BA_MONOLITHIC_BUILD 1

// Graphics.
#define BA_ENABLE_OPENGL 1
#define BA_OPENGL_IS_ES 1

// Disable everything we don't need.
#define BA_ENABLE_DISCORD 0
#define BA_HEADLESS_BUILD 0
#define BA_SDL_BUILD 0
#define BA_MINSDL_BUILD 0
#define BA_DEBUG_BUILD 0
#define BA_VR_BUILD 0
#define BA_GEARVR_BUILD 0
#define BA_RIFT_BUILD 0
#define BA_ENABLE_SDL_JOYSTICKS 0
#define BA_USE_STORE_KIT 0
#define BA_USE_GAME_CENTER 0
#define BA_USE_GOOGLE_PLAY_GAME_SERVICES 0
#define BA_ENABLE_STDIO_CONSOLE 0
#define BA_ENABLE_OS_FONT_RENDERING 0
#define BA_CONTAINS_PYTHON_DIST 0
#define BA_ENABLE_AUDIO 0
#define BA_DEFINE_MAIN 0
#define BA_ENABLE_EXECINFO_BACKTRACES 0

// ODE.
#define dNODEBUG 1
#define dTRIMESH_ENABLED 1

// Release mode.
#ifndef NDEBUG
#define NDEBUG
#endif

// Silence OpenGL ES deprecation warnings.
#define GLES_SILENCE_DEPRECATION 1
#define GL_SILENCE_DEPRECATION 1

// This must always be last.
#include "ballistica/shared/buildconfig/buildconfig_common.h"

#endif  // BUILDCONFIG_IOS_DEMO_H_
