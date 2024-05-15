#pragma once

#include "com/sun/star/uno/Reference.h"
#include <LibreOfficeKit/LibreOfficeKit.h>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>

#include <cstdint>
#include <desktop/dllapi.h>
#include <unordered_map>
#include <vector>
#include <com/sun/star/lang/XComponent.hpp>

// This makes direct extensions to lib/init.cxx much easier to expose in main_wasm.cxx

namespace desktop
{

enum class RenderState : int32_t
{
    IDLE = 0,
    TILE_PAINT = 1,
    RENDERING = 2,
    RESET = 3,
    QUIT = 4,
};

static constexpr int32_t MAX_INVALIDATION_STACK = 4096;

struct ViewData
{
    const int32_t viewId;
    const int32_t tileSize;
    const int32_t paintedTileAllocSize;

    uint32_t tileTwips[4];
    uint8_t* paintedTile;

    ViewData(int32_t viewId_, int32_t tileSize_)
        : viewId(viewId_)
        , tileSize(tileSize_)
        , paintedTileAllocSize(tileSize_ * tileSize_ * 4)
        , paintedTile(new uint8_t[paintedTileAllocSize])
    {}
};

// Used for fast communication between the tile renderer worker and the C++ thread
struct TileRendererData
{
    ViewData* mainViewData;
    ViewData* previewViewData;

    // individual tile paint
    _Atomic RenderState state = RenderState::IDLE;

    // tile invalidations handling using a fixed-size stack that is thread-safe
    _Atomic uint32_t hasInvalidations = 1; // this is a bool
    _Atomic uint32_t invalidationStack[MAX_INVALIDATION_STACK][4];
    _Atomic int32_t invalidationStackHead = -1;
    _Atomic uint32_t pendingFullPaint = 1; // this is a bool

    _Atomic int32_t docWidthTwips;
    _Atomic int32_t docHeightTwips;
    _Atomic int32_t activeViewId;

    LibreOfficeKitDocument* doc;

    TileRendererData(LibreOfficeKitDocument* doc_, ViewData* mainViewData_, ViewData* previewViewData_, int32_t docWidthTwips_, int32_t docHeightTwips_)
        : mainViewData(mainViewData_)
        , previewViewData(previewViewData_)
        , docWidthTwips(docWidthTwips_)
        , docHeightTwips(docHeightTwips_)
        , activeViewId(mainViewData_->viewId)
        , doc(doc_){};

    /** x, y, w, h */
    void pushInvalidation(uint32_t invalidation[4]);
    void reset();
};

struct DESKTOP_DLLPUBLIC WasmDocumentExtension : public _LibreOfficeKitDocument
{
    std::vector<pthread_t> tileRendererThreads_;
    std::vector<TileRendererData> tileRendererData_;
    css::uno::Reference<css::lang::XComponent> mxComponent;

    WasmDocumentExtension(css::uno::Reference<css::lang::XComponent> xComponent);
    TileRendererData& startTileRenderer(
        int32_t viewId,
        int32_t tileSize,
        std::optional<int32_t> secondaryViewId,
        std::optional<int32_t> secondaryTileSize
    );

    std::string getPageColor();
    std::string getPageOrientation();
};
}
