#include "com/sun/star/beans/XPropertySet.hdl"
#include "com/sun/star/document/XDocumentProperties.hdl"
#include "com/sun/star/embed/ElementModes.hpp"
#include "com/sun/star/frame/XModel3.hdl"
#include "com/sun/star/lang/XComponent.hdl"
#include "com/sun/star/uno/Reference.h"
#include "com/sun/star/uno/Sequence.h"
#include "com/sun/star/uno/XComponentContext.hdl"
#include "com/sun/star/util/XCloneable.hdl"
#include "comphelper/hash.hxx"
#include "comphelper/propertysequence.hxx"
#include "comphelper/sequence.hxx"
#include "comphelper/storagehelper.hxx"
#include "editeng/sizeitem.hxx"
#include "sal/log.hxx"
#include "sfx2/bindings.hxx"
#include "sfx2/dispatch.hxx"
#include "sfx2/docfile.hxx"
#include "sfx2/sfxbasemodel.hxx"
#include "sfx2/shell.hxx"
#include "sfx2/viewfrm.hxx"
#include "sfx2/viewsh.hxx"
#include "svl/itemset.hxx"
#include "svl/poolitem.hxx"
#include "svx/rulritem.hxx"
#include "svx/xcolit.hxx"
#include "svx/xflclit.hxx"
#include "tools/json_writer.hxx"
#include <algorithm>
#include <cstdlib>
#include <lib/wasm_extensions.hxx>
#include <emscripten/bind.h>
#include <emscripten/threading.h>
#include <optional>
#include <pthread.h>
#include <rtl/ustring.hxx>
#include <rtl/string.hxx>
#include <svx/svxids.hrc>
#include <string>
#include <iomanip>
#include <sstream>

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

using namespace css::uno;
using namespace com::sun::star::embed;
using namespace com::sun::star::io;

std::string hashToString(const std::vector<unsigned char>& hashVec) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char byte : hashVec) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

std::vector<ExpandedPart> GetExpandedParts(Reference<XStorage> storage, std::optional<OUString> &path)
{
    std::vector<ExpandedPart> expandedParts;
    Sequence<OUString> elementNames = storage->getElementNames();
    for (const OUString& name : elementNames)
    {
        OUString fullPath = path.value_or("") + "/" + name;

        // Indicates that the element is a directory
        if (storage->isStorageElement(name))
        {
            Reference<XStorage> subStorage = storage->openStorageElement(name, ElementModes::READ);
            std::optional<OUString> subPath = std::make_optional<OUString>(fullPath);
            // Recursively initialize the expanded parts
            std::vector<ExpandedPart> subParts = GetExpandedParts(subStorage, subPath);

            for (ExpandedPart part : subParts) {
                expandedParts.push_back(part);
            }
        }
        // Indicates that the element is a streamable file
        else if (storage->isStreamElement(name))
        {
            // Double check that the path exists
            // before trying to open the stream element
            // trying to open a non-existent path will hang indefinately
            if (!storage->hasByName(name))
            {
                SAL_WARN("desktop", "ExpandedStorage::ExpandedStorage() - path: " << fullPath << " does not exist");
                continue;
            }

            Reference<XStream> xStream = storage->openStreamElement(name, ElementModes::READ);

            if (!xStream.is())
            {
                SAL_WARN("desktop", "ExpandedStorage::ExpandedStorage() - xStream for path " << name << " is null");
                continue;
            }

            Reference<XInputStream> inputStream = xStream->getInputStream();

            if (!inputStream.is())
            {
                SAL_WARN("desktop", "ExpandedStorage::ExpandedStorage() - inputStream for path " << fullPath << " is null");
                continue;
            }

            // Read the file content
            Sequence<sal_Int8> aFileContent;
            inputStream->readBytes(aFileContent, inputStream->available());
            const sal_Int8* pData = aFileContent.getConstArray();
            const unsigned char* pUnsignedData = reinterpret_cast<const unsigned char*>(pData);


            // Generate SHA-256 hash
            std::vector<unsigned char> hashVec = comphelper::Hash::calculateHash(
                pUnsignedData, aFileContent.getLength(),
                comphelper::HashType::SHA256
            );

            SAL_WARN("desktop", "ExpandedStorage::ExpandedStorage() - path: " << fullPath << " hash: " << hashToString(hashVec));
            expandedParts.push_back({OUStringToString(fullPath), hashToString(hashVec)});
        }
    }

    return expandedParts;
}

void ExpandedStorage::initializeExpandedParts()
{
    SfxViewFrame* frame = SfxViewFrame::Current();
    SfxMedium* medium = frame->GetObjectShell()->GetMedium();
    css::uno::Reference<com::sun::star::embed::XStorage> storage = medium->GetZipStorageToSign_Impl();
    auto path = std::optional<OUString>();

    std::vector<ExpandedPart> expandedPartsVec  = GetExpandedParts(storage, path);

    for (const ExpandedPart& part : expandedPartsVec)
    {
        expandedParts.insert({part.path, part});
    }
}

ExpandedStorage::ExpandedStorage()
{
    SAL_WARN("desktop", "ExpandedStorage::ExpandedStorage()");
    initializeExpandedParts();
}

std::vector<ExpandedPart> ExpandedStorage::saveToExpandedStorage(void* context, void* component)
{

    SfxViewFrame* frame = SfxViewFrame::Current();
    SfxMedium* medium = frame->GetObjectShell()->GetMedium();

/*     uno::Sequence<beans::PropertyValue> origMediumArgs = medium->GetArgs(); */

/*     Reference< XComponentContext > xComponentContext = static_cast<XComponentContext*>(context); */
/*     Reference< lang::XComponent > xComponent = static_cast<lang::XComponent*>(component); */
/*     uno::Reference< embed::XStorage > xStorage = comphelper::OStorageHelper::GetTemporaryStorage( */
/*         xComponentContext */
/*     ); */

/*     Reference< embed::XStorage> oldStorage = medium->GetZipStorageToSign_Impl(); */
/*     Reference< util::XCloneable > xStorageCloneable (oldStorage, UNO_QUERY_THROW); */
/*     Reference< embed::XStorage> newStorage(xStorageCloneable->createClone(), UNO_QUERY_THROW); */

/*     SfxBaseModel* pBaseModel = dynamic_cast<SfxBaseModel*>(xComponent.get()); */
/*     // We don't need to actually commit the changes to disk, since we are */
/*     // only using the in memory representation of the storage */
/*     uno::Sequence<beans::PropertyValue> descriptor( comphelper::InitPropertySequence({ */
/*         { "ShouldCommitToStorage", Any(false) }, */
/*     })); */

/*     uno::Sequence<beans::PropertyValue> finalArgs = comphelper::concatSequences(origMediumArgs, descriptor); */

/*     pBaseModel->storeToStorage(newStorage, finalArgs); */

    frame->GetObjectShell()->DoSaveAs(*medium);

    auto path = std::optional<OUString>();

    std::vector<ExpandedPart> expandedPartsVec = GetExpandedParts(medium->GetZipStorageToSign_Impl(), path);
    SAL_WARN("desktop", "expandedPartsVec" << expandedPartsVec.size());

    for (const ExpandedPart& part : expandedPartsVec)
    {

        if (expandedParts.find(part.path) != expandedParts.end())
        {
            if (expandedParts[part.path].sha != part.sha)
            {
                SAL_WARN("desktop", "ExpandedStorage::saveToExpandedPart() - path: " << part.path << " sha: " << part.sha << " is different from the existing sha: " << expandedParts[part.path].sha);
                expandedParts[part.path] = part;
            }
        } else {
            expandedParts.insert({part.path, part});
            SAL_WARN("desktop", "ExpandedStorage::saveToExpandedPart() - path: " << part.path << " sha: " << part.sha << " is new " << expandedParts[part.path].sha);
        }
    }

    return expandedPartsVec;
}

}
