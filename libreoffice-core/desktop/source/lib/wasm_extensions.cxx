#include "editeng/sizeitem.hxx"
#include "sal/log.hxx"
#include "sfx2/bindings.hxx"
#include "sfx2/dispatch.hxx"
#include "sfx2/viewfrm.hxx"
#include "sfx2/viewsh.hxx"
#include "svl/poolitem.hxx"
#include "svx/rulritem.hxx"
#include "svx/xflclit.hxx"
#include "tools/json_writer.hxx"
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
    SfxViewShell* pViewShell = SfxViewShell::Current();
    SfxViewFrame* pViewFrm = pViewShell ? pViewShell->GetViewFrame() : nullptr;
    if (!pViewFrm)
    {
        return nullptr;
    }

    static constexpr std::string_view defaultColorHex = "\"#ffffff\"";

    std::unique_ptr<SfxPoolItem> pState;
    const SfxItemState eState (pViewFrm->GetBindings().QueryState(SID_ATTR_PAGE_COLOR, pState));
    if (eState < SfxItemState::DEFAULT) {
        return std::string (defaultColorHex);
    }
    if (pState)
    {
        OUString aColorHex = u"\"" + static_cast<XFillColorItem*>(pState->Clone())->GetColorValue().AsRGBHEXString() + u"\"";
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
    const SvxSizeItem* pSizeItem;
    pViewFrm->GetBindings().GetDispatcher()->QueryState(SID_ATTR_PAGE_SIZE, pSizeItem);

    bool bIsLandscape = (pSizeItem->GetSize().Width() >= pSizeItem->GetSize().Height());

    return bIsLandscape ? "\"landscape\"" : "\"portrait\"";
}

}
