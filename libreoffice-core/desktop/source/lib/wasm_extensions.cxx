#include "sfx2/sfxsids.hrc"
#include <com/sun/star/document/MacroExecMode.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/XDesktop2.hpp>
#include <com/sun/star/ucb/OpenMode.hpp>
#include <com/sun/star/uno/Any.h>
#include <com/sun/star/uno/Reference.h>
#include <comphelper/base64.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/seqstream.hxx>
#include <comphelper/vecstream.hxx>
#include <comphelper/storagehelper.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <lib/init.hxx>
#include <sal/log.hxx>
#include <sot/stg.hxx>
#include <unotools/mediadescriptor.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <editeng/sizeitem.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/viewsh.hxx>
#include <svl/itemset.hxx>
#include <svl/poolitem.hxx>
#include <svx/rulritem.hxx>
#include <svx/xcolit.hxx>
#include <svx/xflclit.hxx>
#include <cstdlib>
#include <lib/wasm_extensions.hxx>
#include <emscripten/bind.h>
#include <emscripten/threading.h>
#include <rtl/ustring.hxx>
#include <rtl/string.hxx>
#include <svx/svxids.hrc>

namespace desktop
{
using cppu::getCaughtException;

static constexpr int MAX_THREADS_TO_NOTIFY = 2;

// Convert tile index to grid coordinates
std::array<int, 2> tileIndexToGridCoord(int tileIndex, int widthTileStride)
{
    const int y = tileIndex / widthTileStride;
    const int x = tileIndex % widthTileStride;
    return { x, y };
}

// Convert tile index to twips rectangle
std::array<int, 4> tileIndexToTwipsRect(int tileIndex, int widthTileStride, int tileDimTwips)
{
    const auto [x, y] = tileIndexToGridCoord(tileIndex, widthTileStride);
    return { x * tileDimTwips, y * tileDimTwips, tileDimTwips, tileDimTwips };
}

WasmDocumentExtension::WasmDocumentExtension(css::uno::Reference<css::lang::XComponent> xComponent)
    : mxComponent(std::move(xComponent))
{
}

void WasmDocumentExtension::paintTiles(uint32_t startIndex, uint32_t endIndex)
{
    if (!tileRendererData_.has_value())
        return;
    auto& d = tileRendererData_.value();

    // this shouldn't happen except for after the tile renderer is stopped
    if (!d.paintedTiles)
        return;

    d.doc->pClass->setView(d.doc, d.viewId);
    // without clearing the painted tiles, any later paints will be composited on top of the existing data
    __builtin_memset(d.paintedTiles.get(), 0, d.tileAllocSize * (endIndex - startIndex + 1));
    for (uint32_t i = startIndex; i <= endIndex; ++i)
    {
        uint32_t byteOffset = (i - startIndex) * d.tileAllocSize;
        auto rect = tileIndexToTwipsRect(i, d.widthTileStride, d.tileDimTwips);
        d.doc->pClass->paintTile(d.doc, &d.paintedTiles[byteOffset], d.tileSize, d.tileSize,
                                 rect[0], rect[1], rect[2], rect[3]);
    }
}

TileRendererData& WasmDocumentExtension::startTileRenderer(int32_t viewId, int32_t tileSize)
{
    long w, h;
    pClass->getDocumentSize(this, &w, &h);
    TileRendererData& data = tileRendererData_.emplace(this, viewId, tileSize, w, h);
    g_activeTileRenderData = &data;
    return data;
}

void WasmDocumentExtension::stopTileRenderer(int32_t /* viewId */)
{
    if (tileRendererData_.has_value())
    {
        if (g_activeTileRenderData == &tileRendererData_.value())
        {
            g_activeTileRenderData = nullptr;
        }

        // This probably isn't necessary anymore, but just in case
        tileRendererData_->paintedTiles.reset();
        tileRendererData_.reset();
    }
    else
    {
        SAL_WARN("tile", "missing tile render data");
    }
}

void TileRendererData::pushInvalidation(uint32_t invalidation[4])
{
    int32_t head = __c11_atomic_fetch_add(&invalidationStackHead, 1, __ATOMIC_RELAXED) + 1;
    if (head > MAX_INVALIDATION_STACK)
    {
        // TODO: should probably warn here, but unlikely
        return;
    }
    for (int i = 0; i < 4; ++i)
    {
        __c11_atomic_store(&invalidationStack[head][i], invalidation[i], __ATOMIC_RELAXED);
    }
    __c11_atomic_store(&hasInvalidations, 1, __ATOMIC_RELAXED);
    __builtin_wasm_memory_atomic_notify((int32_t*)&hasInvalidations, MAX_THREADS_TO_NOTIFY);
}

void TileRendererData::reset()
{
    __c11_atomic_store(&invalidationStackHead, -1, __ATOMIC_RELAXED);
    __c11_atomic_store(&pendingFullPaint, 1, __ATOMIC_SEQ_CST);
    __c11_atomic_store(&hasInvalidations, 1, __ATOMIC_SEQ_CST);
    __builtin_wasm_memory_atomic_notify((int32_t*)&hasInvalidations, MAX_THREADS_TO_NOTIFY);
}

static std::string OUStringToString(OUString str)
{
    return OUStringToOString(str, RTL_TEXTENCODING_UTF8).getStr();
}

std::string WasmDocumentExtension::getPageColor()
{
    SfxViewFrame* pViewFrame = SfxViewFrame::Current();

    SfxDispatcher* pDispatch(pViewFrame->GetDispatcher());
    if (!pViewFrame)
    {
        return nullptr;
    }

    static constexpr std::string_view defaultColorHex = "#ffffff";

    SfxPoolItemHolder pState;
    const SfxItemState eState(pDispatch->QueryState(SID_ATTR_PAGE_COLOR, pState));
    if (eState < SfxItemState::DEFAULT)
    {
        return std::string(defaultColorHex);
    }
    if (pState.getItem())
    {
        XColorItem* pColor = static_cast<XColorItem*>(pState.getItem()->Clone());
        OUString aColorHex = pColor->GetColorValue().AsRGBHEXString();
        return OUStringToString(aColorHex);
    }
    return std::string(defaultColorHex);
}

std::string WasmDocumentExtension::getPageOrientation()
{
    SfxViewFrame* pViewFrm = SfxViewFrame::Current();
    if (!pViewFrm)
    {
        return nullptr;
    }

    SfxPoolItemHolder pState;
    pViewFrm->GetBindings().GetDispatcher()->QueryState(SID_ATTR_PAGE_SIZE, pState);
    SvxSizeItem* pSize = static_cast<SvxSizeItem*>(pState.getItem()->Clone());

    bool bIsLandscape = (pSize->GetSize().Width() >= pSize->GetSize().Height());

    return bIsLandscape ? "landscape" : "portrait";
}

void WasmDocumentExtension::save()
{
    SfxViewFrame* viewFrame = SfxViewFrame::Current();
    if (!viewFrame)
    {
        return;
    }

    // TODO: investigate why we used this?
    viewFrame->GetBindings().ExecuteSynchron(SID_SAVEDOC);
    // TODO: implement
}

std::optional<std::string> WasmDocumentExtension::getCursor(int viewId)
{
    if (SfxViewShell* viewShell
        = SfxViewShell::GetFirst(false, [viewId](const SfxViewShell* shell)
                                 { return shell->GetViewShellId().get() == viewId; }))
    {
        std::optional<OString> payload
            = viewShell->getLOKPayload(LOK_CALLBACK_INVALIDATE_VISIBLE_CURSOR, viewId);
        if (payload)
        {
            return std::string(static_cast<std::string_view>(*payload));
        }
    }
    return {};
}

_LibreOfficeKitDocument* WasmOfficeExtension::loadDocumentFromExpanded(std::string name,
                                                                       const int documentId,
                                                                       const bool readOnly,
                                                                       const bool loadInPlace)
{
    return nullptr;
}

}
