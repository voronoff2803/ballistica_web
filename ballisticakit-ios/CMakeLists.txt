cmake_minimum_required(VERSION 3.20)
project(BallisticaKit LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
endif()

set(APP_NAME BallisticaKit)
set(BA_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# =============================================================================
# Python 3.13 XCFramework
# =============================================================================
set(PYTHON_FRAMEWORK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/Python.xcframework)

# Determine the correct Python platform directory.
# Use Xcode's EFFECTIVE_PLATFORM_NAME or SDK to distinguish device vs simulator.
# At CMake configure time we can't reliably detect this, so we use a variable.
option(BA_IOS_DEVICE "Build for real iOS device (not simulator)" OFF)
if(BA_IOS_DEVICE)
    set(PYTHON_PLATFORM_DIR ${PYTHON_FRAMEWORK_DIR}/ios-arm64)
else()
    set(PYTHON_PLATFORM_DIR ${PYTHON_FRAMEWORK_DIR}/ios-arm64_x86_64-simulator)
endif()

set(PYTHON_INCLUDE_DIR ${PYTHON_PLATFORM_DIR}/include/python3.13)
set(PYTHON_LIB_DIR ${PYTHON_PLATFORM_DIR}/lib)
set(PYTHON_STDLIB_DIR ${PYTHON_FRAMEWORK_DIR}/lib/python3.13)

# =============================================================================
# ODE physics library
# =============================================================================
set(ODE_SRC_ROOT ${BA_SRC_ROOT}/external/open_dynamics_engine-ef)

file(GLOB ODE_SOURCES
    ${ODE_SRC_ROOT}/ode/*.cpp
)
add_library(ode STATIC ${ODE_SOURCES})
target_include_directories(ode PUBLIC ${ODE_SRC_ROOT})
target_include_directories(ode PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ode_stubs)
target_compile_options(ode PRIVATE -w)
target_compile_definitions(ode PRIVATE dNODEBUG=1 dTRIMESH_ENABLED=1)

# =============================================================================
# Engine sources — collect all .cc files, exclude platform-specific ones
# =============================================================================
file(GLOB_RECURSE ALL_ENGINE_CC "${BA_SRC_ROOT}/ballistica/*.cc")

# Exclude platform-specific files we don't want.
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "/linux/")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "/windows/")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "/android/")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "gl_sys_windows")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "/template_fs/")

# Exclude non-Apple adapters (we use the Apple adapter with Swift stub).
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "app_adapter_sdl\\.cc")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "app_adapter_headless\\.cc")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "app_adapter_vr\\.cc")

# Exclude VR graphics.
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "graphics_vr\\.cc")


# Exclude discord (requires external SDK).
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "discord\\.cc")

# Exclude logging.cc — we provide our own Log_ that doesn't use GIL.
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "core/logging/logging\\.cc$")

# Audio: compile the .cc files (they're gated by BA_ENABLE_AUDIO internally).
# ogg_stream.cc is now compiled with real Tremor library.

# Also collect .c files (OGG/Vorbis etc).
file(GLOB_RECURSE ALL_ENGINE_C "${BA_SRC_ROOT}/ballistica/*.c")

# Collect Objective-C++ files (.mm) for Apple platform code.
file(GLOB_RECURSE ALL_ENGINE_MM "${BA_SRC_ROOT}/ballistica/*.mm")

# External libraries (OGG, Vorbis, etc.).
file(GLOB_RECURSE EXTERNAL_C
    "${BA_SRC_ROOT}/external/open_dynamics_engine-ef/*.c"
)
# ODE is built separately, exclude it.
set(EXTERNAL_C "")

# OGG (libogg) + Tremor (integer Vorbis decoder) for audio.
# Built as a separate static library to avoid buildconfig conflicts
# (Tremor's block.h collides with system Block.h on case-insensitive FS).
set(OGG_TREMOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)
file(GLOB TREMOR_OGG_SOURCES
    ${OGG_TREMOR_DIR}/ogg/*.c
    ${OGG_TREMOR_DIR}/tremor/*.c
)
list(FILTER TREMOR_OGG_SOURCES EXCLUDE REGEX "example")
add_library(tremor_ogg STATIC ${TREMOR_OGG_SOURCES})
target_include_directories(tremor_ogg PRIVATE
    ${OGG_TREMOR_DIR}/ogg      # ogg/ogg.h
    ${OGG_TREMOR_DIR}/tremor   # internal headers
)
target_compile_options(tremor_ogg PRIVATE -w)

# Empty — Tremor/OGG compiled as separate lib, not mixed into main target.
set(OGG_VORBIS_C "")

# =============================================================================
# App target
# =============================================================================
add_executable(${APP_NAME} MACOSX_BUNDLE
    ${ALL_ENGINE_CC}
    ${ALL_ENGINE_C}
    ${ALL_ENGINE_MM}
    ${OGG_VORBIS_C}
    # iOS app entry + stubs.
    src/ios_main.mm
    src/ios_stubs.cc
    src/baplus_stub.cc
    # QR code generator.
    ${BA_SRC_ROOT}/external/qr_code_generator/QrCode.cpp
)

set_target_properties(${APP_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.ballistica.ios"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "M3WBMTG67K"
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
)

# Include paths.
target_include_directories(${APP_NAME} PRIVATE
    ${BA_SRC_ROOT}                    # ballistica/ headers
    ${BA_SRC_ROOT}/external           # external deps headers
    ${BA_SRC_ROOT}/external/openal-apple/include  # OpenAL headers
    ${OGG_TREMOR_DIR}/tremor-include        # ivorbisfile.h (forwarding)
    ${OGG_TREMOR_DIR}/ogg                  # OGG headers (ogg/ogg.h)
    ${PYTHON_INCLUDE_DIR}             # Python.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src   # local headers
)

# Force-include our build config before all sources.
target_compile_options(${APP_NAME} PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/src/buildconfig_ios.h
    -Wno-deprecated-declarations
)

target_compile_definitions(${APP_NAME} PRIVATE
    GLES_SILENCE_DEPRECATION=1
)

# On simulator, use DDS textures instead of PVR (PVRTC not supported).
if(NOT BA_IOS_DEVICE)
    target_compile_definitions(${APP_NAME} PRIVATE BA_IOS_DDS_TEXTURES=1)
endif()

# Link libraries.
target_link_libraries(${APP_NAME} PRIVATE
    ode
    tremor_ogg
    ${PYTHON_PLATFORM_DIR}/Python.framework/Python
    "-framework UIKit"
    "-framework Foundation"
    "-framework CoreGraphics"
    "-framework QuartzCore"
    "-framework OpenGLES"
    "-framework GLKit"
    "-framework OpenAL"
    "-framework AudioToolbox"
    "-framework AVFoundation"
    "-framework CoreHaptics"
    "-framework GameController"
    "-framework StoreKit"
)

# Embed Python dylib in the app bundle.
set_target_properties(${APP_NAME} PROPERTIES
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
)

# =============================================================================
# Copy resources into the app bundle.
# =============================================================================
# ba_data (game Python scripts).
set(BA_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/assets/ba_data)
# Python stdlib for Apple.
set(PYLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/assets/pylib-apple)
# Python stdlib from BeeWare framework.
set(PYLIB_BEEWARE ${PYTHON_FRAMEWORK_DIR}/lib/python3.13)

# Copy ba_data/python into bundle as ba_data/python.
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/python"
    COMMAND rsync -a --delete "${BA_DATA_DIR}/python/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/python/"
    COMMENT "Copying ba_data Python scripts..."
)

# Copy built game assets (textures, audio, data, fonts, models).
set(BUILT_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../build/assets/ba_data)
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND rsync -a "${BUILT_ASSETS_DIR}/textures/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/textures/" 2>/dev/null || true
    COMMAND rsync -a "${BUILT_ASSETS_DIR}/audio/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/audio/" 2>/dev/null || true
    COMMAND rsync -a "${BUILT_ASSETS_DIR}/data/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/data/" 2>/dev/null || true
    COMMAND rsync -a "${BUILT_ASSETS_DIR}/fonts/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/fonts/" 2>/dev/null || true
    COMMAND rsync -a "${BUILT_ASSETS_DIR}/meshes/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/meshes/" 2>/dev/null || true
    COMMENT "Copying game assets (textures, audio, data, fonts, meshes)..."
)

# Copy efro and bacommon tool packages (required by baenv.py).
set(TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tools)
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND rsync -a --delete "${TOOLS_DIR}/efro/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/python/efro/"
    COMMAND rsync -a --delete "${TOOLS_DIR}/bacommon/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/python/bacommon/"
    COMMENT "Copying efro and bacommon packages..."
)

# Copy pip packages from the project's .venv (needed by baenv).
set(SITE_PKGS ${CMAKE_CURRENT_SOURCE_DIR}/../.venv/lib/python3.13/site-packages)
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND rsync -a "${SITE_PKGS}/urllib3/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/python/urllib3/" 2>/dev/null || true
    COMMAND rsync -a "${SITE_PKGS}/certifi/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/ba_data/python/certifi/" 2>/dev/null || true
    COMMENT "Copying pip packages (urllib3, certifi)..."
)

# Copy pylib-apple (engine's curated stdlib subset) into bundle.
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:${APP_NAME}>/pylib"
    COMMAND rsync -a --delete "${PYLIB_DIR}/" "$<TARGET_BUNDLE_DIR:${APP_NAME}>/pylib/"
    COMMENT "Copying pylib-apple stdlib..."
)

# Copy Python binary modules (lib-dynload) into pylib/lib-dynload.
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:${APP_NAME}>/pylib/lib-dynload"
    COMMAND rsync -a "${PYTHON_PLATFORM_DIR}/lib-arm64/python3.13/lib-dynload/"
            "$<TARGET_BUNDLE_DIR:${APP_NAME}>/pylib/lib-dynload/"
    COMMENT "Copying Python binary modules..."
)

# Copy Python.framework into Frameworks/ in bundle.
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:${APP_NAME}>/Frameworks"
    COMMAND rsync -a "${PYTHON_PLATFORM_DIR}/Python.framework"
            "$<TARGET_BUNDLE_DIR:${APP_NAME}>/Frameworks/"
    COMMENT "Embedding Python.framework..."
)

# Sign all embedded code (required for device deployment).
# Signs Python.framework AND all .so modules in lib-dynload.
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND codesign --force --sign "6A4A9955F0E47A627E5A39359011A623617AE834"
            "$<TARGET_BUNDLE_DIR:${APP_NAME}>/Frameworks/Python.framework"
    COMMAND find "$<TARGET_BUNDLE_DIR:${APP_NAME}>/pylib/lib-dynload" -name "*.so"
            -exec codesign --force --sign "6A4A9955F0E47A627E5A39359011A623617AE834" {} +
    COMMENT "Signing Python.framework and .so modules..."
)
