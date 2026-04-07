// Minimal stubs for symbols not available in the iOS build.

// =============================================================================
// PlusSoftInterface stub — provides the C++ "plus" feature-set interface
// so that engine subsystems (accounts, scoring, etc.) don't crash.
// =============================================================================
#include "ballistica/base/support/plus_soft.h"
#include "ballistica/base/base.h"

namespace ballistica::base {

class PlusSoftStub : public PlusSoftInterface {
 public:
  void OnAppStart() override {}
  void OnAppSuspend() override {}
  void OnAppUnsuspend() override {}
  void OnAppShutdown() override {}
  void OnAppShutdownComplete() override {}
  void ApplyAppConfig() override {}
  void OnScreenSizeChange() override {}
  void StepDisplayTime() override {}

  auto IsUnmodifiedBlessedBuild() -> bool override { return false; }
  auto HasBlessingHash() -> bool override { return false; }
  auto PutLog(bool) -> bool override { return false; }
  void AAT() override {}
  void AATE() override {}
  auto GAHU() -> std::optional<std::string> override { return std::nullopt; }
  void V1LoginDidChange() override {}
  void SetAdCompletionCall(PyObject*, bool) override {}
  void PushAdViewComplete(const std::string&, bool) override {}
  void PushPublicPartyState() override {}
  void PushSetFriendListCall(const std::vector<std::string>&) override {}
  void DispatchRemoteAchievementList(const std::set<std::string>&) override {}
  void SetProductPrice(const std::string&, const std::string&) override {}
  void PushAnalyticsCall(const std::string&, int) override {}
  void PushPurchaseTransactionCall(const std::string&, const std::string&,
                                   const std::string&, const std::string&,
                                   bool) override {}
  auto GetAccountID() -> std::string override { return ""; }
  void DirectSendV1CloudLogs(const std::string&, const std::string&,
                             bool, int* result) override {
    if (result) *result = 0;
  }
  void ClientInfoQuery(const std::string&, const std::string&,
                       const std::string&, int) override {}
  auto CalcV1PeerHash(const std::string&) -> std::string override { return ""; }
  void V1SetClientInfo(JsonDict*) override {}
  void DoPushSubmitAnalyticsCountsCall(const std::string&) override {}
  void SetHaveIncentivizedAd(bool) override {}
  auto HaveIncentivizedAd() -> bool override { return false; }
};

PlusSoftStub g_plus_soft_stub;

}  // namespace ballistica::base

// Python module init stub for _batemplatefs.
extern "C" {
void* PyInit__batemplatefs() { return nullptr; }
}

// Override Logging::Log_ to avoid GIL crash from background threads.
// The engine's Log_ calls CorePython::LoggingCall which acquires GIL.
// On iOS with BeeWare Python, GIL acquisition from bg threads crashes.
#include "ballistica/core/logging/logging.h"
#include "ballistica/core/core.h"

namespace ballistica::core {

int g_early_v1_cloud_log_writes{10};

void Logging::Log_(LogName, LogLevel level, const char* msg) {
  // Direct to stderr — avoids GIL crash from background threads.
  const char* prefix = "";
  switch (level) {
    case LogLevel::kWarning: prefix = "WARNING: "; break;
    case LogLevel::kError: prefix = "ERROR: "; break;
    case LogLevel::kCritical: prefix = "CRITICAL: "; break;
    default: break;
  }
  fprintf(stderr, "[ba] %s%s\n", prefix, msg);
}

void Logging::EmitLog(std::string_view, LogLevel, double,
                      std::string_view msg) {
  fprintf(stderr, "[ba-emit] %.*s\n", (int)msg.size(), msg.data());
}

void Logging::V1CloudLog(const std::string&) {}
void Logging::ApplyBaEnvConfig() {}
void Logging::UpdateInternalLoggerLevels() {}

}  // namespace ballistica::core

