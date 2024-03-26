#pragma once

#include "SkSurface.h"
#ifdef IOS
#error This file is not for iOS
#endif

#include <sal/config.h>
#include <config_features.h>

#include <osl/endian.h>
#include <vcl/sysdata.hxx>
#include <config_cairo_canvas.h>

#include <font/PhysicalFontFace.hxx>
#include <salgdi.hxx>
#include <sallayout.hxx>
#include <impfontmetricdata.hxx>

#include <headless/SvpGraphicsBackend.hxx>
#include <headless/CairoCommon.hxx>

struct BitmapBuffer;
class FreetypeFont;

class VCL_DLLPUBLIC SvpSalGraphics : public SalGraphicsAutoDelegateToImpl
{
    SvpCairoTextRender                  m_aTextRenderImpl;
    std::unique_ptr<SvpGraphicsBackend> m_pBackend;

public:
    void setSurface(SkSurface* pSurface, const basegfx::B2IVector& rSize);
    SkSurface* getSurface() const { return m_aCairoCommon.m_pSurface; }
    static cairo_user_data_key_t* getDamageKey()
    {
        return CairoCommon::getDamageKey();
    }

protected:

    cairo_t* createTmpCompatibleCairoContext() const;

public:
    SvpSalGraphics();
    virtual ~SvpSalGraphics() override;

    virtual SalGraphicsImpl* GetImpl() const override { return m_pBackend.get(); }
    std::unique_ptr<SvpGraphicsBackend> const& getSvpBackend() { return m_pBackend; }

    virtual void            GetResolution( sal_Int32& rDPIX, sal_Int32& rDPIY ) override;

    virtual void            SetTextColor( Color nColor ) override;
    virtual void            SetFont(LogicalFontInstance*, int nFallbackLevel) override;
    virtual void            GetFontMetric( ImplFontMetricDataRef&, int nFallbackLevel ) override;
    virtual FontCharMapRef  GetFontCharMap() const override;
    virtual bool GetFontCapabilities(vcl::FontCapabilities &rFontCapabilities) const override;
    virtual void            GetDevFontList( vcl::font::PhysicalFontCollection* ) override;
    virtual void ClearDevFontCache() override;
    virtual bool            AddTempDevFont( vcl::font::PhysicalFontCollection*, const OUString& rFileURL, const OUString& rFontName ) override;
    virtual std::unique_ptr<GenericSalLayout>
                            GetTextLayout(int nFallbackLevel) override;
    virtual void            DrawTextLayout( const GenericSalLayout& ) override;

    virtual bool            ShouldDownscaleIconsAtSurface(double* pScaleOut) const override;

    virtual SystemGraphicsData GetGraphicsData() const override;

#if ENABLE_CAIRO_CANVAS
    virtual bool            SupportsCairo() const override;
    virtual cairo::SurfaceSharedPtr CreateSurface(const cairo::CairoSurfaceSharedPtr& rSurface) const override;
    virtual cairo::SurfaceSharedPtr CreateSurface(const OutputDevice& rRefDevice, int x, int y, int width, int height) const override;
    virtual cairo::SurfaceSharedPtr CreateBitmapSurface(const OutputDevice& rRefDevice, const BitmapSystemData& rData, const Size& rSize) const override;
    virtual css::uno::Any   GetNativeSurfaceHandle(cairo::SurfaceSharedPtr& rSurface, const basegfx::B2ISize& rSize) const override;
#endif // ENABLE_CAIRO_CANVAS

    cairo_t* getCairoContext() const
    {
        return m_aCairoCommon.getCairoContext(/*bXorModeAllowed*/false, getAntiAlias());
    }

    void releaseCairoContext(cairo_t* cr, const basegfx::B2DRange& rExtents) const
    {
        return m_aCairoCommon.releaseCairoContext(cr, /*bXorModeAllowed*/false, rExtents);
    }

    void clipRegion(cairo_t* cr)
    {
        m_aCairoCommon.clipRegion(cr);
    }
    void copySource(const SalTwoRect& rTR, cairo_surface_t* source)
    {
        m_aCairoCommon.copySource(rTR, source, getAntiAlias());
    }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
