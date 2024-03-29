#include <emscripten/em_asm.h>
#include <unordered_set>
#define LOK_USE_UNSTABLE_API
#include <LibreOfficeKit/LibreOfficeKitEnums.h>
#include <LibreOfficeKit/LibreOfficeKit.hxx>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/promise.h>
#include <array>
#include <cstdint>
#include <lib/wasm_extensions.hxx>
#include <sal/log.hxx>

using namespace emscripten;

//static
lok::Office* instance()
{
    static lok::Office* instance_ = nullptr;
    if (!instance_)
    {
        instance_ = lok::lok_cpp_init(/* use default path */ nullptr);
        instance_->setOptionalFeatures(
            LOK_FEATURE_PART_IN_INVALIDATION_CALLBACK | LOK_FEATURE_NO_TILED_ANNOTATIONS
            | LOK_FEATURE_RANGE_HEADERS | LOK_FEATURE_VIEWID_IN_VISCURSOR_INVALIDATION_CALLBACK);
    }
    return instance_;
}

static constexpr std::string_view TEXT_PLAIN = "text/plain";

uint32_t document_id_counter = 0;

// actually more unsafe than safe regarding memory safety, but it's safe to pass between workers
using SafeString = std::pair<size_t, char*>;
SafeString makeSafeString(const char* src)
{
    auto len = std::strlen(src) + 1; // +1 for the null terminator
    char* dst = new char[len];
    std::memcpy(dst, src, len);
    return std::make_pair((size_t)dst, dst);
}

void freeSafeString(size_t id) { delete[] (char*)id; }

class DocumentClient
{
public:
    explicit DocumentClient(std::string path)
        : ref_(++document_id_counter)
        , doc_(instance()->documentLoad(path.c_str()))
    {
    }
    bool valid() { return doc_ != nullptr; }

    bool saveAs(std::string url, std::optional<std::string> format,
                std::optional<std::string> filterOptions)
    {
        return doc_->saveAs(url.c_str(), format.has_value() ? format->c_str() : nullptr,
                            filterOptions.has_value() ? filterOptions->c_str() : nullptr);
    }

    int getParts() { return doc_->getParts(); }
    val getPartRectangles() { return val::u8string(doc_->getPartPageRectangles()); }
    val paintTile(int tileWidth, int tileHeight, int xTwips, int yTwips, int widthTwips,
                  int heightTwips)
    {
        doc_->paintTile(tile_scratch_.data(), tileWidth, tileHeight, xTwips, yTwips, widthTwips,
                        heightTwips);
        return val(
            typed_memory_view(tileWidth * tileHeight * BYTES_PER_PIXEL, tile_scratch_.data()));
    }

    val getDocumentSize()
    {
        long size[2];
        val result = val::array();
        doc_->getDocumentSize(&size[0], &size[1]);
        result.call<void>("push", val(size[0]));
        result.call<void>("push", val(size[1]));
        return result;
    }

    void initializeForRendering(std::string args)
    {
        rendering_tiles_ = true;
        doc_->initializeForRendering(args.c_str());
        registerCallbackOnce(doc_->getView());
    }

    void postKeyEvent(int viewId, int type, int charCode, int keyCode)
    {
        doc_->setView(viewId);
        doc_->postKeyEvent(type, charCode, keyCode);
    }

    void postTextInputEvent(int viewId, unsigned windowId, std::string text)
    {
        doc_->setView(viewId);
        doc_->postWindowExtTextInputEvent(windowId, LOK_EXT_TEXTINPUT, text.c_str());
        doc_->postWindowExtTextInputEvent(windowId, LOK_EXT_TEXTINPUT_END, text.c_str());
    }

    void postMouseEvent(int viewId, int type, int x, int y, int count, int buttons, int modifier)
    {
        doc_->setView(viewId);
        doc_->postMouseEvent(type, x, y, count, buttons, modifier);
    }

    void postUnoCommand(int viewId, std::string command, std::optional<std::string> args,
                        bool notifyWhenFinished)
    {
        doc_->setView(viewId);
        doc_->postUnoCommand(command.c_str(), args.has_value() ? args->c_str() : nullptr,
                             notifyWhenFinished);
    }

    void setTextSelection(int viewId, int type, int x, int y)
    {
        doc_->setView(viewId);
        doc_->setTextSelection(type, x, y);
    }

    bool setClipboard(int viewId, val data)
    {
        if (!data.isArray())
            return false;
        const uint32_t entries = data["length"].as<uint32_t>();
        if (entries == 0)
            return false;

        std::vector<const char*> mime_c_str;
        std::vector<size_t> in_sizes;
        in_sizes.reserve(entries);
        std::vector<const char*> streams;
        streams.reserve(entries);

        for (size_t i = 0; i < entries; ++i)
        {
            if (!data[i]["mimeType"].isString())
                continue;

            val dictionary = val::object();

            std::string mime_type = data[i]["mimeType"].as<std::string>();
            std::string buffer = data[i]["buffer"].as<std::string>();

            in_sizes[i] = buffer.size();
            mime_c_str.push_back(mime_type.c_str());
            streams[i] = buffer.data();
        }

        // add the nullptr terminator to the list of null-terminated strings
        mime_c_str.push_back(nullptr);

        doc_->setView(viewId);
        return doc_->setClipboard(entries, mime_c_str.data(), in_sizes.data(), streams.data());
    }

    val getClipboard(int viewId, val mimeTypes)
    {
        std::vector<const char*> mime_c_str;
        if (mimeTypes.isArray())
        {
            const uint32_t len = mimeTypes["length"].as<uint32_t>();
            mime_c_str.reserve(len + 1); // +1 is for the list terminator
            for (uint32_t i = 0; i < len; ++i)
            {
                std::string mime = mimeTypes[i].as<std::string>();
                // LOK explicitly converts all UTF-16 strings to UTF-8, however it still
                // requests an encoding
                if (mime == TEXT_PLAIN)
                {
                    mime_c_str.push_back("text/plain;charset=utf-8");
                    continue;
                }
                else
                {
                    mime_c_str.push_back(mime.c_str());
                }
            }
            // add the nullptr terminator to the list of null-terminated strings
            mime_c_str.push_back(nullptr);
        }

        size_t out_count;
        // these are arrays of out_count size, variable size arrays in C are
        // simply pointers to the first element
        char** out_mime_types = nullptr;
        size_t* out_sizes = nullptr;
        char** out_streams = nullptr;

        doc_->setView(viewId);
        bool success = doc_->getClipboard(mime_c_str.empty() ? nullptr : mime_c_str.data(),
                                          &out_count, &out_mime_types, &out_sizes, &out_streams);
        val result = val::array();
        if (!success)
            return result;

        for (size_t i = 0; i < out_count; ++i)
        {
            size_t buffer_size = out_sizes[i];
            if (buffer_size <= 0)
            {
                result.set(i, val::undefined());
                continue;
            }
            static constexpr std::string_view text_prefix = "text/";
            std::string_view sv_mime_type(out_mime_types[i]);
            val obj = val::object();
            if (sv_mime_type.substr(0, text_prefix.length()) == text_prefix)
            {
                // LOK likes to add character encodings and other odd bits, but browsers expect `text/plain`
                obj.set("mimeType",
                        val::u8string(sv_mime_type.substr(0, TEXT_PLAIN.length()) == TEXT_PLAIN
                                          ? TEXT_PLAIN.data()
                                          : out_mime_types[i]));
                obj.set("text", val::u8string(out_streams[i]));
            }
            else
            {
                obj.set("mimeType", out_mime_types[i]);
                obj.set("data", typed_memory_view(out_sizes[i], out_streams[i]));
            }
            result.call<void>("push", obj);

            // free the clipboard item, can't use std::unique_ptr without a
            // wrapper class since it needs to be size aware
            free(out_streams[i]);
            free(out_mime_types[i]);
        }
        // free the clipboard item containers
        free(out_sizes);
        free(out_streams);
        free(out_mime_types);

        return result;
    }

    void paste(int viewId, std::string mimeType, std::string data)
    {
        doc_->setView(viewId);
        doc_->paste(mimeType.c_str(), data.c_str(), data.size());
    }

    void setGraphicSelection(int viewId, int type, int x, int y)
    {
        doc_->setView(viewId);
        doc_->setGraphicSelection(type, x, y);
    }
    void resetSelection(int viewId)
    {
        doc_->setView(viewId);
        doc_->resetSelection();
    }

    val getCommandValues(int viewId, std::string command)
    {
        doc_->setView(viewId);
        return val::u8string(doc_->getCommandValues(command.c_str()));
    }

    void subscribe(int viewId, int type)
    {
        registerCallbackOnce(viewId);
        subscribed_events_[viewId].emplace(type);
    }

    void unsubscribe(int viewId, int type) { subscribed_events_[viewId].erase(type); }

    uint32_t ref() const { return ref_; }

    int32_t getViewId() { return doc_->getView(); }

    val startTileRenderer(int32_t viewId, int32_t tileSize)
    {
        desktop::WasmDocumentExtension* ext
            = static_cast<desktop::WasmDocumentExtension*>(doc_->get());
        desktop::TileRendererData& data = ext->startTileRenderer(viewId, tileSize);
        val result = val::object();
        result.set("viewId", data.viewId);
        result.set("tileSize", data.tileSize);
        result.set("state", typed_memory_view(1, (int32_t*)&data.state));
        result.set("tileTwips", typed_memory_view(4, (uint32_t*)&data.tileTwips));
        result.set("paintedTile", typed_memory_view(data.paintedTileAllocSize, data.paintedTile));
        result.set("pendingFullPaint", typed_memory_view(1, (int32_t*)&data.pendingFullPaint));
        result.set("hasInvalidations", typed_memory_view(1, (int32_t*)&data.hasInvalidations));
        result.set("invalidationStack", typed_memory_view(4 * desktop::MAX_INVALIDATION_STACK,
                                                          (uint32_t*)&data.invalidationStack));
        result.set("invalidationStackHead",
                   typed_memory_view(1, (int32_t*)&data.invalidationStackHead));
        result.set("docWidthTwips", typed_memory_view(1, (uint32_t*)&data.docWidthTwips));

        return result;
    }

    void dispatchCommand(int viewId, std::string command, std::optional<std::string> arguments,
                         std::optional<bool> notifyWhenFinished)
    {
        doc_->setView(viewId);
        doc_->postUnoCommand(command.c_str(), arguments.has_value() ? arguments->c_str() : nullptr,
                             notifyWhenFinished.value_or(false));
    }

    void removeText(int viewId, unsigned int windowId, int charsBefore, int charsAfter)
    {
        doc_->setView(viewId);
        doc_->removeTextContext(windowId, charsBefore, charsAfter);
    }

    void setCurrentView(int viewId) { doc_->setView(viewId); }

private:
    struct DocWithId
    {
        DocumentClient* cli_;
        const int viewId_;
        DocWithId(DocumentClient* doc, int viewId)
            : cli_(doc)
            , viewId_(viewId){};
    };

    const uint32_t ref_;
    lok::Document* doc_ = nullptr;
    std::unordered_map<int, std::unordered_set<int>> subscribed_events_;
    std::unordered_map<int, bool> callback_registered_;
    bool rendering_tiles_ = false;

    static constexpr int MAX_TILE_DIM = 2048; // this is the de facto max dim for WebGL
    static constexpr int BYTES_PER_PIXEL = 4; /** RGBA **/
    std::array<uint8_t, MAX_TILE_DIM * MAX_TILE_DIM * BYTES_PER_PIXEL> tile_scratch_;

    static void handleCallback(int type, const char* payload, void* docWithId_)
    {
        DocWithId* docWithId = static_cast<DocWithId*>(docWithId_);
        DocumentClient* self = docWithId->cli_;
        // we copy to safePayload because the lifetime of the string is on the stack
        // and the call to the main thread is async
        SafeString safePayload;
        auto subscribed_events_ = self->subscribed_events_[docWithId->viewId_];

        if (subscribed_events_.find(type) != subscribed_events_.end())
        {
            if (!safePayload.first)
                safePayload = makeSafeString(payload);
            MAIN_THREAD_ASYNC_EM_ASM(
                {
                    Module.callbackHandlers.callback($0, $1, UTF8ToString($2));
                    Module.freeSafeString($3);
                },
                self->ref_, type, safePayload.second, safePayload.first);
        }
    }

    void registerCallbackOnce(int viewId)
    {
        // if the viewId is the default, use the default view because registerCallback
        // needs some view to register the callback anyway
        if (viewId < 0)
        {
            viewId = doc_->getView();
            if (viewId < 0)
            {
                SAL_WARN("wasm", "failed to get view id");
                return;
            }
        }

        if (callback_registered_[viewId])
            return;
        doc_->registerCallback(DocumentClient::handleCallback, new DocWithId(this, viewId));
        callback_registered_[viewId] = true;
    }
};

//static
void preload() { instance(); }

EMSCRIPTEN_BINDINGS(lok)
{
    register_optional<bool>();
    register_optional<std::string>();
    function("preload", &preload);
    function("freeSafeString", &freeSafeString);

    class_<DocumentClient>("Document")
        .constructor<std::string>()
        .function("valid", &DocumentClient::valid)
        .function("saveAs", &DocumentClient::saveAs)
        .function("getParts", &DocumentClient::getParts)
        .function("getPartRectangles", &DocumentClient::getPartRectangles)
        .function("paintTile", &DocumentClient::paintTile)
        .function("getDocumentSize", &DocumentClient::getDocumentSize)
        .function("initializeForRendering", &DocumentClient::initializeForRendering)
        .function("postTextInputEvent", &DocumentClient::postTextInputEvent)
        .function("postKeyEvent", &DocumentClient::postKeyEvent)
        .function("postMouseEvent", &DocumentClient::postMouseEvent)
        .function("setTextSelection", &DocumentClient::setTextSelection)
        .function("getClipboard", &DocumentClient::getClipboard)
        .function("setClipboard", &DocumentClient::setClipboard)
        .function("paste", &DocumentClient::paste)
        .function("setGraphicSelection", &DocumentClient::setGraphicSelection)
        .function("resetSelection", &DocumentClient::resetSelection)
        .function("getCommandValues", &DocumentClient::getCommandValues)
        .function("subscribe", &DocumentClient::subscribe)
        .function("unsubscribe", &DocumentClient::unsubscribe)
        .function("getViewId", &DocumentClient::getViewId)
        .function("setCurrentView", &DocumentClient::setCurrentView)
        .function("dispatchCommand", &DocumentClient::dispatchCommand)
        .function("removeText", &DocumentClient::removeText)
        .function("startTileRenderer", &DocumentClient::startTileRenderer)
        .function("ref", &DocumentClient::ref);
}
