#import "GameViewController.h"
#import <OpenGLES/ES3/gl.h>
#import "DemoScene.h"
#import "Renderer.h"
#import "engine_shims.h"

// Engine headers.
#include "ballistica/base/graphics/gl/renderer_gl.h"
#include "ballistica/base/graphics/graphics.h"
#include "ballistica/base/graphics/graphics_server.h"
#include "ballistica/base/graphics/renderer/render_pass.h"
#include "ballistica/base/graphics/support/frame_def.h"
#include "ballistica/base/graphics/support/render_command_buffer.h"
#include "ballistica/core/core.h"

using namespace ballistica;
using namespace ballistica::base;

@interface GameViewController () {
  EAGLContext* _context;
  ::Renderer* _miniRenderer;
  DemoScene* _scene;

  // Engine renderer.
  RendererGL* _engineRenderer;
  bool _engineRendererReady;
  int _frameNumber;
}
@end

@implementation GameViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
  if (!_context) {
    NSLog(@"Failed to create ES 3 context");
    return;
  }

  GLKView* view = (GLKView*)self.view;
  view.context = _context;
  view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
  view.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;
  view.multipleTouchEnabled = YES;

  self.preferredFramesPerSecond = 60;
  [EAGLContext setCurrentContext:_context];

  NSLog(@"GL_VERSION: %s", glGetString(GL_VERSION));
  NSLog(@"GL_RENDERER: %s", glGetString(GL_RENDERER));

  // Initialize engine shims (g_base, g_core globals).
  InitEngineShims();

  // Initialize the engine's RendererGL.
  _engineRendererReady = false;
  _frameNumber = 0;

  // Set screen resolution BEFORE creating renderer.
  int w = (int)view.drawableWidth;
  int h = (int)view.drawableHeight;
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  ShimSetScreenResolution(w, h);

  [self initEngineRenderer];

  // Keep mini-renderer + physics scene.
  _miniRenderer = new ::Renderer();
  if (!_miniRenderer->Init()) {
    NSLog(@"Mini-renderer init failed");
  }
  _scene = new DemoScene();
  _scene->Init();
}

- (void)initEngineRenderer {
  auto* gs = GetShimGraphicsServer();

  // Create and attach RendererGL.
  _engineRenderer = new RendererGL();
  gs->set_renderer(_engineRenderer);

  // Load — compiles all shader programs.
  NSLog(@"Loading engine RendererGL (compiling shaders)...");
  gs->LoadRenderer();
  NSLog(@"Engine RendererGL loaded successfully!");
  _engineRendererReady = true;
}

- (void)update {
  _scene->Step(1.0f / 60.0f);
}

- (void)renderEngineFrame:(int)w height:(int)h {
  try {
  auto* gs = GetShimGraphicsServer();

  // Create a fresh FrameDef.
  auto* fd = new FrameDef();
  fd->set_frame_number(_frameNumber++);
  fd->set_frame_number_filtered(_frameNumber / 2);
  fd->set_app_time_microsecs(g_core->AppTimeMicrosecs());
  fd->set_display_time_microsecs(g_core->AppTimeMicrosecs());
  fd->set_display_time_elapsed_microsecs(16667);  // ~60fps
  fd->set_needs_clear(true);

  // Skip FrameDef::Reset() — it accesses g_base->graphics->camera() which
  // is null. Instead, manually set the quality/settings fields we need.

  // Set camera on the beauty pass.
  auto* beauty = fd->beauty_pass();
  ShimSetPassCamera(beauty, 0, 12, 12, 0, 0, 0, 0, 1, 0);

  // Write draw commands to the beauty pass.
  // For kSimpleColor (opaque), use the world-list command buffer.
  auto* cmd = beauty->GetCommands(ShadingType::kSimpleColor);

  // Shader command: kSimpleColor with RGB.
  cmd->PutCommand(RenderCommandBuffer::Command::kShader);
  cmd->PutInt(static_cast<int>(ShadingType::kSimpleColor));
  cmd->PutFloats(0.2f, 0.6f, 1.0f);  // Blue color.

  // Draw a ground quad using debug draw.
  cmd->PutCommand(RenderCommandBuffer::Command::kPushTransform);
  cmd->PutCommand(RenderCommandBuffer::Command::kBeginDebugDrawTriangles);
  // Triangle 1.
  cmd->PutCommand(RenderCommandBuffer::Command::kDebugDrawVertex3);
  cmd->PutFloats(-5.0f, 0.0f, -5.0f);
  cmd->PutCommand(RenderCommandBuffer::Command::kDebugDrawVertex3);
  cmd->PutFloats(5.0f, 0.0f, -5.0f);
  cmd->PutCommand(RenderCommandBuffer::Command::kDebugDrawVertex3);
  cmd->PutFloats(5.0f, 0.0f, 5.0f);
  // Triangle 2.
  cmd->PutCommand(RenderCommandBuffer::Command::kDebugDrawVertex3);
  cmd->PutFloats(-5.0f, 0.0f, -5.0f);
  cmd->PutCommand(RenderCommandBuffer::Command::kDebugDrawVertex3);
  cmd->PutFloats(5.0f, 0.0f, 5.0f);
  cmd->PutCommand(RenderCommandBuffer::Command::kDebugDrawVertex3);
  cmd->PutFloats(-5.0f, 0.0f, 5.0f);
  cmd->PutCommand(RenderCommandBuffer::Command::kEndDebugDraw);
  cmd->PutCommand(RenderCommandBuffer::Command::kPopTransform);

  // Draw a red cube above it.
  auto* cmd2 = beauty->GetCommands(ShadingType::kSimpleColor);
  cmd2->PutCommand(RenderCommandBuffer::Command::kShader);
  cmd2->PutInt(static_cast<int>(ShadingType::kSimpleColor));
  cmd2->PutFloats(1.0f, 0.3f, 0.2f);  // Red color.

  cmd2->PutCommand(RenderCommandBuffer::Command::kPushTransform);
  cmd2->PutCommand(RenderCommandBuffer::Command::kTranslate3);
  cmd2->PutFloats(0.0f, 1.5f, 0.0f);
  cmd2->PutCommand(RenderCommandBuffer::Command::kBeginDebugDrawTriangles);
  // Simple cube using 12 triangles (36 vertices).
  float s = 1.0f;
  float v[][3] = {
    {-s,-s,-s},{s,-s,-s},{s,s,-s}, {-s,-s,-s},{s,s,-s},{-s,s,-s},  // back
    {-s,-s,s},{s,s,s},{s,-s,s}, {-s,-s,s},{-s,s,s},{s,s,s},        // front
    {-s,-s,-s},{-s,s,s},{-s,-s,s}, {-s,-s,-s},{-s,s,-s},{-s,s,s},  // left
    {s,-s,-s},{s,-s,s},{s,s,s}, {s,-s,-s},{s,s,s},{s,s,-s},        // right
    {-s,s,-s},{s,s,-s},{s,s,s}, {-s,s,-s},{s,s,s},{-s,s,s},        // top
    {-s,-s,-s},{s,-s,s},{s,-s,-s}, {-s,-s,-s},{-s,-s,s},{s,-s,s},  // bottom
  };
  for (int i = 0; i < 36; i++) {
    cmd2->PutCommand(RenderCommandBuffer::Command::kDebugDrawVertex3);
    cmd2->PutFloats(v[i][0], v[i][1], v[i][2]);
  }
  cmd2->PutCommand(RenderCommandBuffer::Command::kEndDebugDraw);
  cmd2->PutCommand(RenderCommandBuffer::Command::kPopTransform);

  // Complete the beauty pass.
  beauty->Complete();

  // Render just the beauty pass directly — skip PreprocessFrameDef
  // (which needs settings snapshot and creates shadow buffers).
  auto* renderer = gs->renderer();
  auto* target = renderer->screen_render_target();
  if (target) {
    target->DrawBegin(true, 0.15f, 0.15f, 0.25f, 1.0f);  // dark blue clear
    beauty->Render(target, false);   // opaque pass
    beauty->Render(target, true);    // transparent pass
    renderer->FinishFrameDef(fd);
  }

  delete fd;
  } catch (const std::exception& e) {
    NSLog(@"Engine render exception: %s", e.what());
    _engineRendererReady = false;
  }
}

- (void)glkView:(GLKView*)view drawInRect:(CGRect)rect {
  int w = (int)view.drawableWidth;
  int h = (int)view.drawableHeight;

  if (false && _engineRendererReady) {  // TEMP disabled
    [self renderEngineFrame:w height:h];
  } else {
    _scene->Render(_miniRenderer, w, h);
  }
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  UIView* v = self.view;
  for (UITouch* t in touches) {
    CGPoint p = [t locationInView:v];
    float nx = (float)(p.x / v.bounds.size.width);
    float ny = (float)(p.y / v.bounds.size.height);
    _scene->SpawnFromTouch(nx, ny);
  }
}

- (void)dealloc {
  if (_scene) { delete _scene; _scene = nullptr; }
  if (_miniRenderer) { delete _miniRenderer; _miniRenderer = nullptr; }
  if ([EAGLContext currentContext] == _context) {
    [EAGLContext setCurrentContext:nil];
  }
}

@end
