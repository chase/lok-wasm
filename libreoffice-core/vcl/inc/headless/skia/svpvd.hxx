#include "SkSurface.h"
#include <salvd.hxx>
#include <vcl/salgtype.hxx>
#include <basegfx/vector/b2ivector.hxx>
#include "SkImage.h"
#include "SkSurfaceProps.h"
#include "gl/GrGLInterface.h"
#include "salgdi.hxx"
#include <emscripten/html5_webgl.h>
#include <SkSurface.h>
#include <SkCanvas.h>
#include <GrDirectContext.h>
#include <memory>

#include <vector>

class SvpSalGraphics;

class VCL_DLLPUBLIC SkiaSalVirtualDevice : public SalVirtualDevice
{
private:
    sk_sp<SkSurface> m_surface;
    sk_sp<GrDirectContext> m_grContext;
    tools::Long m_width;
    tools::Long m_height;
    std::unique_ptr<SkiaSalGraphics> m_pGraphics;

public:
    SkiaSalVirtualDevice(SkiaSalGraphics& rGraphics,  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, tools::Long width, tools::Long height);

    virtual ~SkiaSalVirtualDevice() override;

    virtual SalGraphics* AcquireGraphics() override;
    virtual void ReleaseGraphics(SalGraphics* pGraphics) override;
    virtual bool SetSize(tools::Long nNewDX, tools::Long nNewDY) override;
    virtual tools::Long GetWidth() const override;
    virtual tools::Long GetHeight() const override;
};
