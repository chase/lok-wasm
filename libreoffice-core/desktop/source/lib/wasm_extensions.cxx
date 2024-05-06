#include "LibreOfficeKit/LibreOfficeKit.h"
#include "com/sun/star/frame/Desktop.hpp"
#include "com/sun/star/frame/XDesktop2.hdl"
#include "com/sun/star/uno/XComponentContext.hdl"
#include "comphelper/diagnose_ex.hxx"
#include "comphelper/seqstream.hxx"
#include "cppuhelper/exc_hlp.hxx"
#include "editeng/sizeitem.hxx"
#include "lib/init.hxx"
#include "sal/log.hxx"
#include "sal/types.h"
#include "sfx2/bindings.hxx"
#include "sfx2/dispatch.hxx"
#include "sfx2/viewfrm.hxx"
#include "sfx2/viewsh.hxx"
#include "svl/itemset.hxx"
#include "svl/poolitem.hxx"
#include "svx/rulritem.hxx"
#include "svx/xcolit.hxx"
#include "svx/xflclit.hxx"
#include "tools/json_writer.hxx"
#include "unotools/mediadescriptor.hxx"
#include <algorithm>
#include <cstdlib>
#include <lib/wasm_extensions.hxx>
#include <emscripten/bind.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <rtl/ustring.hxx>
#include <rtl/string.hxx>
#include <svx/svxids.hrc>

namespace desktop
{

static constexpr int MAX_THREADS_TO_NOTIFY = 2;

static void waitWhileInState(TileRendererData* data, RenderState state)
{
    emscripten_futex_wait(&data->state, (uint32_t)state, INFINITY);
}

static void changeState(TileRendererData* data, RenderState state)
{
    __c11_atomic_store(&data->state, state, __ATOMIC_SEQ_CST);
    emscripten_futex_wake(&data->state, MAX_THREADS_TO_NOTIFY);
}

static void* tileRendererWorker(void* data_)
{
    TileRendererData* d = static_cast<TileRendererData*>(data_);

    while (d->state != RenderState::QUIT)
    {
        RenderState state = d->state;
        switch (state)
        {
            case RenderState::IDLE:
                waitWhileInState(d, RenderState::IDLE); // owned by tile_renderer_worker.ts
                break;
            case RenderState::TILE_PAINT:
            {
                int nOrigViewId = d->doc->pClass->getView(d->doc);
                if (nOrigViewId != d->viewId)
                {
                    d->doc->pClass->setView(d->doc, d->viewId);
                }
                std::fill_n(d->paintedTile, d->paintedTileAllocSize, 0);
                d->doc->pClass->paintTile(d->doc, d->paintedTile, d->tileSize, d->tileSize,
                                          d->tileTwips[0], d->tileTwips[1], d->tileTwips[2],
                                          d->tileTwips[3]);
                if (nOrigViewId >= 0 && nOrigViewId != d->viewId)
                {
                    d->doc->pClass->setView(d->doc, nOrigViewId);
                }

                changeState(d, RenderState::IDLE);
                break;
            }
            case RenderState::RENDERING:
                // Wait for the JS worker to switching from RENDERING to other state
                waitWhileInState(d, RenderState::RENDERING); // owned by tile_renderer_worker.ts
                break;
            case RenderState::RESET:
            {
                d->reset();
                changeState(d, RenderState::IDLE);
                break;
            }
            case RenderState::QUIT:
                return nullptr;
        }
    }
    return nullptr;
}

TileRendererData& WasmDocumentExtension::startTileRenderer(int32_t viewId_, int32_t tileSize_)
{
    long w, h;
    pClass->getDocumentSize(this, &w, &h);
    TileRendererData& data = tileRendererData_.emplace_back(this, viewId_, tileSize_, w);
    pthread_t& threadId = tileRendererThreads_.emplace_back();
    if (pthread_create(&threadId, nullptr, tileRendererWorker, &data))
    {
        std::abort();
    }
    pthread_detach(threadId);

    return data;
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

static std::string OUStringToString(OUString str) {
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

    static constexpr std::string_view defaultColorHex = "\"#ffffff\"";


    SfxPoolItemHolder pState;
    const SfxItemState eState (pDispatch->QueryState(SID_ATTR_PAGE_COLOR, pState));
    if (eState < SfxItemState::DEFAULT) {
        return std::string (defaultColorHex);
    }
    if (pState.getItem())
    {
        XColorItem* pColor = static_cast<XColorItem*>(pState.getItem()->Clone());
        OUString aColorHex = u"\"" + pColor->GetColorValue().AsRGBHEXString() + u"\"";
        return OUStringToString(aColorHex);
    }
    return std::string (defaultColorHex);
}

std::string WasmDocumentExtension::getPageOrientation ()
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

    return bIsLandscape ? "\"landscape\"" : "\"portrait\"";
}

_LibreOfficeKitDocument* WasmOfficeExtension::documentExpandedLoad(std::vector<desktop::ExpandedPart>& parts, const char* pFilterOptions)
{
    return nullptr;
    LibreOfficeKitDocument* pDoc = NULL;
    desktop::WasmDocumentExtension* ext
        = static_cast<desktop::WasmDocumentExtension*>(pDoc);

    LibreOfficeKit* pThis = static_cast<LibreOfficeKit*>(this);


    ext->loadFromExpanded(pThis, parts);

    if (pDoc == NULL) {
        return NULL;
    }

    return pDoc;
}

using namespace com::sun::star;
_LibreOfficeKitDocument* WasmDocumentExtension::loadFromExpanded(LibreOfficeKit* pThis, const std::vector<desktop::ExpandedPart> &parts, const char* pFilterOptions)
{
    uno::XComponentContext * xContext =
        static_cast<uno::XComponentContext*>(pThis->pClass->getXComponentContext(pThis));

    if (!xContext) {
        return nullptr;
    }

    uno::Reference<frame::XDesktop2> xComponentLoader = frame::Desktop::create(xContext);

    if (!xComponentLoader.is())
    {
        SAL_WARN("lok", "ComponentLoader is not available");
        return nullptr;
    }


    auto aData = uno::Sequence<sal_Int8>(reinterpret_cast<const sal_Int8*>(parts.data()), parts.size());
    uno::Reference<io::XInputStream> aInputStream(new comphelper::SequenceInputStream(aData));

    utl::MediaDescriptor aMediaDescriptor;
    aMediaDescriptor["ExpandedStorage"] <<= true;
    aMediaDescriptor["FilterName"] <<= OUString("MS Word 2007 XML"); // just hardcode this for now
    aMediaDescriptor["InputStream"] <<= aInputStream;
    aMediaDescriptor["Silent"] <<= true;
    aMediaDescriptor["Hidden"] <<= true;

    {
        SolarMutexGuard aGuard;
        try
        {
            Application::SetDialogCancelMode(DialogCancelMode::LOKSilent);
            SfxViewShell::SetCurrentDocId(ViewShellDocId(0));
            uno::Reference<lang::XComponent> xComponent = xComponentLoader->loadComponentFromURL(
                "private:stream", "_blank", 0, aMediaDescriptor.getAsConstPropertyValueList());

            if (!xComponent.is()) {
                SAL_WARN("lok", "Could not load in memory doc");
                return nullptr;
            }

            return new LibLODocument_Impl(xComponent, 0);
        }
        catch (const uno::Exception& exception)
        {
            uno::Any exAny( cppu::getCaughtException() );
            SAL_WARN("lok", "Failed to load to in-memory stream: " + exceptionToString(exAny));
        }
    }
    return nullptr;




}

}
