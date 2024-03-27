#include "SkCanvas.h"
#include "SkColor.h"
#include "SkPath.h"
#include "SkPoint.h"
#include "headless/skia/SkiaCommon.hxx"
#include <headless/skia/SvpGraphicsBackend.hxx>
#include <skia/gdiimpl.hxx>

SvpGraphicsBackend::SvpGraphicsBackend()
    :SkiaSalGraphicsImpl()
{
}

