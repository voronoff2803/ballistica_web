// iOS entry point for full ballistica engine.

#import <UIKit/UIKit.h>
#import <OpenGLES/ES3/gl.h>
#import <GLKit/GLKit.h>

#include <string>

#include "ballistica/shared/ballistica.h"
#include "ballistica/core/core.h"
#include "ballistica/core/support/core_config.h"
#include "ballistica/base/base.h"
#include "ballistica/base/app_adapter/app_adapter_apple.h"
#include "ballistica/base/graphics/graphics.h"
#include "ballistica/base/graphics/graphics_server.h"
#include "ballistica/base/graphics/renderer/renderer.h"
#include "ballistica/base/input/input.h"
#include "ballistica/base/logic/logic.h"
#include "ballistica/shared/foundation/event_loop.h"

// Implement Swift stub functions.
#include "ballistica/shared/generic/runnable.h"
#include <mutex>
#include <vector>

// Queue for main-thread runnables. We process them during TryRender's
// spin loop (via DoPushGraphicsContextRunnable) AND in the draw callback.
static std::mutex g_main_runnables_mutex;
static std::vector<ballistica::Runnable*> g_main_runnables;

void ProcessPendingMainRunnables() {
  std::vector<ballistica::Runnable*> calls;
  {
    std::scoped_lock lock(g_main_runnables_mutex);
    if (!g_main_runnables.empty()) {
      g_main_runnables.swap(calls);
    }
  }
  for (auto* r : calls) {
    r->RunAndLogErrors();
    delete r;
  }
}

namespace BallisticaKit::FromCpp {

auto getResourcesPath() -> std::string {
  NSString* path = [[NSBundle mainBundle] resourcePath];
  return std::string([path UTF8String]);
}

auto getCacheDirectoryPath() -> std::string {
  NSArray* paths = NSSearchPathForDirectoriesInDomains(
      NSCachesDirectory, NSUserDomainMask, YES);
  if (paths.count > 0) {
    return std::string([paths[0] UTF8String]);
  }
  return std::string([NSTemporaryDirectory() UTF8String]);
}

void pushRawRunnableToMain(void* runnable_ptr) {
  auto* runnable = static_cast<ballistica::Runnable*>(runnable_ptr);
  static int pushCount = 0;
  pushCount++;
  if (pushCount <= 10) {
    fprintf(stderr, "[pushToMain] runnable #%d from thread %s\n",
            pushCount, [NSThread isMainThread] ? "main" : "bg");
  }
  dispatch_async(dispatch_get_main_queue(), ^{
    static int execCount = 0;
    execCount++;
    if (execCount <= 10) {
      fprintf(stderr, "[execOnMain] runnable #%d executed\n", execCount);
    }
    runnable->RunAndLogErrors();
    delete runnable;
  });
}

}  // namespace BallisticaKit::FromCpp


// =============================================================================
// App Delegate
// =============================================================================
@interface BAAppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

// =============================================================================
// Game View Controller
// =============================================================================
@interface BAGameViewController : GLKViewController
@end

@implementation BAAppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {

  self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
  BAGameViewController *vc = [[BAGameViewController alloc] init];
  self.window.rootViewController = vc;
  [self.window makeKeyAndVisible];
  return YES;
}

@end

@interface BAGameViewController () {
  EAGLContext *_context;
  bool _engineStarted;
  bool _renderThreadStarted;
  bool _kickedFirstDraw;
  bool _gotFirstFrame;
  bool _screenResSet;
  int _initStep;
  GLint _glkitFBO;
}
@end

@implementation BAGameViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
  if (!_context) {
    NSLog(@"Failed to create GL ES 3 context");
    return;
  }

  GLKView *view = (GLKView *)self.view;
  view.context = _context;
  view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
  view.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;
  view.multipleTouchEnabled = YES;
  self.preferredFramesPerSecond = 60;
  [EAGLContext setCurrentContext:_context];

  NSLog(@"GL_VERSION: %s", glGetString(GL_VERSION));

  // Start the engine using MonolithicMainIncremental.
  // This breaks init into pieces so we don't block the main thread too long.
  _engineStarted = false;
  [self startEngine];
}

- (void)startEngine {
  NSLog(@"Starting ballistica engine...");

  // Set working directory to the bundle resources path.
  NSString* resPath = [[NSBundle mainBundle] resourcePath];
  chdir([resPath UTF8String]);
  NSLog(@"Working directory: %@", resPath);
  NSLog(@"ba_data exists: %d", [[NSFileManager defaultManager]
      fileExistsAtPath:[resPath stringByAppendingPathComponent:@"ba_data"]]);
  NSLog(@"pylib exists: %d", [[NSFileManager defaultManager]
      fileExistsAtPath:[resPath stringByAppendingPathComponent:@"pylib"]]);

  _initStep = 0;
  [self continueEngineInit];
}

- (void)continueEngineInit {
  // Run multiple steps per dispatch to avoid per-step dispatch_async overhead.
  // On device, warm_start polling can take 500+ iterations.
  bool done = false;
  for (int batch = 0; batch < 50 && !done; batch++) {
    if (_initStep < 10 || _initStep % 100 == 0) {
      NSLog(@"Engine init step %d...", _initStep);
    }
    _initStep++;

    @try {
      ballistica::core::CoreConfig config;
      done = ballistica::MonolithicMainIncremental(
          _initStep == 1 ? &config : nullptr);
    } @catch (NSException *exception) {
      NSLog(@"Engine init exception at step %d: %@", _initStep, exception);
      return;
    }
  }

  if (!done) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self continueEngineInit];
    });
  } else {
    _engineStarted = true;
    NSLog(@"Engine init complete after %d steps!", _initStep);
    using namespace ballistica::base;

    // Screen resolution will be set in the draw callback where
    // drawableWidth/Height are guaranteed to be valid.

    // Ensure Graphics::Reset() has been called (creates Camera).
    // Without this, BuildAndPushFrameDef crashes on camera()->mode().
    if (!g_base->graphics->camera()) {
      NSLog(@"[post-init] Calling graphics->Reset() to create camera...");
      g_base->Reset();
    }
    NSLog(@"[post-init] graphics=%p has_client=%d",
          (void*)g_base->graphics,
          (int)g_base->graphics->has_client_context());
  }
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect {
  int w = (int)view.drawableWidth;
  int h = (int)view.drawableHeight;

  // Process any pending main-thread runnables from the engine.
  ProcessPendingMainRunnables();

  // Process pending main-thread runnables. Critical for engine communication.
  ProcessPendingMainRunnables();

  if (_engineStarted) {
    using namespace ballistica::base;

    // Set screen resolution from draw callback where drawable size is valid.
    if (!_screenResSet && w > 1 && h > 1) {
      _screenResSet = true;
      float pw = (float)w;
      float ph = (float)h;
      NSLog(@"[draw] Setting screen resolution: %.0f x %.0f", pw, ph);
      g_base->logic->event_loop()->PushCall([pw, ph] {
        g_base->graphics->SetScreenResolution(pw, ph);
      });
    }

    // Kick Logic::Draw() once after client context is ready.
    if (!_kickedFirstDraw && g_base->graphics->has_client_context()) {
      _kickedFirstDraw = true;
      NSLog(@"Kicking first Logic::Draw()");
      g_base->logic->event_loop()->PushCall([] { g_base->logic->Draw(); });
    }

    // Render on main thread (GLKit's GL context is current here).
    auto* adapter = AppAdapterApple::Get(g_base);
    if (adapter) {
      static int frameCount = 0;
      frameCount++;

      auto* gs = g_base->graphics_server;
      if (frameCount <= 20 || frameCount % 60 == 0) {
        NSLog(@"[render] frame=%d renderer=%p loaded=%d screen_target=%p w=%d h=%d has_client=%d",
              frameCount,
              (void*)gs->renderer(),
              (int)gs->renderer_loaded(),
              gs->renderer() ? (void*)gs->renderer()->screen_render_target() : nullptr,
              w, h,
              (int)g_base->graphics->has_client_context());
      }

      bool rendered = adapter->TryRender();
      if (frameCount <= 20 || frameCount % 60 == 0) {
        NSLog(@"[render] TryRender returned %d", (int)rendered);
      }

      if (rendered && !_gotFirstFrame) {
        _gotFirstFrame = true;
        NSLog(@"*** FIRST VISIBLE FRAME! ***");
      }
    }
  } else {
    glViewport(0, 0, w, h);
    glClearColor(0.05f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
  return UIInterfaceOrientationMaskLandscape;
}

- (BOOL)shouldAutorotate {
  return YES;
}

// =============================================================================
// Touch input
// =============================================================================
- (void)pushTouches:(NSSet<UITouch*>*)touches
               type:(ballistica::base::TouchEvent::Type)type {
  if (!_engineStarted) return;
  using namespace ballistica::base;
  UIView* v = self.view;
  CGFloat w = v.bounds.size.width;
  CGFloat h = v.bounds.size.height;
  if (w < 1 || h < 1) return;

  for (UITouch* t in touches) {
    CGPoint p = [t locationInView:v];
    TouchEvent ev;
    ev.type = type;
    ev.touch = (__bridge void*)t;
    ev.x = static_cast<float>(p.x / w);
    ev.y = 1.0f - static_cast<float>(p.y / h);
    g_base->input->PushTouchEvent(ev);
  }
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  [self pushTouches:touches type:ballistica::base::TouchEvent::Type::kDown];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  [self pushTouches:touches type:ballistica::base::TouchEvent::Type::kMoved];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  [self pushTouches:touches type:ballistica::base::TouchEvent::Type::kUp];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  [self pushTouches:touches type:ballistica::base::TouchEvent::Type::kCanceled];
}

@end

// =============================================================================
// main
// =============================================================================
int main(int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil,
                             NSStringFromClass([BAAppDelegate class]));
  }
}
