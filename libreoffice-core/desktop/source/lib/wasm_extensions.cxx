#include <com/sun/star/uno/Reference.hxx>
#include <editeng/sizeitem.hxx>
#include <memory>
#include <optional>
#include <sfx2/bindings.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/viewsh.hxx>
#include <svl/itemset.hxx>
#include <svl/poolitem.hxx>
#include <svx/rulritem.hxx>
#include <svx/xcolit.hxx>
#include <svx/xflclit.hxx>
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

        bool bIsMainView = d->viewId == d->activeViewId;
        auto tileTwips
            = bIsMainView ? d->tileTwips : d->previewView.get()->tileTwips;
        uint8_t* paintedTile = bIsMainView ? d->paintedTile : d->previewView.get()->paintedTile;
        int32_t tileSize = bIsMainView ? d->tileSize : d->previewView.get()->tileSize;
        int32_t paintedTileAllocSize
            = bIsMainView ? d->paintedTileAllocSize : d->previewView.get()->paintedTileAllocSize;

        switch (state)
        {
            case RenderState::IDLE:
                waitWhileInState(d, RenderState::IDLE); // owned by tile_renderer_worker.ts
                break;
            case RenderState::TILE_PAINT:
            {
                int nOrigViewId = d->doc->pClass->getView(d->doc);
                if (nOrigViewId != d->activeViewId)
                {
                    d->doc->pClass->setView(d->doc, d->activeViewId);
                }

                std::fill_n(paintedTile, paintedTileAllocSize, 0);
                d->doc->pClass->paintTile(d->doc, paintedTile, tileSize, tileSize, tileTwips[0],
                                          tileTwips[1], tileTwips[2], tileTwips[3]);
                if (nOrigViewId >= 0 && nOrigViewId != d->activeViewId)
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

WasmDocumentExtension::WasmDocumentExtension(css::uno::Reference<css::lang::XComponent> xComponent)
    : mxComponent(std::move(xComponent))
{
}

TileRendererData& WasmDocumentExtension::startTileRenderer(int32_t viewId_, int32_t tileSize_)
{
    long w, h;
    pClass->getDocumentSize(this, &w, &h);

    TileRendererData& data = tileRendererData_.emplace_back(this, viewId_, tileSize_, w, h);

    pthread_t& threadId = tileRendererThreads_.emplace_back();
    if (pthread_create(&threadId, nullptr, tileRendererWorker, &data))
    {
        std::abort();
    }
    pthread_detach(threadId);

    return data;
}

AdditionalView& WasmDocumentExtension::addPreviewView(int32_t mainViewId, int32_t viewId,
                                                      int32_t tileSize)
{
    std::shared_ptr<AdditionalView> previewView
        = std::make_shared<AdditionalView>(viewId, tileSize);

    for (auto& data : tileRendererData_)
        if (data.viewId == mainViewId)
            data.previewView = previewView;

    return *previewView;
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
    bool bIsMainView = activeViewId == viewId;
    __c11_atomic_store(&invalidationStackHead, -1, __ATOMIC_RELAXED);

    __c11_atomic_store(bIsMainView ? &pendingFullPaint : &previewView->pendingFullPaint, 1,
                       __ATOMIC_SEQ_CST);
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

    static constexpr std::string_view defaultColorHex = "\"#ffffff\"";

    SfxPoolItemHolder pState;
    const SfxItemState eState(pDispatch->QueryState(SID_ATTR_PAGE_COLOR, pState));
    if (eState < SfxItemState::DEFAULT)
    {
        return std::string(defaultColorHex);
    }
    if (pState.getItem())
    {
        XColorItem* pColor = static_cast<XColorItem*>(pState.getItem()->Clone());
        OUString aColorHex = u"\"" + pColor->GetColorValue().AsRGBHEXString() + u"\"";
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

    return bIsLandscape ? "\"landscape\"" : "\"portrait\"";
}

}
