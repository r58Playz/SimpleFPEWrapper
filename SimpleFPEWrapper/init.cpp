// SimpleFPEWrapper - SimpleFPEWrapper/init.cpp
// Copyright (c) 2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v3.0:
//   https://www.gnu.org/licenses/gpl-3.0.txt
//   https://www.gnu.org/licenses/lgpl-3.0.txt
// SPDX-License-Identifier: LGPL-3.0-only
// End of Source File Header

#include "init.h"
#include "fpe/fpe.hpp"

SFPEW::External::EGLFunctionsTable g_eglFuncs;
SFPEW::External::BackendGLFunctionsTable g_glFuncs;

#pragma GCC visibility push(default)
SFPEW_APIENTRY int sfpew_init_with_proc(
    __eglMustCastToProperFunctionPointerType (*downstreamProc)(const char*)) {
    static bool initialized = false;
    if (initialized) {
        return 1;
    }
    if (downstreamProc == nullptr) {
        return 0;
    }
    g_eglFuncs.eglGetProcAddress = downstreamProc;
    if (!SFPEW::Utils::BackendLoader::AcquireBackendGLFunctions(g_glFuncs, downstreamProc) ||
        g_glFuncs.glGetString == nullptr) {
        return 0;
    }
    init_fpe();
    initialized = true;
    return 1;
}
#pragma GCC visibility pop

#ifdef __EMSCRIPTEN__
// Backward-compatible entry point used by the wasm GLFW adapter.
extern "C" __attribute__((visibility("default")))
void sfpew_emscripten_init(__eglMustCastToProperFunctionPointerType (*downstreamProc)(const char*)) {
    (void)sfpew_init_with_proc(downstreamProc);
}
#else
void Init() {
    std::string eglLibName;
    const char* envEglLib = std::getenv("SFPEW_EGL");
    if (envEglLib) {
        eglLibName = envEglLib;
    } else {
        eglLibName = "libEGL.so";
    }

    if (!SFPEW::Utils::BackendLoader::AcquireEGLFunctions(g_eglFuncs, eglLibName) ||
        g_eglFuncs.eglGetProcAddress == nullptr) {
        throw std::runtime_error("Failed to acquire EGL functions");
    }

    if (!sfpew_init_with_proc(g_eglFuncs.eglGetProcAddress)) {
        throw std::runtime_error("Failed to acquire BackendGL functions");
    }
}

struct InitClass {
    InitClass() {
        const char* deferred = std::getenv("SFPEW_DEFER_INIT");
        if (!deferred || std::strcmp(deferred, "1") != 0) Init();
    }
};

static InitClass staticInitObject;
#endif // __EMSCRIPTEN__
