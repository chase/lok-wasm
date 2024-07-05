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
#include <oox/helper/expandedstorage.hxx>
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
using cppu::getCaughtException;

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

WasmDocumentExtension::WasmDocumentExtension(css::uno::Reference<css::lang::XComponent> xComponent) : mxComponent(std::move(xComponent)) {

}

TileRendererData& WasmDocumentExtension::startTileRenderer(int32_t viewId_, int32_t tileSize_)
{
    long w, h;
    pClass->getDocumentSize(this, &w, &h);
    pthread_t& threadId = tileRendererThreads_.emplace_back();
    TileRendererData& data = tileRendererData_.emplace_back(this, viewId_, tileSize_, w, h, threadId);
    if (pthread_create(&threadId, nullptr, tileRendererWorker, &data))
    {
        std::abort();
    }
    pthread_detach(threadId);

    return data;
}

void WasmDocumentExtension::stopTileRenderer(int32_t viewId)
{
    auto it = std::find_if(tileRendererData_.begin(), tileRendererData_.end(),
        [viewId](const TileRendererData& data) {
            return data.viewId == viewId;
        });

    if (it != tileRendererData_.end())
    {
        TileRendererData* data = &*it;
        changeState(data, RenderState::QUIT);

        auto threadIt = std::find_if(tileRendererThreads_.begin(), tileRendererThreads_.end(),
            [data](const pthread_t& threadId) {
                return pthread_equal(threadId, data->threadId);
            });

        if (threadIt != tileRendererThreads_.end())
        {
            pthread_join(*threadIt, nullptr);
            tileRendererThreads_.erase(threadIt);
        }
        delete[] data->paintedTile;
        delete data;
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

    static constexpr std::string_view defaultColorHex = "#ffffff";


    SfxPoolItemHolder pState;
    const SfxItemState eState (pDispatch->QueryState(SID_ATTR_PAGE_COLOR, pState));
    if (eState < SfxItemState::DEFAULT) {
        return std::string (defaultColorHex);
    }
    if (pState.getItem())
    {
        XColorItem* pColor = static_cast<XColorItem*>(pState.getItem()->Clone());
        OUString aColorHex = pColor->GetColorValue().AsRGBHEXString();
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

    return bIsLandscape ? "landscape" : "portrait";
}

_LibreOfficeKitDocument* WasmOfficeExtension::documentExpandedLoad(desktop::ExpandedDocument expandedDoc, std::string name, const int documentId, const bool readOnly)
{
    LibreOfficeKitDocument* pDoc = NULL;
    desktop::WasmDocumentExtension* ext
        = static_cast<desktop::WasmDocumentExtension*>(pDoc);

    LibreOfficeKit* pThis = static_cast<LibreOfficeKit*>(this);


    return ext->loadFromExpanded(pThis, expandedDoc, documentId, readOnly);
}


void ExpandedDocument::addPart(std::string path, std::string content)
{
    parts.emplace_back(std::move(path), std::move(content));
}

_LibreOfficeKitDocument* WasmDocumentExtension::loadFromExpanded(LibreOfficeKit* pThis, desktop::ExpandedDocument expandedDoc, const int documentId, const bool readOnly)
{
    using namespace com::sun::star;
    uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();

    if (!xContext) {
        return nullptr;
    }

    uno::Reference<frame::XDesktop2> xComponentLoader = frame::Desktop::create(xContext);

    if (!xComponentLoader.is())
    {
        SAL_WARN("lok", "ComponentLoader is not available");
        return nullptr;
    }


    // Parts of the import pipeline expect a stream
    // this stream isn't actually used, but is required to be passed along
    std::vector<sal_Int8> aEmptyData;
    uno::Reference<io::XInputStream> xEmptyInputStream(new comphelper::VectorInputStream(aEmptyData));

    uno::Reference<oox::ExpandedStorage> storage(new oox::ExpandedStorage(xContext, xEmptyInputStream));

    auto it = expandedDoc.parts.begin();
    while(it != expandedDoc.parts.end())
    {
        if (it->content.length() == 0 || it->path.length() == 0)
            continue;
        storage->addPart(it->path, it->content);
        it = expandedDoc.parts.erase(it);
    }

    storage->readRelationshipInfo();
    // Is this necesarry ?
    storage->setPropertyValue("OpenMode", uno::Any(ucb::OpenMode::ALL));
    storage->setPropertyValue("Version", uno::Any(OUString("1")));
    storage->setPropertyValue("MS Word 2007 XML", uno::Any(OUString("1")));

    uno::Reference<embed::XStorage> xStorage(storage, uno::UNO_QUERY);
    auto storageBase = std::shared_ptr<oox::StorageBase>(storage.get());

    // ExpandedStorage can represent both a BaseStorage and an XStorage
    //
    // Unlike conventional XStorage we don't want to be constantly re-initializing
    // a storage object since the file content is stored in memory.
    // Thus we set a storageBase and xStorage globals to be used
    // throughout the load process.
    comphelper::OStorageHelper::SetIsExpandedStorage(true);
    comphelper::OStorageHelper::SetExpandedStorage(xStorage);
    comphelper::OStorageHelper::SetExpandedStorageBase(storageBase);
    expandedStorage = uno::Reference<oox::ExpandedStorage>(storage.get());

    utl::MediaDescriptor aMediaDescriptor;

    // Expanded Storage only supports .DOCX
    aMediaDescriptor[utl::MediaDescriptor::PROP_FILTERNAME] <<= OUString("MS Word 2007 XML"); // just hardcode this for now
    aMediaDescriptor[utl::MediaDescriptor::PROP_MACROEXECUTIONMODE] <<= document::MacroExecMode::NEVER_EXECUTE;
    // We don't have a general document input stream,
    // so we pass in an empty one. Down the line its crucial we
    // check if we are currently loading from expanded storage
    // and use the storage instead of the stream
    aMediaDescriptor[utl::MediaDescriptor::PROP_INPUTSTREAM] <<= xEmptyInputStream;
    // Silences various exceptions
    aMediaDescriptor[utl::MediaDescriptor::PROP_SILENT] <<= true;

    if (readOnly)
    {
        aMediaDescriptor[utl::MediaDescriptor::PROP_READONLY] <<= true;
        // disable comments which are still enabled with read only:
        aMediaDescriptor[utl::MediaDescriptor::PROP_VIEWONLY] <<= true;
    }

    {
        SolarMutexGuard aGuard;
        try
        {
            Application::SetDialogCancelMode(DialogCancelMode::LOKSilent);
            SfxViewShell::SetCurrentDocId(ViewShellDocId(documentId));
            uno::Reference<lang::XComponent> xComponent = xComponentLoader->loadComponentFromURL(
                "private:stream", "_blank", documentId, aMediaDescriptor.getAsConstPropertyValueList());

            if (!xComponent.is()) {
                SAL_WARN("lok", "Could not load in memory doc");
                return nullptr;
            }

            return new LibLODocument_Impl(xComponent, documentId);
        }
        catch (const uno::Exception& exception)
        {
            uno::Any exAny(getCaughtException());
            SAL_WARN("lok", "Failed to load to in-memory stream: " + exceptionToString(exAny));
        }
    }
    SAL_WARN("lok", "Failed to load to in-memory stream");
    return nullptr;
}

std::optional<std::pair<std::string, std::vector<sal_Int8>>> WasmDocumentExtension::getExpandedPart(const std::string& path) const
{
    return expandedStorage->getPart(path);
}
void WasmDocumentExtension::removePart(const std::string& path) const
{
    return expandedStorage->removePart(path);
}

std::vector<std::pair<const std::string, const std::string>> WasmDocumentExtension::listParts() const
{
    return expandedStorage->listParts();
}
}
