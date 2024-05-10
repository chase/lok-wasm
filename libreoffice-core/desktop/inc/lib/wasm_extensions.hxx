#pragma once

#include <LibreOfficeKit/LibreOfficeKit.h>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>

#include <cstdint>
#include <desktop/dllapi.h>
#include <vector>

// This makes direct extensions to lib/init.cxx much easier to expose in main_wasm.cxx

namespace desktop
{

enum class RenderState : int32_t
{
    IDLE = 0,
    TILE_PAINT = 1,
    RENDERING = 2,
    RESET = 3,
    QUIT = 4
};

static constexpr int32_t MAX_INVALIDATION_STACK = 4096;

// Used for fast communication between the tile renderer worker and the C++ thread
struct TileRendererData
{
    // initialized params
    const int32_t viewId;
    const int32_t tileSize;
    const int32_t paintedTileAllocSize;

    // individual tile paint
    _Atomic RenderState state = RenderState::IDLE;
    uint32_t tileTwips[4];
    uint8_t* paintedTile;

    // tile invalidations handling using a fixed-size stack that is thread-safe
    _Atomic uint32_t pendingFullPaint = 1; // this is a bool
    _Atomic uint32_t hasInvalidations = 1; // this is a bool
    _Atomic uint32_t invalidationStack[MAX_INVALIDATION_STACK][4];
    _Atomic int32_t invalidationStackHead = -1;

    // changes somewhat frequently
    _Atomic uint32_t docHeightTwips;
    // changes infrequently
    _Atomic uint32_t docWidthTwips;

    LibreOfficeKitDocument* doc;

    TileRendererData(LibreOfficeKitDocument* doc_, int32_t viewId_, int32_t tileSize_,
                     uint32_t docWidthTwips_, uint32_t docHeightTwips_)
        : viewId(viewId_)
        , tileSize(tileSize_)
        , paintedTileAllocSize(tileSize_ * tileSize_ * 4)
        , paintedTile(new uint8_t[paintedTileAllocSize])
        , docHeightTwips(docHeightTwips_)
        , docWidthTwips(docWidthTwips_)
        , doc(doc_){};

    /** x, y, w, h */
    void pushInvalidation(uint32_t invalidation[4]);
    void reset();
};

struct DESKTOP_DLLPUBLIC WasmDocumentExtension : public _LibreOfficeKitDocument
{
    std::vector<pthread_t> tileRendererThreads_;
    std::vector<TileRendererData> tileRendererData_;

    TileRendererData& startTileRenderer(int32_t viewId, int32_t tileSize);

    std::string getPageColor();
    std::string getPageOrientation();
};
}
