#include "SkImage.h"
#include "SkSurfaceProps.h"
#include "gl/GrGLInterface.h"
#include "quartz/salgdi.h"
#include <emscripten/html5_webgl.h>
#include <headless/skia/skiavd.hxx>
#include <SkSurface.h>
#include <SkCanvas.h>
#include <headless/svpinst.hxx>
#include <headless/svpgdi.hxx>
#include <GrDirectContext.h>



class SkiaVirtualDevice : public SalVirtualDevice {
private:
    sk_sp<SkSurface> m_surface;
    sk_sp<GrDirectContext> m_grContext;
    tools::Long m_width;
    tools::Long m_height;

public:
    SkiaVirtualDevice(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, tools::Long width, tools::Long height)
        : m_width(width), m_height(height) {

        sk_sp<const GrGLInterface> glInterface = GrGLMakeNativeInterface();
        m_grContext = GrDirectContext::MakeGL(glInterface);

        SkImageInfo info = SkImageInfo::MakeN32Premul(m_width, m_height);
        SkSurfaceProps* props = new SkSurfaceProps();
        m_surface = SkSurface::MakeRenderTarget(m_grContext.get(), SkBudgeted::kNo, info, props, false);
    }

    virtual ~SkiaVirtualDevice() {
        m_surface.reset();
        m_grContext.reset();
    }

    virtual SalGraphics* AcquireGraphics() override {
        // Return a new SalGraphics instance associated with the Skia surface
        //
        /* return new AquaSalGraphics(m_surface->getCanvas()); */
    }

    virtual void ReleaseGraphics(SalGraphics* pGraphics) override {
        // Delete the SalGraphics instance
        delete pGraphics;
    }

    virtual bool SetSize(tools::Long nNewDX, tools::Long nNewDY) override {
        SkImageInfo info = SkImageInfo::MakeN32Premul(nNewDX, nNewDY);
        m_surface = SkSurface::MakeRenderTarget(m_grContext.get(), SkBudgeted::kNo, info);
        m_width = nNewDX;
        m_height = nNewDY;
        return true;
    }
};
