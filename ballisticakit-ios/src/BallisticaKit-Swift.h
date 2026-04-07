// Stub for BallisticaKit-Swift.h — provides C++ declarations that the
// real Swift layer would generate. This allows platform_apple.cc and
// app_adapter_apple.cc to compile without actual Swift code.

#pragma once
#include <string>

namespace BallisticaKit {

struct TextTextureData {
  int width{};
  int height{};
  uint8_t* rgba_data{};
  static auto init(int w, int h, const std::vector<std::string>&,
                   const std::vector<float>&, const std::vector<float>&,
                   float) -> TextTextureData {
    TextTextureData d;
    d.width = w > 0 ? w : 1;
    d.height = h > 0 ? h : 1;
    // Allocate transparent RGBA buffer.
    size_t sz = static_cast<size_t>(d.width) * d.height * 4;
    d.rgba_data = new uint8_t[sz]();  // zero-initialized (transparent)
    return d;
  }
  auto getTextTextureData() -> uint8_t* { return rgba_data; }
  static auto GetTextBoundsAndWidth(const std::string&, float*, float*)
      -> void {}
  struct BoundsResult {
    float operator[](int) const { return 0.0f; }
    int getCount() const { return 5; }
  };
  static auto getTextBoundsAndWidth(const std::string&) -> BoundsResult {
    return {};
  }
};

namespace FromCpp {
// This must dispatch to the main thread and call runnable->RunAndLogErrors(),
// then delete it. Implemented in ios_main.mm.
void pushRawRunnableToMain(void* runnable);
auto getCacheDirectoryPath() -> std::string;  // Implemented in ios_main.mm
inline auto getAppPythonDirectory() -> std::string { return ""; }
// Returns the iOS app bundle's resource path at runtime.
auto getResourcesPath() -> std::string;  // Implemented in ios_main.mm
inline auto getOSVersion() -> std::string { return "18.0"; }
inline auto GetBaLocale() -> std::string { return "en_US"; }
inline auto getBaLocale() -> std::string { return "en_US"; }
inline auto GetLocaleTag() -> std::string { return "en"; }
inline auto getLocaleTag() -> std::string { return "en"; }
}  // namespace FromCpp

namespace UIKitFromCpp {
inline auto getDeviceName() -> std::string { return "iPhone"; }
inline auto getDeviceModelName() -> std::string { return "iPhone"; }
inline auto getLegacyDeviceUUID() -> std::string { return "stub-uuid"; }
inline auto getLocaleTag() -> std::string { return "en"; }
inline auto isTablet() -> bool { return false; }
inline auto isNativeReviewRequestSupported() -> bool { return false; }
inline void nativeReviewRequest() {}
inline auto getClipboardText() -> std::string { return ""; }
inline auto clipboardHasText() -> bool { return false; }
inline auto clipboardIsSupported() -> bool { return false; }
inline void setClipboardText(const std::string&) {}
inline void setCursorVisible(bool) {}
inline void openURL(const std::string&) {}
inline void openDirExternally(const std::string&) {}
inline void openFileExternally(const std::string&) {}
inline auto supportsOpenDirExternally() -> bool { return false; }
inline void loginAdapterGetSignInToken(const std::string&, int) {}
inline void loginAdapterBackEndActiveChange(const std::string&, bool) {}
inline auto isRunningOnDesktop() -> bool { return false; }
}  // namespace UIKitFromCpp

namespace StoreKitContext {
inline void onAppStart() {}
inline void purchase(const std::string&) {}
inline void restorePurchases() {}
inline void purchaseAck(const std::string&, const std::string&) {}
}  // namespace StoreKitContext

namespace GameCenterContext {
inline void onAppStart() {}
inline void submitScore(const std::string&, const std::string&, long long) {}
inline void reportAchievement(const std::string&) {}
inline void resetAchievements() {}
inline void showGameServiceUI(const std::string&, const std::string&, const std::string&) {}
}  // namespace GameCenterContext

}  // namespace BallisticaKit
