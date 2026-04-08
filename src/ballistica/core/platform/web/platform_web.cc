// Released under the MIT License. See LICENSE for details.

#if BA_PLATFORM_WEB
#include "ballistica/core/platform/web/platform_web.h"

#include <list>
#include <string>

#include "ballistica/shared/ballistica.h"

namespace ballistica::core {

PlatformWeb::PlatformWeb() {}

auto PlatformWeb::DoGetDeviceName() -> std::string {
  return "Web Browser";
}

auto PlatformWeb::DoGetDeviceDescription() -> std::string {
  return "Web/WASM";
}

auto PlatformWeb::GetOSVersionString() -> std::string {
  return "1.0";
}

auto PlatformWeb::GetDeviceUUIDInputs() -> std::list<std::string> {
  std::list<std::string> out;
  // In the browser we don't have a stable device ID.
  // Use a placeholder; real implementation can use localStorage.
  out.push_back("web_browser_session");
  return out;
}

auto PlatformWeb::DoHasTouchScreen() -> bool {
  // Assume no touch screen by default; can be detected via JS later.
  return false;
}

auto PlatformWeb::GetLegacyPlatformName() -> std::string { return "web"; }

auto PlatformWeb::GetLegacySubplatformName() -> std::string { return ""; }

auto PlatformWeb::IsRunningOnDesktop() -> bool {
  // Browser could be either; default to true.
  return true;
}

}  // namespace ballistica::core

#endif  // BA_PLATFORM_WEB
