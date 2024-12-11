#pragma once

#include <oox/helper/expandedstorage.hxx>
#include <sal/types.h>
#include <LibreOfficeKit/LibreOfficeKit.h>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>

#include <cstdint>
#include <desktop/dllapi.h>
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
};

static constexpr int32_t MAX_INVALIDATION_STACK = 4096;
static constexpr int32_t MAX_PAINTED_TILES_PER_ITER = 32; // ~8 MiB

// Used for fast communication between the tile renderer worker and the C++ thread
struct TileRendererData
{
    // initialized params
    const int32_t viewId;
    const int32_t tileSize;
    const int32_t tileAllocSize;
    const int32_t scratchTilesAllocSize;

    // individual tile paint
    std::unique_ptr<uint8_t[]> paintedTiles;

    // tile invalidations handling using a fixed-size stack that is thread-safe
    _Atomic uint32_t pendingFullPaint = 1; // this is a bool
    _Atomic uint32_t hasInvalidations = 1; // this is a bool
    _Atomic uint32_t invalidationStack[MAX_INVALIDATION_STACK][4];
    _Atomic int32_t invalidationStackHead = -1;

    // toggled in tile_render_worker.ts, used for any input callback
    _Atomic int32_t isVisibleAreaPainted = 0;
    _Atomic uint32_t tileDimTwips;

    // changes somewhat frequently
    _Atomic uint32_t docHeightTwips;
    // changes infrequently
    _Atomic uint32_t widthTileStride;
    _Atomic uint32_t docWidthTwips;
    // changes infrequently
    _Atomic uint32_t scrollHeightTwips;

    LibreOfficeKitDocument* doc;

    TileRendererData(LibreOfficeKitDocument* doc_, int32_t viewId_, int32_t tileSize_,
                     uint32_t docWidthTwips_, uint32_t docHeightTwips_)
        : viewId(viewId_)
        , tileSize(tileSize_)
        , tileAllocSize(tileSize_ * tileSize_ * 4)
        , scratchTilesAllocSize(tileAllocSize * MAX_PAINTED_TILES_PER_ITER)
        , paintedTiles(std::make_unique<uint8_t[]>(scratchTilesAllocSize))
        , docHeightTwips(docHeightTwips_)
        , docWidthTwips(docWidthTwips_)
        , doc(doc_)
    {
    }

    /** x, y, w, h */
    void pushInvalidation(uint32_t invalidation[4]);
    void reset();
};

// TODO: this is sloppy and not memory safe, but necessary for the any input callback for now
// ideally, any input callback wouldn't require globals but check each client, but this is the
// the next closest solution
static TileRendererData* g_activeTileRenderData = nullptr;

struct ExpandedPart
{
    std::string path;
    std::string content;

    ExpandedPart(std::string path_, std::string content_)
        : path(std::move(path_))
        , content(std::move(content_)) {};
};

struct ExpandedDocument
{
    std::vector<ExpandedPart> parts;

    ExpandedDocument() {};

public:
    void addPart(std::string path, std::string content);
};

struct DESKTOP_DLLPUBLIC WasmDocumentExtension : public _LibreOfficeKitDocument
{
    // technically, we could support multiple views, but realistically we never do
    std::optional<TileRendererData> tileRendererData_;
    css::uno::Reference<css::lang::XComponent> mxComponent;

    WasmDocumentExtension(css::uno::Reference<css::lang::XComponent> xComponent);
    void paintTiles(uint32_t startIndex, uint32_t endIndex);
    TileRendererData& startTileRenderer(int32_t viewId, int32_t tileSize);
    void stopTileRenderer(int32_t viewId);
    bool hasInvalidations();

    std::string getPageColor();
    std::string getPageOrientation();

    _LibreOfficeKitDocument* loadFromExpanded(LibreOfficeKit* pThis,
                                              desktop::ExpandedDocument expandedDoc,
                                              const int documentId = 0,
                                              const bool readOnly = false);

    std::optional<std::pair<std::string, std::shared_ptr<std::vector<sal_Int8>>>>
    getExpandedPart(const std::string& path) const;
    void removePart(const std::string& path) const;
    std::vector<std::pair<const std::string, const std::string>> listParts() const;
    std::vector<std::pair<std::string, std::string>> save();
    std::optional<std::string> getCursor(int viewId);
};

struct DESKTOP_DLLPUBLIC WasmOfficeExtension : public _LibreOfficeKit
{
    _LibreOfficeKitDocument* documentExpandedLoad(desktop::ExpandedDocument expandedDoc,
                                                  std::string name, const int documentId = 0,
                                                  const bool readOnly = false);
};

}
