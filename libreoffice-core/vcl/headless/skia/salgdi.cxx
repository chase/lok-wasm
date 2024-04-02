#include "SkSurface.h"
#include "vcl/fontcharmap.hxx"
#include "vcl/sysdata.hxx"
#include <headless/skia/salgdi.hxx>
#include <memory>

void SkiaSalGraphicsBackend::Init()
{
    SkiaSalGraphicsImpl::Init();
}

void SkiaSalGraphicsBackend::freeResources()
{
}

void SkiaSalGraphicsBackend::createWindowSurfaceInternal(bool forceRaster)
{
}

bool SkiaSalGraphicsBackend::avoidRecreateByResize() const
{
    return true;
}

SkiaSalGraphics::~SkiaSalGraphics()
{
}

void SkiaSalGraphics::GetResolution( sal_Int32& rDPIX, sal_Int32& rDPIY ){}

void SkiaSalGraphics::SetTextColor( Color nColor )
{
    m_aTextRenderImpl.SetTextColor( nColor );
}


void SkiaSalGraphics::SetFont( LogicalFontInstance* , int nFallbackLevel )
{
    m_aTextRenderImpl.SetFont( nullptr, nFallbackLevel );
}

void SkiaSalGraphics::GetFontMetric( ImplFontMetricDataRef& xFontMetric, int nFallbackLevel )
{
    m_aTextRenderImpl.GetFontMetric( xFontMetric, nFallbackLevel );
}

FontCharMapRef SkiaSalGraphics::GetFontCharMap() const
{
    return m_aTextRenderImpl.GetFontCharMap();
}

bool SkiaSalGraphics::GetFontCapabilities(vcl::FontCapabilities &rFontCapabilities) const
{
    return m_aTextRenderImpl.GetFontCapabilities(rFontCapabilities);
}

void SkiaSalGraphics::GetDevFontList( vcl::font::PhysicalFontCollection* rFontCollection)
{
    m_aTextRenderImpl.GetDevFontList(rFontCollection);
}

void SkiaSalGraphics::ClearDevFontCache()
{
    m_aTextRenderImpl.ClearDevFontCache();
}

bool SkiaSalGraphics::AddTempDevFont( vcl::font::PhysicalFontCollection* pFontCollection, const OUString& rFileURL, const OUString& rFontName )
{
    return m_aTextRenderImpl.AddTempDevFont(pFontCollection, rFileURL, rFontName);
}

std::unique_ptr<GenericSalLayout> SkiaSalGraphics::GetTextLayout(int nFallbackLevel)
{
    return m_aTextRenderImpl.GetTextLayout( nFallbackLevel );
}

void SkiaSalGraphics::DrawTextLayout( const GenericSalLayout& layout)
{
    m_aTextRenderImpl.DrawTextLayout( layout, *this );
}

SystemGraphicsData SkiaSalGraphics::GetGraphicsData() const
{
    return SystemGraphicsData();
}

SalGraphicsImpl* SkiaSalGraphics::GetImpl() const
{
    return m_pBackend->GetImpl();
}

SkSurface* SkiaSalGraphics::getSurface() const
{
    return m_pSkiaSurface;
}
