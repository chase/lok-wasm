#include "salinst.hxx"
#include "salvd.hxx"
#include <emscripten/html5_webgl.h>
#include <headless/skia/svpinst.hxx>
#include <headless/skia/svpvd.hxx>
#include <memory>

std::unique_ptr<SalVirtualDevice> SkiaSalInstance::createVirtualDevice(SalGraphics& rGraphics, tools::Long width, tools::Long height, DeviceFormat pFormat, SystemGraphicsData const* pData) {

    SalVirtualDevice *pDevice = nullptr;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_get_current_context();

    pDevice = new SkiaSalVirtualDevice(static_cast<SkiaSalGraphics&>(rGraphics), context, width, height);

    return std::unique_ptr<SalVirtualDevice>(pDevice);
}

std::unique_ptr<SalVirtualDevice> SkiaSalInstance::CreateVirtualDevice(SalGraphics& rGraphics, tools::Long& width, tools::Long& height, DeviceFormat eFormat, SystemGraphicsData const* pData) {

    SalVirtualDevice *pDevice = nullptr;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_get_current_context();

    pDevice = new SkiaSalVirtualDevice(static_cast<SkiaSalGraphics&>(rGraphics), context, width, height);

    return std::unique_ptr<SalVirtualDevice>(pDevice);
}

OpenGLContext* SkiaSalInstance::CreateOpenGLContext() {
    return nullptr;
}

