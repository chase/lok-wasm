#include "vcl/dllapi.h"
#include <salgdi.hxx>
#include <skia/gdiimpl.hxx>

#pragma once

class SkiaSalGraphics; // Forward declaration of SkiaSalGraphics

class VCL_DLLPUBLIC SkiaSalGraphicsBackend final : public SkiaSalGraphicsImpl
{
public:
    SkiaSalGraphicsBackend(SalGraphicsAutoDelegateToImpl& rParent)
        : SkiaSalGraphicsImpl(rParent, nullptr)
        , mrParent(rParent)
    {
    }

    virtual void Init() override;
    virtual void freeResources() override;

    SalGraphicsImpl* GetImpl()
    {
        return this;
    }

private:
    SalGraphicsAutoDelegateToImpl& mrParent; // Reference to the parent SkiaSalGraphics instance

    virtual void createWindowSurfaceInternal(bool forceRaster = false) override;
    virtual bool avoidRecreateByResize() const override;
    static std::unique_ptr<sk_app::WindowContext>
    createWindowContext();
};

class VCL_DLLPUBLIC SkiaSalGraphics final : public SalGraphicsAutoDelegateToImpl
{
    SkSurface* m_pSkiaSurface;
public:
    SkiaSalGraphics()
        : m_pBackend(new SkiaSalGraphicsBackend(static_cast<SalGraphicsAutoDelegateToImpl&>(*this)))
    {
    }

    virtual ~SkiaSalGraphics() override;

    SkiaSalGraphicsBackend* getSkiaGraphicsBackend() const
    {
        return m_pBackend.get();
    }

    virtual void GetResolution( sal_Int32& rDPIX, sal_Int32& rDPIY ) override;
    virtual void SetTextColor( Color nColor ) override;
    virtual void SetFont( LogicalFontInstance*, int nFallbackLevel ) override;
    virtual void GetFontMetric( ImplFontMetricDataRef&, int nFallbackLevel ) override;
    virtual FontCharMapRef  GetFontCharMap() const override;
    virtual bool GetFontCapabilities(vcl::FontCapabilities &rFontCapabilities) const override;
    virtual void GetDevFontList( vcl::font::PhysicalFontCollection* ) override;
    virtual void ClearDevFontCache() override;
    virtual bool AddTempDevFont( vcl::font::PhysicalFontCollection*, const OUString& rFileURL, const OUString& rFontName ) override;
    virtual std::unique_ptr<GenericSalLayout> GetTextLayout(int nFallbackLevel) override;
    virtual void DrawTextLayout( const GenericSalLayout& ) override;
    virtual SystemGraphicsData GetGraphicsData() const override;
    virtual SalGraphicsImpl* GetImpl() const override;
    virtual SkSurface* getSurface() const;

private:
    std::unique_ptr<SkiaSalGraphicsBackend> m_pBackend;
};
