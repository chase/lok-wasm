/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef IOS
#error This file is not for iOS
#endif

#include <sal/config.h>
#include <config_features.h>

#include <osl/endian.h>
#include <vcl/sysdata.hxx>

#include <font/PhysicalFontFace.hxx>
#include <salgdi.hxx>
#include <sallayout.hxx>
#include "headless/skia/svpskiatextrender.hxx"
#include <impfontmetricdata.hxx>

#include <headless/SvpGraphicsBackend.hxx>

#include <SkSurface.h>
#include <SkCanvas.h>
#include <emscripten.h>
#include <emscripten/html5.h>

struct BitmapBuffer;
class FreetypeFont;

class VCL_DLLPUBLIC SvpSalGraphics : public SalGraphicsAutoDelegateToImpl
{
    /* SkiaCommon m_aSkiaCommon; */
    /* SvpSkiaTextRender                   m_aTextRenderImpl; */
    std::unique_ptr<SvpGraphicsBackend> m_pBackend;

public:
    void setSurface(sk_sp<SkSurface> pSurface, const basegfx::B2IVector& rSize);
    /* sk_sp<SkSurface> getSurface() const { return m_aSkiaCommon.m_pSurface; } */

protected:
    std::unique_ptr<SkCanvas> createTmpCompatibleSkCanvas() const;

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

    /* SkCanvas* getSkCanvas() const */
    /* { */
    /*     return m_aSkiaCommon.getSkCanvas(); */
    /* } */

    /* void releaseSkCanvas(SkCanvas* canvas, const basegfx::B2DRange& rExtents) const */
    /* { */
    /*     return m_aSkiaCommon.releaseSkCanvas(canvas, rExtents); */
    /* } */

    /* void clipRegion(SkCanvas* canvas) */
    /* { */
    /*     m_aSkiaCommon.clipRegion(canvas); */
    /* } */
    /* void copySource(const SalTwoRect& rTR, sk_sp<SkImage> source) */
    /* { */
    /*     m_aSkiaCommon.copySource(rTR, source); */
    /* } */
};
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
