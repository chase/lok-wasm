#include "SkImage.h"
#include "SkSurfaceProps.h"
#include "gl/GrGLInterface.h"
#include "salgdi.hxx"
#include <emscripten/html5_webgl.h>
#include <headless/skia/svpvd.hxx>
#include <SkSurface.h>
#include <SkCanvas.h>
#include <headless/svpinst.hxx>
#include <headless/skia/svpgdi.hxx>
#include <GrDirectContext.h>
#include <headless/skia/salgdi.hxx>
#include <memory>



class SkiaVirtualDevice : public SalVirtualDevice {
private:
    sk_sp<SkSurface> m_surface;
    sk_sp<GrDirectContext> m_grContext;
    tools::Long m_width;
    tools::Long m_height;
    std::unique_ptr<SkiaSalGraphics> m_pGraphics;

public:
    SkiaVirtualDevice(SkiaSalGraphics& rGraphics,  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, tools::Long width, tools::Long height)
        : m_width(width), m_height(height), m_pGraphics(std::move(&rGraphics)) {

        sk_sp<const GrGLInterface> glInterface = GrGLMakeNativeInterface();
        m_grContext = GrDirectContext::MakeGL(glInterface);

        SkImageInfo info = SkImageInfo::MakeN32Premul(m_width, m_height);
        m_surface = SkSurface::MakeRenderTarget(m_grContext.get(), SkBudgeted::kNo, info);
    }

    virtual ~SkiaVirtualDevice() {
        m_surface.reset();
        m_grContext.reset();
    }

    virtual SalGraphics* AcquireGraphics() override {
        return m_pGraphics.get();
    }

    virtual void ReleaseGraphics(SalGraphics* pGraphics) override {
        // Delete the SalGraphics instance
        delete pGraphics;
    }

    virtual bool SetSize(tools::Long nNewDX, tools::Long nNewDY) override {
        if (!nNewDX)
            nNewDX = 1;
        if (!nNewDY)
            nNewDY = 1;

        m_width = nNewDX;
        m_height = nNewDY;

    }
};
