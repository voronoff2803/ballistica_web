// Released under the MIT License. See LICENSE for details.

#ifndef BALLISTICA_CORE_PLATFORM_WEB_PLATFORM_WEB_H_
#define BALLISTICA_CORE_PLATFORM_WEB_PLATFORM_WEB_H_
#if BA_PLATFORM_WEB

#include <list>
#include <string>

#include "ballistica/core/platform/platform.h"

namespace ballistica::core {

class PlatformWeb : public Platform {
 public:
  PlatformWeb();
  auto GetDeviceV1AccountUUIDPrefix() -> std::string override { return "w"; }
  auto DoHasTouchScreen() -> bool override;
  auto GetLegacyPlatformName() -> std::string override;
  auto GetLegacySubplatformName() -> std::string override;
  auto GetDeviceUUIDInputs() -> std::list<std::string> override;
  auto DoGetDeviceName() -> std::string override;
  auto DoGetDeviceDescription() -> std::string override;
  auto GetOSVersionString() -> std::string override;
  auto IsRunningOnDesktop() -> bool override;
};

}  // namespace ballistica::core

#endif  // BA_PLATFORM_WEB
#endif  // BALLISTICA_CORE_PLATFORM_WEB_PLATFORM_WEB_H_
