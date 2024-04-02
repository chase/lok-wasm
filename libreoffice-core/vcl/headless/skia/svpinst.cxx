#include "sal/log.hxx"
#include "salinst.hxx"
#include "salvd.hxx"
#include <emscripten/html5_webgl.h>
#include <emscripten/threading.h>
#include <headless/skia/svpinst.hxx>
#include <headless/skia/svpvd.hxx>
#include <memory>

static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = 0;

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE createContext(char* id) {
    if (ctx != 0) {
        emscripten_webgl_destroy_context(ctx);
        ctx = 0;
    }

    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.minorVersion = 0;
    attr.majorVersion = 2;
    attr.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_ALWAYS;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE nctx = emscripten_webgl_create_context(id, &attr);

    if (nctx == 0) {
        SAL_WARN("vcl.headless", "failed to acquire webgl context from element");
        return 0;
    }

    EMSCRIPTEN_RESULT rctx = emscripten_webgl_make_context_current(ctx);

    if (rctx < 0) {
        SAL_WARN("vcl.headless", "failed to make context current");
    }

    return ctx;
}



std::unique_ptr<SalVirtualDevice> SkiaSalInstance::CreateVirtualDevice(SalGraphics& rGraphics, tools::Long& width, tools::Long& height, DeviceFormat pFormat, SystemGraphicsData const* pData) {
    SalVirtualDevice *pDevice = nullptr;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = createContext(strdup("#canvas"));

    pDevice = new SkiaSalVirtualDevice(static_cast<SkiaSalGraphics&>(rGraphics), context, 800, 1100);

    return std::unique_ptr<SalVirtualDevice>(pDevice);
}
