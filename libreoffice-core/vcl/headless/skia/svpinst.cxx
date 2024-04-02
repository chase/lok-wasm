#include "sal/log.hxx"
#include "salinst.hxx"
#include "salvd.hxx"
#include <emscripten/html5_webgl.h>
#include <emscripten/threading.h>
#include <headless/skia/svpinst.hxx>
#include <headless/skia/svpvd.hxx>
#include <memory>

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE createContext(char* id) {
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.minorVersion = 0;
    attr.majorVersion = 2;
    attr.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_ALWAYS;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_get_current_context();
    /* EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(g_offscreenCanvas, &attr); */
    EMSCRIPTEN_RESULT nctx = emscripten_webgl_make_context_current(ctx);
    SAL_WARN("vcl.headless", "Created context " << ctx << " and made it current: " << nctx);
    return ctx;
}



std::unique_ptr<SalVirtualDevice> SkiaSalInstance::CreateVirtualDevice(SalGraphics& rGraphics, tools::Long& width, tools::Long& height, DeviceFormat pFormat, SystemGraphicsData const* pData) {
    SalVirtualDevice *pDevice = nullptr;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = createContext(strdup("#canvas"));

    SAL_WARN("vcl.headless", "Creating virtual device with context " << context << " and size " << width << "x" << height);

    pDevice = new SkiaSalVirtualDevice(static_cast<SkiaSalGraphics&>(rGraphics), context, 1000, 1000);

    return std::unique_ptr<SalVirtualDevice>(pDevice);
}
