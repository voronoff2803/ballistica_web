// Stub _baplus Python C extension module for iOS.
// Provides minimal implementations of all functions that the baplus
// Python package calls, allowing the Plus subsystem to initialize
// and the main menu to load without the closed-source libballisticaplus.a.

#include <Python.h>

// --- Helper: return Py_None ---
#define STUB_NONE(name)                                              \
  static PyObject* name(PyObject*, PyObject*) { Py_RETURN_NONE; }

// --- Helper: return empty string ---
#define STUB_STR(name, val)                                          \
  static PyObject* name(PyObject*, PyObject*) {                      \
    return PyUnicode_FromString(val);                                 \
  }

// --- Helper: return False ---
#define STUB_FALSE(name)                                             \
  static PyObject* name(PyObject*, PyObject*) { Py_RETURN_FALSE; }

// --- Helper: return 0 ---
#define STUB_INT(name, val)                                          \
  static PyObject* name(PyObject*, PyObject*) {                      \
    return PyLong_FromLong(val);                                      \
  }

// Account state / display functions.
STUB_STR(get_v1_account_display_string, "Local Player")
STUB_STR(get_v1_account_name, "Local Player")
STUB_STR(get_v1_account_state, "signed_out")
STUB_INT(get_v1_account_state_num, 1)
STUB_STR(get_v1_account_type, "Local")
STUB_STR(get_v2_fleet, "")

static PyObject* get_v1_account_public_login_id(PyObject*, PyObject*) {
  Py_RETURN_NONE;
}

// Account misc read vals — return the default_value argument.
static PyObject* get_v1_account_misc_read_val(PyObject*, PyObject* args) {
  PyObject* name_obj = nullptr;
  PyObject* default_val = nullptr;
  if (!PyArg_ParseTuple(args, "OO", &name_obj, &default_val)) return nullptr;
  Py_INCREF(default_val);
  return default_val;
}

static PyObject* get_v1_account_misc_read_val_2(PyObject* s, PyObject* args) {
  return get_v1_account_misc_read_val(s, args);
}

static PyObject* get_v1_account_misc_val(PyObject* s, PyObject* args) {
  return get_v1_account_misc_read_val(s, args);
}

// Server addresses.
STUB_STR(get_master_server_address, "https://ballistica.net")
STUB_STR(get_legacy_master_server_address, "https://ballistica.net")

static PyObject* get_bootstrap_server_addresses(PyObject*, PyObject*) {
  PyObject* list = PyList_New(0);
  return list;
}

STUB_STR(get_bootstrap_server_address, "")

// News / prices.
STUB_STR(get_classic_news_show, "")

static PyObject* get_price(PyObject*, PyObject*) {
  Py_RETURN_NONE;
}

// Ads.
STUB_FALSE(can_show_ad)
STUB_FALSE(has_video_ads)
STUB_FALSE(have_incentivized_ad)
STUB_NONE(show_ad)
STUB_NONE(show_ad_2)

// Account transactions.
STUB_NONE(add_v1_account_transaction)
STUB_FALSE(have_outstanding_v1_account_transactions)
STUB_NONE(run_v1_account_transactions)

// Purchases.
STUB_FALSE(supports_purchases)
STUB_NONE(purchase)
STUB_NONE(in_game_purchase)
STUB_NONE(restore_purchases)

// Game services.
STUB_FALSE(game_service_has_leaderboard)
STUB_NONE(show_game_service_ui)
STUB_NONE(report_achievement)
STUB_NONE(reset_achievements)
STUB_NONE(submit_score)
STUB_NONE(tournament_query)
STUB_NONE(power_ranking_query)

// Auth.
STUB_NONE(sign_in_v1)
STUB_NONE(sign_out_v1)

// on_app_loading: create stub cloud/accounts objects on the plus subsystem
// and schedule on_initial_sign_in_complete so the app reaches 'running' state.
static PyObject* on_app_loading(PyObject*, PyObject*) {
  const char* setup_code = R"PY(
import _babase

# Create stub cloud and accounts on the plus subsystem.
app = _babase.app
plus = app.plus
if plus is not None and not hasattr(plus, 'cloud'):
    from efro.call import CallbackSet

    class _StubCloudVals:
        gc_debug_types = set()
        gc_debug_type_limit = 100

    class _StubCloud:
        connected = False
        on_connectivity_changed_callbacks = CallbackSet()
        vals = _StubCloudVals()
        def is_connected(self): return False
        def send_message_cb(self, *args, **kwargs): pass
        def send_message(self, *args, **kwargs): return None

    class _StubAccounts:
        primary = None
        def on_app_loading(self): pass
        def have_primary_credentials(self): return False
        def set_primary_credentials(self, c): pass

    plus.cloud = _StubCloud()
    plus.accounts = _StubAccounts()

# Enable TV border on iOS for safe area (notch/dynamic island).
app.config['TV Border'] = True

_babase.pushcall(_babase.app.on_initial_sign_in_complete)
)PY";

  PyRun_SimpleString(setup_code);
  PyErr_Clear();
  Py_RETURN_NONE;
}

STUB_FALSE(is_blessed)
STUB_NONE(mark_config_dirty)

// --- Method table ---
static PyMethodDef baplus_methods[] = {
    {"on_app_loading", on_app_loading, METH_VARARGS, nullptr},
    {"get_v1_account_display_string", get_v1_account_display_string, METH_VARARGS, nullptr},
    {"get_v1_account_name", get_v1_account_name, METH_VARARGS, nullptr},
    {"get_v1_account_state", get_v1_account_state, METH_VARARGS, nullptr},
    {"get_v1_account_state_num", get_v1_account_state_num, METH_VARARGS, nullptr},
    {"get_v1_account_type", get_v1_account_type, METH_VARARGS, nullptr},
    {"get_v1_account_public_login_id", get_v1_account_public_login_id, METH_VARARGS, nullptr},
    {"get_v1_account_misc_read_val", get_v1_account_misc_read_val, METH_VARARGS, nullptr},
    {"get_v1_account_misc_read_val_2", get_v1_account_misc_read_val_2, METH_VARARGS, nullptr},
    {"get_v1_account_misc_val", get_v1_account_misc_val, METH_VARARGS, nullptr},
    {"get_v2_fleet", get_v2_fleet, METH_VARARGS, nullptr},
    {"get_master_server_address", get_master_server_address, METH_VARARGS, nullptr},
    {"get_legacy_master_server_address", get_legacy_master_server_address, METH_VARARGS, nullptr},
    {"get_bootstrap_server_addresses", get_bootstrap_server_addresses, METH_VARARGS, nullptr},
    {"get_bootstrap_server_address", get_bootstrap_server_address, METH_VARARGS, nullptr},
    {"get_classic_news_show", get_classic_news_show, METH_VARARGS, nullptr},
    {"get_price", get_price, METH_VARARGS, nullptr},
    {"can_show_ad", can_show_ad, METH_VARARGS, nullptr},
    {"has_video_ads", has_video_ads, METH_VARARGS, nullptr},
    {"have_incentivized_ad", have_incentivized_ad, METH_VARARGS, nullptr},
    {"show_ad", show_ad, METH_VARARGS, nullptr},
    {"show_ad_2", show_ad_2, METH_VARARGS, nullptr},
    {"add_v1_account_transaction", add_v1_account_transaction, METH_VARARGS, nullptr},
    {"have_outstanding_v1_account_transactions", have_outstanding_v1_account_transactions, METH_VARARGS, nullptr},
    {"run_v1_account_transactions", run_v1_account_transactions, METH_VARARGS, nullptr},
    {"supports_purchases", supports_purchases, METH_VARARGS, nullptr},
    {"purchase", purchase, METH_VARARGS, nullptr},
    {"in_game_purchase", in_game_purchase, METH_VARARGS, nullptr},
    {"restore_purchases", restore_purchases, METH_VARARGS, nullptr},
    {"game_service_has_leaderboard", game_service_has_leaderboard, METH_VARARGS, nullptr},
    {"show_game_service_ui", show_game_service_ui, METH_VARARGS, nullptr},
    {"report_achievement", report_achievement, METH_VARARGS, nullptr},
    {"reset_achievements", reset_achievements, METH_VARARGS, nullptr},
    {"submit_score", submit_score, METH_VARARGS, nullptr},
    {"tournament_query", tournament_query, METH_VARARGS, nullptr},
    {"power_ranking_query", power_ranking_query, METH_VARARGS, nullptr},
    {"sign_in_v1", sign_in_v1, METH_VARARGS, nullptr},
    {"sign_out_v1", sign_out_v1, METH_VARARGS, nullptr},
    {"is_blessed", is_blessed, METH_VARARGS, nullptr},
    {"mark_config_dirty", mark_config_dirty, METH_VARARGS, nullptr},
    {nullptr, nullptr, 0, nullptr},
};

// --- Module definition ---
static struct PyModuleDef baplus_module = {
    PyModuleDef_HEAD_INIT,
    "_baplus",
    "Stub _baplus module for iOS",
    -1,
    baplus_methods,
};

// This is called by the engine's monolithic init via PyImport_AppendInittab.
// The engine registers it as "DoPyInit__baplus" but the actual symbol
// exported must match what python_modules_monolithic.h declares.
// PlusSoftInterface stub — defined in ios_stubs.cc
#include "ballistica/base/support/plus_soft.h"
#include "ballistica/base/base.h"

// Full class definition needed here so compiler sees the inheritance.
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
extern PlusSoftStub g_plus_soft_stub;
}  // namespace ballistica::base

extern "C" auto DoPyInit__baplus() -> PyObject* {
  // Register C++ PlusSoftInterface with the engine so that
  // g_base->Plus() / g_base->HavePlus() work.
  using namespace ballistica::base;
  if (g_base && !g_base->HavePlus()) {
    g_base->SetPlus(&g_plus_soft_stub);
  }
  return PyModule_Create(&baplus_module);
}
