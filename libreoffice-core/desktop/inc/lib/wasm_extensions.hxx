#pragma once

#include <LibreOfficeKit/LibreOfficeKit.h>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>

#include <emscripten/val.h>
#include <cstdint>
#include <desktop/dllapi.h>
#include <memory>
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

struct AdditionalView
{
    const int32_t viewId;
    const int32_t tileSize;
    const int32_t paintedTileAllocSize;
    _Atomic uint32_t pendingFullPaint = 1;

    uint32_t tileTwips[4];
    uint8_t* paintedTile;

    AdditionalView(int32_t viewId_, int32_t tileSize_)
        : viewId(viewId_)
        , tileSize(tileSize_)
        , paintedTileAllocSize(tileSize_ * tileSize_ * 4)
        , paintedTile(new uint8_t[paintedTileAllocSize])
    {
    }
};

// Used for fast communication between the tile renderer worker and the C++ thread
struct TileRendererData
{
    const int32_t viewId;
    const int32_t tileSize;
    const int32_t paintedTileAllocSize;

    // individual tile paint
    _Atomic RenderState state = RenderState::IDLE;

    // tile invalidations handling using a fixed-size stack that is thread-safe
    _Atomic uint32_t hasInvalidations = 1; // this is a bool
    _Atomic uint32_t invalidationStack[MAX_INVALIDATION_STACK][4];
    _Atomic int32_t invalidationStackHead = -1;
    _Atomic uint32_t pendingFullPaint = 1; // this is a bool

    _Atomic uint32_t docWidthTwips;
    _Atomic uint32_t docHeightTwips;
    _Atomic int32_t activeViewId;

    uint32_t tileTwips[4];
    uint8_t* paintedTile;

    std::shared_ptr<AdditionalView> previewView;

    LibreOfficeKitDocument* doc;

    TileRendererData(LibreOfficeKitDocument* doc_, int32_t viewId_, int32_t tileSize_,
                     uint32_t docWidthTwips_, uint32_t docHeightTwips_)
        : viewId(viewId_)
        , tileSize(tileSize_)
        , paintedTileAllocSize(tileSize_ * tileSize_ * 4)
        , docWidthTwips(docWidthTwips_)
        , docHeightTwips(docHeightTwips_)
        , activeViewId(viewId_)
        , paintedTile(new uint8_t[paintedTileAllocSize])
        , doc(doc_){};

    /** x, y, w, h */
    void pushInvalidation(uint32_t invalidation[4]);
    void reset();

    TileRendererData(TileRendererData&&) = default;

    TileRendererData(const TileRendererData&) = delete;
    TileRendererData& operator=(const TileRendererData&) = delete;
};

struct DESKTOP_DLLPUBLIC WasmDocumentExtension : public _LibreOfficeKitDocument
{
    std::vector<pthread_t> tileRendererThreads_;
    std::vector<TileRendererData> tileRendererData_;
    css::uno::Reference<css::lang::XComponent> mxComponent;

    WasmDocumentExtension(css::uno::Reference<css::lang::XComponent> xComponent);
    TileRendererData& startTileRenderer(int32_t viewId, int32_t tileSize);

    AdditionalView& addPreviewView(int32_t mainViewId, int32_t viewId, int32_t tileSize);

    std::string getPageColor();
    std::string getPageOrientation();
};
}
