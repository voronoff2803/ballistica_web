// Shim implementations for engine globals and classes.
// Provides minimal stubs so RendererGL can compile and run standalone.

#include "engine_shims.h"

#include <chrono>
#include <cstdio>
#include <list>
#include <string>
#include <vector>

// Hack: expose private members of RenderPass and Graphics for setup.
// This is only needed because we're driving the render pipeline externally.
#define private public
#define protected public
#include "ballistica/base/graphics/renderer/render_pass.h"
#include "ballistica/base/graphics/graphics.h"
#undef protected
#undef private

// Engine headers (use real headers, provide our own implementations).
#include "ballistica/base/app_adapter/app_adapter.h"
#include "ballistica/base/base.h"
#include "ballistica/base/graphics/graphics.h"
#include "ballistica/base/graphics/graphics_server.h"
#include "ballistica/base/graphics/renderer/render_pass.h"
#include "ballistica/base/graphics/renderer/render_target.h"
#include "ballistica/base/graphics/renderer/renderer.h"
#include "ballistica/base/graphics/support/frame_def.h"
#include "ballistica/base/graphics/support/graphics_client_context.h"
#include "ballistica/base/graphics/support/graphics_settings.h"
#include "ballistica/base/support/app_config.h"
#include "ballistica/base/graphics/support/camera.h"
#include "ballistica/base/graphics/support/area_of_interest.h"
#include "ballistica/base/graphics/support/net_graph.h"
#include "ballistica/base/graphics/support/screen_messages.h"
#include "ballistica/base/graphics/mesh/image_mesh.h"
#include "ballistica/base/graphics/mesh/nine_patch_mesh.h"
#include "ballistica/base/graphics/mesh/text_mesh.h"
#include "ballistica/base/graphics/text/text_group.h"
#include "ballistica/base/graphics/mesh/sprite_mesh.h"
#include "ballistica/base/python/support/python_context_call.h"
#include "ballistica/shared/generic/snapshot.h"
#include "ballistica/core/core.h"
#include "ballistica/core/logging/logging.h"
#include "ballistica/core/platform/platform.h"
#include "ballistica/shared/foundation/object.h"
#include "ballistica/shared/generic/runnable.h"

// Shim subclass for AppAdapter (abstract base — has pure virtual).
namespace {
class ShimAppAdapter : public ballistica::base::AppAdapter {
 public:
  ShimAppAdapter() = default;
  void DoPushMainThreadRunnable(ballistica::Runnable*) override {}
};
}  // namespace

// ============================================================================
// ballistica namespace — shared symbols
// ============================================================================
namespace ballistica {

const int kEngineBuildNumber = 99999;
const char* kEngineVersion = "0.0.0-ios-demo";
const int kEngineApiVersion = 0;
const char* kFeatureSetDataAttrName = "_ba_feature_set_data";

bool InlineDebugExplicitBool(bool val) { return val; }
void FatalError(const std::string& message) {
  fprintf(stderr, "FATAL: %s\n", message.c_str());
  abort();
}

// ============================================================================
// Object class — minimal release-mode implementation
// ============================================================================
Object::Object() = default;

Object::~Object() {
  // Invalidate weak refs.
  while (object_weak_refs_) {
    auto tmp{object_weak_refs_};
    object_weak_refs_ = tmp->next_;
    tmp->prev_ = nullptr;
    tmp->next_ = nullptr;
    tmp->obj_ = nullptr;
  }
}

void Object::ObjectPostInit() {}

auto Object::GetObjectTypeName() const -> std::string {
  return typeid(*this).name();
}

auto Object::GetObjectDescription() const -> std::string {
  return "<" + GetObjectTypeName() + ">";
}

auto Object::GetDefaultOwnerThread() const -> EventLoopID {
  return EventLoopID::kLogic;
}

auto Object::GetThreadOwnership() const -> Object::ThreadOwnership {
  return ThreadOwnership::kClassDefault;
}

void Object::LsObjects() {}

// Exception class.
auto GetShortExceptionDescription(const std::exception& exc) -> const char* {
  return exc.what();
}

Exception::Exception(std::string message, PyExcType python_type)
    : message_(std::move(message)), python_type_(python_type) {}

auto Exception::what() const noexcept -> const char* {
  return message_.c_str();
}

// ============================================================================
// FeatureSetNativeComponent
// ============================================================================
FeatureSetNativeComponent::~FeatureSetNativeComponent() = default;

void FeatureSetNativeComponent::StoreOnPythonModule(PyObject*) {}

auto FeatureSetNativeComponent::BaseImportThroughPythonModule(const char*)
    -> FeatureSetNativeComponent* {
  return nullptr;
}

// Timer utilities referenced from macros.h.
auto MacroFunctionTimerStartTime() -> millisecs_t { return 0; }

// Runnable stubs.
void Runnable::RunAndLogErrors() { Run(); }
auto Runnable::GetThreadOwnership() const -> ThreadOwnership {
  return ThreadOwnership::kNextReferencing;
}

// Exception.
Exception::~Exception() = default;


}  // namespace ballistica

// ============================================================================
// core namespace
// ============================================================================
namespace ballistica::core {

CoreFeatureSet* g_core{};
BaseSoftInterface* g_base_soft{};

// Logging.
int g_early_v1_cloud_log_writes{};

void Logging::Log_(LogName, LogLevel level, const char* msg) {
  const char* prefix = "";
  switch (level) {
    case LogLevel::kWarning:
      prefix = "WARNING: ";
      break;
    case LogLevel::kError:
      prefix = "ERROR: ";
      break;
    case LogLevel::kCritical:
      prefix = "CRITICAL: ";
      break;
    default:
      break;
  }
  fprintf(stderr, "[ba] %s%s\n", prefix, msg);
}

void Logging::EmitLog(std::string_view, LogLevel, double, std::string_view) {}
void Logging::ApplyBaEnvConfig() {}
void Logging::UpdateInternalLoggerLevels() {}
void Logging::V1CloudLog(const std::string&) {}

// Platform — only provide static methods. No instance created (avoids vtable).
void Platform::SleepMillisecs(millisecs_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// CoreFeatureSet — minimal implementation.
static Logging s_logging;
static auto s_start_time = std::chrono::steady_clock::now();

CoreFeatureSet::CoreFeatureSet(CoreConfig config)
    : python(nullptr),
      platform(nullptr),  // No Platform instance needed for graphics-only demo.
      logging(&s_logging),
      core_config_(std::move(config)) {
  main_thread_id_ = std::this_thread::get_id();
}

auto CoreFeatureSet::Import(const CoreConfig* config) -> CoreFeatureSet* {
  if (!g_core) {
    CoreConfig cfg;
    if (config) cfg = *config;
    g_core = new CoreFeatureSet(cfg);
  }
  return g_core;
}

auto CoreFeatureSet::core_config() const -> const CoreConfig& {
  return core_config_;
}

auto CoreFeatureSet::AppTimeMillisecs() -> millisecs_t {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now - s_start_time)
      .count();
}

auto CoreFeatureSet::AppTimeMicrosecs() -> microsecs_t {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(now - s_start_time)
      .count();
}

auto CoreFeatureSet::AppTimeSeconds() -> seconds_t {
  return static_cast<seconds_t>(AppTimeMicrosecs()) / 1000000.0;
}

auto CoreFeatureSet::SoftImportBase() -> BaseSoftInterface* {
  return g_base_soft;
}
auto CoreFeatureSet::HeadlessMode() -> bool { return false; }
void CoreFeatureSet::StartSuicideTimer(const std::string&, millisecs_t) {}
void CoreFeatureSet::ApplyBaEnvConfig() {}
auto CoreFeatureSet::GetAppPythonDirectory() -> std::optional<std::string> {
  return std::nullopt;
}
auto CoreFeatureSet::GetUserPythonDirectory() -> std::optional<std::string> {
  return std::nullopt;
}
auto CoreFeatureSet::GetConfigDirectory() -> std::string { return "/tmp"; }
auto CoreFeatureSet::GetConfigFilePath() -> std::string { return "/tmp/cfg"; }
auto CoreFeatureSet::GetBackupConfigFilePath() -> std::string {
  return "/tmp/cfg.bak";
}
auto CoreFeatureSet::GetDataDirectory() -> std::string { return "/tmp"; }
auto CoreFeatureSet::GetCacheDirectory() -> std::string { return "/tmp"; }
auto CoreFeatureSet::GetSitePythonDirectory() -> std::optional<std::string> {
  return std::nullopt;
}
void CoreFeatureSet::RegisterThread(const std::string&) {}
void CoreFeatureSet::UnregisterThread() {}
auto CoreFeatureSet::CurrentThreadName() -> std::string { return "main"; }
auto CoreFeatureSet::HandOverInitialAppConfig() -> PyObject* { return nullptr; }

}  // namespace ballistica::core

// ============================================================================
// base namespace
// ============================================================================
namespace ballistica::base {

// Globals.
BaseFeatureSet* g_base{};
// base.h also declares g_core in base namespace (points to same core::g_core).
core::CoreFeatureSet* g_core{};

// AppAdapter — provide implementations for virtual methods declared in header.
// The actual instance is ShimAppAdapter (defined above) which adds
// DoPushMainThreadRunnable.
AppAdapter::AppAdapter() = default;
void AppAdapter::OnMainThreadStartApp() {}
void AppAdapter::OnAppStart() {}
void AppAdapter::OnAppSuspend() {}
void AppAdapter::OnAppUnsuspend() {}
void AppAdapter::OnAppShutdown() {}
void AppAdapter::OnAppShutdownComplete() {}
void AppAdapter::OnScreenSizeChange() {}
void AppAdapter::ApplyAppConfig() {}
auto AppAdapter::GetGraphicsSettings() -> GraphicsSettings* {
  return new GraphicsSettings();
}
auto AppAdapter::GetGraphicsClientContext() -> GraphicsClientContext* {
  return new GraphicsClientContext();
}
auto AppAdapter::ManagesMainThreadEventLoop() const -> bool { return false; }
void AppAdapter::RunMainThreadEventLoopToCompletion() {}
void AppAdapter::DoExitMainThreadEventLoop() {}
auto AppAdapter::InGraphicsContext() -> bool { return true; }
auto AppAdapter::ShouldUseCursor() -> bool { return false; }
auto AppAdapter::HasHardwareCursor() -> bool { return false; }
void AppAdapter::SetHardwareCursorVisible(bool) {}
void AppAdapter::ApplyGraphicsSettings(const GraphicsSettings*) {}
void AppAdapter::CursorPositionForDraw(float* x, float* y) {
  if (x) *x = 0;
  if (y) *y = 0;
}
auto AppAdapter::FullscreenControlAvailable() const -> bool { return false; }
auto AppAdapter::FullscreenControlGet() const -> bool { return false; }
void AppAdapter::FullscreenControlSet(bool) {}
auto AppAdapter::FullscreenControlKeyShortcut() const
    -> std::optional<std::string> {
  return std::nullopt;
}
auto AppAdapter::SupportsVSync() -> bool const { return false; }
auto AppAdapter::SupportsMaxFPS() -> bool const { return false; }
auto AppAdapter::ShouldSilenceAudioForInactive() -> bool const { return true; }
auto AppAdapter::CanSoftQuit() -> bool { return false; }
void AppAdapter::DoSoftQuit() {}
auto AppAdapter::CanBackQuit() -> bool { return false; }
void AppAdapter::DoBackQuit() {}
void AppAdapter::TerminateApp() {}
auto AppAdapter::HasDirectKeyboardInput() -> bool { return false; }
auto AppAdapter::GetKeyRepeatDelay() -> float { return 0.3f; }
auto AppAdapter::GetKeyRepeatInterval() -> float { return 0.05f; }
void AppAdapter::DoPushGraphicsContextRunnable(Runnable* runnable) {
  // Execute immediately in single-threaded demo.
  if (runnable) {
    runnable->RunAndLogErrors();
    delete runnable;
  }
}
auto AppAdapter::GetKeyName(int) -> std::string { return ""; }
auto AppAdapter::NativeReviewRequestSupported() -> bool { return false; }
void AppAdapter::DoNativeReviewRequest() {}
auto AppAdapter::DoClipboardIsSupported() -> bool { return false; }
auto AppAdapter::DoClipboardHasText() -> bool { return false; }
auto AppAdapter::DoClipboardGetText() -> std::string { return ""; }
void AppAdapter::DoClipboardSetText(const std::string&) {}
auto AppAdapter::SupportsPurchases() -> bool { return false; }
AppAdapter::~AppAdapter() = default;

// AppConfig stub.
auto AppConfig::Resolve(AppConfig::FloatID) -> float { return 1.0f; }
auto AppConfig::Resolve(AppConfig::BoolID) -> bool { return false; }

// Graphics stubs.
auto Graphics::GraphicsQualityFromAppConfig() -> GraphicsQualityRequest {
  return GraphicsQualityRequest::kAuto;
}
auto Graphics::TextureQualityFromAppConfig() -> TextureQualityRequest {
  return TextureQualityRequest::kAuto;
}

// GraphicsServer — minimal implementation (matrix stack is mostly inline).
GraphicsServer::GraphicsServer() = default;
GraphicsServer::~GraphicsServer() = default;

void GraphicsServer::OnMainThreadStartApp() {}

void GraphicsServer::set_renderer(Renderer* renderer) {
  renderer_ = renderer;
}

void GraphicsServer::LoadRenderer() {
  if (!renderer_ || renderer_loaded_) return;

  graphics_quality_ = GraphicsQuality::kMedium;
  texture_quality_ = TextureQuality::kHigh;
  renderer_->Load();
  renderer_->OnScreenSizeChange();
  renderer_->PostLoad();
  renderer_loaded_ = true;
}

void GraphicsServer::UnloadRenderer() {
  if (!renderer_ || !renderer_loaded_) return;
  renderer_->Unload();
  renderer_loaded_ = false;
}

void GraphicsServer::SetRenderHold() { render_hold_++; }
void GraphicsServer::EnqueueFrameDef(FrameDef*) {}

void GraphicsServer::ApplySettings(const GraphicsSettings* settings) {
  if (!settings || settings->index == settings_index_) return;
  settings_index_ = settings->index;
  tv_border_ = settings->tv_border;
  if (settings->resolution.x > 0) {
    res_x_ = settings->resolution.x;
    res_y_ = settings->resolution.y;
    res_x_virtual_ = settings->resolution_virtual.x;
    res_y_virtual_ = settings->resolution_virtual.y;
    if (renderer_) renderer_->OnScreenSizeChange();
  }
}

void GraphicsServer::set_client_context(GraphicsClientContext*) {}
auto GraphicsServer::TryRender() -> bool { return false; }
auto GraphicsServer::WaitForRenderFrameDef_() -> FrameDef* { return nullptr; }

void GraphicsServer::RunFrameDefMeshUpdates(FrameDef* frame_def) {
  for (auto&& i : frame_def->mesh_data_creates()) {
    if (i) {
      i->iterator_ = mesh_datas_.insert(mesh_datas_.end(), i);
      i->Load(renderer_);
    }
  }
  for (auto&& i : frame_def->mesh_data_destroys()) {
    if (i) {
      i->Unload(renderer_);
      mesh_datas_.erase(i->iterator_);
    }
  }
}

void GraphicsServer::PreprocessRenderFrameDef(FrameDef* frame_def) {
  if (renderer_ && renderer_loaded_) {
    renderer_->PreprocessFrameDef(frame_def);
  }
}

void GraphicsServer::DrawRenderFrameDef(FrameDef* frame_def, int) {
  if (renderer_ && renderer_loaded_) {
    renderer_->RenderFrameDef(frame_def);
  }
}

void GraphicsServer::FinishRenderFrameDef(FrameDef* frame_def) {
  if (renderer_ && renderer_loaded_) {
    renderer_->FinishFrameDef(frame_def);
  }
}

void GraphicsServer::SetTextureCompressionTypes(
    const std::list<TextureCompressionType>& types) {
  texture_compression_types_ = 0;
  for (auto&& i : types) {
    texture_compression_types_ |= (0x01u << (static_cast<uint32_t>(i)));
  }
  texture_compression_types_set_ = true;
}

void GraphicsServer::SetOrthoProjection(float left, float right, float bottom,
                                        float top, float nearval,
                                        float farval) {
  float tx = -((right + left) / (right - left));
  float ty = -((top + bottom) / (top - bottom));
  float tz = -((farval + nearval) / (farval - nearval));
  projection_matrix_.m[0] = 2.0f / (right - left);
  projection_matrix_.m[4] = 0.0f;
  projection_matrix_.m[8] = 0.0f;
  projection_matrix_.m[12] = tx;
  projection_matrix_.m[1] = 0.0f;
  projection_matrix_.m[5] = 2.0f / (top - bottom);
  projection_matrix_.m[9] = 0.0f;
  projection_matrix_.m[13] = ty;
  projection_matrix_.m[2] = 0.0f;
  projection_matrix_.m[6] = 0.0f;
  projection_matrix_.m[10] = -2.0f / (farval - nearval);
  projection_matrix_.m[14] = tz;
  projection_matrix_.m[3] = 0.0f;
  projection_matrix_.m[7] = 0.0f;
  projection_matrix_.m[11] = 0.0f;
  projection_matrix_.m[15] = 1.0f;
  model_view_projection_matrix_dirty_ = true;
  projection_matrix_state_++;
}

void GraphicsServer::SetCamera(const Vector3f& eye, const Vector3f& target,
                               const Vector3f& up_vector) {
  model_view_stack_.clear();
  auto forward = (target - eye).Normalized();
  auto side = Vector3f::Cross(forward, up_vector).Normalized();
  Vector3f up = Vector3f::Cross(side, forward);
  model_view_matrix_.m[0] = side.x;
  model_view_matrix_.m[4] = side.y;
  model_view_matrix_.m[8] = side.z;
  model_view_matrix_.m[12] = 0.0f;
  model_view_matrix_.m[1] = up.x;
  model_view_matrix_.m[5] = up.y;
  model_view_matrix_.m[9] = up.z;
  model_view_matrix_.m[13] = 0.0f;
  model_view_matrix_.m[2] = -forward.x;
  model_view_matrix_.m[6] = -forward.y;
  model_view_matrix_.m[10] = -forward.z;
  model_view_matrix_.m[14] = 0.0f;
  model_view_matrix_.m[3] = model_view_matrix_.m[7] =
      model_view_matrix_.m[11] = 0.0f;
  model_view_matrix_.m[15] = 1.0f;
  model_view_matrix_ =
      Matrix44fTranslate(-eye.x, -eye.y, -eye.z) * model_view_matrix_;
  view_world_matrix_ = model_view_matrix_.Inverse();
  model_view_projection_matrix_dirty_ = true;
  model_world_matrix_dirty_ = true;
  cam_pos_ = eye;
  cam_target_ = target;
  cam_pos_state_++;
  cam_orient_matrix_dirty_ = true;
}

void GraphicsServer::UpdateCamOrientMatrix_() {
  if (cam_orient_matrix_dirty_) {
    cam_orient_matrix_ = kMatrix44fIdentity;
    Vector3f to_cam = cam_pos_ - cam_target_;
    to_cam.Normalize();
    Vector3f world_up(0, 1, 0);
    Vector3f side = Vector3f::Cross(world_up, to_cam);
    side.Normalize();
    Vector3f up = Vector3f::Cross(side, to_cam);
    cam_orient_matrix_.m[0] = side.x;
    cam_orient_matrix_.m[1] = side.y;
    cam_orient_matrix_.m[2] = side.z;
    cam_orient_matrix_.m[4] = to_cam.x;
    cam_orient_matrix_.m[5] = to_cam.y;
    cam_orient_matrix_.m[6] = to_cam.z;
    cam_orient_matrix_.m[8] = up.x;
    cam_orient_matrix_.m[9] = up.y;
    cam_orient_matrix_.m[10] = up.z;
    cam_orient_matrix_.m[3] = cam_orient_matrix_.m[7] =
        cam_orient_matrix_.m[11] = cam_orient_matrix_.m[12] =
            cam_orient_matrix_.m[13] = cam_orient_matrix_.m[14] = 0.0f;
    cam_orient_matrix_.m[15] = 1.0f;
    cam_orient_matrix_state_++;
    cam_orient_matrix_dirty_ = false;
  }
}

void GraphicsServer::PushReloadMediaCall() {}
void GraphicsServer::PushComponentUnloadCall(
    const std::vector<Object::Ref<Asset>*>&) {}
void GraphicsServer::PushRemoveRenderHoldCall() {
  render_hold_--;
  if (render_hold_ < 0) render_hold_ = 0;
}
auto GraphicsServer::InGraphicsContext_() const -> bool { return true; }
void GraphicsServer::Shutdown() {}
void GraphicsServer::ReloadLostRenderer() {}

// Graphics — minimal stub for settings access.
auto Graphics::GraphicsQualityFromRequest(GraphicsQualityRequest,
                                          GraphicsQuality auto_quality)
    -> GraphicsQuality {
  return auto_quality;
}
auto Graphics::TextureQualityFromRequest(TextureQualityRequest,
                                         TextureQuality auto_quality)
    -> TextureQuality {
  return auto_quality;
}

// Asset stub.
void Asset::Load(bool) {}

// Camera stub.
Camera::Camera() = default;
Camera::~Camera() = default;

// AreaOfInterest stub (needed for Graphics destructor).
AreaOfInterest::~AreaOfInterest() = default;

// Graphics virtual methods (for vtable).
void Graphics::DoDrawFade(FrameDef*, float) {}
void Graphics::ApplyCamera(FrameDef*) {}
void Graphics::CalcVirtualRes_(float* x, float* y) {
  if (x) *x = 1024;
  if (y) *y = 768;
}
void Graphics::DrawUI(FrameDef*) {}
void Graphics::DrawDevUI(FrameDef*) {}
void Graphics::DrawWorld(FrameDef*) {}
auto Graphics::ValueTest(const std::string&, double*, double*, double*)
    -> bool { return false; }

// Graphics — provide enough for FrameDef::Reset() to work.
// These are heap-allocated to avoid static init order issues (their default
// constructors access g_base which doesn't exist yet during static init).
static GraphicsSettings* s_demo_settings_ptr{};
static GraphicsClientContext* s_demo_client_ctx_ptr{};
static Snapshot<GraphicsSettings>* s_settings_snapshot{};
static Snapshot<GraphicsClientContext>* s_client_context_snapshot{};

Graphics::Graphics()
    : screenmessages(nullptr) {
  // Don't create camera here — FrameDef::Reset() will handle the null check.
}

Graphics::~Graphics() = default;

auto Graphics::GetGraphicsSettingsSnapshot() -> Snapshot<GraphicsSettings>* {
  if (s_settings_snapshot) {
    s_settings_snapshot->ObjectIncrementStrongRefCount();
  }
  return s_settings_snapshot;
}

void Graphics::set_client_context(Snapshot<GraphicsClientContext>* context) {
  // In the full engine, this manages ref-counting between threads.
  // For our shim, just update the snapshot directly.
  if (context) {
    context->ObjectIncrementStrongRefCount();
    client_context_snapshot_ = Object::Ref<Snapshot<GraphicsClientContext>>(context);
    texture_quality_placeholder_ = context->get()->auto_texture_quality;
  }
}

void Graphics::EnableProgressBar(bool) {}
void Graphics::ReturnCompletedFrameDef(FrameDef* fd) { delete fd; }
void Graphics::OnAppStart() {}
void Graphics::OnAppSuspend() {}
void Graphics::OnAppUnsuspend() {}
void Graphics::OnAppShutdown() {}
void Graphics::OnAppShutdownComplete() {}
void Graphics::ApplyAppConfig() {}
void Graphics::Reset() {}

auto Graphics::IsShaderTransparent(ShadingType t) -> bool {
  // Simple check: types with "Transparent" in their name.
  switch (t) {
    case ShadingType::kSimpleColorTransparent:
    case ShadingType::kSimpleColorTransparentDoubleSided:
    case ShadingType::kSimpleTextureModulatedTransparent:
    case ShadingType::kSimpleTextureModulatedTransFlatness:
    case ShadingType::kSimpleTextureModulatedTransparentDoubleSided:
    case ShadingType::kSimpleTextureModulatedTransparentColorized:
    case ShadingType::kSimpleTextureModulatedTransparentColorized2:
    case ShadingType::kSimpleTextureModulatedTransparentColorized2Masked:
    case ShadingType::kSimpleTextureModulatedTransparentShadow:
    case ShadingType::kSimpleTexModulatedTransShadowFlatness:
    case ShadingType::kSimpleTextureModulatedTransparentGlow:
    case ShadingType::kSimpleTextureModulatedTransparentGlowMaskUV2:
    case ShadingType::kObjectTransparent:
    case ShadingType::kObjectLightShadowTransparent:
    case ShadingType::kObjectReflectTransparent:
    case ShadingType::kObjectReflectAddTransparent:
    case ShadingType::kSmoke:
    case ShadingType::kSmokeOverlay:
    case ShadingType::kPostProcess:
    case ShadingType::kPostProcessEyes:
    case ShadingType::kPostProcessNormalDistort:
    case ShadingType::kSprite:
    case ShadingType::kSpecial:
    case ShadingType::kShield:
      return true;
    default:
      return false;
  }
}

void Graphics::UpdatePlaceholderSettings() {}

// BaseFeatureSet.
static ShimAppAdapter s_app_adapter;
static GraphicsServer s_graphics_server;
// Graphics allocated on heap to avoid protected destructor issues.
static Graphics* s_graphics_ptr = nullptr;

BaseFeatureSet::BaseFeatureSet()
    : app_adapter(&s_app_adapter),
      app_config(nullptr),
      assets(nullptr),
      assets_server(nullptr),
      audio(nullptr),
      audio_server(nullptr),
      platform(nullptr),
      python(nullptr),
      bg_dynamics(nullptr),
      bg_dynamics_server(nullptr),
      context_ref(nullptr),
      graphics(s_graphics_ptr),
      graphics_server(&s_graphics_server),
      input(nullptr),
      logic(nullptr),
      networking(nullptr),
      network_reader(nullptr),
      network_writer(nullptr),
      stdio_console(nullptr),
      text_graphics(nullptr),
      ui(nullptr),
      utils(nullptr),
      discord(nullptr),
      app_mode_(nullptr) {}

auto BaseFeatureSet::Import() -> BaseFeatureSet* {
  if (!g_base) {
    g_base = new BaseFeatureSet();
  }
  return g_base;
}

void BaseFeatureSet::OnModuleExec(PyObject*) {}
void BaseFeatureSet::StartApp() {}
void BaseFeatureSet::SetAppActive(bool) {}
auto BaseFeatureSet::InGraphicsContext() const -> bool { return true; }
auto BaseFeatureSet::InLogicThread() const -> bool { return true; }
auto BaseFeatureSet::InAudioThread() const -> bool { return false; }
auto BaseFeatureSet::InAssetsThread() const -> bool { return false; }
auto BaseFeatureSet::InBGDynamicsThread() const -> bool { return false; }
auto BaseFeatureSet::InNetworkWriteThread() const -> bool { return false; }
auto BaseFeatureSet::IsUnmodifiedBlessedBuild() -> bool { return false; }
auto BaseFeatureSet::AppManagesMainThreadEventLoop() -> bool { return false; }
void BaseFeatureSet::RunAppToCompletion() {}
auto BaseFeatureSet::IsAppStarted() const -> bool { return true; }
void BaseFeatureSet::PlusDirectSendV1CloudLogs(const std::string&,
                                               const std::string&, bool,
                                               int*) {}
auto BaseFeatureSet::CreateFeatureSetData(FeatureSetNativeComponent*)
    -> PyObject* {
  return nullptr;
}
auto BaseFeatureSet::FeatureSetFromData(PyObject*)
    -> FeatureSetNativeComponent* {
  return nullptr;
}
void BaseFeatureSet::DoV1CloudLog(const std::string&) {}
void BaseFeatureSet::PushDevConsolePrintCall(std::string_view, float,
                                             Vector4f) {}
auto BaseFeatureSet::GetPyExceptionType(PyExcType) -> PyObject* {
  return nullptr;
}
auto BaseFeatureSet::PrintPythonStackTrace() -> bool { return false; }
auto BaseFeatureSet::GetPyLString(PyObject*) -> std::string { return ""; }
auto BaseFeatureSet::DoContextBaseString() -> std::string { return ""; }
void BaseFeatureSet::DoPrintContextAuto() {}
void BaseFeatureSet::DoPushObjCall(const PythonObjectSetBase*, int) {}
void BaseFeatureSet::DoPushObjCall(const PythonObjectSetBase*, int,
                                   const std::string&) {}
void BaseFeatureSet::ScreenMessage(const std::string&, const Vector3f&,
                                   bool) {}
void BaseFeatureSet::PushMainThreadRunnable(Runnable*) {}
void BaseFeatureSet::HandleInterruptSignal() {}
void BaseFeatureSet::HandleTerminateSignal() {}
auto BaseFeatureSet::IsAppBootstrapped() const -> bool { return true; }

}  // namespace ballistica::base

// ============================================================================
// InitEngineShims — call once at startup
// ============================================================================
void InitEngineShims() {
  using namespace ballistica;
  using namespace ballistica::core;
  using namespace ballistica::base;

  // Init core first.
  CoreFeatureSet::Import(nullptr);

  // Allocate settings objects (their default constructors access g_base,
  // so we use placement-new with zeroed memory to bypass that).
  s_demo_settings_ptr = reinterpret_cast<GraphicsSettings*>(
      calloc(1, sizeof(GraphicsSettings)));
  s_demo_client_ctx_ptr = reinterpret_cast<GraphicsClientContext*>(
      calloc(1, sizeof(GraphicsClientContext)));

  // Init base (this sets base::g_base, graphics=nullptr for now).
  s_graphics_ptr = nullptr;
  BaseFeatureSet::Import();

  // Sync base::g_core to point to the same core instance.
  ballistica::base::g_core = ballistica::core::g_core;

  // Settings/snapshots init skipped for now (Graphics not yet created).
}

auto GetShimGraphicsServer() -> ballistica::base::GraphicsServer* {
  return ballistica::base::g_base->graphics_server;
}

void ShimSetPassCamera(ballistica::base::RenderPass* pass,
                       float px, float py, float pz,
                       float tx, float ty, float tz,
                       float ux, float uy, float uz) {
  pass->cam_pos_ = ballistica::Vector3f(px, py, pz);
  pass->cam_target_ = ballistica::Vector3f(tx, ty, tz);
  pass->cam_up_ = ballistica::Vector3f(ux, uy, uz);
}

void ShimSetScreenResolution(float w, float h) {
  using namespace ballistica::base;
  // Update settings.
  (*s_demo_settings_ptr).resolution = ballistica::Vector2f(w, h);
  (*s_demo_settings_ptr).resolution_virtual = ballistica::Vector2f(w, h);
  (*s_demo_settings_ptr).index++;
  // Re-apply to GraphicsServer.
  auto* gs = g_base->graphics_server;
  gs->ApplySettings(&(*s_demo_settings_ptr));
}
