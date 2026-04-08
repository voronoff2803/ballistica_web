// App adapter for Emscripten/WASM web builds.

#ifndef BALLISTICA_WEB_APP_ADAPTER_WEB_H_
#define BALLISTICA_WEB_APP_ADAPTER_WEB_H_

#if BA_PLATFORM_WEB

#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ballistica/base/app_adapter/app_adapter.h"
#include "ballistica/shared/generic/runnable.h"

namespace ballistica::base {

class AppAdapterWeb : public AppAdapter {
 public:
  static auto Get(BaseFeatureSet* base) -> AppAdapterWeb* {
    auto* val = static_cast<AppAdapterWeb*>(base->app_adapter);
    assert(val);
    return val;
  }

  void OnMainThreadStartApp() override;
  auto ManagesMainThreadEventLoop() const -> bool override;
  void ApplyAppConfig() override;

  /// Called from the emscripten main loop to render a frame.
  auto TryRender() -> bool;

  /// Process pending main-thread runnables. Called from main loop.
  void ProcessMainRunnables();

  auto FullscreenControlAvailable() const -> bool override;
  auto FullscreenControlGet() const -> bool override;
  void FullscreenControlSet(bool fullscreen) override;
  auto FullscreenControlKeyShortcut() const
      -> std::optional<std::string> override;

  auto HasDirectKeyboardInput() -> bool override;
  auto GetKeyRepeatDelay() -> float override;
  auto GetKeyRepeatInterval() -> float override;
  auto GetKeyName(int keycode) -> std::string override;
  auto NativeReviewRequestSupported() -> bool override;

 protected:
  void DoPushMainThreadRunnable(Runnable* runnable) override;
  void DoPushGraphicsContextRunnable(Runnable* runnable) override;
  auto InGraphicsContext() -> bool override;
  auto ShouldUseCursor() -> bool override;
  auto HasHardwareCursor() -> bool override;
  void SetHardwareCursorVisible(bool visible) override;
  void TerminateApp() override;
  void ApplyGraphicsSettings(const GraphicsSettings* settings) override;
  auto DoClipboardIsSupported() -> bool override;
  auto DoClipboardHasText() -> bool override;
  void DoClipboardSetText(const std::string& text) override;
  auto DoClipboardGetText() -> std::string override;

 private:
  void ReloadRenderer_(const GraphicsSettings* settings);

  std::thread::id graphics_thread_{};
  bool graphics_allowed_{};
  std::mutex graphics_calls_mutex_;
  std::vector<Runnable*> graphics_calls_;
  std::mutex main_calls_mutex_;
  std::vector<Runnable*> main_calls_;
};

}  // namespace ballistica::base

#endif  // BA_PLATFORM_WEB

#endif  // BALLISTICA_WEB_APP_ADAPTER_WEB_H_
