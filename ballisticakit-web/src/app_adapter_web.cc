// App adapter for Emscripten/WASM web builds.
#if BA_PLATFORM_WEB

#include "app_adapter_web.h"

#include <vector>

#include "ballistica/base/app_platform/support/min_sdl_key_names.h"
#include "ballistica/base/graphics/gl/renderer_gl.h"
#include "ballistica/base/graphics/graphics.h"
#include "ballistica/base/graphics/graphics_server.h"
#include "ballistica/base/input/input.h"
#include "ballistica/base/logic/logic.h"
#include "ballistica/base/support/app_config.h"
#include "ballistica/shared/ballistica.h"
#include "ballistica/shared/foundation/event_loop.h"

namespace ballistica::base {

auto AppAdapterWeb::ManagesMainThreadEventLoop() const -> bool {
  return false;
}

void AppAdapterWeb::DoPushMainThreadRunnable(Runnable* runnable) {
  std::scoped_lock lock(main_calls_mutex_);
  main_calls_.push_back(runnable);
}

void AppAdapterWeb::ProcessMainRunnables() {
  std::vector<Runnable*> calls;
  {
    std::scoped_lock lock(main_calls_mutex_);
    if (!main_calls_.empty()) {
      main_calls_.swap(calls);
    }
  }
  for (auto* r : calls) {
    r->RunAndLogErrors();
    delete r;
  }
}

void AppAdapterWeb::OnMainThreadStartApp() {
  AppAdapter::OnMainThreadStartApp();
  g_base->input->PushCreateKeyboardInputDevices();
}

void AppAdapterWeb::ApplyAppConfig() { assert(g_base->InLogicThread()); }

void AppAdapterWeb::ApplyGraphicsSettings(const GraphicsSettings* settings) {
  auto* graphics_server = g_base->graphics_server;

  bool need_full_reload = ((graphics_server->texture_quality_requested()
                            != settings->texture_quality)
                           || (graphics_server->graphics_quality_requested()
                               != settings->graphics_quality));

  if (need_full_reload) {
    ReloadRenderer_(settings);
  }
}

void AppAdapterWeb::ReloadRenderer_(const GraphicsSettings* settings) {
  auto* gs = g_base->graphics_server;

  if (gs->renderer() && gs->renderer_loaded()) {
    gs->UnloadRenderer();
  }
  if (!gs->renderer()) {
    gs->set_renderer(new RendererGL());
  }

  gs->set_graphics_quality_requested(settings->graphics_quality);
  gs->set_texture_quality_requested(settings->texture_quality);
  gs->LoadRenderer();
}

auto AppAdapterWeb::TryRender() -> bool {
  graphics_thread_ = std::this_thread::get_id();
  graphics_allowed_ = true;

  // Run pending graphics runnables.
  std::vector<Runnable*> calls;
  {
    auto lock = std::scoped_lock(graphics_calls_mutex_);
    if (!graphics_calls_.empty()) {
      graphics_calls_.swap(calls);
    }
  }
  for (auto* call : calls) {
    call->RunAndLogErrors();
    delete call;
  }

  auto result = g_base->graphics_server->TryRender();
  graphics_allowed_ = false;
  return result;
}

auto AppAdapterWeb::InGraphicsContext() -> bool {
  return std::this_thread::get_id() == graphics_thread_ && graphics_allowed_;
}

void AppAdapterWeb::DoPushGraphicsContextRunnable(Runnable* runnable) {
  auto lock = std::scoped_lock(graphics_calls_mutex_);
  if (graphics_calls_.size() > 1000) {
    BA_LOG_ONCE(LogName::kBa, LogLevel::kError, "graphics_calls_ got too big.");
  }
  graphics_calls_.push_back(runnable);
}

auto AppAdapterWeb::ShouldUseCursor() -> bool { return true; }

auto AppAdapterWeb::HasHardwareCursor() -> bool { return true; }

void AppAdapterWeb::SetHardwareCursorVisible(bool visible) {}

void AppAdapterWeb::TerminateApp() {}

auto AppAdapterWeb::FullscreenControlAvailable() const -> bool {
  return false;
}
auto AppAdapterWeb::FullscreenControlGet() const -> bool { return false; }
void AppAdapterWeb::FullscreenControlSet(bool fullscreen) {}
auto AppAdapterWeb::FullscreenControlKeyShortcut() const
    -> std::optional<std::string> {
  return std::nullopt;
}

auto AppAdapterWeb::HasDirectKeyboardInput() -> bool { return true; }

auto AppAdapterWeb::GetKeyRepeatDelay() -> float {
  return AppAdapter::GetKeyRepeatDelay();
}
auto AppAdapterWeb::GetKeyRepeatInterval() -> float {
  return AppAdapter::GetKeyRepeatInterval();
}

auto AppAdapterWeb::DoClipboardIsSupported() -> bool { return false; }
auto AppAdapterWeb::DoClipboardHasText() -> bool { return false; }
void AppAdapterWeb::DoClipboardSetText(const std::string& text) {}
auto AppAdapterWeb::DoClipboardGetText() -> std::string { return ""; }

auto AppAdapterWeb::GetKeyName(int keycode) -> std::string {
  return MinSDL_GetKeyName(keycode);
}

auto AppAdapterWeb::NativeReviewRequestSupported() -> bool { return false; }

}  // namespace ballistica::base

#endif  // BA_PLATFORM_WEB
