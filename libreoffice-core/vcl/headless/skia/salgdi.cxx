#include "vcl/fontcharmap.hxx"
#include "vcl/sysdata.hxx"
#include <headless/skia/salgdi.hxx>

SkiaSalGraphics::~SkiaSalGraphics()
{
}

void SkiaSalGraphics::GetResolution( sal_Int32& rDPIX, sal_Int32& rDPIY )
{
}

void SkiaSalGraphics::SetTextColor( Color nColor )
{
}


void SkiaSalGraphics::SetFont( LogicalFontInstance*, int nFallbackLevel )
{
}

void SkiaSalGraphics::GetFontMetric( ImplFontMetricDataRef&, int nFallbackLevel )
{
}

FontCharMapRef SkiaSalGraphics::GetFontCharMap() const
{
    return nullptr;
}

bool SkiaSalGraphics::GetFontCapabilities(vcl::FontCapabilities &rFontCapabilities) const
{
    return false;
}

void SkiaSalGraphics::GetDevFontList( vcl::font::PhysicalFontCollection* )
{
}

void SkiaSalGraphics::ClearDevFontCache()
{
}

bool SkiaSalGraphics::AddTempDevFont( vcl::font::PhysicalFontCollection*, const OUString& rFileURL, const OUString& rFontName )
{
    return false;
}

std::unique_ptr<GenericSalLayout> SkiaSalGraphics::GetTextLayout(int nFallbackLevel)
{
    return nullptr;
}

void SkiaSalGraphics::DrawTextLayout( const GenericSalLayout& )
{
}

SystemGraphicsData SkiaSalGraphics::GetGraphicsData() const
{
    return SystemGraphicsData();
}

SalGraphicsImpl* SkiaSalGraphics::GetImpl() const
{
    return nullptr;
}
