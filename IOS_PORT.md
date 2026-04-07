# BombSquad iOS Port

Experimental iOS port of [BombSquad](https://github.com/efroemling/ballistica) (ballistica engine), built in one evening with [Claude Code](https://claude.ai/code).

## What Works

- Full 3D rendering at 60fps (OpenGL ES 3.0)
- Main menu, game settings, team/player selection
- Gameplay with physics (ODE trimesh collisions)
- Touch controls (on-screen joystick + action buttons)
- Sound (OGG Vorbis via Tremor decoder + OpenAL)
- Landscape orientation with safe area borders

## What Doesn't Work (Yet)

- Online accounts / cloud features (`_baplus` is a stub module)
- MFi / Bluetooth game controllers
- In-app purchases, Game Center
- Dynamic text textures with emoji (transparent stubs, no CoreText)

## Building

### Prerequisites

- Xcode 15+ with iOS SDK
- CMake 3.20+
- Python 3.13 XCFramework at `ballisticakit-ios/deps/Python.xcframework/` (from [BeeWare](https://github.com/beeware/Python-Apple-support))
- Pre-built game assets in `build/assets/ba_data/` (run the main project's asset build first)

### Device Build

```bash
mkdir build-ios-device && cd build-ios-device
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DBA_IOS_DEVICE=ON ../ballisticakit-ios
xcodebuild -scheme BallisticaKit -sdk iphoneos -configuration Debug build
```

Open `build-ios-device/BallisticaKit.xcodeproj` in Xcode to deploy to a device.

### Simulator Build

```bash
mkdir build-ios && cd build-ios
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS ../ballisticakit-ios
xcodebuild -scheme BallisticaKit -sdk iphonesimulator -configuration Debug build
```

Note: Simulator uses DDS textures (PVRTC not supported), device uses PVR.

## Project Structure

```
ballisticakit-ios/
  CMakeLists.txt          # iOS build configuration
  Info.plist              # App manifest (landscape-only)
  src/
    ios_main.mm           # App delegate, GLKit view controller, touch input
    ios_stubs.cc          # PlusSoftInterface stub, logging override
    baplus_stub.cc        # _baplus Python C extension (stub for accounts/cloud)
    buildconfig_ios.h     # Compile-time flags for iOS
    BallisticaKit-Swift.h # Swift interop stubs (TextTextureData, etc.)
  deps/
    Python.xcframework/   # Python 3.13 for iOS
    tremor/               # Integer Vorbis decoder
    ogg/                  # libogg
  vorbis_stubs/           # (legacy, unused with Tremor)
  ode_stubs/              # ODE build compatibility header
```

## Engine Changes

4 minimal changes to the upstream engine (all guarded by `#if`):

| File | Change |
|------|--------|
| `app_adapter_apple.cc` | `TerminateApp()` → `exit(0)` on iOS; `HasDirectKeyboardInput()` → false |
| `assets.cc` | Use `.dds` textures on iOS simulator (`BA_IOS_DDS_TEXTURES`) |
| `render_target.cc` | Clamp framebuffer size to 1x1 minimum |
| `_profile.py` | Use `.get()` to avoid `KeyError` on empty config |
