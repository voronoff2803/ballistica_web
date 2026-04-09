cmake_minimum_required(VERSION 3.20)
project(BallisticaKit LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BA_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# =============================================================================
# Python 3.13 (WASM build)
# =============================================================================
# These paths point to the CPython WASM build from the previous research step.
# Adjust if you move/rebuild CPython.
set(CPYTHON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/cpython-3.13")
set(CPYTHON_BUILD "${CPYTHON_ROOT}/builddir/emscripten-browser")

set(PYTHON_INCLUDE_DIR "${CPYTHON_ROOT}/Include")
set(PYTHON_BUILD_INCLUDE "${CPYTHON_BUILD}")
set(PYTHON_LIB "${CPYTHON_BUILD}/libpython3.13.a")
set(PYTHON_HACL_LIB "${CPYTHON_BUILD}/Modules/_hacl/libHacl_Hash_SHA2.a")
set(PYTHON_EXPAT_LIB "${CPYTHON_BUILD}/Modules/expat/libexpat.a")
set(PYTHON_MPDEC_LIB "${CPYTHON_BUILD}/Modules/_decimal/libmpdec/libmpdec.a")
set(PYTHON_STDLIB_DIR "${CPYTHON_ROOT}/Lib")

# =============================================================================
# ODE physics library
# =============================================================================
set(ODE_SRC_ROOT ${BA_SRC_ROOT}/external/open_dynamics_engine-ef)

file(GLOB ODE_SOURCES ${ODE_SRC_ROOT}/ode/*.cpp)
add_library(ode STATIC ${ODE_SOURCES})
target_include_directories(ode PUBLIC ${ODE_SRC_ROOT})
target_include_directories(ode PRIVATE ${BA_SRC_ROOT})
target_compile_options(ode PRIVATE -w)
target_compile_definitions(ode PRIVATE dNODEBUG=1 dTRIMESH_ENABLED=1)
target_compile_options(ode PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/src/buildconfig_web.h
)

# =============================================================================
# Engine sources
# =============================================================================
file(GLOB_RECURSE ALL_ENGINE_CC "${BA_SRC_ROOT}/ballistica/*.cc")

# Exclude platform-specific files.
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "/linux/")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "/windows/")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "/android/")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "/apple/")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "gl_sys_windows")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "/template_fs/")

# Exclude adapters we don't use.
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "app_adapter_sdl\\.cc")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "app_adapter_headless\\.cc")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "app_adapter_vr\\.cc")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "app_adapter_apple\\.cc")
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "app_adapter_android\\.cc")

# Exclude VR graphics.
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "graphics_vr\\.cc")

# Exclude discord.
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "discord\\.cc")

# Exclude logging.cc — we provide our own in web_stubs.cc.
list(FILTER ALL_ENGINE_CC EXCLUDE REGEX "core/logging/logging\\.cc$")

# Exclude .mm files (Objective-C++, Apple only).
# Also exclude any .m files.

# Collect .c files.
file(GLOB_RECURSE ALL_ENGINE_C "${BA_SRC_ROOT}/ballistica/*.c")

# =============================================================================
# App target
# =============================================================================
add_executable(${PROJECT_NAME}
    ${ALL_ENGINE_CC}
    ${ALL_ENGINE_C}
    # Web-specific sources.
    src/web_main.cc
    src/web_stubs.cc
    src/baplus_stub.cc
    src/app_adapter_web.cc
    # QR code generator.
    ${BA_SRC_ROOT}/external/qr_code_generator/QrCode.cpp
)

# Include paths.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${BA_SRC_ROOT}                    # ballistica/ headers
    ${BA_SRC_ROOT}/external           # external deps headers
    ${PYTHON_INCLUDE_DIR}             # Python.h
    ${PYTHON_BUILD_INCLUDE}           # pyconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src   # local headers (app_adapter_web.h etc.)
    $ENV{EMSDK}/upstream/emscripten/cache/sysroot/include/AL  # OpenAL headers
)

# Force-include our build config before all sources.
target_compile_options(${PROJECT_NAME} PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/src/buildconfig_web.h
    -Wno-deprecated-declarations
    -fexceptions
    -sUSE_OGG=1
    -sUSE_VORBIS=1
)

# Link libraries.
target_link_libraries(${PROJECT_NAME} PRIVATE
    ode
    ${PYTHON_LIB}
    ${PYTHON_HACL_LIB}
    ${PYTHON_EXPAT_LIB}
    ${PYTHON_MPDEC_LIB}
)

# =============================================================================
# Emscripten link flags
# =============================================================================
set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".html"
)

target_link_options(${PROJECT_NAME} PRIVATE
    -sALLOW_MEMORY_GROWTH=1
    -sALLOW_TABLE_GROWTH=1
    -sTOTAL_MEMORY=268435456       # 256MB initial
    -sWASM_BIGINT=1
    -sFORCE_FILESYSTEM=1
    -sUSE_ZLIB=1
    -sUSE_BZIP2=1
    -sERROR_ON_UNDEFINED_SYMBOLS=0
    -sEXIT_RUNTIME=1
    -sABORTING_MALLOC=0
    -sSTACK_SIZE=1048576
    -sNO_DISABLE_EXCEPTION_CATCHING
    -lopenal
    -lidbfs.js
    -sUSE_OGG=1
    -sUSE_VORBIS=1
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    -sFULL_ES3=1
    -O2
    "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    # Preload Python stdlib into virtual filesystem at ./pylib
    # (the engine looks for stdlib at <data_dir>/pylib).
    "--preload-file=${PYTHON_STDLIB_DIR}@/pylib"
    # Preload game Python scripts (ba_data/python contains baenv.py, game packages).
    "--preload-file=${BA_SRC_ROOT}/assets/ba_data/python@/ba_data/python"
    # Preload tool packages required by baenv.
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/../tools/efro@/ba_data/python/efro"
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/../tools/bacommon@/ba_data/python/bacommon"
    # Preload game data (languages, maps, etc.)
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/data@/ba_data/data"
    # Preload game assets (textures, meshes, fonts).
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets_textures@/ba_data/textures"
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets_textures2@/ba_data/textures2"
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets_meshes@/ba_data/meshes"
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets_fonts@/ba_data/fonts"
    # Audio stub (blank.ogg fallback for missing sounds).
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets_audio@/ba_data/audio"
)
