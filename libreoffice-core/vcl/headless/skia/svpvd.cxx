#include "SkImage.h"
#include "SkSurfaceProps.h"
#include "gl/GrGLExtensions.h"
#include "gl/GrGLInterface.h"
#include "salgdi.hxx"
#include <emscripten/html5_webgl.h>
#include <headless/skia/svpvd.hxx>
#include <SkSurface.h>
#include <SkCanvas.h>
#include <headless/svpinst.hxx>
#include <GrDirectContext.h>
#include <headless/skia/HeadlessSalInstance.hxx>
#include <memory>




SkiaSalVirtualDevice::SkiaSalVirtualDevice(SkiaSalGraphics& rGraphics,  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, tools::Long width, tools::Long height)

    : m_width(width), m_height(height), m_pGraphics(std::move(&rGraphics))
{

    sk_sp<const GrGLInterface> glInterface = GrGLMakeNativeInterface();
    m_grContext = GrDirectContext::MakeGL(glInterface);

    SkImageInfo info = SkImageInfo::MakeN32Premul(m_width, m_height);
    m_surface = SkSurface::MakeRenderTarget(m_grContext.get(), SkBudgeted::kNo, info);
}

SkiaSalVirtualDevice::~SkiaSalVirtualDevice() {
    m_surface.reset();
    m_grContext.reset();
}

SalGraphics* SkiaSalVirtualDevice::AcquireGraphics() {
    emscripten_console_log("acquiring graphics");
    return m_pGraphics.get();
}

void SkiaSalVirtualDevice::ReleaseGraphics(SalGraphics* pGraphics) {
    // Delete the SalGraphics instance
    delete pGraphics;
}

bool SkiaSalVirtualDevice::SetSize(tools::Long nNewDX, tools::Long nNewDY) {
    if (!nNewDX)
        nNewDX = 1;
    if (!nNewDY)
        nNewDY = 1;

    m_width = nNewDX;
    m_height = nNewDY;

    return true;
}

tools::Long SkiaSalVirtualDevice::GetWidth() const {
    return m_width;
}

tools::Long SkiaSalVirtualDevice::GetHeight() const {
    return m_height;
}
