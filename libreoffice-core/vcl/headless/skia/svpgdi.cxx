/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "salgdi.hxx"
#include "salgeom.hxx"
#include "vcl/dllapi.h"
#include <skia/gdiimpl.hxx>
#include <headless/skia/svpgdi.hxx>
#include <vcl/sysdata.hxx>

#include <SkCanvas.h>
#include <SkSurface.h>
#include <SkTypeface.h>
#include <SkFontMgr.h>

SvpSalGraphics::SvpSalGraphics()
{
}

SvpSalGraphics::~SvpSalGraphics()
{
}

void SvpSalGraphics::setSurface(sk_sp<SkSurface> pSurface, const basegfx::B2IVector& rSize)
{
    m_aSkiaCommon.setSurface(pSurface, rSize);
}

std::unique_ptr<SkCanvas> SvpSalGraphics::createTmpCompatibleSkCanvas() const
{
    return m_aSkiaCommon.createTmpCompatibleSkCanvas();
}

void SvpSalGraphics::GetResolution(sal_Int32& rDPIX, sal_Int32& rDPIY)
{
    rDPIX = 96;
    rDPIY = 96;
}

void SvpSalGraphics::SetTextColor(Color nColor)
{
    m_aTextRenderImpl.SetTextColor(nColor);
}

void SvpSalGraphics::SetFont(LogicalFontInstance* pFont, int nFallbackLevel)
{
    m_aTextRenderImpl.SetFont(pFont, nFallbackLevel);
}

void SvpSalGraphics::GetFontMetric(ImplFontMetricDataRef& rMetric, int nFallbackLevel)
{
    m_aTextRenderImpl.GetFontMetric(rMetric, nFallbackLevel);
}

FontCharMapRef SvpSalGraphics::GetFontCharMap() const
{
    return m_aTextRenderImpl.GetFontCharMap();
}

bool SvpSalGraphics::GetFontCapabilities(vcl::FontCapabilities& rFontCapabilities) const
{
    return m_aTextRenderImpl.GetFontCapabilities(rFontCapabilities);
}

void SvpSalGraphics::GetDevFontList(vcl::font::PhysicalFontCollection* pFontCollection)
{
    // TODO: Implement GetDevFontList using Skia and WebGL
}

void SvpSalGraphics::ClearDevFontCache()
{
    // TODO: Implement ClearDevFontCache using Skia and WebGL
}

bool SvpSalGraphics::AddTempDevFont(vcl::font::PhysicalFontCollection* pFontCollection,
                                     const OUString& rFileURL, const OUString& rFontName)
{
    // TODO: Implement AddTempDevFont using Skia and WebGL
    return false;
}

std::unique_ptr<GenericSalLayout> SvpSalGraphics::GetTextLayout(int nFallbackLevel)
{
    return m_aTextRenderImpl.GetTextLayout(nFallbackLevel);
}

void SvpSalGraphics::DrawTextLayout(const GenericSalLayout& rLayout)
{
    m_aTextRenderImpl.DrawTextLayout(rLayout);
}

bool SvpSalGraphics::ShouldDownscaleIconsAtSurface(double* pScaleOut) const
{
    // TODO: Implement ShouldDownscaleIconsAtSurface using Skia and WebGL
    return false;
}

SystemGraphicsData SvpSalGraphics::GetGraphicsData() const
{
    SystemGraphicsData aRes;
    aRes.nSize = sizeof(aRes);
    // TODO: Fill in the necessary graphics data using Skia and WebGL
    return aRes;
}
