#include <unx/freetypetextrender.hxx>

class GenericSalLayout;
class SalGraphics;
typedef struct _skia skia_t;

class VCL_DLLPUBLIC SvpSkiaTextRender final : public FreeTypeTextRenderImpl
{
protected:
    virtual skia_t*             getSkiaContext() = 0;
    virtual void                getSurfaceOffset(double& nDX, double& nDY) = 0;
    virtual void                releaseSkiaContext(skia_t* cr) = 0;

    virtual void                clipRegion(cairo_t* cr) = 0;

public:
    virtual void                DrawTextLayout(const GenericSalLayout&, const SalGraphics&) override;
    SvpSkiaTextRender();
    virtual ~SvpSkiaTextRender();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
