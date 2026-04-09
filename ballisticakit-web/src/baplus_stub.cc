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
STUB_STR(get_v1_account_display_string, "Web Player")
STUB_STR(get_v1_account_name, "Web Player")
STUB_STR(get_v1_account_state, "signed_in")
STUB_INT(get_v1_account_state_num, 2)
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

// Account transactions — process locally on web.
static PyObject* add_v1_account_transaction(PyObject*, PyObject* args) {
  // Accept (transaction) or (transaction, callback).
  PyObject* transaction = nullptr;
  PyObject* callback = nullptr;
  if (!PyArg_ParseTuple(args, "O|O", &transaction, &callback)) return nullptr;

  // Handle COMPLETE_LEVEL immediately by setting level complete in config.
  if (PyDict_Check(transaction)) {
    PyObject* type_obj = PyDict_GetItemString(transaction, "type");
    if (type_obj && PyUnicode_Check(type_obj)) {
      const char* type_str = PyUnicode_AsUTF8(type_obj);
      if (type_str && strcmp(type_str, "COMPLETE_LEVEL") == 0) {
        PyObject* campaign_obj = PyDict_GetItemString(transaction, "campaign");
        PyObject* level_obj = PyDict_GetItemString(transaction, "level");
        if (campaign_obj && level_obj) {
          const char* campaign = PyUnicode_AsUTF8(campaign_obj);
          const char* level = PyUnicode_AsUTF8(level_obj);
          char code[512];
          snprintf(code, sizeof(code), R"PY(
import _babase
cfg = _babase.app.config
camp = cfg.setdefault('Campaigns', {}).setdefault('%s', {})
lvl = camp.setdefault('%s', {'Rating': 0.0, 'Complete': False})
lvl['Complete'] = True
)PY", campaign, level);
          PyRun_SimpleString(code);
          PyErr_Clear();
        }
      }
      // Save high scores locally.
      if (type_str && strcmp(type_str, "SET_LEVEL_LOCAL_HIGH_SCORES") == 0) {
        // Pass the full transaction to Python for processing.
        PyObject* main_mod = PyImport_AddModule("__main__");
        if (main_mod) {
          PyObject* ns = PyModule_GetDict(main_mod);
          PyDict_SetItemString(ns, "_web_hs_transaction", transaction);
          PyRun_SimpleString(R"PY(
import _babase
t = _web_hs_transaction
cfg = _babase.app.config
camp = cfg.setdefault('Campaigns', {}).setdefault(t['campaign'], {})
lvl = camp.setdefault(t['level'], {'Rating': 0.0, 'Complete': False})
key = 'High Scores' + t.get('scoreVersion', '')
lvl[key] = t['scores']
)PY");
          PyDict_DelItemString(ns, "_web_hs_transaction");
          PyErr_Clear();
        }
      }
    }
  }
  Py_RETURN_NONE;
}

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
// submit_score: call the callback with None (scores unavailable).
static PyObject* submit_score(PyObject*, PyObject* args, PyObject* kwargs) {
  // Extract the callback (5th positional arg).
  Py_ssize_t nargs = PyTuple_Size(args);
  if (nargs >= 5) {
    PyObject* callback = PyTuple_GetItem(args, 4);
    if (callback && PyCallable_Check(callback)) {
      // Schedule callback(None) via pushcall.
      PyObject* babase = PyImport_ImportModule("_babase");
      if (babase) {
        PyObject* pushcall = PyObject_GetAttrString(babase, "pushcall");
        if (pushcall) {
          // Create a lambda that calls callback(None).
          PyObject* functools = PyImport_ImportModule("functools");
          if (functools) {
            PyObject* partial = PyObject_GetAttrString(functools, "partial");
            if (partial) {
              PyObject* partial_args = Py_BuildValue("(OO)", callback, Py_None);
              PyObject* bound = PyObject_CallObject(partial, partial_args);
              if (bound) {
                PyObject* pc_args = Py_BuildValue("(O)", bound);
                PyObject_CallObject(pushcall, pc_args);
                Py_XDECREF(pc_args);
                Py_DECREF(bound);
              }
              Py_XDECREF(partial_args);
              Py_XDECREF(partial);
            }
            Py_DECREF(functools);
          }
          Py_DECREF(pushcall);
        }
        Py_DECREF(babase);
      }
    }
  }
  PyErr_Clear();
  Py_RETURN_NONE;
}
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
        on_primary_account_changed_callbacks = CallbackSet()
        login_adapters = {}
        def on_app_loading(self): pass
        def have_primary_credentials(self): return False
        def set_primary_credentials(self, c): pass

    plus.cloud = _StubCloud()
    plus.accounts = _StubAccounts()

# Enable TV border for safe area on web.
app.config['TV Border'] = True
app.config.apply_and_commit()

_babase.pushcall(_babase.app.on_initial_sign_in_complete)
)PY";

  PyRun_SimpleString(setup_code);
  PyErr_Clear();
  Py_RETURN_NONE;
}

STUB_FALSE(is_blessed)

// Write config to disk when marked dirty.
static PyObject* mark_config_dirty(PyObject*, PyObject*) {
  PyRun_SimpleString(R"PY(
import json, os, _babase
cfg = _babase.app.config
path = _babase.app.env.config_file_path
try:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(dict(cfg), f, indent=2, ensure_ascii=False)
except Exception as e:
    print(f'[web] Error saving config: {e}')
)PY");
  PyErr_Clear();
  Py_RETURN_NONE;
}

// --- JavaScript bridge ---
extern "C" {
  extern void web_js_run(const char* code);
  extern const char* web_js_eval(const char* code);
}

static PyObject* py_web_js_run(PyObject*, PyObject* args) {
  const char* code;
  if (!PyArg_ParseTuple(args, "s", &code)) return nullptr;
  web_js_run(code);
  Py_RETURN_NONE;
}

static PyObject* py_web_js_eval(PyObject*, PyObject* args) {
  const char* code;
  if (!PyArg_ParseTuple(args, "s", &code)) return nullptr;
  const char* result = web_js_eval(code);
  return PyUnicode_FromString(result ? result : "");
}

// --- Method table ---
static PyMethodDef baplus_methods[] = {
    {"web_js_run", py_web_js_run, METH_VARARGS, nullptr},
    {"web_js_eval", py_web_js_eval, METH_VARARGS, nullptr},
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
    {"submit_score", (PyCFunction)submit_score, METH_VARARGS | METH_KEYWORDS, nullptr},
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
    "Stub _baplus module for web",
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
