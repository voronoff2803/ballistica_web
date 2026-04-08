// Web/Emscripten entry point for ballistica engine.

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

#include <cstdio>
#include <string>

#include "ballistica/shared/ballistica.h"
#include "ballistica/core/core.h"
#include "ballistica/core/support/core_config.h"
#include "ballistica/base/base.h"
#include "ballistica/base/graphics/graphics.h"
#include "ballistica/base/graphics/graphics_server.h"
#include "ballistica/base/input/input.h"
#include "ballistica/base/logic/logic.h"
#include "ballistica/base/assets/assets_server.h"
#include "ballistica/base/audio/audio_server.h"
#include "ballistica/base/dynamics/bg/bg_dynamics_server.h"
#include "ballistica/shared/foundation/event_loop.h"

#include "ballistica/core/platform/support/min_sdl.h"

#include "app_adapter_web.h"

// ---- Input callbacks ----
static int g_canvas_w = 1, g_canvas_h = 1;

static EM_BOOL OnMouseDown(int type, const EmscriptenMouseEvent* e, void*) {
  using namespace ballistica::base;
  if (!g_base) return EM_FALSE;
  float x = static_cast<float>(e->targetX) / g_canvas_w;
  float y = 1.0f - static_cast<float>(e->targetY) / g_canvas_h;
  int button = (e->button == 0) ? 1 : (e->button == 2) ? 3 : e->button + 1;
  g_base->input->PushMouseDownEvent(button, {x, y});
  return EM_TRUE;
}

static EM_BOOL OnMouseUp(int type, const EmscriptenMouseEvent* e, void*) {
  using namespace ballistica::base;
  if (!g_base) return EM_FALSE;
  float x = static_cast<float>(e->targetX) / g_canvas_w;
  float y = 1.0f - static_cast<float>(e->targetY) / g_canvas_h;
  int button = (e->button == 0) ? 1 : (e->button == 2) ? 3 : e->button + 1;
  g_base->input->PushMouseUpEvent(button, {x, y});
  return EM_TRUE;
}

static EM_BOOL OnMouseMove(int type, const EmscriptenMouseEvent* e, void*) {
  using namespace ballistica::base;
  if (!g_base) return EM_FALSE;
  float x = static_cast<float>(e->targetX) / g_canvas_w;
  float y = 1.0f - static_cast<float>(e->targetY) / g_canvas_h;
  g_base->input->PushMouseMotionEvent({x, y});
  return EM_TRUE;
}

static EM_BOOL OnWheel(int type, const EmscriptenWheelEvent* e, void*) {
  using namespace ballistica::base;
  if (!g_base) return EM_FALSE;
  g_base->input->PushMouseScrollEvent(
      {static_cast<float>(e->deltaX) * -0.01f,
       static_cast<float>(e->deltaY) * -0.01f});
  return EM_TRUE;
}

// Map browser 'code' field to SDL_Keycode (layout-independent).
static SDL_Keycode BrowserKeyToSDL(const EmscriptenKeyboardEvent* e) {
  const char* code = e->code;

  #define SC(scan) static_cast<SDL_Keycode>(scan | (1 << 30))

  // Letters: "KeyA".."KeyZ" → 'a'..'z'
  if (code[0] == 'K' && code[1] == 'e' && code[2] == 'y'
      && code[3] >= 'A' && code[3] <= 'Z' && code[4] == '\0') {
    return static_cast<SDL_Keycode>(code[3] + 32);  // to lowercase
  }

  // Digits: "Digit0".."Digit9" → '0'..'9'
  if (strncmp(code, "Digit", 5) == 0 && code[5] >= '0' && code[5] <= '9'
      && code[6] == '\0') {
    return static_cast<SDL_Keycode>(code[5]);
  }

  // Numpad digits.
  if (strncmp(code, "Numpad", 6) == 0 && code[6] >= '0' && code[6] <= '9'
      && code[7] == '\0') {
    return SC(98 + (code[6] - '0'));  // SDL_SCANCODE_KP_0=98
  }

  // Function keys: "F1".."F12"
  if (code[0] == 'F' && code[1] >= '1' && code[1] <= '9') {
    int fnum = atoi(code + 1);
    if (fnum >= 1 && fnum <= 12) return SC(57 + fnum);
  }

  // Arrows.
  if (strcmp(code, "ArrowUp") == 0) return SC(82);
  if (strcmp(code, "ArrowDown") == 0) return SC(81);
  if (strcmp(code, "ArrowLeft") == 0) return SC(80);
  if (strcmp(code, "ArrowRight") == 0) return SC(79);

  // Common keys.
  if (strcmp(code, "Space") == 0) return SDLK_SPACE;
  if (strcmp(code, "Enter") == 0) return SDLK_RETURN;
  if (strcmp(code, "NumpadEnter") == 0) return SC(88);
  if (strcmp(code, "Escape") == 0) return SDLK_ESCAPE;
  if (strcmp(code, "Backspace") == 0) return SDLK_BACKSPACE;
  if (strcmp(code, "Tab") == 0) return SDLK_TAB;
  if (strcmp(code, "Delete") == 0) return SC(76);
  if (strcmp(code, "Insert") == 0) return SC(73);
  if (strcmp(code, "Home") == 0) return SC(74);
  if (strcmp(code, "End") == 0) return SC(77);
  if (strcmp(code, "PageUp") == 0) return SC(75);
  if (strcmp(code, "PageDown") == 0) return SC(78);

  // Punctuation/symbols.
  if (strcmp(code, "Minus") == 0) return SDLK_MINUS;
  if (strcmp(code, "Equal") == 0) return static_cast<SDL_Keycode>('=');
  if (strcmp(code, "BracketLeft") == 0) return static_cast<SDL_Keycode>('[');
  if (strcmp(code, "BracketRight") == 0) return static_cast<SDL_Keycode>(']');
  if (strcmp(code, "Backslash") == 0) return static_cast<SDL_Keycode>('\\');
  if (strcmp(code, "Semicolon") == 0) return static_cast<SDL_Keycode>(';');
  if (strcmp(code, "Quote") == 0) return SDLK_QUOTE;
  if (strcmp(code, "Backquote") == 0) return static_cast<SDL_Keycode>('`');
  if (strcmp(code, "Comma") == 0) return SDLK_COMMA;
  if (strcmp(code, "Period") == 0) return SDLK_PERIOD;
  if (strcmp(code, "Slash") == 0) return SDLK_SLASH;

  // Modifiers.
  if (strcmp(code, "ShiftLeft") == 0) return SC(225);
  if (strcmp(code, "ShiftRight") == 0) return SC(229);
  if (strcmp(code, "ControlLeft") == 0) return SC(224);
  if (strcmp(code, "ControlRight") == 0) return SC(228);
  if (strcmp(code, "AltLeft") == 0) return SC(226);
  if (strcmp(code, "AltRight") == 0) return SC(230);
  if (strcmp(code, "MetaLeft") == 0) return SC(227);
  if (strcmp(code, "MetaRight") == 0) return SC(231);
  if (strcmp(code, "CapsLock") == 0) return SC(57);

  #undef SC

  return SDLK_UNKNOWN;
}

static EM_BOOL OnKeyDown(int type, const EmscriptenKeyboardEvent* e, void*) {
  using namespace ballistica::base;
  if (!g_base) return EM_FALSE;
  SDL_Keycode sym = BrowserKeyToSDL(e);
  if (sym == SDLK_UNKNOWN) return EM_FALSE;
  SDL_Keysym keysym{};
  keysym.sym = sym;
  g_base->input->PushKeyPressEvent(keysym);
  return EM_TRUE;
}

static EM_BOOL OnKeyUp(int type, const EmscriptenKeyboardEvent* e, void*) {
  using namespace ballistica::base;
  if (!g_base) return EM_FALSE;
  SDL_Keycode sym = BrowserKeyToSDL(e);
  if (sym == SDLK_UNKNOWN) return EM_FALSE;
  SDL_Keysym keysym{};
  keysym.sym = sym;
  g_base->input->PushKeyReleaseEvent(keysym);
  return EM_TRUE;
}

// ---- Persistent storage via IDBFS ----
static const char* kPersistDir = "/home/web_user/.ballisticakit";
static bool g_idbfs_synced = false;
static int g_sync_counter = 0;

static void SyncFS() {
  // Flush virtual FS to IndexedDB.
  EM_ASM(
    FS.syncfs(false, function(err) {
      if (err) console.warn('[web] syncfs error:', err);
    });
  );
}

// ---- IDBFS ready callback ----
static bool g_idbfs_ready = false;

extern "C" {
EMSCRIPTEN_KEEPALIVE void web_on_idbfs_ready() {
  g_idbfs_ready = true;
  printf("[web] IDBFS ready.\n");
}
}

// ---- Global state ----
static bool g_engine_started = false;
static bool g_init_config_passed = false;
static bool g_has_graphics = false;
static bool g_screen_set = false;
static int g_frame_count = 0;

// ---- Main loop callback ----
static void MainLoopCallback() {
  using namespace ballistica::base;

  // Wait for IDBFS to sync before starting engine.
  if (!g_idbfs_ready) return;


  if (!g_engine_started) {
    ballistica::core::CoreConfig config;
    bool done = ballistica::MonolithicMainIncremental(
        g_init_config_passed ? nullptr : &config);
    g_init_config_passed = true;
    if (done) {
      g_engine_started = true;
      printf("[web] Engine init complete!\n");
    }
    return;
  }

  g_frame_count++;

  // 1. Process pending main-thread runnables.
  auto* adapter = AppAdapterWeb::Get(g_base);
  if (adapter) {
    adapter->ProcessMainRunnables();
  }

  // 2. Set screen resolution once + update canvas size for input.
  if (g_has_graphics && !g_screen_set) {
    double css_w, css_h;
    emscripten_get_element_css_size("#canvas", &css_w, &css_h);
    if (css_w > 1 && css_h > 1) {
      g_screen_set = true;
      g_canvas_w = static_cast<int>(css_w);
      g_canvas_h = static_cast<int>(css_h);
      auto pw = static_cast<float>(css_w);
      auto ph = static_cast<float>(css_h);
      printf("[web] Screen: %.0f x %.0f\n", pw, ph);
      g_base->graphics->SetScreenResolution(pw, ph);
      g_base->Reset();
    }
  }

  // 3. Tick logic event loop.
  if (auto* logic_loop = g_base->logic->event_loop()) {
    logic_loop->RunSingleCycle();
  }

  // 4. Tick assets event loop.
  if (auto* assets_loop = g_base->assets_server->event_loop()) {
    assets_loop->RunSingleCycle();
  }

  // 5. Tick audio event loop.
  if (auto* audio_loop = g_base->audio_server->event_loop()) {
    audio_loop->RunSingleCycle();
  }

  // 6. Tick bg-dynamics event loop (particles, chunks, tendrils).
  if (auto* bgd_loop = g_base->bg_dynamics_server->event_loop()) {
    bgd_loop->RunSingleCycle();
  }

  // 7. Periodically sync save data to IndexedDB (~every 5 seconds).
  g_sync_counter++;
  if (g_sync_counter >= 300) {
    g_sync_counter = 0;
    SyncFS();
  }

  // Draw + render.
  if (g_has_graphics && g_screen_set && adapter) {
    try {
      g_base->logic->Draw();
      adapter->TryRender();
    } catch (const std::exception& e) {
      if (g_frame_count <= 5) {
        printf("[web] Render error: %s\n", e.what());
      }
    } catch (...) {}
  }

}

// ---- Entry point ----
int main(int argc, char* argv[]) {
  printf("=== Ballistica Web Build ===\n");

  // Mount IDBFS for persistent save data and wait for sync before starting.
  EM_ASM(
    FS.mkdir('/home/web_user/.ballisticakit');
    FS.mount(IDBFS, {}, '/home/web_user/.ballisticakit');
    FS.syncfs(true, function(err) {
      if (err) {
        console.warn('[web] IDBFS initial sync error:', err);
      } else {
        console.log('[web] Save data loaded from IndexedDB.');
      }
      // Signal C++ that sync is done.
      Module._web_on_idbfs_ready();
    });
  );
  printf("[web] IDBFS mounted, waiting for sync...\n");

  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  attrs.majorVersion = 2;
  attrs.minorVersion = 0;
  attrs.alpha = 0;
  attrs.depth = 1;
  attrs.stencil = 0;
  attrs.antialias = 1;
  attrs.premultipliedAlpha = 0;
  attrs.preserveDrawingBuffer = 1;

  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx =
      emscripten_webgl_create_context("#canvas", &attrs);
  if (ctx <= 0) {
    printf("WARNING: Failed to create WebGL2 context: %d (no canvas?)\n", ctx);
    g_has_graphics = false;
  } else {
    emscripten_webgl_make_context_current(ctx);
    g_has_graphics = true;
    printf("[web] WebGL2 context created.\n");
  }

  // Register input callbacks.
  emscripten_set_mousedown_callback("#canvas", nullptr, EM_TRUE, OnMouseDown);
  emscripten_set_mouseup_callback("#canvas", nullptr, EM_TRUE, OnMouseUp);
  emscripten_set_mousemove_callback("#canvas", nullptr, EM_TRUE, OnMouseMove);
  emscripten_set_wheel_callback("#canvas", nullptr, EM_TRUE, OnWheel);
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, OnKeyDown);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, OnKeyUp);

  int fps = g_has_graphics ? 0 : 60;
  emscripten_set_main_loop(MainLoopCallback, fps, 1);
  return 0;
}
